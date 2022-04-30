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
#include "PathPolygon.h"
#include "Navmesh.h"

// Constructor & Destructor
CBotPathPolygon::CBotPathPolygon(void) {
  #if NAVMESH_GEN_TYPE == NAVMESH_POLYGONS
    bppo_avVertices.Clear();

  #else
    bppo_avVertices.New(3);

    bppo_avVertices[0] = FLOAT3D(0.0f, 0.0f, 0.0f);
    bppo_avVertices[1] = FLOAT3D(0.0f, 0.0f, 0.0f);
    bppo_avVertices[2] = FLOAT3D(0.0f, 0.0f, 0.0f);
  #endif
};

CBotPathPolygon::~CBotPathPolygon(void) {
  bppo_avVertices.Clear();
};

// Writing & Reading
void CBotPathPolygon::WritePolygon(CTStream *strm) {
  strm->WriteID_t("PPO2"); // Path POlygon v2

  // write vertex count
  *strm << bppo_avVertices.Count();
  
  // write vertices
  for (INDEX iVtx = 0; iVtx < bppo_avVertices.Count(); iVtx++) {
    *strm << bppo_avVertices[iVtx];
  }
};

void CBotPathPolygon::ReadPolygon(CTStream *strm) {
  if (strm->PeekID_t() == CChunkID("PPO1")) {
    strm->ExpectID_t("PPO1"); // Path POlygon v1
    
    // read three vertices
    bppo_avVertices.New(3);
    *strm >> bppo_avVertices[0] >> bppo_avVertices[1] >> bppo_avVertices[2];

  } else {
    strm->ExpectID_t("PPO2"); // Path POlygon v2
    
    // read vertex count
    INDEX ctVtx;
    *strm >> ctVtx;
    
    // read vertices
    bppo_avVertices.New(ctVtx);

    for (INDEX iVtx = 0; iVtx < ctVtx; iVtx++) {
      *strm >> bppo_avVertices[iVtx];
    }
  }
};

// Absolute center position of this polygon
FLOAT3D CBotPathPolygon::Center(void) {
  #if NAVMESH_GEN_TYPE == NAVMESH_POLYGONS
    FLOAT3D vCenter = FLOAT3D(0.0f, 0.0f, 0.0f);

    for (INDEX i = 0; i < bppo_avVertices.Count(); i++) {
      vCenter += bppo_avVertices[i];
    }

    return vCenter / (FLOAT)Max((INDEX)bppo_avVertices.Count(), (INDEX)1);

  #else
    FLOAT3D vCenter = bppo_avVertices[0] + bppo_avVertices[1] + bppo_avVertices[2];
    return vCenter / 3.0f;
  #endif
};
