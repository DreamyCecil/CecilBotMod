## Key shortcuts in-game

- `=` - quickly add one bot
- `Num .` - change NavMesh connection type (from `0` to `3`)
- `Num 1` - select closest NavMesh point to the crosshair (or target it if connection type is not `0`)
- `Num +` - add a new NavMesh point
- `Num -` - delete selected NavMesh point
- `Num *` - move NavMesh point to the player position
- `Num /` - snap NavMesh point position to `0.25` grid
- `Pg Up` - increase NavMesh point range by `1`
- `Pg Dn` - decrease NavMesh point range by `1`
- `Num Enter` - display information about a NavMesh point

## Bot Mod

- `MOD_QuickBot()` - quick bot addition (uses `BOT_strSpawnName` and `BOT_strSpawnTeam`)
- `MOD_AddBot(name, AMC filename, team)` - bot addition with custom name and player skin (empty strings for random) and also team (empty string for no team)
- `MOD_RemoveBot(name)` - remove bots with a certain name from the game
- `MOD_RemoveAllBots()` - remove all bots from the game
- `MOD_BotUpdate()` - update settings for all bots (if `BOT_strBotEdit` isn't blank, only updates bots with that name)

---
- `BOT_strBotEdit` - name of a bot for editing its settings (leave blank for all bots)
- `BOT_strSpawnName` - quick add bots with this specific name
- `BOT_strSpawnTeam` - quick add bots in this specific team
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

- `MOD_fNavMeshRenderRange` - NavMesh point rendering range (`0` for infinite)
- `MOD_iNavMeshPoint` - currently selected NavMesh point by its ID
- `MOD_NavMeshSelectPoint()` - select closest NavMesh point to the player's crosshair position or target already selected point (if `MOD_iNavMeshConnecting` is not `0`)

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
- `MOD_NavMeshPointNext(point ID)` - next important point to go to after this one

---
- `MOD_NavMeshPointLock(entity ID)` - set some entity as the point's "lock" (meaning that this point will only be accessible when the set entity is at the exact same place as it was when it's been set)
```
If you see a [pale yellow] line going to a NavMesh point, it means that it's connected to this "lock entity"
If you see any of these lines connected to the set "lock" entity, it means that the NavMesh point is currently inaccessible:
  [Pale green] line indicates the difference between the origin position and the current position of the "lock" entity
  [Pale blue] line indicates origin angle of the "lock" entity
  [Pale red] line indicates the current angle of the "lock" entity
```