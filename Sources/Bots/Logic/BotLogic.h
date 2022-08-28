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

#ifndef _CECILBOTS_BOTLOGIC_H
#define _CECILBOTS_BOTLOGIC_H

// [Cecil] 2021-06-14: Bot logic flags
#define BLF_ENEMYEXISTS  (1 << 0)
#define BLF_SEEENEMY     (1 << 1)
#define BLF_CANSHOOT     (1 << 2)
#define BLF_ITEMEXISTS   (1 << 3)
#define BLF_FOLLOWPLAYER (1 << 4)
#define BLF_FOLLOWING    (1 << 5)
#define BLF_SEEPLAYER    (1 << 6)
#define BLF_BACKOFF      (1 << 7)
#define BLF_STAYONPOINT  (1 << 8)

// [Cecil] 2021-06-14: Bot logic settings
struct SBotLogic {
  ULONG ulFlags; // Things bot is thinking about
  EntityInfo *peiTarget; // Info of a target that bot is looking at
  const struct SBotWeaponConfig *aWeapons; // Selected weapon config

  CPlacement3D plBotView; // Bot's viewpoint
  ANGLE3D aAim; // In which direction bot needs to aim

  INDEX iDesiredWeapon; // Weapon type for the bot to select

  // Constructor
  SBotLogic(void);

  // Viewpoint functions
  inline FLOAT3D &ViewPos(void) { return plBotView.pl_PositionVector; };
  inline ANGLE3D &ViewAng(void) { return plBotView.pl_OrientationAngle; };
  inline const FLOAT3D &ViewPos(void) const { return plBotView.pl_PositionVector; };
  inline const ANGLE3D &ViewAng(void) const { return plBotView.pl_OrientationAngle; };

  // Check flags
  inline BOOL EnemyExists(void)  const { return ulFlags & BLF_ENEMYEXISTS; };
  inline BOOL SeeEnemy(void)     const { return ulFlags & BLF_SEEENEMY; };
  inline BOOL CanShoot(void)     const { return ulFlags & BLF_CANSHOOT; };
  inline BOOL ItemExists(void)   const { return ulFlags & BLF_ITEMEXISTS; };
  inline BOOL FollowPlayer(void) const { return ulFlags & BLF_FOLLOWPLAYER; };
  inline BOOL Following(void)    const { return ulFlags & BLF_FOLLOWING; };
  inline BOOL SeePlayer(void)    const { return ulFlags & BLF_SEEPLAYER; };
  inline BOOL BackOff(void)      const { return ulFlags & BLF_BACKOFF; };
  inline BOOL StayOnPoint(void)  const { return ulFlags & BLF_STAYONPOINT; };
};

#endif // _CECILBOTS_BOTLOGIC_H
