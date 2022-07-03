/* ********************************************************************************* *
 * Copyright (C) 2011-2021 VMware, Inc.  All rights reserved. -- VMware Confidential *
 * ********************************************************************************* */

/*
 * vdpService_interfaces.h --
 *
 */

#ifndef VDP_SERVICE_INTERFACES_H
#define VDP_SERVICE_INTERFACES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "vdpService_defines.h"


/*
 * VDPService_ServerNotifySink -
 *
 *   Callbacks for connection/disconnection of a server connection
 * from underlying protocols.
 *   Callbacks for each server instance(fired on its main thread).
 *
 */

typedef struct _VDPService_ServerNotifySink {
   #define VDP_SERVICE_SERVER_NOTIFY_SINK_V1   1
   uint32 version;

   struct {

      /*
       * OnServerConnected -
       *
       * Callback for a server connected from underlying protocol.
       *
       * Return TRUE if App want vdpService to start channel connection.
       * Otherwise App need to call StartServer in VDPService_ServerInterface
       * Return FALSE if App want to start channel connection later.
       *
       * Fired on internal thread, should retun ASAP.
       */

      Bool (* OnServerConnected) (void *userData,                       // IN
                                  VDPService_SessionType type,          // IN
                                  VdpServerID  hServerId);              // IN

      /*
       * OnServerDisconnected -
       *
       * Callback for a server disconnected from underlying protocol.
       * Fired on server's main thread.
       * hServerId is for App's book-keeping purpose only.
       */

      Bool (* OnServerDisconnected) (void    *userData,                  // IN
                                     VdpServerID  hServerId);            // IN

      /*
       * OnServerInstanceCreated -
       *
       * Callback when a server instance is created by vdpservice.
       *
       * All callbacks of this server will be fired on this thread,
       * which is also known as "main thread".
       */

      Bool (* OnServerInstanceCreated) (void* pServerUserData,           // IN
                                        VdpServerID hServerId,           // IN
                                        void** pInstanceUserData);       // OUT

      /*
       * OnServerInstanceDestroyed -
       *
       * Last Callback before the server "main thread" exit.
       * It could be trigger by client disconnect or Stop(All)Server(s) is
       * called in VDPService_ServerInterface.
       */

      Bool (* OnServerInstanceDestroyed) (void   *pServerUserData,       // IN
                                          void   *pInstanceUserData,     // IN
                                          VdpServerID hServerId);        // IN

   } v1;

} VDPService_ServerNotifySink;


/*
 * VDPService_MsgChannelNotifySink -
 *
 *  VDPService Message channel notification callbacks.
 *
 *  Note(XXX): The following comment will be removed before release.
 *    Wrapper of VvcMsgChannelEvents
 */

typedef struct _VDPService_MsgChannelNotifySink {
   #define VDP_SERVICE_MSG_CHANNEL_NOTIFY_SINK_V1      1
   uint32 version;

   struct {

      /*
       * OnMsgChannelOpened -
       *
       * Callback when underlying protocol opens a Message Channel.
       *
       * Note(XXX): The following comment will be removed before release
       *    hChannelId will map to an internal object which include:
       *    VvcSessionID,
       *    VvcMsgChannelIdentity(featureName derive from tokenName),
       *    VvcMsgChannelHandle,
       *    VvcMsgChannelGroupID
       */

      Bool (* OnMsgChannelOpened) (void   *userData,                   // IN
                                   VdpMsgChannelID hChannelId);        // IN

      /*
       * OnMsgChannelMembershipChanged -
       *
       * Callback when a msg channel membership changes from underlying protocol.
       * if bAdd is TRUE, new MsgChannel is added, otherwise an existing MsgChannel
       * is removed.
       */

      void (* OnMsgChannelMembershipChange) (void   *userData,          // IN
                                             VdpMsgChannelID hChannelId,// IN
                                             VdpMsgGroupID   hGroupId,  // IN
                                             Bool   bAdd);              // IN

      /*
       * OnMsgReceived -
       *
       *   It will be fired on main thread when a message is received from
       * the message channel.
       */

      void (* OnMsgReceived) (void   *userData,                         // IN
                              VdpMsgGroupID hGroupId,                   // IN
                              VdpMsgGroupID hSrcChannelId,              // IN
                              void   *msg,                              // IN
                              int    msgLen);                           // IN

   } v1;

} VDPService_MsgChannelNotifySink;


/*
 * VDPService_MsgChannelGroupNotifySink -
 *
 *  VDPService Message channel group notification callbacks.
 *
 *  Note(XXX): The following comment will be removed before release.
 *    Waiting for API from VVC team
 */

typedef struct _VDPService_MsgChannelGroupNotifySink {
   #define VDP_SERVICE_MSG_CHANNEL_GROUP_NOTIFY_SINK_V1      1
   uint32 version;

   struct {

      /*
       * OnMsgChannelGroupCreated -
       *
       * Callback when underlying protocol create a Message Channel group.
       *
       * Note:(XXX todo: remove commemts) hGroupId also maps to internal group object.
       */

      Bool (* OnMsgChannelGroupCreated) (void   *userData,                   // IN
                                         VdpMsgGroupID hGroupId);            // IN

   } v1;

} VDPService_MsgChannelGroupNotifySink;


/*
 * VDPService_MsgChannelInfo -
 *
 *    struct to retrieve msgChannel informations.
 */

typedef struct _VDPService_MsgChannelInfo {
   #define VDP_SERVICE_MSG_CHANNEL_INFO_V1  1
   uint32 version;

   struct {
      int32  serverPid;
      char   serverToken[VDP_SERVICE_MAX_MSG_CHANNEL_INFO_LENGTH + 1];
      char   msgChannelName[VDP_SERVICE_MAX_MSG_CHANNEL_INFO_LENGTH + 1];
   } v1;

} VDPService_MsgChannelInfo;

/*
 * VDPService_ServerInterface -
 *
 *    The methods in this struct are used to register ServerNotifySink(callbacks)
 * start/stop particular server, create message channel and message channel group.
 *
 * Unlike other vdpservice APIs, they are thread-safe APIs(could be
 * called from any thread).
 */

typedef struct _VDPService_ServerInterface {
   #define VDP_SERVICE_SERVER_INTERFACE_V1  1
   uint32  version;

   struct {

      /*
       * RegisterServerNotifySink -
       *
       *    Register callbacks for server notifications.
       *
       *    sink - the table of callbacks.
       *
       *    userData - the user defined object that will be passed when sink
       *               callbacks are invoked.
       *
       *    hMultiServerId - ID returned from VDPService_MultiServerInit.
       *
       *    phSinkId - The handle used to unregister the sink.
       *
       *    It is per-process per user based for v1(single feature per process).
       */

      Bool (* RegisterServerNotifySink) (void *userData,                             // IN
                                         const VDPService_ServerNotifySink *sink,    // IN
                                         VdpMultiServerID  hMultiServerId,           // IN
                                         VdpServerNotifySinkID *phSinkId);           // OUT

      /*
       * UnregisterServerNotifySink -
       *
       *    Unregister sink for server notifications.
       *
       *    hSink - The handle returned from RegisterServerNotifySink.
       */

      Bool (* UnregisterServerNotifySink) (VdpServerNotifySinkID hSinkId);                          // IN


      /*
       * StartServer -
       *
       *    Start a server.
       *
       *    hServerId - server id from OnServerConnected.
       *    No need if OnServerConnected returns TRUE.
       */

      Bool (* StartServer) (void   *userData,                                         // IN
                            VdpServerID  hServerId);                                  // IN

      /*
       * StopServer -
       *
       *    Stop the given server.
       *
       *    hServerId - server id from OnServerConnected
       */

      Bool (* StopServer) (VdpServerID  hServerId);                                    // IN

      /*
       * StopAllServers -
       *
       *    Stop all servers for the same user of hServerId.
       */

      Bool (* StopAllServers) (VdpServerID  hServerId);

      /*
       * GetChannel -
       *
       *     Obtain channel from serverId.
       */

      void * (* GetChannel) (VdpServerID  hServerId);                                  // IN

      /*
       * GetUniqueSerializedIdSize -
       *
       *     Helper function to obtain the size of Blast unique Serialized id.
       *
       * Note:
       *   Always return 0 for PCoIP or Blast in single server mode.
       *   Otherwise, return the size of id including null terminator.
       */

      int (* GetUniqueSerializedIdSize) (VdpServerID  hServerId);                       // IN

      /*
       * GetUniqueSerializedId -
       *
       *     Helper function to obtain Blast unique Serialized id.
       *
       * Note:
       *   Always return False for PCoIP or Blast in single server mode.
       *   Otherwise, return True if buffer is enough to hold the id.
       */

      Bool (* GetUniqueSerializedId) (VdpServerID hServerId,                            // IN
                                      int size,                                         // IN
                                      char *sId);                                       // OUT

      /*
       * IsUniqueSerializedIdOwner -
       *
       *     Check if the given id belongs to hServerId.
       *
       * Note:
       *   Always return False for PCoIP or Blast in single server mode.
       *   Otherwise, return True if they matches in Blast.
       */

      Bool (* IsUniqueSerializedIdOwner) (VdpServerID hServerId,                        // IN
                                          const char *sUniqueSerializedId);             // IN

      /*
       * OpenMsgChannel -
       *
       *    Open message channel in underlying protocol.
       *    It is non-blocking call, hChannelId should obtain from OnMsgChannelOpened callback.
       *
       *  Note(XXX): Remove when release.
       *     VvcMsgChannelIdentity will be formed as
       *            msgChannelName = channelName
       *            featureName = BlastUtils::VvcGetFeatureNameFromToken(mToken)
       */

      Bool (* OpenMsgChannel) (void   *userData,                                        // IN
                               VdpServerID hServerId,                                   // IN
                               const char *channelName,                                 // IN
                               VDPService_MsgChannelNotifySink *sink);                  // IN

      /*
       * GetMsgChannelInfo -
       *
       *     Retrieve message channel information into pInfo.
       */

      Bool (* GetMsgChannelInfo) (VdpServerID hServerId,                                // IN
                                  VdpMsgGroupID hChannelId,                           // IN
                                  VDPService_MsgChannelInfo *pInfo);                    // IN/OUT

      /*
       * CloseMsgChannel -
       *
       *    Close message channel in underlying protocol.
       *    It is blocking call.
       */

      Bool (* CloseMsgChannel) (VdpMsgChannelID hChannelId);                             // IN

      /*
       * CreateMsgChannelGroup -
       *
       *   Create message channel group in underlying protocol.
       */

      Bool (* CreateMsgChannelGroup) (void   *userData,                                 // IN
                                      VdpServerID hServerId,                            // IN
                                      VDPService_MsgChannelGroupNotifySink *sink,       // IN
                                      VdpMsgChannelID hChannel[],                       // IN
                                      uint32 numOfChannel);                             // IN

      /*
       * SendMsgToGroup -
       *
       *   Send message channel to group in underlying protocol.
       *
       *   Note:
       *      Group need to be create for single receipt case as well.
       */

      Bool (* SendMsgToGroup) (VdpMsgChannelID hChannelId,                              // IN
                               VdpMsgGroupID hGroupId,                                  // IN
                               void   *msg,                                             // IN
                               int    msgLen);                                          // IN

      /*
       * DestroyMsgChannelGroup -
       *
       *    Destroy message channel group.
       */

      Bool (* DestroyMsgChannelGroup) (VdpMsgGroupID hGroupId);                         // IN

   } v1;

} VDPService_ServerInterface;


/*
 * VDPService_ChannelNotifySink -
 *
 *    Call backs for channel state changes.
 */

typedef struct _VDPService_ChannelNotifySink {
   #define VDP_SERVICE_CHANNEL_NOTIFY_SINK_V1   1
   uint32 version;

   struct {
      /*
       * OnConnectionStateChanged -
       *
       *    Callback fired when the overall connection state changed.
       */
       void (*OnConnectionStateChanged)(void *userData,                            // IN
                                        VDPService_ConnectionState currentState,   // IN
                                        VDPService_ConnectionState transientState, // IN
                                        void *reserved);                           // IN


      /*
       * OnChannelStateChanged -
       *
       *    Callback fired when the channel state changed.
       */
      void (*OnChannelStateChanged)(void *userData,                         // IN
                                    VDPService_ChannelState currentState,   // IN
                                    VDPService_ChannelState transientState, // IN
                                    void *reserved);                        // IN


      /*
       * OnPeerObjectCreated -
       *
       *    Callback fired when peer creates an object.
       */
      void (*OnPeerObjectCreated)(void *userData,      // IN
                                  const char *objName, // IN
                                  void *reserved);     // IN
   } v1;
} VDPService_ChannelNotifySink;


/*
 * VDPService_ChannelInterface -
 *    The methods in this struct are used to interact with the
 *    VDPService channel. The channel represents the connection
 *    between the two endpoints.
 */

typedef struct _VDPService_ChannelInterface {
   #define VDP_SERVICE_CHANNEL_INTERFACE_V1  1
   #define VDP_SERVICE_CHANNEL_INTERFACE_V2  2
   #define VDP_SERVICE_CHANNEL_INTERFACE_V3  3
   #define VDP_SERVICE_CHANNEL_INTERFACE_V4  4
   uint32 version;

   struct {
      /*
       * ThreadInitialize -
       *
       *    Initialize the thread to use VDPService interface.
       *
       *    The function needs to be invoked on the thread that uses VDPService
       *    interface. It has been invoked by default on the thread that calls
       *    VDPService_ServerInit.
       *
       *    channelHandle - the channel handle.
       *    At server side, VDPService_ServerInit returns this handle.
       *    At client side, VDPService_PluginCreateInstanceFn will get the channel object.
       */
      Bool (*ThreadInitialize)(void *channelHandle, // IN
                               uint32 unusedFlag);  // IN   Always 0 for now.


      /*
       * ThreadUninitialize -
       *
       *    Uninitialize the thread to use VDPService interfaces. The function
       *    has been invoked by default on the thread that calls
       *    VDPService_ServerExit.
       */
      Bool (*ThreadUninitialize)(void);


      /*
       * Poll -
       *
       *    Allow the thread to process any waiting events.  This
       *    must be called on any thread where ThreadInitialize() was called.
       *    On windows, Poll() dispatches windows RPC message through a window
       *    message pump. So it is not required if there is already a messasge
       *    loop provided in the calling thread.
       */
      void (*Poll)(void);


      /*
       * RegisterChannelNotifySink -
       *
       *    Register callbacks for channel notifications.
       *
       *    sink - the callback table.
       *
       *    userData - the user defined object that will be passed when sink
       *               callbacks are invoked.
       *
       *    sinkHandle - The handle used to unregister the sink.
       */
      Bool (*RegisterChannelNotifySink)(const VDPService_ChannelNotifySink *sink, // IN
                                        void *userData,                           // IN
                                        uint32 *sinkHandle);                      // OUT


      /*
       * UnregisterChannelNotifySink -
       *
       *    Unregister sink for channel notifications.
       *
       *    sinkHandle - The handle returned from RegisterChannelNotifySink.
       */
      Bool (*UnregisterChannelNotifySink)(uint32 sinkHandle); // IN


      /*
       * Connect -
       *
       *    Start the channel connection.
       */
      Bool (*Connect)(void);


      /*
       * Disconnect -
       *
       *    Disconnect the channel connection.
       */
      Bool (*Disconnect)(void);


      /*
       * GetConnectionState -
       *
       *    Retrieve the current state of the connection.
       */
      VDPService_ConnectionState (*GetConnectionState)(void);


      /*
       * GetChannelState -
       *
       *    Retrieve the current channel state.
       */
      VDPService_ChannelState (*GetChannelState)(void);
   } v1;

   struct {

      /*
       * SwitchToStreamDataMode
       *
       * Switch to "streamData mode" and output tcpSidechanel socket handle.
       * In "streamData mode",
       *    No worker thread is running.
       *    No incoming/outgoing data is processed(except channelDisconnect).
       *    ChannelDisconnect callback will be fired on vchan callback thread.
       *    ServerExit2 need to be called for final cleanup.
       * If it is already in "streamData mode", output fd and return True
       *
       * return:
       *    Return true if everything go well on agent.
       *    For client, return False with ASSERT_NOT_IMPLEMENTED("")
       *
       */

      Bool (* SwitchToStreamDataMode) (const char *tcpObjName, void *channelHandle, int *fd);

      /*
       * GetSessionType
       *
       * Returns an enum indicating the type of session that the vdpservice
       * is connected to.
       */
      VDPService_SessionType (* GetSessionType) ();

   } v2;

   struct {

      /*
       * Poll -
       *
       *    Allow vdpservice to process its message on main thread.
       *    If there is no pending message, it will block till next
       *    message or given timeout(ms) is reached.
       *
       *    Note: It should be only called from vdpservice main thread.
       *
       *    On windows, Poll() dispatches windows RPC message through a window
       *    message pump. So it is not required if there is already a messasge
       *    loop provided in the calling thread.
       */

      void (*Poll)(int timeout);
   } v3;

   struct {

      /*
       * ThreadInitialize -
       *
       *    Initialize the thread to use VDPService interface.
       *
       *  Same as v1.ThreadInitialize() but return VdpLocalJobDispatcher for
       *  user to schedule "local jobs" from vdpservice.
       *
       *  Note:
       *     The VdpLocalJobDispatcher is valid till ThreadUninitialize().
       *     For main thread, it is valid till ServerExit(),
       *     OnServerInstanceDestroyed() or DestroyPluginInstance() is called.
       */

      VdpLocalJobDispatcher
      (*ThreadInitialize)(void *channelHandle,                    // IN
                          uint32 unusedFlag);                     // IN  reserved.

      /*
       * Obtain the VdpLocalJobDispatcher of the channel.
       */

      VdpLocalJobDispatcher
      (* GetChannelLocalJobDispatcher)(void *channelHandle);        // IN

   } v4;

} VDPService_ChannelInterface;


/*
 * VDPService_ObserverInterface
 *
 * VDPService Observer interface
 *    This is an in-proc observer interface, i.e observers and subject
 * suposed to be in the same process.
 *    User could register observer from any plugins or rmks with name,
 * context and callback function.
 *    When data/subject call broadcast, registered callbacks will be fired
 * with data and cookie as parameter in updater's thread.
 *    Context could be use to pass data between registerObserver and broadcast.
 *
 */

typedef long VDPService_ObserverId;
typedef Bool (*VdpServiceObserverCallback) (void *context,
                                            const char *sourceToken,
                                            const void *cookie,
                                            const void *data);

typedef struct _VDPService_ObserverInterface {

   #define VDPSERVICE_OBSERVER_INTERFACE_V1 1
   uint32 version;

   struct {

      /*
       * RegisterObserver --
       *
       *    Register observer with given name and callbacks.
       *
       * Return
       *     return VDPService_ObserverId(0 means error).
       */

      VDPService_ObserverId (* RegisterObserver) (const char *name,
                                                  void *context,
                                                  VdpServiceObserverCallback cb);

      /*
       * UnregisterObserver --
       *
       *    Unregister observer with given VDPService_ObserverId.
       *
       * Return
       *     return true if succeed, false if it could not be found
       */

      Bool (* UnregisterObserver)(VDPService_ObserverId id);

      /*
       * Broadcast --
       *
       *    Broadcast data/subject to all observers.
       *
       * Note:
       *    Cookie is user-defined. It could be as simply as request id.
       *    More than one update with data=NULL is not allowed to prevent
       *    potential deadlock.
       *
       * Return:
       *    true if everything goes well.
       *
       * Side effects:
       *    callback hold mutex lock for particular name.
       *
       */

      Bool (* Broadcast) (const char *name,
                          const void *cookie,
                          const void *data);

   } v1;

} VDPService_ObserverInterface;


/*
 * VDPService_LocalJobInterface
 *
 * vdpService local job interface is used to schedule a local
 * job on vdpservice's thread(same thread for RPC callbacks).
 *
 */

typedef Bool (*VdpServiceLocalJob) (void *userData);

typedef struct _VDPService_LocalJobInterface {

   #define VDP_SERVICE_LOCAL_JOB_INTERFACE_V1 1
   uint32 version;

   struct {
      /*
       * Request local job running on the given VdpLocalJobDispatcher.
       * (From VDPService_ChannelInterface.v4.ThreadInitialize() or
       *  VDPService_ChannelInterface.v4.GetChannelLocalJobDispatcher
       *
       * "job" will run with userData as input parameter.
       * And it is caller's responsibility to ensure userData is valid
       * 1) when job is issued(called)
       * 2) or before VDPService_ChannelInterface.v1(4).ThreadUninitialize() returns
       * 3) or before DestroyPluginInstance() and OnServerInstanceDestroyed
       *    for vdpservice main thread.
       */

      Bool
      (* Request) (VdpLocalJobDispatcher      hDispatcher,    // IN
                   const VdpServiceLocalJob   job,            // IN
                   void                       *userData);     // IN
   } v1;

} VDPService_LocalJobInterface;

#ifdef __cplusplus
}
#endif

#endif   // VDP_SERVICE_INTERFACES_H
