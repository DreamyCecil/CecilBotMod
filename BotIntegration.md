### All projects

- `Bots` folder should be present at the very root of mod sources (alongside Engine, EntitiesMP etc.).
- `Bots/_BotMod.h` is included in `StdH.h` in EntitiesMP (at the end) and in `stdafx.h` in GameMP (instead of `EntitiesMP/Player.h`).

### EntitiesMP

- All instances of `GetMaxPlayers()` and `GetPlayerEntity()` are replaced with `CECIL_PlayerCount()` and `CECIL_GetPlayerEntity()` respectively (except for three places where it's noted not to do so in `Player.es`).
- All instances of `IsOfClass(<ptr>, "Player")` and `IsDerivedFromClass(<ptr>, "Player")` are replaced with `IS_PLAYER(<ptr>)`. Can be achieved automatically by replacing `, "Player")` with `)` and then fixing errors where they appear.
- All `Player.es` additions that are on top of the vanilla `Player.es` (commented with `// [Cecil]`).

### GameMP

- Initialization function at the end of `CGame::InitInternal()`:
  ```cpp
  CECIL_InitBotMod();
  ```
- End function at the end of `CGame::EndInternal()`:
  ```cpp
  CECIL_EndBotMod();
  ```
- Bot game start function in `CGame::NewGame()` right after `_pNetwork->StartPeerToPeer_t`:
  ```cpp
  CECIL_BotGameStart(sp);
  ```
- Bot game cleanup at the very beginning of `CGame::StopGame()`:
  ```cpp
  CECIL_BotGameCleanup();
  ```
- To make bots appear in the observer views, follow these steps (all within `CGame::GameRedrawView()` function):
  1. Create a container of player entities like so: `CDynamicContainer<CPlayer> cenPlayerEntities;` before these lines:
  ```cpp
    // fill in all players that are not local
    INDEX ctNonlocals = 0;
  ```
  2. Remove this array after it:
  ```cpp
    CEntity *apenNonlocals[16];
    memset(apenNonlocals, 0, sizeof(apenNonlocals));
  ```
  3. Replace `apenNonlocals[ctNonlocals++] = pen;` with `cenPlayerEntities.Add((CPlayer *)pen);` within the `for` loop.
  4. Add the following block after the `for` loop:
  ```cpp
    // [Cecil] Add bots
    for (INDEX iBot = 0; iBot < _cenPlayerBots.Count(); iBot++) {
      CPlayerBot *penBot = _cenPlayerBots.Pointer(iBot);
      cenPlayerEntities.Add(penBot);
    }
    // [Cecil] Count non-local players
    ctNonlocals = cenPlayerEntities.Count();
  ```
  5. Replace `apenViewers[ctViewers++] = apenNonlocals[iPlayer];` with `apenViewers[ctViewers++] = cenPlayerEntities.Pointer(iPlayer);` within the next `for` block.
  
  Resulting block of code:
  ```cpp
    CDynamicContainer<CPlayer> cenPlayerEntities;

    // fill in all players that are not local
    INDEX ctNonlocals = 0;
    {for (INDEX i=0; i<16; i++) {
      CEntity *pen = CEntity::GetPlayerEntity(i);
      if (pen!=NULL && !_pNetwork->IsPlayerLocal(pen)) {
        cenPlayerEntities.Add((CPlayer *)pen);
      }
    }}

    // [Cecil] Add bots
    for (INDEX iBot = 0; iBot < _cenPlayerBots.Count(); iBot++) {
      CPlayerBot *penBot = _cenPlayerBots.Pointer(iBot);
      cenPlayerEntities.Add(penBot);
    }
    // [Cecil] Count non-local players
    ctNonlocals = cenPlayerEntities.Count();

    // if there are any non-local players
    if (ctNonlocals>0) {
      // for each observer
      {for (INDEX i=0; i<ctObservers; i++) {
        // get the given player with given offset that is not local
        INDEX iPlayer = (i+iObserverOffset)%ctNonlocals;
        apenViewers[ctViewers++] = cenPlayerEntities.Pointer(iPlayer);
      }}
    }
  ```