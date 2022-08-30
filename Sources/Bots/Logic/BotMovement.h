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

// [Cecil] 2021-06-14: This file is for functions primarily used by PlayerBot class
#ifndef _CECILBOTS_BOTMOVEMENT_H
#define _CECILBOTS_BOTMOVEMENT_H

#include "BotFunctions.h"

// [Cecil] 2021-06-25: Too long since the last position change
BOOL NoPosChange(SPlayerBot &pb);

// [Cecil] 2021-06-14: Try to find some path
void BotPathFinding(SPlayerBot &pb, SBotLogic &sbl);

// [Cecil] 2021-06-15: Set bot aim
void BotAim(SPlayerBot &pb, CPlayerAction &pa, SBotLogic &sbl);

// [Cecil] 2021-06-14: Set bot movement
void BotMovement(SPlayerBot &pb, CPlayerAction &pa, SBotLogic &sbl);

#endif // _CECILBOTS_BOTMOVEMENT_H
