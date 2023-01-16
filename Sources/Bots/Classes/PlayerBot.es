/* Copyright (c) 2018-2023 Dreamy Cecil
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
  // Controller of this entity
  CPlayerBotController *m_pBot;
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
      CPlayerBotController &pbRemoved = _aPlayerBots.Pop();

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
    m_pBot->WriteBot(ostr);
  };

  // Read from stream
  void Read_t(CTStream *istr) {
    CPlayer::Read_t(istr);
    m_pBot->ReadBot(istr);
  };

  // [Cecil] 2021-06-12: Apply fake actions
  void PostMoving(void) {
    CPlayer::PostMoving();
    CPlayer::ApplyAction(CPlayerAction(), 0.0f);
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
        m_pBot->BotThinking(paAction, sbl);

        // Weapon functions
        m_pBot->BotWeapons(paAction, sbl);

        m_pBot->BotSelectNewWeapon(sbl.iDesiredWeapon);
      }

    // While dead
    } else {
      // Remember time of death
      if (m_iMayRespawn == 0) {
        GetProps().m_tmDied = _pTimer->CurrentTick();
      }

      // If allowed to respawn and delay has ended
      const FLOAT fRespawn = GetProps().m_sbsBot.fRespawnDelay;

      if (m_iMayRespawn == 2 && fRespawn >= 0.0f
       && _pTimer->CurrentTick() >= GetProps().m_tmDied + fRespawn)
      {
        // In singleplayer
        if (GetSP()->sp_bSinglePlayer) {
          // Respawn manually to avoid reloading the game
          SendEvent(EEnd());

        // Try to respawn
        } else if (m_pBot->ButtonAction()) {
          paAction.pa_ulButtons |= PLACT_FIRE;
        }
      }
    }

    // Adjust bot's speed
    FLOAT3D &vTranslation = paAction.pa_vTranslation;
    FLOAT fSpeedMul = GetProps().m_sbsBot.fSpeedMul;

    vTranslation(1) *= fSpeedMul;
    vTranslation(3) *= fSpeedMul;
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
