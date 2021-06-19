## Bot Mod

- `MOD_QuickBot()` - quick bot addition (uses `BOT_strSpawnName`)
- `MOD_AddBot(name, AMC filename)` - bot addition with custom name and player skin (empty strings for random)
- `MOD_RemoveBot(name)` - remove bots with a certain name from the game
- `MOD_RemoveAllBots()` - remove all bots from the game
- `MOD_BotUpdate()` - update settings for all bots (if `BOT_strBotEdit` isn't blank, only updates bots with that name)

---
- `BOT_strBotEdit` - name of a bot for editing its settings (leave blank for all bots)
- `BOT_strSpawnName` - quick add bots with this specific name
- `BOT_ResetBotConfig()` - reset all bot settings to default values

For specific bot behaviour customization go to `Options -> Advanced options -> Bot Mod Customization` in game (all commands start with `BOT_`)

## Miscellaneous

- `MOD_bEntityIDs` - display IDs of all entities nearby (for `MOD_NavMeshPointEntity()`)

- `MOD_SetWeapons(weapon type/mask, players)` - change all weapon items or player weapons on the map:
```
Change weapon type of all CWeaponItems:
  weapon type - WeaponItemType index of CWeaponItem entity
  players - set to 0

Change current player weapons and "Give Weapons" property of all CPlayerMarkers:
  weapon mask - binary mask in base 10 (e.g. 148 = 128 + 16 + 4)
  players - set to 1
```

## Navmesh creation

- `MOD_GenerateNavMesh(connect)` - simple NavMesh generator for the current world:
```
  0 - generate points
  1 - connect points
```

- `MOD_NavMeshSave()` - save current NavMesh into `Cecil/Navmeshes` under the world filename
- `MOD_NavMeshLoad()` - load current NavMesh from `Cecil/Navmeshes` under the world filename
- `MOD_NavMeshClear()` - clear the entire NavMesh

## Navmesh editing

- `MOD_iRenderNavMesh` - display world NavMesh:
```
  0 - disabled
  1 - render points
  2 - render connections and range
  3 - display IDs
  4 - display flags
```

- `MOD_iNavMeshPoint` - currently selected NavMesh point by its ID
- `MOD_NavMeshSelectPoint()` - select closest NavMesh point to the player's crosshair position

---
- `MOD_NavMeshConnectionType()` - change NavMesh connection type (see `MOD_iNavMeshConnecting`)
- `MOD_iNavMeshConnecting` - currently selected NavMesh connection type:
```
  0 - disabled (point selection)
  1 - one-way connection (connect current point to the target)
  2 - two-way connection (connect points to each other)
  3 - one-way backwards connection (connect target point to the current one)
```

- `MOD_AddNavMeshPoint(offset)` - create a new NavMesh point at the player position (with vertical offset)
- `MOD_DeleteNavMeshPoint()` - delete currently selected NavMesh point
- `MOD_NavMeshPointInfo()` - display NavMesh point info
- `MOD_ConnectNavMeshPoint(target ID)` - connect NavMesh point to the target point using its ID.
- `MOD_TeleportNavMeshPoint(offset)` - move NavMesh point to the player position (with vertical offset)

---
- `MOD_NavMeshPointPos(x, y, z)` - change NavMesh point position
- `MOD_SnapNavMeshPoint(grid size)` - snap NavMesh point position to a some grid
- `MOD_NavMeshPointFlags(mask)` - set NavMesh point flags as a base 10 bit mask (e.g. 21 = 16 + 4 + 1)
- `MOD_NavMeshPointEntity(entity ID)` - set NavMesh point entity by its ID (-1 for none)
- `MOD_NavMeshPointRange(range)` - change NavMesh point range