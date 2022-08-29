/* Copyright (c) 2022 Dreamy Cecil
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

#ifndef _CECILBOTS_BOTSTRUCTURE_H
#define _CECILBOTS_BOTSTRUCTURE_H

#include "Bots/Logic/BotSettings.h"
#include "Bots/Logic/BotThoughts.h"
#include "Bots/Logic/BotWeapons.h"

// Properties of a bot entity
struct SBotProperties {
  CEntityPointer m_penTarget; // Shooting target
  CEntityPointer m_penFollow; // Following target

  FLOAT m_tmLastBotTarget; // Cooldown for target selection
  FLOAT m_tmLastSawTarget; // Last time the enemy has been seen
  FLOAT m_tmButtonAction;  // Cooldown for button actions
  FLOAT m_tmPosChange;     // Last time bot has significantly moved
  FLOAT3D m_vLastPos;      // Last bot position

  FLOAT m_fTargetDist;    // How far is the following target
  FLOAT m_fSideDir;       // Prioritize going left or right
  FLOAT m_tmChangeBotDir; // When to randomize the side direction
  FLOAT3D m_vAccuracy;    // Accuracy angle (should be preserved between ticks)
  FLOAT m_tmBotAccuracy;  // Accuracy update cooldown

  FLOAT m_tmChangePath;    // Path update cooldown
  FLOAT m_tmPickImportant; // How often to pick important points
  BOOL m_bImportantPoint;  // Focused on the important point or not

  INDEX m_iBotWeapon;      // Which weapon is currently prioritized
  FLOAT m_tmLastBotWeapon; // Cooldown for weapon selection
  FLOAT m_tmShootTime;     // When to shoot the next time

  FLOAT m_tmLastItemSearch;     // Item search cooldown
  CEntityPointer m_penLastItem; // Last selected item to run towards

  CBotPathPoint *m_pbppCurrent; // Current path point
  CBotPathPoint *m_pbppTarget;  // Target point
  ULONG m_ulPointFlags;         // Last point's flags

  SBotSettings m_sbsBot; // Bot settings
  SBotThoughts m_btThoughts; // Bot thoughts

  // Constructor
  SBotProperties() {
    // Temporary properties
    m_tmButtonAction = 0.0f;
    m_fTargetDist = 1000.0f;
    m_fSideDir = -1.0f;
    m_tmChangeBotDir = 0.0f;
    m_vAccuracy = FLOAT3D(0.0f, 0.0f, 0.0f);
    m_tmBotAccuracy = 0.0f;

    // Common properties
    Reset();
  };

  // Reset common properties
  void Reset(void) {
    m_pbppCurrent = NULL;
    m_pbppTarget = NULL;
    m_ulPointFlags = 0;

    m_tmLastBotTarget = 0.0f;
    m_tmLastSawTarget = 0.0f;
    m_tmPosChange = _pTimer->CurrentTick();
    m_vLastPos = FLOAT3D(0.0f, 0.0f, 0.0f);
    
    m_tmChangePath = 0.0f;
    m_tmPickImportant = 0.0f;
    m_bImportantPoint = FALSE;
    
    m_iBotWeapon = CT_BOT_WEAPONS;
    m_tmLastBotWeapon = 0.0f;
    m_tmShootTime = -1.0f;

    // Give some time before picking anything up
    m_tmLastItemSearch = _pTimer->CurrentTick() + 1.0f;
    m_penLastItem = NULL;
  };

  // Reset last position to the current one
  void ResetLastPos(CEntity *penThis) {
    m_vLastPos = penThis->GetPlacement().pl_PositionVector;
  };
};

#endif // _CECILBOTS_BOTSTRUCTURE_H
