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

#ifndef _CECILBOTS_PATHPOINT_H
#define _CECILBOTS_PATHPOINT_H

#include "PathPolygon.h"

// Path Point Flags (rearranging them can break previously created NavMeshes)
#define PPF_WALK        (1 << 0) // don't run from this point
#define PPF_JUMP        (1 << 1) // jump from this point
#define PPF_CROUCH      (1 << 2) // crouch while going from this point
#define PPF_OVERRIDE    (1 << 3) // apply point's flags before reaching the point
#define PPF_UNREACHABLE (1 << 4) // cannot be reached directly by foot (only for target points)
#define PPF_TELEPORT    (1 << 5) // acts as a teleport and has 0 distance to any target point

// [Cecil] 2018-10-22: Bot Path Points
class DECL_DLL CBotPathPoint {
  public:
    INDEX bpp_iIndex;  // personal ID of this point in the NavMesh
    FLOAT3D bpp_vPos;  // position of this point
    FLOAT bpp_fRange;  // walking radius of a point
    ULONG bpp_ulFlags; // special point flags
    // [Cecil] TODO: Add ability to just mark points as "important" without any entity attached to them
    CEntityPointer bpp_penImportant; // important entity
    CBotPathPoint *bpp_pbppNext; // next important point
    // [Cecil] TODO: Add defending time which would force bots to stay on important points for some time
    //FLOAT bpp_fDefendTime;

    CEntityPointer bpp_penLock; // entity that locks the point if it's not on the origin position
    CPlacement3D bpp_plLockOrigin; // origin placement of the locking entity

    // Polygon of this point
    CPathPolygon *bpp_bppoPolygon;

    // Possible connections
    CDynamicContainer<CBotPathPoint> bpp_cbppPoints;

  public:
    // Constructor & Destructor
    CBotPathPoint(void);
    ~CBotPathPoint(void);

    // Reset path point
    void Reset(void);

    // Clear path point
    void Clear(void);

    // Writing & Reading
    void WritePoint(CTStream *strm);
    void ReadPoint(CTStream *strm, INDEX iVersion);

    // Path points comparison
    BOOL operator==(const CBotPathPoint &bppOther) const;

    // Check if the point is locked (cannot be passed through)
    BOOL IsLocked(void);

    // Make a connection with a specific point
    void Connect(CBotPathPoint *pbppPoint, INDEX iType);
};

// [Cecil] Path points in open and closed lists (only for path finding)
class CPathPoint {
  public:
    CBotPathPoint *pp_bppPoint;
    CPathPoint *pp_ppFrom;
    FLOAT pp_fG;
    FLOAT pp_fH;
    FLOAT pp_fF;

    CPathPoint() :
      pp_bppPoint(NULL), pp_ppFrom(NULL),
      pp_fG(-1.0f), // infinity
      pp_fH( 0.0f), // nothing
      pp_fF(-1.0f)  // infinity
    {};
};

#endif // _CECILBOTS_PATHPOINT_H
