/* Copyright (c) 2018-2022 Dreamy Cecil
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

// [Cecil] 2021-06-20: This file is for functions primarily used by PlayerBot class
#ifndef _CECILBOTS_BOTWEAPONS_H
#define _CECILBOTS_BOTWEAPONS_H

#include "EntitiesMP/PlayerWeapons.h"
#include "BotLogic.h"

// [Cecil] 2018-10-12: Bot's weapon configuration
struct SBotWeaponConfig {
  INDEX bw_iType;         // Weapon type
  FLOAT bw_fMinDistance;  // Min (closest) distance to use it
  FLOAT bw_fMaxDistance;  // Max (furthest) distance to use it
  FLOAT bw_fDamage;       // Average damage
  FLOAT bw_fAccuracy;     // How accurate the weapon is
  FLOAT bw_fStrafe;       // At what percent of min to max distance to start strafing
  FLOAT bw_fSpecialRange; // At what percentage from max distance to use a special attack (negative for "closer than %", positive for "further than %")
  FLOAT bw_tmShotFreq;    // How frequently to spam the fire button (-1 or NO_FREQ to hold)
};

#define CT_BOT_WEAPONS WEAPON_LAST
#define NO_SPEC (0.0f)
#define NO_FREQ (-1.0f)

// [Cecil] 2018-10-12: Bot weapon priority (from best to worst)
extern const SBotWeaponConfig _abwDeathmatchWeapons[CT_BOT_WEAPONS];
extern const SBotWeaponConfig _abwCooperativeWeapons[CT_BOT_WEAPONS];

// Pick weapon config
inline const SBotWeaponConfig *PickWeaponConfig(void) {
  if (IsCoopGame()) {
    return _abwCooperativeWeapons;
  }
  return _abwDeathmatchWeapons;
};

// --- Customizable helper functions

// Default weapons
#define WPN_NOTHING   WEAPON_NONE // No weapon
#define WPN_DEFAULT_1 WEAPON_KNIFE // Increases running speed in deathmatch
#define WPN_DEFAULT_2 WEAPON_COLT

// Weapon flag from the weapon index
#define WPN_FLAG(_Weapon) (1 << (_Weapon - 1))

// Default weapon set
#define WPN_DEFAULT_MASK (WPN_FLAG(WPN_DEFAULT_1) | WPN_FLAG(WPN_DEFAULT_2))

// Has this weapon
#define WPN_EXISTS(_Plw, _Weapon) (_Plw->m_iAvailableWeapons & WPN_FLAG(_Weapon))

// Weapon has ammo
#define WPN_HAS_AMMO(_Plw, _Weapon) (_Plw->HasAmmo((WeaponType)_Weapon))

// Currently zooming in with a scope or not
BOOL UsingScope(class CPlayerBot *pen);

// Able to use the scope or not
BOOL CanUseScope(class CPlayerBot *pen);

// Use weapon scope for a bot now
void UseWeaponScope(class CPlayerBot *pen, CPlayerAction &pa, const SBotLogic &sbl);

// Fire the weapon now
void FireWeapon(class CPlayerBot *pen, CPlayerAction &pa, const SBotLogic &sbl);

#endif // _CECILBOTS_BOTWEAPONS_H
