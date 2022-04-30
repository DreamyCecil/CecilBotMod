/* Copyright (c) 2018-2021 Dreamy Cecil
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
#include "CustomPackets.h"
#include "Bots/NetworkPatch/ServerIntegration.h"

// [Cecil] 2022-04-27: Handle packets coming from a client (CServer::Handle alternative)
BOOL ServerHandlePacket(CMessageDispatcher &md, INDEX iClient, CNetworkMessage &nmReceived) {
  CServer &srv = _pNetwork->ga_srvServer;

  switch (nmReceived.GetType())
  {
    // [Cecil] Sandbox actions
    case MSG_CECIL_SANDBOX: {
      INDEX iAction;
      nmReceived >> iAction;
      
      extern INDEX MOD_bClientSandbox;

      // If not an admin or no permissions
      if (!_cmiComm.Server_IsClientLocal(iClient) && (iAction <= ESA_LAST_ADMIN || !MOD_bClientSandbox)) {
        nmReceived.IgnoreContents();

        // Reply to the client
        CNetworkMessage nmReply(MSG_CHAT_OUT);
        nmReply << (ULONG)0; // From
        nmReply << CTString("Server"); // Sender
        nmReply << CTString("You don't have permission to use this command!");

        _pNetwork->SendToClient(iClient, nmReply);
        return FALSE;
      }

      nmReceived.Rewind();

      // Forward the packet to all clients
      CCecilStreamBlock nsb(nmReceived, ++srv.srv_iLastProcessedSequence);
      
      CECIL_AddBlockToAllSessions(nsb);

    } return FALSE;
  }

  // Let CServer::Handle process other packets
  return TRUE;
};

// [Cecil] 2022-04-26: Handle custom packets coming from a server
BOOL HandleCustomPacket(CSessionState *pses, CNetworkMessage &nmMessage) {
  switch (nmMessage.GetType())
  {
    // [Cecil] Sandbox actions
    case MSG_CECIL_SANDBOX: {
      INDEX iAction, iPlayer;
      nmMessage >> iAction >> iPlayer;

      CPlayer *pen = NULL;

      if (iPlayer != -1) {
        pen = (CPlayer *)CEntity::GetPlayerEntity(iPlayer);
      }

      // Perform sandbox action
      CECIL_SandboxAction(pen, iAction, nmMessage);

    } return FALSE;
  }

  // Let default methods handle packets
  return TRUE;
};
