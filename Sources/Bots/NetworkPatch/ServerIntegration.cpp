/* Copyright (c) 2021-2023 Dreamy Cecil
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
#include "ServerIntegration.h"

#include "Bots/Patcher/patcher.h"
#include "Bots/CustomPackets.h"

// Server patches
class CCecilMessageDispatcher : public CMessageDispatcher {
  public:
    BOOL P_ReceiveFromClient(INDEX iClient, CNetworkMessage &nmMessage);
    BOOL P_ReceiveFromClientReliable(INDEX iClient, CNetworkMessage &nmMessage);
};

// Client patches
class CCecilSessionState : public CSessionState {
  public:
    void P_ProcessGameStreamBlock(CNetworkMessage &nmMessage);
};

// Add a block to streams for all sessions
void CECIL_AddBlockToAllSessions(CCecilStreamBlock &nsb) {
  CServer &srv = _pNetwork->ga_srvServer;

  // For each active session
  for (INDEX iSession = 0; iSession < srv.srv_assoSessions.Count(); iSession++) {
    CSessionSocket &sso = srv.srv_assoSessions[iSession];

    if (iSession > 0 && !sso.sso_bActive) {
      continue;
    }

    // Add the block to the buffer
    ((CCecilNetworkStream &)sso.sso_nsBuffer).AddBlock(nsb);
  }
};

// Original function pointers
typedef void (CSessionState::*CProcGameStreamBlockFunc)(CNetworkMessage &);
static CProcGameStreamBlockFunc pProcGameStreamBlock = NULL;

// Patch some networking functions
extern void CECIL_ApplyNetworkPatches(void) {
  // Receive messages from the client
  BOOL (CMessageDispatcher::*pRecFromClient)(INDEX, CNetworkMessage &) = &CMessageDispatcher::ReceiveFromClient;
  CPatch *pPatchRFC = new CPatch(pRecFromClient, &CCecilMessageDispatcher::P_ReceiveFromClient, true, true);

  BOOL (CMessageDispatcher::*pRecFromClientReliable)(INDEX, CNetworkMessage &) = &CMessageDispatcher::ReceiveFromClientReliable;
  CPatch *pPatchRFCR = new CPatch(pRecFromClientReliable, &CCecilMessageDispatcher::P_ReceiveFromClientReliable, true, true);

  // Process game stream block from the server
  pProcGameStreamBlock = &CSessionState::ProcessGameStreamBlock;
  CPatch *pPatchPGSB = new CPatch(pProcGameStreamBlock, &CCecilSessionState::P_ProcessGameStreamBlock, true, true);
};

// Server receives a packet
BOOL CCecilMessageDispatcher::P_ReceiveFromClient(INDEX iClient, CNetworkMessage &nmMessage) {
  // Receive message in static buffer
  nmMessage.nm_slSize = nmMessage.nm_slMaxSize;
  BOOL bReceived = _cmiComm.Server_Receive_Unreliable(iClient, (void *)nmMessage.nm_pubMessage, nmMessage.nm_slSize);

  // If there is message
  if (bReceived) {
    // Init the message structure
    nmMessage.nm_pubPointer = nmMessage.nm_pubMessage;
    nmMessage.nm_iBit = 0;

    UBYTE ubType;
    nmMessage.Read(&ubType, sizeof(ubType));
    nmMessage.nm_mtType = (MESSAGETYPE)ubType;

    // Replace default CServer packet processor or return TRUE to process through the original function
    return ServerHandlePacket(*this, iClient, nmMessage);
  }

  return bReceived;
};

// Server receives a reliable packet
BOOL CCecilMessageDispatcher::P_ReceiveFromClientReliable(INDEX iClient, CNetworkMessage &nmMessage) {
  // Receive message in static buffer
  nmMessage.nm_slSize = nmMessage.nm_slMaxSize;
  BOOL bReceived = _cmiComm.Server_Receive_Reliable(iClient, (void *)nmMessage.nm_pubMessage, nmMessage.nm_slSize);

  // If there is a message
  if (bReceived) {
    // Init the message structure
    nmMessage.nm_pubPointer = nmMessage.nm_pubMessage;
    nmMessage.nm_iBit = 0;

    UBYTE ubType;
    nmMessage.Read(&ubType, sizeof(ubType));
    nmMessage.nm_mtType = (MESSAGETYPE)ubType;
    
    // Replace default CServer packet processor or return TRUE to process through the original function
    return ServerHandlePacket(*this, iClient, nmMessage);
  }

  return bReceived;
};

// Client processes received packet from the server
void CCecilSessionState::P_ProcessGameStreamBlock(CNetworkMessage &nmMessage) {
  // Copy the tick to process into tick used for all tasks
  _pTimer->SetCurrentTick(ses_tmLastProcessedTick);

  // If cannot handle custom packet
  if (HandleCustomPacket(this, nmMessage)) {
    // Call the original function for standard packets
    (this->*pProcGameStreamBlock)(nmMessage);
  }
};
