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
#define SETTINGS         (pb.props.m_sbsBot)
#define THOUGHT(_String) (pb.props.m_btThoughts.Push(_String))

// [Cecil] 2021-06-14: Check if item is pickable
BOOL IsItemPickable(SPlayerBot &pb, class CItem *penItem, const BOOL &bCheckDist) {
  // [Cecil] TEMP: Too far
  if (bCheckDist && DistanceTo(pb.pen, penItem) > GetItemDist(pb, penItem)) {
    return FALSE;
  }

  BOOL bPicked = (1 << CECIL_PlayerIndex(pb.pen)) & penItem->m_ulPickedMask;

  return !bPicked && (penItem->en_RenderType == CEntity::RT_MODEL
                   || penItem->en_RenderType == CEntity::RT_SKAMODEL);
};

// [Cecil] 2021-06-17: Search for an item
void BotItemSearch(SPlayerBot &pb, SBotLogic &sbl) {
  // Check for nearby items
  FLOAT fItemDist = MAX_ITEM_DIST;

  // Need this to determine the distance to the closest one
  CEntity *penItem = ClosestItemType(pb, CItem_DLLClass, fItemDist, sbl);

  if (penItem != NULL) {
    // Determine close distance for the item
    FLOATaabbox3D boxItem;
    penItem->GetBoundingBox(boxItem);
    FLOAT3D vItemSize = boxItem.Size();

    FLOAT fCloseItemDist = Max(Abs(vItemSize(1)), Abs(vItemSize(3))) / 2.0f;

    // Check if bot wants an item (if it's not too close)
    BOOL bWantItem = (fItemDist > fCloseItemDist);

    // [Cecil] TEMP 2022-05-11: Go for items in coop anyway
    BOOL bImportantPoint = (pb.props.m_bImportantPoint && !IsCoopGame());

    // Check if item is really needed (because going for an important point)
    BOOL bNeedItem = (!bImportantPoint || fItemDist < 8.0f);

    if (bWantItem && bNeedItem) {
      // Determine the closest item
      penItem = GetClosestItem(pb, fItemDist, sbl);

      // Put searching on cooldown if selected some item
      if (penItem != NULL) {
        pb.props.m_penLastItem = penItem;
        pb.props.m_tmLastItemSearch = _pTimer->CurrentTick() + SETTINGS.fItemSearchCD;

        THOUGHT(CTString(0, "Going for ^c7f7fff%s", penItem->en_pecClass->ec_pdecDLLClass->dec_strName));
      }
    }
  }

  // Has some item
  if (pb.props.m_penLastItem != NULL) {
    // Item is pickable
    if (IsItemPickable(pb, (CItem *)&*pb.props.m_penLastItem, TRUE)) {
      sbl.ulFlags |= BLF_ITEMEXISTS;
      pb.props.m_penFollow = pb.props.m_penLastItem;

    // Not pickable anymore
    } else {
      pb.props.m_penLastItem = NULL;
      pb.props.m_tmLastItemSearch = 0.0f;

      THOUGHT("^c7f7fffItem is no longer pickable");
    }
  }
};

// [Cecil] 2021-06-28: Distance to a specific item type
FLOAT GetItemDist(SPlayerBot &pb, CEntity *penItem) {
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
CEntity *GetClosestItem(SPlayerBot &pb, FLOAT &fItemDist, const SBotLogic &sbl) {
  // Run towards the weapon
  CEntity *penItem = ClosestItemType(pb, CWeaponItem_DLLClass, fItemDist, sbl);

  // Within range
  if (penItem != NULL && fItemDist < pb.props.m_fTargetDist && fItemDist < SETTINGS.fWeaponDist) {
    return penItem;
  }
  
  // Need health
  const FLOAT fBotHealth = pb.pen->GetHealth();
  
  if (fBotHealth < SETTINGS.fHealthSearch) {
    // Run towards health
    penItem = ClosestItemType(pb, CHealthItem_DLLClass, fItemDist, sbl);
    
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
  penItem = ClosestItemType(pb, CPowerUpItem_DLLClass, fItemDist, sbl);
  
  // Within range
  if (penItem != NULL && fItemDist < SETTINGS.fWeaponDist) {
    return penItem;
  }

  // [Cecil] TODO: Try to find 'm_fArmor' from the property list
  CPlayer *penPlayer = (CPlayer *)pb.pen;

  // Need armor
  if (penPlayer->m_fArmor < 100.0f) {
    // Run towards armor
    penItem = ClosestItemType(pb, CArmorItem_DLLClass, fItemDist, sbl);
  
    // Within range
    if (penItem != NULL && fItemDist < SETTINGS.fArmorDist) {
      FLOAT fArmor = ((CItem *)penItem)->m_fValue;

      // Only pick health if it's essential
      if (fArmor <= 10.0f && penPlayer->m_fArmor < 75.0f) {
        return penItem;

      } else if (fArmor <= 50.0f && penPlayer->m_fArmor < 85.0f) {
        return penItem;

      } else if (fArmor >= 100.0f) {
        return penItem;
      }
    }
  }

  // Run towards ammo
  penItem = ClosestItemType(pb, CAmmoPack_DLLClass, fItemDist, sbl);

  if (penItem == NULL) {
    // Search for ammo if no ammo packs
    penItem = ClosestItemType(pb, CAmmoItem_DLLClass, fItemDist, sbl);
  }

  // Within range
  if (penItem != NULL && fItemDist < SETTINGS.fAmmoDist) {
    return penItem;
  }

  return NULL;
};

// [Cecil] Closest item entity
CEntity *ClosestItemType(SPlayerBot &pb, const CDLLEntityClass &decClass, FLOAT &fDist, const SBotLogic &sbl) {
  // Can't search for items right now
  if (!SETTINGS.bItemSearch || pb.props.m_tmLastItemSearch > _pTimer->CurrentTick()) {
    return NULL;
  }

  CEntity *penReturn = NULL;
  fDist = MAX_ITEM_DIST;

  // For each bot item
  {FOREACHINDYNAMICCONTAINER(pb.pen->GetWorld()->wo_cenEntities, CEntity, iten) {
    CEntity *penCheck = iten;

    // If not an item or already picked up
    if (!IsDerivedFromDllClass(penCheck, decClass)
     || !IsItemPickable(pb, (CItem *)penCheck, TRUE)) {
      continue;
    }

    // If not visible
    if (SETTINGS.bItemVisibility && !CastBotRay(pb, penCheck, sbl, TRUE)) {
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
  if (penReturn == pb.props.m_penLastItem) {
    penReturn = NULL;
  }

  // Reset last item
  pb.props.m_penLastItem = NULL;

  return penReturn;
};
