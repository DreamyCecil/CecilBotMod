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

#ifndef _CECILBOTS_HOTKEYS_H
#define _CECILBOTS_HOTKEYS_H

#include <Engine/Base/KeyNames.h>

// Key structure (KeyConversion from "Engine/Base/Input.cpp")
struct DECL_DLL SKeyConversion {
  INDEX kc_iKID;
  INDEX kc_iVirtKey;
  INDEX kc_iScanCode;
  char *kc_strName;
  char *kc_strNameTrans;
};

// Name that is not translated (international)
#define INTNAME(str) str, ""
// Name that is translated
#define TRANAME(str) str, "ETRS" str

// [Cecil] 2022-05-11: Some keys are renamed to fit into filenames
// Key conversion table (_akcKeys from "Engine/Base/Input.cpp")
static const SKeyConversion _akcKeyTable[] = {
  // Reserved for 'no-key-pressed'
  { KID_NONE, -1, -1, TRANAME("None")},

  // Numbers row
  { KID_1               , '1',   2, INTNAME("1")},
  { KID_2               , '2',   3, INTNAME("2")},
  { KID_3               , '3',   4, INTNAME("3")},
  { KID_4               , '4',   5, INTNAME("4")},
  { KID_5               , '5',   6, INTNAME("5")},
  { KID_6               , '6',   7, INTNAME("6")},
  { KID_7               , '7',   8, INTNAME("7")},
  { KID_8               , '8',   9, INTNAME("8")},
  { KID_9               , '9',  10, INTNAME("9")},
  { KID_0               , '0',  11, INTNAME("0")},
  { KID_MINUS           , 189,  12, INTNAME("Minus")}, // [Cecil] Renamed
  { KID_EQUALS          , 187,  13, INTNAME("Equals")}, // [Cecil] Renamed

  // 1st alpha row
  { KID_Q               , 'Q',  16, INTNAME("Q")},
  { KID_W               , 'W',  17, INTNAME("W")},
  { KID_E               , 'E',  18, INTNAME("E")},
  { KID_R               , 'R',  19, INTNAME("R")},
  { KID_T               , 'T',  20, INTNAME("T")},
  { KID_Y               , 'Y',  21, INTNAME("Y")},
  { KID_U               , 'U',  22, INTNAME("U")},
  { KID_I               , 'I',  23, INTNAME("I")},
  { KID_O               , 'O',  24, INTNAME("O")},
  { KID_P               , 'P',  25, INTNAME("P")},
  { KID_LBRACKET        , 219,  26, INTNAME("BracketOpen")}, // [Cecil] Renamed
  { KID_RBRACKET        , 221,  27, INTNAME("BracketClose")}, // [Cecil] Renamed
  { KID_BACKSLASH       , 220,  43, INTNAME("Backslash")}, // [Cecil] Renamed

  // 2nd alpha row
  { KID_A               , 'A',  30, INTNAME("A")},
  { KID_S               , 'S',  31, INTNAME("S")},
  { KID_D               , 'D',  32, INTNAME("D")},
  { KID_F               , 'F',  33, INTNAME("F")},
  { KID_G               , 'G',  34, INTNAME("G")},
  { KID_H               , 'H',  35, INTNAME("H")},
  { KID_J               , 'J',  36, INTNAME("J")},
  { KID_K               , 'K',  37, INTNAME("K")},
  { KID_L               , 'L',  38, INTNAME("L")},
  { KID_SEMICOLON       , 186,  39, INTNAME("Semicolon")}, // [Cecil] Renamed
  { KID_APOSTROPHE      , 222,  40, INTNAME("Apostrophe")}, // [Cecil] Renamed

  // 3rd alpha row
  { KID_Z               , 'Z',  44, INTNAME("Z")},
  { KID_X               , 'X',  45, INTNAME("X")},
  { KID_C               , 'C',  46, INTNAME("C")},
  { KID_V               , 'V',  47, INTNAME("V")},
  { KID_B               , 'B',  48, INTNAME("B")},
  { KID_N               , 'N',  49, INTNAME("N")},
  { KID_M               , 'M',  50, INTNAME("M")},
  { KID_COMMA           , 190,  51, INTNAME("Comma")}, // [Cecil] Renamed
  { KID_PERIOD          , 188,  52, INTNAME("Period")}, // [Cecil] Renamed
  { KID_SLASH           , 191,  53, INTNAME("Slash")}, // [Cecil] Renamed

  // Row with F-keys
  { KID_F1              ,  VK_F1,  59, INTNAME("F1")},
  { KID_F2              ,  VK_F2,  60, INTNAME("F2")},
  { KID_F3              ,  VK_F3,  61, INTNAME("F3")},
  { KID_F4              ,  VK_F4,  62, INTNAME("F4")},
  { KID_F5              ,  VK_F5,  63, INTNAME("F5")},
  { KID_F6              ,  VK_F6,  64, INTNAME("F6")},
  { KID_F7              ,  VK_F7,  65, INTNAME("F7")},
  { KID_F8              ,  VK_F8,  66, INTNAME("F8")},
  { KID_F9              ,  VK_F9,  67, INTNAME("F9")},
  { KID_F10             , VK_F10,  68, INTNAME("F10")},
  { KID_F11             , VK_F11,  87, INTNAME("F11")},
  { KID_F12             , VK_F12,  88, INTNAME("F12")},

  // Extra keys
  { KID_ESCAPE          , VK_ESCAPE,     1, TRANAME("Escape")},
  { KID_TILDE           , -1,           41, TRANAME("Tilde")},
  { KID_BACKSPACE       , VK_BACK,      14, TRANAME("Backspace")},
  { KID_TAB             , VK_TAB,       15, TRANAME("Tab")},
  { KID_CAPSLOCK        , VK_CAPITAL,   58, TRANAME("CapsLock")}, // [Cecil] Renamed
  { KID_ENTER           , VK_RETURN,    28, TRANAME("Enter")},
  { KID_SPACE           , VK_SPACE,     57, TRANAME("Space")},

  // Modifier keys
  { KID_LSHIFT          , VK_LSHIFT  , 42, TRANAME("LShift")}, // [Cecil] Renamed
  { KID_RSHIFT          , VK_RSHIFT  , 54, TRANAME("RShift")}, // [Cecil] Renamed
  { KID_LCONTROL        , VK_LCONTROL, 29, TRANAME("LControl")}, // [Cecil] Renamed
  { KID_RCONTROL        , VK_RCONTROL, 256+29, TRANAME("RControl")}, // [Cecil] Renamed
  { KID_LALT            , VK_LMENU   , 56, TRANAME("LAlt")}, // [Cecil] Renamed
  { KID_RALT            , VK_RMENU   , 256+56, TRANAME("RAlt")}, // [Cecil] Renamed

  // Navigation keys
  { KID_ARROWUP         , VK_UP,        256+72, TRANAME("ArrUp")}, // [Cecil] Renamed
  { KID_ARROWDOWN       , VK_DOWN,      256+80, TRANAME("ArrDown")}, // [Cecil] Renamed
  { KID_ARROWLEFT       , VK_LEFT,      256+75, TRANAME("ArrLeft")}, // [Cecil] Renamed
  { KID_ARROWRIGHT      , VK_RIGHT,     256+77, TRANAME("ArrRight")}, // [Cecil] Renamed
  { KID_INSERT          , VK_INSERT,    256+82, TRANAME("Insert")},
  { KID_DELETE          , VK_DELETE,    256+83, TRANAME("Delete")},
  { KID_HOME            , VK_HOME,      256+71, TRANAME("Home")},
  { KID_END             , VK_END,       256+79, TRANAME("End")},
  { KID_PAGEUP          , VK_PRIOR,     256+73, TRANAME("PgUp")}, // [Cecil] Renamed
  { KID_PAGEDOWN        , VK_NEXT,      256+81, TRANAME("PgDn")}, // [Cecil] Renamed
  { KID_PRINTSCR        , VK_SNAPSHOT,  256+55, TRANAME("PrtSc")}, // [Cecil] Renamed
  { KID_SCROLLLOCK      , VK_SCROLL,    70, TRANAME("ScrollLock")}, // [Cecil] Renamed
  { KID_PAUSE           , VK_PAUSE,     69, TRANAME("Pause")},

  // Numpad numbers
  { KID_NUM0            , VK_NUMPAD0, 82, INTNAME("Num0")}, // [Cecil] Renamed
  { KID_NUM1            , VK_NUMPAD1, 79, INTNAME("Num1")}, // [Cecil] Renamed
  { KID_NUM2            , VK_NUMPAD2, 80, INTNAME("Num2")}, // [Cecil] Renamed
  { KID_NUM3            , VK_NUMPAD3, 81, INTNAME("Num3")}, // [Cecil] Renamed
  { KID_NUM4            , VK_NUMPAD4, 75, INTNAME("Num4")}, // [Cecil] Renamed
  { KID_NUM5            , VK_NUMPAD5, 76, INTNAME("Num5")}, // [Cecil] Renamed
  { KID_NUM6            , VK_NUMPAD6, 77, INTNAME("Num6")}, // [Cecil] Renamed
  { KID_NUM7            , VK_NUMPAD7, 71, INTNAME("Num7")}, // [Cecil] Renamed
  { KID_NUM8            , VK_NUMPAD8, 72, INTNAME("Num8")}, // [Cecil] Renamed
  { KID_NUM9            , VK_NUMPAD9, 73, INTNAME("Num9")}, // [Cecil] Renamed
  { KID_NUMDECIMAL      , VK_DECIMAL, 83, INTNAME("NumPeriod")}, // [Cecil] Renamed

  // Numpad grey keys
  { KID_NUMLOCK         , VK_NUMLOCK,   256+69, INTNAME("NumLock")}, // [Cecil] Renamed
  { KID_NUMSLASH        , VK_DIVIDE,    256+53, INTNAME("NumDiv")}, // [Cecil] Renamed
  { KID_NUMMULTIPLY     , VK_MULTIPLY,  55, INTNAME("NumMul")}, // [Cecil] Renamed
  { KID_NUMMINUS        , VK_SUBTRACT,  74, INTNAME("NumSub")}, // [Cecil] Renamed
  { KID_NUMPLUS         , VK_ADD,       78, INTNAME("NumAdd")}, // [Cecil] Renamed
  { KID_NUMENTER        , VK_SEPARATOR, 256+28, TRANAME("NumEnter")}, // [Cecil] Renamed

  // Mouse buttons
  { KID_MOUSE1          , VK_LBUTTON, -1, TRANAME("MB1")}, // [Cecil] Renamed
  { KID_MOUSE2          , VK_RBUTTON, -1, TRANAME("MB2")}, // [Cecil] Renamed
  { KID_MOUSE3          , VK_MBUTTON, -1, TRANAME("MB3")}, // [Cecil] Renamed
  { KID_MOUSE4          , -1, -1, TRANAME("MB4")}, // [Cecil] Renamed
  { KID_MOUSE5          , -1, -1, TRANAME("MB5")}, // [Cecil] Renamed
  { KID_MOUSEWHEELUP    , -1, -1, TRANAME("WheelUp")}, // [Cecil] Renamed
  { KID_MOUSEWHEELDOWN  , -1, -1, TRANAME("WheelDown")}, // [Cecil] Renamed

  // 2nd mouse buttons
  { KID_2MOUSE1         , -1, -1, TRANAME("2MB1")}, // [Cecil] Renamed
  { KID_2MOUSE2         , -1, -1, TRANAME("2MB2")}, // [Cecil] Renamed
  { KID_2MOUSE3         , -1, -1, TRANAME("2MB3")}, // [Cecil] Renamed
};

static const INDEX _ctKeysInTable = ARRAYCOUNT(_akcKeyTable);

// [Cecil] 2022-05-11: Hotkey structure
struct DECL_DLL SHotkey {
  INDEX iKey; // Key index (KID_*)
  CTString strPressed; // Shell command upon pressing the key
  CTString strReleased; // Shell command upon releasing the key

  // Constructor
  SHotkey() : iKey(-1), strPressed(""), strReleased("") {};
};

// [Cecil] 2022-05-11: Load commands from a config for some key
DECL_DLL BOOL LoadCommands(SHotkey &hk, const SKeyConversion &kc);

#endif // _CECILBOTS_BOTMOD_H
