/* Copyright (c) 2022-2023 Dreamy Cecil
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

#include "BotStructure.h"

#include "Bots/Logic/BotFunctions.h"
#include "Bots/Logic/BotItems.h"

#include "EntitiesMP/Player.h"
#include "EntitiesMP/PlayerWeapons.h"

// Add new bot thought
void SBotProperties::Thought(const char *strFormat, ...) {
  // Don't bother
  extern INDEX MOD_bBotThoughts;
  if (!MOD_bBotThoughts) return;

  va_list arg;
  va_start(arg, strFormat);

  CTString strNew;
  strNew.VPrintF(strFormat, arg);

  va_end(arg);

  // Update the last thought if it's the same
  if (m_btThoughts.strLast == strNew) {
    m_btThoughts.SetLast(strNew);
  } else {
    m_btThoughts.Push(strNew);
  }
};

// Retrieve player weapons class
CPlayerWeapons *CPlayerBotController::GetWeapons(void) {
  return ((CPlayer *)pen)->GetPlayerWeapons();
};

// Write bot properties
void CPlayerBotController::WriteBot(CTStream *strm) {
  // Write entity pointers
  if (props.m_penTarget != NULL) {
    *strm << (INDEX)props.m_penTarget->en_ulID;
  } else {
    *strm << (INDEX)-1;
  }

  if (props.m_penFollow != NULL) {
    *strm << (INDEX)props.m_penFollow->en_ulID;
  } else {
    *strm << (INDEX)-1;
  }

  if (props.m_penLastItem != NULL) {
    *strm << (INDEX)props.m_penLastItem->en_ulID;
  } else {
    *strm << (INDEX)-1;
  }

  // Write fields
  *strm << props.m_tmLastBotTarget;
  *strm << props.m_tmLastSawTarget;
  *strm << props.m_tmButtonAction;
  *strm << props.m_tmPosChange;
  *strm << props.m_vLastPos;

  *strm << props.m_fTargetDist;
  *strm << props.m_fSideDir;
  *strm << props.m_tmChangeBotDir;
  *strm << props.m_vAccuracy;
  *strm << props.m_tmBotAccuracy;

  *strm << props.m_tmChangePath;
  *strm << props.m_tmPickImportant;
  *strm << props.m_bImportantPoint;

  *strm << props.m_iBotWeapon;
  *strm << props.m_tmLastBotWeapon;
  *strm << props.m_tmShootTime;

  *strm << props.m_tmLastItemSearch;

  // Write current point
  if (props.m_pbppCurrent == NULL || !_pNavmesh->bnm_cbppPoints.IsMember(props.m_pbppCurrent)) {
    *strm << (INDEX)-1;
  } else {
    *strm << props.m_pbppCurrent->bpp_iIndex;
  }

  // Write target point
  if (props.m_pbppTarget == NULL || !_pNavmesh->bnm_cbppPoints.IsMember(props.m_pbppTarget)) {
    *strm << (INDEX)-1;
  } else {
    *strm << props.m_pbppTarget->bpp_iIndex;
  }

  // Write point flags
  *strm << props.m_ulPointFlags;

  // Write settings
  *strm << props.m_sbsBot;
};

// Read bot properties
void CPlayerBotController::ReadBot(CTStream *strm) {
  // Read entity pointers
  INDEX iEntity;

  *strm >> iEntity;
  props.m_penTarget = FindEntityByID(pen->GetWorld(), iEntity);

  *strm >> iEntity;
  props.m_penFollow = FindEntityByID(pen->GetWorld(), iEntity);

  *strm >> iEntity;
  props.m_penLastItem = FindEntityByID(pen->GetWorld(), iEntity);

  // Read fields
  *strm >> props.m_tmLastBotTarget;
  *strm >> props.m_tmLastSawTarget;
  *strm >> props.m_tmButtonAction;
  *strm >> props.m_tmPosChange;
  *strm >> props.m_vLastPos;

  *strm >> props.m_fTargetDist;
  *strm >> props.m_fSideDir;
  *strm >> props.m_tmChangeBotDir;
  *strm >> props.m_vAccuracy;
  *strm >> props.m_tmBotAccuracy;

  *strm >> props.m_tmChangePath;
  *strm >> props.m_tmPickImportant;
  *strm >> props.m_bImportantPoint;

  *strm >> props.m_iBotWeapon;
  *strm >> props.m_tmLastBotWeapon;
  *strm >> props.m_tmShootTime;

  *strm >> props.m_tmLastItemSearch;

  // Read current point
  INDEX iPoint;
  *strm >> iPoint;

  if (iPoint != -1) {
    props.m_pbppCurrent = _pNavmesh->FindPointByID(iPoint);
  }
  
  // Read target point
  *strm >> iPoint;

  if (iPoint != -1) {
    props.m_pbppTarget = _pNavmesh->FindPointByID(iPoint);
  }

  // Read point flags
  *strm >> props.m_ulPointFlags;

  // Read settings
  *strm >> props.m_sbsBot;
};

// Update bot settings
void CPlayerBotController::UpdateBot(const SBotSettings &sbs) {
  props.m_sbsBot = sbs;

  // Adjust target type
  if (props.m_sbsBot.iTargetType == -1)
  {
    if (IsCoopGame()) {
      props.m_sbsBot.iTargetType = 1; // Only enemies
    } else {
      props.m_sbsBot.iTargetType = 2; // Enemies and players
    }
  }

  // Various player settings
  CPlayerSettings *pps = (CPlayerSettings *)GetPlayerBot()->en_pcCharacter.pc_aubAppearance;
    
  // Third person view
  if (props.m_sbsBot.b3rdPerson) {
    pps->ps_ulFlags |= PSF_PREFER3RDPERSON;
  } else {
    pps->ps_ulFlags &= ~PSF_PREFER3RDPERSON;
  }

  // Change crosshair type
  if (props.m_sbsBot.iCrosshair < 0) {
    pps->ps_iCrossHairType = rand() % 7; // Randomize
  } else {
    pps->ps_iCrossHairType = props.m_sbsBot.iCrosshair;
  }
};

// Perform a button action if possible
BOOL CPlayerBotController::ButtonAction(void) {
  if (props.m_tmButtonAction <= _pTimer->CurrentTick()) {
    props.m_tmButtonAction = _pTimer->CurrentTick() + 0.2f;
    return TRUE;
  }
  return FALSE;
};

// Check if selected point is a current one
BOOL CPlayerBotController::CurrentPoint(CBotPathPoint *pbppExclude) {
  return (pbppExclude != NULL && props.m_pbppCurrent == pbppExclude);
};

// Select new weapon
void CPlayerBotController::BotSelectNewWeapon(const INDEX &iSelect) {
  // Nothing to select or on a cooldown
  if (iSelect == WPN_NOTHING || props.m_tmLastBotWeapon > _pTimer->CurrentTick()) {
    return;
  }

  CPlayerWeapons *penWeapons = GetWeapons();

  // Already selected
  if (penWeapons->m_iCurrentWeapon == iSelect) {
    return;
  }

  // Select it
  if (penWeapons->WeaponSelectOk((WeaponType)iSelect)) {
    penWeapons->SendEvent(EBegin());
    props.m_tmLastBotWeapon = _pTimer->CurrentTick() + props.m_sbsBot.fWeaponCD;
  }
};

// Bot weapon logic
void CPlayerBotController::BotWeapons(CPlayerAction &pa, SBotLogic &sbl) {
  CPlayerWeapons *penWeapons = GetWeapons();
    
  // User sniper scope
  if (props.m_sbsBot.bSniperZoom && CanUseScope()) {
    UseWeaponScope(pa, sbl);
  }

  // Pick weapon config
  const SBotWeaponConfig *aWeapons = sbl.aWeapons;
  props.m_iBotWeapon = CT_BOT_WEAPONS - 1;

  // Select knife for faster speed if haven't seen the enemy in a while
  if (!IsCoopGame() && _pTimer->CurrentTick() - props.m_tmLastSawTarget > 2.0f)
  {
    for (INDEX iWeapon = 0; iWeapon < CT_BOT_WEAPONS; iWeapon++) {
      INDEX iType = aWeapons[iWeapon].bw_iType;

      if (iType == WPN_DEFAULT_1) {
        props.m_iBotWeapon = iWeapon;
        sbl.iDesiredWeapon = WPN_DEFAULT_1;
        break;
      }
    }
    return;
  }

  // Pick currently suitable weapon
  INDEX iSelect = WPN_NOTHING;
  FLOAT fLastDamage = 0.0f;

  INDEX iWeaponType;
  FLOAT fMin, fMax, fAccuracy;

  for (INDEX iWeapon = 0; iWeapon < CT_BOT_WEAPONS; iWeapon++) {
    iWeaponType = aWeapons[iWeapon].bw_iType;

    fMin = aWeapons[iWeapon].bw_fMinDistance;
    fMax = aWeapons[iWeapon].bw_fMaxDistance;
    fAccuracy = aWeapons[iWeapon].bw_fAccuracy;

    // Skip unexistent weapons
    if (!WPN_EXISTS(penWeapons, iWeaponType)) {
      continue;
    }

    // Not allowed
    if (props.m_sbsBot.iAllowedWeapons != -1 && !(props.m_sbsBot.iAllowedWeapons & WPN_FLAG(iWeaponType))) {
      continue;
    }

    // Check if distance is okay
    if (props.m_fTargetDist > fMax || props.m_fTargetDist < fMin) {
      continue;
    }

    // No ammo
    if (!GetSP()->sp_bInfiniteAmmo && !WPN_HAS_AMMO(penWeapons, iWeaponType)) {
      continue;
    }

    FLOAT fDistRatio = (props.m_fTargetDist - fMin) / (fMax - fMin); // From min to max [0 .. 1]
    FLOAT fMul = fAccuracy + (1 - fAccuracy) * (1 - fDistRatio); // From min to max [fAccuracy .. 1]

    // Check damage
    if (fLastDamage < aWeapons[iWeapon].bw_fDamage * fMul) {
      // Select this weapon
      iSelect = iWeaponType;
      fLastDamage = aWeapons[iWeapon].bw_fDamage * fMul;

      props.m_iBotWeapon = iWeapon;
    }
  }

  // Select new weapon
  sbl.iDesiredWeapon = iSelect;
};

// Complete bot logic
void CPlayerBotController::BotThinking(CPlayerAction &pa, SBotLogic &sbl) {
  const FLOAT3D &vBotPos = pen->GetPlacement().pl_PositionVector;

  if (DistanceToPos(vBotPos, props.m_vLastPos) > 2.0f) {
    props.m_tmPosChange = _pTimer->CurrentTick();
    props.m_vLastPos = vBotPos;
  }

  // Set bot's absolute viewpoint
  sbl.plBotView = GetPlayerBot()->en_plViewpoint;
  sbl.plBotView.RelativeToAbsolute(pen->GetPlacement());
    
  // Bot targeting and following
  CEntity *penBotTarget = ClosestEnemy(props.m_fTargetDist, sbl);

  // Select new target only if it doesn't exist or after a cooldown
  if (!ASSERT_ENTITY(props.m_penTarget) || props.m_tmLastBotTarget <= _pTimer->CurrentTick()) {
    props.m_penTarget = penBotTarget;
    props.m_tmLastBotTarget = _pTimer->CurrentTick() + props.m_sbsBot.fTargetCD;

    // Select new weapon immediately
    props.m_tmLastBotWeapon = 0.0f;
  }

  props.m_penFollow = NULL;

  // Follow players in cooperative
  BOOL bFollowInCoop = (IsCoopGame() && props.m_sbsBot.iFollowPlayers != 0);

  if (bFollowInCoop) {
    sbl.ulFlags |= BLF_FOLLOWPLAYER;
  }

  // Enemy exists
  if (props.m_penTarget != NULL) {
    sbl.ulFlags |= BLF_ENEMYEXISTS;
    sbl.peiTarget = (EntityInfo *)props.m_penTarget->GetEntityInfo();

    // Can see the enemy
    if (CastBotRay(props.m_penTarget, sbl, TRUE)) {
      sbl.ulFlags |= BLF_SEEENEMY;
      props.m_tmLastSawTarget = _pTimer->CurrentTick();
    }
      
    // Follow the enemy
    props.m_penFollow = props.m_penTarget;

    // Stop following the player if detected an enemy
    if (props.m_sbsBot.iFollowPlayers == 2) {
      if (sbl.SeeEnemy() || props.m_fTargetDist < 16.0f) {
        sbl.ulFlags &= ~BLF_FOLLOWPLAYER;
      }
    }
  }

  // Aim at the target
  BotAim(pa, sbl);

  // Shoot if possible
  if (props.m_sbsBot.bShooting) {
    const SBotWeaponConfig &bwWeapon = sbl.aWeapons[props.m_iBotWeapon];

    // Allowed to shoot
    BOOL bCanShoot = sbl.CanShoot();
      
    // Only shoot allowed weapons
    if (props.m_sbsBot.iAllowedWeapons != -1) {
      bCanShoot = bCanShoot && props.m_sbsBot.iAllowedWeapons & WPN_FLAG(GetWeapons()->m_iCurrentWeapon);
    }

    // If allowed to shoot
    if (bCanShoot) {
      // Enough shooting time
      if (props.m_tmShootTime <= 0.0f || props.m_tmShootTime > _pTimer->CurrentTick()) {
        FireWeapon(pa, sbl);

      } else if (Abs(props.m_tmShootTime - _pTimer->CurrentTick()) < 0.05f) {
        props.Thought("Stop shooting");
      }

      // Reset shooting time a few ticks later
      if (props.m_tmShootTime + 0.05f <= _pTimer->CurrentTick()) {
        // Shooting frequency
        FLOAT tmShotFreq = bwWeapon.bw_tmShotFreq;

        // This weapon has a certain shooting frequency
        if (tmShotFreq > 0.0f) {
          props.m_tmShootTime = _pTimer->CurrentTick() + tmShotFreq;
          props.Thought("Shoot for %.2fs", tmShotFreq);

        // No frequency
        } else {
          props.m_tmShootTime = -1.0f;
        }
      }
    }
  }

  // Follow players
  if (bFollowInCoop) {
    FLOAT fDistToPlayer = -1.0f;
    CEntity *penPlayer = ClosestRealPlayer(vBotPos, fDistToPlayer);
      
    // Player exists
    if (penPlayer != NULL) {
      // Currently following players
      if (sbl.FollowPlayer()) {
        // Don't follow anything else
        props.m_penFollow = NULL;

        sbl.ulFlags |= BLF_SEEPLAYER;

        // Follow the player specifically
        if (fDistToPlayer > 5.0f) {
          props.m_penFollow = penPlayer;
          sbl.ulFlags |= BLF_FOLLOWING;

          // Player is too far
          if (fDistToPlayer > 100.0f || !CastBotRay(penPlayer, sbl, TRUE)) {
            sbl.ulFlags &= ~BLF_SEEPLAYER;
          }

        } else if (fDistToPlayer < 2.0f) {
          props.m_penFollow = penPlayer;
          sbl.ulFlags |= BLF_BACKOFF;
        }

        // Look at the player
        if (!sbl.SeeEnemy() && sbl.SeePlayer()) {
          // Relative position
          CPlacement3D plToPlayer(penPlayer->GetPlacement().pl_PositionVector - vBotPos, sbl.ViewAng());

          // Angle towards the target
          FLOAT2D vToPlayer = FLOAT2D(GetRelH(plToPlayer), GetRelP(plToPlayer));

          // Set rotation speed
          sbl.aAim(1) = vToPlayer(1) / 0.5f;
          sbl.aAim(2) = vToPlayer(2) / 0.5f;
        }
      }

      // Teleport back to the player
      if (fDistToPlayer > 200.0f && pen->GetFlags() & ENF_ALIVE) {
        FLOAT3D vPlayer = penPlayer->GetPlacement().pl_PositionVector;
        ANGLE3D aPlayer = penPlayer->GetPlacement().pl_OrientationAngle;

        FLOAT3D vDirToBot = HorizontalDiff(pen->GetPlacement().pl_PositionVector - vPlayer, ((CPlayer *)penPlayer)->en_vGravityDir);
        vDirToBot.Normalize();

        pen->Teleport(CPlacement3D(vPlayer + vDirToBot, aPlayer), FALSE);
      }
    }
  }
    
  // Search for items (more important than players)
  BotItemSearch(sbl);

  // Try to find a path to the target
  BotPathFinding(sbl);

  // Aim
  pa.pa_aRotation(1) += sbl.aAim(1);
  pa.pa_aRotation(2) += sbl.aAim(2);

  // Set bot movement
  BotMovement(pa, sbl);
};
