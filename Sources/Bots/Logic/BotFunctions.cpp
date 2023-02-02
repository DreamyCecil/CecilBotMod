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

// [Cecil] 2019-06-04: This file is for functions primarily used by PlayerBot class
#include "StdH.h"
#include "BotFunctions.h"
#include "BotItems.h"

#include "EntitiesMP/Switch.h"
#include "EntitiesMP/MovingBrush.h"

// Constructor
SBotLogic::SBotLogic(void) : ulFlags(0), peiTarget(NULL),  aAim(0.0f, 0.0f, 0.0f),
  plBotView(FLOAT3D(0.0f, 0.0f, 0.0f), ANGLE3D(0.0f, 0.0f, 0.0f)), iDesiredWeapon(WPN_DEFAULT_1)
{
  aWeapons = PickWeaponConfig();
};

// [Cecil] 2019-05-28: Find nearest NavMesh point to some position
CBotPathPoint *NearestNavMeshPointPos(CEntity *pen, const FLOAT3D &vCheck) {
  if (_pNavmesh->bnm_cbppPoints.Count() <= 0) {
    return NULL;
  }

  // Gravity direction
  FLOAT3D vGravityDir(0.0f, -1.0f, 0.0f);

  if (pen != NULL && pen->GetPhysicsFlags() & EPF_MOVABLE) {
    vGravityDir = ((CMovableEntity *)pen)->en_vGravityDir;
  }

  FLOAT fDist = 1000.0f;
  CBotPathPoint *pbppNearest = NULL;

  FOREACHINDYNAMICCONTAINER(_pNavmesh->bnm_cbppPoints, CBotPathPoint, itbpp) {
    CBotPathPoint *pbpp = itbpp;
    FLOAT3D vPosDiff = (pbpp->bpp_vPos - vCheck);

    // Vertical and horizontal position differences
    FLOAT3D vDiffV = VerticalDiff(vPosDiff, vGravityDir);
    FLOAT3D vDiffH = vPosDiff + vDiffV;
    
    // Apply range to horizontal difference
    FLOAT fDiffH = ClampDn(vDiffH.Length() - pbpp->bpp_fRange, 0.0f);

    // Distance to the point
    FLOAT fToPoint = FLOAT3D(fDiffH, vDiffV.Length(), 0.0f).Length();

    if (fToPoint < fDist) {
      pbppNearest = pbpp;
      fDist = fToPoint;
    }
  }

  return pbppNearest;
};

// [Cecil] 2021-06-21: Find nearest NavMesh point to the bot
CBotPathPoint *CPlayerBotController::NearestNavMeshPointBot(BOOL bSkipCurrent) {
  if (_pNavmesh->bnm_cbppPoints.Count() <= 0) {
    return NULL;
  }

  // Bot's body center
  FLOAT3D vBot;

  EntityInfo *peiBot = (EntityInfo *)pen->GetEntityInfo();
  GetEntityInfoPosition(pen, peiBot->vTargetCenter, vBot);

  FLOAT fDist = 1000.0f;
  CBotPathPoint *pbppNearest = NULL;

  FOREACHINDYNAMICCONTAINER(_pNavmesh->bnm_cbppPoints, CBotPathPoint, itbpp) {
    CBotPathPoint *pbpp = itbpp;
    FLOAT3D vPosDiff = (pbpp->bpp_vPos - vBot);

    // Vertical and horizontal position differences
    FLOAT3D vDiffV = VerticalDiff(vPosDiff, pen->en_vGravityDir);
    FLOAT3D vDiffH = vPosDiff + vDiffV;
    
    // Apply range to horizontal difference
    FLOAT fDiffH = ClampDn(vDiffH.Length() - pbpp->bpp_fRange, 0.0f);

    // Distance to the point
    FLOAT fToPoint = FLOAT3D(fDiffH, vDiffV.Length(), 0.0f).Length();

    BOOL bNotCurrent = (!bSkipCurrent || !CurrentPoint(pbpp));

    if (fToPoint < fDist && bNotCurrent) {
      pbppNearest = pbpp;
      fDist = fToPoint;
    }
  }

  return pbppNearest;
};

// [Cecil] 2019-06-05: Check if this entity is important for a path point
BOOL CPlayerBotController::ImportantForNavMesh(CEntity *penEntity) {
  // Is item pickable
  if (IsDerivedFromClass(penEntity, "Item")) {
    return IsItemPickable((CItem *)penEntity, FALSE);

  // Is switch usable
  } else if (IsOfClass(penEntity, "Switch")) {
    return ((CSwitch &)*penEntity).m_bUseable;

  // Is moving brush usable
  } else if (IsOfClass(penEntity, "Moving Brush")) {
    CEntity *penSwitch = ((CMovingBrush *)penEntity)->m_penSwitch;
    return ImportantForNavMesh(penSwitch);

  // Can go to markers
  } else if (IsDerivedFromClass(penEntity, "Marker")) {
    return TRUE;
  }

  return FALSE;
};

// [Cecil] 2021-06-25: Use important entity
void CPlayerBotController::UseImportantEntity(CEntity *penEntity) {
  if (!ASSERT_ENTITY(penEntity)) {
    return;
  }

  // Press the switch
  if (IsOfDllClass(penEntity, CSwitch_DLLClass)) {
    if (((CSwitch &)*penEntity).m_bUseable) {
      SendToTarget(penEntity, EET_TRIGGER, pen);
    }

  // Use the moving brush
  } else if (IsOfDllClass(penEntity, CMovingBrush_DLLClass)) {
    CEntity *penSwitch = ((CMovingBrush *)penEntity)->m_penSwitch;
    UseImportantEntity(penSwitch);

  // Go to the point near the marker target
  } else if (IsOfDllClass(penEntity, CMarker_DLLClass)) {
    if (IsDerivedFromDllClass(pen, CPlayerBot_DLLClass)) {
      CEntity *penTarget = penEntity->GetTarget();

      if (penTarget != NULL) {
        props.m_pbppTarget = NearestNavMeshPointPos(penTarget, penTarget->GetPlacement().pl_PositionVector);
        props.m_bImportantPoint = TRUE;
      }
    }
  }
};

// [Cecil] Cast bot view ray
BOOL CPlayerBotController::CastBotRay(CEntity *penTarget, const SBotLogic &sbl, BOOL bPhysical) {
  // [Cecil] TEMP: Target is too far
  if (DistanceTo(pen, penTarget) > 1000.0f) {
    return FALSE;
  }

  FLOAT3D vBody = FLOAT3D(0.0f, 0.0f, 0.0f);

  // Target's body center
  if (sbl.peiTarget != NULL) {
    FLOAT *v = sbl.peiTarget->vTargetCenter;
    vBody = FLOAT3D(v[0], v[1], v[2]) * penTarget->GetRotationMatrix();
  }

  FLOAT3D vTarget = penTarget->GetPlacement().pl_PositionVector + vBody;
  CCastRay crBot(pen, sbl.ViewPos(), vTarget);

  crBot.cr_ttHitModels = CCastRay::TT_NONE;
  crBot.cr_bHitTranslucentPortals = TRUE;
  crBot.cr_bPhysical = bPhysical;
  CastRayFlags(crBot, pen->GetWorld(), (bPhysical ? BPOF_PASSABLE : 0));

  return (vTarget - crBot.cr_vHit).Length() <= 0.1f;
};

// [Cecil] Cast path point ray
BOOL CastPathPointRay(const FLOAT3D &vSource, const FLOAT3D &vPoint, FLOAT &fDist, BOOL bPhysical) {
  CCastRay crBot(NULL, vSource, vPoint);

  crBot.cr_ttHitModels = CCastRay::TT_NONE;
  crBot.cr_bHitTranslucentPortals = TRUE;
  crBot.cr_bPhysical = bPhysical;
  CastRayFlags(crBot, &_pNetwork->ga_World, (bPhysical ? BPOF_PASSABLE : 0));

  fDist = (vPoint - crBot.cr_vHit).Length();
  return fDist <= 1.0f;
};

// [Cecil] 2021-06-13: Check if it's an enemy player
BOOL CPlayerBotController::IsEnemyPlayer(CEntity *penEnemy) {
  // Not a player
  if (!IS_PLAYER(penEnemy)) {
    return FALSE;
  }

  const CTString &strTeam = GetPlayerBot()->en_pcCharacter.GetTeam();

  // No team has been set
  if (strTeam == "") {
    return TRUE;
  }

  // Different teams
  return (strTeam != ((CPlayer *)penEnemy)->en_pcCharacter.GetTeam());
};

// [Cecil] 2021-06-19: Check if it's a monster enemy
BOOL CPlayerBotController::IsEnemyMonster(CEntity *penEnemy) {
  // simple class type check
  return IsDerivedFromDllClass(penEnemy, CEnemyBase_DLLClass)
      /*&& !IsOfDllClass(penEnemy, CCannonStatic_DLLClass)
      && !IsOfDllClass(penEnemy, CCannonRotating_DLLClass)*/;
};

// [Cecil] 2018-10-11: Bot enemy searching
CEntity *CPlayerBotController::ClosestEnemy(FLOAT &fLast, const SBotLogic &sbl) {
  CEntity *penReturn = NULL;

  // Don't search for enemies
  if (!props.m_sbsBot.bTargetSearch) {
    return NULL;
  }

  // Priorities
  FLOAT fLastHP = 1000.0f;
  BOOL bVisible = FALSE;
  fLast = -1.0f;

  // How many priorities have been fulfilled
  INDEX iLastPriority = 0;
  INDEX iPriority = 0;
  CEntity *penLastTarget = NULL;

  // For each entity in the world
  {FOREACHINDYNAMICCONTAINER(pen->GetWorld()->wo_cenEntities, CEntity, iten) {
    CEntity *penCheck = iten;

    // If enemy (but not cannons - usually hard to reach)
    if (props.m_sbsBot.iTargetType >= 1 && IsEnemyMonster(penCheck)) {
      // If not alive
      CEnemyBase *penEnemy = (CEnemyBase *)penCheck;

      if (penEnemy->m_bTemplate || !(penEnemy->GetFlags() & ENF_ALIVE) || penEnemy->GetHealth() <= 0.0f) {
        continue;
      }

    // If player and it's not a coop or a singleplayer game
    } else if (props.m_sbsBot.iTargetType != 1 && IsEnemyPlayer(penCheck)) {
      // If not alive
      CPlayer *penEnemy = (CPlayer *)penCheck;

      if (penEnemy == pen || !(penEnemy->GetFlags() & ENF_ALIVE) || penEnemy->GetHealth() <= 0.0f) {
        continue;
      }

    } else {
      // Skip every other entity
      continue;
    }

    FLOAT3D vEnemy = penCheck->GetPlacement().pl_PositionVector;

    FLOAT fHealth = ((CMovableEntity *)penCheck)->GetHealth();
    FLOAT fDist = PosDist(sbl.ViewPos(), vEnemy);
    BOOL bCurrentVisible = CastBotRay(penCheck, sbl, TRUE);
    CEntity *penTargetEnemy = NULL;

    // Target's target
    if (IsOfDllClass(penCheck, CPlayerBot_DLLClass)) {
      penTargetEnemy = ((CPlayerBot *)penCheck)->GetProps().m_penTarget;

    } else if (IsDerivedFromClass(penCheck, "Enemy Base")) {
      penTargetEnemy = ((CEnemyBase *)penCheck)->m_penEnemy;
    }

    // Priorities
    if (bCurrentVisible)                 iPriority++;
    if (fHealth < fLastHP)               iPriority++;
    if (fDist < fLast || fLast == -1.0f) iPriority++;
    if (penTargetEnemy == pen)           iPriority++;

    // If more priorities have been fulfilled
    if (iPriority >= iLastPriority) {
      // Remember the target
      fLastHP = fHealth;
      fLast = fDist;
      bVisible = bCurrentVisible;

      penReturn = penCheck;
      penLastTarget = penTargetEnemy;

      iLastPriority = iPriority;
    }

    iPriority = 0;
  }}

  // Target is too far
  if (fLast < 0.0f) {
    fLast = 1000.0f;
  }

  return penReturn;
};

// [Cecil] 2019-05-30: Find closest real player
CEntity *CPlayerBotController::ClosestRealPlayer(FLOAT3D vCheckPos, FLOAT &fDist) {
  CEntity *penReturn = NULL;
  fDist = -1.0f;

  // For each real player
  for (INDEX i = 0; i < CEntity::GetMaxPlayers(); i++) {
    CPlayer *penReal = (CPlayer *)CEntity::GetPlayerEntity(i);
      
    // Skip unexistent and dead players
    if (!ASSERT_ENTITY(penReal) || !(penReal->GetFlags() & ENF_ALIVE)) {
      continue;
    }

    FLOAT3D vPlayer = penReal->GetPlacement().pl_PositionVector;

    if (fDist == -1.0f || PosDist(vCheckPos, vPlayer) < fDist) {
      fDist = PosDist(vCheckPos, vPlayer);
      penReturn = penReal;
    }
  }

  return penReturn;
};
