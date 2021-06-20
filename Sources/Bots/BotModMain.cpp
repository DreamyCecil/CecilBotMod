/* Copyright (c) 2018-2021 Dreamy Cecil
This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

#include "StdH.h"
#include "BotModMain.h"
#include "BotFunctions.h"
#include "NetworkPatch/ServerIntegration.h"

// [Cecil] 2021-06-18: For weapon switching
#include "EntitiesMP/PlayerMarker.h"

// [Cecil] 2019-05-28: NavMesh Commands
extern INDEX MOD_iRenderNavMesh = 0; // NavMesh render mode (0 - disabled, 1 - points, 2 - connections, 3 - IDs, 4 - flags)
extern INDEX MOD_iNavMeshPoint = -1; // currently selected NavMesh point
extern INDEX MOD_iNavMeshConnecting = 0; // connecting mode (0 - disabled, 1 - to point, 2 - to each other, 3 - others to this one)

// [Cecil] 2021-06-19: Render entity IDs
static INDEX MOD_bEntityIDs = FALSE;

// [Cecil] 2021-06-11: List of bot entities
extern CDynamicContainer<CPlayerBot> _cenPlayerBots = CDynamicContainer<CPlayerBot>();

// [Cecil] 2019-06-02: Bot names and skins
static CStaticArray<CTString> BOT_astrNames;
static CStaticArray<CTString> BOT_astrSkins;

// [Cecil] 2018-10-15: Bot Editing
static CTString BOT_strBotEdit = ""; // Name of a bot to edit
// [Cecil] TEMP
static CTString BOT_strSpawnName = ""; // Name to spawn with

// [Cecil] 2020-07-28: A structure with bot settings
static SBotSettings _sbsBotSettings;

// [Cecil] 2019-11-07: Special client packet for NavMesh editing
static CNetworkMessage CECIL_NavMeshClientPacket(const INDEX &iAction) {
  NEW_PACKET(nmNavmesh, MSG_CECIL_SANDBOX);
  nmNavmesh << LOCAL_PLAYER_INDEX; // local player
  nmNavmesh << (INDEX)_pNetwork->IsServer(); // it's a server
  nmNavmesh << iAction; // specific NavMesh action

  return nmNavmesh;
};

// [Cecil] 2019-11-07: Special server packet for bots
static CCecilStreamBlock CECIL_BotServerPacket(const INDEX &iAction) {
  CServer &srvServer = _pNetwork->ga_srvServer;

  CCecilStreamBlock nsbBot(MSG_CECIL_SANDBOX, ++srvServer.srv_iLastProcessedSequence);
  nsbBot << LOCAL_PLAYER_INDEX; // local player
  nsbBot << (INDEX)TRUE; // it's a server
  nsbBot << iAction; // specific NavMesh action

  return nsbBot;
};

// --- Bot manipulation

// [Cecil] 2018-10-15: Config reset
static void CECIL_ResetBotConfig(void) {
  CPrintF(BOTCOM_NAME("ResetBotConfig:\n"));

  _sbsBotSettings.Reset();
  CPrintF("  Bot config has been reset!\n");
};

// [Cecil] 2018-10-09: Bot adding
static void CECIL_AddBot(CTString *pstrBotName, CTString *pstrBotSkin) {
  CTString strBotName = *pstrBotName;
  CTString strBotSkin = *pstrBotSkin;

  CPrintF(MODCOM_NAME("AddBot:\n"));

  if (!_pNetwork->IsServer()) {
    CPrintF("  <not a server>\n");
    return;
  }
  
  // pick random name and skin
  const INDEX ctNames = BOT_astrNames.Count();
  const INDEX ctSkins = BOT_astrSkins.Count();
  CTString strName = (ctNames > 0) ? BOT_astrNames[rand() % ctNames] : "Bot";
  CTString strSkin = (ctSkins > 0) ? BOT_astrSkins[rand() % ctSkins] : "SeriousSam";

  // replace random name and skin
  if (strBotName != "") {
    strName = strBotName;
  }
  if (strBotSkin != "") {
    strSkin = strBotSkin;
  }

  CPlayerCharacter pcBot;
  CPlayerSettings *pps = (CPlayerSettings *)pcBot.pc_aubAppearance;

  pps->ps_iWeaponAutoSelect = PS_WAS_NONE; // never select new weapons
  memset(pps->ps_achModelFile, 0, sizeof(pps->ps_achModelFile));
  strncpy(pps->ps_achModelFile, strSkin, sizeof(pps->ps_achModelFile));

  //pps->ps_ulFlags |= PSF_PREFER3RDPERSON; // [Cecil] TEMP

  for (INDEX iGUID = 0; iGUID < 16; iGUID++) {
    pcBot.pc_aubGUID[iGUID] = rand() % 256;
  }

  pcBot.pc_strName = strName;
  pcBot.pc_strTeam = "CECIL_BOTZ";
  //pcBot.pc_bBot = TRUE;

  // create message for adding player data to sessions
  CCecilStreamBlock nsbAddBot = CECIL_BotServerPacket(ESA_ADDBOT);
  nsbAddBot << pcBot; // character data
  nsbAddBot << _sbsBotSettings; // update bot settings

  // put the message in buffer to be sent to all servers
  CECIL_AddBlockToAllSessions(nsbAddBot);
};

static void CECIL_QuickBot(void) {
  CTString strName = BOT_strSpawnName;
  CTString strSkin = "";
  CECIL_AddBot(&strName, &strSkin);
};

// [Cecil] 2018-10-14: Bot removing
static void CECIL_RemoveAllBots(void) {
  CPrintF(MODCOM_NAME("RemoveAllBots:\n"));

  if (!_pNetwork->IsServer()) {
    CPrintF("  <not a server>\n");
    return;
  }

  INDEX iBot = 0;
  BOOL bRemoved = FALSE;

  for (iBot = 0; iBot < _cenPlayerBots.Count(); iBot++) {
    CPlayerBot *penBot = _cenPlayerBots.Pointer(iBot);

    if (!ASSERT_ENTITY(penBot)) {
      continue;
    }

    // create message for removing player from all session
    CCecilStreamBlock nsbRemPlayerData = CECIL_BotServerPacket(ESA_REMBOT);
    nsbRemPlayerData << iBot; // bot index

    // put the message in buffer to be sent to all sessions
    CECIL_AddBlockToAllSessions(nsbRemPlayerData);

    bRemoved = TRUE;
  }

  if (!bRemoved) {
    CPrintF("  <no bots in the current session>\n");
  } else {
    CPrintF("  <removed %d bots>\n", iBot);
  }
}

static void CECIL_RemoveBot(CTString *pstrBotName) {
  CPrintF(MODCOM_NAME("RemoveBot:\n"));

  if (!_pNetwork->IsServer()) {
    CPrintF("  <not a server>\n");
    return;
  }

  CTString strBotName = *pstrBotName;
  BOOL bRemoved = FALSE;

  for (INDEX iBot = 0; iBot < _cenPlayerBots.Count(); iBot++) {
    CPlayerBot *penBot = _cenPlayerBots.Pointer(iBot);

    if (!ASSERT_ENTITY(penBot)) {
      continue;
    }

    // if bot name matches
    if (penBot->GetName().Undecorated().Matches(strBotName)) {
      // create message for removing player from all session
      CCecilStreamBlock nsbRemPlayerData = CECIL_BotServerPacket(ESA_REMBOT);
      nsbRemPlayerData << iBot; // bot index

      // put the message in buffer to be sent to all sessions
      CECIL_AddBlockToAllSessions(nsbRemPlayerData);

      bRemoved = TRUE;
    }
  }

  if (!bRemoved) {
    CPrintF("  <no bots with the name '%s'>\n", strBotName);
  } else {
    CPrintF("  <removed bots with the name '%s'>\n", strBotName);
  }
};

// [Cecil] 2018-10-15: Bot updating
static void CECIL_BotUpdate(void) {
  CPrintF(MODCOM_NAME("BotUpdate:\n"));

  if (!_pNetwork->IsServer()) {
    CPrintF("  <not a server>\n");
    return;
  }

  CCecilStreamBlock nsbBotUpdate = CECIL_BotServerPacket(ESA_UPDATEBOT);
  nsbBotUpdate << BOT_strBotEdit; // Bot name
  nsbBotUpdate << _sbsBotSettings; // Bot settings

  // put the message in buffer to be sent to all sessions
  CECIL_AddBlockToAllSessions(nsbBotUpdate);
};

// [Cecil] 2021-06-18: Change all weapons
static void CECIL_SetWeapons(INDEX iWeapon, INDEX bPlayer) {
  CPrintF(MODCOM_NAME("SetWeapons:\n"));

  if (!_pNetwork->IsServer()) {
    CPrintF("  <not a server>\n");
    return;
  }

  CCecilStreamBlock nsbSetWeapons = CECIL_BotServerPacket(ESA_SETWEAPONS);
  nsbSetWeapons << iWeapon; // weapon type
  nsbSetWeapons << (UBYTE)bPlayer; // player weapons

  // put the message in buffer to be sent to all sessions
  CECIL_AddBlockToAllSessions(nsbSetWeapons);
};

// --- Navmesh creation

// [Cecil] 2018-11-10: Quick Function For NavMeshGenerator
static void CECIL_GenerateNavMesh(INDEX iPoints) {
  CPrintF(MODCOM_NAME("GenerateNavMesh:\n"));

  if (!_pNetwork->IsServer()) {
    CPrintF("  <not a server>\n");
    return;
  }

  CCecilStreamBlock nsbNavMesh = CECIL_BotServerPacket(ESA_NAVMESH_GEN);
  nsbNavMesh << Clamp(iPoints, 0L, 1L); // Connect points or generate them

  // put the message in buffer to be sent to all sessions
  CECIL_AddBlockToAllSessions(nsbNavMesh);
};

// [Cecil] 2019-05-28: Quick functions for NavMesh states
static void CECIL_NavMeshSave(void) {
  CPrintF(MODCOM_NAME("NavMeshSave:\n"));

  if (!_pNetwork->IsServer()) {
    CPrintF("  <not a server>\n");
    return;
  }

  CCecilStreamBlock nsbNavMesh = CECIL_BotServerPacket(ESA_NAVMESH_STATE);
  nsbNavMesh << (INDEX)0; // Save state

  // put the message in buffer to be sent to all sessions
  CECIL_AddBlockToAllSessions(nsbNavMesh);
};

static void CECIL_NavMeshLoad(void) {
  CPrintF(MODCOM_NAME("NavMeshLoad:\n"));

  if (!_pNetwork->IsServer()) {
    CPrintF("  <not a server>\n");
    return;
  }
  
  CCecilStreamBlock nsbNavMesh = CECIL_BotServerPacket(ESA_NAVMESH_STATE);
  nsbNavMesh << (INDEX)1; // Load state

  // put the message in buffer to be sent to all sessions
  CECIL_AddBlockToAllSessions(nsbNavMesh);
};

// [Cecil] 2021-06-16: Quick function for NavMesh clearing
static void CECIL_NavMeshClear(INDEX iPoints) {
  CPrintF(MODCOM_NAME("NavMeshClear:\n"));

  if (!_pNetwork->IsServer()) {
    CPrintF("  <not a server>\n");
    return;
  }
  
  // put the message in buffer to be sent to all sessions
  CCecilStreamBlock nsbNavMesh = CECIL_BotServerPacket(ESA_NAVMESH_CLEAR);
  CECIL_AddBlockToAllSessions(nsbNavMesh);
};

// --- Navmesh editing

// [Cecil] 2019-05-28: Add new NavMesh point with vertical offset
static void CECIL_AddNavMeshPoint(FLOAT fOffset) {
  CNetworkMessage nmNavmesh = CECIL_NavMeshClientPacket(ESA_NAVMESH_CREATE);
  nmNavmesh << MOD_iNavMeshPoint;
  nmNavmesh << MOD_iNavMeshConnecting;
  nmNavmesh << fOffset;

  _pNetwork->SendToServerReliable(nmNavmesh);
};

// [Cecil] 2019-05-28: Delete NavMesh point
static void CECIL_DeleteNavMeshPoint(void) {
  CNetworkMessage nmNavmesh = CECIL_NavMeshClientPacket(ESA_NAVMESH_DELETE);
  nmNavmesh << MOD_iNavMeshPoint;

  _pNetwork->SendToServerReliable(nmNavmesh);
};

// [Cecil] 2019-05-28: Display NavMesh point info
static void CECIL_NavMeshPointInfo(void) {
  CPrintF(MODCOM_NAME("NavMeshPointInfo: "));

  CBotPathPoint *pbpp = _pNavmesh->FindPointByID(MOD_iNavMeshPoint);

  if (pbpp == NULL) {
    CPrintF("NavMesh point doesn't exist!\n");
    return;
  }

  CPrintF("^cffffff%d\n", pbpp->bpp_iIndex); // point index on top

  CEntity *penImportant = FindEntityByID(&_pNetwork->ga_World, pbpp->bpp_iImportant);
  
  CPrintF("Connections: %d\n", pbpp->bpp_cbppPoints.Count());
  CPrintF("Pos:    %.2f, %.2f, %.2f\n", pbpp->bpp_vPos(1), pbpp->bpp_vPos(2), pbpp->bpp_vPos(3));
  CPrintF("Range:  %.2f\n", pbpp->bpp_fRange);
  CPrintF("Flags:  %s\n", ULongToBinary(pbpp->bpp_ulFlags));
  CPrintF("Entity: %s\n", (penImportant == NULL) ? "<none>" : penImportant->GetName());
};

// [Cecil] 2021-06-12: Connect current NavMesh point to another one
static void CECIL_ConnectNavMeshPoint(INDEX iTargetPoint) {
  CNetworkMessage nmNavmesh = CECIL_NavMeshClientPacket(ESA_NAVMESH_CONNECT);
  nmNavmesh << MOD_iNavMeshPoint;
  nmNavmesh << iTargetPoint;
  nmNavmesh << UBYTE(MOD_iNavMeshConnecting);

  _pNetwork->SendToServerReliable(nmNavmesh);
};

// [Cecil] 2021-06-18: Move NavMesh point to the player position
static void CECIL_TeleportNavMeshPoint(FLOAT fOffset) {
  CNetworkMessage nmNavmesh = CECIL_NavMeshClientPacket(ESA_NAVMESH_TELEPORT);
  nmNavmesh << MOD_iNavMeshPoint;
  nmNavmesh << fOffset;

  _pNetwork->SendToServerReliable(nmNavmesh);
};

// [Cecil] 2021-06-18: Change NavMesh point absolute position
static void CECIL_NavMeshPointPos(FLOAT fX, FLOAT fY, FLOAT fZ) {
  CNetworkMessage nmNavmesh = CECIL_NavMeshClientPacket(ESA_NAVMESH_POS);
  nmNavmesh << MOD_iNavMeshPoint;
  nmNavmesh << fX << fY << fZ;

  _pNetwork->SendToServerReliable(nmNavmesh);
};

// [Cecil] 2021-06-18: Snap NavMesh point position to a custom-sized grid
static void CECIL_SnapNavMeshPoint(FLOAT fGridSize) {
  CNetworkMessage nmNavmesh = CECIL_NavMeshClientPacket(ESA_NAVMESH_SNAP);
  nmNavmesh << MOD_iNavMeshPoint;
  nmNavmesh << fGridSize;

  _pNetwork->SendToServerReliable(nmNavmesh);
};

// [Cecil] 2019-06-04: Change NavMesh point flags
static void CECIL_NavMeshPointFlags(INDEX iFlags) {
  CNetworkMessage nmNavmesh = CECIL_NavMeshClientPacket(ESA_NAVMESH_FLAGS);
  nmNavmesh << MOD_iNavMeshPoint;
  nmNavmesh << iFlags;

  _pNetwork->SendToServerReliable(nmNavmesh);
};

// [Cecil] 2019-06-05: Change NavMesh point important entity
static void CECIL_NavMeshPointEntity(INDEX iEntityID) {
  CNetworkMessage nmNavmesh = CECIL_NavMeshClientPacket(ESA_NAVMESH_ENTITY);
  nmNavmesh << MOD_iNavMeshPoint;
  nmNavmesh << iEntityID;

  _pNetwork->SendToServerReliable(nmNavmesh);
};

// [Cecil] 2019-06-06: Change NavMesh point range
static void CECIL_NavMeshPointRange(FLOAT fRange) {
  CNetworkMessage nmNavmesh = CECIL_NavMeshClientPacket(ESA_NAVMESH_RANGE);
  nmNavmesh << MOD_iNavMeshPoint;
  nmNavmesh << fRange;

  _pNetwork->SendToServerReliable(nmNavmesh);
};

// [Cecil] 2019-05-28: NavMesh Point Selection
static void CECIL_NavMeshSelectPoint(void) {
  if (LOCAL_PLAYER_INDEX == -1) {
    return;
  }

  CPlayer *pen = (CPlayer *)CEntity::GetPlayerEntity(LOCAL_PLAYER_INDEX);
  CBotPathPoint *pbppNearest = NearestNavMeshPoint(NULL, pen->GetPlayerWeapons()->m_vRayHit, NULL);
      
  // point target selection
  CBotPathPoint *pbppConnect = _pNavmesh->FindPointByID(MOD_iNavMeshPoint);
      
  if (MOD_iNavMeshConnecting > 0 && pbppConnect != NULL) {
    CECIL_ConnectNavMeshPoint(pbppNearest->bpp_iIndex);
        
  } else {
    MOD_iNavMeshPoint = pbppNearest->bpp_iIndex;
  }
};

// [Cecil] 2021-06-13: Change NavMesh connection type
static void CECIL_NavMeshConnectionType(void) {
  MOD_iNavMeshConnecting = (MOD_iNavMeshConnecting + 1) % 4;

  CPrintF(MODCOM_NAME("iNavMeshConnecting = %d "), MOD_iNavMeshConnecting);

  switch (MOD_iNavMeshConnecting) {
    case 0: CPrintF("(disabled)\n"); break;
    case 1: CPrintF("(one-way connection)\n"); break;
    case 2: CPrintF("(two-way connection)\n"); break;
    case 3: CPrintF("(one-way backwards connection)\n"); break;
    default: CPrintF("(other)\n");
  }
};

// [Cecil] 2021-06-12: Initialized bot mod
static BOOL _bBotModInit = FALSE;

// [Cecil] 2019-06-01: Initialize the bot mod
void CECIL_InitBotMod(void) {
  // mark as initialized
  if (_bBotModInit) {
    return;
  }

  _bBotModInit = TRUE;

  // [Cecil] 2021-06-12: Create Bot NavMesh
  _pNavmesh = new CBotNavmesh();

  // [Cecil] 2021-06-11: Apply networking patch
  extern void CECIL_ApplyNetworkPatches(void);
  CECIL_ApplyNetworkPatches();

  // [Cecil] Bot mod
  _pShell->DeclareSymbol("user void " MODCOM_NAME("QuickBot(void);"), &CECIL_QuickBot);
  _pShell->DeclareSymbol("user void " MODCOM_NAME("AddBot(CTString, CTString);"), &CECIL_AddBot);
  _pShell->DeclareSymbol("user void " MODCOM_NAME("RemoveBot(CTString);"), &CECIL_RemoveBot);
  _pShell->DeclareSymbol("user void " MODCOM_NAME("RemoveAllBots(void);"), &CECIL_RemoveAllBots);
  _pShell->DeclareSymbol("user void " MODCOM_NAME("BotUpdate(void);"), &CECIL_BotUpdate);

  _pShell->DeclareSymbol("user void " MODCOM_NAME("GenerateNavMesh(INDEX);"), &CECIL_GenerateNavMesh);
  _pShell->DeclareSymbol("user void " MODCOM_NAME("NavMeshSave(void);"), &CECIL_NavMeshSave);
  _pShell->DeclareSymbol("user void " MODCOM_NAME("NavMeshLoad(void);"), &CECIL_NavMeshLoad);
  _pShell->DeclareSymbol("user void " MODCOM_NAME("NavMeshClear(void);"), &CECIL_NavMeshClear);

  // [Cecil] Misc
  _pShell->DeclareSymbol("user INDEX " MODCOM_NAME("bEntityIDs;"), &MOD_bEntityIDs);

  _pShell->DeclareSymbol("user void " MODCOM_NAME("SetWeapons(INDEX, INDEX);"), &CECIL_SetWeapons);

  // [Cecil] Bot editing
  _pShell->DeclareSymbol("user CTString " BOTCOM_NAME("strBotEdit;"), &BOT_strBotEdit);
  _pShell->DeclareSymbol("persistent user CTString " BOTCOM_NAME("strSpawnName;"), &BOT_strSpawnName); // [Cecil] TEMP

  _pShell->DeclareSymbol("user void " BOTCOM_NAME("ResetBotConfig(void);"), &CECIL_ResetBotConfig);
  _pShell->DeclareSymbol("persistent user INDEX " BOTCOM_NAME("b3rdPerson;"     ), &_sbsBotSettings.b3rdPerson);
  _pShell->DeclareSymbol("persistent user INDEX " BOTCOM_NAME("iCrosshair;"     ), &_sbsBotSettings.iCrosshair);

  _pShell->DeclareSymbol("persistent user INDEX " BOTCOM_NAME("bSniperZoom;"    ), &_sbsBotSettings.bSniperZoom);
  _pShell->DeclareSymbol("persistent user INDEX " BOTCOM_NAME("bShooting;"      ), &_sbsBotSettings.bShooting);
  _pShell->DeclareSymbol("persistent user FLOAT " BOTCOM_NAME("fShootAngle;"    ), &_sbsBotSettings.fShootAngle);
  _pShell->DeclareSymbol("persistent user FLOAT " BOTCOM_NAME("fAccuracyAngle;" ), &_sbsBotSettings.fAccuracyAngle);

  _pShell->DeclareSymbol("persistent user FLOAT " BOTCOM_NAME("fRotSpeedDist;"  ), &_sbsBotSettings.fRotSpeedDist);
  _pShell->DeclareSymbol("persistent user FLOAT " BOTCOM_NAME("fRotSpeedMin;"   ), &_sbsBotSettings.fRotSpeedMin);
  _pShell->DeclareSymbol("persistent user FLOAT " BOTCOM_NAME("fRotSpeedMax;"   ), &_sbsBotSettings.fRotSpeedMax);
  _pShell->DeclareSymbol("persistent user FLOAT " BOTCOM_NAME("fRotSpeedLimit;" ), &_sbsBotSettings.fRotSpeedLimit);

  _pShell->DeclareSymbol("persistent user FLOAT " BOTCOM_NAME("fWeaponCD;"      ), &_sbsBotSettings.fWeaponCD);
  _pShell->DeclareSymbol("persistent user FLOAT " BOTCOM_NAME("fTargetCD;"      ), &_sbsBotSettings.fTargetCD);

  _pShell->DeclareSymbol("persistent user FLOAT " BOTCOM_NAME("fSpeedMul;"      ), &_sbsBotSettings.fSpeedMul);
  _pShell->DeclareSymbol("persistent user INDEX " BOTCOM_NAME("bStrafe;"        ), &_sbsBotSettings.bStrafe);
  _pShell->DeclareSymbol("persistent user INDEX " BOTCOM_NAME("bJump;"          ), &_sbsBotSettings.bJump);

  _pShell->DeclareSymbol("persistent user FLOAT " BOTCOM_NAME("fPrediction;"    ), &_sbsBotSettings.fPrediction);
  _pShell->DeclareSymbol("persistent user FLOAT " BOTCOM_NAME("fPredictRnd;"    ), &_sbsBotSettings.fPredictRnd);

  _pShell->DeclareSymbol("persistent user INDEX " BOTCOM_NAME("iAllowedWeapons;"), &_sbsBotSettings.iAllowedWeapons);
  _pShell->DeclareSymbol("persistent user INDEX " BOTCOM_NAME("iTargetType;"    ), &_sbsBotSettings.iTargetType);
  _pShell->DeclareSymbol("persistent user INDEX " BOTCOM_NAME("bTargetSearch;"  ), &_sbsBotSettings.bTargetSearch);
  _pShell->DeclareSymbol("persistent user INDEX " BOTCOM_NAME("bItemSearch;"    ), &_sbsBotSettings.bItemSearch);
  _pShell->DeclareSymbol("persistent user FLOAT " BOTCOM_NAME("fItemSearchCD;"  ), &_sbsBotSettings.fItemSearchCD);

  _pShell->DeclareSymbol("persistent user INDEX " BOTCOM_NAME("bItemVisibility;"), &_sbsBotSettings.bItemVisibility);
  _pShell->DeclareSymbol("persistent user FLOAT " BOTCOM_NAME("fWeaponDist;"    ), &_sbsBotSettings.fWeaponDist);
  _pShell->DeclareSymbol("persistent user FLOAT " BOTCOM_NAME("fHealthSearch;"  ), &_sbsBotSettings.fHealthSearch);
  _pShell->DeclareSymbol("persistent user FLOAT " BOTCOM_NAME("fHealthDist;"    ), &_sbsBotSettings.fHealthDist);
  _pShell->DeclareSymbol("persistent user FLOAT " BOTCOM_NAME("fArmorDist;"     ), &_sbsBotSettings.fArmorDist);
  _pShell->DeclareSymbol("persistent user FLOAT " BOTCOM_NAME("fAmmoDist;"      ), &_sbsBotSettings.fAmmoDist);

  // [Cecil] NavMesh editing
  _pShell->DeclareSymbol("persistent user INDEX " MODCOM_NAME("iRenderNavMesh;"), &MOD_iRenderNavMesh);
  _pShell->DeclareSymbol("user INDEX " MODCOM_NAME("iNavMeshPoint;"), &MOD_iNavMeshPoint);
  _pShell->DeclareSymbol("user INDEX " MODCOM_NAME("iNavMeshConnecting;"), &MOD_iNavMeshConnecting);

  _pShell->DeclareSymbol("user void " MODCOM_NAME("AddNavMeshPoint(FLOAT);"), &CECIL_AddNavMeshPoint);
  _pShell->DeclareSymbol("user void " MODCOM_NAME("DeleteNavMeshPoint(void);"), &CECIL_DeleteNavMeshPoint);
  _pShell->DeclareSymbol("user void " MODCOM_NAME("NavMeshPointInfo(void);"), &CECIL_NavMeshPointInfo);
  _pShell->DeclareSymbol("user void " MODCOM_NAME("ConnectNavMeshPoint(INDEX);"), &CECIL_ConnectNavMeshPoint);
  _pShell->DeclareSymbol("user void " MODCOM_NAME("TeleportNavMeshPoint(FLOAT);"), &CECIL_TeleportNavMeshPoint);

  _pShell->DeclareSymbol("user void " MODCOM_NAME("NavMeshPointPos(FLOAT, FLOAT, FLOAT);"), &CECIL_NavMeshPointPos);
  _pShell->DeclareSymbol("user void " MODCOM_NAME("SnapNavMeshPoint(FLOAT);"), &CECIL_SnapNavMeshPoint);
  _pShell->DeclareSymbol("user void " MODCOM_NAME("NavMeshPointFlags(INDEX);"), &CECIL_NavMeshPointFlags);
  _pShell->DeclareSymbol("user void " MODCOM_NAME("NavMeshPointEntity(INDEX);"), &CECIL_NavMeshPointEntity);
  _pShell->DeclareSymbol("user void " MODCOM_NAME("NavMeshPointRange(FLOAT);"), &CECIL_NavMeshPointRange);

  _pShell->DeclareSymbol("user void " MODCOM_NAME("NavMeshSelectPoint(void);"), &CECIL_NavMeshSelectPoint);
  _pShell->DeclareSymbol("user void " MODCOM_NAME("NavMeshConnectionType(void);"), &CECIL_NavMeshConnectionType);

  // load bot names
  try {
    CTFileStream strmNames;
    strmNames.Open_t(CTFILENAME("Cecil\\Bots\\BotNames.txt"));

    // clear names list
    BOT_astrNames.Clear();

    // fill names array
    while (!strmNames.AtEOF()) {
      CTString str = "";
      strmNames.GetLine_t(str);

      INDEX ctNames = BOT_astrNames.Count();
      BOT_astrNames.Expand(ctNames + 1);
      BOT_astrNames[ctNames] = str;
    }

    strmNames.Close();
  } catch (char *strError) {
    CPrintF("%s\n", strError);
  }

  // load bot skins
  try {
    CTFileStream strmSkins;
    strmSkins.Open_t(CTFILENAME("Cecil\\Bots\\BotSkins.txt"));

    // clear skins list
    BOT_astrSkins.Clear();

    // fill skins array
    while (!strmSkins.AtEOF()) {
      CTString str = "";
      strmSkins.GetLine_t(str);

      INDEX ctSkins = BOT_astrSkins.Count();
      BOT_astrSkins.Expand(ctSkins + 1);
      BOT_astrSkins[ctSkins] = str;
    }

    strmSkins.Close();
  } catch (char *strError) {
    CPrintF("%s\n", strError);
  }
};

// [Cecil] 2021-06-13: End the bot mod
void CECIL_EndBotMod(void) {
  // [Cecil] 2021-06-12: Destroy Bot NavMesh
  if (_pNavmesh != NULL) {
    delete _pNavmesh;
    _pNavmesh = NULL;
  }
};

// [Cecil] 2021-06-13: Bot game start
void CECIL_BotGameStart(CSessionProperties &sp) {
  CWorld &wo = _pNetwork->ga_World;

  // [Cecil] 2021-06-12: Global bot mod entity
  CPlacement3D plEntity = CPlacement3D(FLOAT3D(0.0f, 0.0f, 0.0f), ANGLE3D(0.0f, 0.0f, 0.0f));
  CEntity *penNew = wo.CreateEntity_t(plEntity, CTFILENAME("Classes\\BotModGlobal.ecl"));
  penNew->Initialize();

  // [Cecil] 2021-06-13: Load NavMesh for a map
  try {
    _pNavmesh->Load(wo);

  } catch (char *strError) {
    (void)strError;

    // [Cecil] TEMP: No point in generating unconnected points
    // generate instead
    //_pNavmesh->GenerateNavmesh(&wo);
    //_pNavmesh->bnm_bGenerated = TRUE;
  }
};

// [Cecil] 2021-06-12: Bot game cleanup
void CECIL_BotGameCleanup(void) {
  // [Cecil] 2018-10-23: Clear the NavMesh
  _pNavmesh->ClearNavMesh();

  // [Cecil] 2021-06-12: Clear bot list
  _cenPlayerBots.Clear();
};

// [Cecil] Render extras on top of the world
void CECIL_WorldOverlayRender(CPlayer *penOwner, CEntity *penViewer, CAnyProjection3D &apr, CDrawPort *pdp) {
  // not a server
  if (!_pNetwork->IsServer()) {
    return;
  }

  CPerspectiveProjection3D &prProjection = *(CPerspectiveProjection3D *)(CProjection3D *)apr;
  prProjection.Prepare();

  pdp->SetFont(_pfdConsoleFont);
  pdp->SetTextScaling(1.0f);

  // [Cecil] TEMP: NavMesh Rendering
  if (MOD_iRenderNavMesh > 0) {
    if (_pNavmesh->bnm_cbppPoints.Count() > 0)
    {
      const BOOL bConnections = (MOD_iRenderNavMesh > 1);
      const BOOL bIDs = (MOD_iRenderNavMesh > 2);
      const BOOL bFlags = (MOD_iRenderNavMesh > 3);

      const CBotPathPoint *pbppClosest = NearestNavMeshPoint(penOwner, penOwner->GetPlayerWeapons()->m_vRayHit, NULL);

      FOREACHINDYNAMICCONTAINER(_pNavmesh->bnm_cbppPoints, CBotPathPoint, itbpp) {
        CBotPathPoint *pbpp = itbpp;
        INDEX iPointID = pbpp->bpp_iIndex;
        
        FLOAT3D vPointOnScreen;
        FLOAT3D vPoint1 = pbpp->bpp_vPos;
        prProjection.ProjectCoordinate(pbpp->bpp_vPos, vPointOnScreen);

        if (vPointOnScreen(3) > 0.0f) {
          continue;
        }

        vPointOnScreen(2) = -vPointOnScreen(2) + pdp->GetHeight();

        // highlight closest point for selection
        BOOL bClosestPoint = (pbppClosest == pbpp);
        CBotPathPoint *pbppSelected = _pNavmesh->FindPointByID(MOD_iNavMeshPoint);

        // range and connections
        if (bConnections) {
          // range
          for (INDEX iRange = 0; iRange < 4; iRange++) {
            FLOAT3D vRangeDir = FLOAT3D(CosFast(iRange * 45.0f), 0.0f, SinFast(iRange * 45.0f)) * pbpp->bpp_fRange;
            
            FLOAT3D vRangeEnd1, vRangeEnd2;
            prProjection.ProjectCoordinate(vPoint1 + vRangeDir, vRangeEnd1);
            prProjection.ProjectCoordinate(vPoint1 - vRangeDir, vRangeEnd2);

            if (vRangeEnd1(3) > 0.0f || vRangeEnd2(3) > 0.0f) {
              continue;
            }

            // range is too small for rendering
            if ((vRangeEnd1 - vRangeEnd2).Length() < 12.0f) {
              continue;
            }
            
            vRangeEnd1(2) = -vRangeEnd1(2) + pdp->GetHeight();
            vRangeEnd2(2) = -vRangeEnd2(2) + pdp->GetHeight();

            pdp->DrawLine(vRangeEnd1(1), vRangeEnd1(2), vRangeEnd2(1), vRangeEnd2(2), 0x00FFFF9F);
          }

          // connections
          INDEX ctTargets = 0;

          {FOREACHINDYNAMICCONTAINER(pbpp->bpp_cbppPoints, CBotPathPoint, itbppT) {
            CBotPathPoint *pbppT = itbppT;
            ctTargets++;
          
            FLOAT3D vOnScreen1, vOnScreen2;
            FLOAT3D vPoint2 = pbppT->bpp_vPos;

            // [Cecil] TEMP: Show direction to the target
            /*FLOAT3D vToTarget = (vPoint2 - pbpp->bpp_vPos).Normalize();
            vToTarget *= ClampUp(4.0f, (vPoint2 - pbpp->bpp_vPos).Length());
            vPoint2 = pbpp->bpp_vPos + vToTarget;*/
          
            if (ProjectLine(&prProjection, vPoint1, vPoint2, vOnScreen1, vOnScreen2)) {
              pdp->DrawLine(vOnScreen1(1), vOnScreen1(2), vOnScreen2(1), vOnScreen2(2), C_ORANGE|0x7F);
            }
          }}

          // connect with important entity
          if (pbpp->bpp_iImportant != -1) {
            CEntity *penImportant = FindEntityByID(penOwner->GetWorld(), pbpp->bpp_iImportant);
            FLOAT3D vEntity = penImportant->GetPlacement().pl_PositionVector;

            if (penImportant != NULL) {
              FLOAT3D vOnScreen1, vEntityOnScreen;

              if (ProjectLine(&prProjection, vPoint1, vEntity, vOnScreen1, vEntityOnScreen)) {
                pdp->DrawLine(vOnScreen1(1), vOnScreen1(2), vEntityOnScreen(1), vEntityOnScreen(2), 0x00FF00FF);
              }
            }
          }
        }
        
        // selected point
        if (pbppSelected == pbpp) {
          pdp->DrawPoint(vPointOnScreen(1), vPointOnScreen(2), 0xFF0000FF, 10);

        // point for selection
        } else if (bClosestPoint) {
          pdp->DrawPoint(vPointOnScreen(1), vPointOnScreen(2), 0x009900FF, 10);

        // normal point
        } else {
          pdp->DrawPoint(vPointOnScreen(1), vPointOnScreen(2), 0xFFFF00FF, 5);
        }

        // point IDs
        if (bIDs) {
          CTString strPoint;
          strPoint.PrintF("ID: %d", iPointID);

          // point flags
          if (bFlags) {
            #define POINT_DESC(_Type) strPoint += ((pbpp->bpp_ulFlags & PPF_##_Type) ? "\n " #_Type : "")

            POINT_DESC(WALK);
            POINT_DESC(JUMP);
            POINT_DESC(CROUCH);
            POINT_DESC(OVERRIDE);
            POINT_DESC(UNREACHABLE);
            POINT_DESC(TELEPORT);

            #undef POINT_DESC
          }

          pdp->PutTextC(strPoint, vPointOnScreen(1), vPointOnScreen(2) + 16, 0xFFFFFFFF);
        }
      }
    }

    // [Cecil] TEMP 2019-06-04: Render bots' target points
    for (INDEX iBot = 0; iBot < _cenPlayerBots.Count(); iBot++) {
      CPlayerBot *penBot = _cenPlayerBots.Pointer(iBot);

      if (!ASSERT_ENTITY(penBot)) {
        continue;
      }
      
      FLOAT3D vPos1, vPos2;
      FLOAT3D vBot = penBot->GetLerpedPlacement().pl_PositionVector;
      FLOAT3D vCurPoint = FLOAT3D(0.0f, 0.0f, 0.0f);

      // current point
      if (penBot->m_pbppCurrent != NULL) {
        vCurPoint = penBot->m_pbppCurrent->bpp_vPos;

        if (ProjectLine(&prProjection, vBot, vCurPoint, vPos1, vPos2)) {
          pdp->DrawLine(vPos1(1), vPos1(2), vPos2(1), vPos2(2), 0xFFFF00FF);
        }
      }

      // target point
      if (penBot->m_pbppTarget != NULL) {
        vCurPoint = penBot->m_pbppTarget->bpp_vPos;
          
        if (ProjectLine(&prProjection, vBot, vCurPoint, vPos1, vPos2)) {
          pdp->DrawLine(vPos1(1), vPos1(2), vPos2(1), vPos2(2), 0xFF0000FF);
        }
      }
    }
  }

  // render entity IDs
  if (MOD_bEntityIDs) {
    FOREACHINDYNAMICCONTAINER(_pNetwork->ga_World.wo_cenEntities, CEntity, iten) {
      CEntity *pen = iten;

      FLOAT fDist = (penViewer->GetPlacement().pl_PositionVector - pen->GetLerpedPlacement().pl_PositionVector).Length();

      // don't render IDs from the whole map
      if (fDist > 192.0f) {
        continue;
      }
      
      FLOAT3D vEntityID;
      prProjection.ProjectCoordinate(pen->GetLerpedPlacement().pl_PositionVector, vEntityID);

      if (vEntityID(3) >= 0.0f) {
        continue;
      }

      FLOAT fAlpha = 1.0f - Clamp((fDist - 16.0f) / 16.0f, 0.0f, 0.8f);
      UBYTE ubAlpha = NormFloatToByte(fAlpha);

      CTString strID;
      strID.PrintF("%d (%s)", pen->en_ulID, pen->en_pecClass->ec_pdecDLLClass->dec_strName);
      pdp->PutTextCXY(strID, vEntityID(1), -vEntityID(2)+pdp->GetHeight(), 0xBBD1EB00|ubAlpha);
    }
  }
};

// [Cecil] Render extras on top of the HUD
void CECIL_HUDOverlayRender(CPlayer *penOwner, CEntity *penViewer, CAnyProjection3D &apr, CDrawPort *pdp) {
  FLOAT fScaling = (FLOAT)pdp->GetHeight() / 480.0f;

  // [Cecil] TEMP 2021-06-20: Bot thoughts
  if (penOwner->IsBot()) {
    pdp->SetFont(_pfdDisplayFont);
    pdp->SetTextScaling(fScaling);
    pdp->SetTextAspect(1.0f);

    CPlayerBot *penBot = (CPlayerBot *)penOwner;
    
    PIX pixX = 16 * fScaling;
    PIX pixY = 56 * fScaling;
    PIX pixThought = 18 * fScaling;

    for (INDEX iThought = 0; iThought < 16; iThought++) {
      UBYTE ubAlpha = NormFloatToByte(1.0f - iThought / 50.0f);
      COLOR colText = LerpColor(0xFFFFFF00, 0x7F7F7F00, iThought / 15.0f) | ubAlpha;

      pdp->PutText(penBot->m_btThoughts.strThoughts[iThought], pixX, pixY + iThought*pixThought, colText);
    }

    // target point
    if (penBot->m_pbppCurrent != NULL) {
      CTString strTarget(0, "Target: %d ^caf3f3f%s", penBot->m_pbppCurrent->bpp_iIndex, penBot->m_bImportantPoint ? "(Important)" : "");
      pdp->PutText(strTarget, pixX, pixY + pixThought*17, 0xCCCCCCFF);
    }

    CTString strTime(0, "Current: %.2f\nShooting: %.2f", _pTimer->CurrentTick(), penBot->m_tmShootTime);
    pdp->PutText(strTime, pixX, pixY + pixThought*18, 0xCCCCCCFF);
  }
};

// Receive and perform a sandbox action
void CECIL_SandboxAction(CPlayer *pen, const INDEX &iAction, const BOOL &bAdmin, CNetworkMessage &nmMessage) {
  BOOL bLocal = _pNetwork->IsPlayerLocal(pen);
  CWorld &wo = _pNetwork->ga_World;

  switch (iAction) {
    // [Cecil] 2019-06-03: Add a new bot to the game
    case ESA_ADDBOT: {
      CPlayerCharacter pcBot;
      nmMessage >> pcBot; // player character

      SBotSettings sbsSettings;
      nmMessage >> sbsSettings;

      // delete all predictors
      wo.DeletePredictors();

      // if there is no entity with that character in the world
      CPlayerBot *penNewBot = (CPlayerBot *)wo.FindEntityWithCharacter(pcBot);

      if (penNewBot == NULL) {
        // create an entity for it
        CPlacement3D pl(FLOAT3D(0.0f, 0.0f, 0.0f), ANGLE3D(0.0f, 0.0f, 0.0f));

        try {
          CTFileName fnmPlayer = CTString("Classes\\PlayerBot.ecl");
          penNewBot = (CPlayerBot *)wo.CreateEntity_t(pl, fnmPlayer);

          // attach the character to it
          penNewBot->en_pcCharacter = pcBot;

          // update settings and initialize
          penNewBot->UpdateBot(sbsSettings);
          penNewBot->Initialize();

        } catch (char *strError) {
          FatalError(TRANS("Cannot load PlayerBot class:\n%s"), strError);
        }

        CPrintF(TRANS("Added bot '%s^r'\n"), penNewBot->GetPlayerName());

      } else {
        CPrintF(TRANS("Player entity with the given character already exists!\n"));
      }
    } break;

    // [Cecil] 2021-06-12: Remove the bot
    case ESA_REMBOT: {
      INDEX iBot;
      nmMessage >> iBot; // bot index

      CPlayerBot *penBot = _cenPlayerBots.Pointer(iBot);

      // delete all predictors
      wo.DeletePredictors();

      // inform entity of disconnnection
      CPrintF(TRANS("Removed %s\n"), penBot->GetPlayerName());
      penBot->Disconnect();
    } break;

    // Bot updating
    case ESA_UPDATEBOT: {
      CTString strBotEdit;
      nmMessage >> strBotEdit;

      SBotSettings sbsSettings;
      nmMessage >> sbsSettings;

      for (INDEX iBot = 0; iBot < _cenPlayerBots.Count(); iBot++) {
        CPlayerBot *penBot = _cenPlayerBots.Pointer(iBot);

        // for only one specific bot or all bots
        if (strBotEdit == "" || penBot->GetName().Undecorated().Matches(strBotEdit)) {
          penBot->UpdateBot(sbsSettings);
          CPrintF(" Updated Bot: %s^r\n", penBot->GetName());
        }
      }
    } break;

    // Change all weapons
    case ESA_SETWEAPONS: {
      INDEX iWeapon;
      nmMessage >> iWeapon;
      UBYTE bPlayer;
      nmMessage >> bPlayer;

      // add default weapons to the desired one
      INDEX iSetWeapons = WPN_FLAG(WEAPON_KNIFE) | WPN_FLAG(WEAPON_COLT) | iWeapon;

      FOREACHINDYNAMICCONTAINER(wo.wo_cenEntities, CEntity, iten) {
        CEntity *penFound = iten;

        if (!bPlayer) {
          if (!IsDerivedFromDllClass(penFound, CWeaponItem_DLLClass)) {
            continue;
          }

          ((CWeaponItem *)penFound)->m_EwitType = (WeaponItemType)iWeapon;
          penFound->Reinitialize();

        } else {
          // player markers
          if (IsDerivedFromDllClass(penFound, CPlayerMarker_DLLClass)) {
            ((CPlayerMarker *)penFound)->m_iGiveWeapons = iSetWeapons;
          }

          // current player weapons
          if (IsDerivedFromDllClass(penFound, CPlayerWeapons_DLLClass)) {
            ((CPlayerWeapons *)penFound)->m_iAvailableWeapons = iSetWeapons;

            for (INDEX iAddAmmo = WEAPON_NONE; iAddAmmo < WEAPON_LAST; iAddAmmo++) {
              INDEX iAddAmmoFlag = WPN_FLAG(iAddAmmo);

              if (iSetWeapons & iAddAmmoFlag) {
                ((CPlayerWeapons *)penFound)->AddDefaultAmmoForWeapon(iAddAmmo, 0);
              }
            }

            // force change from the current weapon
            ((CPlayerWeapons *)penFound)->WeaponSelectOk(WEAPON_COLT);
            penFound->SendEvent(EBegin());
          }
        }
      }

      if (!bPlayer) {
        CPrintF("Replaced all weapon items with \"%s\"\n", WeaponItemType_enum.NameForValue(iWeapon));
      } else {
        CPrintF("Set all PlayerMarkers and PlayerWeapons to %d\n", iSetWeapons);
      }
    } break;

    // NavMesh generation
    case ESA_NAVMESH_GEN: {
      INDEX iPoints;
      nmMessage >> iPoints;

      if (iPoints) {
        CTFileName fnClass = CTString("Classes\\NavMeshGenerator.ecl");
        CEntity *penNew = wo.CreateEntity_t(CPlacement3D(FLOAT3D(0, 0, 0), ANGLE3D(0, 0, 0)), fnClass);
        penNew->Initialize();

      } else {
        if (_pNavmesh->bnm_bGenerated) {
          _pNavmesh->ClearNavMesh();
          CPrintF("[NavMeshGenerator]: NavMesh has been cleared\n");

        } else {
          CPrintF("[NavMeshGenerator]: Generating points...\n");
          _pNavmesh->GenerateNavmesh(&wo);
          _pNavmesh->bnm_bGenerated = TRUE;
        }
      }
    } break;

    // NavMesh state
    case ESA_NAVMESH_STATE: {
      INDEX iLoad;
      nmMessage >> iLoad;
        
      try {
        // load NavMesh
        if (iLoad) {
          _pNavmesh->Load(wo);

        // save NavMesh
        } else {
          _pNavmesh->Save(wo);
        }

      } catch (char *strError) {
        CPrintF("%s\n", strError);

        if (iLoad) {
          _pNavmesh->ClearNavMesh();
        }
      }
    } break;
    
    // NavMesh clearing
    case ESA_NAVMESH_CLEAR: {
      CPrintF("NavMesh has been cleared\n");
      _pNavmesh->ClearNavMesh();
    } break;

    // NavMesh editing
    case ESA_NAVMESH_CREATE: {
      INDEX iTargetPoint, iConnect;
      nmMessage >> iTargetPoint >> iConnect;
      FLOAT fOffset;
      nmMessage >> fOffset;

      // no player
      if (pen == NULL) {
        break;
      }

      FLOAT3D vPoint = pen->GetPlacement().pl_PositionVector + FLOAT3D(0.0f, fOffset, 0.0f) * pen->GetRotationMatrix();

      // snap to a small grid
      Snap(vPoint(1), 0.25f);
      Snap(vPoint(3), 0.25f);

      CBotPathPoint *pbppNext = _pNavmesh->AddPoint(vPoint, NULL);

      // connect with the previous point like in a chain
      if (iConnect > 0) {
        CBotPathPoint *pbppPrev = _pNavmesh->FindPointByID(iTargetPoint);

        if (pbppPrev != NULL) {
          pbppPrev->Connect(pbppNext, iConnect);
        }
      }

      MOD_iNavMeshPoint = pbppNext->bpp_iIndex;
    } break;

    case ESA_NAVMESH_DELETE: {
      INDEX iCurrentPoint;
      nmMessage >> iCurrentPoint;

      CBotPathPoint *pbpp = _pNavmesh->FindPointByID(iCurrentPoint);

      if (pbpp != NULL) {
        // remove this point from every connection
        FOREACHINDYNAMICCONTAINER(_pNavmesh->bnm_cbppPoints, CBotPathPoint, itbpp) {
          CBotPathPoint *pbppCheck = itbpp;

          // remove connection with this point
          if (pbppCheck->bpp_cbppPoints.IsMember(pbpp)) {
            pbppCheck->bpp_cbppPoints.Remove(pbpp);
          }
        }

        // remove point from the NavMesh
        _pNavmesh->bnm_cbppPoints.Remove(pbpp);
        delete pbpp;

        _pShell->Execute("MOD_iNavMeshPoint = -1;");
      }
    } break;

    case ESA_NAVMESH_CONNECT: {
      INDEX iCurrentPoint, iTargetPoint;
      nmMessage >> iCurrentPoint >> iTargetPoint;
      UBYTE iConnect;
      nmMessage >> iConnect;

      CBotPathPoint *pbpp = _pNavmesh->FindPointByID(iCurrentPoint);

      if (pbpp == NULL) {
        CPrintF("NavMesh point doesn't exist!\n");

      } else {
        CBotPathPoint *pbppTarget = _pNavmesh->FindPointByID(iTargetPoint);
        pbpp->Connect(pbppTarget, iConnect);
      }
    } break;

    case ESA_NAVMESH_TELEPORT: {
      INDEX iCurrentPoint;
      nmMessage >> iCurrentPoint;
      FLOAT fOffset;
      nmMessage >> fOffset;

      // no player
      if (pen == NULL) {
        break;
      }

      CBotPathPoint *pbpp = _pNavmesh->FindPointByID(iCurrentPoint);

      if (pbpp == NULL) {
        CPrintF("NavMesh point doesn't exist!\n");

      } else {
        FLOAT3D vLastPos = pbpp->bpp_vPos;
        pbpp->bpp_vPos = pen->GetPlacement().pl_PositionVector + FLOAT3D(0.0f, fOffset, 0.0f) * pen->GetRotationMatrix();

        CPrintF("Point's position: [%.2f, %.2f, %.2f] -> [%.2f, %.2f, %.2f]\n",
                vLastPos(1), vLastPos(2), vLastPos(3), pbpp->bpp_vPos(1), pbpp->bpp_vPos(2), pbpp->bpp_vPos(3));
      }
    } break;

    case ESA_NAVMESH_POS: {
      INDEX iCurrentPoint;
      nmMessage >> iCurrentPoint;
      FLOAT fX, fY, fZ;
      nmMessage >> fX >> fY >> fZ;

      CBotPathPoint *pbpp = _pNavmesh->FindPointByID(iCurrentPoint);

      if (pbpp == NULL) {
        CPrintF("NavMesh point doesn't exist!\n");

      } else {
        FLOAT3D vLastPos = pbpp->bpp_vPos;
        pbpp->bpp_vPos = FLOAT3D(fX, fY, fZ);

        CPrintF("Point's position: [%.2f, %.2f, %.2f] -> [%.2f, %.2f, %.2f]\n",
                vLastPos(1), vLastPos(2), vLastPos(3), pbpp->bpp_vPos(1), pbpp->bpp_vPos(2), pbpp->bpp_vPos(3));
      }
    } break;

    case ESA_NAVMESH_SNAP: {
      INDEX iCurrentPoint;
      nmMessage >> iCurrentPoint;
      FLOAT fGridSize;
      nmMessage >> fGridSize;

      CBotPathPoint *pbpp = _pNavmesh->FindPointByID(iCurrentPoint);

      if (pbpp == NULL) {
        CPrintF("NavMesh point doesn't exist!\n");

      } else {
        FLOAT3D vLastPos = pbpp->bpp_vPos;

        for (INDEX iPos = 1; iPos <= 3; iPos++) {
          Snap(pbpp->bpp_vPos(iPos), fGridSize);
        }

        CPrintF("Point's position: [%.2f, %.2f, %.2f] -> [%.2f, %.2f, %.2f]\n",
                vLastPos(1), vLastPos(2), vLastPos(3), pbpp->bpp_vPos(1), pbpp->bpp_vPos(2), pbpp->bpp_vPos(3));
      }
    } break;

    case ESA_NAVMESH_FLAGS: {
      INDEX iCurrentPoint, iFlags;
      nmMessage >> iCurrentPoint >> iFlags;

      CBotPathPoint *pbpp = _pNavmesh->FindPointByID(iCurrentPoint);

      if (pbpp == NULL) {
        CPrintF("NavMesh point doesn't exist!\n");

      } else {
        ULONG ulFlags = pbpp->bpp_ulFlags;
        pbpp->bpp_ulFlags = (ULONG)iFlags;

        CPrintF("Point's flags: %s -> %s\n", ULongToBinary(ulFlags), ULongToBinary((ULONG)iFlags));
      }
    } break;

    case ESA_NAVMESH_ENTITY: {
      INDEX iCurrentPoint, iEntityID;
      nmMessage >> iCurrentPoint >> iEntityID;

      CBotPathPoint *pbpp = _pNavmesh->FindPointByID(iCurrentPoint);

      if (pbpp == NULL) {
        CPrintF("NavMesh point doesn't exist!\n");

      } else {
        pbpp->bpp_iImportant = iEntityID;

        CPrintF("Changed point's entity to %d\n", iEntityID);
      }
    } break;

    case ESA_NAVMESH_RANGE: {
      INDEX iCurrentPoint;
      nmMessage >> iCurrentPoint;
      FLOAT fRange;
      nmMessage >> fRange;

      CBotPathPoint *pbpp = _pNavmesh->FindPointByID(iCurrentPoint);

      if (pbpp == NULL) {
        CPrintF("NavMesh point doesn't exist!\n");

      } else {
        pbpp->bpp_fRange = fRange;

        CPrintF("Changed point's range to %s\n", FloatToStr(fRange));
      }
    } break;

    // Invalid action
    default:
      if (bLocal) {
        CPrintF(" Invalid sandbox action: %d\n", iAction);
      }
  }
};
