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

// [Cecil] 2019-06-02: This file is for common elements and functions from the mod
#ifndef _CECILBOTS_SANDBOXCOMMON_H
#define _CECILBOTS_SANDBOXCOMMON_H

// --- Helper functions

// [Cecil] 2019-06-02: Function from Serious Gang mod that returns amount of numbers in the fraction
DECL_DLL INDEX FractionNumbers(FLOAT fNumber);

// [Cecil] 2019-06-02: Convert float number into the string without extra zeros
DECL_DLL inline CTString FloatToStr(const FLOAT &f);

// [Cecil] 2019-06-04: Convert unsigned long into a binary number
DECL_DLL CTString ULongToBinary(ULONG ul);

// [Cecil] 2019-06-05: Return file name with extension
DECL_DLL CTString FileNameWithExt(CTString strFileName);

// [Cecil] 2020-07-29: Project 3D line onto 2D space
DECL_DLL BOOL ProjectLine(CProjection3D *ppr, FLOAT3D vPoint1, FLOAT3D vPoint2, FLOAT3D &vOnScreen1, FLOAT3D &vOnScreen2);

// [Cecil] 2018-10-28: Finds an entity by its ID
DECL_DLL CEntity *FindEntityByID(CWorld *pwo, const INDEX &iEntityID);

// [Cecil] 2018-10-11: Distance Length Function
inline FLOAT DistanceToPos(FLOAT3D vPos1, FLOAT3D vPos2) {
  return (vPos1 - vPos2).Length();
};

// [Cecil] 2021-06-16: Determine vertical position difference
DECL_DLL FLOAT3D VerticalDiff(FLOAT3D vPosDiff, const FLOAT3D &vGravityDir);

// [Cecil] 2021-06-14: Determine position difference on the same plane
DECL_DLL FLOAT3D HorizontalDiff(FLOAT3D vPosDiff, const FLOAT3D &vGravityDir);

// [Cecil] 2021-06-28: Get relative angles from the directed placement
DECL_DLL FLOAT GetRelH(const CPlacement3D &pl);
DECL_DLL FLOAT GetRelP(const CPlacement3D &pl);

// [Cecil] 2020-07-29: Do the ray casting with specific passable flags
DECL_DLL void CastRayFlags(CCastRay &cr, CWorld *pwoWorld, ULONG ulPass);

// [Cecil] 2018-10-15: Check if polygon is suitable for walking on
DECL_DLL BOOL FlatPolygon(CWorld *wo, CBrushPolygon *pbpo);

// [Cecil] Check if entity is of given DLL class
DECL_DLL BOOL IsOfDllClass(CEntity *pen, const CDLLEntityClass &dec);

// [Cecil] Check if entity is of given DLL class or derived from it
DECL_DLL BOOL IsDerivedFromDllClass(CEntity *pen, const CDLLEntityClass &dec);

// [Cecil] 2022-05-01: Check if it's a non-deathmatch game
inline BOOL IsCoopGame(void) {
  return GetSP()->sp_bCooperative || GetSP()->sp_bSinglePlayer;
};

// --- Replacement functions

// [Cecil] 2021-06-17: Check if an entity exists
#define ASSERT_ENTITY(_Entity) (_Entity != NULL && !(_Entity->GetFlags() & ENF_DELETED))

// [Cecil] 2021-06-12: Looping through players and bots
DECL_DLL INDEX CECIL_GetMaxPlayers(void);
DECL_DLL CPlayer *CECIL_GetPlayerEntity(const INDEX &iPlayer);

#define GET_PLAYER(_Player, _Index) \
  CPlayer *_Player = CECIL_GetPlayerEntity(_Index); \
  if (!ASSERT_ENTITY(_Player)) continue;

// [Cecil] 2021-06-13: Get personal player index
DECL_DLL INDEX CECIL_PlayerIndex(CPlayerEntity *pen);

// [Cecil] Check player and bot entities
#define IS_PLAYER(_Entity) IsDerivedFromDllClass(_Entity, CPlayer_DLLClass)

#endif // _CECILBOTS_SANDBOXCOMMON_H
