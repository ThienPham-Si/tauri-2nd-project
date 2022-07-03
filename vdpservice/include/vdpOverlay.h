/* ********************************************************************************* *
 * Copyright (C) 2011-2021 VMware, Inc.  All rights reserved. -- VMware Confidential *
 * ********************************************************************************* */

/*
 * VDPOverlay.h --
 *
 *    This header file is part of a public API and must be self contained.
 *    It can not contain any references to any non-public VMware header files.
 *
 *    This API will allow a client-side plugin to render to an area on the
 *    guest desktop window displayed on the client. The ultimate goal is to
 *    make it impossible to tell that the rendering is being done on the
 *    client and not on the guest.
 */

#ifndef VDPOVERLAY_H
#define VDPOVERLAY_H

#include "vmware.h"

#ifdef __cplusplus
extern "C" {
#endif


/*
 * VDPOverlay_WindowId --
 *
 *    Type used to refer to a native OS window
 *
 */
typedef uint32 VDPOverlay_WindowId;


/*
 * VDPOverlay_OverlayId --
 *
 *    Type used to refer to a local overlay which is an overlay
 *    created by the client that doesn't map to a window in the
 *    guest.  An Overlay ID can be used in any function that
 *    takes a Window ID but Window IDs can not be used as
 *    Overlay IDs.
 *
 */
typedef uint32 VDPOverlay_OverlayId;


/*
 * VDPOverlay_HWND --
 *
 * It's more efficient for Windows to work with its native HWND
 * but we need something platform independent for the other platforms
 */
#if defined(_WIN32) && !defined(VM_WIN_UWP)
   #define VDPOverlay_HWND HWND
#else
   #define VDPOverlay_HWND void*
#endif


/*
 * VDPOverlay_Rect --
 *
 * It's more efficient for Windows to work with its native RECT
 * but we need something platform independent for the other platforms
 */
#if defined(_WIN32) && !defined(VM_WIN_UWP)
   #define VDPOverlay_Rect RECT
#else
   #define VDPOverlay_Rect VMRect
#endif


/*
 * VDPOverlay_UserArgs --
 *
 *    Used in calls to
 *       VDPOverlayGuest_RegisterWindow()
 *       VDPOverlayGuest_UnregisterWindow()
 *       VDPOverlayGuest_EnableOverlay()
 *       VDPOverlayGuest_DisableOverlay()
 */
typedef uint32 VDPOverlay_UserArgs;


/*
 * VDP_OVERLAY_WINDOW_ID_NONE --
 *
 *    This value is defined to not refer to a valid overlay window.
 *    It can be used to initialize VDPOverlay_WindowId type
 *    variables to denote that they do not refer to a overlay window.
 */
#define VDP_OVERLAY_WINDOW_ID_NONE ((VDPOverlay_WindowId)0)


/*
 * VDP_OVERLAY_USER_MSG_MAX_LEN --
 *
 *    The maximum length of a message that can be sent
 *    with VDPOverlayGuest/Client_SendMsg().
 */
#define VDP_OVERLAY_USER_MSG_MAX_LEN 1024


/*
 * VDP_OVERLAY_INFO_STR_MAX_LEN --
 *
 *    The maximum length of the information string that can be
 *    used with VDPOverlayGuest/Client_SetInfoString().
 */
#define VDP_OVERLAY_INFO_STR_MAX_LEN 1024


/*
 * VDP_OVERLAY_COLORKEY_NONE --
 *
 *    A color key is normally an RGB color, in 24-bit 0xRRGGBB format.
 *    The invalid VDP_OVERLAY_COLORKEY_NONE value indicates that
 *    there is no color key.
 */
#define VDP_OVERLAY_COLORKEY_NONE  ((uint32) -1)


/*
 * VDP_OVERLAY_LAYER_DEFAULT --
 *
 *    The default layer value.  Overlays with a higher layer value
 *    will be on top of overlays with a lower layer value.
 */
 #define VDP_OVERLAY_LAYER_DEFAULT  0x10000


/*
 * VDPOverlay_LayoutMode --
 *
 *    Determines how the image is drawn in the overlay.
 *    The default is VDP_OVERLAY_LAYOUT_CENTER.
 */
typedef enum {
   /*
    * V1 - the image will be drawn centered in the overlay and
    * clipped to the size of the overlay.  No scaling is done.
    */
   VDP_OVERLAY_LAYOUT_CENTER,

   /*
    * V1 - the image will be drawn to fill the entire overlay.  No
    * attempt at maintaining the aspect ratio of the image is made.
    */
   VDP_OVERLAY_LAYOUT_SCALE,
   VDP_OVERLAY_LAYOUT_SCALE_SHRINK_ONLY,

   /*
    * V1 - the image will be scaled to fill the entire overlay while
    * maintaining the aspect ratio.  Parts of the image will be clipped
    * if necessary.
    */
   VDP_OVERLAY_LAYOUT_CROP,
   VDP_OVERLAY_LAYOUT_CROP_SHRINK_ONLY,

   /*
    * V1 - the image will be scaled such that either the width or height
    * of image will match the width/height of the overlay.  The other
    * dimension will be scaled to maintain the aspect ratio.  No part
    * of the image will be clipped but the image may not fill the entire
    * overlay.
    */
   VDP_OVERLAY_LAYOUT_LETTERBOX,
   VDP_OVERLAY_LAYOUT_LETTERBOX_SHRINK_ONLY,

   /*
    * V4 - the image will be tiled to fill the overlay.  The image is not
    * scaled but will be clipped on the right/bottom edges of the overlay.
    */
   VDP_OVERLAY_LAYOUT_TILE,

   /*
    * Mark the largest basic layout mode
    */
   VDP_OVERLAY_LAYOUT_MAX,

   /*
    * V1 - if the given image is smaller than the overlay, the image is
    * centered in the overlay.  If the image is larger, in either width
    * or height, the image is scaled accordingly.  These modes don't
    * provide new functionality but the new names better describe what
    * they do.
    */
   VDP_OVERLAY_LAYOUT_CLIP_OR_CENTER      = VDP_OVERLAY_LAYOUT_CENTER,
   VDP_OVERLAY_LAYOUT_CROP_OR_CENTER      = VDP_OVERLAY_LAYOUT_CROP_SHRINK_ONLY,
   VDP_OVERLAY_LAYOUT_SCALE_OR_CENTER     = VDP_OVERLAY_LAYOUT_SCALE_SHRINK_ONLY,
   VDP_OVERLAY_LAYOUT_LETTERBOX_OR_CENTER = VDP_OVERLAY_LAYOUT_LETTERBOX_SHRINK_ONLY,

   /*
    * V4 - MULTIPLE mode splits the overlay into 9 equal sized boxes (like
    * a tic-tac-toe board).  The image is then scaled to fit into the center
    * and corner boxes.  This mode can be combined with any of the basic
    * layout modes to determine how the image is scaled to fit in the box.
    * If after applying the layout mode the image doesn't fill the entire
    * box, the CENTER version places the image in the center of each box
    * and the CORNER version justifies the image within each box to the
    * nearest corner of the overlay.  For layout modes that always fill
    * the overlay (e.g. SCALE and TILE) CORNER and CENTER behave the same.
    */
   VDP_OVERLAY_LAYOUT_MULTIPLE_CENTER = 0x100,
   VDP_OVERLAY_LAYOUT_MULTIPLE_CORNER = 0x200,

} VDPOverlay_LayoutMode;


/*
 * VDP_OVERLAY_LAYOUT_TO_MULTIPLE --
 *
 *    Returns a layout mode from the given MULTIPLE and basic layout modes.
 *    The first parameter, multipleMode, must be either CENTER or CORNER.
 *    The second parameter, basicMode, must be one of the basic layout modes.
 *    e.g VDP_OVERLAY_LAYOUT_TO_MULTIPLE(CORNER, LETTERBOX_OR_CENTER)
 */
#define VDP_OVERLAY_LAYOUT_TO_MULTIPLE(multipleMode, basicMode)            \
   ((VDPOverlay_LayoutMode) (VDP_OVERLAY_LAYOUT_MULTIPLE_##multipleMode |  \
                             VDP_OVERLAY_LAYOUT_##basicMode))


/*
 * VDPOverlay_ImageFormat --
 *
 *    Defines the pixel format of an image passed to
 *    VDPOverlayClient_Interface.v2.Update().
 *
 *    Note that VDPOverlayClient_Interface.v1.Update()
 *    always assumes VDP_OVERLAY_BGRX formated images.
 */
typedef enum {
   VDP_OVERLAY_BGRX,

   /*
    * VDP_OVERLAY_BGRA expects the given image
    * to be pre-multiplied by the alpha channel
    */
   VDP_OVERLAY_BGRA,

   /*
    * VDP_OVERLAY_YV12 support was removed in View Client v5.2
    * Calling v2.Update() with this format will return an error
    */
   VDP_OVERLAY_YV12

} VDPOverlay_ImageFormat;


/*
 * VDP_OVERLAY_FORMAT_STR --
 *
 *    Returns a string description of a VDPOverlay_ImageFormat
 */
#define VDP_OVERLAY_FORMAT_STR(format)             \
         ((format) == VDP_OVERLAY_BGRX ? "BGRX" :  \
          (format) == VDP_OVERLAY_BGRA ? "BGRA" :  \
          (format) == VDP_OVERLAY_YV12 ? "YV12" : "FMTX")


/*
 * VDP_OVERLAY_FORMAT_IS_RGB --
 *
 *    Returns TRUE if the format is an RGB type
 */
#define VDP_OVERLAY_FORMAT_IS_RGB(format) \
   ((format) == VDP_OVERLAY_BGRX || (format) == VDP_OVERLAY_BGRA)


/*
 * VDP_OVERLAY_FORMAT_IS_YUV --
 *
 *    Returns TRUE if the format is an YUV type
 */
#define VDP_OVERLAY_FORMAT_IS_YUV(format) \
   ((format) == VDP_OVERLAY_YV12)


/*
 * VDP_OVERLAY_UPDATE_FLAG_* --
 *
 *    Flags that can be passed into VDPOverlayClient_Interface.v2.Update()
 *
 * VDP_OVERLAY_UPDATE_FLAG_NONE --
 *    This is just place holder denoting that no flags are being passed.
 */
#define VDP_OVERLAY_UPDATE_FLAG_NONE 0

/*
 * VDP_OVERLAY_UPDATE_FLAG_COPY_IMAGE --
 *    If set a copy of the image data is made, if FALSE no copy is
 *    made and the image data must remain valid until another call
 *    to Update() is made.
 */
#define VDP_OVERLAY_UPDATE_FLAG_COPY_IMAGE  (1 << 0)

/*
 * VDP_OVERLAY_UPDATE_FLAG_SHARED_SURFACE --
 *    If set the image pointer is really a DX shared surface handle.
 */
#define VDP_OVERLAY_UPDATE_FLAG_SHARED_SURFACE  (1 << 1)


/*
 * VDPOverlay_Error --
 *
 *    List of all possible error codes for the VDPOverlay library
 */
typedef enum {
   VDP_OVERLAY_ERROR_SUCCESS,
   VDP_OVERLAY_ERROR_NOT_INITIALIZED,
   VDP_OVERLAY_ERROR_ALREADY_INITIALIZED,
   VDP_OVERLAY_ERROR_INVALID_PARAMETER,
   VDP_OVERLAY_ERROR_ALLOCATION_ERROR,
   VDP_OVERLAY_ERROR_APPLICATION_ERROR,
   VDP_OVERLAY_ERROR_NO_MORE_OVERLAYS,
   VDP_OVERLAY_ERROR_OVERLAY_REJECTED,
   VDP_OVERLAY_ERROR_OVERLAY_NOT_READY,
   VDP_OVERLAY_ERROR_WINDOW_NOT_REGISTERED,
   VDP_OVERLAY_ERROR_WINDOW_ALREADY_REGISTERED,
   VDP_OVERLAY_ERROR_NOT_LOCAL_OVERLAY,
   VDP_OVERLAY_ERROR_HOST_OVERLAY_ERROR,
   VDP_OVERLAY_ERROR_NOT_SUPPORTED_BY_CLIENT,
   VDP_OVERLAY_ERROR_NOT_ALLOWED,
   VDP_OVERLAY_ERROR_MAX
} VDPOverlay_Error;



/************************************************************
 *
 * Guest side API
 *
 ************************************************************/

/*
 * VDPOverlayGuest_Sink --
 *
 *    This structure contains a set of event handers.  The version
 *    field and the function pointers are filled in by the caller
 *    before calling VDPOverlayGuest_Init().
 */
typedef struct {
   #define VDP_OVERLAY_GUEST_SINK_V1     1
   #define VDP_OVERLAY_GUEST_SINK_V2     2
   #define VDP_OVERLAY_GUEST_SINK_V3     3
   #define VDP_OVERLAY_GUEST_SINK_V4     4
   #define VDP_OVERLAY_GUEST_SINK_VERSON 4
   uint32 version;

   /*
    * VDP_OVERLAY_GUEST_SINK_V1
    */
   struct {
      /*
       * OnOverlayReady --
       *
       *    This event handler is called when the client-side overlay
       *    is ready to be displayed.  It does not mean that the overlay
       *    is enabled or even that the client-side has loaded an image
       *    into the overlay, just that the overlay was properly
       *    created and is ready to display an image.
       */
      void (*OnOverlayReady)(
               void* userData,               // IN - user data
               VDPOverlay_WindowId windowId, // IN - window ID
               uint32 response);             // IN - client-side plugin response


      /*
       * OnOverlayRejected --
       *
       *    This event handler is called when the client-side overlay was
       *    not created because the client side plug-in choose to reject it.
       *
       *    Note: the window that is associated with the overlay is
       *    automatically unregistered.
       */
      void (*OnOverlayRejected)(
               void* userData,               // IN - user data
               VDPOverlay_WindowId windowId, // IN - window ID
               uint32 reason);               // IN - client-side plugin reason for rejecting


      /*
       * OnOverlayCreateError --
       *
       *    This event handler is called when the client-side overlay was
       *    not created because of an error.
       *
       *    Note: the window that is associated with the overlay is
       *    automatically unregistered.
       */
      void (*OnOverlayCreateError)(
               void* userData,               // IN - user data
               VDPOverlay_WindowId windowId, // IN - window ID
               VDPOverlay_Error error);      // IN - client-side error encountered
                                             //         creating the overlay

      /*
       * OnUserMsg --
       *
       *    This event handler is called in response to a call to
       *    VDPOverlayClient_SendMsg() from the client.
       *
       */
      void (*OnUserMsg)(
               void* userData,               // IN - user data
               VDPOverlay_WindowId windowId, // IN - window ID that the message is sent to
                                             //       or VDP_OVERLAY_WINDOW_ID_NONE if the
                                             //       message wasn't sent to a particular window
               void* msg,                    // IN - message data.  Not valid once handler returns
               uint32 msgLen);               // IN - message length, in bytes
   } v1;

   /*
    * VDP_OVERLAY_GUEST_SINK_V2
    */
   struct {
      void* noChanges;
   } v2;

   /*
    * VDP_OVERLAY_GUEST_SINK_V3
    */
   struct {
      void* noChanges;
   } v3;

   /*
    * VDP_OVERLAY_GUEST_SINK_V4
    */
   struct {
      void* noChanges;
   } v4;
} VDPOverlayGuest_Sink;


/*
 * structure VDPOverlayGuest_Interface --
 *
 *    This is the structure returned when
 *       QueryInterface(GUID_VDPOverlay_GuestInterface_*) is called
 */
typedef struct {
   #define VDP_OVERLAY_GUEST_INTERFACE_V1      1
   #define VDP_OVERLAY_GUEST_INTERFACE_V2      2
   #define VDP_OVERLAY_GUEST_INTERFACE_V3      3
   #define VDP_OVERLAY_GUEST_INTERFACE_V4      4
   #define VDP_OVERLAY_GUEST_INTERFACE_VERSION 4
   uint32 version;

   /*
    * VDP_OVERLAY_GUEST_INTERFACE_V1
    */
   struct {
      /*
       * Init --
       *
       *    This function initializes the guest-side overlay library.
       *
       *    sink - contains the function pointers that are called
       *       when events are generated by the Overlay library.
       *
       *    userData - parameter that is passed to event handler whenever
       *       an event is delivered.
       */
      VDPOverlay_Error (*Init)(const VDPOverlayGuest_Sink* sink, // IN
                               void* userData);                  // IN


      /*
       * Exit --
       *
       *    Performs clean up operations; unregisters all windows
       *    and releases all allocated resources.
       */
      VDPOverlay_Error (*Exit)(void);


      /*
       * RegisterWindow --
       *
       *    Registers a window to be overlayed.  The position, size, etc. of the
       *    window will be sent to the client so that a client-side plug-in can
       *    draw to an area of the desktop UI that will cover the window giving
       *    the illusion that the drawing is happening on the guest-side.
       *
       *    windowId - the operating system window identifier.  A window can
       *       can only be registered once.
       *
       *    userArgs - parameter will be passed to the client-side plug-in when
       *       the OnWindowRegistered() event handler is called.
       */
      VDPOverlay_Error (*RegisterWindow)(VDPOverlay_WindowId windowId,  // IN
                                         VDPOverlay_UserArgs userArgs); // IN


      /*
       * UnregisterWindow --
       *
       *    Unregisters a previously registered window.  This will not only disable
       *    the client-side overlay, but also release any resources allocated to
       *    maintain the overlay.
       *
       *    windowId - the operating system window identifier.  The windowId must
       *       have been previously registered.
       *
       *    userArgs - parameter will be passed to the client-side plug-in when
       *       the VDPOverlayClient_WindowUnregistered event is sent.
       */
      VDPOverlay_Error (*UnregisterWindow)(VDPOverlay_WindowId windowId,  // IN
                                           VDPOverlay_UserArgs userArgs); // IN


      /*
       * IsWindowRegistered --
       *
       *    Checks if a window has been previously registered via
       *    VDPOverlayGuest_RegisterWindow().
       *
       *    windowId - an operating system window identifier.
       */
      Bool (*IsWindowRegistered)(VDPOverlay_WindowId windowId); // IN


      /*
       * EnableOverlay --
       *
       *    Enables the client-side overlay.  Once the window is registered, and
       *    ready, this function must be called to display the client-side overlay.
       *
       *    windowId - the operating system window identifier.  The windowId must
       *       have been previously registered.
       *
       *    userArgs - parameter will be passed to the client-side plug-in when
       *       the VDPOverlayClient_OverlayEnabled event is sent.
       */
      VDPOverlay_Error (*EnableOverlay)(VDPOverlay_WindowId windowId,  // IN
                                        VDPOverlay_UserArgs userArgs); // IN


      /*
       * DisableOverlay --
       *
       *    Disables the client-side overlay.  Disabling the overlay is a light-weight way
       *       to hide the client-side overlay.  Unlike VDPOverlayGuest_UnregisterWindow(),
       *       resources used to maintain the overlay are not released.
       *
       *    windowId - the operating system window identifier.  The windowId must
       *       have been previously registered.
       *
       *    userArgs - parameter will be passed to the client-side plug-in when
       *       the VDPOverlayClient_OverlayDisabled event is sent.
       */
      VDPOverlay_Error (*DisableOverlay)(VDPOverlay_WindowId windowId,  // IN
                                         VDPOverlay_UserArgs userArgs); // IN


      /*
       * IsOverlayEnabled --
       *
       *    Checks if the client-side overlay is enabled.  The overlay can be
       *    enabled and disabled by calling VDPOverlayGuest_EnableOverlay()
       *    and VDPOverlayGuest_DisableOverlay().
       *
       *    windowId - the operating system window identifier.  The windowId must
       *       have been previously registered.
       */
      Bool (*IsOverlayEnabled)(VDPOverlay_WindowId windowId); // IN


      /*
       * SetLayoutMode --
       *
       *    Sets the current layout mode for the overlay.  The layout mode is
       *    used to determine how an image is drawn (e.g. scaled, cropped, etc)
       *    when the size of the image doesn't match the size of the overlay.
       *
       *    windowId - the operating system window identifier.  The windowId must
       *       have been previously registered.
       *
       *    layoutMode - determines how the image is drawn.  This is one of the
       *       values in VDPOverlay_LayoutMode.
       */
      VDPOverlay_Error (*SetLayoutMode)(VDPOverlay_WindowId windowId,      // IN
                                        VDPOverlay_LayoutMode layoutMode); // IN


      /*
       * GetLayoutMode --
       *
       *    Gets the current layout mode for the overlay.  The layout mode is
       *    used to determine how an image is drawn (e.g. scaled, cropped, etc)
       *    when the size of the image doesn't match the size of the overlay.
       *
       *    windowId - the operating system window identifier.  The windowId must
       *       have been previously registered.
       *
       *    pLayoutMode - returns the current layout mode as set by SetLayoutMode().
       */
      VDPOverlay_Error (*GetLayoutMode)(VDPOverlay_WindowId windowId,         // IN
                                        VDPOverlay_LayoutMode* pLayoutMode);  // OUT


      /*
       * SendMsg --
       *
       *    Sends a message to the client.  The client's OnUserMsg() event handler
       *    will be called with the message.
       *
       *    windowId - the operating system window identifier.  The windowId must
       *       have been previously registered.
       *
       *       You may also pass VDP_OVERLAY_WINDOW_ID_NONE if the message isn't
       *       directed to a particular window.
       *
       *    msg - a pointer to a buffer that contains the message.
       *
       *    msgLen - the length of the message in bytes.  The maximum
       *       message length is VDP_OVERLAY_USER_MSG_MAX_LEN bytes.
       */
      VDPOverlay_Error (*SendMsg)(VDPOverlay_WindowId windowId, // IN
                                  void* msg,                    // IN
                                  uint32 msgLen);               // IN
   } v1;


   /*
    * VDP_OVERLAY_GUEST_INTERFACE_V2
    */
   struct {
      /*
       * GetColorkey --
       *
       *    Retrieve the colorkey currently assigned to the windowId.  This will
       *    be VDP_OVERLAY_COLORKEY_NONE until the windowId is assigned a
       *    colorkey by the overlay services.
       *
       *    windowId - the operating system window identifier.  The windowId must
       *       have been previously registered.
       *
       *       You may also pass VDP_OVERLAY_WINDOW_ID_NONE if the message isn't
       *       directed to a particular window.
       *
       *    pColorkey - a pointer to a uint32 that will return the colorkey.
       */
      VDPOverlay_Error (*GetColorkey)(VDPOverlay_WindowId windowId, // IN
                                      uint32* pColorkey);           // OUT
   } v2;


   /*
    * VDP_OVERLAY_GUEST_INTERFACE_V3
    */
   struct {
      /*
       * RegisterWindow --
       *
       *    Registers a window to be overlayed in the same manner as v1.RegisterWindow()
       *    but has a couple of key differences:
       *
       *       1) a window can be registered multiple times with the intent that
       *       different areas of the window be used to display the overlay image.
       *       Use SetAreaRect() to define the area, within the window, to display
       *       the overlay image.
       *
       *       2) the first parameter is an VDPOverlay_HWND not a VDPOverlay_WindowId.
       *       The size of a VDPOverlay_WindowId is 32-bits but on 64-bit Windows an
       *       HWND is 64 bits.  Defining the parameter as a VDPOverlay_HWND, which
       *       is defined as an HWND, removed the need to cast the HWND to a
       *       VDPOverlay_WindowId which guarantees that bits are not lost when casting.
       *
       *    hWnd - the operating system window identifier.
       *
       *    userArgs - parameter will be passed to the client-side plug-in when
       *       the OnWindowRegistered() event handler is called.
       *
       *    pWindowId - returns a VDPOverlay_WindowId to be used in the other
       *       VDPOverlayGuest_Interface API calls.
       */
      VDPOverlay_Error (*RegisterWindow)(VDPOverlay_HWND hWnd,             // IN
                                         VDPOverlay_UserArgs userArgs,     // IN
                                         VDPOverlay_WindowId* pWindowId);  // OUT


      /*
       * SetAreaRect --
       *
       *    Sets the area of window to display the overlay.
       *
       *    windowId - the operating system window identifier.  The windowId must
       *       have been previously registered.
       *
       *    pRect - A pointer to a VDPOverlay_Rect which defines the area of
       *       the window to display the overlay.  Passing NULL will remove the
       *       constraining area and the image will be displayed in the entire
       *       area of the window.
       */
      VDPOverlay_Error (*SetAreaRect)(VDPOverlay_WindowId windowId,  // IN
                                      VDPOverlay_Rect* pRect);       // IN


      /*
       * GetAreaRect --
       *
       *    Gets the current constraining area of the overlay.
       *
       *    windowId - the operating system window identifier.  The windowId must
       *       have been previously registered.
       *
       *    pRect - A pointer to a VDPOverlay_Rect which returns the area of the
       *       area of the window that is displaying the overlay.  An area of all
       *       0s means that the overlay doesn't have a constraining area set on it.
       */
      VDPOverlay_Error (*GetAreaRect)(VDPOverlay_WindowId windowId,  // IN
                                      VDPOverlay_Rect* pRect);       // OUT


      /*
       * SetLayer --
       *
       *    Sets the layer on an overlay.  When two overlays registered to the
       *    same window have overlapping area rectangles you can specify which
       *    overlay is on top by setting its layer.  Layers have no effect on
       *    overlays registered to different operating system windows.
       *
       *    windowId - the operating system window identifier.  The windowId must
       *       have been previously registered.
       *
       *    layer - the layer of the overlay.  Overlays with a higher layer
       *       value will be on top of overlays with a lower layer value.
       *       If two overlays have the same layer value the overlay created
       *       last will be on top.
       */
      VDPOverlay_Error (*SetLayer)(VDPOverlay_WindowId windowId,  // IN
                                   uint32 layer);                 // IN


      /*
       * GetLayer --
       *
       *    Gets the layer of an overlay as set by SetLayer().
       *
       *    windowId - the operating system window identifier.  The windowId must
       *       have been previously registered.
       *
       *    pLayer - returns the layer of the overlay.
       */
      VDPOverlay_Error (*GetLayer)(VDPOverlay_WindowId windowId,  // IN
                                   uint32* pLayer);               // IN
   } v3;


   /*
    * VDP_OVERLAY_GUEST_INTERFACE_V4
    */
   struct {
      /*
       * GetHWnd --
       *
       *    With v1.RegisterWindow() the HWND and windowId are the same but
       *    v3.RegisterWindow() returns a unique windowId.  This function
       *    provides a way to retrieving the original HWND used to register
       *    the window.
       *
       *    windowId - the operating system window identifier.  The windowId must
       *       have been previously registered.
       *
       *    pHWnd - a pointer to a VDPOverlay_HWND which returns the window
       *       handle used to register the window.
       */
      VDPOverlay_Error (*GetHWnd)(VDPOverlay_WindowId windowId,   // IN
                                  VDPOverlay_HWND* pHWnd);        // OUT


      /*
       * SetBackgroundColor --
       *
       *    Sets the background color to use when painting the area of the window
       *    that the overlay covers.  This color will be visible in the area of
       *    the overlay that the image does not cover; e.g. the borders of the
       *    image when the layout mode is LETTERBOX, or if the image has an
       *    alpha channel.
       *
       *    windowId - the operating system window identifier.  The windowId must
       *       have been previously registered.
       *
       *    bgColor - The color to use when painting the overlay in XXRRGGBB format.
       *       The alpha value in the color is ignored and set to 0xFF. Pass 0 to
       *       disable painting the background which allows the application to
       *       show through.
       */
      VDPOverlay_Error (*SetBackgroundColor)(VDPOverlay_WindowId windowId, // IN
                                             uint32 bgColor);              // IN


      /*
       * GetBackgroundColor --
       *
       *    Gets the current color used to paint the background of the overlay.
       *
       *    windowId - the operating system window identifier.  The windowId must
       *       have been previously registered.
       *
       *    pBgColor - a pointer to a uint32 which returns the color passed to
       *       SetBackgroundColor().
       */
      VDPOverlay_Error (*GetBackgroundColor)(VDPOverlay_WindowId windowId, // IN
                                             uint32* pBackgroundColor);    // OUT


      /*
       * SetAreaRect --
       *
       *    Sets the area of window to display the overlay.
       *
       *    windowId - the operating system window identifier.  The windowId must
       *       have been previously registered.
       *
       *    enabled - determines when the area rectangle is enabled.  If TRUE
       *       the overlay is constrained to the given rectangle.  If FALSE
       *       the overlay will be displayed in the entire area of the window.
       *
       *    clipToWindow - determines if the given area rectangle is clipped to
       *       the window.  Note: the image displayed in the overlay is always
       *       clipped to the window.
       *
       *       When this flag is TRUE, the given area rectangle is clipped to the
       *       window before the layout mode is applied which means that the image
       *       is scaled down to fit inside the clipped area rectangle.
       *
       *       When this flag is FALSE the layout mode is applied first and then
       *       the image is clipped to the window which means that image doesn't
       *       shrink when the bounds of the area rectangle extend past the bounds
       *       of the window, but less of the image is shown.
       *
       *    pRect - a pointer to a VDPOverlay_Rect which defines the area of
       *       the window to display the overlay.
       */
      VDPOverlay_Error (*SetAreaRect)(VDPOverlay_WindowId windowId,  // IN
                                      Bool enabled,                  // IN
                                      Bool clipToWindow,             // IN
                                      VDPOverlay_Rect* pRect);       // IN


      /*
       * GetAreaRect --
       *
       *    Gets the current constraining area of the overlay.
       *
       *    windowId - the operating system window identifier.  The windowId must
       *       have been previously registered.
       *
       *    pEnabled - a pointer to a Bool which returns the enabled flag passed
       *       to SetAreaRect().  Pass NULL to not return the value.
       *
       *    pClipToWindow - a pointer to a Bool which returns the clipToWindow
       *       flag passed to SetAreaRect().  Pass NULL to not return the value.
       *
       *    pRect - a pointer to a VDPOverlay_Rect which returns the rectangle
       *       passed to SetAreaRect().  Pass NULL to not return the value.
       */
      VDPOverlay_Error (*GetAreaRect)(VDPOverlay_WindowId windowId,  // IN
                                      Bool* pEnabled,                // OUT
                                      Bool* pClipToWindow,           // OUT
                                      VDPOverlay_Rect* pRect);       // OUT


      /*
       * SetInfoString --
       *
       *    Sets a string that is rendered on top of the overlay.  The string can
       *    contain arbitrary information which can useful for closed captioning
       *    or debugging information.
       *
       *    windowId - the operating system window identifier.  The windowId must
       *       have been previously registered.
       *
       *    infoStr - the information string.
       *       see VDPOverlayClient_Interface.v4.SetInfoString() for details.
       */
      VDPOverlay_Error (*SetInfoString)(VDPOverlay_WindowId windowId,   // IN
                                        const char* infoStr);           // IN


      /*
       * GetInfoString --
       *
       *    Gets the current information string for the overlay as set by
       *    SetInfoString()
       *
       *    windowId - the operating system window identifier.  The windowId must
       *       have been previously registered.
       *
       *    infoStr - a pointer to a buffer that is filled with the current
       *    information string.
       *
       *    infoStrSize - the size of infoStr buffer.
       */
      VDPOverlay_Error (*GetInfoString)(VDPOverlay_WindowId windowId,   // IN
                                        char* infoStr,                  // OUT
                                        int32 infoStrSize);             // IN
   } v4;
} VDPOverlayGuest_Interface;



/************************************************************
 *
 * Client side API
 *
 ************************************************************/

/*
 * VDPOverlayClient_ContextId --
 *
 *    This ID is returned from VDPOverlayClient_Init and
 *    is passed to all API calls.
 */
typedef uint32 VDPOverlayClient_ContextId;


/*
 * VDP_OVERLAY_CLIENT_CONTEXT_ID_NONE --
 *
 *    This value is defined to not refer to a valid client context.
 *    It can be used to initialize VDPOverlayClient_ContextId type
 *    variables to denote that they do not refer to a valid context.
 */
#define VDP_OVERLAY_CLIENT_CONTEXT_ID_NONE ((VDPOverlayClient_ContextId)0)


/*
 * VDPOverlayClient_OverlayInfo --
 *
 *    This structure is used in the call to VDPOverlayClient::GetInfo().
 *
 *    In V1 the first member of VDPOverlayClient_OverlayInfo was cbSize
 *    which was set by the caller to determine the version of the struct.
 *    But doing that wasn't backward compatible; e.g. a program written
 *    to V2 would return an error if it called GetInfo() because the
 *    size wouldn't be set correctly.
 *
 *    Starting with V2 the first member of VDPOverlayClient_OverlayInfo
 *    is a version and is set by GetInfo() to the version of the function
 *    that filled the structure.  For backward compatibility when calling
 *    v1.GetInfo() the caller needs to set version = VDP_OVERLAY_INFO_V1_SIZE
 *    before calling v1.GetInfo().
 */
typedef struct {
   #define VDP_OVERLAY_INFO_V1   1
   #define VDP_OVERLAY_INFO_V2   2
   #define VDP_OVERLAY_INFO_V1_SIZE    offsetof(VDPOverlayClient_OverlayInfo, v2)
   uint32 version;                     // Returns which version of GetInfo() filled the structure

   struct {
      VDPOverlay_WindowId windowId;    // The window ID which will match the window ID
                                       //    passed to VDPOverlayClient_GetInfo()

      int32 xUI, yUI;                  // The position of the overlay on the guest UI desktop

      int32 width, height;             // The size of the overlay

      Bool enabled;                    // The enabled status of the overlay

      Bool visible;                    // The visible status of the overlay

      VDPOverlay_LayoutMode layoutMode; // The current layout mode as set by
                                        //   VDPOverlayGuest::SetLayoutMode()
   } v1;

   struct {
      VDPOverlay_ImageFormat imageFormat; // The pixel format fo the image

      uint32 colorkey;                    // The colorkey used by the overlay

      uint32 layer;                       // The layer used by the overlay

      Bool hasClipRegion;                 // TRUE if there is a clip region set on the overlay
   } v2;

} VDPOverlayClient_OverlayInfo;


/*
 * VDPOverlayClient_InfoStringProperties --
 *
 *    This structure is used in the call to
 *       to VDPOverlayClient::Get/SetInfoStringProperties().
 *
 */
typedef struct {
   // V1 .. V3 do not support this structure
   #define VDP_OVERLAY_INFO_STRING_PROPERTIES_V4 4
   uint32 version;

   struct {
      Bool enabled;           // Enables/disables the information string

      uint32 fgColor;         // The foreground/text color to render the information string
      uint32 bgColor;         // The background color to render the information string
                              // 0 uses the default foreground/background colors

     int32 xBox;              // Distance between the background and the edge of the overlay
     int32 yBox;              // Positive numbers position the background on the top/left
                              // Negative numbers position the background on the bottom/right
                              // 0 uses the default margin

     int32 wBox;              // Defines the size of background
     int32 hBox;              // Positive numbers denote the maximum size of the background;
                              //    the text will be scaled to fit in this size using a
                              //    LETTERBOX_SHRINK_ONLY layout mode.
                              // Negative numbers are an absolute size for the background;
                              //    the text will be scaled to fit in this size using a
                              //    LETTERBOX layout mode.
                              // 0 means to size the background to the size of the text.
   } v4;

} VDPOverlayClient_InfoStringProperties;


/*
 * VDPOverlayClient_Sink --
 *
 *    This structure contains a set of event handers.  The version
 *    field and the function pointers are filled in by the caller
 *    before calling VDPOverlayClient_Init().
 */
typedef struct {
   #define VDP_OVERLAY_CLIENT_SINK_V1      1
   #define VDP_OVERLAY_CLIENT_SINK_V2      2
   #define VDP_OVERLAY_CLIENT_SINK_V3      3
   #define VDP_OVERLAY_CLIENT_SINK_V4      4
   #define VDP_OVERLAY_CLIENT_SINK_VERSION 4
   uint32 version;

   /*
    * VDP_OVERLAY_CLIENT_SINK_V1
    */
   struct {
      /*
       * OnWindowRegistered --
       *
       *    This event handler is called when the guest-side registers
       *    a window using VDPOverlayGuest_RegisterWindow().
       *
       *    The event handler can reject the request to create the overlay
       *    by setting "reject" to TRUE.  "response" can be used to return
       *    return a reason back to the guest.
       *
       *    If the event handler doesn't reject the request to create the
       *    overlay it can send a response back to the guest by setting
       *    "response".
       *
       *    The window ID passed to the event handler should be cached
       *    because it is needed to identify the window/overlay to the
       *    overlay API.
       */
      void (*OnWindowRegistered)(
               void* userData,                        // IN - user data that was passed
                                                      //       to VDPOverlayClient_Init()
               VDPOverlayClient_ContextId contextId,  // IN - the plugin's context ID which was
                                                      //       returned from VDPOverlayClient_Init()
               VDPOverlay_WindowId windowId,          // IN - window ID
               VDPOverlay_UserArgs userArgs,          // IN - value sent by the guest side in the
                                                      //       call to VDPOverlayGuest_RegisterWindow()
               Bool* reject,                          // OUT - set to TRUE to reject the overlay
               uint32* response);                     // OUT - the response sent back to the guest


      /*
       * OnWindowUnregistered --
       *
       *    This event handler is called when the guest-side unregisters a window.
       *    The window ID is no longer valid and the overlay associated with the
       *    window ID is destroyed.
       */
      void (*OnWindowUnregistered)(
               void* userData,                        // IN - user data
               VDPOverlayClient_ContextId contextId,  // IN - context ID
               VDPOverlay_WindowId windowId,          // IN - window ID
               VDPOverlay_UserArgs userArgs);         // IN - value sent by the guest side in the
                                                      //       call to VDPOverlayGuest_UnregisterWindow()

      /*
       * OnOverlayEnabled --
       *
       *    This event handler is called when the guest-side enables the overlay.
       *    This causes the current image in the overlay to be displayed.
       */
      void (*OnOverlayEnabled)(
               void* userData,                        // IN - user data
               VDPOverlayClient_ContextId contextId,  // IN - context ID
               VDPOverlay_WindowId windowId,          // IN - window ID
               VDPOverlay_UserArgs userArgs);         // IN - value sent by the guest side in the
                                                      //       call to VDPOverlayGuest_EnableOverlay()

      /*
       * OnOverlayDisabled --
       *
       *    This event handler is called when the guest-side disables the overlay.
       *    This causes the current image in the overlay to be hidden.  The overlay
       *    image data is maintained and will be re-displayed when the overlay is
       *    re-enabled.
       */
      void (*OnOverlayDisabled)(
               void* userData,                        // IN - user data
               VDPOverlayClient_ContextId contextId,  // IN - context ID
               VDPOverlay_WindowId windowId,          // IN - window ID
               VDPOverlay_UserArgs userArgs);         // IN - value sent by the guest side in the
                                                      //       call to VDPOverlayGuest_DisableOverlay()

      /*
       * OnWindowPositionChanged --
       *
       *    This event handler is called when the guest-side window which the
       *    overlay is tracking changes position.  The overlay will be drawn
       *    at the new location.  This is for information only, no action is
       *    required by the plugin.
       */
      void (*OnWindowPositionChanged)(
               void* userData,                        // IN - user data
               VDPOverlayClient_ContextId contextId,  // IN - context ID
               VDPOverlay_WindowId windowId,          // IN - window ID
               int32 x, int32 y);                     // IN - the new position


      /*
       * OnWindowSizeChanged --
       *
       *    This event handler is called when the guest-side window which the
       *    overlay is tracking changes size.  The old overlay image will be
       *    redrawn according to the layout mode of the overlay.  This is for
       *    information only, no action is required by the plugin.
       */
      void (*OnWindowSizeChanged)(
               void* userData,                        // IN - user data
               VDPOverlayClient_ContextId contextId,  // IN - context ID
               VDPOverlay_WindowId windowId,          // IN - window ID
               int32 width, int32 height);            // IN - the new size


      /*
       * OnWindowObscured --
       *
       *    This event handler is called when the guest-side window which the
       *    overlay is tracking is completely obscured.  The client-side can
       *    use this as a hint to scale back drawing to the overlay.
       */
      void (*OnWindowObscured)(
               void* userData,                        // IN - user data
               VDPOverlayClient_ContextId contextId,  // IN - context ID
               VDPOverlay_WindowId windowId);         // IN - window ID


      /*
       * OnWindowVisible --
       *
       *    This event handler is called when the guest-side window which the
       *    overlay is tracking was obscured but now is at least partially visible.
       */
      void (*OnWindowVisible)(
               void* userData,                        // IN - user data
               VDPOverlayClient_ContextId contextId,  // IN - context ID
               VDPOverlay_WindowId windowId);         // IN - window ID


      /*
       * OnLayoutModeChanged --
       *
       *    This event handler is called when the layout mode for the overlay
       *    is changed.  This is for information only, no action is required
       *    by the plugin.
       */
      void (*OnLayoutModeChanged)(
               void* userData,                        // IN - user data
               VDPOverlayClient_ContextId contextId,  // IN - context ID
               VDPOverlay_WindowId windowId,          // IN - window ID
               VDPOverlay_LayoutMode layoutMode);     // IN - the new layout mode


      /*
       * OnUserMsg --
       *
       *    This event handler is called in response to a call to
       *    VDPOverlayGuest_SendMsg() from the guest.
       *
       */
      void (*OnUserMsg)(
               void* userData,                        // IN - user data
               VDPOverlayClient_ContextId contextId,  // IN - context ID
               VDPOverlay_WindowId windowId,          // IN - window ID that the message is sent to
                                                      //       or VDP_OVERLAY_WINDOW_ID_NONE if the
                                                      //       message wasn't sent to a particular window
               void* msg,                             // IN - message data.  Not valid once handler returns
               uint32 msgLen);                        // IN - message length, in bytes
   } v1;

   /*
    * VDP_OVERLAY_CLIENT_SINK_V2
    */
   struct {
      void* noChanges;
   } v2;

   /*
    * VDP_OVERLAY_CLIENT_SINK_V3
    */
   struct {
      /*
       * OnTopologyChanged --
       *
       *    This event handler is called when the desktop topology of the
       *    View client has changed.  The desktopTopology array is only
       *    valid during the callback.  This is for information only, no
       *    action is required by the plugin.
       */
      void (*OnTopologyChanged)(
               void* userData,                        // IN - user data
               VDPOverlayClient_ContextId contextId,  // IN - context ID
               const VDPOverlay_Rect* desktopBounds,  // IN - the desktop bounding box
               int32 szDesktopTopology,               // IN - the size of the desktopTopology array
               const VDPOverlay_Rect* desktopTopology); // IN - the desktop topology

      /*
       * OnLayerChanged --
       *
       *    This event handler is called when the layer for the overlay is
       *    changed.  This is for information only, no action is required
       *    by the plugin.
       */
      void (*OnLayerChanged)(
               void* userData,                        // IN - user data
               VDPOverlayClient_ContextId contextId,  // IN - context ID
               VDPOverlay_WindowId windowId,          // IN - window ID
               uint32 layer);                         // IN - the new layer
   } v3;

   /*
    * VDP_OVERLAY_CLIENT_SINK_V4
    */
   struct {
      void* noChanges;
   } v4;
} VDPOverlayClient_Sink;


/*
 * structure VDPOverlayClient_Interface --
 *
 *    This is the structure returned when
 *       QueryInterface(GUID_VDPOverlay_ClientInterface_*) is called
 */
typedef struct {
   #define VDP_OVERLAY_CLIENT_INTERFACE_V1      1
   #define VDP_OVERLAY_CLIENT_INTERFACE_V2      2
   #define VDP_OVERLAY_CLIENT_INTERFACE_V3      3
   #define VDP_OVERLAY_CLIENT_INTERFACE_V4      4
   #define VDP_OVERLAY_CLIENT_INTERFACE_VERSION 4
   uint32 version;

   /*
    * VDP_OVERLAY_CLIENT_INTERFACE_V1
    */
   struct {
      /*
       * Init --
       *
       *    This function initializes the client-side overlay library.
       *
       *    sink - contains the function pointers that are called
       *       when events are generated by the Overlay library.
       *
       *    userData - parameter that is passed to event handler whenever
       *       an event is delivered.
       *
       *    pContextId - returns an ID that is used to identify the instance
       *       of the API.  This ID must be passed to all other API functions.
       *       This ID is also passed when calling the sink handlers.
       */
      VDPOverlay_Error (*Init)(const VDPOverlayClient_Sink* sink,       // IN
                               void* userData,                          // IN
                               VDPOverlayClient_ContextId* pContextId); // OUT


      /*
       * Exit --
       *
       *    Performs clean up operations and releases all allocated resources.
       *
       *    contextId - the ID returned from VDPOverlayClient_Init().
       */
      VDPOverlay_Error (*Exit)(VDPOverlayClient_ContextId contextId); // IN


      /*
       * Update --
       *
       *    Updates the overlay with a new image.  The updated image is displayed
       *    when the next frame is drawn.
       *
       *    contextId - the ID returned from VDPOverlayClient_Init().
       *
       *    windowId - a window ID that was cached from a
       *       previous OnWindowRegistered() event.
       *
       *    pImage - a pointer to the BGRX pixels to copy to the overlay.
       *
       *    width, height - the width,height of the image, in pixels, pointed
       *       to by pImage.  If the width/height of the image does not match
       *       the width/height of overlay the given image is drawn according
       *       the layout mode of the overlay.
       *
       *    pitch - the number of bytes that a single row of image occupies.
       *       In the normal case, for BGRX images, this value will be width*4.
       *
       *    copyImage - if TRUE a copy of the image data is made, if FALSE no
       *       copy is made and the image data must remain valid until another
       *       call to VDPOverlayClient_Update() is made.
       */
      VDPOverlay_Error (*Update)(VDPOverlayClient_ContextId contextId, // IN
                                 VDPOverlay_WindowId windowId,         // IN
                                 void* pImage,                         // IN
                                 int32 width,                          // IN
                                 int32 height,                         // IN
                                 int32 pitch,                          // IN
                                 Bool copyImage);                      // IN


      /*
       * GetInfo --
       *
       *    Retrieves current information about the overlay
       *
       *    contextId - the ID returned from VDPOverlayClient_Init().
       *
       *    windowId - a window ID that was cached from a
       *       previous OnWindowRegistered() event.
       *
       *    pOverlayInfo - a pointer to a VDPOverlayClient_OverlayInfo
       *       structure which will be filled in with information about the overlay.
       */
      VDPOverlay_Error (*GetInfo)(VDPOverlayClient_ContextId contextId,        // IN
                                  VDPOverlay_WindowId windowId,                // IN
                                  VDPOverlayClient_OverlayInfo* pOverlayInfo); // OUT


      /*
       * SendMsg --
       *
       *    Sends a message to the guest.  The guests's OnUserMsg()
       *    event handler will be called with the message.
       *
       *    contextId - the ID returned from VDPOverlayClient_Init().
       *
       *    windowId - a window ID that was cached from a previous
       *       OnWindowRegistered() event. You may also pass
       *       VDP_OVERLAY_WINDOW_ID_NONE if the message isn't
       *       directed to a particular window.
       *
       *    msg - a pointer to a buffer that contains the message.
       *
       *    msgLen - the length of the message in bytes.  The maximum
       *       message length is VDP_OVERLAY_USER_MSG_MAX_LEN bytes.
       */
      VDPOverlay_Error (*SendMsg)(VDPOverlayClient_ContextId contextId, // IN
                                  VDPOverlay_WindowId windowId,         // IN
                                  void* msg,                            // IN
                                  uint32 msgLen);                       // IN
   } v1;


   /*
    * VDP_OVERLAY_CLIENT_INTERFACE_V2
    */
   struct {
      /*
       * InitLocal --
       *
       *    This function initializes the client-side overlay library
       *    for use with just local overlays.  The overhead of creating
       *    an RPC connection to track guest side windows is not performed.
       *    You do not need to call this function if you've already called
       *    v1.Init().
       *
       *    sink - contains the function pointers that are called
       *       when events are generated by the Overlay library.
       *
       *    userData - parameter that is passed to event handler whenever
       *       an event is delivered.
       *
       *    pContextId - returns an ID that is used to identify the instance
       *       of the API.  This ID must be passed to all other API functions.
       *       This ID is also passed when calling the sink handlers.
       */
      VDPOverlay_Error (*InitLocal)(const VDPOverlayClient_Sink* sink,        // IN
                                    void* userData,                           // IN
                                    VDPOverlayClient_ContextId* pContextId);  // OUT

      /*
       * CreateOverlay --
       *
       *    Creates a local overlay.  The overlay is not tied to a window
       *    on the guest (such an overlay is referred to as a "guest created
       *    overlay").  Locally created overlays give the client complete
       *    control over the overlay but also require the client to do more
       *    of the work.
       *
       *    contextId - the ID returned from VDPOverlayClient_Init().
       *
       *    pOverlayId - returns a VDPOverlay_OverlayId that can be used to set
       *       properties on the overlay.  This ID may also be passed to functions
       *       that take a VDPOverlay_WindowId (e.g. Update(), GetInfo(), etc.).
       */
      VDPOverlay_Error (*CreateOverlay)(VDPOverlayClient_ContextId contextId,  // IN
                                        VDPOverlay_OverlayId* pOverlayId);     // OUT


      /*
       * DestroyOverlay --
       *
       *    Destroys a local overlay.  All of the resources associated with the
       *    overlay are released.  This function can not be called on guest
       *    created overlays.
       *
       *    contextId - the ID returned from VDPOverlayClient_Init().
       *
       *    overlayId - an overlay ID that was returned from a previous
       *       call to CreateOverlay().
       */
      VDPOverlay_Error (*DestroyOverlay)(VDPOverlayClient_ContextId contextId, // IN
                                         VDPOverlay_OverlayId overlayId);      // IN


      /*
       * SetPosition --
       *
       *    Sets the position of a local overlay.  This function can not
       *    be called on guest created overlays.
       *
       *    contextId - the ID returned from VDPOverlayClient_Init().
       *
       *    overlayId - an overlay ID that was returned from a previous
       *       call to CreateOverlay().
       *
       *    x,y - the position of the overlay.  The position is specified as
       *    the upper-left corner of the overlay in guest UI coordinates.
       */
      VDPOverlay_Error (*SetPosition)(VDPOverlayClient_ContextId contextId,   // IN
                                      VDPOverlay_OverlayId overlayId,         // IN
                                      int32 x,                                // IN
                                      int32 y);                               // IN


      /*
       * SetSize --
       *
       *    Sets the size of a local overlay.  This function can not be
       *    called on guest created overlays.
       *
       *    contextId - the ID returned from VDPOverlayClient_Init().
       *
       *    overlayId - an overlay ID that was returned from a previous
       *       call to CreateOverlay().
       *
       *    width,height - the size of the overlay in pixels.  If the size
       *       of the image specified in Update() does not match the size of
       *       the overlay the image is drawn as specified by the layout mode.
       */
      VDPOverlay_Error (*SetSize)(VDPOverlayClient_ContextId contextId, // IN
                                  VDPOverlay_OverlayId overlayId,       // IN
                                  int32 width,                          // IN
                                  int32 height);                        // IN


      /*
       * SetClipRegion --
       *
       *    Sets the a clipping region on the overlay.  This function can not be
       *    called on guest created overlays.
       *
       *    contextId - the ID returned from VDPOverlayClient_Init().
       *
       *    overlayId - an overlay ID that was returned from a previous
       *       call to CreateOverlay().
       *
       *    pClipRects - an array of VMRect's that describe the visible area
       *       of the overlay.  The clipping information is relative to the
       *       screen; e.g. 0,0 is the top-left corner of the screen.  This
       *       means that the clipping information describes a specific area
       *       of the screen that does not change when the overlay is moved.
       *       A copy of the VMRect array is made so that the caller doesn't
       *       have to maintain the memory.
       *
       *    nClipRects - the number of VMRect's in the pClipRects array.
       *
       *    Note: Passing an empty region makes the overlay completely non-visible.
       *          Passing nClipRects == 0 removes the clip region.
       */
      VDPOverlay_Error (*SetClipRegion)(VDPOverlayClient_ContextId contextId, // IN
                                        VDPOverlay_OverlayId overlayId,       // IN
                                        VMRect* pClipRects,                   // IN
                                        int32 nClipRects);                    // IN


      /*
       * SetLayer --
       *
       *    Sets the layer on a local overlay.  This function can not be called
       *    on guest created overlays.
       *
       *    contextId - the ID returned from VDPOverlayClient_Init().
       *
       *    overlayId - an overlay ID that was returned from a previous
       *       call to CreateOverlay().
       *
       *    layer - the layer of the overlay.  Overlays will a higher layer
       *       value will be on top of overlays with a lower layer value.
       *       If two overlays have the same layer value the overlay created
       *       last will be on top.
       */
      VDPOverlay_Error (*SetLayer)(VDPOverlayClient_ContextId contextId,   // IN
                                   VDPOverlay_OverlayId overlayId,         // IN
                                   uint32 layer);                          // IN


      /*
       * SetColorkey --
       *
       *    Sets the color key on a local overlay.  This function can not be called
       *    on guest created overlays.
       *
       *    contextId - the ID returned from VDPOverlayClient_Init().
       *
       *    overlayId - an overlay ID that was returned from a previous
       *       call to CreateOverlay().
       *
       *    colorkey - an RGB value that will limit the area of the guest UI where
       *       the overlay is drawn.  When a color key is set on an overlay only the
       *       pixels on the guest's UI that match the color key value will be updated.
       *       It is the caller's responsibility to draw the color key to an area on the
       *       guest's desktop that corresponds to the position of the overlay as set
       *       by SetPosition().  Passing VDP_OVERLAY_COLORKEY_NONE will remove the
       *       color key from the overlay.
       */
      VDPOverlay_Error (*SetColorkey)(VDPOverlayClient_ContextId contextId,   // IN
                                      VDPOverlay_OverlayId overlayId,         // IN
                                      uint32 colorkey);                       // IN


      /*
       * EnableOverlay --
       *
       *    Enables an overlay that has been previously disabled.  An overlay can
       *    can be disabled if either the guest or client calls DisableOverlay()
       *    for the given window ID.
       *
       *    contextId - the ID returned from VDPOverlayClient_Init().
       *
       *    windowId - a window ID that was cached from a
       *       previous OnWindowRegistered() event.
       */
      VDPOverlay_Error (*EnableOverlay)(VDPOverlayClient_ContextId contextId,  // IN
                                        VDPOverlay_WindowId windowId);         // IN


      /*
       * DisableOverlay --
       *
       *    Disables an overlay.  Disabling an overlay is a light-weight way to
       *    hide an overlay.  Unlike DestroyOverlay(), resources used to maintain
       *    the overlay are not released.
       *
       *    contextId - the ID returned from VDPOverlayClient_Init().
       *
       *    windowId - a window ID that was cached from a
       *       previous OnWindowRegistered() event.
       */
      VDPOverlay_Error (*DisableOverlay)(VDPOverlayClient_ContextId contextId,  // IN
                                         VDPOverlay_WindowId windowId);         // IN


      /*
       * SetLayoutMode --
       *
       *    Sets the current layout mode for the overlay.  The layout mode is
       *    used to determine how an image is drawn (e.g. scaled, cropped, etc)
       *    when the size of the image doesn't match the size of the overlay.
       *
       *    contextId - the ID returned from VDPOverlayClient_Init().
       *
       *    windowId - a window ID that was cached from a
       *       previous OnWindowRegistered() event.
       *
       *    layoutMode - determines how the image is drawn.  This is one of the
       *       values in VDPOverlay_LayoutMode.
       */
      VDPOverlay_Error (*SetLayoutMode)(VDPOverlayClient_ContextId contextId,  // IN
                                        VDPOverlay_WindowId windowId,          // IN
                                        VDPOverlay_LayoutMode layoutMode);     // IN


      /*
       * Update --
       *
       *    Updates the overlay with a new image.  The updated image is displayed
       *    when the next frame is drawn.
       *
       *    contextId - the ID returned from VDPOverlayClient_Init().
       *
       *    windowId - a window ID that was cached from a
       *       previous OnWindowRegistered() event.
       *
       *    pImage - a pointer to the pixels to copy to the overlay.  pImage may
       *       also be a handle to a DirectX shared surface if flag is set.
       *
       *    width, height - the width,height of the image, in pixels, pointed
       *       to by pImage.  If the width/height of the image does not match
       *       the width/height of overlay the given image is drawn according
       *       the layout mode of the overlay.
       *
       *    pitch - the number of bytes that a single row of image occupies.
       *       In the normal case, for BGRX images, this value will be width*4.
       *
       *    format - the pixel format of the image.  This is one of the values
       *       in VDPOverlay_ImageFormat.
       *
       *    flags - VDP_OVERLAY_UPDATE_FLAG_*  See above for more details.
       */
      VDPOverlay_Error (*Update)(VDPOverlayClient_ContextId contextId, // IN
                                 VDPOverlay_WindowId windowId,         // IN
                                 void* pImage,                         // IN
                                 int32 width,                          // IN
                                 int32 height,                         // IN
                                 int32 pitch,                          // IN
                                 VDPOverlay_ImageFormat format,        // IN
                                 uint32 flags);                        // IN


      /*
       * GetInfo --
       *
       *    Retrieves current information about the overlay
       *
       *    contextId - the ID returned from VDPOverlayClient_Init().
       *
       *    windowId - a window ID that was cached from a
       *       previous OnWindowRegistered() event.
       *
       *    pOverlayInfo - a pointer to a VDPOverlayClient_OverlayInfo
       *       structure which will be filled in with information about the overlay.
       */
      VDPOverlay_Error (*GetInfo)(VDPOverlayClient_ContextId contextId,        // IN
                                  VDPOverlay_WindowId windowId,                // IN
                                  VDPOverlayClient_OverlayInfo* pOverlayInfo); // OUT
   } v2;


   /*
    * VDP_OVERLAY_CLIENT_INTERFACE_V3
    */
   struct {
      /*
       * GetTopology --
       *
       *    Retrieves the topology of the View desktop.
       *
       *    contextId - the ID returned from VDPOverlayClient_Init().
       *
       *    desktopBounds - returns a rectangle which contains the bounding box
       *       for the entire View desktop.  Pass NULL to avoid returning the
       *       information.
       *
       *    pszDesktopTopology - a pointer to an int32.  On input the value is
       *       the size of the desktopTopology array which will return the View
       *       desktop topology.  On output the value is the number of rectangles
       *       required to hold the entire desktop topology; which may be larger
       *       than the size passed in if the desktopTopology array is too small
       *       to hold the entire desktop topology.  Passing NULL is treated the
       *       same as *pszDesktopTopology == 0.
       *
       *    desktopTopology - a pointer to an array which returns the rectangles
       *       that make up the the View desktop topology.  Can be NULL only if
       *       pszDesktopTopology is NULL or *pszDesktopTopology == 0.
       */
      VDPOverlay_Error (*GetTopology)(VDPOverlayClient_ContextId contextId,   // IN
                                      VDPOverlay_Rect* desktopBounds,         // OUT
                                      int32* pszDesktopTopology,              // IN/OUT
                                      VDPOverlay_Rect* desktopTopology);      // OUT
   } v3;


   /*
    * VDP_OVERLAY_CLIENT_INTERFACE_V4
    */
   struct {
      /*
       * SetInfoString --
       *
       *    Sets a string that is rendered on top of the overlay.  The string can
       *    contain arbitrary information which can useful for closed captioning
       *    or debugging information.
       *
       *    contextId - the ID returned from VDPOverlayClient_Init().
       *
       *    windowId - a window ID that was cached from a
       *       previous OnWindowRegistered() event.
       *
       *    infoStr - the information string.  The maximum string length is
       *       VDP_OVERLAY_INFO_STR_MAX_LEN bytes, including the NULL terminator.
       *
       *    The information string can contain the following macros
       *       $(FPS)               - FPS or image format (default)
       *       $(IMAGE_SIZE)        - Source image size (WxH)
       *       $(IMAGE_FORMAT)      - Source image format
       *       $(OVERLAY_ID)        - Overlay ID
       *       $(OVERLAY_POS)       - Overlay position (X,Y)
       *       $(OVERLAY_SIZE)      - Overlay size (WxH)
       *       $(OVERLAY_LAYER)     - Overlay layer
       *       $(OVERLAY_LAYOUT)    - Overlay layout mode
       *       $(OVERLAY_SURFACE)   - Overlay surface type
       *       $(OVERLAY_FPS)       - Overlay frame rate
       *       $(OVERLAY_FRAME_NUM) - Overlay frame number
       *       $(VIEW_FPS)          - View client frame rate
       *       $(VIEW_FRAME_NUM)    - View client frame number
       *       $(VIEW_WINDOW_SIZE)  - View client window size
       *       $(VIEW_PROTOCOL)     - View client protocol (Blast/PCoIP)
       *       $(TIME)              - Current time
       *       $(DATE)              - Current date
       *
       *    The following escape characters are recognized.  This assumes that the
       *    string is read from a file or the registry.  If you are hardcoding the
       *    informaton string in C/C++ code then the backslash character itself
       *    needs to be escaped.
       *       \n    - new line; the LF character ('\n' in C/C++) is also a new line
       *       \$    - dollar sign
       *       \\    - backslash
       */
      VDPOverlay_Error (*SetInfoString)(VDPOverlayClient_ContextId contextId, // IN
                                        VDPOverlay_WindowId windowId,         // IN
                                        const char* infoStr);                 // IN


      /*
       * GetInfoString --
       *
       *    Gets the current information string for the overlay as set by
       *    SetInfoString()
       *
       *    contextId - the ID returned from VDPOverlayClient_Init().
       *
       *    windowId - a window ID that was cached from a
       *       previous OnWindowRegistered() event.
       *
       *    infoStr - a pointer to a buffer that is filled with the current
       *    information string.
       *
       *    infoStrSize - the size of infoStr buffer.
       */
      VDPOverlay_Error (*GetInfoString)(VDPOverlayClient_ContextId contextId, // IN
                                        VDPOverlay_WindowId windowId,         // IN
                                        char* infoStr,                        // OUT
                                        int32 infoStrSize);                   // IN


      /*
       * SetInfoStringProperties --
       *
       *    Sets properties for how the information string is rendered.
       *
       *    contextId - the ID returned from VDPOverlayClient_Init().
       *
       *    windowId - a window ID that was cached from a
       *       previous OnWindowRegistered() event.
       *
       *    pProperties - a pointer to a VDPOverlayClient_InfoStringProperties structure.
       *       It's best to initialize the structure by calling GetInfoStringProperties()
       *       if there are properties that you don't want to change.
       */
      VDPOverlay_Error (*SetInfoStringProperties)(
                              VDPOverlayClient_ContextId contextId,                 // IN
                              VDPOverlay_WindowId windowId,                         // IN
                              VDPOverlayClient_InfoStringProperties *pProperties);  // IN


      /*
       * GetInfoStringProperties --
       *
       *    Gets properties for how the information string is rendered.
       *
       *    contextId - the ID returned from VDPOverlayClient_Init().
       *
       *    windowId - a window ID that was cached from a
       *       previous OnWindowRegistered() event.
       *
       *    pProperties - a pointer to a VDPOverlayClient_InfoStringProperties structure.
       */
      VDPOverlay_Error (*GetInfoStringProperties)(
                              VDPOverlayClient_ContextId contextId,                 // IN
                              VDPOverlay_WindowId windowId,                         // IN
                              VDPOverlayClient_InfoStringProperties *pProperties);  // OUT
   } v4;
} VDPOverlayClient_Interface;


#ifdef __cplusplus
}
#endif

#endif /* VDPOVERLAY_H */
