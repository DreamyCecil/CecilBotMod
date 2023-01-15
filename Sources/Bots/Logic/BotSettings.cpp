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

#include "StdH.h"
#include "BotSettings.h"

static void PresetDummy(SBotSettings &sbs) {
  sbs.fRespawnDelay = 0.0f;

  sbs.bSniperZoom = FALSE;
  sbs.bShooting = FALSE;
  sbs.fShootAngle = 15.0f;
  sbs.fAccuracyAngle = 5.0f;
  
  sbs.fRotSpeedDist = 400.0f;
  sbs.fRotSpeedMin = 0.05f;
  sbs.fRotSpeedMax = 0.2f;
  sbs.fRotSpeedLimit = 30.0f;
  
  sbs.fWeaponCD = 3.0f;
  sbs.fTargetCD = 1.0f;
  
  sbs.iFollowPlayers = 0;
  sbs.fSpeedMul = 1.0f;
  sbs.bStrafe = TRUE;
  sbs.bJump = TRUE;
  
  sbs.fPrediction = 0.1f;
  sbs.fPredictRnd = 0.1f;
  
  sbs.iAllowedWeapons = -1;
  sbs.iTargetType = -1;
  sbs.bTargetSearch = TRUE;
  sbs.fImportantChance = 0.2f;
  sbs.bItemSearch = TRUE;
  sbs.fItemSearchCD = 5.0f;
  
  sbs.bItemVisibility = TRUE;
  sbs.fWeaponDist = 16.0f;
  sbs.fHealthSearch = 100.0f;
  sbs.fHealthDist = 0.0f;
  sbs.fArmorDist = 0.0f;
  sbs.fAmmoDist = 0.0f;
};

static void PresetEasy(SBotSettings &sbs) {
  sbs.fRespawnDelay = 3.0f;

  sbs.bSniperZoom = TRUE;
  sbs.bShooting = TRUE;
  sbs.fShootAngle = 15.0f;
  sbs.fAccuracyAngle = 10.0f;
  
  sbs.fRotSpeedDist = 400.0f;
  sbs.fRotSpeedMin = 0.1f;
  sbs.fRotSpeedMax = 0.5f;
  sbs.fRotSpeedLimit = 20.0f;
  
  sbs.fWeaponCD = 5.0f;
  sbs.fTargetCD = 3.0f;
  
  sbs.iFollowPlayers = 1;
  sbs.fSpeedMul = 1.0f;
  sbs.bStrafe = FALSE;
  sbs.bJump = TRUE;
  
  sbs.fPrediction = 0.2f;
  sbs.fPredictRnd = 0.5f;
  
  sbs.iAllowedWeapons = -1;
  sbs.iTargetType = -1;
  sbs.bTargetSearch = TRUE;
  sbs.fImportantChance = 0.1f;
  sbs.bItemSearch = TRUE;
  sbs.fItemSearchCD = 5.0f;
  
  sbs.bItemVisibility = TRUE;
  sbs.fWeaponDist = 16.0f;
  sbs.fHealthSearch = 50.0f;
  sbs.fHealthDist = 32.0f;
  sbs.fArmorDist = 16.0f;
  sbs.fAmmoDist = 16.0f;
};

static void PresetNormal(SBotSettings &sbs) {
  sbs.fRespawnDelay = 1.0f;

  sbs.bSniperZoom = TRUE;
  sbs.bShooting = TRUE;
  sbs.fShootAngle = 15.0f;
  sbs.fAccuracyAngle = 5.0f;
  
  sbs.fRotSpeedDist = 400.0f;
  sbs.fRotSpeedMin = 0.05f;
  sbs.fRotSpeedMax = 0.2f;
  sbs.fRotSpeedLimit = 30.0f;
  
  sbs.fWeaponCD = 3.0f;
  sbs.fTargetCD = 1.0f;
  
  sbs.iFollowPlayers = 1;
  sbs.fSpeedMul = 1.0f;
  sbs.bStrafe = TRUE;
  sbs.bJump = TRUE;
  
  sbs.fPrediction = 0.1f;
  sbs.fPredictRnd = 0.1f;
  
  sbs.iAllowedWeapons = -1;
  sbs.iTargetType = -1;
  sbs.bTargetSearch = TRUE;
  sbs.fImportantChance = 0.2f;
  sbs.bItemSearch = TRUE;
  sbs.fItemSearchCD = 5.0f;
  
  sbs.bItemVisibility = TRUE;
  sbs.fWeaponDist = 16.0f;
  sbs.fHealthSearch = 80.0f;
  sbs.fHealthDist = 32.0f;
  sbs.fArmorDist = 16.0f;
  sbs.fAmmoDist = 16.0f;
};

static void PresetHard(SBotSettings &sbs) {
  sbs.fRespawnDelay = 0.0f;

  sbs.bSniperZoom = TRUE;
  sbs.bShooting = TRUE;
  sbs.fShootAngle = 15.0f;
  sbs.fAccuracyAngle = 2.0f;
  
  sbs.fRotSpeedDist = 50.0f;
  sbs.fRotSpeedMin = 0.05f;
  sbs.fRotSpeedMax = 0.1f;
  sbs.fRotSpeedLimit = 30.0f;
  
  sbs.fWeaponCD = 3.0f;
  sbs.fTargetCD = 1.0f;
  
  sbs.iFollowPlayers = 2;
  sbs.fSpeedMul = 1.0f;
  sbs.bStrafe = TRUE;
  sbs.bJump = TRUE;
  
  sbs.fPrediction = 0.5f;
  sbs.fPredictRnd = 0.0f;
  
  sbs.iAllowedWeapons = -1;
  sbs.iTargetType = -1;
  sbs.bTargetSearch = TRUE;
  sbs.fImportantChance = 0.4f;
  sbs.bItemSearch = TRUE;
  sbs.fItemSearchCD = 5.0f;
  
  sbs.bItemVisibility = TRUE;
  sbs.fWeaponDist = 16.0f;
  sbs.fHealthSearch = 100.0f;
  sbs.fHealthDist = 32.0f;
  sbs.fArmorDist = 16.0f;
  sbs.fAmmoDist = 16.0f;
};

typedef void (*CPresetFunc)(SBotSettings &);

// Reset settings to a certain difficulty preset
void SBotSettings::Reset(const EDifficulty eDiff) {
  static CPresetFunc aPresets[BDF_LAST] = {
    &PresetDummy,
    &PresetEasy,
    &PresetNormal,
    &PresetHard,
  };

  aPresets[eDiff](*this);
};
