/* ********************************************************************************* *
 * Copyright (C) 2011-2021 VMware, Inc.  All rights reserved. -- VMware Confidential *
 * ********************************************************************************* */

/*
 * VMR9OverlayGuest.h --
 *
 */

#ifndef VMR9OVERLAYGUEST_H
#define VMR9OVERLAYGUEST_H

#include "resource.h"
#include "VMR9OverlayInterface.h"


/*
 *----------------------------------------------------------------------
 *
 * Extra layout modes to use with the layout menu
 *
 *----------------------------------------------------------------------
 */
static const VDPOverlay_LayoutMode VDP_OVERLAY_LAYOUT_SCALE_MULTIPLE =
   (VDPOverlay_LayoutMode)(VDP_OVERLAY_LAYOUT_SCALE | VDP_OVERLAY_LAYOUT_MULTIPLE_CENTER);

static const VDPOverlay_LayoutMode VDP_OVERLAY_LAYOUT_CROP_MULTIPLE =
   (VDPOverlay_LayoutMode)(VDP_OVERLAY_LAYOUT_CROP | VDP_OVERLAY_LAYOUT_MULTIPLE_CENTER);

static const VDPOverlay_LayoutMode VDP_OVERLAY_LAYOUT_LETTERBOX_MULTIPLE =
   (VDPOverlay_LayoutMode)(VDP_OVERLAY_LAYOUT_LETTERBOX | VDP_OVERLAY_LAYOUT_MULTIPLE_CENTER);

static const VDPOverlay_LayoutMode VDP_OVERLAY_LAYOUT_DEFAULT = VDP_OVERLAY_LAYOUT_LETTERBOX;


/*
 *----------------------------------------------------------------------
 *
 * Color values to use the background menu
 *
 *----------------------------------------------------------------------
 */
static const uint32 BGCOLOR_BLACK    = 0xFF000000;
static const uint32 BGCOLOR_WHITE    = 0xFFFFFFFF;
static const uint32 BGCOLOR_RED      = 0xFFFF4040;
static const uint32 BGCOLOR_BLUE     = 0xFF4040FF;
static const uint32 BGCOLOR_PURPLE   = 0XFFC000C0;
static const uint32 BGCOLOR_DISABLED = 0;
static const uint32 BGCOLOR_DEFAULT  = BGCOLOR_DISABLED;


/*
 *----------------------------------------------------------------------
 *
 * Struct OverlaySettings
 *
 *    Saved settings if I have to destroy and re-create the overlay
 *
 *----------------------------------------------------------------------
 */
struct OverlaySettings
{
   WINDOWPLACEMENT         mainWindowPlacement = { 0 };
   std::string             videoPath;
   bool                    overlayEnabled = true;
   bool                    copyImages = false;
   VDPOverlay_LayoutMode   layoutMode = VDP_OVERLAY_LAYOUT_DEFAULT;
   uint32                  bgColor = BGCOLOR_DEFAULT;
   VDPOverlay_Rect         areaRect = { 0 };
   bool                    areaEnabled = false;
   bool                    areaClipped = false;
   UINT                    areaSizeId = ID_SIZE_16x9;
   UINT                    areaPosId = ID_POSITION_CENTER;
};


/*
 *----------------------------------------------------------------------
 *
 * Class VMR9OverlayApp
 *
 *    Manages the vdpService Overlay and RPC APIs
 *
 *----------------------------------------------------------------------
 */
class VMR9OverlayApp : public VMR9OverlayPlugin
{
public:
   VMR9OverlayApp()
      : m_rpcMgr(VMR9_OVERLAY_TOKEN_NAME),
        VMR9OverlayPlugin(&m_rpcMgr)
   {
   }

   void OnReady();
   void OnNotReady();

   void OnVideoReady(VDPOverlay_WindowId overlayWindowId);
   void OnOverlayReady(VDPOverlay_WindowId overlayWindowId);

   RPCManager  m_rpcMgr;
};

#endif // VMR9OVERLAYGUEST_H