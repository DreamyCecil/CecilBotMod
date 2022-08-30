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

// [Cecil] ID matches vanilla CPlayer on purpose
410
%{
#include "StdH.h"

#include "Bots/Logic/BotFunctions.h"
#include "Bots/Logic/BotMovement.h"
#include "Bots/Logic/BotItems.h"

#define THOUGHT(_String) (GetProps().m_btThoughts.Push(_String))
%}

// [Cecil] 2022-05-01: Includes headers to these in "PlayerBot.h"
uses "Bots/Logic/BotLogic";
uses "Bots/BotStructure";

uses "EntitiesMP/Player";
uses "EntitiesMP/PlayerWeapons";

uses "EntitiesMP/AmmoItem";
uses "EntitiesMP/AmmoPack";
uses "EntitiesMP/ArmorItem";
uses "EntitiesMP/HealthItem";
uses "EntitiesMP/Item";
uses "EntitiesMP/PowerUpItem";
uses "EntitiesMP/WeaponItem";

// [Cecil] Player bot
class CPlayerBot : CPlayer {
name      "PlayerBot";
thumbnail "";

properties:
{
  SPlayerBot *m_pBot; // Bot reference
}

components:
  0 class CLASS_PLAYER "Classes\\Player.ecl",

functions:
  // Initialize the bot  
  void InitBot(void) {
    GetProps().Reset();
    GetProps().ResetLastPos(this);
  };
  
  // Bot destructor
  void EndBot(void) {
    // Remove from the bot list
    INDEX iBot = FindBotByPointer(this);

    if (iBot != -1) {
      // Replace current bot with the last one
      SPlayerBot &pbRemoved = _aPlayerBots.Pop();

      // Only if current index is below the last one
      if (iBot < _aPlayerBots.Count()) {
        _aPlayerBots[iBot] = pbRemoved;
      }
    }
  };

  // Get bot properites
  SBotProperties &GetProps(void) {
    return m_pBot->props;
  };

  // Write to stream
  void Write_t(CTStream *ostr) {
    CPlayer::Write_t(ostr);
    BotWrite(this, ostr);
  };

  // Read from stream
  void Read_t(CTStream *istr) {
    CPlayer::Read_t(istr);
    BotRead(this, istr);
  };

  // [Cecil] 2021-06-12: Apply fake actions
  void PostMoving(void) {
    CPlayer::PostMoving();
    CPlayer::ApplyAction(CPlayerAction(), 0.0f);
  };

  // Check if selected point is a current one
  BOOL CurrentPoint(CBotPathPoint *pbppExclude) {
    return (pbppExclude != NULL && GetProps().m_pbppCurrent == pbppExclude);
  };

  // Apply action for bots
  virtual void BotApplyAction(CPlayerAction &paAction) {
    // Allow falling off all the time
    en_fStepDnHeight = -1;

    // While alive
    if (GetFlags() & ENF_ALIVE) {
      if (m_penCamera == NULL && m_penActionMarker == NULL) {
        // Bot's brain
        SBotLogic sbl;

        // Main bot logic
        BotThinking(paAction, sbl);

        // Weapon functions
        BotWeapons(paAction, sbl);

        BotSelectNewWeapon(sbl.iDesiredWeapon);
      }

    // While dead in singleplayer
    } else if (GetSP()->sp_bSinglePlayer) {
      // Respawn manually to avoid reloading the game
      if (m_iMayRespawn == 2) {
        SendEvent(EEnd());
      }

    // While dead in any other case
    } else {
      // Try to respawn
      if (ButtonAction()) {
        paAction.pa_ulButtons |= PLACT_FIRE;
      }
    }

    // Adjust bot's speed
    FLOAT3D &vTranslation = paAction.pa_vTranslation;
    FLOAT fSpeedMul = GetProps().m_sbsBot.fSpeedMul;

    vTranslation(1) *= fSpeedMul;
    vTranslation(3) *= fSpeedMul;
  };

  // [Cecil] 2018-10-15: Update bot settings
  void UpdateBot(const SBotSettings &sbs) {
    GetProps().m_sbsBot = sbs;

    // Adjust target type
    if (GetProps().m_sbsBot.iTargetType == -1)
    {
      if (IsCoopGame()) {
        GetProps().m_sbsBot.iTargetType = 1; // Only enemies
      } else {
        GetProps().m_sbsBot.iTargetType = 2; // Enemies and players
      }
    }

    // Various player settings
    CPlayerSettings *pps = (CPlayerSettings *)en_pcCharacter.pc_aubAppearance;
    
    // Third person view
    if (GetProps().m_sbsBot.b3rdPerson) {
      pps->ps_ulFlags |= PSF_PREFER3RDPERSON;
    } else {
      pps->ps_ulFlags &= ~PSF_PREFER3RDPERSON;
    }

    // Change crosshair type
    if (GetProps().m_sbsBot.iCrosshair < 0) {
      pps->ps_iCrossHairType = rand() % 7; // randomize
    } else {
      pps->ps_iCrossHairType = GetProps().m_sbsBot.iCrosshair;
    }
  };

  // [Cecil] 2021-06-16: Perform a button action if possible
  BOOL ButtonAction(void) {
    if (GetProps().m_tmButtonAction <= _pTimer->CurrentTick()) {
      GetProps().m_tmButtonAction = _pTimer->CurrentTick() + 0.2f;
      return TRUE;
    }
    return FALSE;
  }

  // [Cecil] 2021-06-16: Select new weapon
  void BotSelectNewWeapon(const INDEX &iSelect) {
    // Nothing to select or on a cooldown
    if (iSelect == WPN_NOTHING || GetProps().m_tmLastBotWeapon > _pTimer->CurrentTick()) {
      return;
    }

    CPlayerWeapons *penWeapons = GetPlayerWeapons();

    // Already selected
    if (penWeapons->m_iCurrentWeapon == iSelect) {
      return;
    }

    // Select it
    if (penWeapons->WeaponSelectOk((WeaponType)iSelect)) {
      penWeapons->SendEvent(EBegin());
      GetProps().m_tmLastBotWeapon = _pTimer->CurrentTick() + GetProps().m_sbsBot.fWeaponCD;
    }
  };

  // [Cecil] 2018-10-10: Bot weapon logic
  void BotWeapons(CPlayerAction &pa, SBotLogic &sbl) {
    CPlayerWeapons *penWeapons = GetPlayerWeapons();
    
    // sniper scope
    if (GetProps().m_sbsBot.bSniperZoom && CanUseScope(this)) {
      UseWeaponScope(this, pa, sbl);
    }

    // Pick weapon config
    const SBotWeaponConfig *aWeapons = sbl.aWeapons;
    GetProps().m_iBotWeapon = CT_BOT_WEAPONS - 1;

    // [Cecil] 2021-06-16: Select knife for faster speed if haven't seen the enemy in a while
    if (!IsCoopGame() && _pTimer->CurrentTick() - GetProps().m_tmLastSawTarget > 2.0f)
    {
      for (INDEX iWeapon = 0; iWeapon < CT_BOT_WEAPONS; iWeapon++) {
        INDEX iType = aWeapons[iWeapon].bw_iType;

        if (iType == WPN_DEFAULT_1) {
          GetProps().m_iBotWeapon = iWeapon;
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
      if (GetProps().m_sbsBot.iAllowedWeapons != -1 && !(GetProps().m_sbsBot.iAllowedWeapons & WPN_FLAG(iWeaponType))) {
        continue;
      }

      // Check if distance is okay
      if (GetProps().m_fTargetDist > fMax || GetProps().m_fTargetDist < fMin) {
        continue;
      }

      // No ammo
      if (!GetSP()->sp_bInfiniteAmmo && !WPN_HAS_AMMO(penWeapons, iWeaponType)) {
        continue;
      }

      FLOAT fDistRatio = (GetProps().m_fTargetDist - fMin) / (fMax - fMin); // From min to max [0 .. 1]
      FLOAT fMul = fAccuracy + (1 - fAccuracy) * (1 - fDistRatio); // From min to max [fAccuracy .. 1]

      // Check damage
      if (fLastDamage < aWeapons[iWeapon].bw_fDamage * fMul) {
        // Select this weapon
        iSelect = iWeaponType;
        fLastDamage = aWeapons[iWeapon].bw_fDamage * fMul;
        GetProps().m_iBotWeapon = iWeapon;
      }
    }

    // Select new weapon
    sbl.iDesiredWeapon = iSelect;
  };

  void BotThinking(CPlayerAction &pa, SBotLogic &sbl) {
    const FLOAT3D &vBotPos = GetPlacement().pl_PositionVector;

    if (DistanceToPos(vBotPos, GetProps().m_vLastPos) > 2.0f) {
      GetProps().m_tmPosChange = _pTimer->CurrentTick();
      GetProps().m_vLastPos = vBotPos;
    }

    // Set bot's absolute viewpoint
    sbl.plBotView = en_plViewpoint;
    sbl.plBotView.RelativeToAbsolute(GetPlacement());
    
    // [Cecil] 2018-10-11 / 2018-10-13: Bot targeting and following
    CEntity *penBotTarget = ClosestEnemy(this, GetProps().m_fTargetDist, sbl);

    // Select new target only if it doesn't exist or after a cooldown
    if (!ASSERT_ENTITY(GetProps().m_penTarget) || GetProps().m_tmLastBotTarget <= _pTimer->CurrentTick()) {
      GetProps().m_penTarget = penBotTarget;
      GetProps().m_tmLastBotTarget = _pTimer->CurrentTick() + GetProps().m_sbsBot.fTargetCD;

      // [Cecil] 2021-06-14: Select new weapon immediately
      GetProps().m_tmLastBotWeapon = 0.0f;
    }

    GetProps().m_penFollow = NULL;

    // [Cecil] 2019-05-28: Follow players in cooperative
    BOOL bFollowInCoop = (IsCoopGame() && GetProps().m_sbsBot.iFollowPlayers != 0);

    if (bFollowInCoop) {
      sbl.ulFlags |= BLF_FOLLOWPLAYER;
    }

    // Enemy exists
    if (GetProps().m_penTarget != NULL) {
      sbl.ulFlags |= BLF_ENEMYEXISTS;
      sbl.peiTarget = (EntityInfo *)GetProps().m_penTarget->GetEntityInfo();

      // Can see the enemy
      if (CastBotRay(this, GetProps().m_penTarget, sbl, TRUE)) {
        sbl.ulFlags |= BLF_SEEENEMY;
        GetProps().m_tmLastSawTarget = _pTimer->CurrentTick();
      }
      
      // Follow the enemy
      GetProps().m_penFollow = GetProps().m_penTarget;

      // Stop following the player if detected an enemy
      if (GetProps().m_sbsBot.iFollowPlayers == 2) {
        if (sbl.SeeEnemy() || GetProps().m_fTargetDist < 16.0f) {
          sbl.ulFlags &= ~BLF_FOLLOWPLAYER;
        }
      }
    }

    // Aim at the target
    BotAim(this, pa, sbl);

    // Shoot if possible
    if (GetProps().m_sbsBot.bShooting) {
      const SBotWeaponConfig &bwWeapon = sbl.aWeapons[GetProps().m_iBotWeapon];

      // Allowed to shoot
      BOOL bCanShoot = sbl.CanShoot();
      
      // Only shoot allowed weapons
      if (GetProps().m_sbsBot.iAllowedWeapons != -1) {
        bCanShoot = bCanShoot && GetProps().m_sbsBot.iAllowedWeapons & WPN_FLAG(GetPlayerWeapons()->m_iCurrentWeapon);
      }

      // If allowed to shoot
      if (bCanShoot) {
        // Enough shooting time
        if (GetProps().m_tmShootTime <= 0.0f || GetProps().m_tmShootTime > _pTimer->CurrentTick()) {
          FireWeapon(this, pa, sbl);

        } else if (Abs(GetProps().m_tmShootTime - _pTimer->CurrentTick()) < 0.05f) {
          THOUGHT("Stop shooting");
        }

        // Reset shooting time a few ticks later
        if (GetProps().m_tmShootTime + 0.05f <= _pTimer->CurrentTick()) {
          // Shooting frequency
          FLOAT tmShotFreq = bwWeapon.bw_tmShotFreq;

          // This weapon has a certain shooting frequency
          if (tmShotFreq > 0.0f) {
            GetProps().m_tmShootTime = _pTimer->CurrentTick() + tmShotFreq;
            THOUGHT(CTString(0, "Shoot for %.2fs", tmShotFreq));

          // No frequency
          } else {
            GetProps().m_tmShootTime = -1.0f;
          }
        }
      }
    }

    // Follow players
    if (bFollowInCoop) {
      FLOAT fDistToPlayer = -1.0f;
      CEntity *penPlayer = ClosestRealPlayer(this, vBotPos, fDistToPlayer);
      
      // Player exists
      if (penPlayer != NULL) {
        // Currently following players
        if (sbl.FollowPlayer()) {
          // Don't follow anything else
          GetProps().m_penFollow = NULL;

          sbl.ulFlags |= BLF_SEEPLAYER;

          // Follow the player specifically
          if (fDistToPlayer > 5.0f) {
            GetProps().m_penFollow = penPlayer;
            sbl.ulFlags |= BLF_FOLLOWING;

            // Player is too far
            if (fDistToPlayer > 100.0f || !CastBotRay(this, penPlayer, sbl, TRUE)) {
              sbl.ulFlags &= ~BLF_SEEPLAYER;
            }

          } else if (fDistToPlayer < 2.0f) {
            GetProps().m_penFollow = penPlayer;
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
        if (fDistToPlayer > 200.0f && GetFlags() & ENF_ALIVE) {
          FLOAT3D vPlayer = penPlayer->GetPlacement().pl_PositionVector;
          ANGLE3D aPlayer = penPlayer->GetPlacement().pl_OrientationAngle;

          FLOAT3D vDirToBot = HorizontalDiff(GetPlacement().pl_PositionVector - vPlayer, ((CPlayer *)penPlayer)->en_vGravityDir);
          vDirToBot.Normalize();

          Teleport(CPlacement3D(vPlayer + vDirToBot, aPlayer), FALSE);
        }
      }
    }
    
    // Search for items (more important than players)
    BotItemSearch(this, sbl);

    // Try to find a path to the target
    BotPathFinding(this, sbl);

    // Aim
    pa.pa_aRotation(1) += sbl.aAim(1);
    pa.pa_aRotation(2) += sbl.aAim(2);

    // Set bot movement
    BotMovement(this, pa, sbl);
  };

  // Override player initialization
  virtual void InitializePlayer(void) {
    CPlayer::InitializePlayer();

    // Initialize the bot
    InitBot();
  };

  // Upon class destruction (bot removal)
  void OnEnd(void) {
    CPlayer::OnEnd();

    // End the bot
    EndBot();
  };

procedures:
  // Entry point
  Main() {
    // Initialize the player
    jump CPlayer::SubMain();
  };
};
