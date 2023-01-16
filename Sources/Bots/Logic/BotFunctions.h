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
#ifndef _CECILBOTS_BOTFUNCTIONS_H
#define _CECILBOTS_BOTFUNCTIONS_H

#include "BotWeapons.h"

// [Cecil] 2019-05-28: Find nearest NavMesh point to some position
CBotPathPoint *NearestNavMeshPointPos(CEntity *pen, const FLOAT3D &vCheck);

// [Cecil] Cast path point ray
BOOL CastPathPointRay(const FLOAT3D &vSource, const FLOAT3D &vPoint, FLOAT &fDist, BOOL bPhysical);

#endif // _CECILBOTS_BOTFUNCTIONS_H
