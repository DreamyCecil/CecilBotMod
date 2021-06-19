/* Copyright (c) 2018-2021 Dreamy Cecil
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

// [Cecil] 2021-06-14: This file is for functions primarily used by PlayerBot class
#include "StdH.h"

#include "BotFunctions.h"
#include "BotMovement.h"

// Shortcuts
#define SBS    (pen->m_sbsBot)
#define WEAPON (pen->GetPlayerWeapons())
#define WORLD  (_pNetwork->ga_World)

// [Cecil] 2021-06-14: Try to find some path
void BotPathFinding(CPlayerBot *pen, SBotLogic &sbl) {
  if (_pNavmesh->bnm_cbppPoints.Count() <= 0) {
    return;
  }

  const FLOAT3D &vBotPos = pen->GetPlacement().pl_PositionVector;

  CEntity *penTarget = NULL;
  BOOL bSeeTarget = FALSE;

  // go to the player
  if (sbl.Following()) {
    // if can't see him
    if (!sbl.SeePlayer()) {
      penTarget = pen->m_penFollow;
    }

  // go to the enemy
  } else {
    penTarget = pen->m_penTarget;
    bSeeTarget = sbl.SeeEnemy();

    // select important points sometimes
    if (!pen->m_bImportantPoint && pen->m_tmPickImportant <= _pTimer->CurrentTick()) {
      // 20% chance to select it
      if (pen->IRnd() % 5 == 0) {
        pen->m_pbppTarget = _pNavmesh->FindImportantPoint(pen, -1);
        pen->m_bImportantPoint = (pen->m_pbppTarget != NULL);
      }

      pen->m_tmPickImportant = _pTimer->CurrentTick() + 5.0f;
    }
  }

  if (penTarget == NULL) {
    pen->m_pbppCurrent = NULL;
    pen->m_ulPointFlags = 0;
    return;
  }

  // able to select new target point
  BOOL bChangeTargetPoint = (pen->m_pbppCurrent == NULL || pen->m_tmChangePath <= _pTimer->CurrentTick());

  // if timer is up and there's a point
  if (!bChangeTargetPoint) {
    // position difference
    FLOAT3D vPointDiff = (pen->m_pbppCurrent->bpp_vPos - vBotPos);

    // close to the point
    FLOAT &fRange = pen->m_pbppCurrent->bpp_fRange;
    bChangeTargetPoint = (vPointDiff.Length() < fRange);

    // a bit higher up
    vPointDiff = (pen->m_pbppCurrent->bpp_vPos - vBotPos) - pen->en_vGravityDir * (fRange * 0.5f);
    bChangeTargetPoint |= (vPointDiff.Length() < fRange);

    vPointDiff = (pen->m_pbppCurrent->bpp_vPos - vBotPos) - pen->en_vGravityDir * fRange;
    bChangeTargetPoint |= (vPointDiff.Length() < fRange);
    
    // and down
    vPointDiff = (pen->m_pbppCurrent->bpp_vPos - vBotPos) + pen->en_vGravityDir * (fRange * 0.5f);
    bChangeTargetPoint |= (vPointDiff.Length() < fRange);

    vPointDiff = (pen->m_pbppCurrent->bpp_vPos - vBotPos) + pen->en_vGravityDir * fRange;
    bChangeTargetPoint |= (vPointDiff.Length() < fRange);
  }

  BOOL bReachedImportantPoint = FALSE;

  // only change the important point if reached it
  if (pen->m_bImportantPoint) {
    if (pen->m_pbppTarget != NULL) {
      // position difference
      FLOAT3D vPointDiff = (pen->m_pbppTarget->bpp_vPos - vBotPos);
      
      // close to the point
      FLOAT &fRange = pen->m_pbppTarget->bpp_fRange;
      bReachedImportantPoint = (vPointDiff.Length() < fRange);

      // a bit higher up
      vPointDiff = (pen->m_pbppTarget->bpp_vPos - vBotPos) - pen->en_vGravityDir * (fRange * 0.5f);
      bReachedImportantPoint |= (vPointDiff.Length() < fRange);

      vPointDiff = (pen->m_pbppTarget->bpp_vPos - vBotPos) - pen->en_vGravityDir * fRange;
      bReachedImportantPoint |= (vPointDiff.Length() < fRange);

    // lost target point
    } else {
      pen->m_bImportantPoint = FALSE;
    }
  }

  if (bChangeTargetPoint) { 
    // find first point to go to
    CBotPathPoint *pbppClosest = NearestNavMeshPoint(pen, vBotPos, NULL);

    BOOL bSelectTarget = (bSeeTarget || pen->m_pbppCurrent == NULL);

    // don't change important point or if can't see the enemy
    if ((!pen->m_bImportantPoint && bSelectTarget) || bReachedImportantPoint) {
      pen->m_pbppTarget = NearestNavMeshPoint(pen, penTarget->GetPlacement().pl_PositionVector, NULL);
    }

    CBotPathPoint *pbppNext = _pNavmesh->FindNextPoint(pbppClosest, pen->m_pbppTarget);

    // remember the point if found
    if (pbppNext != NULL) {
      // [Cecil] 2021-06-16: Target point is unreachable, stay where you are
      pen->m_pbppCurrent = (pbppNext->bpp_ulFlags & PPF_UNREACHABLE) ? pbppClosest : pbppNext;

      // get flags of the closest point or override them
      pen->m_ulPointFlags = (pbppNext->bpp_ulFlags & PPF_OVERRIDE) ? pbppNext->bpp_ulFlags : pbppClosest->bpp_ulFlags;

    } else {
      pen->m_pbppCurrent = NULL;
      pen->m_ulPointFlags = 0;
    }

    if (bReachedImportantPoint) {
      pen->m_bImportantPoint = FALSE;
    }

    pen->m_tmChangePath = _pTimer->CurrentTick() + 5.0f;
  }
};

// [Cecil] 2021-06-15: Set bot aim
void BotAim(CPlayerBot *pen, CPlayerAction &pa, SBotLogic &sbl) {
  // [Cecil] 2021-06-16: Aim in the walking direction on a path if haven't seen the enemy in a while
  if (pen->m_pbppCurrent != NULL && _pTimer->CurrentTick() - pen->m_tmLastSawTarget > 2.0f) {
    // calculate an angle
    FLOAT3D vToTarget = pen->en_vCurrentTranslationAbsolute;
    vToTarget.SafeNormalize();

    ANGLE3D aToTarget;
    DirectionVectorToAnglesNoSnap(vToTarget, aToTarget);
    aToTarget(2) = ClampUp(aToTarget(2), 0.0f); // don't look up

    // don't look down for some time
    if (pen->m_fFallTime < 1.0f) {
      aToTarget(2) = ClampDn(aToTarget(2), 0.0f);
    }

    aToTarget = aToTarget - sbl.ViewAng();

    // set rotation speed
    sbl.aAim(1) = Clamp(NormalizeAngle(aToTarget(1)) * 0.3f, -50.0f, 50.0f) / _pTimer->TickQuantum;
    sbl.aAim(2) = Clamp(NormalizeAngle(aToTarget(2)) * 0.3f, -50.0f, 50.0f) / _pTimer->TickQuantum;
    return;
  }

  // no enemy
  if (!sbl.EnemyExists()) {
    return;
  }

  EntityInfo *peiTarget = (EntityInfo *)pen->m_penTarget->GetEntityInfo();
  FLOAT3D vEnemy;

  // get target center position
  if (peiTarget != NULL) {
    GetEntityInfoPosition(pen->m_penTarget, peiTarget->vTargetCenter, vEnemy);

  // just enemy position if no entity info
  } else {
    vEnemy = pen->m_penTarget->GetPlacement().pl_PositionVector + FLOAT3D(0.0f, 1.0f, 0.0f) * pen->m_penTarget->GetRotationMatrix();
  }

  // next position prediction
  vEnemy += ((CMovableEntity*)&*pen->m_penTarget)->en_vCurrentTranslationAbsolute
          * (SBS.fPrediction + pen->FRnd() * SBS.fPredictRnd) // default: *= 0.2f
          * (1 + sbl.aWeapons[pen->m_iBotWeapon].bw_bPredict); // [Cecil] TEMP: Twice as much prediction for predicting weapons
          //* sbl.aWeapons[m_iBotWeapon].bw_bPredict;

  // look a bit higher if it's a player
  if (IS_PLAYER(pen->m_penTarget)) {
    vEnemy += FLOAT3D(0, 0.25f, 0) * pen->m_penTarget->GetRotationMatrix();
  }
      
  // calculate an angle
  FLOAT3D vToTarget = vEnemy - sbl.ViewPos();
  vToTarget.Normalize();

  ANGLE3D aToTarget;
  DirectionVectorToAnglesNoSnap(vToTarget, aToTarget);

  // update accuracy angles
  if (SBS.fAccuracyAngle > 0.0f) {
    FLOAT tmNow = _pTimer->CurrentTick();
        
    // randomize every half a second
    if (pen->m_tmBotAccuracy <= tmNow) {
      // more accurate with the sniper
      FLOAT fAccuracyMul = (WEAPON->m_bSniping ? 0.2f : 1.0f) * SBS.fAccuracyAngle;

      // invisible targets are hard to aim at
      if (pen->m_penTarget->GetFlags() & ENF_INVISIBLE) {
        fAccuracyMul *= 3.0f;
      }

      pen->m_vAccuracy = FLOAT3D(pen->FRnd() - 0.5f, pen->FRnd() - 0.5f, 0.0f) * fAccuracyMul;
      pen->m_tmBotAccuracy = tmNow + 0.5f;
    }
  }

  aToTarget = aToTarget - sbl.ViewAng();
  aToTarget(1) = NormalizeAngle(aToTarget(1)) + pen->m_vAccuracy(1);
  aToTarget(2) = NormalizeAngle(aToTarget(2)) + pen->m_vAccuracy(2);
      
  // limit to one tick, otherwise aim will go too far and miss
  const FLOAT fDistRotSpeed = ClampDn(SBS.fRotSpeedDist, 0.05f);      // 400
  const FLOAT fMinRotSpeed = ClampDn(SBS.fRotSpeedMin, 0.05f);        // 0.05f
  const FLOAT fMaxRotSpeed = ClampDn(SBS.fRotSpeedMax, fMinRotSpeed); // 0.2f
  const FLOAT fSpeedLimit = SBS.fRotSpeedLimit;                       // 30

  // clamp the speed
  FLOAT fRotationSpeed = Clamp(pen->m_fTargetDist / fDistRotSpeed, fMinRotSpeed, fMaxRotSpeed);

  // max speed
  if (fSpeedLimit >= 0.0f) {
    aToTarget(1) = Clamp(aToTarget(1), -fSpeedLimit, fSpeedLimit);
    aToTarget(2) = Clamp(aToTarget(2), -fSpeedLimit, fSpeedLimit);
  }

  // set rotation speed
  sbl.aAim(1) = aToTarget(1) / fRotationSpeed;
  sbl.aAim(2) = aToTarget(2) / fRotationSpeed;

  // try to shoot
  if (Abs(aToTarget(1)) < SBS.fShootAngle && Abs(aToTarget(2)) < SBS.fShootAngle) {
    // shoot if the enemy is visible or the crosshair is on them
    BOOL bTargetingEnemy = WEAPON->m_penRayHit == pen->m_penTarget;

    if (sbl.SeeEnemy() || bTargetingEnemy) {
      sbl.ubFlags |= BLF_CANSHOOT;
    }

    // [Cecil] TEMP: Don't shoot if there's an obstacle in the way
    if (!bTargetingEnemy) {
      FLOAT3D vTargetView = vEnemy - sbl.ViewPos();

      if (WEAPON->m_fRayHitDistance <= 4.0f && WEAPON->m_fRayHitDistance < vTargetView.Length()) {
        sbl.ubFlags &= ~BLF_CANSHOOT;
      }
    }
  }
};

// [Cecil] 2021-06-14: Set bot movement
void BotMovement(CPlayerBot *pen, CPlayerAction &pa, SBotLogic &sbl) {
  // no need to set any moving speed if nowhere to go
  if (!sbl.EnemyExists() && !sbl.ItemExists()
   && pen->m_penFollow == NULL && pen->m_pbppCurrent == NULL) {
    return;
  }

  const FLOAT3D &vBotPos = pen->GetPlacement().pl_PositionVector;

  // randomize strafe direction every once in a while
  if (pen->m_tmChangeBotDir < _pTimer->CurrentTick()) {
    pen->m_fSideDir = (pen->IRnd() % 2 == 0) ? -1.0f : 1.0f;
    pen->m_tmChangeBotDir = _pTimer->CurrentTick() + (pen->FRnd() * 2.0f) + 2.0f; // 2 to 4 seconds
  }

  FLOAT3D vBotMovement = FLOAT3D(0.0f, 0.0f, 0.0f); // in which direction bot needs to go
  FLOAT fVerticalMove = 0.0f; // jumping or crouching

  SBotWeaponConfig &bwcWeapon = sbl.aWeapons[pen->m_iBotWeapon]; // current weapon config

  // strafe further if lower health
  FLOAT fHealthRatio = Clamp(100.0f - pen->GetHealth(), 0.0f, 100.0f)/100.0f;
  const FLOAT fStrafeDist = (bwcWeapon.bw_fMaxDistance - bwcWeapon.bw_fMinDistance) * bwcWeapon.bw_fStrafe;

  // avoid the target in front of the bot
  FLOAT fStrafe = Clamp(fStrafeDist * fHealthRatio, 5.0f, 16.0f);

  if (SBS.bStrafe && pen->m_fTargetDist < (bwcWeapon.bw_fMinDistance + fStrafe)
   && (pen->m_penFollow == NULL || pen->m_penFollow == pen->m_penTarget || sbl.Following())) {
    // run around the enemy
    vBotMovement = FLOAT3D(pen->m_fSideDir, 0.0f, 0.0f);

  } else {
    FLOAT3D vDelta = FLOAT3D(0.0f, 0.0f, 0.0f);
    BOOL bShouldFollow = FALSE;
    BOOL bShouldJump = FALSE;
    BOOL bShouldCrouch = FALSE;
    
    // run towards the following target
    if (pen->m_penFollow != NULL) {
      vDelta = (pen->m_penFollow->GetPlacement().pl_PositionVector - vBotPos);

      // check vertical difference
      FLOAT3D vVertical = VerticalDiff(vDelta, pen->en_vGravityDir);

      // if going for the item
      if (!sbl.Following() && sbl.ItemExists()) {
        // jump if it's higher and close
        bShouldJump = vVertical.Length() > 1.0f && vDelta.Length() < 8.0f;

      } else {
        // jump if it's higher and not an item
        bShouldJump = vVertical.Length() > 1.0f && !sbl.ItemExists(); //!sbl.Following() && !sbl.ItemExists();
      }

      bShouldFollow = TRUE;
    }
    
    // run towards the point if nothing to pickup
    if (pen->m_pbppCurrent != NULL && !sbl.ItemExists()) {
      vDelta = (pen->m_pbppCurrent->bpp_vPos - vBotPos);
      bShouldJump = pen->m_ulPointFlags & PPF_JUMP;
      bShouldCrouch = pen->m_ulPointFlags & PPF_CROUCH;

      bShouldFollow = TRUE;
    }

    // set the speed if there's a place to go
    if (bShouldFollow && vDelta.Length() > 0.0f) {
      ANGLE aDH = pen->GetRelativeHeading(vDelta.Normalize());

      FLOAT3D vRunDir;
      AnglesToDirectionVector(ANGLE3D(aDH, 0.0f, 0.0f), vRunDir);
      vBotMovement = vRunDir;

      // crouch if needed instead of jumping
      if (bShouldCrouch) {
        fVerticalMove = -1.0f;

      // jump if allowed
      } else if (bShouldJump && SBS.bJump) {
        fVerticalMove = 1.0f;

      } else {
        fVerticalMove = 0.0f;
      }
    }
  }

  // try to avoid obstacles
  if (pen->m_pbppCurrent == NULL && WEAPON->m_fRayHitDistance < 4.0f
   && !IsDerivedFromDllClass(WEAPON->m_penRayHit, CMovableEntity_DLLClass)) {
    // only jump if following players
    if (sbl.SeePlayer()) {
      fVerticalMove = 1.0f;

    } else {
      vBotMovement = FLOAT3D(pen->m_fSideDir, 0.0f, 1.0f);
    }
  }

  // vertical movement
  if (Abs(fVerticalMove) > 0.1f && pen->ButtonAction()) {
    vBotMovement(2) = fVerticalMove;
  } else {
    vBotMovement(2) = 0.0f;
  }

  // move around
  const FLOAT3D vMoveSpeed = FLOAT3D(plr_fSpeedSide, plr_fSpeedUp, (vBotMovement(3) > 0.0f) ? plr_fSpeedForward : plr_fSpeedBackward);

  for (INDEX iDir = 1; iDir <= 3; iDir++) {
    /*if (vBotMovement(iDir) != 0.0f) {
      pa.pa_vTranslation(iDir) += (vBotMovement(iDir) < 0) ? -vMoveSpeed(iDir) : vMoveSpeed(iDir);
    }*/

    // [Cecil] TEMP: Use direction instead of pressed buttons (it's like bot is using a controller)
    pa.pa_vTranslation(iDir) += vBotMovement(iDir) * vMoveSpeed(iDir);
  }

  // walk if needed
  if (pen->m_ulPointFlags & PPF_WALK) {
    pa.pa_vTranslation(1) /= 2.0f;
    pa.pa_vTranslation(3) /= 2.0f;
  }
};
