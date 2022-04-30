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

#ifndef _CECILBOTS_BOTSETTINGS_H
#define _CECILBOTS_BOTSETTINGS_H

// [Cecil] 2020-07-28: A structure for bot settings
struct DECL_DLL SBotSettings {
  INDEX b3rdPerson; // Set third person view
  INDEX iCrosshair; // Preferred crosshair (-1 for random)

  INDEX bSniperZoom;    // Use sniper zoom or not
  INDEX bShooting;      // Attack or not
  FLOAT fShootAngle;    // Maximum attack angle
  FLOAT fAccuracyAngle; // Target angle accuracy
  
  FLOAT fRotSpeedDist;  // Maximum distance for the difference
  FLOAT fRotSpeedMin;   // Minimal rotation speed multiplier
  FLOAT fRotSpeedMax;   // Maximum rotation speed multiplier
  FLOAT fRotSpeedLimit; // Maximum rotation speed limit
  
  FLOAT fWeaponCD; // Weapon selection cooldown
  FLOAT fTargetCD; // Target selection cooldown
  
  FLOAT fSpeedMul; // Speed multiplier
  INDEX bStrafe;   // Strafe near the target or not
  INDEX bJump;     // Jump or not
  
  FLOAT fPrediction; // Position prediction multiplier
  FLOAT fPredictRnd; // Prediction randomness
  
  INDEX iAllowedWeapons;  // Allowed weapons for the bot (-1 = everything is allowed)
  INDEX iTargetType;      // Target type (0 - players, 1 - enemies, 2 - both)
  INDEX bTargetSearch;    // Search for a target or not
  FLOAT fImportantChance; // How often to pick important points
  INDEX bItemSearch;      // Search for items or not
  FLOAT fItemSearchCD;    // Item search cooldown
  
  INDEX bItemVisibility; // Check for item's visibility or not
  FLOAT fWeaponDist;     // Weapon search distance
  FLOAT fHealthSearch;   // Start health searching below this HP
  FLOAT fHealthDist;     // Health search distance
  FLOAT fArmorDist;      // Armor search distance
  FLOAT fAmmoDist;       // Ammo search distance

  // Constructor
  SBotSettings(void) {
    Reset();
  };

  // Reset settings
  void Reset(void) {
    b3rdPerson = TRUE;
    iCrosshair = -1;

    bSniperZoom = TRUE;
    bShooting = TRUE;
    fShootAngle = 15.0f;
    fAccuracyAngle = 5.0f;
  
    fRotSpeedDist = 400.0f;
    fRotSpeedMin = 0.05f;
    fRotSpeedMax = 0.2f;
    fRotSpeedLimit = 30.0f;
  
    fWeaponCD = 3.0f;
    fTargetCD = 1.0f;
  
    fSpeedMul = 1.0f;
    bStrafe = TRUE;
    bJump = TRUE;
  
    fPrediction = 0.1f;
    fPredictRnd = 0.1f;
  
    iAllowedWeapons = -1;
    iTargetType = -1;
    bTargetSearch = TRUE;
    fImportantChance = 0.2f;
    bItemSearch = TRUE;
    fItemSearchCD = 5.0f;
  
    bItemVisibility = TRUE;
    fWeaponDist = 16.0f;
    fHealthSearch = 100.0f;
    fHealthDist = 32.0f;
    fArmorDist = 16.0f;
    fAmmoDist = 16.0f;
  };

  // Stream operations
  friend CTStream &operator<<(CTStream &strm, SBotSettings &sbs) {
    strm.Write_t(&sbs, sizeof(sbs));
    return strm;
  };
  
  friend CTStream &operator>>(CTStream &strm, SBotSettings &sbs) {
    strm.Read_t(&sbs, sizeof(sbs));
    return strm;
  };
  
  // Message operations
  friend CNetworkMessage &operator<<(CNetworkMessage &nm, SBotSettings &sbs) {
    nm.Write(&sbs, sizeof(sbs));
    return nm;
  };
  
  friend CNetworkMessage &operator>>(CNetworkMessage &nm, SBotSettings &sbs) {
    nm.Read(&sbs, sizeof(sbs));
    return nm;
  };
};

#endif // _CECILBOTS_BOTSETTINGS_H
