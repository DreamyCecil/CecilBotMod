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
#include "PathPolygon.h"
#include "Navmesh.h"

// Constructor & Destructor
CPathPolygon::CPathPolygon(void)
{
};

CPathPolygon::~CPathPolygon(void) {
  bppo_avVertices.Clear();
};

// Copy constructor
CPathPolygon::CPathPolygon(const CPathPolygon &bppoOther) {
  bppo_bpoPolygon = bppoOther.bppo_bpoPolygon;

  INDEX ct = bppoOther.bppo_avVertices.Count();

  for (INDEX i = 0; i < ct; i++) {
    bppo_avVertices.Push() = bppoOther.bppo_avVertices[i];
  }
};

// Writing & Reading
void CPathPolygon::WritePolygon(CTStream *strm) {
  strm->WriteID_t("PPO2"); // Path POlygon v2

  // write vertex count
  INDEX ctVtx = bppo_avVertices.Count();
  *strm << ctVtx;

  // write vertices
  for (INDEX iVtx = 0; iVtx < ctVtx; iVtx++) {
    *strm << bppo_avVertices[iVtx];
  }
};

void CPathPolygon::ReadPolygon(CTStream *strm) {
  if (strm->PeekID_t() == CChunkID("PPO1")) {
    strm->ExpectID_t("PPO1"); // Path POlygon v1

    // read three vertices
    FLOAT3D vVtx;

    for (INDEX iVtx = 0; iVtx < 3; iVtx++) {
      *strm >> vVtx;
      bppo_avVertices.Push() = vVtx;
    }

  } else {
    strm->ExpectID_t("PPO2"); // Path POlygon v2

    // read vertex count
    INDEX ctVtx;
    *strm >> ctVtx;

    // read vertices
    FLOAT3D vVtx;

    for (INDEX iVtx = 0; iVtx < ctVtx; iVtx++) {
      *strm >> vVtx;
      bppo_avVertices.Push() = vVtx;
    }
  }
};

// Absolute center position of this polygon
FLOAT3D CPathPolygon::Center(void) {
  #if NAVMESH_GEN_TYPE != NAVMESH_TRIANGLES
    FLOAT3D vCenter = FLOAT3D(0.0f, 0.0f, 0.0f);
    const INDEX ctVertices = bppo_avVertices.Count();

    if (ctVertices == 0) {
      return vCenter;
    }

    for (INDEX i = 0; i < ctVertices; i++) {
      vCenter += bppo_avVertices[i];
    }

    return vCenter / ctVertices;

  #else
    FLOAT3D vCenter = bppo_avVertices[0] + bppo_avVertices[1] + bppo_avVertices[2];
    return vCenter / 3.0f;
  #endif
};
