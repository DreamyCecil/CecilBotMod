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

#ifndef _CECILBOTS_NAVMESH_H
#define _CECILBOTS_NAVMESH_H

#include "PathPoint.h"

// [Cecil] 2021-06-17: NavMesh generation types
#define NAVMESH_TRIANGLES 0 // on each triangle of a polygon
#define NAVMESH_POLYGONS  1 // on full polygons
#define NAVMESH_EDGES     2 // on each edge of a polygon

// Current NavMesh generation type
#define NAVMESH_GEN_TYPE NAVMESH_EDGES

// [Cecil] 2022-04-17: Current NavMesh version
#define CURRENT_NAVMESH_VERSION 6

// [Cecil] 2021-09-09: Legacy path point version
#define LEGACY_PATHPOINT_VERSION 4

// [Cecil] 2018-10-23: Bot NavMesh
class DECL_DLL CBotNavmesh {
  public:
    // All path points
    CDynamicContainer<CBotPathPoint> bnm_cbppPoints;
    // All brush polygons in the world
    CStaticArray<CBrushPolygon *> bnm_apbpoPolygons;
    // World for this NavMesh
    CWorld *bnm_pwoWorld; 

    BOOL bnm_bGenerated; // has NavMesh been generated or not
    INDEX bnm_iNextPointID; // index for the next point

    // Find next point in the NavMesh
    CBotPathPoint *FindNextPoint(CBotPathPoint *bppSrc, CBotPathPoint *bppDst);
    CBotPathPoint *ReconstructPath(CPathPoint *ppCurrent);

    // Constructor & Destructor
    CBotNavmesh(void);
    ~CBotNavmesh(void);

    // Writing & Reading
    void WriteNavmesh(CTStream *strm);
    void ReadNavmesh(CTStream *strm);

    // Saving & Loading for a specific world
    void SaveNavmesh(CWorld &wo);
    void LoadNavmesh(CWorld &wo);

    // Clear the NavMesh
    void ClearNavMesh(void);

    // Add a new path point to the navmesh
    CBotPathPoint *AddPoint(const FLOAT3D &vPoint, CBotPathPolygon *bppo);
    // Find a point by its ID
    CBotPathPoint *FindPointByID(const INDEX &iPoint);
    // Find some important point
    CBotPathPoint *FindImportantPoint(SPlayerBot &pb, const INDEX &iPoint);

    // Generate the NavMesh
    void GenerateNavmesh(CWorld *pwo);
    // Connect all points together
    void ConnectPoints(const INDEX &iPoint);
};

// [Cecil] 2018-10-23: Bot NavMesh
DECL_DLL extern CBotNavmesh *_pNavmesh;

#endif // _CECILBOTS_NAVMESH_H
