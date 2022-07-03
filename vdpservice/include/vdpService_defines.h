/* ********************************************************************************* *
 * Copyright (C) 2011-2021 VMware, Inc.  All rights reserved. -- VMware Confidential *
 * ********************************************************************************* */

/*
 * vdpService_defines.h --
 *
 */

#if !defined (VDP_SERVICE_DEFINES_H)
#define VDP_SERVICE_DEFINES_H

#include "vmware.h"

/*
 * Reg key to find where 32bits vdpservice is installed.
 * 64bits vdpservice.dll is installed under .\x64 in this folder.
 */

#define VDP_SERVICE_INSTALL_PATH_REG_KEY  "HKLM\\Software\\VMware, Inc.\\VMware VDM\\RemoteExperienceAgent\\InstallPath"

#define VDP_SERVICE_PATH_REG_HIVE         HKEY_LOCAL_MACHINE
#define VDP_SERVICE_PATH_REG_KEY          "Software\\VMware, Inc.\\VMware VDM\\RemoteExperienceAgent"
#define VDP_SERVICE_PATH_REG_NAME         "InstallPath"


/*
 * Invalid observer id
 */

#define VDPOBSERVER_INVALID_ID            0


/*
 * define invalid sink handle
 */

#define VDP_SERVICE_INVALID_SINK_HANDLE   -1


/*
 * MultiServer ID
 */

typedef void* VdpMultiServerID;
#define VDP_SERVICE_INVALID_MULTISERVER_ID NULL


/*
 * server id
 */

typedef void* VdpServerID;
#define VDP_SERVICE_INVALID_SERVER_ID     NULL


/*
 * server notification sink id
 */

typedef void* VdpServerNotifySinkID;
#define VDP_SERVICE_INVALID_SERVER_NOTIFY_SINK_ID   NULL


/*
 * MsgChannel id
 */

typedef void* VdpMsgChannelID;
#define VDP_SERVICE_INVALID_MSGCHANNEL_ID NULL


/*
 * Vdpservice Local job Dispatcher
 */

typedef uint32 VdpLocalJobDispatcher;
#define VDP_SERVICE_INVALID_LOCAL_JOB_DISPATCHER        0


/*
 * message group id
 */

typedef void* VdpMsgGroupID;
#define VDP_SERVICE_INVALID_MSGGROUP_ID   NULL


/*
 * Broadcast message group id
 */

#define VDP_SERVICE_BROADCAST_GROUP_ID    ((VdpMsgGroupID) (-1))


/*
 * Maximum buffer length for any msgChannel info.
 */

#define VDP_SERVICE_MAX_MSG_CHANNEL_INFO_LENGTH        255


/*
 * VDPService_ConnectionState -
 *
 *    The state that indicates remote connection.
 */

typedef enum _VDPService_ConnectionState {
   VDP_SERVICE_CONN_UNINITIALIZED = -1,
   VDP_SERVICE_CONN_DISCONNECTED = 0,
   VDP_SERVICE_CONN_PENDING = 1,
   VDP_SERVICE_CONN_CONNECTED = 2
} VDPService_ConnectionState;


/*
 * VDPService_ChannelState -
 *
 *    The state indicates the channel connection.
 */

typedef enum _VDPService_ChannelState {
   VDP_SERVICE_CHAN_UNINITIALIZED = -1,
   VDP_SERVICE_CHAN_DISCONNECTED = 0,
   VDP_SERVICE_CHAN_PENDING = 1,
   VDP_SERVICE_CHAN_CONNECTED = 2
} VDPService_ChannelState;


/*
 * VDPService_SessionType -
 *
 *    The session type of the channel.
 */

typedef enum _VDPService_SessionType {
   VDP_SERVICE_NONE_SESSION = -1,
   VDP_SERVICE_PCOIP_SESSION = 0,
   VDP_SERVICE_BLAST_SESSION
} VDPService_SessionType;

#endif
