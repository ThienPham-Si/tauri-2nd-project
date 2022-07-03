/* ********************************************************************************* *
 * Copyright (C) 2011-2021 VMware, Inc.  All rights reserved. -- VMware Confidential *
 * ********************************************************************************* */

/*
 * vdpService.h --
 *
 */

#ifndef VDP_SERVICE_H
#define VDP_SERVICE_H

#include "vdpService_defines.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * GUID --
 *    For non-Windows platforms and UWP without WIN32 defined, GUID is needed.
 *
 */

#if !defined(_WIN32) && !defined(_VMGUID_H_)
   typedef struct _GUID {
      unsigned int   field1;
      unsigned short field2;
      unsigned short field3;
      unsigned short field4;
      unsigned char  field5[6];
   } GUID;
#endif


/*
 *
 * VDP_SERVICE_MAX_TOKEN_LENGTH --
 *
 *    The maximum length of the token passed to VDPService_ServerInit()
 *    and which can be returned from VDPService_PluginGetTokenName().
 *
 */

#define VDP_SERVICE_MAX_TOKEN_LENGTH 16


/*
 *
 * VDP_SERVICE_VERSION --
 *    The current major and minor version numbers combined into a single value
 *
 * VDP_SERVICE_VERSION_MAJOR --
 *    This value is incremented when a release includes changes to one of the
 *    interfaces.
 *
 * VDP_SERVICE_VERSION_MINOR --
 *    This value is incremented when a release doesn't make a change to one
 *    of the interfaces.  This value is reset to 0 if VDP_SERVICE_VERSION_MAJOR
 *    is incremented (e.g.  v1.2 -> v2.0).
 *
 * VDP_SERVICE_VERSION_GET_MAJOR() --
 *    Returns the major version number from the version number
 *
 * VDP_SERVICE_VERSION_GET_MINOR() --
 *    Returns the minor version number from the version number
 *
 */

#define VDP_SERVICE_VERSION_MAJOR 1
#define VDP_SERVICE_VERSION_MINOR 0

#define VDP_SERVICE_VERSION \
   ((VDP_SERVICE_VERSION_MAJOR << 16) + VDP_SERVICE_VERSION_MINOR)

#define VDP_SERVICE_VERSION_GET_MAJOR(version) (((version) >> 16) & 0xffff)
#define VDP_SERVICE_VERSION_GET_MINOR(version) ((version) & 0xffff)


/*
 * VDP_SERVICE_QUERY_INTERFACE -
 *    The entry point for retrieving all the interfaces.
 *    Each interface has a unique GUID. Use the GUID to
 *    query the corresponding interface.
 */

typedef struct _VDP_SERVICE_QUERY_INTERFACE {

   /*
    * Version -
    *
    *    The version of the API
    */
   uint32 Version;


   /*
    * QueryInterface -
    *
    *    Use the interface GUID to get the interface table.
    *
    *    iid - The GUID defined by each interface.
    *
    *    iface - The interface structure coresponding to the interface GUID.
    *    Each funciton entry will be filled.
    */
   Bool (*QueryInterface)(const GUID *iid,  // IN
                          void *iface);     // OUT
} VDP_SERVICE_QUERY_INTERFACE;


/*
 *
 * VDPService_ServerInit --
 * VDPService_ServerInitEx --
 *
 *    Function needs to be invoked when the server app gets launched.
 *
 *    sessionId - windows session ID
 *
 *    token - token name that identifies the server app and client plugin.
 *    If the same server app launches multiple instances, the client plugin
 *    function VDPService_PluginCreateInstanceFn will be invoked multiple times.
 *
 *    params - "value=string" pair delimited by the ";".
 *
 *    qi - a function table, which can be used to query all the VDPService interfaces.
 *
 *    channelHandle - handle to the internal channel object.
 *
 */

Bool VDPService_ServerInit(const char *token,               // IN
                           VDP_SERVICE_QUERY_INTERFACE *qi, // OUT
                           void **channelHandle);           // OUT

Bool VDPService_ServerInit2(unsigned long sessionId,         // IN
                            const char *token,               // IN
                            VDP_SERVICE_QUERY_INTERFACE *qi, // OUT
                            void **channelHandle);           // OUT

Bool VDPService_ServerInitEx(const char *token,               // IN
                             const char *params,              // IN
                             VDP_SERVICE_QUERY_INTERFACE *qi, // OUT
                             void **channelHandle);           // OUT

Bool VDPService_ServerInitEx2(unsigned long sessionId,         // IN
                              const char *token,               // IN
                              const char *params,              // IN
                              VDP_SERVICE_QUERY_INTERFACE *qi, // OUT
                              void **channelHandle);           // OUT

/*
 * Initialize vdpservice interface and set it to privilege.
 */

Bool VDPService_ServerInitLP(const char *token,               // IN
                             const char *params,              // IN
                             VDP_SERVICE_QUERY_INTERFACE *qi, // OUT
                             void **channelHandle);           // OUT

/*
 * Initialize vdpService multi server connection interfaces.
 *
 * Parameters:
 *    sessionId - windows session ID
 *    token : name to identify the server app and client plugins.
 *    params : "value=string" pair delimited by the ";".
 *    qi: a function table to query all the vdpService interfaces.
 *    hMultiServer : Multi-server handle.
 *
 * Return value:
 *    True if vdpService is ready to register mulit-server sink.
 *
 * Remarks:
 *    This function will put vdpservice in "listening mode" of incoming
 *    session connections from underlying protocols. Once a new server is
 *    created, OnServerConnected will be fired from vdpService internal
 *    thread. App should return the callback ASAP to not block internal
 *    vdpService thread. App could starts this server instance later when
 *    it desires. All callbacks will be fired on a dedicate vdpservice
 *    thread for both server and channel, which is A.K.A "main thread" for
 *    each server instance.
 */

Bool VDPService_MultiServerInit(unsigned long sessionId,             // IN
                                const char *token,                   // IN
                                const char *params,                  // IN
                                VDP_SERVICE_QUERY_INTERFACE *qi,     // OUT
                                VdpMultiServerID *phMultiServerId);  // OUT

/*
 * Uninitialize vdpService multi-server interfaces.
 */

Bool VDPService_MultiServerExit(VdpMultiServerID hMultiServerId,     // IN
                                void             *reserved);         // IN

/*
 *--------------------------------------------------------------------------------------
 *
 * VDPService_ServerExit --
 *
 *    Function needs to be invoked in order to notify the VDPRPC library
 *    that the server application is shutting down.
 *
 *--------------------------------------------------------------------------------------
 */

Bool VDPService_ServerExit();
Bool VDPService_ServerExit2(unsigned long sessionId);

/*
 * Client plugin interfaces.
 *
 *    VDPService_PluginInit
 *    VDPService_PluginInitEx
 *    VDPService_PluginExit
 *    VDPService_PluginGetTokenName
 *    VDPService_PluginCreateInstance
 *    VDPService_PluginDestroyInstance
 */


/*
 *
 * VDPService_PluginInit --
 * VDPService_PluginInitEx --
 *
 *    Function invoked when initializing the client plugin.
 *
 *    qi - function pointer to allow the plugin to query all
 *
 *    params - "value=string" pair delimited by the ";" returned from the client
 *    plugin.
 *
 */

#define VDP_SERVICE_PLUGIN_INIT_FN "VDPService_PluginInit"
typedef Bool (*VDPService_PluginInitFn)(const VDP_SERVICE_QUERY_INTERFACE qi);   // IN

#define VDP_SERVICE_PLUGIN_INIT_EX_FN "VDPService_PluginInitEx"
typedef Bool (*VDPService_PluginInitExFn)(const VDP_SERVICE_QUERY_INTERFACE qi, // IN
                                          char **params);                       // OUT

#define VDP_SERVICE_PLUGIN_INIT_WITH_PATH "VDPService_PluginInitWithPath"
typedef Bool (*VDPService_PluginInitWithPathFn)(const VDP_SERVICE_QUERY_INTERFACE qi, // IN
                                                const char *pluginAbsPath);           // OUT

/*
 *
 * VDPService_PluginExit --
 *
 *    Function is defined as VDPService_PluginExitFn
 *    and should be implemented by the client.
 *
 *    The function is invoked during the application shutdown.
 *
 */

#define VDP_SERVICE_PLUGIN_EXIT_FN "VDPService_PluginExit"
typedef Bool (*VDPService_PluginExitFn)(void);


/*
 *
 * VDPService_PluginGetTokenName --
 *
 *    Function is defined as VDPService_PluginGetTokenNameFn
 *    and should be implemented by the client.
 *
 *    tokenName - returned by the plugin. The token name should
 *    match the server app's token. The token length is defined by
 *    VDP_SERVICE_MAX_TOKEN_LENGTH.
 *
 *    size - the buffer size.
 */

#define VDP_SERVICE_PLUGIN_GET_TOKEN_NAME_FN "VDPService_PluginGetTokenName"
typedef Bool (*VDPService_PluginGetTokenNameFn)(char *tokenName,   // OUT
                                                int size);         // IN


/*
 *
 * VDPService_PluginCreateInstance --
 *
 *    Function is defined as VDPService_PluginCreateInstanceFn
 *    and should be implemented by the client.
 *
 *    Invoked when an app is launched on the server that has a matching token
 *    for this plugin.
 *
 *    channel - Handle to the channel object.
 *
 *    userData - customer defined object.
 *
 */

#define VDP_SERVICE_PLUGIN_CREATE_INSTANCE_FN "VDPService_PluginCreateInstance"
typedef Bool (*VDPService_PluginCreateInstanceFn)(void *channel,     // IN
                                                  void **userData);  // OUT


/*
 *
 * VDPService_PluginDestroyInstance --
 *
 *    Function is defined as VDPService_PluginDestroyInstanceFn
 *    and should be implemented by the client.
 *
 *    Invoked when the peer quits or when the client process is shutting down.
 *
 *    userData - user defined data.
 *
 */

#define VDP_SERVICE_PLUGIN_DESTROY_INSTANCE_FN "VDPService_PluginDestroyInstance"
typedef Bool (*VDPService_PluginDestroyInstanceFn)(void *userData);   // IN


/*
 * Obtain vdpservice query interface.
 */

Bool VDPService_GetQueryInterface(const char *token, VDP_SERVICE_QUERY_INTERFACE *api);

#ifdef __cplusplus
}
#endif

#endif   // VDP_SERVICE_H
