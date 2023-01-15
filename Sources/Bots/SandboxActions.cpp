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
#include "SandboxActions.h"
#include "Bots/Logic/BotFunctions.h"

#include "CustomPackets.h"
#include "Bots/NetworkPatch/ServerIntegration.h"

// [Cecil] 2021-06-18: For weapon switching
#include "EntitiesMP/PlayerMarker.h"

// [Cecil] 2021-06-19: Render entity IDs
extern INDEX MOD_bEntityIDs = FALSE;

// [Cecil] 2022-04-27: Let clients send sandbox actions
extern INDEX MOD_bClientSandbox = FALSE;

// [Cecil] 2021-06-21: Display bot thoughts
extern INDEX MOD_bBotThoughts = FALSE;

// [Cecil] 2019-06-02: Bot names and skins
static CStaticArray<CTString> BOT_astrNames;
static CStaticArray<CTString> BOT_astrSkins;

// [Cecil] 2021-08-28: Names and skins for the current game
static CDynamicContainer<CTString> BOT_cnCurrentNames;
static CDynamicContainer<CTString> BOT_cnCurrentSkins;

// [Cecil] 2018-10-15: Bot Editing
static CTString BOT_strBotEdit = ""; // name of a bot to edit
static CTString BOT_strSpawnName = ""; // name to spawn with
static CTString BOT_strSpawnTeam = ""; // team to spawn in

// [Cecil] 2020-07-28: A structure with current bot settings
static SBotSettings _sbsBotSettings;

// [Cecil] 2019-11-07: Special client packet for NavMesh editing
static CNetworkMessage CECIL_NavMeshClientPacket(const INDEX &iAction) {
  NEW_PACKET(nmNavmesh, MSG_CECIL_SANDBOX);
  nmNavmesh << iAction; // Specific NavMesh action
  nmNavmesh << LOCAL_PLAYER_INDEX; // Local player

  return nmNavmesh;
};

// [Cecil] 2019-11-07: Special server packet for bots
static CCecilStreamBlock CECIL_BotServerPacket(const INDEX &iAction) {
  CServer &srvServer = _pNetwork->ga_srvServer;

  CCecilStreamBlock nsbBot(MSG_CECIL_SANDBOX, ++srvServer.srv_iLastProcessedSequence);
  nsbBot << iAction; // Specific NavMesh action
  nsbBot << LOCAL_PLAYER_INDEX; // Local player

  return nsbBot;
};

// --- Bot manipulation

// [Cecil] 2021-08-28: Populate current bot names
extern void CopyBotNames(void) {
  // clear current names
  BOT_cnCurrentNames.Clear();

  for (int i = 0; i < BOT_astrNames.Count(); i++) {
    BOT_cnCurrentNames.Add(&BOT_astrNames[i]);
  }
};

// [Cecil] 2021-08-28: Populate current bot skins
extern void CopyBotSkins(void) {
  // clear current skins
  BOT_cnCurrentSkins.Clear();

  for (int i = 0; i < BOT_astrSkins.Count(); i++) {
    BOT_cnCurrentSkins.Add(&BOT_astrSkins[i]);
  }
};

// [Cecil] 2018-10-15: Config reset
static void CECIL_ResetBotConfig(INDEX iDifficulty) {
  CPrintF(BOTCOM_NAME("ResetBotConfig:\n"));

  if (iDifficulty < 0 || iDifficulty >= SBotSettings::BDF_LAST) {
    iDifficulty = SBotSettings::BDF_NORMAL;
  }

  _sbsBotSettings.Reset((SBotSettings::EDifficulty)iDifficulty);

  static CTString astrNames[SBotSettings::BDF_LAST] = {
    "Dummy", "Easy", "Normal", "Hard",
  };

  CPrintF("  Bot config has been reset to \"%s\" preset!\n", astrNames[iDifficulty]);
};

// [Cecil] 2018-10-09: Bot adding
static void CECIL_AddBot(CTString *pstrBotName, CTString *pstrBotSkin, CTString *pstrBotTeam) {
  CTString strBotName = *pstrBotName;
  CTString strBotSkin = *pstrBotSkin;

  CPrintF(MODCOM_NAME("AddBot:\n"));

  if (!_pNetwork->IsServer()) {
    CPrintF("  <not a server>\n");
    return;
  }
  
  // pick random name if none
  if (strBotName == "") {
    INDEX ctNames = BOT_cnCurrentNames.Count();

    // restore names
    if (ctNames <= 1) {
      CopyBotNames();
      ctNames = BOT_cnCurrentNames.Count();
    }

    CTString *pstrName = BOT_cnCurrentNames.Pointer(rand() % ctNames);
    strBotName = (ctNames > 0) ? *pstrName : "Bot";

    // remove this name from the list
    BOT_cnCurrentNames.Remove(pstrName);
  }
  
  // pick random skin if none
  if (strBotSkin == "") {
    INDEX ctSkins = BOT_cnCurrentSkins.Count();

    // restore skins
    if (ctSkins <= 0) {
      CopyBotSkins();
      ctSkins = BOT_cnCurrentSkins.Count();
    }

    CTString *pstrSkin = BOT_cnCurrentSkins.Pointer(rand() % ctSkins);
    strBotSkin = (ctSkins > 0) ? *pstrSkin : "SeriousSam";

    // remove this skin from the list
    //BOT_cnCurrentSkins.Remove(pstrSkin);
  }

  CPlayerCharacter pcBot;
  CPlayerSettings *pps = (CPlayerSettings *)pcBot.pc_aubAppearance;

  pps->ps_iWeaponAutoSelect = PS_WAS_NONE; // never select new weapons
  memset(pps->ps_achModelFile, 0, sizeof(pps->ps_achModelFile));
  strncpy(pps->ps_achModelFile, strBotSkin.str_String, sizeof(pps->ps_achModelFile));

  for (INDEX iGUID = 0; iGUID < 16; iGUID++) {
    pcBot.pc_aubGUID[iGUID] = rand() % 256;
  }

  pcBot.pc_strName = strBotName;
  pcBot.pc_strTeam = *pstrBotTeam;

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
  CTString strTeam = BOT_strSpawnTeam;
  CECIL_AddBot(&strName, &strSkin, &strTeam);
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

  for (iBot = 0; iBot < _aPlayerBots.Count(); iBot++) {
    CPlayerBot *penBot = (CPlayerBot *)_aPlayerBots[iBot].pen;

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

  for (INDEX iBot = 0; iBot < _aPlayerBots.Count(); iBot++) {
    CPlayerBot *penBot = (CPlayerBot *)_aPlayerBots[iBot].pen;

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

// [Cecil] 2023-01-15: Bot teleporting
static void CECIL_BotTeleport(void) {
  CPrintF(MODCOM_NAME("BotTeleport:\n"));

  if (!_pNetwork->IsServer()) {
    CPrintF("  <not a server>\n");
    return;
  }

  CCecilStreamBlock nsbBotUpdate = CECIL_BotServerPacket(ESA_TELEPORTBOTS);
  nsbBotUpdate << BOT_strBotEdit; // Bot name

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
#ifdef _SE1_10
static void CECIL_GenerateNavMesh(void *pArgs) {
  INDEX iPoints = NEXTARGUMENT(INDEX);
#else
static void CECIL_GenerateNavMesh(INDEX iPoints) {
#endif

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
  CWorld &wo = _pNetwork->ga_World;
  
  // save NavMesh locally
  try {
    _pNavmesh->SaveNavmesh(wo);

  } catch (char *strError) {
    CPrintF("%s\n", strError);
  }
};

static void CECIL_NavMeshLoad(void) {
  CPrintF(MODCOM_NAME("NavMeshLoad:\n"));

  if (!_pNetwork->IsServer()) {
    CPrintF("  <not a server>\n");
    return;
  }
  
  CCecilStreamBlock nsbNavMesh = CECIL_BotServerPacket(ESA_NAVMESH_LOAD);

  // put the message in buffer to be sent to all sessions
  CECIL_AddBlockToAllSessions(nsbNavMesh);
};

// [Cecil] 2021-06-16: Quick function for NavMesh clearing
static void CECIL_NavMeshClear(void) {
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
static void CECIL_AddNavMeshPoint(FLOAT fOffset, FLOAT fGridSnap) {
  // editing is disabled
  if (MOD_iRenderNavMesh <= 0) {
    return;
  }

  CNetworkMessage nmNavmesh = CECIL_NavMeshClientPacket(ESA_NAVMESH_CREATE);
  nmNavmesh << MOD_iNavMeshPoint;
  nmNavmesh << MOD_iNavMeshConnecting;
  nmNavmesh << fOffset;
  nmNavmesh << fGridSnap;

  _pNetwork->SendToServerReliable(nmNavmesh);
};

// [Cecil] 2019-05-28: Delete NavMesh point
static void CECIL_DeleteNavMeshPoint(void) {
  // editing is disabled
  if (MOD_iRenderNavMesh <= 0) {
    return;
  }

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

  CEntity *penImportant = pbpp->bpp_penImportant;
  CEntity *penLock = pbpp->bpp_penLock;
  
  CPrintF("Connections: %d\n", pbpp->bpp_cbppPoints.Count());
  CPrintF("Pos:    %.2f, %.2f, %.2f\n", pbpp->bpp_vPos(1), pbpp->bpp_vPos(2), pbpp->bpp_vPos(3));
  CPrintF("Range:  %.2f\n", pbpp->bpp_fRange);
  CPrintF("Flags:  %s\n", ULongToBinary(pbpp->bpp_ulFlags));
  CPrintF("Entity: \"%s\"\n", (penImportant == NULL) ? "<none>" : penImportant->GetName());
  CPrintF("Next:   %d\n", (pbpp->bpp_pbppNext == NULL) ? -1 : pbpp->bpp_pbppNext->bpp_iIndex);
  CPrintF("Lock:   \"%s\"\n", (penLock == NULL) ? "<none>" : penLock->GetName());

  if (penLock != NULL) {
    FLOAT3D &vPos = pbpp->bpp_plLockOrigin.pl_PositionVector;
    ANGLE3D &aRot = pbpp->bpp_plLockOrigin.pl_OrientationAngle;
    CPrintF("Origin: [%.2f, %.2f, %.2f], [%.2f, %.2f, %.2f]\n",
            vPos(1), vPos(2), vPos(3), aRot(1), aRot(2), aRot(3));
  }
};

// [Cecil] 2021-06-12: Connect current NavMesh point to another one
static void CECIL_ConnectNavMeshPoint(INDEX iTargetPoint) {
  // editing is disabled
  if (MOD_iRenderNavMesh <= 0) {
    return;
  }

  CNetworkMessage nmNavmesh = CECIL_NavMeshClientPacket(ESA_NAVMESH_CONNECT);
  nmNavmesh << MOD_iNavMeshPoint;
  nmNavmesh << iTargetPoint;
  nmNavmesh << UBYTE(MOD_iNavMeshConnecting);

  _pNetwork->SendToServerReliable(nmNavmesh);
};

// [Cecil] 2021-06-21: Untarget current NavMesh point from another one
static void CECIL_UntargetNavMeshPoint(INDEX iTargetPoint) {
  // editing is disabled
  if (MOD_iRenderNavMesh <= 0) {
    return;
  }

  CNetworkMessage nmNavmesh = CECIL_NavMeshClientPacket(ESA_NAVMESH_UNTARGET);
  nmNavmesh << MOD_iNavMeshPoint;
  nmNavmesh << iTargetPoint;

  _pNetwork->SendToServerReliable(nmNavmesh);
};

// [Cecil] 2021-06-18: Move NavMesh point to the player position
static void CECIL_TeleportNavMeshPoint(FLOAT fOffset) {
  // editing is disabled
  if (MOD_iRenderNavMesh <= 0) {
    return;
  }

  CNetworkMessage nmNavmesh = CECIL_NavMeshClientPacket(ESA_NAVMESH_TELEPORT);
  nmNavmesh << MOD_iNavMeshPoint;
  nmNavmesh << fOffset;

  _pNetwork->SendToServerReliable(nmNavmesh);
};

// [Cecil] 2021-06-18: Change NavMesh point absolute position
static void CECIL_NavMeshPointPos(FLOAT fX, FLOAT fY, FLOAT fZ) {
  // editing is disabled
  if (MOD_iRenderNavMesh <= 0) {
    return;
  }

  CNetworkMessage nmNavmesh = CECIL_NavMeshClientPacket(ESA_NAVMESH_POS);
  nmNavmesh << MOD_iNavMeshPoint;
  nmNavmesh << fX << fY << fZ;

  _pNetwork->SendToServerReliable(nmNavmesh);
};

// [Cecil] 2021-06-18: Snap NavMesh point position to a custom-sized grid
static void CECIL_SnapNavMeshPoint(FLOAT fGridSize) {
  // editing is disabled
  if (MOD_iRenderNavMesh <= 0) {
    return;
  }

  CNetworkMessage nmNavmesh = CECIL_NavMeshClientPacket(ESA_NAVMESH_SNAP);
  nmNavmesh << MOD_iNavMeshPoint;
  nmNavmesh << fGridSize;

  _pNetwork->SendToServerReliable(nmNavmesh);
};

// [Cecil] 2019-06-04: Change NavMesh point flags
static void CECIL_NavMeshPointFlags(INDEX iFlags) {
  // editing is disabled
  if (MOD_iRenderNavMesh <= 0) {
    return;
  }

  CNetworkMessage nmNavmesh = CECIL_NavMeshClientPacket(ESA_NAVMESH_FLAGS);
  nmNavmesh << MOD_iNavMeshPoint;
  nmNavmesh << iFlags;

  _pNetwork->SendToServerReliable(nmNavmesh);
};

// [Cecil] 2019-06-05: Change NavMesh point important entity
static void CECIL_NavMeshPointEntity(INDEX iEntityID) {
  // editing is disabled
  if (MOD_iRenderNavMesh <= 0) {
    return;
  }

  CNetworkMessage nmNavmesh = CECIL_NavMeshClientPacket(ESA_NAVMESH_ENTITY);
  nmNavmesh << MOD_iNavMeshPoint;
  nmNavmesh << iEntityID;

  _pNetwork->SendToServerReliable(nmNavmesh);
};

// [Cecil] 2019-06-06: Change NavMesh point range
static void CECIL_NavMeshPointRange(FLOAT fRange) {
  // editing is disabled
  if (MOD_iRenderNavMesh <= 0) {
    return;
  }

  CNetworkMessage nmNavmesh = CECIL_NavMeshClientPacket(ESA_NAVMESH_RANGE);
  nmNavmesh << MOD_iNavMeshPoint;
  nmNavmesh << fRange;

  _pNetwork->SendToServerReliable(nmNavmesh);
};

// [Cecil] 2021-06-25: Change NavMesh point next important point
static void CECIL_NavMeshPointNext(INDEX iNextPoint) {
  // editing is disabled
  if (MOD_iRenderNavMesh <= 0) {
    return;
  }

  CNetworkMessage nmNavmesh = CECIL_NavMeshClientPacket(ESA_NAVMESH_NEXT);
  nmNavmesh << MOD_iNavMeshPoint;
  nmNavmesh << iNextPoint;

  _pNetwork->SendToServerReliable(nmNavmesh);
};

// [Cecil] 2021-09-09: Change NavMesh point lock entity
static void CECIL_NavMeshPointLock(INDEX iEntityID) {
  // editing is disabled
  if (MOD_iRenderNavMesh <= 0) {
    return;
  }

  CNetworkMessage nmNavmesh = CECIL_NavMeshClientPacket(ESA_NAVMESH_LOCK);
  nmNavmesh << MOD_iNavMeshPoint;
  nmNavmesh << iEntityID;

  _pNetwork->SendToServerReliable(nmNavmesh);
};

// [Cecil] 2021-06-23: Add NavMesh point range
static void CECIL_AddNavMeshPointRange(FLOAT fRange) {
  // editing is disabled
  if (MOD_iRenderNavMesh <= 0) {
    return;
  }

  CBotPathPoint *pbpp = _pNavmesh->FindPointByID(MOD_iNavMeshPoint);

  if (pbpp == NULL) {
    CPrintF("NavMesh point doesn't exist!\n");
    return;
  }

  FLOAT fNewRange = ClampDn(pbpp->bpp_fRange + fRange, 0.0f);
  CECIL_NavMeshPointRange(fNewRange);
};

// [Cecil] 2019-05-28: NavMesh Point Selection
static void CECIL_NavMeshSelectPoint(void) {
  // editing is disabled
  if (MOD_iRenderNavMesh <= 0) {
    return;
  }

  if (LOCAL_PLAYER_INDEX == -1) {
    return;
  }

  CPlayer *pen = (CPlayer *)CEntity::GetPlayerEntity(LOCAL_PLAYER_INDEX);
  CBotPathPoint *pbppNearest = NearestNavMeshPointPos(pen, pen->GetPlayerWeapons()->m_vRayHit);

  // no point
  if (pbppNearest == NULL) {
    return;
  }
      
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

// [Cecil] 2021-06-23: Initialize sandbox actions
extern void CECIL_InitSandboxActions(void) {
  // [Cecil] Bot mod
  _pShell->DeclareSymbol("user void " MODCOM_NAME("QuickBot(void);"), &CECIL_QuickBot);
  _pShell->DeclareSymbol("user void " MODCOM_NAME("AddBot(CTString, CTString, CTString);"), &CECIL_AddBot);
  _pShell->DeclareSymbol("user void " MODCOM_NAME("RemoveBot(CTString);"), &CECIL_RemoveBot);
  _pShell->DeclareSymbol("user void " MODCOM_NAME("RemoveAllBots(void);"), &CECIL_RemoveAllBots);
  _pShell->DeclareSymbol("user void " MODCOM_NAME("BotUpdate(void);"), &CECIL_BotUpdate);
  _pShell->DeclareSymbol("user void " MODCOM_NAME("BotTeleport(void);"), &CECIL_BotTeleport);

  _pShell->DeclareSymbol("user void " MODCOM_NAME("GenerateNavMesh(INDEX);"), &CECIL_GenerateNavMesh);
  _pShell->DeclareSymbol("user void " MODCOM_NAME("NavMeshSave(void);"), &CECIL_NavMeshSave);
  _pShell->DeclareSymbol("user void " MODCOM_NAME("NavMeshLoad(void);"), &CECIL_NavMeshLoad);
  _pShell->DeclareSymbol("user void " MODCOM_NAME("NavMeshClear(void);"), &CECIL_NavMeshClear);

  // [Cecil] Misc
  _pShell->DeclareSymbol("user INDEX " MODCOM_NAME("bEntityIDs;"), &MOD_bEntityIDs);
  _pShell->DeclareSymbol("user INDEX " MODCOM_NAME("bClientSandbox;"), &MOD_bClientSandbox);
  _pShell->DeclareSymbol("persistent user INDEX " MODCOM_NAME("bBotThoughts;"), &MOD_bBotThoughts);

  _pShell->DeclareSymbol("user void " MODCOM_NAME("SetWeapons(INDEX, INDEX);"), &CECIL_SetWeapons);

  // [Cecil] Bot editing
  _pShell->DeclareSymbol("user CTString " BOTCOM_NAME("strBotEdit;"), &BOT_strBotEdit);
  _pShell->DeclareSymbol("persistent user CTString " BOTCOM_NAME("strSpawnName;"), &BOT_strSpawnName);
  _pShell->DeclareSymbol("persistent user CTString " BOTCOM_NAME("strSpawnTeam;"), &BOT_strSpawnTeam);

  _pShell->DeclareSymbol("user void " BOTCOM_NAME("ResetBotConfig(INDEX);"), &CECIL_ResetBotConfig);
  _pShell->DeclareSymbol("persistent user INDEX " BOTCOM_NAME("b3rdPerson;"      ), &_sbsBotSettings.b3rdPerson);
  _pShell->DeclareSymbol("persistent user INDEX " BOTCOM_NAME("iCrosshair;"      ), &_sbsBotSettings.iCrosshair);
  _pShell->DeclareSymbol("persistent user FLOAT " BOTCOM_NAME("fRespawnDelay;"   ), &_sbsBotSettings.fRespawnDelay);

  _pShell->DeclareSymbol("persistent user INDEX " BOTCOM_NAME("bSniperZoom;"     ), &_sbsBotSettings.bSniperZoom);
  _pShell->DeclareSymbol("persistent user INDEX " BOTCOM_NAME("bShooting;"       ), &_sbsBotSettings.bShooting);
  _pShell->DeclareSymbol("persistent user FLOAT " BOTCOM_NAME("fShootAngle;"     ), &_sbsBotSettings.fShootAngle);
  _pShell->DeclareSymbol("persistent user FLOAT " BOTCOM_NAME("fAccuracyAngle;"  ), &_sbsBotSettings.fAccuracyAngle);

  _pShell->DeclareSymbol("persistent user FLOAT " BOTCOM_NAME("fRotSpeedDist;"   ), &_sbsBotSettings.fRotSpeedDist);
  _pShell->DeclareSymbol("persistent user FLOAT " BOTCOM_NAME("fRotSpeedMin;"    ), &_sbsBotSettings.fRotSpeedMin);
  _pShell->DeclareSymbol("persistent user FLOAT " BOTCOM_NAME("fRotSpeedMax;"    ), &_sbsBotSettings.fRotSpeedMax);
  _pShell->DeclareSymbol("persistent user FLOAT " BOTCOM_NAME("fRotSpeedLimit;"  ), &_sbsBotSettings.fRotSpeedLimit);

  _pShell->DeclareSymbol("persistent user FLOAT " BOTCOM_NAME("fWeaponCD;"       ), &_sbsBotSettings.fWeaponCD);
  _pShell->DeclareSymbol("persistent user FLOAT " BOTCOM_NAME("fTargetCD;"       ), &_sbsBotSettings.fTargetCD);
  
  _pShell->DeclareSymbol("persistent user INDEX " BOTCOM_NAME("iFollowPlayers;"  ), &_sbsBotSettings.iFollowPlayers);
  _pShell->DeclareSymbol("persistent user FLOAT " BOTCOM_NAME("fSpeedMul;"       ), &_sbsBotSettings.fSpeedMul);
  _pShell->DeclareSymbol("persistent user INDEX " BOTCOM_NAME("bStrafe;"         ), &_sbsBotSettings.bStrafe);
  _pShell->DeclareSymbol("persistent user INDEX " BOTCOM_NAME("bJump;"           ), &_sbsBotSettings.bJump);
  _pShell->DeclareSymbol("persistent user INDEX " BOTCOM_NAME("bAvoidPits;"      ), &_sbsBotSettings.bAvoidPits);

  _pShell->DeclareSymbol("persistent user FLOAT " BOTCOM_NAME("fPrediction;"     ), &_sbsBotSettings.fPrediction);
  _pShell->DeclareSymbol("persistent user FLOAT " BOTCOM_NAME("fPredictRnd;"     ), &_sbsBotSettings.fPredictRnd);

  _pShell->DeclareSymbol("persistent user INDEX " BOTCOM_NAME("iAllowedWeapons;" ), &_sbsBotSettings.iAllowedWeapons);
  _pShell->DeclareSymbol("persistent user INDEX " BOTCOM_NAME("iTargetType;"     ), &_sbsBotSettings.iTargetType);
  _pShell->DeclareSymbol("persistent user INDEX " BOTCOM_NAME("bTargetSearch;"   ), &_sbsBotSettings.bTargetSearch);
  _pShell->DeclareSymbol("persistent user FLOAT " BOTCOM_NAME("fImportantChance;"), &_sbsBotSettings.fImportantChance);
  _pShell->DeclareSymbol("persistent user INDEX " BOTCOM_NAME("bItemSearch;"     ), &_sbsBotSettings.bItemSearch);
  _pShell->DeclareSymbol("persistent user FLOAT " BOTCOM_NAME("fItemSearchCD;"   ), &_sbsBotSettings.fItemSearchCD);

  _pShell->DeclareSymbol("persistent user INDEX " BOTCOM_NAME("bItemVisibility;" ), &_sbsBotSettings.bItemVisibility);
  _pShell->DeclareSymbol("persistent user FLOAT " BOTCOM_NAME("fWeaponDist;"     ), &_sbsBotSettings.fWeaponDist);
  _pShell->DeclareSymbol("persistent user FLOAT " BOTCOM_NAME("fHealthSearch;"   ), &_sbsBotSettings.fHealthSearch);
  _pShell->DeclareSymbol("persistent user FLOAT " BOTCOM_NAME("fHealthDist;"     ), &_sbsBotSettings.fHealthDist);
  _pShell->DeclareSymbol("persistent user FLOAT " BOTCOM_NAME("fArmorDist;"      ), &_sbsBotSettings.fArmorDist);
  _pShell->DeclareSymbol("persistent user FLOAT " BOTCOM_NAME("fAmmoDist;"       ), &_sbsBotSettings.fAmmoDist);

  // [Cecil] NavMesh editing
  _pShell->DeclareSymbol("persistent user INDEX " MODCOM_NAME("iRenderNavMesh;"), &MOD_iRenderNavMesh);
  _pShell->DeclareSymbol("persistent user FLOAT " MODCOM_NAME("fNavMeshRenderRange;"), &MOD_fNavMeshRenderRange);
  _pShell->DeclareSymbol("user INDEX " MODCOM_NAME("iNavMeshPoint;"), &MOD_iNavMeshPoint);
  _pShell->DeclareSymbol("user INDEX " MODCOM_NAME("iNavMeshConnecting;"), &MOD_iNavMeshConnecting);

  _pShell->DeclareSymbol("user void " MODCOM_NAME("AddNavMeshPoint(FLOAT, FLOAT);"), &CECIL_AddNavMeshPoint);
  _pShell->DeclareSymbol("user void " MODCOM_NAME("DeleteNavMeshPoint(void);"), &CECIL_DeleteNavMeshPoint);
  _pShell->DeclareSymbol("user void " MODCOM_NAME("NavMeshPointInfo(void);"), &CECIL_NavMeshPointInfo);
  _pShell->DeclareSymbol("user void " MODCOM_NAME("ConnectNavMeshPoint(INDEX);"), &CECIL_ConnectNavMeshPoint);
  _pShell->DeclareSymbol("user void " MODCOM_NAME("UntargetNavMeshPoint(INDEX);"), &CECIL_UntargetNavMeshPoint);
  _pShell->DeclareSymbol("user void " MODCOM_NAME("TeleportNavMeshPoint(FLOAT);"), &CECIL_TeleportNavMeshPoint);

  _pShell->DeclareSymbol("user void " MODCOM_NAME("NavMeshPointPos(FLOAT, FLOAT, FLOAT);"), &CECIL_NavMeshPointPos);
  _pShell->DeclareSymbol("user void " MODCOM_NAME("SnapNavMeshPoint(FLOAT);"), &CECIL_SnapNavMeshPoint);
  _pShell->DeclareSymbol("user void " MODCOM_NAME("NavMeshPointFlags(INDEX);"), &CECIL_NavMeshPointFlags);
  _pShell->DeclareSymbol("user void " MODCOM_NAME("NavMeshPointEntity(INDEX);"), &CECIL_NavMeshPointEntity);
  _pShell->DeclareSymbol("user void " MODCOM_NAME("NavMeshPointRange(FLOAT);"), &CECIL_NavMeshPointRange);
  _pShell->DeclareSymbol("user void " MODCOM_NAME("NavMeshPointNext(INDEX);"), &CECIL_NavMeshPointNext);
  _pShell->DeclareSymbol("user void " MODCOM_NAME("NavMeshPointLock(INDEX);"), &CECIL_NavMeshPointLock);
  _pShell->DeclareSymbol("user void " MODCOM_NAME("AddNavMeshPointRange(FLOAT);"), &CECIL_AddNavMeshPointRange);

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

// Receive and perform a sandbox action
void CECIL_SandboxAction(CPlayer *pen, const INDEX &iAction, CNetworkMessage &nmMessage) {
  BOOL bLocal = _pNetwork->IsPlayerLocal(pen);
  CWorld &wo = _pNetwork->ga_World;

  switch (iAction) {
    // [Cecil] 2019-06-03: Add a new bot to the game
    case ESA_ADDBOT: {
      CPlayerCharacter pcBot;
      nmMessage >> pcBot; // Player character

      SBotSettings sbsSettings;
      nmMessage >> sbsSettings;

      // Delete all predictors
      wo.DeletePredictors();

      // If there is no entity with that character in the world
      CPlayerBot *penNewBot = (CPlayerBot *)wo.FindEntityWithCharacter(pcBot);

      if (penNewBot == NULL) {
        // Create an entity for it
        CPlacement3D pl(FLOAT3D(0.0f, 0.0f, 0.0f), ANGLE3D(0.0f, 0.0f, 0.0f));

        try {
          CTFileName fnmPlayer = CTString("Classes\\PlayerBot.ecl");
          penNewBot = (CPlayerBot *)wo.CreateEntity_t(pl, fnmPlayer);

          // Add to the bot list
          CPlayerBotController &pbNew = _aPlayerBots.Push();

          penNewBot->m_pBot = &pbNew;
          *penNewBot->m_pBot = penNewBot;

          // Attach the character to it
          penNewBot->en_pcCharacter = pcBot;

          // Update settings and initialize
          pbNew.UpdateBot(sbsSettings);
          penNewBot->Initialize();

          CPrintF(TRANS("Added bot '%s^r'\n"), penNewBot->GetPlayerName());

        } catch (char *strError) {
          FatalError(TRANS("Cannot load PlayerBot class:\n%s"), strError);
        }

      } else {
        CPrintF(TRANS("Player entity with the given character already exists!\n"));
      }
    } break;

    // [Cecil] 2021-06-12: Remove the bot
    case ESA_REMBOT: {
      INDEX iBot;
      nmMessage >> iBot; // Bot index

      CPlayerBot *penBot = (CPlayerBot *)_aPlayerBots[iBot].pen;

      // Delete all predictors
      wo.DeletePredictors();

      // Inform entity of disconnnection
      CPrintF(TRANS("Removed %s\n"), penBot->GetPlayerName());
      penBot->Disconnect();
    } break;

    // Bot updating
    case ESA_UPDATEBOT: {
      CTString strBotEdit;
      nmMessage >> strBotEdit;

      SBotSettings sbsSettings;
      nmMessage >> sbsSettings;

      for (INDEX iBot = 0; iBot < _aPlayerBots.Count(); iBot++) {
        CPlayerBotController &pb = _aPlayerBots[iBot];

        // For only one specific bot or all bots
        if (strBotEdit == "" || pb.pen->GetName().Undecorated().Matches(strBotEdit)) {
          pb.UpdateBot(sbsSettings);
          CPrintF(" Updated Bot: %s^r\n", pb.pen->GetName());
        }
      }
    } break;

    // Bot teleporting
    case ESA_TELEPORTBOTS: {
      CTString strBotEdit;
      nmMessage >> strBotEdit;

      // No player
      if (pen == NULL) {
        break;
      }

      CPrintF(" Teleported Bots to %s^r\n", pen->GetName());

      for (INDEX iBot = 0; iBot < _aPlayerBots.Count(); iBot++) {
        CPlayerBotController &pb = _aPlayerBots[iBot];

        // Only one specific bot or all bots
        if (strBotEdit == "" || pb.pen->GetName().Undecorated().Matches(strBotEdit)) {
          pb.pen->Teleport(pen->GetPlacement(), FALSE);
          CPrintF(" - %s^r\n", pb.pen->GetName());
        }
      }
    } break;

    // Change all weapons
    case ESA_SETWEAPONS: {
      INDEX iWeapon;
      nmMessage >> iWeapon;
      UBYTE bPlayer;
      nmMessage >> bPlayer;

      // Add default weapons to the desired one
      INDEX iSetWeapons = WPN_DEFAULT_MASK | iWeapon;

      FOREACHINDYNAMICCONTAINER(wo.wo_cenEntities, CEntity, iten) {
        CEntity *penFound = iten;

        if (!bPlayer) {
          if (!IsDerivedFromClass(penFound, "Weapon Item")) {
            continue;
          }

          ((CWeaponItem *)penFound)->m_EwitType = (WeaponItemType)iWeapon;
          penFound->Reinitialize();

        } else {
          // Player markers
          if (IsDerivedFromDllClass(penFound, CPlayerMarker_DLLClass)) {
            ((CPlayerMarker *)penFound)->m_iGiveWeapons = iSetWeapons;
          }

          // Current player weapons
          if (IsDerivedFromDllClass(penFound, CPlayerWeapons_DLLClass)) {
            ((CPlayerWeapons *)penFound)->m_iAvailableWeapons = iSetWeapons;

            for (INDEX iAddAmmo = 0; iAddAmmo < CT_BOT_WEAPONS; iAddAmmo++) {
              INDEX iAddAmmoFlag = WPN_FLAG(iAddAmmo);

              if (iSetWeapons & iAddAmmoFlag) {
                ((CPlayerWeapons *)penFound)->AddDefaultAmmoForWeapon(iAddAmmo, 0);
              }
            }

            // Force change from the current weapon
            ((CPlayerWeapons *)penFound)->WeaponSelectOk(WPN_DEFAULT_2);
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
    case ESA_NAVMESH_LOAD: {
      // Load the NavMesh
      try {
        _pNavmesh->LoadNavmesh(wo);

      } catch (char *strError) {
        CPrintF("%s\n", strError);
        _pNavmesh->ClearNavMesh();
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
      FLOAT fOffset, fGridSnap;
      nmMessage >> fOffset >> fGridSnap;

      // No player
      if (pen == NULL) {
        break;
      }

      FLOAT3D vPoint = pen->GetPlacement().pl_PositionVector + FLOAT3D(0.0f, fOffset, 0.0f) * pen->GetRotationMatrix();

      // Snap to some grid
      if (fGridSnap > 0.0f) {
        for (INDEX iPos = 1; iPos <= 3; iPos++) {
          Snap(vPoint(iPos), fGridSnap);
        }
      }

      CBotPathPoint *pbppNext = _pNavmesh->AddPoint(vPoint, NULL);

      // Connect with the previous point like in a chain
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
        // Remove this point from every connection
        FOREACHINDYNAMICCONTAINER(_pNavmesh->bnm_cbppPoints, CBotPathPoint, itbpp) {
          CBotPathPoint *pbppCheck = itbpp;

          // Remove connection with this point
          if (pbppCheck->bpp_cbppPoints.IsMember(pbpp)) {
            pbppCheck->bpp_cbppPoints.Remove(pbpp);
          }
        }

        // Remove point from the NavMesh
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

        CPrintF("Connected points %d and %d (type: %d)\n", iCurrentPoint, iTargetPoint, iConnect);
      }
    } break;

    case ESA_NAVMESH_UNTARGET: {
      INDEX iCurrentPoint, iTargetPoint;
      nmMessage >> iCurrentPoint >> iTargetPoint;

      CBotPathPoint *pbpp = _pNavmesh->FindPointByID(iCurrentPoint);

      if (pbpp == NULL) {
        CPrintF("NavMesh point doesn't exist!\n");

      } else {
        CBotPathPoint *pbppTarget = _pNavmesh->FindPointByID(iTargetPoint);

        if (pbpp->bpp_cbppPoints.IsMember(pbppTarget)) {
          pbpp->bpp_cbppPoints.Remove(pbppTarget);
          CPrintF("Untargeted point %d from %d\n", iTargetPoint, iCurrentPoint);

        } else {
          CPrintF("Point %d is not targeted!\n", iTargetPoint);
        }
      }
    } break;

    case ESA_NAVMESH_TELEPORT: {
      INDEX iCurrentPoint;
      nmMessage >> iCurrentPoint;
      FLOAT fOffset;
      nmMessage >> fOffset;

      // No player
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
        CEntity *penImportant = FindEntityByID(&wo, iEntityID);

        if (penImportant != NULL) {
          pbpp->bpp_penImportant = penImportant;
          CPrintF("Changed point's entity to %d\n", iEntityID);

        } else {
          pbpp->bpp_penImportant = NULL;
          CPrintF("Reset point's entity (entity under ID %d doesn't exist)\n", iEntityID);
        }
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
        FLOAT fOldRange = pbpp->bpp_fRange;
        pbpp->bpp_fRange = fRange;

        CPrintF("Changed point's range from %s to %s\n", FloatToStr(fOldRange), FloatToStr(fRange));
      }
    } break;

    case ESA_NAVMESH_NEXT: {
      INDEX iCurrentPoint, iNextPoint;
      nmMessage >> iCurrentPoint >> iNextPoint;

      CBotPathPoint *pbpp = _pNavmesh->FindPointByID(iCurrentPoint);

      if (pbpp == NULL) {
        CPrintF("NavMesh point doesn't exist!\n");

      } else {
        INDEX iLastNext = (pbpp->bpp_pbppNext == NULL) ? -1 : pbpp->bpp_pbppNext->bpp_iIndex;
        pbpp->bpp_pbppNext = _pNavmesh->FindPointByID(iNextPoint);

        INDEX iNewNext = (pbpp->bpp_pbppNext == NULL) ? -1 : pbpp->bpp_pbppNext->bpp_iIndex;
        CPrintF("Point's next important point: %d -> %d\n", iLastNext, iNewNext);
      }
    } break;

    case ESA_NAVMESH_LOCK: {
      INDEX iCurrentPoint, iEntityID;
      nmMessage >> iCurrentPoint >> iEntityID;

      CBotPathPoint *pbpp = _pNavmesh->FindPointByID(iCurrentPoint);

      if (pbpp == NULL) {
        CPrintF("NavMesh point doesn't exist!\n");

      } else {
        CEntity *penLock = FindEntityByID(&wo, iEntityID);

        if (penLock != NULL) {
          pbpp->bpp_penLock = penLock;
          pbpp->bpp_plLockOrigin = penLock->GetPlacement();

          CPrintF("Changed point's lock entity to %d\n", iEntityID);

        } else {
          pbpp->bpp_penLock = NULL;
          CPrintF("Reset point's lock entity (entity under ID %d doesn't exist)\n", iEntityID);
        }
      }
    } break;

    // Invalid action
    default:
      if (bLocal) {
        CPrintF(" Invalid sandbox action: %d\n", iAction);
      }
  }
};
