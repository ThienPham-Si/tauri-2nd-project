/* ********************************************************************************* *
 * Copyright (C) 2011-2021 VMware, Inc.  All rights reserved. -- VMware Confidential *
 * ********************************************************************************* */

/*
 * RPCManager.h --
 *
 */

#pragma once

#include "vdpService.h"
#include "vdpService_guids.h"
#include "vdpService_interfaces.h"
#include "vdprpc_interfaces.h"
#include "vdpOverlay.h"
#include "helpers.h"

// command to send the channel type of ping program.
#define VDP_PING_CHANNEL       "VdpPingChannel"

// command for TCP ECHO.
#define VDP_PING_CMD           1
#define VDP_PING_ECHO          2
#define VDP_TCP_ECHO           "VdpTcpEcho"

// For current windows session
#define VDP_CURRENT_SESSION    -1

/* The different channels to send ping packet  */
typedef enum {
   VDPSERVICE_MAIN_CHANNEL    = 0x1,   /* vdpservice main channel */
   VDPSERVICE_VCHAN_CHANNEL   = 0x2,   /* vdpservice virtual side channel */
   VDPSERVICE_TCP_CHANNEL     = 0x3,   /* vdpservice tcp side channel */
   VDPSERVICE_TCPRAW_CHANNEL  = 0x4    /* vdpservice tcp side channel
                                          streamData mode(raw tcp socket) */
} VdpServiceChannelType;


/*
 * Explicitly mark exported methods as visible for linux
 */
#ifdef _WIN32
#define EXPORTFN
#else
#define EXPORTFN __attribute__((visibility("default")))
#endif

class RPCManager;

/*
 *----------------------------------------------------------------------
 *
 * Class RPCPluginInstance
 *
 *    This class tracks an instance of an RPC plugin.  On the server
 *    there is only a single instance per process.  On the client
 *    there can be multiple instances, each instance talks to a
 *    different process on the server.
 *
 *----------------------------------------------------------------------
 */
class RPCPluginInstance
{
public:
   RPCPluginInstance(RPCManager* rpcManagerPtr);
   virtual ~RPCPluginInstance();

   /*
    * Getter for the RPCManager object that
    * belongs to this plugin instance.
    */
   RPCManager* GetRPCManager() { return m_rpcManager; }

   /*
    * Messages can only be sent to the peer when the RPCManager
    * is ready.  These functions can be used to determine the
    * "ready" status.  As an alternative the virtual methods
    * OnReady() and OnNotReady() can be overridden to track the
    * ready status of the RPCManager.
    */
   bool WaitUntilReady(uint32 msTimeout);
   bool IsReady() { return m_isReady; }

   /*
    * This method waits for the peer to process and respond to all
    * messages before returning.  While waiting it ensures that RPC
    * is being given timeslices so that it can do its work.  This
    * method returns TRUE if all pending messages have been processed.
    */
   bool WaitForPendingMessages(uint32 msTimeout);

   const VdpServiceChannelType GetChannelType();

   /* make it public for RPCVariant */
   const VDPRPC_VariantInterface* VariantInterface();

protected:
   /*
    * These methods are used to create and then invoke (i.e. send)
    * messages to the peer.  After a successfull call to InvokeMessage()
    * the RPC library owns the message context and will delete it.
    */
   bool CreateMessage(void** pMessageCtx);
   bool DestroyMessage(void* messageCtx);
   bool InvokeMessage(void* messageCtx, bool channelTypeMessage=false);

   /*
    * ChannelContextInterface() and VariantInterface() can be used
    * to get incoming parameters and set outgoing parameters.
    * ChannelObjectInterface() can be used to request for sidechannel.
    *
    * See the RPC documentation for more information.
    */
   const VDPRPC_ChannelContextInterface* ChannelContextInterface();
   const VDPRPC_ChannelObjectInterface* ChannelObjectInterface();
   const VDPRPC_StreamDataInterface* StreamDataInterface();
   const VDPService_ObserverInterface* VdpObserverInterface();

   /*
    * In case you want to use the other VDP services
    */
   const VDPOverlayGuest_Interface* OverlayGuestInterface();
   const VDPOverlayClient_Interface* OverlayClientInterface();

private:
   /*
    * These two methods can be overridden to be notified
    * when the RPCManager is ready to send messages.  As an
    * alternative the methods IsReady() and WaitUntilReady()
    * can also be used to determine the "ready" status.
    */
   virtual void OnReady() { }
   virtual void OnNotReady() { }

   /*
    * These two methods can be overridden to track the status
    * of messages that you have sent to the peer.  OnDone()
    * is called when the peer has responded to your message.
    * OnAbort() is called if there was an error sending your
    * message.
    */
   virtual void OnDone(uint32 requestCtxId, void *returnCtx) { }
   virtual void OnAbort(uint32 requestCtxId, Bool userCancelled, uint32 reason) { }

   /*
    * This method can be overriden to be notified when the peer
    * has sent you a message.  ChannelContextInterface and
    * VariantInterface can be used to get incoming parameters
    * and set outgoing return values.
    */
   virtual void OnInvoke(void* messageCtx) { }

   /*
    * Receive channel type message.
    */
   void OnChannelTypeInvoke(void* messageCtx);

public:
   /*
    * I'd like this set of methods, from here to the 'private' tag,
    * to be private but they are called by the global VDPService_*
    * functions that are used by the client so I have to make them public.
    */
   bool RegisterChannelSink(void* hChannel);
   bool UnregisterChannelSink();

   bool ChannelConnect();
   bool ChannelDisconnect();
   int  GetTcpRawSocket();
   uint32 GetChannelObjOptions() const { return m_channelObjOptions; }

private:
   RPCManager*       m_rpcManager;
   void*             m_hChannel;
   void*             m_hChannelObj;
   uint32            m_hChannelSink;

   bool              m_connectRequested;
   bool              m_connected;
   bool              m_sideChannelPending;

   bool              m_isReady;
   HANDLE            m_hReadyEvent;

   HANDLE            m_pendingMsgMutex;
   HANDLE            m_pendingMsgEvent;
   int32             m_pendingMsgCount;
   int               m_socketHandle;

   uint32            m_channelObjOptions;
   int32 TrackPendingMessages(bool msgSent, char* msg, int32 maxMsgLen);

   void OnChannelConnected();
   void OnChannelDisconnected();

   bool ChannelObjCreate();
   bool ChannelObjDestroy();

   void OnChannelObjConnected();
   void OnChannelObjDisconnected();
   void OnSidechannelConnected();

   friend class RPCManager;

   /*
    * Implementation/platform specific implementations
    * RM == RPCManager
    */
   void InitializeEventsAndMutexes();
   void CloseEventsAndMutexes();
   void RMLockMutex(HANDLE hMutex);
   void RMUnlockMutex(HANDLE hMutex);
   void RMSetEvent(HANDLE hEvent);
   void RMResetEvent(HANDLE hEvent);
};


/*
 *----------------------------------------------------------------------
 *
 * Class RPCManager
 *
 *    This class does all the bookkeeping for sending and receiving RPC
 *    messages.  There can only be one instance of this class per process.
 *    It handles the desktop connection and channel connection callbacks.
 *    It supports a single channel object to send and receive messages.
 *
 *----------------------------------------------------------------------
 */
class RPCManager
{
public:
   RPCManager(const char *tokenName);
   virtual ~RPCManager();

   /*
    * Returns the name of the token that was used when creating
    * the RPCManager.  The VDPService library uses the token
    * name to match a server side application with its client
    * side plugin.
    */
   const char* TokenName() { return m_tokenName; }

   /*
    * These functions will be invoked by each of the plugin's
    * globally exposed vdpservice APIs
    */
   virtual bool VDPPluginInit(const VDP_SERVICE_QUERY_INTERFACE qi);
   virtual bool VDPPluginExit(void);
   virtual bool VDPPluginCreateInstance(void* hChannel, void** pUserData);
   virtual bool VDPPluginDestroyInstance(void* userData);

   /*
    * Determines if the RPCManager was initialized in server mode
    * or client mode.
    */
   bool IsServer() { return m_isServer; }
   bool IsClient() { return !m_isServer; }

   /*
    * Initializes the RPCManager in server mode.  You must pass in the
    * single instance of a RPCPluginInstance object which you have likely
    * subclassed.
    */
   bool ServerInit(RPCPluginInstance* rpcPlugin, uint32 msTimeoutReady);
   bool ServerInit2(DWORD sid, VdpServiceChannelType type,
                    bool compressionEnabled, bool encryptionEnabled,
                    RPCPluginInstance* rpcPlugin, uint32 msTimeoutReady);
   bool ServerExit(RPCPluginInstance* rpcPlugin);
   bool ServerExit2(DWORD sid, RPCPluginInstance* rpcPlugin);

   virtual void OnServerInit() { }
   virtual void OnServerExit() { }

   /*
    * Because RPC takes a single-threaded approach it needs to be given
    * timeslices to do it's work when an application is doing work.
    * This method gives RPC some time to do its work.  The timeout
    * value should be considered a minimum as RPC will take as long
    * as it needs to process all pending messages.
    */
   void Poll(uint32 msTimeout=0);

   /*
    * This method waits for the given event to become signaled.  While
    * waiting it ensures that RPC is being given timeslices so that it
    * can do its work.  This method returns TRUE if the event is signaled.
    */
   bool WaitForEvent(HANDLE hEvent, uint32 msTimeout);

   /*
    * In case you need other vdpService interfaces (e.g. Overlay)
    */
   bool QueryInterface(const GUID *iid, void *iface);

   /*
    * OnCreateInstance() can be overridden to return a subclass of RPCPluginInstance
    * so that you can track state for each instance of the RPC plugin.  You will also
    * need to subclass RPCPluginInstance so that you can override the methods that
    * inform you when message are sent by the peer.
    *
    * OnDestroyInstance() can be overridden to track when the RPCPluginInstance
    * needs to be destroyed.  Overriding this method is optional as you could
    * also do the work in the destructor of your subclass.
    *
    * VDPService_PluginCreate/DestroyInstance() will call these methods at the
    * appropriate time.  These methods should not be called by anyone else.
    */
   virtual RPCPluginInstance* OnCreateInstance() { return new RPCPluginInstance(this); }
   virtual void OnDestroyInstance(RPCPluginInstance* pluginInstance) { delete pluginInstance; }

   /*
    * VDPService_PluginInit/Exit() call these functions to do the bookkeeping
    * related to initializing and tearing down the client interfaces.  These
    * methods should not be called by anyone else.
    */
   bool ClientInit(const VDP_SERVICE_QUERY_INTERFACE* qi);
   bool ClientExit();

   /*
    * These two methods can be overrideen to be notified
    * when the plugin has been loaded.
    */
   virtual void OnClientInit() { }
   virtual void OnClientExit() { }

   void SetChannelType(VdpServiceChannelType t) { m_channelType = t; }

private:
   static RPCManager*               s_instance;

   bool                             m_isServer;
   bool                             m_serverInit;
   bool                             m_initialized;
   VdpServiceChannelType            m_channelType;
   bool                             m_compressionEnabled;
   bool                             m_encryptionEnabled;


   char                             m_tokenName[60];
   char                             m_channelObjName[64];

   VDP_SERVICE_QUERY_INTERFACE      m_qi;
   VDPService_ChannelInterface      m_iChannel;
   VDPRPC_ChannelObjectInterface    m_iChannelObj;
   VDPRPC_ChannelContextInterface   m_iChannelCtx;
   VDPRPC_VariantInterface          m_iVariant;

   VDPService_ChannelNotifySink     m_channelSink;
   VDPRPC_ObjectNotifySink          m_channelObjSink;
   VDPRPC_RequestCallback           m_requestSink;

   VDPOverlayGuest_Interface        m_iOverlayGuest;
   VDPOverlayClient_Interface       m_iOverlayClient;

   VDPRPC_StreamDataInterface       m_iStreamData;
   VDPService_ObserverInterface     m_iVdpObserverInterface;

   bool Init(bool isServer, const VDP_SERVICE_QUERY_INTERFACE* qi);

   static void __cdecl OnConnectionStateChanged(
                  void *userData,
                  VDPService_ConnectionState currentState,
                  VDPService_ConnectionState transientState,
                  void *reserved);

   static void __cdecl OnChannelStateChanged(
                  void *userData,
                  VDPService_ChannelState currentState,
                  VDPService_ChannelState transientState,
                  void *reserved);

   static void __cdecl OnPeerChannelObjectCreated(
                  void *userData,
                  const char *objName,
                  void *reserved);

   static void __cdecl OnChannelObjectStateChanged(
                  void *userData,
                  void *reserved);

   static void __cdecl OnMsgDone(
                  void* userData,
                  uint32 requestCtxId,
                  void *returnCtx);

   static void __cdecl OnMsgAbort(
                  void* userData,
                  uint32 requestCtxId,
                  Bool userCancelled,
                  uint32 reason);

   static void __cdecl OnMsgInvoke(
                  void* userData,
                  void* messageCtx,
                  void* reserved);

   static const char*
   ConnectionStateToStr(VDPService_ConnectionState connState);

   static const char*
   ChannelStateToStr(VDPService_ChannelState chanState);

   static const char*
   ChannelObjectStateToStr(VDPRPC_ObjectState objState);

   friend class RPCPluginInstance;

   /*
    * Implementation/platform specific implementations
    * RM == RPCManager
    */
   virtual bool AllowRunAsServer();
   bool RMWaitForEvent(HANDLE hEvent, uint32 msTimeout);
};

/*
 *----------------------------------------------------------------------
 *
 * The VDP_RPC_VARIANT structure must be initialized before use and
 * cleared after.  Failure to do the first could result in memory
 * heap corruption while failure to do the second causes memory
 * leaks.  Use of this object will avoid both problems.
 *
 *----------------------------------------------------------------------
 */

class RPCVariant : public VDP_RPC_VARIANT
{
private:
   RPCPluginInstance*      m_plugin;

public:
   RPCVariant(RPCPluginInstance* plugin)     // IN
   {
      m_plugin = plugin;
      VM_ASSERT(m_plugin);
      const VDPRPC_VariantInterface* pVariant = m_plugin->VariantInterface();
      pVariant->v1.VariantInit(this);
   }

   ~RPCVariant()
   {
      VM_ASSERT(m_plugin);
      const VDPRPC_VariantInterface* pVariant = m_plugin->VariantInterface();

      /*
       * Although only str abd blob variant need to cleanup,
       * it is always a good practice to call VariantClear.
       */
      pVariant->v1.VariantClear(this);
   }
};



/*
 * MACRO DEFINITIONS
 */
#ifdef __cplusplus
#define EXTERN_C_OPEN extern "C" {
#define EXTERN_C_CLOSE }
#else
#define EXTERN_C_OPEN
#define EXTERN_C_CLOSE
#endif


#define VDP_SERVICE_CREATE_INTERFACE(token, rpcManagerObj) VDP_SERVICE_CREATE_INTERFACE_IMPL(token, rpcManagerObj)

#define GETRPCMANAGER(castType, plugin) reinterpret_cast<castType *>(plugin->GetRPCManager());

/*
 *----------------------------------------------------------------------
 *
 * VDPService_PluginGetTokenName --
 *
 *    This function is called by the VDP Service DLL to determine the
 *    token used by this DLL. The token is used to match the server apps
 *    with client plugins.
 *
 *----------------------------------------------------------------------
 */
/*
 *----------------------------------------------------------------------
 *
 * VDPService_PluginInit --
 *
 *    This function is called by the VDP Service DLL right after
 *    it loaded this DLL.
 *
 *----------------------------------------------------------------------
 */
/*
 *----------------------------------------------------------------------------
 *
 * VDPService_PluginExit --
 *
 *    This function is called by the VDP Service DLL right before
 *    it unloads this DLL.
 *
 *----------------------------------------------------------------------------
 */
/*
 *----------------------------------------------------------------------
 *
 * VDPService_PluginCreateInstance --
 *
 *    This function is called by the VDP Service DLL each time a server
 *    application is started that uses the same token as this DLL.
 *
 *----------------------------------------------------------------------
 */
/*
 *----------------------------------------------------------------------
 *
 * VDPService_PluginDestroyInstance --
 *
 *    This function is called by the VDP Service DLL each time a server
 *    application is closed that uses the same token as this DLL.
 *
 *----------------------------------------------------------------------
 */
#define VDP_SERVICE_CREATE_INTERFACE_IMPL(token, rpcManagerObj)                          \
EXTERN_C_OPEN                                                                            \
EXPORTFN Bool VDPService_PluginGetTokenName(char* tokenName,  /* OUT */                  \
                                            int size)         /* IN */                   \
{                                                                                        \
   LogUtils::LogInit(token, false);                                                      \
   FUNCTION_TRACE;                                                                       \
   strncpy_s(tokenName, size, token, _TRUNCATE);                                         \
   FUNCTION_EXIT_MSG("\"%s\"", tokenName);                                               \
   return true;                                                                          \
}                                                                                        \
EXPORTFN Bool VDPService_PluginInit(const VDP_SERVICE_QUERY_INTERFACE qi)                \
{                                                                                        \
   FUNCTION_TRACE;                                                                       \
   return rpcManagerObj.VDPPluginInit(qi);                                               \
}                                                                                        \
EXPORTFN Bool VDPService_PluginExit(void)                                                \
{                                                                                        \
   FUNCTION_TRACE;                                                                       \
   return rpcManagerObj.VDPPluginExit();                                                 \
}                                                                                        \
EXPORTFN Bool VDPService_PluginCreateInstance(void* hChannel,     /* IN */               \
                                              void** pUserData)   /* OUT */              \
{                                                                                        \
   FUNCTION_TRACE;                                                                       \
   return rpcManagerObj.VDPPluginCreateInstance(hChannel, pUserData);                    \
}                                                                                        \
EXPORTFN Bool VDPService_PluginDestroyInstance(void* userData)   /* IN */                \
{                                                                                        \
   FUNCTION_TRACE;                                                                       \
   return rpcManagerObj.VDPPluginDestroyInstance(userData);                              \
}                                                                                        \
EXTERN_C_CLOSE

