/* ********************************************************************************* *
 * Copyright (C) 2011-2021 VMware, Inc.  All rights reserved. -- VMware Confidential *
 * ********************************************************************************* */

/*
 * vdprpc_interfaces.h
 *
 *    The VDPRPC library defines RPC over PCoIP virtual channel.
 *
 *    VDPRPC_VariantInterface -
 *       The RPC uses VDP_RPC_VARIANT to pass the data. The parameter
 *       is encoded in a platform independant format.
 *
 *    VDPRPC_ChannelObjectInterface
 *       Application-define named object that allows to create request
 *       or receive callbacks.
 *
 *    VDPRPC_ChannelContextInterface
 *       The context for making each call.
 *
 */

#if !defined (VDP_RPC_INTERFACES_H)
#define VDP_RPC_INTERFACES_H

#include "vdprpc_defines.h"

/*
 * VDPRPC_VariantInterface
 */

typedef struct VDPRPC_VariantInterface {
   #define VDP_RPC_VARIANT_INTERFACE_V1 1
   uint32 version;

   struct {
      Bool (*VariantInit)(VDP_RPC_VARIANT *v);
      Bool (*VariantCopy)(VDP_RPC_VARIANT *target, const VDP_RPC_VARIANT *src);
      Bool (*VariantClear)(VDP_RPC_VARIANT *v);
      Bool (*VariantFromChar)(VDP_RPC_VARIANT *v, char);
      Bool (*VariantFromShort)(VDP_RPC_VARIANT *v, short);
      Bool (*VariantFromUShort)(VDP_RPC_VARIANT *v, unsigned short);
      Bool (*VariantFromInt32)(VDP_RPC_VARIANT *v, int32);
      Bool (*VariantFromUInt32)(VDP_RPC_VARIANT *v, uint32);
      Bool (*VariantFromInt64)(VDP_RPC_VARIANT *v, int64);
      Bool (*VariantFromUInt64)(VDP_RPC_VARIANT *v, uint64);
      Bool (*VariantFromFloat)(VDP_RPC_VARIANT *v, float);
      Bool (*VariantFromDouble)(VDP_RPC_VARIANT *v, double);
      Bool (*VariantFromStr)(VDP_RPC_VARIANT *v, const char *);
      Bool (*VariantFromBlob)(VDP_RPC_VARIANT *v, const VDP_RPC_BLOB *blob);
   } v1;
} VDPRPC_VariantInterface;


/*
 * Object callbacks.
 */

typedef struct _VDPRPC_ObjectNotifySink {
   #define VDP_RPC_OBJECT_NOTIFY_SINK_V1  1
   uint32 version;

   struct {
      /*
       * OnInvoke -
       *
       *    Callback fired when the peer starts a RPC request.
       */
      void (*OnInvoke)(void *userData,      // IN
                       void *contextHandle, // IN
                       void *reserved);     // IN


      /*
       * OnObjectStateChanged -
       *
       *    Callback fired when the object state has been changed.
       */
      void (*OnObjectStateChanged)(void *userData,  // IN
                                   void *reserved); // IN
   } v1;
} VDPRPC_ObjectNotifySink;


/*
 * Request callbacks.
 */

typedef struct _VDPRPC_RequestCallback {
   #define VDP_RPC_REQUEST_CALLBACK_V1 1
   uint32 version;

   struct {
      /*
       * OnDone -
       *
       *    Callback fired when the peer process the request and passed
       *    the data back.
       */
      void (*OnDone)(void *userData,       // IN
                     uint32 contextId,     // IN
                     void *contextHandle); // IN


      /*
       * OnAbort -
       *
       *    Callback fired when the call has been disrupted such as
       *    cannot find the peer object.
       */
      void (*OnAbort)(void *userData,     // IN
                      uint32 contextId,   // IN
                      Bool userCancelled, // IN
                      uint32 reason);     // IN
   } v1;
} VDPRPC_RequestCallback;


/*
 *  VDPRPC_ChannelObjectInterface
 */
typedef struct _VDPRPC_ChannelObjectInterface {
   #define VDP_RPC_CHANNEL_OBJECT_INTERFACE_V1 1
   #define VDP_RPC_CHANNEL_OBJECT_INTERFACE_V2 2
   #define VDP_RPC_CHANNEL_OBJECT_INTERFACE_V3 3
   #define VDP_RPC_CHANNEL_OBJECT_INTERFACE_V4 4
   uint32 version;

   struct {
      /*
       * CreateChannelObject -
       *
       *    Create a named object. The object is created with pending
       *    state. The peer will receive an OnPeerObjectCreated() callback.
       *    If the peer calls CreateChannelObject() with the same name, the
       *    object will be set as connected.
       *
       *    name - object name. The user defined object name that can
       *    be recognized by server/client. If the object with the same
       *    name already exists, this is a no-op. The new object
       *    handle will be returned.
       *
       *    sink - callback table.
       *
       *    userData - User defined data that will be passed to the sink
       *    callbacks.
       *
       *    configFlags - The configuration flags for this new object.
       *                  Those flags are defined in VDPRPC_ObjectConfigurationFlags.
       *
       *    objectHandle - The object handle. The handle interface is defined
       *    in VDPRPC_ChannelObjectInterface.
       */
      Bool(*CreateChannelObject)(const char *name,                            // IN
                                 const VDPRPC_ObjectNotifySink *sink,         // IN
                                 void *userData,                              // IN
                                 VDPRPC_ObjectConfigurationFlags configFlags, // IN
                                 void **objectHandle);                        // OUT


      /*
       * DestroyChannelObject -
       *
       *    Destroy the channel object.
       */
      Bool(*DestroyChannelObject)(void *objectHandle); // IN


      /*
       * GetObjectState -
       *
       *    retrieve the current object state.
       *
       *    objectHandle - The object handle.
       */
      VDPRPC_ObjectState (*GetObjectState)(void *objectHandle); // IN


      /*
       * GetObjectName -
       *
       *    retrieve the current object name.
       *
       *    objectHandle - The object handle.
       */
      Bool (*GetObjectName)(void *objectHandle,                // IN
                            char *buffer,                      // IN
                            uint32 *bufferSize);               // IN/OUT


      /*
       * CreateContext -
       *
       *    Create a new context. The context object will be auto destroyed
       *    after a whole call cycle that starts with Invoke() and ends on
       *    OnDone/OnAbort. If user doesn't call Invoke or Invoke returns
       *    false, use DestroyContext to free resource.
       *
       *    If no Invoke() on the context object gets called, the contextObject will leak.
       *
       *    objectHandle - The object handle.
       *
       *    ppcontextHandle - The context handle.
       */
      Bool (*CreateContext)(void *objectHandle,      // IN
                            void **ppcontextHandle); // OUT


      /*
       * DestroyContext -
       *
       *    Destroy the context object. See Also CreateContext.
       *
       */
      Bool (*DestroyContext)(void *contextHandle); // IN


      /*
       * Invoke -
       *
       *    Make the RPC call with the given object and the context.
       *
       *    objectHandle - The object handle.
       *
       *    contextHandle - The context handle.
       *
       *    callback - The callback fired when rpc call completes.
       *
       *    userData - The user defined data that will be passed in
       *    when rpc call completes.
       */
      Bool (*Invoke)(void *objectHandle,                     // IN
                     void *contextHandle,                    // IN
                     const VDPRPC_RequestCallback *callback, // IN
                     void *userData);                        // IN
   } v1;

   struct {
      /*
       * IsSideChannelAvailable --
       *
       *    Determines wheter a side channel of the given type is
       *    available for use by any channel objects. Currently,
       *    only one object may use an available channel.
       *
       *    type - Enum value representing the type of channel.
       */
      Bool (*IsSideChannelAvailable)(VDPRPC_SideChannelType type); // IN

      /*
       * RequestSideChannel --
       *
       *    Indicates that this channel object should send messages
       *    over a separate side channel.
       *
       *    objectHandle - The object handle.
       *
       *    type - Enum value representing the type of channel being
       *           requested.
       *
       *    token - The name of the channel to use. If NULL, then the
       *            application token will be used by default.
       */
      Bool (*RequestSideChannel)(void *objectHandle,          // IN
                                 VDPRPC_SideChannelType type, // IN
                                 const char *token);          // IN/OPT
   } v2;

   struct {

      /*
       * GetObjectOptions --
       *
       * Obtain the following object options after object is created.
       *     1) enctyption and compression options which both sides agree on.
       *     2) sidechannel types which peer does not support.
       *
       * Note:
       *    Capacity is a bitmask of which compression and encryption algorithm
       * supported at both side by vdpservice.
       *    App/Plugin could choose which algorithm it will use when it create
       * channelContext to send to peer.
       *
       *    VDP_RPC_PEER_NO_TCPSIDECHANNEL and VDP_RPC_PEER_NO_VCHANSIEDECHANNEL
       * are used to indicate which sidechannel peer could not supported.
       *    This function should be called when user requests sidechannel(channelObj is
       * already created). And user should not try to request the sidechannel which
       * peer does not supoort.
       *
       * return:
       *    False if object is not created yet.
       */

      Bool (*GetObjectOptions)(void *objectHandle,            // IN
                               uint32 *options);              // OUT

      /*
       * CreateContext -
       *
       *    Create a new context.
       *
       *    Same as v1.CreateContext but support particular compression and encryption.
       *
       * Return false if
       *    1) All error case for CreateContext
       *    2) Selected Encytption/compression algorithm does not support at either side
       *
       * Note:
       *    Encryption option will be ignored if
       *    1) Context is on secure channel
       *    2) over security tunnel and encyption if off for security tunnel.
       *
       */
      Bool (*CreateContext)(void *objectHandle,      // IN
                            uint32 options,          // IN
                            void **ppcontextHandle); // OUT
   } v3;

    struct {

      /*
       * GetObjectStateByName -
       *
       *    retrieve the given object state.
       *
       *    objectHandle - The name of the given object.
       *
       *  Return:
       *    The state of the given name object.
       */

      VDPRPC_ObjectState (*GetObjectStateByName)(const char *name); // IN

   } v4;
} VDPRPC_ChannelObjectInterface;


/*
 *  VDPRPC_ContextInterface
 */
typedef struct _VDPRPC_ChannelContextInterface {
   #define VDP_RPC_CHANNEL_CONTEXT_INTERFACE_V1 1
   #define VDP_RPC_CHANNEL_CONTEXT_INTERFACE_V2 2
   uint32 version;

   struct {
      /*
       * GetId -
       *
       *    Returns an identifier that is unique to this channel
       *    context.  This identifier is passed to the OnDone
       *    and OnAbort handlers.
       */
      uint32 (*GetId)(void *contextHandle); // IN


      /*
       * Get/Set Command -
       *
       *    For each context object, there is one command field.
       *    The command can be set as an index or as a string.
       *    The client/server should maintain the same interpretation of the command.
       */
      uint32 (*GetCommand)(void *contextHandle);    // IN

      Bool (*SetCommand)(void *contextHandle,       // IN
                         uint32);                   // IN

      Bool (*GetNamedCommand)(void *contextHandle,  // IN
                              char *buffer,         // IN
                              int bufferSize);      // IN

      Bool (*SetNamedCommand)(void *contextHandle,  // IN
                              const char *command); // IN


      /*
       * Get/Set Parameters -
       *
       *    Append VDP_RPC_VARIANT to the parameter list.
       *    Optionally, user can define parameter name for each parameter.
       *    The parameter will maintain the same order as Append* function called.
       *    No retriction applied if the same parameter name has been used multiple
       *    times. The caller needs to maintain order.
       */
      int (*GetParamCount)(void *contextHandle);          // IN

      Bool (*AppendParam)(void *contextHandle,            // IN
                          const VDP_RPC_VARIANT *v);      // IN

      Bool (*GetParam)(void *contextHandle,               // IN
                       int i,                             // IN
                       VDP_RPC_VARIANT *copy);            // IN

      Bool (*AppendNamedParam)(void *contextHandle,       // IN
                               const char *name,          // IN
                               const VDP_RPC_VARIANT *v); // IN

      Bool (*GetNamedParam)(void *contextHandle,          // IN
                            int index,                    // IN
                            char *name,                   // OUT
                            int nameSize,                 // IN
                            VDP_RPC_VARIANT *copy);       // OUT


      /*
       * Get/Set return values -
       *
       *    Append VDP_RPC_VARIANT to the return value list.
       *    Optionally, user can define name for each returned value.
       *    The returned value list maintains the same order as Append* function called.
       *    No retriction applied if the same name has been used multiple times.
       *    The caller needs to maintain order.
       */

      uint32 (*GetReturnCode)(void *contextHandle);           // IN

      Bool (*SetReturnCode)(void *contextHandle,              // IN
                            uint32 code);                     // IN

      int (*GetReturnValCount)(void *contextHandle);          // IN

      Bool (*AppendReturnVal)(void *contextHandle,            // IN
                              const VDP_RPC_VARIANT *v);      // IN

      Bool (*GetReturnVal)(void *contextHandle,               // IN
                           int i,                             // IN
                           VDP_RPC_VARIANT *v);               // IN

      Bool (*AppendNamedReturnVal)(void *contextHandle,       // IN
                                   const char *name,          // IN
                                   const VDP_RPC_VARIANT *v); // IN

      Bool (*GetNamedReturnVal)(void *contextHandle,          // IN
                                int index,                    // IN
                                char *name,                   // OUT
                                int nameSize,                 // IN
                                VDP_RPC_VARIANT *v);          // OUT
   } v1;
   struct {
      /*
       * SetOps  -
       *
       *    Options for channel context object defined in VDPRPC_ChannelContextOps.
       *
       *    VDP_RPC_CHANNEL_CONTEXT_OPT_POST
       *    it specifies if the RPC call should run in a "post" mode, which won't receive
       *    any returns from the peer. By default, this option is disabled.
       *    To enable "mode" mode, set option value to a non-zero variant.
       */
      Bool (*SetOps)(void *contextHandle, VDPRPC_ChannelContextOps op, const VDP_RPC_VARIANT *v);
   } v2;
} VDPRPC_ChannelContextInterface;


/*
 * VDPRPC_StreamDataInterface
 *
 * VDPService stream data interface for TCP
 *
 */

typedef struct _VDPRPC_StreamDataInterface {

   #define VDP_RPC_STREAMDATA_INTERFACE_V1 1
   #define VDP_RPC_STREAMDATA_INTERFACE_V2 2
   uint32 version;

   struct {

      /*
       * GetStreamDataHeaderTailSize --
       *
       *     Obtain the size of header and tail.
       *
       * Note:
       *     Header: all necessary data before blob data
       *     Tail:   all necessary data after blob data(No tail for MiniRPC)
       *     current we only support RPC command with one BLOB parameter.
       *     So, sizes are the same for each TCP connection.
       *
       *  tailLen could return 0, in this case, App should not send tail.
       *
       * Return
       *     true if everything goes well
       */

      Bool (* GetStreamDataHeaderTailSize) (int fd, int dataSize, int *headerLen, int *tailLen);


      /*
       * GetStreamDataHeaderTail --
       *     Obtain header and tail.
       *
       * Note:
       *     current we only support RPC command with one BLOB parameter.
       *
       *     Header: all necessary data before blob data
       *             include hmac if data integrity check enabled
       *     Tail:   all necessary data after blob date
       *     reqId [OUT] : return internal vdpservice request id for APP
       *     reqCmd: App defined command
       *
       *
       *     This function is mainly for optimization purpose to elimiate one memcpy.
       *     use GetStreamData for entire payload and send it via tcp socket if encryption
       *     or compression is on since memcpy could not be same any more due to data itself
       *     changed.
       *
       * Return:
       *     true if everything goes well
       */

      Bool (* GetStreamDataHeaderTail) (int fd,
                                        int *reqId,
                                        int reqCmd,
                                        VDP_RPC_BLOB *blob,
                                        char *header,
                                        int headerBufLen,
                                        char *tail,
                                        int tailBufLen);


      /*
       * GetMinimalStreamDataSize --
       *
       *   Get minimal StreamData size before we could check StreamData length.
       *   Size is the same for each TCP connection
       *
       * return:
       *   RPC wire header length + hmac (if data integrity check enabled)
       *
       */

      int (* GetMinimalStreamDataSize) (int fd);

      /*
       * GetStreamDataSize --
       *
       *    Get StreamData size after receive at least GetMinimalStreamDataSize() bytes data.
       */

      int (* GetStreamDataSize) (int fd, const char *recvData);

      /*
       * GetStreamDataInfo --
       *
       *     Parse StreamData information from received binary data.
       *
       * Note:
       *     current we only support RPC command with one BLOB parameter.
       *
       * Parameter:
       *     fd(IN) : Socket handle return from channel_interface.v2.GetTcpSocketHandle
       *     recvData(IN) : received binary data from peer
       *     reqId(OUT) : request/response Id
       *     reqType(OUT) : RPC request or response
       *     reqCmd(OUT) : request cmd (App defined)
       *     blob(OUT) : blob data(for request only)
       *
       * Return
       *     true if everything goes well.
       *
       */

      Bool (* GetStreamDataInfo) (int fd,
                                  const char *recvData,
                                  int *reqId,
                                  int *reqType,
                                  int *reqCmd,
                                  VDP_RPC_BLOB *blob);


     } v1;

     struct {

	  /*
       * GetStreamData --
       *     Obtain the entire payload to be sent via tcp socket.
       *
       * Note:
       *     current we only support RPC command with one BLOB parameter.
       *
       *     ctxOption: Context option i.e encryption, compression and MiniRPC
       *     reqId [OUT] : return internal vdpservice request id for APP
       *     reqCmd: App defined command
       *
       * Note:
       *     Please call FreeStreamDataPayload() after it is done to avoid
       * memory leak.
       *
       * Return:
       *     true if everything goes well
       */

      Bool (* GetStreamData) (int fd,
                              uint32 ctxOptions,
                              int *reqId,
                              int reqCmd,
                              VDP_RPC_BLOB *blob,
                              VDP_RPC_BLOB *payload);

      /*
       * FreeStreamDataPayload --
       *     Free payload returned by data
       *
       * Return:
       *     true if everything goes well
       */

      Bool (* FreeStreamDataPayload) (VDP_RPC_BLOB *payload);


      /*
       * GetStreamDataInfo --
       *
       *     Same as GetStreamDataInfo in v1 except one more
       * parameter bNeedCleanup to indicate if it blobdata
       * need to cleanup by app.
	   *
       *
       * Return
       *     true if everything goes well.
       *
       */

      Bool (* GetStreamDataInfo) (int fd,
                                  const char *recvData,
                                  int *reqId,
                                  int *reqType,
                                  int *reqCmd,
                                  Bool *bNeedCleanup,
                                  VDP_RPC_BLOB *blob);
   } v2;

} VDPRPC_StreamDataInterface;

#endif
