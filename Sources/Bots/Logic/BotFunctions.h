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

// [Cecil] 2019-06-04: This file is for functions primarily used by PlayerBot class
#ifndef _CECILBOTS_BOTFUNCTIONS_H
#define _CECILBOTS_BOTFUNCTIONS_H

#include "BotWeapons.h"

// [Cecil] 2019-05-28: Find nearest NavMesh point to some position
CBotPathPoint *NearestNavMeshPointPos(CEntity *pen, const FLOAT3D &vCheck);
// [Cecil] 2021-06-21: Find nearest NavMesh point to the bot
CBotPathPoint *NearestNavMeshPointBot(CPlayerBot *pen, BOOL bSkipCurrent);

// Write and read bot properties
void BotWrite(CPlayerBot *pen, CTStream *strm);
void BotRead(CPlayerBot *pen, CTStream *strm);

// [Cecil] 2019-06-05: Check if this entity is important for a path point
BOOL ImportantForNavMesh(CPlayer *penBot, CEntity *penEntity);

// [Cecil] 2021-06-25: Use important entity
void UseImportantEntity(CPlayer *penBot, CEntity *penEntity);

// [Cecil] Cast bot view ray
BOOL CastBotRay(CPlayerBot *pen, CEntity *penTarget, const SBotLogic &sbl, BOOL bPhysical);

// [Cecil] Cast path point ray
BOOL CastPathPointRay(const FLOAT3D &vSource, const FLOAT3D &vPoint, FLOAT &fDist, BOOL bPhysical);

// [Cecil] 2021-06-13: Check if it's an enemy player
BOOL IsEnemyPlayer(CPlayerBot *penBot, CEntity *penEnemy);

// [Cecil] 2021-06-19: Check if it's a monster enemy
BOOL IsEnemyMonster(CPlayerBot *penBot, CEntity *penEnemy);

// [Cecil] 2018-10-11: Bot enemy searching
CEntity *ClosestEnemy(CPlayerBot *pen, FLOAT &fLastDist, const SBotLogic &sbl);

// [Cecil] 2019-05-30: Find closest real player
CEntity *ClosestRealPlayer(CPlayerBot *pen, FLOAT3D vCheckPos, FLOAT &fDist);

#endif // _CECILBOTS_BOTFUNCTIONS_H
