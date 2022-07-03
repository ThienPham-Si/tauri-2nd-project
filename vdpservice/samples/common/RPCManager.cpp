/* ********************************************************************************* *
 * Copyright (C) 2011-2021 VMware, Inc.  All rights reserved. -- VMware Confidential *
 * ********************************************************************************* */

/*
 * RPCManager.cpp --
 *
 */

#include "stdafx.h"
#include "RPCManager.h"

#ifndef INVALID_SOCKET
#define INVALID_SOCKET        -1
#endif

/*
 * RPCManager::s_instance
 *
 *    This is the single instance of the RPCManager
 */
RPCManager* RPCManager::s_instance = NULL;

#ifndef _WIN32
   #if !defined(_TRUNCATE)
   #define _TRUNCATE (-1)
   #endif
#endif

/*
 * These are the functions that each of the plugins need to call
 * inside vdp global functions.
 */
/*
 *----------------------------------------------------------------------
 *
 * RPCManager::VDPPluginInit --
 *
 *    This function needs to be invoked by each of the plugin inside
 *    VdpService_PluginInit. VDP Service DLL calls the plugin function
 *    right after it loads the plugin dll.
 *
 *----------------------------------------------------------------------
 */

bool
RPCManager::VDPPluginInit(const VDP_SERVICE_QUERY_INTERFACE qi) // IN
{
   LOG_FUNC_NAME;
   ClientInit(&qi);
   return true;
}


/*
 *----------------------------------------------------------------------------
 *
 * RPCManager::VDPPluginExit --
 *
 *    This function needs to be invoked by each of the plugin inside
 *    VdpService_PluginExit. VDP Service DLL calls the plugin function
 *    right before it unloads the plugin dll.
 *
 *----------------------------------------------------------------------------
 */

bool
RPCManager::VDPPluginExit(void)
{
   LOG("Plugin with token %s is exiting.\n", this->TokenName());
   ClientExit();
   return true;
}


/*
 *----------------------------------------------------------------------
 *
 * RPCManager::VDPPluginCreateInstance --
 *
 *    This function needs to be invoked by each of the plugin inside
 *    VdpService_PluginCreateInstance which is called by the VDP Service
 *    DLL each time a server application is started that uses the same
 *    token as the plugin DLL.
 *
 *----------------------------------------------------------------------
 */

bool
RPCManager::VDPPluginCreateInstance(void* hChannel,    // IN
                                    void** pUserData)  // OUT
{
   LOG_FUNC_NAME;

   if (!m_initialized) {
      LOG("Warning: Plugin is not initialized yet.\n");
      return false;
   }

   RPCPluginInstance* rpcPlugin = OnCreateInstance();
   rpcPlugin->RegisterChannelSink(hChannel);

   *pUserData = (void*)rpcPlugin;
   return true;
}


/*
 *----------------------------------------------------------------------
 *
 * RPCManager::VDPPluginDestroyInstance --
 *
 *    This function needs to be invoked by each of the plugin inside
 *    VdpService_PluginDestroyInstance which called by the VDP Service DLL
 *    each time a server application is closed that uses the same token
 *    as the plugin DLL.
 *
 *----------------------------------------------------------------------
 */

bool
RPCManager::VDPPluginDestroyInstance(void* userData)  // IN
{
   LOG_FUNC_NAME;
   RPCPluginInstance* rpcPlugin = static_cast<RPCPluginInstance*>(userData);

   rpcPlugin->ChannelDisconnect();
   rpcPlugin->UnregisterChannelSink();

   OnDestroyInstance(rpcPlugin);
   return true;
}


/*
 *----------------------------------------------------------------------
 *
 * Class RPCManager --
 *
 *----------------------------------------------------------------------
 */
RPCManager::RPCManager(const char* tokenName)   // IN
   : m_isServer(false),
     m_serverInit(false),
     m_initialized(false),
     m_channelType(VDPSERVICE_MAIN_CHANNEL),
     m_compressionEnabled(true),
     m_encryptionEnabled(true)
{
   s_instance = this;

   strncpy_s(m_tokenName, sizeof m_tokenName, tokenName, _TRUNCATE);
   m_tokenName[sizeof(m_tokenName) - 1] = 0;
   strcpy_s(m_channelObjName, sizeof m_channelObjName, m_tokenName);
   strcat_s(m_channelObjName, sizeof m_channelObjName, "Obj");

   memset(&m_qi,             0, sizeof m_qi);
   memset(&m_iChannel,       0, sizeof m_iChannel);
   memset(&m_iChannelObj,    0, sizeof m_iChannelObj);
   memset(&m_iChannelCtx,    0, sizeof m_iChannelCtx);
   memset(&m_iVariant,       0, sizeof m_iVariant);
   memset(&m_iOverlayGuest,  0, sizeof m_iOverlayGuest);
   memset(&m_iOverlayClient, 0, sizeof m_iOverlayClient);
   memset(&m_iStreamData,    0, sizeof m_iStreamData);
   memset(&m_iVdpObserverInterface, 0, sizeof m_iVdpObserverInterface);

   m_channelSink.version = VDP_SERVICE_CHANNEL_NOTIFY_SINK_V1;
   m_channelSink.v1.OnConnectionStateChanged = OnConnectionStateChanged;
   m_channelSink.v1.OnChannelStateChanged = OnChannelStateChanged;
   m_channelSink.v1.OnPeerObjectCreated = OnPeerChannelObjectCreated;

   m_channelObjSink.version = VDP_RPC_OBJECT_NOTIFY_SINK_V1;
   m_channelObjSink.v1.OnInvoke = OnMsgInvoke;
   m_channelObjSink.v1.OnObjectStateChanged = OnChannelObjectStateChanged;

   m_requestSink.version = VDP_RPC_REQUEST_CALLBACK_V1;
   m_requestSink.v1.OnDone = OnMsgDone;
   m_requestSink.v1.OnAbort = OnMsgAbort;
}

RPCManager::~RPCManager()
{
}


/*
 *----------------------------------------------------------------------
 *
 * RPCManager::ServerInit2 --
 *
 *    Initialize the RPCManager to run on the Agent.
 *
 * Results:
 *    Returns true if everything went well.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

bool
RPCManager::ServerInit2(DWORD sid,                     // IN
                        VdpServiceChannelType type,    // IN
                        bool compressionEnabled,       // IN
                        bool encryptionEnabled,        // IN
                        RPCPluginInstance* rpcPlugin,  // IN
                        uint32 msTimeoutReady)         // IN
{
   FUNCTION_TRACE;

   if (!AllowRunAsServer()) {
      FUNCTION_EXIT_MSG("Running as server not allowed.");
      return false;
   }

#ifdef _WIN32
   if (m_initialized) {
      FUNCTION_EXIT_MSG("Already initialized");
      return false;
   }

   void* hChannel = NULL;
   m_channelType = type;

   VDP_SERVICE_QUERY_INTERFACE qi;
   // You could call VDPService_ServerInit for CURRENT SESSION
   m_serverInit = VDPService_ServerInit2(sid,
                                         m_tokenName,
                                         &qi,
                                         &hChannel) != 0;
   if (!m_serverInit) {
      FUNCTION_EXIT_MSG("VDPService_ServerInit2() failed");
      return false;
   }

   if (!Init(true, &qi)) {
      FUNCTION_EXIT_MSG("Init() failed");
      return false;
   }

   if (!rpcPlugin->RegisterChannelSink(hChannel)) {
      FUNCTION_EXIT_MSG("RegisterChannelSink() failed");
      return false;
   }

   if (msTimeoutReady != 0) {
      if (!rpcPlugin->WaitUntilReady(msTimeoutReady)) {
         FUNCTION_EXIT_MSG("WaitUntilReady() failed");
         return false;
      }
   }

   if (encryptionEnabled) {
      // Ensure the setting of encryption is valid.
      VM_ASSERT(m_channelType == VDPSERVICE_TCP_CHANNEL ||
                m_channelType == VDPSERVICE_TCPRAW_CHANNEL);
      if ((VDP_RPC_CRYPTO_AES & rpcPlugin->m_channelObjOptions) == 0) {
         LOG("Error: Peer does not support encryption.\n");
         VDPService_ServerExit2(sid);
         return false;
      } else {
         m_encryptionEnabled = encryptionEnabled;
      }
   }

   if (compressionEnabled) {
      VM_ASSERT(m_channelType != VDPSERVICE_MAIN_CHANNEL);
      if ((VDP_RPC_COMP_SNAPPY & rpcPlugin->m_channelObjOptions) == 0) {
         VDPService_ServerExit2(sid);
         LOG("Error: Peer dose not support compression.\n");
         return false;
      } else {
         m_compressionEnabled = compressionEnabled;
      }
   }

   OnServerInit();
#endif

   m_initialized = true;
   return true;
}


/*
 *----------------------------------------------------------------------
 *
 * RPCManager::ServerInit --
 *
 *    convenient function for code written with previous SDK release.
 *    Initialize the RPCManager to run on the Agent.
 *
 * Results:
 *    Returns true if everything went well.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

bool
RPCManager::ServerInit(RPCPluginInstance* rpcPlugin,  // IN
                       uint32 msTimeoutReady)         // IN
{
    return ServerInit2(VDP_CURRENT_SESSION, VDPSERVICE_MAIN_CHANNEL,
                       false, false, rpcPlugin, msTimeoutReady);
}


/*
 *----------------------------------------------------------------------
 *
 * RPCManager::ServerExit2 --
 *
 *    Close down the RPCManager Server resources.
 *
 * Results:
 *    Returns true if everything went well.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

bool
RPCManager::ServerExit2(DWORD sid,                    // IN
                        RPCPluginInstance* rpcPlugin) // IN
{
   FUNCTION_TRACE;
   bool ok = true;

   if (!m_initialized) {
      FUNCTION_EXIT_MSG("Not initialized");
      return false;
   }

#ifdef _WIN32
   static const uint32 msTimeout = 10*1000;
   rpcPlugin->WaitForPendingMessages(msTimeout);

   if (!rpcPlugin->ChannelDisconnect()) {
      LOG("ChannelDisconnect() failed");
      ok = false;
   }

   if (!rpcPlugin->UnregisterChannelSink()) {
      LOG("UnregisterChannelSink() failed");
      ok = false;
   }

   if (m_serverInit) {
      if (!VDPService_ServerExit2(sid)) {
         LOG("VDPService_ServerExit2() failed");
         ok = false;
      } else {
         LOG("VDPService_ServerExit2() [OK]");
      }
      m_serverInit = false;
   }

   OnServerExit();
#endif

   m_initialized = false;
   return ok;
}


/*
 *----------------------------------------------------------------------
 *
 * RPCManager::ServerExit --
 *
 *    convenient function for code written with previous SDK release.
 *    Close down the RPCManager Server resources for current session.
 *
 *
 * Results:
 *    Returns true if everything went well.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

bool
RPCManager::ServerExit(RPCPluginInstance* rpcPlugin) // IN
{
   return ServerExit2(VDP_CURRENT_SESSION, rpcPlugin);
}


/*
 *----------------------------------------------------------------------
 *
 * RPCManager::ClientInit --
 *
 *    Initialize the RPCManager to run on the Client.
 *
 * Results:
 *    Returns true if everything went well.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

bool
RPCManager::ClientInit(const VDP_SERVICE_QUERY_INTERFACE* qi) // IN
{
   FUNCTION_TRACE;

   if (m_initialized) {
      LOG("Already initialized");
      return false;
   }

   if (!Init(false, qi)) {
      FUNCTION_EXIT_MSG("Init() failed");
      return false;
   }

   m_initialized = true;
   OnClientInit();
   return true;
}


/*
 *----------------------------------------------------------------------
 *
 * RPCManager::ClientExit --
 *
 *    Close down the RPCManager Client resources.
 *
 * Results:
 *    Returns true if everything went well.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

bool
RPCManager::ClientExit()
{
   FUNCTION_TRACE;

   if (!m_initialized) {
      FUNCTION_EXIT_MSG("Not initialized");
      return false;
   }

   m_initialized = false;
   OnClientExit();
   return true;
}


/*
 *----------------------------------------------------------------------
 *
 * RPCManager::Init --
 *
 *    Initialize the RPCManager common resources.
 *
 * Results:
 *    Returns true if everything went well.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

bool
RPCManager::Init(bool isServer,                         // IN
                 const VDP_SERVICE_QUERY_INTERFACE* qi) // IN
{
   FUNCTION_TRACE;

   if (!qi->QueryInterface(&GUID_VDPService_ChannelInterface_V2,
                           (void*)&m_iChannel)) {
      if (!qi->QueryInterface(&GUID_VDPService_ChannelInterface_V1,
                           (void*)&m_iChannel)) {
         FUNCTION_EXIT_MSG("Failed to get VDPService_ChannelInterface.");
         return false;
      }
   } else {
      LOG("Warning: Failed to get version2 VDPService_ChannelInterface.");
   }

   if (!qi->QueryInterface(&GUID_VDPRPC_ChannelObjectInterface_V3,
                           (void*)&m_iChannelObj)) {
      if (!qi->QueryInterface(&GUID_VDPRPC_ChannelObjectInterface_V2,
                              (void*)&m_iChannelObj)) {
         if (!qi->QueryInterface(&GUID_VDPRPC_ChannelObjectInterface_V1,
                                 (void*)&m_iChannelObj)) {
            FUNCTION_EXIT_MSG("Failed to get VDPRPC_ChannelObjectInterface");
            return false;
         }
      } else {
         LOG("Warning: Failed to get version 2 VDPRPC_ChannelObjectInterface.");
      }
   } else {
      LOG("Warning: Failed to get version 3 VDPRPC_ChannelObjectInterface.");
   }

   if (!qi->QueryInterface(&GUID_VDPRPC_ChannelContextInterface_V2,
                           (void*)&m_iChannelCtx)) {
      if (!qi->QueryInterface(&GUID_VDPRPC_ChannelContextInterface_V1,
                              (void*)&m_iChannelCtx)) {
         FUNCTION_EXIT_MSG("Failed to get VDPRPC_ChannelContextInterface.");
         return false;
      }
   } else {
      LOG("Warning: Failed to get VDPRPC_ChannelContextInterface.");
   }

   if (!qi->QueryInterface(&GUID_VDPRPC_VariantInterface_V1,
                           (void *)&m_iVariant)) {
      FUNCTION_EXIT_MSG("Failed to get VDPRPC_VariantInterface.");
      return false;
   }

   if (isServer) {
      if (!qi->QueryInterface(&GUID_VDPOverlay_GuestInterface_V4,
                              (void *)&m_iOverlayGuest)) {

         if (!qi->QueryInterface(&GUID_VDPOverlay_GuestInterface_V3,
                                 (void *)&m_iOverlayGuest)) {

            if (!qi->QueryInterface(&GUID_VDPOverlay_GuestInterface_V2,
                                    (void *)&m_iOverlayGuest)) {

               if (!qi->QueryInterface(&GUID_VDPOverlay_GuestInterface_V1,
                                       (void *)&m_iOverlayGuest)) {

                  FUNCTION_EXIT_MSG("Failed to get VDPOverlay_GuestInterface.");
                  return false;
               }
            }
         }
      }
   } else {
      if (!qi->QueryInterface(&GUID_VDPOverlay_ClientInterface_V4,
                              (void *)&m_iOverlayClient)) {

         if (!qi->QueryInterface(&GUID_VDPOverlay_ClientInterface_V3,
                                 (void *)&m_iOverlayClient)) {

            if (!qi->QueryInterface(&GUID_VDPOverlay_ClientInterface_V2,
                                    (void *)&m_iOverlayClient)) {

               if (!qi->QueryInterface(&GUID_VDPOverlay_ClientInterface_V1,
                                       (void *)&m_iOverlayClient)) {

                  FUNCTION_EXIT_MSG("Failed to get VDPOverlay_ClientInterface.");
                  return false;
               }
            }
         }
      }
   }

   if (!qi->QueryInterface(&GUID_VDPRPC_StreamDataInterface_V2,
                           (void*)&m_iStreamData)) {
      if (!qi->QueryInterface(&GUID_VDPRPC_StreamDataInterface_V1,
                           (void*)&m_iStreamData)) {
         FUNCTION_EXIT_MSG("Failed to get VDPRPC_StreamDataInterface.");
         return false;
      }
   } else {
      FUNCTION_EXIT_MSG("Failed to get version2 VDPRPC_StreamDataInterface.");
   }

   if (!qi->QueryInterface(&GUID_VDPService_ObserverInterface_V1,
                           (void *)&m_iVdpObserverInterface)) {
      FUNCTION_EXIT_MSG("query observer interface failed.");
      return false;
   }

   m_isServer = isServer;
   m_qi = *qi;
   return true;
}


/*
 *----------------------------------------------------------------------
 *
 * RPCManager::QueryInterface --
 *
 *    Requests the interface indicated by the GUID.
 *
 * Results:
 *    Returns true if everything went well.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

bool
RPCManager::QueryInterface(const GUID *iid,  // IN
                           void *iface)      // OUT
{
   if (m_qi.QueryInterface == NULL) {
      return false;
   }

   return m_qi.QueryInterface(iid, iface) != 0;
}


/*
 *----------------------------------------------------------------------
 *
 * RPCManager::OnConnectionStateChanged --
 *
 *    Callback function for state change in the underlying
 *    PCoIP connection.
 *
 * Results:
 *    None.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

void
RPCManager::OnConnectionStateChanged(void *userData,                            // IN
                                     VDPService_ConnectionState currentState,   // IN
                                     VDPService_ConnectionState transientState, // IN
                                     void *reserved)                            // IN
{
   RPCPluginInstance* rpcPlugin = static_cast<RPCPluginInstance*>(userData);

   /*
    * Log the state
    *
    *    The "transientState" is the state that caused the event to be sent.
    *    The "currentState" is just that, the current state of the connection.
    *    They can be different due to delays in processing the callback.
    */
   FUNCTION_TRACE_MSG("Connection is now %s", ConnectionStateToStr(transientState));
   if (transientState != currentState) {
      LOG("   but the current state is %s", ConnectionStateToStr(currentState));
   }

   /*
    * Track state changes to the connection
    */
   switch (currentState)
   {
   case VDP_SERVICE_CONN_CONNECTED:
      /*
       * When a connection is made, the channel must be opened
       */
      rpcPlugin->ChannelConnect();
      break;

   case VDP_SERVICE_CONN_DISCONNECTED:
      /*
       * When the connection is lost, the channel must be closed
       */
      rpcPlugin->ChannelDisconnect();
      break;

   default:
      break;
   }
}


/*
 *----------------------------------------------------------------------
 *
 * RPCManager::OnChannelStateChanged --
 *
 *    Callback function for state change in the virtual channel
 *    that we are using.
 *
 * Results:
 *    None.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

void
RPCManager::OnChannelStateChanged(void *userData,                         // IN
                                  VDPService_ChannelState currentState,   // IN
                                  VDPService_ChannelState transientState, // IN
                                  void *reserved)                         // IN
{
   RPCPluginInstance* rpcPlugin = static_cast<RPCPluginInstance*>(userData);
   RPCManager* rpcManager = rpcPlugin->GetRPCManager();

   /*
    * Log the state
    *
    *    The "transientState" is the state that caused the event to be sent.
    *    The "currentState" is just that, the current state of the channel.
    *    They can be different due to delays in processing the callback.
    */
   FUNCTION_TRACE_MSG("Channel is now %s", ChannelStateToStr(transientState));
   if (transientState != currentState) {
      LOG("   but the current state is %s", ChannelStateToStr(currentState));
   }

   /*
    * Track state changes to the channel
    */
   switch (transientState)
   {
   case VDP_SERVICE_CHAN_CONNECTED:
      rpcPlugin->OnChannelConnected();
      break;

   case VDP_SERVICE_CHAN_DISCONNECTED:
      rpcPlugin->OnChannelDisconnected();
      break;

   default:
      break;
   }

   /*
    * The server needs to create the channel object when the state
    * changes to CONNECTED but it should only do so when the
    * transient and the current state are both CONNECTED.
    *
    * If the current state isn't CONNECTED then attempting to
    * create the channel object will fail.  If the transient
    * state isn't CONNECTED then another callback is going to
    * come in real soon where both will be CONNECTED, it's best
    * to wait for that callback so that I don't try to create
    * the channel object twice.
    */
   if (rpcManager->IsServer()) {
      if (currentState == VDP_SERVICE_CHAN_CONNECTED &&
          transientState == VDP_SERVICE_CHAN_CONNECTED) {
         rpcPlugin->ChannelObjCreate();
      }
   }
}


/*
 *----------------------------------------------------------------------
 *
 * RPCManager::OnPeerChannelObjectCreated --
 *
 *    Callback function for the peer creating an object.
 *
 *    Currently, the RPCManager only supports one object name. Also
 *    assumes that object creation is always initiated by the server/agent,
 *    which will be true if the RPCManager is used on both ends.
 *
 * Results:
 *    None.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

void
RPCManager::OnPeerChannelObjectCreated(void *userData,       // IN
                                       const char *objName,  // IN
                                       void *reserved)       // IN
{
   RPCPluginInstance* rpcPlugin = static_cast<RPCPluginInstance*>(userData);
   RPCManager *rpcManager = rpcPlugin->GetRPCManager();
   FUNCTION_TRACE_MSG("Peer channel object \"%s\" created", objName);

   /*
    * The client creates the channel object in response
    * to the server creating its channel object.
    */
   if (rpcManager->IsClient() &&
       strcmp(objName, rpcManager->m_channelObjName) == 0) {
      rpcPlugin->ChannelObjCreate();
   }
}


/*
 *----------------------------------------------------------------------
 *
 * RPCManager::OnChannelObjectStateChanged --
 *
 *    Callback function for state change of the channel object.
 *
 * Results:
 *    None.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

void
RPCManager::OnChannelObjectStateChanged(void *userData,   // IN
                                        void *reserved)   // IN
{
   RPCPluginInstance* rpcPlugin = static_cast<RPCPluginInstance*>(userData);
   RPCManager *rpcManager = rpcPlugin->GetRPCManager();

   VDPRPC_ObjectState objectState =
      rpcManager->m_iChannelObj.v1.GetObjectState(rpcPlugin->m_hChannelObj);

   /*
    * Log the current state
    */
   FUNCTION_TRACE_MSG("Channel object \"%s\" is now %s",
      rpcManager->m_channelObjName, ChannelObjectStateToStr(objectState));

   /*
    * Track the state changes to the channel object
    */
   switch (objectState)
   {
   case VDP_RPC_OBJ_CONNECTED:
      rpcPlugin->OnChannelObjConnected();
      break;

   case VDP_RPC_OBJ_DISCONNECTED:
      rpcPlugin->OnChannelObjDisconnected();
      break;

   case VDP_RPC_OBJ_SIDE_CHANNEL_CONNECTED:
      if (rpcPlugin->m_sideChannelPending) {
         rpcPlugin->m_sideChannelPending = false;
         rpcPlugin->OnSidechannelConnected();
      } else {
         LOG("Side channel was not pending when connected was received.\n");
      }
      break;

   case VDP_RPC_OBJ_SIDE_CHANNEL_PENDING:
      rpcPlugin->m_sideChannelPending = true;
      break;

   default:
      break;
   }
}


/*
 *----------------------------------------------------------------------
 *
 * RPCManager::OnMsgDone --
 *
 *    Callback to indicate that a message has successfully been sent.
 *
 * Results:
 *    None.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

void
RPCManager::OnMsgDone(void* userData,        // IN
                      uint32 requestCtxId,   // IN
                      void* returnCtx)       // IN
{
   char cmd[32];
   RPCPluginInstance* rpcPlugin = static_cast<RPCPluginInstance*>(userData);
   const VDPRPC_ChannelContextInterface* iChannelCtx;

   /*
    * skip OnDone for channelType request.
    */
   iChannelCtx = rpcPlugin->ChannelContextInterface();
   iChannelCtx->v1.GetNamedCommand(returnCtx, cmd, sizeof cmd);
   if (strcmp(cmd, VDP_PING_CHANNEL) != 0) {
      rpcPlugin->TrackPendingMessages(false, NULL, 0);
      rpcPlugin->OnDone(requestCtxId, returnCtx);
   }
}


/*
 *----------------------------------------------------------------------
 *
 * RPCManager::OnMsgAbort --
 *
 *    Callback to indicate that the given message failed to be sent.
 *
 * Results:
 *    None.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

void
RPCManager::OnMsgAbort(void* userData,       // IN
                       uint32 requestCtxId,  // IN
                       Bool userCancelled,   // IN
                       uint32 reason)        // IN
{
   RPCPluginInstance* rpcPlugin = static_cast<RPCPluginInstance*>(userData);
   rpcPlugin->TrackPendingMessages(false, NULL, 0);
   rpcPlugin->OnAbort(requestCtxId, userCancelled, reason);
}


/*
 *----------------------------------------------------------------------
 *
 * RPCManager::OnMsgInvoke --
 *
 *    Callback to indicate that the peer has sent a message.
 *
 * Results:
 *    None.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

void
RPCManager::OnMsgInvoke(void* userData,   // IN
                        void* messageCtx, // IN
                        void* reserved)   // IN
{
   RPCPluginInstance* rpcPlugin = static_cast<RPCPluginInstance*>(userData);
   RPCManager *rpcManager = rpcPlugin->GetRPCManager();

   /* Has to receive channel type message first. */
   if (rpcManager->IsClient() && !rpcPlugin->m_isReady) {
      char cmd[32];
      const VDPRPC_ChannelContextInterface* iChannelCtx;
      iChannelCtx = rpcPlugin->ChannelContextInterface();
      iChannelCtx->v1.GetNamedCommand(messageCtx, cmd, sizeof cmd);

      if (strcmp(cmd, VDP_PING_CHANNEL) == 0) {
         rpcPlugin->OnChannelTypeInvoke(messageCtx);
      } else {
         /* For backward compatible between CDK 4.2 and 6.2.3 Agent*/
         LOG("Receive unexpect cmd[%s], could be old Agent.\n", cmd);

         rpcPlugin->m_isReady = true;
         rpcPlugin->OnInvoke(messageCtx);
      }
   } else {
      rpcPlugin->OnInvoke(messageCtx);
   }
}


/*
 *----------------------------------------------------------------------
 *
 * RPCManager::Poll --
 *
 *    Because RPC takes a single-threaded approach it needs to be given
 *    timeslices to do it's work when an application is doing work.
 *    This method gives RPC some time to do its work.  The timeout
 *    value should be considered a minimum as RPC will take as long
 *    as it needs to process all pending messages.
 *
 * Results:
 *    None.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

void
RPCManager::Poll(uint32 msTimeout)  // IN
{
   WaitForEvent(NULL, msTimeout);
}


/*
 *----------------------------------------------------------------------
 *
 * RPCManager::WaitForEvent --
 *
 *    This method waits for the given event to become signaled.  While
 *    waiting it ensures that RPC is being given timeslices so that it
 *    can do its work.
 *
 * Results:
 *    true if the event pointed to by hEvent was signaled.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

bool
RPCManager::WaitForEvent(HANDLE hEvent,      // IN
                         uint32 msTimeout)   // IN
{
   return RMWaitForEvent(hEvent, msTimeout);
}


/*
 *----------------------------------------------------------------------
 *
 * RPCManager::ConnectionStateToStr --
 *
 *    Converts the VDPService_ConnectionState into a user readable
 *    string.
 *
 * Results:
 *    User readable string representation of connState.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

const char*
RPCManager::ConnectionStateToStr(VDPService_ConnectionState connState) //IN
{
   switch(connState)
   {
   CASE_RETURN(VDP_SERVICE_CONN_, UNINITIALIZED);
   CASE_RETURN(VDP_SERVICE_CONN_, DISCONNECTED);
   CASE_RETURN(VDP_SERVICE_CONN_, PENDING);
   CASE_RETURN(VDP_SERVICE_CONN_, CONNECTED);
   }

   static char str[32];
   _snprintf_s(str, ARRAYSIZE(str), _TRUNCATE, "unknown%d", connState);
   return str;
}


/*
 *----------------------------------------------------------------------
 *
 * RPCManager::ChannelStateToStr --
 *
 *    Converts the VDPService_ChannelState into a user readable
 *    string.
 *
 * Results:
 *    User readable string representation of chanState.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

const char*
RPCManager::ChannelStateToStr(VDPService_ChannelState chanState) //IN
{
   switch(chanState)
   {
   CASE_RETURN(VDP_SERVICE_CHAN_, UNINITIALIZED);
   CASE_RETURN(VDP_SERVICE_CHAN_, DISCONNECTED);
   CASE_RETURN(VDP_SERVICE_CHAN_, PENDING);
   CASE_RETURN(VDP_SERVICE_CHAN_, CONNECTED);
   }

   static char str[32];
   _snprintf_s(str, ARRAYSIZE(str), _TRUNCATE, "unknown%d", chanState);
   return str;
}


/*
 *----------------------------------------------------------------------
 *
 * RPCManager::ChannelObjectStateToStr --
 *
 *    Converts the VDPRPC_ObjectState into a user readable
 *    string.
 *
 * Results:
 *    User readable string representation of objState.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

const char*
RPCManager::ChannelObjectStateToStr(VDPRPC_ObjectState objState) //IN
{
   switch(objState)
   {
   CASE_RETURN(VDP_RPC_OBJ_, UNINITIALIZED);
   CASE_RETURN(VDP_RPC_OBJ_, DISCONNECTED);
   CASE_RETURN(VDP_RPC_OBJ_, PENDING);
   CASE_RETURN(VDP_RPC_OBJ_, CONNECTED);
   CASE_RETURN(VDP_RPC_OBJ_, SIDE_CHANNEL_PENDING);
   CASE_RETURN(VDP_RPC_OBJ_, SIDE_CHANNEL_CONNECTED);
   default:
      break;
   }

   static char str[32];
   _snprintf_s(str, ARRAYSIZE(str), _TRUNCATE, "unknown%d", objState);
   return str;
}


/*
 *----------------------------------------------------------------------
 *
 * Class RPCPluginInstance
 *
 *----------------------------------------------------------------------
 */
RPCPluginInstance::RPCPluginInstance(RPCManager *rpcManagerPtr)
   : m_rpcManager(rpcManagerPtr),
     m_hChannel(NULL),
     m_hChannelObj(NULL),
     m_hChannelSink(VDP_SERVICE_INVALID_SINK_HANDLE),
     m_connectRequested(false),
     m_connected(false),
     m_sideChannelPending(false),
     m_isReady(false),
     m_pendingMsgCount(0),
     m_socketHandle((int)INVALID_SOCKET),
     m_channelObjOptions(0)
{
   InitializeEventsAndMutexes();

   LOG("RPCPluginInstance 0x%x created", this);
}

RPCPluginInstance::~RPCPluginInstance()
{
   CloseEventsAndMutexes();

   LOG("RPCPluginInstance 0x%x destroyed", this);
}

/*
 *----------------------------------------------------------------------
 *
 * RPCPluginInstance::ChannelObjectInterface --
 *
 *    Returns the channel Object interface.
 *
 * Results:
 *    Pointer to VDPRPC_ChannelObjectInterface.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

const VDPRPC_ChannelObjectInterface*
RPCPluginInstance::ChannelObjectInterface()
{
   RPCManager* rpcManager = GetRPCManager();
   return &rpcManager->m_iChannelObj;
}


/*
 *----------------------------------------------------------------------
 *
 * RPCPluginInstance::ChannelContextInterface --
 *
 *    Returns the channel context interface.
 *
 * Results:
 *    Pointer to VDPRPC_ChannelContextInterface.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

const VDPRPC_ChannelContextInterface*
RPCPluginInstance::ChannelContextInterface()
{
   RPCManager* rpcManager = GetRPCManager();
   return &rpcManager->m_iChannelCtx;
}


/*
 *----------------------------------------------------------------------
 *
 * RPCPluginInstance::VariantInterface --
 *
 *    Returns the variant interface.
 *
 * Results:
 *    Pointer to VDPRPC_VariantInterface.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

const VDPRPC_VariantInterface*
RPCPluginInstance::VariantInterface()
{
   RPCManager* rpcManager = GetRPCManager();
   return &rpcManager->m_iVariant;
}


/*
 *----------------------------------------------------------------------
 *
 * VDPRPC_StreamDataInterface* StreamDataInterface --
 *
 *    Returns the StreamData interface.
 *
 * Results:
 *    Pointer to VDPRPC_StreamDataInterface.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

const VDPRPC_StreamDataInterface*
RPCPluginInstance::StreamDataInterface()
{
   RPCManager* rpcManager = GetRPCManager();

   return &rpcManager->m_iStreamData;
}


/*
 *----------------------------------------------------------------------
 *
 * VDPService_ObserverInterface* VdpObserverInterface --
 *
 *    Returns the vdp observer interface.
 *
 * Results:
 *    Pointer to VDPService_ObserverInterface.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

const VDPService_ObserverInterface*
RPCPluginInstance::VdpObserverInterface()
{
   RPCManager* rpcManager = GetRPCManager();

   return &rpcManager->m_iVdpObserverInterface;
}


/*
 *----------------------------------------------------------------------
 *
 * RPCPluginInstance::OverlayGuestInterface --
 *
 *    Returns the overlay guest interface.
 *
 * Results:
 *    Pointer to VDPOverlayGuest_Interface.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

const VDPOverlayGuest_Interface*
RPCPluginInstance::OverlayGuestInterface()
{
   RPCManager *rpcManager = GetRPCManager();
   if (!rpcManager->AllowRunAsServer()) {
      LOG("Cannot use overlay guest interface due to server "
          "not being supported.");
      return NULL;
   }
   return &rpcManager->m_iOverlayGuest;
}


/*
 *----------------------------------------------------------------------
 *
 * RPCPluginInstance::OverlayClientInterface --
 *
 *    Returns the overlay client interface.
 *
 * Results:
 *    Pointer to VDPOverlayClient_Interface.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

const VDPOverlayClient_Interface*
RPCPluginInstance::OverlayClientInterface()
{
   RPCManager* rpcManager = GetRPCManager();
   return &rpcManager->m_iOverlayClient;
}


/*
 *----------------------------------------------------------------------
 *
 * RPCPluginInstance::GetChannelType --
 *
 *    Returns ping channel type.
 *
 * Results:
 *    type of channel.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

const VdpServiceChannelType
RPCPluginInstance::GetChannelType()
{
   RPCManager* rpcManager = GetRPCManager();
   return rpcManager->m_channelType;
}


/*
 *----------------------------------------------------------------------
 *
 * RPCPluginInstance::WaitUntilReady --
 *
 *    Messages can only be sent to the peer when the RPCManager
 *    is ready.  This method will block until either the timeout
 *    occurs or the ready event is signaled.
 *
 * Results:
 *    true if the ready event was signaled.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

bool
RPCPluginInstance::WaitUntilReady(uint32 msTimeout) // IN
{
   RPCManager* rpcManager = GetRPCManager();
   FUNCTION_TRACE_MSG("timeout %d", msTimeout);

   bool ok = rpcManager->WaitForEvent(m_hReadyEvent, msTimeout);

   FUNCTION_EXIT_MSG(ok ? "Ready" : "Not ready");
   return ok;
}


/*
 *----------------------------------------------------------------------
 *
 * RPCPluginInstance::RegisterChannelSink --
 *
 *    Registers the RPCManager channel sink callbacks for the
 *    given channel.
 *
 * Results:
 *    true if the callbacks were successfully registered.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

bool
RPCPluginInstance::RegisterChannelSink(void* hChannel) // IN
{
   RPCManager* rpcManager = GetRPCManager();
   FUNCTION_TRACE;

   /*
    * Make sure that were aren't already registered
    */
   if (m_hChannelSink != (uint32)VDP_SERVICE_INVALID_SINK_HANDLE) {
      FUNCTION_EXIT_MSG("Channel sink already registered");
      return true;
   }

   if (!rpcManager->m_iChannel.v1.
         RegisterChannelNotifySink(&rpcManager->m_channelSink,
                                   (void*)this,
                                   &m_hChannelSink)) {
      FUNCTION_EXIT_MSG("Channel.v1.RegisterChannelNotifySink() failed");
      return false;
   }

   m_hChannel = hChannel;
   LOG("Channel.v1.RegisterChannelNotifySink() [OK]");


   /*
    * I'll simulate a connect callback if we are already connected
    */
   if (rpcManager->m_iChannel.v1.
         GetConnectionState() == VDP_SERVICE_CONN_CONNECTED) {
      VDPService_ConnectionState cs = VDP_SERVICE_CONN_CONNECTED;
      LOG("Simulating connect callback");
      RPCManager::OnConnectionStateChanged((void*)this, cs, cs, NULL);
   }

   return true;
}


/*
 *----------------------------------------------------------------------
 *
 * RPCPluginInstance::UnregisterChannelSink --
 *
 *    Unregisters the RPCManager channel sink callbacks for the
 *    given channel.
 *
 * Results:
 *    true if the callbacks were successfully unregistered.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

bool
RPCPluginInstance::UnregisterChannelSink()
{
   RPCManager* rpcManager = GetRPCManager();
   bool ok = true;
   FUNCTION_TRACE;

   /*
    * Make sure that were are registered
    */
   if (m_hChannelSink == (uint32)VDP_SERVICE_INVALID_SINK_HANDLE) {
      FUNCTION_EXIT_MSG("Channel sink not registered");
      return true;
   }

   /*
    * I'll simulate a disconnect callback if we are still connected
    */
   if (rpcManager->m_iChannel.v1.
         GetConnectionState() == VDP_SERVICE_CONN_CONNECTED) {
      VDPService_ConnectionState cs = VDP_SERVICE_CONN_DISCONNECTED;
      LOG("Simulating disconnect callback");
      RPCManager::OnConnectionStateChanged((void*)this, cs, cs, NULL);
   }

   if (!rpcManager->m_iChannel.v1.
         UnregisterChannelNotifySink(m_hChannelSink)) {
      FUNCTION_EXIT_MSG("Channel.v1.UnregisterChannelNotifySink() failed");
      ok = false;
   } else {
      FUNCTION_EXIT_MSG("Channel.v1.UnregisterChannelNotifySink() [OK]");
   }

   m_hChannel = NULL;
   m_hChannelSink = VDP_SERVICE_INVALID_SINK_HANDLE;
   return ok;
}


/*
 *----------------------------------------------------------------------
 *
 * RPCPluginInstance::ChannelConnect --
 *
 *    Connects the underlying channel for this application/plugin pair.
 *
 * Results:
 *    true if the connect call succeeded.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

bool
RPCPluginInstance::ChannelConnect()
{
   RPCManager* rpcManager = GetRPCManager();
   FUNCTION_TRACE;

   if (!rpcManager->m_iChannel.v1.Connect()) {
      FUNCTION_EXIT_MSG("Channel.v1.Connect() failed");
      return false;
   }

   FUNCTION_EXIT_MSG("Channel.v1.Connect() [OK]");
   m_connectRequested = true;
   return true;
}


/*
 *----------------------------------------------------------------------
 *
 * RPCPluginInstance::ChannelDisconnect --
 *
 *    Disconnects the underlying channel for this application/plugin pair.
 *
 * Results:
 *    true if the disconnect call succeeded.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

bool
RPCPluginInstance::ChannelDisconnect()
{
   RPCManager* rpcManager = GetRPCManager();
   bool ok = true;
   FUNCTION_TRACE;

   if (m_connectRequested) {
      if (!rpcManager->m_iChannel.v1.Disconnect()) {
         LOG("Channel.v1.Disconnect() failed");
         ok = false;
      } else {
         LOG("Channel.v1.Disconnect() [OK]");
      }
   }

   OnChannelDisconnected();
   return ok;
}

/*
 *----------------------------------------------------------------------
 *
 * RPCPluginInstance::OnChannelTypeInvoke --
 *
 *    Callback to notify that the channel type message is received.
 *    It should only happen on client side.
 *
 * Results:
 *    None.
 *
 * Side Effects:
 *    1) The OnReady() callback will be fired and the m_hReadyEvent will
 *       be set for main channel.
 *    2) Otherwise sidechannel request will be issued.
 *
 *----------------------------------------------------------------------
 */

void
RPCPluginInstance::OnChannelTypeInvoke(void* messageCtx)             // IN
{
   RPCManager* rpcManager = GetRPCManager();
   const VDPRPC_ChannelObjectInterface *iChannelObj = ChannelObjectInterface();
   const VDPRPC_ChannelContextInterface *iChannelCtx = ChannelContextInterface();

   char cmd[32];
   iChannelCtx->v1.GetNamedCommand(messageCtx, cmd, sizeof cmd);

   if (strcmp(cmd, VDP_PING_CHANNEL) == 0) {
      RPCVariant var(this);
      if (iChannelCtx->v1.GetParam(messageCtx, 0, &var) &&
          var.vt == VDP_RPC_VT_UI4) {

         // receive channel type from agent.
         rpcManager->SetChannelType((VdpServiceChannelType) var.ulVal);
         switch (var.ulVal) {
         case VDPSERVICE_MAIN_CHANNEL:
            RMSetEvent(m_hReadyEvent);
            m_isReady = true;
            OnReady();
            break;
         case VDPSERVICE_VCHAN_CHANNEL:
            iChannelObj->v2.RequestSideChannel(m_hChannelObj,
                                               VDP_RPC_SIDE_CHANNEL_TYPE_PCOIP,
                                               rpcManager->m_tokenName);
            break;
         case VDPSERVICE_TCP_CHANNEL:
         case VDPSERVICE_TCPRAW_CHANNEL:
            iChannelObj->v2.RequestSideChannel(m_hChannelObj,
                                               VDP_RPC_SIDE_CHANNEL_TYPE_TCP,
                                               rpcManager->m_tokenName);
            break;
         default:
            LOG("Error: invalid channel type [%d].", var.ulVal);
            break;
         }
      } else {
         LOG("Error: GetParam failed [%d].", var.vt);
      }
   } else {
      LOG("Discard command [%s] before channel type is received.", cmd);
   }
}


/*
 *----------------------------------------------------------------------
 *
 * RPCPluginInstance::OnChannelConnected --
 *
 *    Callback to notify that the channel has been connected.
 *
 * Results:
 *    None.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

void
RPCPluginInstance::OnChannelConnected()
{
   FUNCTION_TRACE;
   m_connected = true;
}


/*
 *----------------------------------------------------------------------
 *
 * RPCPluginInstance::OnChannelDisconnected --
 *
 *    Callback to notify that the channel has been disconnected.
 *
 * Results:
 *    None.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

void
RPCPluginInstance::OnChannelDisconnected()
{
   FUNCTION_TRACE;

   /*
    * Update variables that tell me the status of the channel
    */
   m_connectRequested = false;
   m_connected = false;

   /*
    * I need to destroy the channel object when the channel closes
    */
   ChannelObjDestroy();
}


/*
 *----------------------------------------------------------------------
 *
 * RPCPluginInstance::ChannelObjCreate --
 *
 *    Instantiates the channel object to be used by this
 *    application/plugin pair. Note that RPCManager only supports
 *    one channel object currently (named m_channelObjName).
 *
 * Results:
 *    true if the object was created successfully.
 *
 * Side Effects:
 *    A message is sent by VDPService to the peer to notify them that
 *    the channel object has been created.
 *
 *----------------------------------------------------------------------
 */

bool
RPCPluginInstance::ChannelObjCreate()
{
   RPCManager* rpcManager = GetRPCManager();
   FUNCTION_TRACE;

   if (m_hChannelObj != NULL) {
      FUNCTION_EXIT_MSG("Channel object \"%s\" already created", rpcManager->m_channelObjName);
      return true;
   }

   /*
    * 1) For client, both encryption and compression are enabled to perform
    *    all kind of tests.
    * 2) For agent, app pass encryption and compression options from ServerInit.
    */
   int flags = VDP_RPC_OBJ_CONFIG_INVOKE_ALLOW_ANY_THREAD;
   flags |= rpcManager->m_encryptionEnabled ? VDP_RPC_OBJ_SUPPORT_ENCRYPTION : 0;
   flags |= rpcManager->m_compressionEnabled ? VDP_RPC_OBJ_SUPPORT_COMPRESSION : 0;

   if (!rpcManager->m_iChannelObj.v1.
         CreateChannelObject(rpcManager->m_channelObjName,
                             &rpcManager->m_channelObjSink,
                             (void*)this,
                             (VDPRPC_ObjectConfigurationFlags) flags,
                             &m_hChannelObj)) {
      FUNCTION_EXIT_MSG("Failed to create channel object \"%s\"", rpcManager->m_channelObjName);
      return false;
   }

   FUNCTION_EXIT_MSG("Channel object \"%s\" created", rpcManager->m_channelObjName);
   return true;
}


/*
 *----------------------------------------------------------------------
 *
 * RPCPluginInstance::ChannelObjDestroy --
 *
 *    Destroys the channel object used by this
 *    application/plugin pair. Note that RPCManager only supports
 *    one channel object currently (named m_channelObjName).
 *
 * Results:
 *    true if the object was destroyed successfully.
 *
 * Side Effects:
 *    A message is sent by VDPService to the peer to notify them that
 *    the channel object has been destroyed. Also, the OnNotReady()
 *    callback will be fired (can be overridden by subclasses).
 *
 *----------------------------------------------------------------------
 */

bool
RPCPluginInstance::ChannelObjDestroy()
{
   RPCManager* rpcManager = GetRPCManager();
   bool ok = true;
   FUNCTION_TRACE;

   if (m_hChannelObj != NULL) {
      if (!rpcManager->m_iChannelObj.v1.DestroyChannelObject(m_hChannelObj)) {
         LOG("Failed to destroy channel object \"%s\"", rpcManager->m_channelObjName);
         ok = false;
      } else {
         LOG("Channel object \"%s\" destroyed", rpcManager->m_channelObjName);
      }

      m_hChannelObj = NULL;
   }

   if (m_isReady) {
      RMResetEvent(m_hReadyEvent);
      m_isReady = false;
      OnNotReady();
   }

   return ok;
}


/*
 *----------------------------------------------------------------------
 *
 * RPCPluginInstance::OnChannelObjConnected --
 *
 *    Callback function to notify that the peer object has been created
 *    and the connection is complete. This indicates that the object
 *    can now be used to send messages to the peer.
 *
 * Results:
 *    None.
 *
 * Side Effects:
 *    The OnReady() callback will be fired and the m_hReadyEvent will
 *    be set.
 *
 *----------------------------------------------------------------------
 */

void
RPCPluginInstance::OnChannelObjConnected()
{
   RPCManager* rpcManager = GetRPCManager();
   FUNCTION_TRACE;

   /*
    * It should be ready to run ping program now.
    *
    * But to demostrate sending data via different channels, both
    * agent app and client plugin need to setup the same type
    * sidechannel if it is necessary.
    *
    * For common practice, it could be hard coded, regKey configured
    * or from another channel.
    *
    * Here we just send a message to client via main channel before ping
    * program start.
    *
    */

   if (rpcManager->IsServer()) {
      void* messageCtx = NULL;

      if (!CreateMessage(&messageCtx)) {
         LOG("Error: cannot create channelCtx to send channel type.");
         return;
      }

      rpcManager->m_iChannelCtx.v1.SetNamedCommand(messageCtx, VDP_PING_CHANNEL);
      RPCVariant var(this);
      const VDPRPC_VariantInterface *iVariant = VariantInterface();
      const VDPRPC_ChannelContextInterface *iChannelCtx = ChannelContextInterface();

      iVariant->v1.VariantFromUInt32(&var, rpcManager->m_channelType);
      iChannelCtx->v1.AppendParam(messageCtx, &var);

      /*
       * After a successfull call to invoke, the RPC library owns
       * the message context and will destroy it.  We only need to
       * destroy it if InvokeMessage() fails.
       */
      if (!InvokeMessage(messageCtx, true)) {
         DestroyMessage(messageCtx);
         return;
      }

      if (rpcManager->m_channelType == VDPSERVICE_MAIN_CHANNEL) {
         RMSetEvent(m_hReadyEvent);
         m_isReady = true;
         OnReady();
      } else if (rpcManager->m_channelType == VDPSERVICE_VCHAN_CHANNEL) {
         rpcManager->m_iChannelObj.v2.RequestSideChannel(m_hChannelObj,
                                                         VDP_RPC_SIDE_CHANNEL_TYPE_PCOIP,
                                                         rpcManager->m_tokenName);
      } else { //request tcp sidechannel
         rpcManager->m_iChannelObj.v2.RequestSideChannel(m_hChannelObj,
                                                         VDP_RPC_SIDE_CHANNEL_TYPE_TCP,
                                                         rpcManager->m_tokenName);

      }
   } // Client need to get channel type first.

   // Query channel object options (e.g compression and encryption)
   const VDPRPC_ChannelObjectInterface* iObj = ChannelObjectInterface();
   if (iObj->v3.GetObjectOptions != NULL) {
      iObj->v3.GetObjectOptions(m_hChannelObj, &m_channelObjOptions);
   }
   LOG("Channel Object options : 0x%08x.", m_channelObjOptions);
}


/*
 *----------------------------------------------------------------------
 *
 * RPCPluginInstance::OnSidechannelConnected --
 *
 *    Callback function to notify that sidechannel is connected.
 *
 * Results:
 *    None.
 *
 * Side Effects:
 *    The OnReady() callback will be fired and the m_hReadyEvent will be set.
 *
 *----------------------------------------------------------------------
 */

void
RPCPluginInstance::OnSidechannelConnected()
{
   RMSetEvent(m_hReadyEvent);
   m_isReady = true;
   OnReady();
}


/*
 *----------------------------------------------------------------------
 *
 * RPCPluginInstance::OnChannelObjDisconnected --
 *
 *    Callback function to notify that the peer object has been destroyed.
 *    In response, we will destroy our local object.
 *
 * Results:
 *    None.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

void
RPCPluginInstance::OnChannelObjDisconnected()
{
   FUNCTION_TRACE;
   ChannelObjDestroy();
}


/*
 *----------------------------------------------------------------------
 *
 * RPCPluginInstance::CreateMessage --
 *
 *    Allocates a message context to be used to send an RPC to the peer.
 *
 * Results:
 *    true if the message context was created successfully.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

bool
RPCPluginInstance::CreateMessage(void** pMessageCtx) // OUT
{
   RPCManager* rpcManager = GetRPCManager();

   if (m_hChannelObj == NULL) {
      LOG("Failed to create message (not ready)");
      return false;
   }

   if (m_isReady) {
      /*
       * Here we choose client always perform encryption and compression if possible.
       * (because encryption and compression are always true for client). For agent, it
       * uses configuration from user. And it is up to user to use right one for their app.
       */

      uint32 comp = rpcManager->m_compressionEnabled ? VDP_RPC_COMP_SNAPPY : 0;
      uint32 enc  = rpcManager->m_encryptionEnabled ? VDP_RPC_CRYPTO_AES : 0;
      uint32 options = m_channelObjOptions & (comp | enc);

      if (comp && (m_channelObjOptions & comp) == 0) {
         LOG("Error: vdpservice object does not support compression.");
      }

      if (enc && (m_channelObjOptions & enc) == 0) {
         LOG("Error: vdpservice object does not support encryption.");
      }

      if (!rpcManager->m_iChannelObj.v3.CreateContext(m_hChannelObj, options, pMessageCtx)) {
         LOG("Failed to create message (CreateContext failed)");
         return false;
      }
   } else {
      // Let Channel type message send through.
      if (!rpcManager->m_iChannelObj.v1.CreateContext(m_hChannelObj, pMessageCtx)) {
         LOG("Failed to create message (CreateContext failed)");
         return false;
      }
   }

   return true;
}


/*
 *----------------------------------------------------------------------
 *
 * RPCPluginInstance::DestroyMessage --
 *
 *    Frees a message that was previously created. Note that this should
 *    ONLY be done if a call to Invoke failed, or the message is not
 *    going to be sent. The Invoke call will free the message if
 *    successful.
 *
 * Results:
 *    true if the message context was destroyed successfully.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

bool
RPCPluginInstance::DestroyMessage(void* messageCtx) // IN
{
   RPCManager* rpcManager = GetRPCManager();

   if (!rpcManager->m_iChannelObj.v1.DestroyContext(messageCtx)) {
      LOG("Failed to destroy message (DestroyContext failed)");
      return false;
   }

   return true;
}


/*
 *----------------------------------------------------------------------
 *
 * RPCPluginInstance::InvokeMessage --
 *
 *    Sends the given message to the peer.
 *
 * Results:
 *    true if the message context was sent successfully.
 *
 * Side Effects:
 *    If successful, the given messageCtx was destroyed.
 *
 *----------------------------------------------------------------------
 */

bool
RPCPluginInstance::InvokeMessage(void* messageCtx,     // IN
                                 bool  channelTypeMsg) // IN
{
   RPCManager* rpcManager = GetRPCManager();

   if (m_hChannelObj == NULL || !(m_isReady || channelTypeMsg)) {
      LOG("Failed to send message (not ready)");
      return false;
   }

   if (!channelTypeMsg) {
      TrackPendingMessages(true, NULL, 0);
   }

   if (!rpcManager->m_iChannelObj.v1.Invoke(m_hChannelObj,
                                            messageCtx,
                                            &rpcManager->m_requestSink,
                                            (void*)this)) {
      LOG("Failed to send message (Invoke failed)");
      return false;
   }

   return true;
}


/*
 *----------------------------------------------------------------------
 *
 * RPCPluginInstance::WaitForPendingMessages --
 *
 *    This method waits for the peer to process and respond to all
 *    messages before returning.  While waiting it ensures that RPC
 *    is being given timeslices so that it can do its work.
 *
 * Results:
 *    This method returns TRUE if all pending messages have been
 *    processed.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

bool
RPCPluginInstance::WaitForPendingMessages(uint32 msTimeout)  // IN
{
   RPCManager* rpcManager = GetRPCManager();

   /*
    * WaitForEvent calls Poll() which will ASSERT for TCPRAW channels
    */
   if (rpcManager->m_channelType == VDPSERVICE_TCPRAW_CHANNEL) {
      LOG("Channel type is TCPRAW, skipping wait");
   } else {
      rpcManager->WaitForEvent(m_pendingMsgEvent, msTimeout);
   }

   if (m_pendingMsgCount != 0) {
      const char* s = m_pendingMsgCount == 1 ? "" : "s";
      LOG("%d message%s still pending", m_pendingMsgCount, s);
      m_pendingMsgCount = 0;
      return false;
   }

   return true;
}


/*
 *----------------------------------------------------------------------
 *
 * RPCPluginInstance::TrackPendingMessages --
 *
 *    This function is called from SndMsg() with <msgSent> set to
 *    TRUE to track the number of pending messages.
 *
 *    This function is called by OnMsgDone() and OnMsgAbort() with
 *    <msgSent> set to FALSE to track the number of pending messages.
 *
 * Results:
 *    A debugging message is returned via <msg> that can be written
 *    to the logs to track the number of pending messages
 *
 * Side effects:
 *    When the number of pending messages is 0 an event is signaled.
 *
 *----------------------------------------------------------------------
 */

int32
RPCPluginInstance::TrackPendingMessages(bool msgSent,     // IN
                                        char* msg,        // OUT
                                        int32 maxMsgLen)  // IN
{
   RMLockMutex(m_pendingMsgMutex);
   int32 pendingMsgCount = m_pendingMsgCount + (msgSent ? 1 : -1);

   if (pendingMsgCount < 0) {
      pendingMsgCount = 0;

      if (msg != NULL && maxMsgLen > 0 ) {
         _snprintf_s(msg, maxMsgLen, _TRUNCATE, " (unexpected message)");
      }

   } else {
      if (msg != NULL && maxMsgLen > 0 ) {
         const char* s = pendingMsgCount == 1 ? "" : "s";
         _snprintf_s(msg, maxMsgLen, _TRUNCATE,
                     " (%d message%s pending)", pendingMsgCount, s);
      }
   }

   if (pendingMsgCount == 0) {
      RMSetEvent(m_pendingMsgEvent);
   } else {
      RMResetEvent(m_pendingMsgEvent);
   }

   m_pendingMsgCount = pendingMsgCount;
   RMUnlockMutex(m_pendingMsgMutex);

   return pendingMsgCount;
}


/*
 *----------------------------------------------------------------------
 *
 * RPCPluginInstance::GetTcpRawSocket --
 *
 *    Switch to TcpStreamData mode and return Tcp raw socket
 *
 * Note:
 *    Could not be called from vdpservice callbacks.
 *
 * Results:
 *    Tcp Raw socket handle.
 *
 * Side effects:
 *    vdpservice entering streamData mode.
 *
 *----------------------------------------------------------------------
 */

int
RPCPluginInstance::GetTcpRawSocket()
{
   RPCManager* rpcManager = GetRPCManager();
   FUNCTION_TRACE;

   if (rpcManager->IsServer() && m_socketHandle == INVALID_SOCKET &&
       rpcManager->m_channelType == VDPSERVICE_TCPRAW_CHANNEL) {
      VDPService_ChannelInterface *itf = &rpcManager->m_iChannel;
      if (itf->v2.SwitchToStreamDataMode == NULL) {
         LOG("Error: vdpservice is too old to support streamDataMode");
         return (int)INVALID_SOCKET;
      }

      if (!itf->v2.SwitchToStreamDataMode(rpcManager->m_channelObjName,
                                          this->m_hChannel,
                                          &m_socketHandle)) {
         LOG("Error: fail to switch to streamDataMode.");
         return (int)INVALID_SOCKET;
      }
   }

   return m_socketHandle;
}
