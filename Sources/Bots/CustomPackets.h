/* Copyright (c) 2018-2023 Dreamy Cecil
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

#ifndef _CECILBOTS_CUSTOMPACKETS_H
#define _CECILBOTS_CUSTOMPACKETS_H

#include "Engine/Network/NetworkMessage.h"

// Start with 49 to continue the NetworkMessageType list
enum ECecilPackets {
  MSG_CECIL_SANDBOX = 49, // Sandbox action
};

// [Cecil] 2019-05-28: Sandbox Action Types
enum ESandboxAction {
  // Admin actions
  ESA_ADDBOT, // Add a new bot
  ESA_REMBOT, // Remove a bot
  ESA_UPDATEBOT, // Update bot settings
  ESA_TELEPORTBOTS, // Teleport specific bots to the player

  ESA_SETWEAPONS, // Change all weapons in the world

  ESA_NAVMESH_GEN,   // Generate Navigation Mesh
  ESA_NAVMESH_LOAD,  // Load the NavMesh
  ESA_NAVMESH_CLEAR, // Clear the NavMesh

  ESA_LAST_ADMIN = ESA_NAVMESH_CLEAR, // Last admin action

  // [Cecil] 2019-11-07: NavMesh Editing Actions
  ESA_NAVMESH_CREATE,   // Add a new path point
  ESA_NAVMESH_DELETE,   // Delete a path point
  ESA_NAVMESH_CONNECT,  // Connect two points
  ESA_NAVMESH_UNTARGET, // Untarget one point from another
  ESA_NAVMESH_TELEPORT, // Move the point to the player

  ESA_NAVMESH_POS,    // Change point's position
  ESA_NAVMESH_SNAP,   // Snap point's position
  ESA_NAVMESH_FLAGS,  // Change point's flags
  ESA_NAVMESH_ENTITY, // Change point's entity
  ESA_NAVMESH_RANGE,  // Change point's range
  ESA_NAVMESH_NEXT,   // Change point's next important point
  ESA_NAVMESH_LOCK,   // Change point's lock entity
};

// [Cecil] 2022-04-27: Handle packets coming from a client (CServer::Handle alternative)
BOOL ServerHandlePacket(CMessageDispatcher &md, INDEX iClient, CNetworkMessage &nmReceived);

// [Cecil] 2022-04-26: Handle custom packets coming from a server
BOOL HandleCustomPacket(CSessionState *pses, CNetworkMessage &nmMessage);

#endif // _CECILBOTS_CUSTOMPACKETS_H
