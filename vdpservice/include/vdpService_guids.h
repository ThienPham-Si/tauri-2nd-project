/* ********************************************************************************* *
 * Copyright (C) 2011-2021 VMware, Inc.  All rights reserved. -- VMware Confidential *
 * ********************************************************************************* */

/*
 * vdpService_guids.h --
 *
 *    This header file contains the GUID definitions for the vdpService
 *    interfaces.  As it actually defines the GUIDs it should only be
 *    included once.  Including more than once create multiple copies
 *    of the GUIDs, which isn't a problem, but isn't ideal.
 */

#ifndef VDP_SERVICE_GUIDS_H
#define VDP_SERVICE_GUIDS_H

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(_WIN32)
   // GUID typedef is needed for non windows platforms
   #include "vdpService.h"
#elif defined(VM_WIN_UWP)
   // For UWP, use system defined GUID
   #include <guiddef.h>
#endif

#if defined(_WIN32)
   #define VDP_SERVICE_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
           static const type name = { l, w1, w2, { b1, b2, b3, b4, b5, b6, b7, b8 } }
#else
   #define VDP_SERVICE_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
           static const type name = { l, w1, w2, (b2 << 8 | b1), { b3, b4, b5, b6, b7, b8 } }
#endif

/*
 * This macro defines a standard VDPService GUID
 */
#define VDP_SERVICE_STD_GUID(name, counter) \
   VDP_SERVICE_DEFINE_GUID(GUID, GUID_##name, \
                           0xA500A600, counter, 0x81E2, 0x88, \
                           0xc1, 0x29, 0xA7, 0xD3, 0xA9, 0x3A, 0x62)


/*
 * GUID for VDPRPC_VariantInterface.
 */
VDP_SERVICE_STD_GUID(VDPRPC_VariantInterface_V1, 0x0000); // 0

/*
 * GUID for VDPRPC_ChannelObjectInterface.
 */
VDP_SERVICE_STD_GUID(VDPRPC_ChannelObjectInterface_V1, 0x0002); // 2
VDP_SERVICE_STD_GUID(VDPRPC_ChannelObjectInterface_V2, 0x000A); // 10
VDP_SERVICE_STD_GUID(VDPRPC_ChannelObjectInterface_V3, 0x000F); // 15
VDP_SERVICE_STD_GUID(VDPRPC_ChannelObjectInterface_V4, 0x0012); // 18

/*
 * GUID for VDPRPC_ChannelContextInterface.
 */
VDP_SERVICE_STD_GUID(VDPRPC_ChannelContextInterface_V1, 0x0003); // 3
VDP_SERVICE_STD_GUID(VDPRPC_ChannelContextInterface_V2, 0x0008); // 8

/*
 * GUID for VDPOverlayGuest_Interface.
 */
VDP_SERVICE_STD_GUID(VDPOverlay_GuestInterface_V1, 0x0004); // 4
VDP_SERVICE_STD_GUID(VDPOverlay_GuestInterface_V2, 0x0009); // 9
VDP_SERVICE_STD_GUID(VDPOverlay_GuestInterface_V3, 0x0015); // 21
VDP_SERVICE_STD_GUID(VDPOverlay_GuestInterface_V4, 0x0017); // 23

/*
 * GUID for VDPOverlayClient_Interface.
 */
VDP_SERVICE_STD_GUID(VDPOverlay_ClientInterface_V1, 0x0005); // 5
VDP_SERVICE_STD_GUID(VDPOverlay_ClientInterface_V2, 0x0007); // 7
VDP_SERVICE_STD_GUID(VDPOverlay_ClientInterface_V3, 0x0016); // 22
VDP_SERVICE_STD_GUID(VDPOverlay_ClientInterface_V4, 0x0018); // 24

/*
 * GUID for VDPService_ChannelInterface
 */
VDP_SERVICE_STD_GUID(VDPService_ChannelInterface_V1, 0x0006); // 6
VDP_SERVICE_STD_GUID(VDPService_ChannelInterface_V2, 0x0010); // 16
VDP_SERVICE_STD_GUID(VDPService_ChannelInterface_V3, 0x000B); // 11
VDP_SERVICE_STD_GUID(VDPService_ChannelInterface_V4, 0x0013); // 19

/*
 * GUID for VDPRPC_StreamDataInterface
 */

VDP_SERVICE_STD_GUID(VDPRPC_StreamDataInterface_V1, 0x000C); // 12
VDP_SERVICE_STD_GUID(VDPRPC_StreamDataInterface_V2, 0x000E); // 14

/*
 * GUID for VDPService_ObserverInterface
 */
VDP_SERVICE_STD_GUID(VDPService_ObserverInterface_V1, 0x000D); // 13

/*
 * GUID for VDPService_ServerInterface
 */
VDP_SERVICE_STD_GUID(VDPService_ServerInterface_V1, 0x0011); // 17

/*
 * GUID for VDPService_LocalJobInterface
 */
VDP_SERVICE_STD_GUID(VDPService_LocalJobInterface_V1, 0x0014); // 20

#ifdef __cplusplus
}
#endif

#endif /* VDP_SERVICE_GUIDS_H */
