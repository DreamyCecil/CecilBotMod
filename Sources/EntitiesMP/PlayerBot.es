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

410
%{
#include "StdH.h"

#include "Bots/BotFunctions.h"
#include "Bots/BotMovement.h"
%}

uses "EntitiesMP/Player";
uses "EntitiesMP/PlayerWeapons";

uses "EntitiesMP/EnemyBase";
uses "EntitiesMP/CannonStatic";
uses "EntitiesMP/CannonRotating";

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
  1 CEntityPointer m_penTarget, // shooting target
  2 CEntityPointer m_penFollow, // following target
  3 FLOAT m_tmLastBotTarget = 0.0f, // cooldown for target selection
  4 FLOAT m_tmLastSawTarget = 0.0f, // last time the enemy has been seen
  5 FLOAT m_tmButtonAction = 0.0f, // cooldown for button actions
 
 10 FLOAT m_fTargetDist = 1000.0f, // how far is the following target
 11 FLOAT m_fBotDir = -1.0f,       // prioritize going left or right
 12 FLOAT3D m_vAccuracy = FLOAT3D(0.0f, 0.0f, 0.0f), // accuracy angle (should be preserved between ticks)
 13 FLOAT m_tmBotAccuracy = 0.0f,  // accuracy update cooldown
 
 20 FLOAT m_tmChangePath = 0.0f,    // path update cooldown
 21 FLOAT m_tmPickImportant = 0.0f, // how often to pick important points
 22 BOOL m_bImportantPoint = FALSE, // focused on the important point or not
 
 30 INDEX m_iBotWeapon = CT_BOT_WEAPONS, // which weapon is currently prioritized
 31 FLOAT m_tmLastBotWeapon = 0.0f, // cooldown for weapon selection

 40 FLOAT m_tmLastItemSearch = 0.0f, // item search cooldown
 41 CEntityPointer m_penLastItem,    // last selected item to run towards

{
  CBotPathPoint *m_pbppCurrent; // current path point
  CBotPathPoint *m_pbppTarget; // target point
  ULONG m_ulPointFlags; // last point's flags

  SBotSettings m_sbsBot; // bot settings
}

components:
  0 class CLASS_PLAYER "Classes\\Player.ecl",

functions:
  // Constructor
  void CPlayerBot(void) {
    m_pbppCurrent = NULL;
    m_pbppTarget = NULL;
    m_ulPointFlags = 0;
  };

  // Initialize the bot  
  virtual void InitBot(void) {
    m_fBotDir = (IRnd() % 2 == 0) ? -1.0f : 1.0f;
    m_pbppCurrent = NULL;
    m_pbppTarget = NULL;
    m_ulPointFlags = 0;
    m_bImportantPoint = FALSE;
    m_tmChangePath = 0.0f;
    m_tmPickImportant = 0.0f;
    m_tmLastBotWeapon = 0.0f;
    m_tmLastItemSearch = _pTimer->CurrentTick() + 1.0f; // give some time before picking anything up
    m_penLastItem = NULL;
  };
  
  // Bot destructor
  virtual void EndBot(void) {
    // remove from the bot list
    _cenPlayerBots.Remove(this);
  };

  // Identify as a bot
  virtual BOOL IsBot(void) {
    return TRUE;
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
  virtual BOOL CurrentPoint(CBotPathPoint *pbppExclude) {
    return (pbppExclude != NULL && m_pbppCurrent == pbppExclude);
  };

  // Apply action for bots
  virtual void BotApplyAction(CPlayerAction &paAction) {
    // alive
    if (GetFlags() & ENF_ALIVE) {
      if (m_penCamera == NULL && m_penActionMarker == NULL) {
        // bot's brain
        SBotLogic sbl;

        // main bot logic
        BotThinking(paAction, sbl);

        // weapon functions
        BotWeapons(paAction, sbl);
      }

    // dead
    } else {
      // try to respawn
      if (ButtonAction()) {
        paAction.pa_ulButtons |= PLRA_FIRE;
      }
    }
  };

  // Change bot's speed
  virtual void BotSpeed(FLOAT3D &vTranslation) {
    vTranslation(1) *= m_sbsBot.fSpeedMul;
    vTranslation(3) *= m_sbsBot.fSpeedMul;
  }

  // [Cecil] 2018-10-15: Update bot settings
  void UpdateBot(const SBotSettings &sbs) {
    m_sbsBot = sbs;

    // adjust target type
    if (m_sbsBot.iTargetType == -1)
    {
      if (GetSP()->sp_bCooperative || GetSP()->sp_bSinglePlayer) {
        m_sbsBot.iTargetType = 1; // only enemies
      } else {
        m_sbsBot.iTargetType = 2; // enemies and players
      }
    }
  };

  // [Cecil] 2021-06-16: Perform a button action if possible
  BOOL ButtonAction(void) {
    if (m_tmButtonAction < _pTimer->CurrentTick()) {
      m_tmButtonAction = _pTimer->CurrentTick() + 0.1f;
      return TRUE;
    }
    return FALSE;
  }

  // [Cecil] 2021-06-13: Check if it's an enemy player
  BOOL IsEnemyPlayer(CEntity *pen) {
    // simple class type check
    return IS_PLAYER(pen);
  };

  // [Cecil] 2021-06-16: Select new weapon
  void BotSelectNewWeapon(const WeaponType &wtSelect) {
    // nothing to select or on a cooldown
    if (wtSelect == WEAPON_NONE || m_tmLastBotWeapon > _pTimer->CurrentTick()) {
      return;
    }

    CPlayerWeapons *penWeapons = GetPlayerWeapons();

    // already selected
    if (penWeapons->m_iCurrentWeapon == wtSelect) {
      return;
    }

    // select it
    if (penWeapons->WeaponSelectOk(wtSelect)) {
      penWeapons->SendEvent(EBegin());
      m_tmLastBotWeapon = _pTimer->CurrentTick() + m_sbsBot.fWeaponCD;
    }
  };

  // [Cecil] 2018-10-10: Bot weapon logic
  void BotWeapons(CPlayerAction &pa, SBotLogic &sbl) {
    CPlayerWeapons *penWeapons = GetPlayerWeapons();
    
    // sniper zoom
    if (penWeapons->m_iCurrentWeapon == WEAPON_SNIPER && m_sbsBot.bSniperZoom)
    {
      if (ButtonAction()) {
        // zoom in if enemy is visible
        if (!penWeapons->m_bSniping && sbl.SeeEnemy()) {
          pa.pa_ulButtons |= PLRA_SNIPER_USE|PLRA_SNIPER_ZOOMIN;

        // zoom out if can't see the enemy
        } else if (penWeapons->m_bSniping && !sbl.SeeEnemy()) {
          pa.pa_ulButtons |= PLRA_SNIPER_USE;
        }
      }
    }

    // pick weapon config
    SBotWeaponConfig *aWeapons = PickWeaponConfig();

    // [Cecil] 2021-06-16: Select knife for faster speed if haven't seen the enemy in a while
    if (!GetSP()->sp_bCooperative && _pTimer->CurrentTick() - m_tmLastSawTarget > 2.0f) {
      m_iBotWeapon = CT_BOT_WEAPONS - 1;

      for (INDEX iWeapon = 0; iWeapon < CT_BOT_WEAPONS; iWeapon++) {
        WeaponType wtType = aWeapons[iWeapon].bw_wtType;

        if (wtType == WEAPON_KNIFE) {
          m_iBotWeapon = iWeapon;
          break;
        }
      }
      
      BotSelectNewWeapon(WEAPON_KNIFE);
      return;
    }

    WeaponType wtSelect = WEAPON_NONE;
    FLOAT fLastDamage = 0.0f;
    m_iBotWeapon = CT_BOT_WEAPONS - 1;

    WeaponType wtType;
    INDEX iWeaponFlag;
    FLOAT fMin, fMax, fAccuracy;

    for (INDEX iWeapon = 0; iWeapon < CT_BOT_WEAPONS; iWeapon++) {
      wtType = aWeapons[iWeapon].bw_wtType;
      iWeaponFlag = 1 << (wtType - 1);

      fMin = aWeapons[iWeapon].bw_fMinDistance;
      fMax = aWeapons[iWeapon].bw_fMaxDistance;
      fAccuracy = aWeapons[iWeapon].bw_fAccuracy;

      // if we have a priority weapon
      if (penWeapons->m_iAvailableWeapons & iWeaponFlag) {
        // check if it's allowed
        if (m_sbsBot.iAllowedWeapons != -1 && !(m_sbsBot.iAllowedWeapons & iWeaponFlag)) {
          continue;
        }

        // check if distance if okay
        if (m_fTargetDist > fMax || m_fTargetDist < fMin) {
          continue;
        }

        FLOAT fDistRatio = (m_fTargetDist - fMin) / (fMax - fMin); // from min to max [0 .. 1]
        FLOAT fMul = fAccuracy + (1 - fAccuracy) * (1 - fDistRatio); // from min to max [fAccuracy .. 1]

        // check damage
        if (fLastDamage < aWeapons[iWeapon].bw_fDamage * fMul) {
          // select this weapon
          wtSelect = wtType;
          fLastDamage = aWeapons[iWeapon].bw_fDamage * fMul;
          m_iBotWeapon = iWeapon;
        }
      }
    }

    // select new weapon
    BotSelectNewWeapon(wtSelect);
  };

  void BotThinking(CPlayerAction &pa, SBotLogic &sbl) {
    const FLOAT3D &vBotPos = GetPlacement().pl_PositionVector;

    // set bot's absolute viewpoint
    sbl.plBotView = en_plViewpoint;
    sbl.plBotView.RelativeToAbsolute(GetPlacement());
    
    // [Cecil] 2018-10-11 / 2018-10-13: Bot targeting and following
    CEntity *penBotTarget = ClosestEnemy(this, m_fTargetDist, sbl);

    // select new target only if it doesn't exist or after a cooldown
    if (ASSERT_ENTITY(m_penTarget) || m_tmLastBotTarget < _pTimer->CurrentTick()) {
      m_penTarget = penBotTarget;
      m_tmLastBotTarget = _pTimer->CurrentTick() + m_sbsBot.fTargetCD;

      // [Cecil] 2021-06-14: Select new weapon immediately
      m_tmLastBotWeapon = 0.0f;
    }

    m_penFollow = NULL;

    // [Cecil] TEMP 2019-05-28: Follow players in cooperative
    if (GetSP()->sp_bCooperative || GetSP()->sp_bSinglePlayer) {
      sbl.ubFlags |= BLF_FOLLOWPLAYER;
    }

    // enemy exists
    if (m_penTarget != NULL) {
      sbl.ubFlags |= BLF_ENEMYEXISTS;
      sbl.peiTarget = (EntityInfo *)m_penTarget->GetEntityInfo();

      // can see the enemy
      if (CastBotRay(this, m_penTarget, sbl, TRUE)) {
        sbl.ubFlags |= BLF_SEEENEMY;
        m_tmLastSawTarget = _pTimer->CurrentTick();
      }
      
      // follow the enemy
      m_penFollow = m_penTarget;
    }

    // aim at the target
    BotAim(this, pa, sbl);

    // only shoot allowed weapons
    BOOL bAllowedWeapon = TRUE;
    if (m_sbsBot.iAllowedWeapons != -1) {
      bAllowedWeapon = m_sbsBot.iAllowedWeapons & (1 << (GetPlayerWeapons()->m_iCurrentWeapon-1));
    }

    if (sbl.CanShoot() && bAllowedWeapon && m_sbsBot.bShooting) {
      pa.pa_ulButtons |= PLRA_FIRE;
    } else {
      pa.pa_ulButtons &= ~PLRA_FIRE;
    }
    
    // search for items
    BotItemSearch(this, sbl);

    // follow players
    if (sbl.FollowPlayer()) {

      FLOAT fDistToPlayer = -1.0f;
      CEntity *penPlayer = ClosestRealPlayer(this, vBotPos, fDistToPlayer);
      
      // player exists
      if (penPlayer != NULL) {
        // don't follow anything else
        m_penFollow = NULL;

        sbl.ubFlags |= BLF_SEEPLAYER;

        // follow the player
        if (fDistToPlayer > 3.0f) {
          m_penFollow = penPlayer;
          sbl.ubFlags |= BLF_FOLLOWING;

          // player is too far
          if (fDistToPlayer > 100.0f || !CastBotRay(this, penPlayer, sbl, TRUE)) {
            sbl.ubFlags &= ~BLF_SEEPLAYER;
          }
        }

        // look at the player
        if (!sbl.SeeEnemy() && sbl.SeePlayer()) {
          // calculate an angle
          FLOAT3D vToTarget = penPlayer->GetPlacement().pl_PositionVector - sbl.ViewPos();
          vToTarget.Normalize();

          ANGLE3D aToTarget;
          DirectionVectorToAnglesNoSnap(vToTarget, aToTarget);

          // reset vertical angle
          aToTarget(2) = 0.0f;

          // angle difference
          aToTarget -= sbl.ViewAng();

          // set rotation speed
          sbl.aAim(1) = NormalizeAngle(aToTarget(1)) / 0.5f;
          sbl.aAim(2) = NormalizeAngle(aToTarget(2)) / 0.5f;
        }
      }
    }

    // try to find a path to the target
    BotPathFinding(this, sbl);

    // aim
    pa.pa_aRotation(1) += sbl.aAim(1);
    pa.pa_aRotation(2) += sbl.aAim(2);

    // set bot movement
    BotMovement(this, pa, sbl);
  };

procedures:
  // Entry point
  Main() {
    // add to the bot list
    _cenPlayerBots.Add(this);

    // initialize the player
    jump CPlayer::SubMain();
  };
};
