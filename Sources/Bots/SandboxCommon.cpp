/* Copyright (c) 2018-2025 Dreamy Cecil
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
#include "StdH.h"
#include "Bots/Logic/BotItems.h"

// --- Helper functions

// [Cecil] 2019-06-04: Convert unsigned long into a binary number
CTString ULongToBinary(ULONG ul) {
  if (ul == 0) {
    return "0";
  }

  CTString strOut = "";

  while (ul) {
    strOut.InsertChar(0, (ul & 1) ? '1' : '0');
    ul >>= 1;
  }

  return strOut;
};

// [Cecil] 2020-07-29: Project 3D line onto 2D space
BOOL ProjectLine(CProjection3D *ppr, FLOAT3D vPoint1, FLOAT3D vPoint2, FLOAT3D &vOnScreen1, FLOAT3D &vOnScreen2) {
  vOnScreen1 = FLOAT3D(0.0f, 0.0f, 0.0f);
  vOnScreen2 = FLOAT3D(0.0f, 0.0f, 0.0f);

  // Project before clipping
  FLOAT3D vClip1, vClip2;
  ppr->PreClip(vPoint1, vClip1);
  ppr->PreClip(vPoint2, vClip2);

  ULONG ulClipFlags = ppr->ClipLine(vClip1, vClip2);

  // The edge remains after clipping
  if (ulClipFlags != LCF_EDGEREMOVED) {
    // Project points
    ppr->PostClip(vClip1, vOnScreen1);
    ppr->PostClip(vClip2, vOnScreen2);

    return TRUE;
  }

  return FALSE;
};

// [Cecil] 2018-10-28: Find an active entity by its ID
CEntity *FindEntityByID(CWorld *pwo, const INDEX &iEntityID) {
  // Invalid ID
  if (iEntityID < 0) return NULL;

  FOREACHINDYNAMICCONTAINER(pwo->wo_cenEntities, CEntity, iten) {
    CEntity *pen = iten;

    // If exists and is under the same ID
    if (!(pen->GetFlags() & ENF_DELETED) && pen->en_ulID == iEntityID) {
      return pen;
    }
  }

  // None found
  return NULL;
};

// [Cecil] 2021-06-16: Determine vertical position difference
FLOAT3D VerticalDiff(FLOAT3D vPosDiff, const FLOAT3D &vGravityDir) {
  // Vertical difference based on the gravity vector
  vPosDiff(1) *= vGravityDir(1);
  vPosDiff(2) *= vGravityDir(2);
  vPosDiff(3) *= vGravityDir(3);

  return vPosDiff;
};

// [Cecil] 2021-06-14: Determine position difference on the same plane
FLOAT3D HorizontalDiff(FLOAT3D vPosDiff, const FLOAT3D &vGravityDir) {
  // Remove vertical difference
  return vPosDiff + VerticalDiff(vPosDiff, vGravityDir);
};

// [Cecil] 2021-06-28: Get relative angles from the directed placement
FLOAT GetRelH(FLOAT3D vDesiredDir, const ANGLE3D &aCurrent) {
  FLOATmatrix3D mRot;
  MakeRotationMatrix(mRot, aCurrent);

  FLOAT3D vDir = vDesiredDir.SafeNormalize();

  // Get front component of vector
  FLOAT fFront = -vDir(1) * mRot(1, 3)
                 -vDir(2) * mRot(2, 3)
                 -vDir(3) * mRot(3, 3);

  // Get left component of vector
  FLOAT fLeft = -vDir(1) * mRot(1, 1)
                -vDir(2) * mRot(2, 1)
                -vDir(3) * mRot(3, 1);

  // Relative heading is arctan of angle between front and left
  return ATan2(fLeft, fFront);
};

FLOAT GetRelP(FLOAT3D vDesiredDir, const ANGLE3D &aCurrent) {
  FLOATmatrix3D mRot;
  MakeRotationMatrix(mRot, aCurrent);

  FLOAT3D vDir = vDesiredDir.SafeNormalize();

  // Get front component of vector
  FLOAT fFront = -vDir(1) * mRot(1, 3)
                 -vDir(2) * mRot(2, 3)
                 -vDir(3) * mRot(3, 3);

  // Get up component of vector
  FLOAT fUp = +vDir(1) * mRot(1, 2)
              +vDir(2) * mRot(2, 2)
              +vDir(3) * mRot(3, 2);

  // Relative pitch is arctan of angle between front and up
  return ATan2(fUp, fFront);
};

FLOAT2D GetRelAngles(FLOAT3D vDesiredDir, const ANGLE3D &aCurrent) {
  FLOATmatrix3D mRot;
  MakeRotationMatrix(mRot, aCurrent);

  FLOAT3D vDir = vDesiredDir.SafeNormalize();

  // Get right component of vector
  FLOAT fRight = +vDir(1) * mRot(1, 1)
                 +vDir(2) * mRot(2, 1)
                 +vDir(3) * mRot(3, 1);

  // Get up component of vector
  FLOAT fUp = +vDir(1) * mRot(1, 2)
              +vDir(2) * mRot(2, 2)
              +vDir(3) * mRot(3, 2);

  // Get back component of vector
  FLOAT fBack = +vDir(1) * mRot(1, 3)
                +vDir(2) * mRot(2, 3)
                +vDir(3) * mRot(3, 3);

  // Calculate pitch
  FLOAT2D vAngles(0, ASin(fUp));

  // Heading is irrelevant with perfectly vertical pitch
  if (fUp > 0.99 || fUp < -0.99) {
    vAngles(1) = 0;

  } else {
    vAngles(1) = ATan2(-fRight, -fBack);
  }

  return vAngles;
};

// [Cecil] 2020-07-29: Do the ray casting with specific passable flags
void CastRayFlags(CCastRay &cr, CWorld *pwoWorld, ULONG ulPass) {
  // initially no polygon is found
  cr.cr_pbpoBrushPolygon= NULL;
  cr.cr_pbscBrushSector = NULL;
  cr.cr_penHit = NULL;

  // [Cecil] 2020-07-29: Set own flags
  if (ulPass != 0) {
    cr.cr_ulPassablePolygons = ulPass;

  } else if (cr.cr_bPhysical) {
    cr.cr_ulPassablePolygons = BPOF_PASSABLE | BPOF_SHOOTTHRU;

  } else {
    cr.cr_ulPassablePolygons = BPOF_PORTAL | BPOF_OCCLUDER;
  }

  // if origin entity is given
  if (cr.cr_penOrigin != NULL) {
    // add all sectors around it
    cr.AddSectorsAroundEntity(cr.cr_penOrigin);

    // test all sectors recursively
    cr.TestThroughSectors();

  // if there is no origin entity
  } else {
    // test entire world against ray
    cr.TestWholeWorld(pwoWorld);
  }

  // calculate the hit point from the hit distance
  cr.cr_vHit = cr.cr_vOrigin + (cr.cr_vTarget - cr.cr_vOrigin).Normalize() * cr.cr_fHitDistance;
};

// [Cecil] 2018-10-15: Check if polygon is suitable for walking on
BOOL FlatPolygon(CWorld *wo, CBrushPolygon *pbpo) {
  // check its type
  INDEX iSurface = pbpo->bpo_bppProperties.bpp_ubSurfaceType;
  CSurfaceType &st = wo->wo_astSurfaceTypes[iSurface];

  // compare planes
  FLOAT3D vPlane = -(FLOAT3D&)pbpo->bpo_pbplPlane->bpl_plAbsolute;
  ANGLE3D aCur, aPol;
  // [Cecil] NOTE: Should be a gravity vector here, which is hard to get from a sector, so just point down
  DirectionVectorToAngles(FLOAT3D(0.0f, -1.0f, 0.0f), aCur);
  DirectionVectorToAngles(vPlane, aPol);

  // find the difference
  aPol -= aCur;

  // [Cecil] 2021-06-18: Determine angle difference based on a stairs flag
  FLOAT fAngleDiff = (pbpo->bpo_ulFlags & BPOF_STAIRS) ? 85.0f : 45.0f;

  // it's a suitable polygon if the plane is not slippery and isn't vertical
  return (st.st_fFriction >= 1.0f && aPol(2) <= fAngleDiff && aPol(3) <= fAngleDiff);
};

// Check for class names, rather than DLL classes
extern INDEX MOD_bCheckClassNames;

// [Cecil] Check if entity is of given DLL class
BOOL IsOfDllClass(CEntity *pen, const CDLLEntityClass &dec) {
  if (MOD_bCheckClassNames) {
    return IsOfClass(pen, dec.dec_strName);
  }

  if (pen == NULL) {
    return FALSE;
  }

  if (pen->GetClass()->ec_pdecDLLClass == &dec) {
    return TRUE;
  }

  return FALSE;
};

// [Cecil] Check if entity is of given DLL class or derived from it
BOOL IsDerivedFromDllClass(CEntity *pen, const CDLLEntityClass &dec) {
  if (MOD_bCheckClassNames) {
    return IsDerivedFromClass(pen, dec.dec_strName);
  }

  if (pen == NULL) {
    return FALSE;
  }

  // for all classes in hierarchy of the entity
  for (CDLLEntityClass *pdecDLLClass = pen->GetClass()->ec_pdecDLLClass;
       pdecDLLClass != NULL;
       pdecDLLClass = pdecDLLClass->dec_pdecBase) {
    // the same DLL class
    if (pdecDLLClass == &dec) {
      return TRUE;
    }
  }

  return FALSE;
};

// --- Replacement functions

// [Cecil] 2021-06-12: Looping through players and bots
INDEX CECIL_GetMaxPlayers(void) {
  return CEntity::GetMaxPlayers() + _aPlayerBots.Count();
};

CPlayer *CECIL_GetPlayerEntity(const INDEX &iPlayer) {
  const INDEX ctReal = CEntity::GetMaxPlayers();

  // prioritize players
  if (iPlayer < ctReal) {
    return (CPlayer *)CEntity::GetPlayerEntity(iPlayer);
  }

  return (CPlayer *)_aPlayerBots[iPlayer - ctReal].pen;
};

// [Cecil] 2021-06-13: Get personal player index
INDEX CECIL_PlayerIndex(CPlayerEntity *pen) {
  INDEX ctPlayers = CEntity::GetMaxPlayers();

  if (IsDerivedFromDllClass(pen, CPlayerBot_DLLClass)) {
    INDEX iBot = FindBotByPointer(pen);

    // occupy the rest of the bits by bots
    return ctPlayers + (iBot % (32 - ctPlayers));
  }

  return pen->GetMyPlayerIndex();
};
