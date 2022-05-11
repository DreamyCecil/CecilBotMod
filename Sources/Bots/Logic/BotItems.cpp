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
#include "BotItems.h"
#include "BotFunctions.h"

// Shortcuts
#define SETTINGS         (pen->m_sbsBot)
#define THOUGHT(_String) (pen->m_btThoughts.Push(_String))

// [Cecil] 2021-06-14: Check if item is pickable
BOOL IsItemPickable(class CPlayer *pen, class CItem *penItem, const BOOL &bCheckDist) {
  // [Cecil] TEMP: Too far
  if (bCheckDist && DistanceTo(pen, penItem) > GetItemDist((CPlayerBot *)pen, penItem)) {
    return FALSE;
  }

  BOOL bPicked = (pen == NULL ? FALSE : (1 << CECIL_PlayerIndex(pen)) & penItem->m_ulPickedMask);

  return !bPicked && (penItem->en_RenderType == CEntity::RT_MODEL
                   || penItem->en_RenderType == CEntity::RT_SKAMODEL);
};

// [Cecil] 2021-06-17: Search for an item
void BotItemSearch(CPlayerBot *pen, SBotLogic &sbl) {
  // Check for nearby items
  FLOAT fItemDist = MAX_ITEM_DIST;

  // Need this to determine the distance to the closest one
  CEntity *penItem = ClosestItemType(pen, CItem_DLLClass, fItemDist, sbl);

  if (penItem != NULL) {
    // Determine close distance for the item
    FLOATaabbox3D boxItem;
    penItem->GetBoundingBox(boxItem);
    FLOAT3D vItemSize = boxItem.Size();

    FLOAT fCloseItemDist = Max(Abs(vItemSize(1)), Abs(vItemSize(3))) / 2.0f;

    // Check if bot wants an item (if it's not too close)
    BOOL bWantItem = (fItemDist > fCloseItemDist);

    // [Cecil] TEMP 2022-05-11: Go for items in coop anyway
    BOOL bImportantPoint = (pen->m_bImportantPoint && !IsCoopGame());

    // Check if item is really needed (because going for an important point)
    BOOL bNeedItem = (!bImportantPoint || fItemDist < 8.0f);

    if (bWantItem && bNeedItem) {
      // Determine the closest item
      penItem = GetClosestItem(pen, fItemDist, sbl);

      // Put searching on cooldown if selected some item
      if (penItem != NULL) {
        pen->m_penLastItem = penItem;
        pen->m_tmLastItemSearch = _pTimer->CurrentTick() + SETTINGS.fItemSearchCD;

        THOUGHT(CTString(0, "Going for ^c7f7fff%s", penItem->en_pecClass->ec_pdecDLLClass->dec_strName));
      }
    }
  }

  // Has some item
  if (pen->m_penLastItem != NULL) {
    // Item is pickable
    if (IsItemPickable(pen, (CItem *)&*pen->m_penLastItem, TRUE)) {
      sbl.ulFlags |= BLF_ITEMEXISTS;
      pen->m_penFollow = pen->m_penLastItem;

    // Not pickable anymore
    } else {
      pen->m_penLastItem = NULL;
      pen->m_tmLastItemSearch = 0.0f;

      THOUGHT("^c7f7fffItem is no longer pickable");
    }
  }
};

// [Cecil] 2021-06-28: Distance to a specific item type
FLOAT GetItemDist(CPlayerBot *pen, CEntity *penItem) {
  // Weapons and powerups
  if (IsOfDllClass(penItem, CWeaponItem_DLLClass)
   || IsOfDllClass(penItem, CPowerUpItem_DLLClass)) {
    return SETTINGS.fWeaponDist;
  }

  // Health
  if (IsOfDllClass(penItem, CHealthItem_DLLClass)) {
    return SETTINGS.fHealthDist;
  }

  // Armor
  if (IsOfDllClass(penItem, CArmorItem_DLLClass)) {
    return SETTINGS.fArmorDist;
  }

  // Ammo
  if (IsOfDllClass(penItem, CAmmoItem_DLLClass)
   || IsOfDllClass(penItem, CAmmoPack_DLLClass)) {
    return SETTINGS.fAmmoDist;
  }

  // Max distance
  return 64.0f;
};

// [Cecil] 2021-06-14: Determine the closest item
CEntity *GetClosestItem(CPlayerBot *pen, FLOAT &fItemDist, const SBotLogic &sbl) {
  // Run towards the weapon
  CEntity *penItem = ClosestItemType(pen, CWeaponItem_DLLClass, fItemDist, sbl);

  // Within range
  if (penItem != NULL && fItemDist < pen->m_fTargetDist && fItemDist < SETTINGS.fWeaponDist) {
    return penItem;
  }
  
  // Need health
  const FLOAT fBotHealth = pen->GetHealth();
  
  if (fBotHealth < SETTINGS.fHealthSearch) {
    // Run towards health
    penItem = ClosestItemType(pen, CHealthItem_DLLClass, fItemDist, sbl);
    
    // Within range
    if (penItem != NULL && fItemDist < SETTINGS.fHealthDist) {
      FLOAT fHealth = ((CItem *)penItem)->m_fValue;
      
      // Only pick health if it's essential
      if (fHealth <= 10.0f && fBotHealth < 75.0f) {
        return penItem;

      } else if (fHealth <= 50.0f && fBotHealth < 85.0f) {
        return penItem;

      } else if (fHealth >= 100.0f && fBotHealth < 150.0f) {
        return penItem;
      }
    }
  }

  // Run towards power ups
  penItem = ClosestItemType(pen, CPowerUpItem_DLLClass, fItemDist, sbl);
  
  // Within range
  if (penItem != NULL && fItemDist < SETTINGS.fWeaponDist) {
    return penItem;
  }

  // Need armor
  if (pen->m_fArmor < 100.0f) {
    // Run towards armor
    penItem = ClosestItemType(pen, CArmorItem_DLLClass, fItemDist, sbl);
  
    // Within range
    if (penItem != NULL && fItemDist < SETTINGS.fArmorDist) {
      FLOAT fArmor = ((CItem *)penItem)->m_fValue;

      // Only pick health if it's essential
      if (fArmor <= 10.0f && pen->m_fArmor < 75.0f) {
        return penItem;

      } else if (fArmor <= 50.0f && pen->m_fArmor < 85.0f) {
        return penItem;

      } else if (fArmor >= 100.0f) {
        return penItem;
      }
    }
  }

  // Run towards ammo
  penItem = ClosestItemType(pen, CAmmoPack_DLLClass, fItemDist, sbl);

  if (penItem == NULL) {
    // Search for ammo if no ammo packs
    penItem = ClosestItemType(pen, CAmmoItem_DLLClass, fItemDist, sbl);
  }

  // Within range
  if (penItem != NULL && fItemDist < SETTINGS.fAmmoDist) {
    return penItem;
  }

  return NULL;
};

// [Cecil] Closest item entity
CEntity *ClosestItemType(CPlayerBot *pen, const CDLLEntityClass &decClass, FLOAT &fDist, const SBotLogic &sbl) {
  // Can't search for items right now
  if (!SETTINGS.bItemSearch || pen->m_tmLastItemSearch > _pTimer->CurrentTick()) {
    return NULL;
  }

  CEntity *penReturn = NULL;
  fDist = MAX_ITEM_DIST;

  // For each bot item
  {FOREACHINDYNAMICCONTAINER(pen->GetWorld()->wo_cenEntities, CEntity, iten) {
    CEntity *penCheck = iten;

    // If not an item or already picked up
    if (!IsDerivedFromDllClass(penCheck, decClass)
     || !IsItemPickable(pen, (CItem *)penCheck, TRUE)) {
      continue;
    }

    // If not visible
    if (SETTINGS.bItemVisibility && !CastBotRay(pen, penCheck, sbl, TRUE)) {
      continue;
    }

    // Multiply vertical difference (further distance)
    FLOAT3D vPosDiff = (penCheck->GetPlacement().pl_PositionVector - sbl.ViewPos());
    vPosDiff(2) *= 3.0f;

    FLOAT fDistToItem = vPosDiff.Length();

    if (fDistToItem < fDist) {
      fDist = fDistToItem;
      penReturn = penCheck;
    }
  }}

  // If it's the same item as before, don't bother
  if (penReturn == pen->m_penLastItem) {
    penReturn = NULL;
  }

  // Reset last item
  pen->m_penLastItem = NULL;

  return penReturn;
};
