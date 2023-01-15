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

#ifndef _CECILBOTS_BOTTHOUGHTS_H
#define _CECILBOTS_BOTTHOUGHTS_H

// [Cecil] 2021-06-20: Bot thoughts
struct SBotThoughts {
  CTString strThoughts[16]; // Thoughts
  CTString strLast; // Last thought text

  // Constructor
  SBotThoughts(void) {
    Reset();
  };

  // Reset thoughts
  void Reset(void) {
    for (INDEX i = 0; i < 16; i++) {
      strThoughts[i] = "";
    }

    strLast = "";
  };

  // Push new thought
  void Push(const CTString &str) {
    for (INDEX i = 15; i > 0; i--) {
      strThoughts[i] = strThoughts[i - 1];
    }
    SetLast(str);
  };

  // Set new last thought
  void SetLast(const CTString &str) {
    strThoughts[0].PrintF("[%s] %s", TimeToString(_pTimer->CurrentTick()), str);
    strLast = str;
  };
};

#endif // _CECILBOTS_BOTTHOUGHTS_H
