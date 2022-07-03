/* ********************************************************************************* *
 * Copyright (C) 2011-2021 VMware, Inc.  All rights reserved. -- VMware Confidential *
 * ********************************************************************************* */

/*
 * vdprpc_defines.h --
 *
 */

#if !defined (VDP_RPC_DEFINES_H)
#define VDP_RPC_DEFINES_H

#include "vmware.h"

/*
 * define invalid channel context ID
 */
#define VDP_RPC_INVALID_CHANNEL_CTX_ID 0

/*
 * define VDPRPC facility code
 */
#define VDP_RPC_FACILITY_CODE     199

#define VDP_RPC_WARNING_FIRST   (VDP_RPC_FACILITY_CODE << 16 | 0x40000000)
#define VDP_RPC_INFO_FIRST      (VDP_RPC_FACILITY_CODE << 16 | 0x80000000)
#define VDP_RPC_ERROR_FIRST     (VDP_RPC_FACILITY_CODE << 16 | 0xC0000000)

/*
 * define error code.
 */
#define VDP_RPC_SUCCESS                   0
#define VDP_RPC_FROM_WIN32(x)             (x <= 0 ? x : (((x) & 0x0000FFFF) | (FACILITY_WIN32 << 16) | 0x80000000)
#define VDP_RPC_E_FAIL                    (VDP_RPC_ERROR_FIRST + 1)
#define VDP_RPC_E_APARTMENT_UNINITIALIZED (VDP_RPC_ERROR_FIRST + 2)
#define VDP_RPC_E_APARTMENT_THREAD        (VDP_RPC_ERROR_FIRST + 3)
#define VDP_RPC_E_OBJECT_EXISTS           (VDP_RPC_ERROR_FIRST + 4)
#define VDP_RPC_E_OBJECT_NOT_FOUND        (VDP_RPC_ERROR_FIRST + 5)
#define VDP_RPC_E_OBJECT_NOT_CONNECTED    (VDP_RPC_ERROR_FIRST + 6)
#define VDP_RPC_E_PARAMETER               (VDP_RPC_ERROR_FIRST + 7)
#define VDP_RPC_E_MEMORY                  (VDP_RPC_ERROR_FIRST + 8)

/*
 * Define encryption support
 */
#define VDP_RPC_CRYPTO_AES                (1 << 22)       // Support AES
#define VDP_RPC_CRYPTO_SALSA              (1 << 23)       // Support SALSA

/*
 * Define compression support
 */
#define VDP_RPC_COMP_ZLIB                 (1 << 10)       // Support ZLIB
#define VDP_RPC_COMP_SNAPPY               (1 << 11)       // Support Snappy
#define VDP_RPC_COMP_MSFT                 (1 << 15)       // Support microsoft API

/*
 * Define the type of sidechannel peer does not support.
 *
 * For thest two bits,
 *   1  1   means: Peer cannot have sidechannel.
 *   0  1   means: Peer can only create Tcp sidechannel.
 *   1  0   means: Peer can only create vchan sidechannel.
 *   0  0   means: Peer could create either type of sidechannel.
 *                 (default and old client scenario)
 *
 * These two bits should be set when channelObj is created as configuration
 * flags(see VDP_RPC_OBJ_CONFIG_NO_TCPSIDECHANNEL/VCHANSIDECHANNEL) and
 * send to peer via vdpservice. When channelObj is connected, peer
 * could query it via GetObjectOptions. To be consistent, same bits are used
 * for both CreateChannelObject and GetObjectOptions.
 */

#define VDP_RPC_PEER_NO_TCPSIDECHANNEL       VDP_RPC_OBJ_CONFIG_NO_TCPSIDECHANNEL
#define VDP_RPC_PEER_NO_VCHANSIEDECHANNEL    VDP_RPC_OBJ_CONFIG_NO_VCHANSIDECHANNEL

// Peer will try BEAT sidechannel if possible.
#define VDP_RPC_PEER_PREFER_BEATSIDECHANNEL  VDP_RPC_OBJ_PREFER_BEATSIDECHANNEL

/*
 * Define enum for the accepted data type used in the RPC channel.
 */
typedef unsigned short VDP_RPC_VARTYPE;
typedef enum _VDP_RPC_VARENUM {
   VDP_RPC_VT_EMPTY = 0,
   VDP_RPC_VT_NULL = 1,
   VDP_RPC_VT_I2 = 2,
   VDP_RPC_VT_I4 = 3,
   VDP_RPC_VT_R4 = 4,
   VDP_RPC_VT_R8 = 5,
   VDP_RPC_VT_I1 = 16,
   VDP_RPC_VT_UI1 = 17,
   VDP_RPC_VT_UI2 = 18,
   VDP_RPC_VT_UI4 = 19,
   VDP_RPC_VT_I8 = 20,
   VDP_RPC_VT_UI8 = 21,
   VDP_RPC_VT_LPSTR = 30,
   VDP_RPC_VT_BLOB = 65,
} VDP_RPC_VARENUM;

/*
 * VDPRPC blob data.
 */
typedef struct _VDP_RPC_BLOB {
   uint32 size;
   char *blobData;
} VDP_RPC_BLOB;

/*
 * Generic struct for wrapping data.
 */
typedef struct _VDP_RPC_VARIANT {
   VDP_RPC_VARTYPE vt;
   union {
       short iVal;
       unsigned short uiVal;
       int32 lVal;
       uint32 ulVal;
       int64 llVal;
       uint64 ullVal;
       char cVal;
       float fVal;
       double dVal;
       char *strVal;
       VDP_RPC_BLOB blobVal;
   };
} VDP_RPC_VARIANT;


/*
 * VDPRPC_ObjectState -
 *
 *    The RPC object state.
 */

typedef enum _VDPRPC_ObjectState {
   VDP_RPC_OBJ_UNINITIALIZED = -1,
   VDP_RPC_OBJ_DISCONNECTED = 0,
   VDP_RPC_OBJ_PENDING = 1,
   VDP_RPC_OBJ_CONNECTED = 2,
   VDP_RPC_OBJ_SIDE_CHANNEL_PENDING = 3,
   VDP_RPC_OBJ_SIDE_CHANNEL_CONNECTED = 4,
   VDP_RPC_OBJ_PENDING_AND_PEER_OBJ_CREATED = 5,
} VDPRPC_ObjectState;


/*
 * VDPRPC_ConfigurationFlags -
 *
 *    Configuration flags for the channel object.
 *
 * VDP_RPC_OBJ_CONFIG_NO_TCPSIDECHANNEL and VDP_RPC_OBJ_CONFIG_NO_VCHANSIDECHANNEL
 * are used to notify its peer about desired type of sidechannel(if need).
 *
 * For those two flags,
 * 1  1   means: Peer cannot have sidechannel.
 * 0  1   means: Peer can only create Tcp sidechannel.
 * 1  0   means: Peer can only create vchan sidechannel.
 * 0  0   means: Peer could create either type of sidechannel.
 *               (default and old version scenario).
 */

typedef enum _VDPRPC_ObjectConfigurationFlags {
   VDP_RPC_OBJ_CONFIG_DEFAULT = 0,
   VDP_RPC_OBJ_CONFIG_INVOKE_ALLOW_ANY_THREAD  = 1 << 0,
   VDP_RPC_OBJ_SUPPORT_COMPRESSION = 1 << 2,
   VDP_RPC_OBJ_SUPPORT_ENCRYPTION = 1 << 3,
   VDP_RPC_OBJ_CONFIG_NO_TCPSIDECHANNEL = 1 << 4,
   VDP_RPC_OBJ_CONFIG_NO_VCHANSIDECHANNEL = 1 << 5,
   VDP_RPC_OBJ_PREFER_BEATSIDECHANNEL = 1 << 6
} VDPRPC_ObjectConfigurationFlags;

/*
 * VDPRPC_ChannelContextOps -
 *
 *    Channel context options.
 */

typedef enum _VDPRPC_ChannelContextOps {
   VDP_RPC_CHANNEL_CONTEXT_OPT_POST = 1,
   VDP_RPC_CHANNEL_CONTEXT_OPT_BEGIN_ASYNC_RESULT = 2,
   VDP_RPC_CHANNEL_CONTEXT_OPT_END_ASYNC_RESULT = 3,
} VDPRPC_ChannelContextOps;


/*
 * VDPRPC_SideChannelType -
 *
 *    Potential types for side channels.
 */
typedef enum _VDPRPC_SideChannelType {
   VDP_RPC_SIDE_CHANNEL_TYPE_PCOIP = 1,
   VDP_RPC_SIDE_CHANNEL_TYPE_TCP = 2,
   VDP_RPC_SIDE_CHANNEL_TYPE_BEAT = 3,
} VDPRPC_SideChannelType;

#endif
