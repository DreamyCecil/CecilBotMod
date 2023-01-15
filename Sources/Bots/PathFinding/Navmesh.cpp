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
#include "Navmesh.h"
#include "PathPoint.h"

// [Cecil] 2021-06-16: Only for ImportantForNavMesh() function
#include "Bots/Logic/BotFunctions.h"

// [Cecil] 2018-10-23: Bot NavMesh
extern CBotNavmesh *_pNavmesh = NULL;

// Constructor & Destructor
CBotNavmesh::CBotNavmesh(void) {
  bnm_pwoWorld = NULL;
  bnm_bGenerated = FALSE;
  bnm_iNextPointID = 0;
};

CBotNavmesh::~CBotNavmesh(void) {
  ClearNavMesh();
};

// Writing & Reading
void CBotNavmesh::WriteNavmesh(CTStream *strm) {
  strm->WriteID_t("BNMV"); // Bot NavMesh Version
  *strm << INDEX(CURRENT_NAVMESH_VERSION); // latest NavMesh version

  INDEX ctPoints = bnm_cbppPoints.Count();

  *strm << bnm_bGenerated; // write if generated or not
  *strm << bnm_iNextPointID; // next point ID
  *strm << ctPoints; // amount of points

  // write points
  for (INDEX iPoint = 0; iPoint < ctPoints; iPoint++) {
    CBotPathPoint *pbpp = bnm_cbppPoints.Pointer(iPoint);
    pbpp->WritePoint(strm);
  }
};

void CBotNavmesh::ReadNavmesh(CTStream *strm) {
  INDEX iVersion;
  INDEX ctPoints;

  if (strm->PeekID_t() == CChunkID("BNM1")) {
    strm->ExpectID_t("BNM1"); // Bot NavMesh v1
    iVersion = LEGACY_PATHPOINT_VERSION; // last legacy path point version

  } else {
    strm->ExpectID_t("BNMV"); // Bot NavMesh Version
    *strm >> iVersion;
  }

  *strm >> bnm_bGenerated; // read if generated or not
  *strm >> bnm_iNextPointID; // next point ID
  *strm >> ctPoints; // amount of points

  // create points
  INDEX iPoint;

  for (iPoint = 0; iPoint < ctPoints; iPoint++) {
    CBotPathPoint *bppNew = new CBotPathPoint();
    bnm_cbppPoints.Add(bppNew);
  }

  // read points
  for (iPoint = 0; iPoint < ctPoints; iPoint++) {
    CBotPathPoint *pbpp = &bnm_cbppPoints[iPoint];
    pbpp->ReadPoint(strm, iVersion);
  }
};

// Saving & Loading
void CBotNavmesh::SaveNavmesh(CWorld &wo) {
  const CTFileName &fnWorld = wo.wo_fnmFileName;

  // Get level path hash and append it to the level filename
  CTFileName fnFile;
  fnFile.PrintF("Cecil\\Navmeshes\\%s_%08X.nav", fnWorld.FileName().str_String, fnWorld.GetHash());
  
  CTFileStream strm;
  strm.Create_t(fnFile);
            
  bnm_pwoWorld = &wo;
  WriteNavmesh(&strm);

  CPrintF("Saved NavMesh for the current map into '%s'\n", fnFile.str_String);
  strm.Close();
};

void CBotNavmesh::LoadNavmesh(CWorld &wo) {
  const CTFileName &fnWorld = wo.wo_fnmFileName;

  // Get navmesh file using a level path hash
  CTFileName fnFile;
  fnFile.PrintF("Cecil\\Navmeshes\\%s_%08X.nav", fnWorld.FileName().str_String, fnWorld.GetHash());
  
  CTFileStream strm;
  strm.Open_t(fnFile);

  ClearNavMesh();

  bnm_pwoWorld = &wo;
  ReadNavmesh(&strm);

  CPrintF("Loaded NavMesh for the current map from '%s'\n", fnFile.str_String);
  strm.Close();
};

void CBotNavmesh::ClearNavMesh(void) {
  // [Cecil] 2021-06-22: Untarget all bots
  for (INDEX iBot = 0; iBot < _aPlayerBots.Count(); iBot++) {
    CPlayerBot *penBot = (CPlayerBot *)_aPlayerBots[iBot].pen;

    penBot->GetProps().m_pbppCurrent = NULL;
    penBot->GetProps().m_pbppTarget = NULL;
  }

  // destroy all path points
  for (INDEX iDeletePoint = 0; iDeletePoint < bnm_cbppPoints.Count(); iDeletePoint++) {
    CBotPathPoint *pbpp = bnm_cbppPoints.Pointer(iDeletePoint);
    delete pbpp;
  }

  bnm_cbppPoints.Clear();

  // clear pointers to polygons
  bnm_apbpoPolygons.Clear();

  // ready for the next generation
  bnm_bGenerated = FALSE;
  bnm_iNextPointID = 0;
};

// Add a new path point to the navmesh
CBotPathPoint *CBotNavmesh::AddPoint(const FLOAT3D &vPoint, CPathPolygon *bppo) {
  CBotPathPoint *bppNew = new CBotPathPoint;
  bppNew->bpp_iIndex = bnm_iNextPointID++;
  bppNew->bpp_vPos = vPoint;
  bppNew->bpp_bppoPolygon = bppo;
  bnm_cbppPoints.Add(bppNew);

  return bppNew;
};

// Find a point by its ID
CBotPathPoint *CBotNavmesh::FindPointByID(INDEX iPoint) {
  // ID can't be negative
  if (iPoint < 0) {
    return NULL;
  }

  FOREACHINDYNAMICCONTAINER(bnm_cbppPoints, CBotPathPoint, itbpp) {
    CBotPathPoint *pbpp = itbpp;

    if (pbpp->bpp_iIndex == iPoint) {
      return pbpp;
    }
  }

  return NULL;
};

// Find some important point
CBotPathPoint *CBotNavmesh::FindImportantPoint(CPlayerBotController &pb, INDEX iPoint) {
  if (bnm_pwoWorld == NULL) {
    ASSERT(FALSE);

    CPrintF("NavMesh::FindImportantPoint : Cannot find the world!\n");
    return NULL;
  }

  // create array of points
  CDynamicContainer<CBotPathPoint> cbpp;
  INDEX iImportantPoint = 0;

  // [Cecil] 2020-07-28: Reference entity
  CEntity *penReference = NULL;

  FOREACHINDYNAMICCONTAINER(bnm_cbppPoints, CBotPathPoint, itbpp) {
    CBotPathPoint *pbpp = itbpp;

    // not important
    if (pbpp->bpp_penImportant == NULL) {
      continue;
    }

    CEntity *penEntity = pbpp->bpp_penImportant;
    
    // not important at the moment
    if (penEntity == NULL || !ImportantForNavMesh(pb, penEntity)) {
      continue;
    }

    penReference = penEntity;
    cbpp.Add(pbpp);
    iImportantPoint++;
  }

  if (iImportantPoint > 0) {
    CBotPathPoint *pbppReturn = NULL;
    INDEX iIndex = penReference->IRnd() % iImportantPoint;

    // pick specific point
    if (iPoint >= 0) {
      iIndex = Clamp(iPoint, (INDEX)0, INDEX(iImportantPoint - 1));
      pbppReturn = cbpp.Pointer(iIndex);
      cbpp.Clear();

      return pbppReturn;
    }

    // pick random point
    pbppReturn = cbpp.Pointer(iIndex);
    cbpp.Clear();

    return pbppReturn;
  }

  return NULL;
};

// Get vertices of a specific polygon triangle
static void GetPolygonTriangle(CBrushPolygon *pbpo, INDEX iTriangle, CDynamicContainer<CBrushVertex> &cOutput) {
  // Go through triangle vertices
  for (INDEX iEnd = 0; iEnd < 3; iEnd++) {
    // Get transformed end vertices
    INDEX iElem0 = pbpo->bpo_aiTriangleElements[iTriangle * 3 + (iEnd + 0)];
    INDEX iElem1 = pbpo->bpo_aiTriangleElements[iTriangle * 3 + (iEnd + 1) % 3];
    INDEX iElem2 = pbpo->bpo_aiTriangleElements[iTriangle * 3 + (iEnd + 2) % 3];

    cOutput.Add(pbpo->bpo_apbvxTriangleVertices[iElem0]);
    cOutput.Add(pbpo->bpo_apbvxTriangleVertices[iElem1]);
    cOutput.Add(pbpo->bpo_apbvxTriangleVertices[iElem2]);
  }
};

void CBotNavmesh::GenerateNavmesh(CWorld *pwo) {
  if (bnm_bGenerated) {
    CPrintF("Already generated!\n");
    return;
  }

  bnm_pwoWorld = pwo;
  bnm_apbpoPolygons.New(pwo->wo_baBrushes.ba_apbpo.Count());

  INDEX iPoly = 0;

  // Go through all brush polygons
  FOREACHINSTATICARRAY(pwo->wo_baBrushes.ba_apbpo, CBrushPolygon *, itbpo) {
    CBrushPolygon *pbpo = itbpo.Current();
    
    // Add every polygon to the NavMesh
    bnm_apbpoPolygons[iPoly] = pwo->wo_baBrushes.ba_apbpo[iPoly];
    iPoly++;

    // Skip passable polygons
    if (pbpo->bpo_ulFlags & BPOF_PASSABLE) {
      continue;
    }

    // Skip field brushes
    CBrush3D *pbr = pbpo->bpo_pbscSector->bsc_pbmBrushMip->bm_pbrBrush;

    if (pbr->br_penEntity->GetRenderType() == CEntity::RT_FIELDBRUSH) {
      continue;
    }

    // Not a flat polygon
    if (!FlatPolygon(pwo, pbpo)) {
      continue;
    }

    #if NAVMESH_GEN_TYPE != NAVMESH_TRIANGLES
      INDEX ctVtx = pbpo->bpo_aiTriangleElements.Count();

      // Create path polygon
      CPathPolygon *bppoNew = new CPathPolygon;
      bppoNew->bppo_bpoPolygon = pbpo;

      // Center position
      FLOAT3D vPoint = FLOAT3D(0.0f, 0.0f, 0.0f);

      for (INDEX iVtx = 0; iVtx < ctVtx; iVtx++) {
        // Get polygon vertex
        INDEX iElement = pbpo->bpo_aiTriangleElements[iVtx];
        CBrushVertex *pbvx = pbpo->bpo_apbvxTriangleVertices[iElement];

        // Add new vertex position
        bppoNew->bppo_avVertices.Push() = pbvx->bvx_vAbsolute;

        vPoint += pbvx->bvx_vAbsolute;
      }

      // Average position
      vPoint /= ctVtx;

      // Shift it up a little bit
      vPoint(2) += 0.5f;
      
      // Check if there's a similar point already
      CBotPathPoint *pbppCheck = NULL;
      
      FOREACHINDYNAMICCONTAINER(bnm_cbppPoints, CBotPathPoint, itbpp) {
        pbppCheck = itbpp;

        FLOAT fDiff = (pbppCheck->bpp_vPos - vPoint).Length();

        if (fDiff <= 1.0f) {
          break;
        }

        pbppCheck = NULL;
      }

      if (pbppCheck == NULL) {
        // Create a new point
        AddPoint(vPoint, bppoNew);

      } else {
        // Move vertices from this polygon to the similar one
        if (pbppCheck->bpp_bppoPolygon != NULL) {
          CStaticStackArray<FLOAT3D> &avVertices = pbppCheck->bpp_bppoPolygon->bppo_avVertices;

          for (INDEX iMoveVtx = 0; iMoveVtx < ctVtx; iMoveVtx++) {
            avVertices.Push() = bppoNew->bppo_avVertices[iMoveVtx];
          }
        }

        // Delete polygon
        delete bppoNew;
      }
    
    #else
      INDEX ctTris = pbpo->bpo_aiTriangleElements.Count() / 3;

      for (INDEX iTri = 0; iTri < ctTris; iTri++) {
        CDynamicContainer<CBrushVertex> cVtx;
        GetPolygonTriangle(pbpo, iTri, cVtx);

        // Create path polygon
        CPathPolygon *bppoNew = new CPathPolygon;
        bppoNew->bppo_avVertices.Push() = cVtx[0].bvx_vAbsolute;
        bppoNew->bppo_avVertices.Push() = cVtx[1].bvx_vAbsolute;
        bppoNew->bppo_avVertices.Push() = cVtx[2].bvx_vAbsolute;
        bppoNew->bppo_bpoPolygon = pbpo;

        // Find center position
        FLOAT3D vPoint = ((cVtx[0].bvx_vAbsolute)
                        + (cVtx[1].bvx_vAbsolute)
                        + (cVtx[2].bvx_vAbsolute)) / 3.0f;
        // Shift it up a little bit
        vPoint(2) += 0.5f;

        // Check if there's a similar point already
        BOOL bSkip = FALSE;

        FOREACHINDYNAMICCONTAINER(bnm_cbppPoints, CBotPathPoint, itbpp) {
          CBotPathPoint *pbppCheck = itbpp;

          FLOAT fDiff = (pbppCheck->bpp_vPos - vPoint).Length();

          if (fDiff <= 1.0f) {
            bSkip = TRUE;
            break;
          }
        }

        if (!bSkip) {
          // Create a new point
          AddPoint(vPoint, bppoNew);

        } else {
          // Delete polygon
          delete bppoNew;
        }
      }
    #endif
  }

  CPrintF("%d polygons, generated %d points\n", bnm_apbpoPolygons.Count(), bnm_cbppPoints.Count());
};

void CBotNavmesh::ConnectPoints(INDEX iPoint) {
  if (iPoint < 0 || iPoint >= bnm_cbppPoints.Count()) {
    return;
  }

  CBotPathPoint *bppCurrent = &bnm_cbppPoints[iPoint];

  // no polygon
  if (bppCurrent->bpp_bppoPolygon == NULL) {
    return;
  }

  // brush polygon of this point
  CBrushPolygon *pbpoCurrent = bppCurrent->bpp_bppoPolygon->bppo_bpoPolygon;

  INDEX ctConnections = 0;

  // For each point, go through all points again
  INDEX ctPoints = bnm_cbppPoints.Count();

  for (INDEX iPointIter = 0; iPointIter < ctPoints; iPointIter++) {
    CBotPathPoint *bppTarget = bnm_cbppPoints.Pointer(iPointIter);

    // Skip itself, with no polygon, existing targets, high points
    if (bppCurrent == bppTarget || bppTarget->bpp_bppoPolygon == NULL
     || bppCurrent->bpp_cbppPoints.IsMember(bppTarget)
     /*|| bppTarget->bpp_vPos(2) - bppCurrent->bpp_vPos(2) > 3.5f*/) {
      continue;
    }

    BOOL bConnect = FALSE;
    CBotPathPoint *bppMiddle = NULL;

    // Try to connect close vertices
    INDEX iVertices = 0;
    FLOAT3D vVertexPos[2];

    // Brush polygon of the target point
    CBrushPolygon *pbpoTarget = bppTarget->bpp_bppoPolygon->bppo_bpoPolygon;

    #if NAVMESH_GEN_TYPE == NAVMESH_EDGES
      INDEX ctTris = pbpoCurrent->bpo_aiTriangleElements.Count() / 3;
      
      for (INDEX iTri = 0; iTri < ctTris; iTri++) {
        for (INDEX iEnd = 0; iEnd < 3; iEnd++) {
          // Get transformed end vertex
          INDEX iElement = pbpoCurrent->bpo_aiTriangleElements[iTri * 3 + iEnd];

          // Get vertex position
          FLOAT3D vVtx = pbpoCurrent->bpo_apbvxTriangleVertices[iElement]->bvx_vAbsolute;

          // Distance from the vertex to one of the edges on the target polygon
          FLOAT fDist = pbpoTarget->GetDistanceFromEdges(vVtx);

          // If close enough
          if (fDist <= 0.5f) {
            // Remember vertex position
            if (iVertices < 2) {
              vVertexPos[iVertices] = vVtx;
            }

            // Count it
            iVertices++;
          }

          // Create middle point that connects these two points
          if (iVertices >= 2) {
            FLOAT3D vMiddlePoint = (vVertexPos[0] + vVertexPos[1]) / 2.0f;
            vMiddlePoint(2) += 0.5f;

            bppMiddle = AddPoint(vMiddlePoint, NULL);

            bConnect = TRUE;
            break;
          }
        }
      }

    #else
      #if NAVMESH_GEN_TYPE == NAVMESH_POLYGONS
        #define CT_POLYGON_VTX(_Point) _Point->bpp_bppoPolygon->bppo_avVertices.Count()
      #else
        #define CT_POLYGON_VTX(_Point) 3
      #endif

      for (INDEX iVtx1 = 0; iVtx1 < CT_POLYGON_VTX(bppCurrent); iVtx1++) {
        for (INDEX iVtx2 = 0; iVtx2 < CT_POLYGON_VTX(bppTarget); iVtx2++) {
          FLOAT3D vDist = (bppCurrent->bpp_bppoPolygon->bppo_avVertices[iVtx1] - bppTarget->bpp_bppoPolygon->bppo_avVertices[iVtx2]);

          // if close enough
          if (vDist.Length() <= 0.5f) {
            // remember vertex position
            if (iVertices < 2) {
              vVertexPos[iVertices] = bppCurrent->bpp_bppoPolygon->bppo_avVertices[iVtx1];
            }

            // count it
            iVertices++;
          }

          // create middle point that connects these two points
          if (iVertices >= 2) {
            FLOAT3D vMiddlePoint = (vVertexPos[0] + vVertexPos[1]) / 2.0f;
            vMiddlePoint(2) += 0.5f;
            //bppMiddle = AddPoint(vMiddlePoint, NULL);

            bConnect = TRUE;
            break;
          }
        }
      }
    #endif

    // connect the points
    if (bConnect) {
      if (bppMiddle == NULL) {
        bppCurrent->Connect(bppTarget, 2);

      } else {
        bppCurrent->Connect(bppMiddle, 2);
        bppMiddle->Connect(bppTarget, 2);
      }

      ctConnections++;
    }
  }

  if (ctConnections <= 0) {
    CPrintF("Point %d/%d: No connections\n", iPoint + 1, bnm_cbppPoints.Count());

  } else {
    CPrintF("Point %d/%d: Connected to %d points\n", iPoint + 1, bnm_cbppPoints.Count(), ctConnections);
  }
};

// Remove orphan points
void CBotNavmesh::CleanupPoints(void) {
  CDynamicContainer<CBotPathPoint> cToRemove;
  CDynamicContainer<CBotPathPoint> cToKeep;

  FOREACHINDYNAMICCONTAINER(bnm_cbppPoints, CBotPathPoint, itbpp) {
    CBotPathPoint *pbpp = itbpp;

    // No targets
    if (pbpp->bpp_cbppPoints.Count() == 0) {
      if (!cToKeep.IsMember(pbpp)) {
        cToRemove.Add(pbpp);
      }
      continue;
    }

    INDEX ctTargets = pbpp->bpp_cbppPoints.Count();

    for (INDEX iTarget = 0; iTarget < ctTargets; iTarget++) {
      CBotPathPoint *pbppTarget = pbpp->bpp_cbppPoints.Pointer(iTarget);

      // Mark this point as targeted
      if (!cToKeep.IsMember(pbppTarget)) {
        cToKeep.Add(pbppTarget);
      }

      // Remove from points to remove
      if (cToRemove.IsMember(pbpp)) {
        cToRemove.Remove(pbppTarget);
      }
    }
  }

  CPrintF("Removed %d orphan points\n", cToRemove.Count());

  while (cToRemove.Count() > 0) {
    CBotPathPoint *pbppRemove = cToRemove.Pointer(0);

    // Remove from navmesh
    bnm_cbppPoints.Remove(pbppRemove);

    // Delete from memory
    cToRemove.Remove(pbppRemove);
    delete pbppRemove;
  }
};

// [Cecil] Local path points list
static CDynamicContainer<CPathPoint> _cppPoints;
// [Cecil] Open and closed lists of nodes
static CDynamicContainer<CPathPoint> _cppOpen;
static CDynamicContainer<CPathPoint> _cppClosed;
// [Cecil] Final path
static CDynamicContainer<CBotPathPoint> _cbppPath;

// Heuristic cost
static FLOAT PointsDist(CBotPathPoint *pbppSrc, CBotPathPoint *pbppDst) {
  return (pbppSrc->bpp_vPos - pbppDst->bpp_vPos).Length();
};

CBotPathPoint *CBotNavmesh::ReconstructPath(CPathPoint *ppCurrent) {
  while (ppCurrent != NULL) {
    _cbppPath.Add(ppCurrent->pp_bppPoint);
    ppCurrent = ppCurrent->pp_ppFrom;
  }

  // delete created points
  for (INDEX iDeletePoint = 0; iDeletePoint < _cppPoints.Count(); iDeletePoint++) {
    CPathPoint *pp = _cppPoints.Pointer(iDeletePoint);
    delete pp;
  }
  
  // take next point from the end
  INDEX ctPoints = _cbppPath.Count();
  return (ctPoints > 1) ? _cbppPath.Pointer(ctPoints - 2) : NULL;
};

CBotPathPoint *CBotNavmesh::FindNextPoint(CBotPathPoint *pbppSrc, CBotPathPoint *pbppDst) {
  // no points at all
  if (pbppSrc == NULL || pbppDst == NULL) {
    return NULL;
  }

  // reset lists
  _cppPoints.Clear();
  _cppOpen.Clear();
  _cppClosed.Clear();
  _cbppPath.Clear();

  // source point
  CPathPoint *ppSrc = NULL;

  // recreate every point
  FOREACHINDYNAMICCONTAINER(_pNavmesh->bnm_cbppPoints, CBotPathPoint, itbpp) {
    CBotPathPoint *pbpp = itbpp;
    CPathPoint *ppPoint = new CPathPoint;

    ppPoint->pp_bppPoint = pbpp;

    _cppPoints.Add(ppPoint);

    // remember the source
    if (pbpp == pbppSrc) {
      ppSrc = ppPoint;
    }
  }

  ppSrc->pp_fG = 0.0f;
  ppSrc->pp_fH = PointsDist(pbppSrc, pbppDst);
  ppSrc->pp_fF = ppSrc->pp_fG + ppSrc->pp_fH;

  _cppOpen.Add(ppSrc);

  while (_cppOpen.Count() > 0) {
    CPathPoint *ppShortest = _cppOpen.Pointer(0);

    // find point with shortest path
    if (_cppOpen.Count() > 1) {
      FOREACHINDYNAMICCONTAINER(_cppOpen, CPathPoint, itpp) {
        CPathPoint *pp = itpp;

        if (pp->pp_fF < ppShortest->pp_fF) {
          ppShortest = pp;
        }
      }
    }

    // found destination point
    if (ppShortest->pp_bppPoint == pbppDst) {
      CPathPoint *ppDst = NULL;

      {FOREACHINDYNAMICCONTAINER(_cppPoints, CPathPoint, itpp) {
        CPathPoint *pp = itpp;

        if (pp->pp_bppPoint == pbppDst) {
          ppDst = pp;
          break;
        }
      }}

      return ReconstructPath(ppDst);
    }

    // move this point
    _cppOpen.Remove(ppShortest);
    _cppClosed.Add(ppShortest);

    CBotPathPoint *pbppShortest = ppShortest->pp_bppPoint;

    // check each connection
    FOREACHINDYNAMICCONTAINER(pbppShortest->bpp_cbppPoints, CBotPathPoint, itbpp) {
      CBotPathPoint *pbpp = itbpp;
      CPathPoint *ppNode = NULL;

      {FOREACHINDYNAMICCONTAINER(_cppPoints, CPathPoint, itpp) {
        CPathPoint *pp = itpp;

        if (pp->pp_bppPoint == pbpp) {
          ppNode = pp;
          break;
        }
      }}

      // skip points from the closed list
      if (ppNode == NULL || _cppClosed.IsMember(ppNode)) {
        continue;
      }

      // skip locked points
      if (pbpp->IsLocked()) {
        continue;
      }

      // distance to current target point (none)
      FLOAT fToTarget = 0.0f;

      // if not a teleport point, calculate natural distance
      if (!(pbppShortest->bpp_ulFlags & PPF_TELEPORT)) {
        fToTarget = PointsDist(pbppShortest, pbpp);
      }

      FLOAT fTestG = ppShortest->pp_fG + fToTarget;
      BOOL bSuitable = FALSE;

      if (!_cppOpen.IsMember(ppNode)) {
        _cppOpen.Add(ppNode);
        bSuitable = TRUE;

      } else {
        bSuitable = (fTestG < ppNode->pp_fG);
      }

      if (bSuitable) {
        ppNode->pp_ppFrom = ppShortest;
        ppNode->pp_fG = fTestG;
        ppNode->pp_fH = PointsDist(ppNode->pp_bppPoint, pbppDst);
        ppNode->pp_fF = ppNode->pp_fG + ppNode->pp_fH;
      }
    }
  }

  // delete created points
  for (INDEX iDeletePoint = 0; iDeletePoint < _cppPoints.Count(); iDeletePoint++) {
    CPathPoint *pp = _cppPoints.Pointer(iDeletePoint);
    delete pp;
  }

  return NULL;
};
