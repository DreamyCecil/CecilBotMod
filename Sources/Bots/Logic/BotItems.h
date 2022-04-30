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

// [Cecil] 2022-05-01: This file is for functions primarily used by PlayerBot class
#ifndef _CECILBOTS_BOTITEMS_H
#define _CECILBOTS_BOTITEMS_H

#include "EntitiesMP/Item.h"

// [Cecil] 2021-06-14: Check if item is pickable
BOOL IsItemPickable(class CPlayer *pen, class CItem *penItem, const BOOL &bCheckDist);

// [Cecil] 2021-06-17: Search for an item
void BotItemSearch(CPlayerBot *pen, SBotLogic &sbl);

// [Cecil] 2021-06-28: Distance to a specific item type
FLOAT GetItemDist(CPlayerBot *pen, CEntity *penItem);

// [Cecil] 2021-06-14: Determine the closest item
CEntity *GetClosestItem(CPlayerBot *pen, FLOAT &fItemDist, const SBotLogic &sbl);

// [Cecil] Closest item entity
CEntity *ClosestItemType(CPlayerBot *pen, const CDLLEntityClass &decClass, FLOAT &fDist, const SBotLogic &sbl);

#endif // _CECILBOTS_BOTITEMS_H
