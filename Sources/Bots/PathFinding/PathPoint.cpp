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

#include "StdH.h"
#include "PathPoint.h"
#include "Navmesh.h"

// Constructor & Destructor
CBotPathPoint::CBotPathPoint(void) {
  Reset();
};

CBotPathPoint::~CBotPathPoint(void) {
  Clear();
};

// Reset path point
void CBotPathPoint::Reset(void) {
  bpp_iIndex = -1;
  bpp_vPos = FLOAT3D(0.0f, 0.0f, 0.0f);
  bpp_fRange = 1.0f; // Normal range for walking points
  bpp_ulFlags = 0;
  bpp_penImportant = NULL;
  bpp_pbppNext = NULL;
  bpp_penLock = NULL;
  bpp_plLockOrigin = CPlacement3D(FLOAT3D(0.0f, 0.0f, 0.0f), ANGLE3D(0.0f, 0.0f, 0.0f));

  bpp_bppoPolygon = NULL;
};

// Clear path point
void CBotPathPoint::Clear(void) {
  // Clear connections
  bpp_cbppPoints.Clear();

  // Destroy the polygon
  if (bpp_bppoPolygon != NULL) {
    delete bpp_bppoPolygon;
    bpp_bppoPolygon = NULL;
  }

  Reset();
};

// Writing & Reading
void CBotPathPoint::WritePoint(CTStream *strm) {
  strm->WriteID_t(CChunkID("BPPH")); // Bot Path Point Header

  *strm << bpp_iIndex;
  *strm << bpp_vPos;
  *strm << bpp_fRange;
  *strm << bpp_ulFlags;

  // write important entity
  if (bpp_penImportant != NULL) {
    *strm << INDEX(bpp_penImportant->en_ulID);
  } else {
    *strm << INDEX(-1);
  }

  // write next important point
  if (bpp_pbppNext != NULL) {
    *strm << _pNavmesh->bnm_aPoints.Index(bpp_pbppNext);
  } else {
    *strm << INDEX(-1);
  }

  // write lock entity
  if (bpp_penLock != NULL) {
    *strm << INDEX(bpp_penLock->en_ulID);
    *strm << bpp_plLockOrigin;
  } else {
    *strm << INDEX(-1);
  }

  // write possible connections
  *strm << (bpp_cbppPoints.Count());
  
  // write indices
  FOREACHINDYNAMICCONTAINER(bpp_cbppPoints, CBotPathPoint, itbpp) {
    INDEX iPoint = _pNavmesh->bnm_aPoints.Index(itbpp);
    *strm << iPoint;
  }

  // write the polygon
  *strm << UBYTE(bpp_bppoPolygon != NULL);

  if (bpp_bppoPolygon != NULL) {
    bpp_bppoPolygon->WritePolygon(strm);
  }
};

void CBotPathPoint::ReadPoint(CTStream *strm, INDEX iVersion) {
  INDEX iImportantEntity = -1;
  INDEX iNext = -1;
  INDEX iLockEntity = -1;

  // legacy version support
  if (strnicmp(strm->PeekID_t(), "NMP", 3) == 0) {
    // read version 4
    strm->ExpectID_t(CChunkID("NMP4"));
    iVersion = LEGACY_PATHPOINT_VERSION;

  } else {
    strm->ExpectID_t(CChunkID("BPPH")); // Bot Path Point Header
  }

  // main properties (version 4)
  *strm >> bpp_iIndex;
  *strm >> bpp_vPos;
  *strm >> bpp_fRange;
  *strm >> bpp_ulFlags;
  *strm >> iImportantEntity;
  *strm >> iNext;

  // new versions
  switch (iVersion) {
    case 5:
      *strm >> iLockEntity;

      if (iLockEntity != -1) {
        strm->Read_t(&bpp_plLockOrigin.pl_PositionVector, sizeof(FLOAT3D));
      }
      break;

    case 6:
      *strm >> iLockEntity;

      if (iLockEntity != -1) {
        *strm >> bpp_plLockOrigin;
      }
      break;
  }

  // set important entity
  bpp_penImportant = FindEntityByID(&_pNetwork->ga_World, iImportantEntity);
  
  // set next important point
  if (iNext != -1) {
    bpp_pbppNext = &_pNavmesh->bnm_aPoints[iNext];
  } else {
    bpp_pbppNext = NULL;
  }

  // set lock entity
  bpp_penLock = FindEntityByID(&_pNetwork->ga_World, iLockEntity);

  // read possible connections
  INDEX ctConnections;
  *strm >> ctConnections;

  while (ctConnections > 0) {
    INDEX iPoint;
    *strm >> iPoint;

    CBotPathPoint *pbpp = &_pNavmesh->bnm_aPoints[iPoint];
    bpp_cbppPoints.Add(pbpp);

    ctConnections--;
  }

  // read the polygon
  UBYTE bPolygon;
  *strm >> bPolygon;

  if (bPolygon) {
    bpp_bppoPolygon = new CPathPolygon;
    bpp_bppoPolygon->ReadPolygon(strm);
  }
};

// Path points comparison
BOOL CBotPathPoint::operator==(const CBotPathPoint &bppOther) const {
  return (this->bpp_iIndex == bppOther.bpp_iIndex);
};

// Check if the point is locked (cannot be passed through)
BOOL CBotPathPoint::IsLocked(void) {
  // no lock entity
  if (bpp_penLock == NULL) {
    return FALSE;
  }

  // lock entity is away from its origin
  return (bpp_penLock->GetPlacement().pl_PositionVector - bpp_plLockOrigin.pl_PositionVector).Length() > 0.1f
      || (bpp_penLock->GetPlacement().pl_OrientationAngle - bpp_plLockOrigin.pl_OrientationAngle).Length() > 0.1f;
};

// Make a connection with a specific point
void CBotPathPoint::Connect(CBotPathPoint *pbppPoint, INDEX iType) {
  // same point
  if (pbppPoint == this) {
    return;
  }

  // connect to the target
  if (iType == 1 || iType == 2) {
    if (!bpp_cbppPoints.IsMember(pbppPoint)) {
      bpp_cbppPoints.Add(pbppPoint);
    }
  }

  // connect target to this one
  if (iType == 2 || iType == 3) {
    if (!pbppPoint->bpp_cbppPoints.IsMember(this)) {
      pbppPoint->bpp_cbppPoints.Add(this);
    }
  }
};
