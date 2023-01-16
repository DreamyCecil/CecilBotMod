/* Copyright (c) 2002-2012 Croteam Ltd.
   Copyright (c) 2022-2023 Dreamy Cecil

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

#include "Hotkeys.h"

// [Cecil] 2022-05-11: Load commands from a config for some key
BOOL LoadCommands(SHotkey &hk, const SKeyConversion &kc) {
  // Use untranslated key name in config files
  CTFileName fnPressed = CTString(0, "Scripts\\Hotkeys\\%s_Pressed.ini", kc.kc_strName);
  CTFileName fnReleased = CTString(0, "Scripts\\Hotkeys\\%s_Released.ini", kc.kc_strName);

  CTString strCommand;
  BOOL bLoaded = FALSE; // Loaded at least something

  hk.iKey = kc.kc_iKID;
  
  // Load commands upon pressing the key
  try {
    strCommand.Load_t(fnPressed);

    hk.strPressed = strCommand;
    bLoaded = TRUE;

  // Ignore errors if can't load
  } catch (char *strError) {
    (void)strError;
  }

  // Load commands upon releasing the key
  try {
    strCommand.Load_t(fnReleased);

    hk.strReleased = strCommand;
    bLoaded = TRUE;

  // Ignore errors if can't load
  } catch (char *strError) {
    (void)strError;
  }

  return bLoaded;
};
