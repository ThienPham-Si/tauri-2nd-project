/* ********************************************************************************* *
 * Copyright (C) 2011-2021 VMware, Inc.  All rights reserved. -- VMware Confidential *
 * ********************************************************************************* */

/*
 * VMR9OverlayInterface.cpp --
 *
 */

#include "stdafx.h"
#include "VMR9OverlayInterface.h"

#define VMR9_OVERLAY_VALIDATE(rValue, minVersion)                    \
                                                                     \
   if (!m_overlayInit) {                                             \
      LOG("VMR9OverlayPlugin not initialized");                      \
      return rValue;                                                 \
   }                                                                 \
                                                                     \
   if (!OverlayGuestInterface()->v1.IsWindowRegistered(windowId)) {  \
      LOG("WindowId 0x%x not registered", windowId);                 \
      return rValue;                                                 \
   }                                                                 \
                                                                     \
   OverlayWindow* overlayWindow = WindowIdToOverlay(windowId);       \
   if (overlayWindow == NULL) {                                      \
      LOG("WindowId 0x%x not found", windowId);                      \
      return rValue;                                                 \
   }                                                                 \
                                                                     \
   uint32 version = OverlayGuestInterface()->version;                \
   if (version < VDP_OVERLAY_GUEST_INTERFACE_##minVersion) {         \
      LOG("VDP_OVERLAY_GUEST_INTERFACE_##minVersion required"        \
          " (current is V%d)", version);                             \
      return rValue;                                                 \
   }                                                                 \




/*
 *----------------------------------------------------------------------
 *
 * Class VMR9OverlayPlugin
 *
 *----------------------------------------------------------------------
 */
VMR9OverlayPlugin::VMR9OverlayPlugin(RPCManager *rpcManagerPtr)
   : m_overlayInit(false),
     RPCPluginInstance(rpcManagerPtr)
{
}

VMR9OverlayPlugin::~VMR9OverlayPlugin()
{
}


/*
 *----------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::Init --
 *
 * Results:
 *    Returns TRUE if all went well.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
bool
VMR9OverlayPlugin::Init(int32 msTimeout)
{
   #ifdef _DEBUG
      ::MessageBox(NULL, _T("VMR9OverlayGuest"), _T("Waiting for a debugger"), MB_OK);
   #endif

   RPCManager *vmr9OverlayRPCMgr = GetRPCManager();
   if (vmr9OverlayRPCMgr == NULL) {
      return false;
   }

   /*
    * Make sure I'm not already initialized
    */
   if (m_overlayInit) {
      LOG("VMR9OverlayPlugin is already initialized");
      return false;
   }

   /*
    * Initialize the RPC Manager in server mode
    */
   if (!vmr9OverlayRPCMgr->ServerInit(this, msTimeout)) {
      LOG("InitServer() failed");
      return false;
   }

   /*
    * Initialize the Overlay API
    */
   VDPOverlayGuest_Sink guestSink = {
      VDP_OVERLAY_GUEST_SINK_V1,
      {
         OnOverlayReady,
         OnOverlayRejected,
         OnOverlayCreateError,
         NULL
      }
   };

   VDPOverlay_Error err =
      OverlayGuestInterface()->v1.Init(&guestSink, (void*)this);

   if (err != VDP_OVERLAY_ERROR_SUCCESS) {
      LOG("OverlayGuest.v1.Init() failed");
      Exit();
      return false;
   }

   m_overlayInit = true;
   return true;
}


/*
 *----------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::Exit --
 *
 * Results:
 *    Returns TRUE if all went well.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
bool
VMR9OverlayPlugin::Exit()
{
   bool ok = true;
   RPCManager *vmr9OverlayRPCMgr = GetRPCManager();

   if (vmr9OverlayRPCMgr == NULL) {
      return false;
   }

   if (m_overlayInit) {
      VDPOverlay_Error err =
         OverlayGuestInterface()->v1.Exit();

      if (err != VDP_OVERLAY_ERROR_SUCCESS) {
         LOG("OverlayGuest.v1.Exit() failed");
         ok = false;
      }

      m_overlayInit = false;
   }

   if (!vmr9OverlayRPCMgr->ServerExit(this)) {
      LOG("ServerExit() failed");
      ok = false;
   }

   return ok;
}


/*
 *----------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::CreateOverlay --
 *
 * Results:
 *    Returns an VDPOverlay_WindowId that
 *    can be used with the other methods.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
VDPOverlay_WindowId
VMR9OverlayPlugin::CreateOverlay(HWND hWnd)
{
   VDPOverlay_WindowId windowId;
   VDPOverlay_UserArgs userArgs = 0;

   if (!m_overlayInit) {
      LOG("VMR9OverlayPlugin not initialized");
      return VDP_OVERLAY_WINDOW_ID_NONE;
   }

   uint32 version = OverlayGuestInterface()->version;
   if (version < VDP_OVERLAY_GUEST_INTERFACE_V3) {
      LOG("VDP_OVERLAY_GUEST_INTERFACE_V3 required (current is V%d)", version);
      return VDP_OVERLAY_WINDOW_ID_NONE;
   }

   VDPOverlay_Error err =
      OverlayGuestInterface()->v3.RegisterWindow(hWnd, userArgs, &windowId);

   if (err != VDP_OVERLAY_ERROR_SUCCESS) {
      LOG("OverlayGuest.v3.RegisterWindow(0x%p) failed", hWnd);
      return VDP_OVERLAY_WINDOW_ID_NONE;
   }

   LOG("OverlayGuest.v3.RegisterWindow(0x%p) -> 0x%x [OK]", hWnd, windowId);
   m_overlayWindows[windowId].m_windowId = windowId;
   m_overlayWindows[windowId].m_hWnd = hWnd;

   return windowId;
}


/*
 *----------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::DestroyOverlay --
 *
 * Results:
 *    Returns TRUE if all went well.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
bool
VMR9OverlayPlugin::DestroyOverlay(VDPOverlay_WindowId windowId)
{
   VMR9_OVERLAY_VALIDATE(false, V1);
   VDPOverlay_UserArgs userArgs = 0;

   VDPOverlay_Error err =
      OverlayGuestInterface()->v1.UnregisterWindow(windowId, userArgs);

   if (err != VDP_OVERLAY_ERROR_SUCCESS) {
      LOG("OverlayGuest.v1.UnregisterWindow(0x%x) failed", windowId);
      return false;
   }

   LOG("OverlayGuest.v1.UnregisterWindow(0x%x) [OK]", windowId);
   m_overlayWindows.erase(windowId);

   return true;
}


/*
 *----------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::OpenFile --
 *
 * Results:
 *    Returns TRUE if all went well.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
bool
VMR9OverlayPlugin::OpenFile(VDPOverlay_WindowId windowId,
                            const std::string& path)
{
   VMR9_OVERLAY_VALIDATE(false, V1);

   const VDPRPC_ChannelContextInterface* iChannelCtx = ChannelContextInterface();
   const VDPRPC_VariantInterface* iVariant = VariantInterface();

   void* messageCtx = NULL;
   if (!CreateMessage(&messageCtx)) {
      LOG("CreateMessage() failed");
      return false;
   }

   iChannelCtx->v1.SetCommand(messageCtx, VMR9_OVERLAY_FILE_OPEN);

   VDP_RPC_VARIANT var;
   iVariant->v1.VariantInit(&var);
   iVariant->v1.VariantFromInt32(&var, windowId);
   iChannelCtx->v1.AppendParam(messageCtx, &var);

   iVariant->v1.VariantFromStr(&var, path.c_str());
   iChannelCtx->v1.AppendParam(messageCtx, &var);

   if (!InvokeMessage(messageCtx)) {
      LOG("InvokeMessage() failed");
      DestroyMessage(messageCtx);
      return false;
   }

   return true;
}


/*
 *----------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::Close --
 *
 * Results:
 *    Returns TRUE if all went well.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
bool
VMR9OverlayPlugin::CloseFile(VDPOverlay_WindowId windowId)
{
   VMR9_OVERLAY_VALIDATE(false, V1);

   const VDPRPC_ChannelContextInterface* iChannelCtx = ChannelContextInterface();
   const VDPRPC_VariantInterface* iVariant = VariantInterface();

   void* messageCtx = NULL;
   if (!CreateMessage(&messageCtx)) {
      LOG("CreateMessage() failed");
      return false;
   }

   iChannelCtx->v1.SetCommand(messageCtx, VMR9_OVERLAY_FILE_CLOSE);

   VDP_RPC_VARIANT var;
   iVariant->v1.VariantInit(&var);
   iVariant->v1.VariantFromInt32(&var, windowId);
   iChannelCtx->v1.AppendParam(messageCtx, &var);

   if (!InvokeMessage(messageCtx)) {
      LOG("InvokeMessage() failed");
      DestroyMessage(messageCtx);
      return false;
   }

   return true;
}


/*
 *----------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::StartVideo --
 *
 * Results:
 *    Returns TRUE if all went well.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
bool
VMR9OverlayPlugin::StartVideo(VDPOverlay_WindowId windowId)
{
   VMR9_OVERLAY_VALIDATE(false, V1);

   const VDPRPC_ChannelContextInterface* iChannelCtx = ChannelContextInterface();
   const VDPRPC_VariantInterface* iVariant = VariantInterface();

   void* messageCtx = NULL;
   if (!CreateMessage(&messageCtx)) {
      LOG("CreateMessage() failed");
      return false;
   }

   iChannelCtx->v1.SetCommand(messageCtx, VMR9_OVERLAY_PLAYBACK_START);

   VDP_RPC_VARIANT var;
   iVariant->v1.VariantInit(&var);
   iVariant->v1.VariantFromInt32(&var, windowId);
   iChannelCtx->v1.AppendParam(messageCtx, &var);

   if (!InvokeMessage(messageCtx)) {
      LOG("InvokeMessage() failed");
      DestroyMessage(messageCtx);
      return false;
   }

   return true;
}


/*
 *----------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::StopVideo --
 *
 * Results:
 *    Returns TRUE if all went well.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
bool
VMR9OverlayPlugin::StopVideo(VDPOverlay_WindowId windowId)
{
   VMR9_OVERLAY_VALIDATE(false, V1);

   const VDPRPC_ChannelContextInterface* iChannelCtx = ChannelContextInterface();
   const VDPRPC_VariantInterface* iVariant = VariantInterface();

   void* messageCtx = NULL;
   if (!CreateMessage(&messageCtx)) {
      LOG("CreateMessage() failed");
      return false;
   }

   iChannelCtx->v1.SetCommand(messageCtx, VMR9_OVERLAY_PLAYBACK_STOP);

   VDP_RPC_VARIANT var;
   iVariant->v1.VariantInit(&var);
   iVariant->v1.VariantFromInt32(&var, windowId);
   iChannelCtx->v1.AppendParam(messageCtx, &var);

   if (!InvokeMessage(messageCtx)) {
      LOG("InvokeMessage() failed");
      DestroyMessage(messageCtx);
      return false;
   }

   return true;
}


/*
 *----------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::EnableOverlay --
 *
 * Results:
 *    Returns TRUE if all went well.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
bool
VMR9OverlayPlugin::EnableOverlay(VDPOverlay_WindowId windowId,
                                 bool enabled)
{
   VMR9_OVERLAY_VALIDATE(false, V1);
   VDPOverlay_UserArgs userArgs = 1;

   VDPOverlay_Error err = enabled
      ? OverlayGuestInterface()->v1.EnableOverlay(windowId, userArgs)
      : OverlayGuestInterface()->v1.DisableOverlay(windowId, userArgs);

   if (err != VDP_OVERLAY_ERROR_SUCCESS) {
      LOG("OverlayGuest.v1.%sableOverlay(0x%x) failed",
                                    (enabled?"En":"Dis"), windowId);
      return false;
   }

   return true;
}


/*
 *----------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::IsOverlayEnabled --
 *
 * Results:
 *    Returns TRUE if the given overlay is enabled.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
bool
VMR9OverlayPlugin::IsOverlayEnabled(VDPOverlay_WindowId windowId)
{
   VMR9_OVERLAY_VALIDATE(false, V1);

   return OverlayGuestInterface()->v1.IsOverlayEnabled(windowId) != 0;
}


/*
 *----------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::CopyImages --
 *
 * Results:
 *    Returns TRUE if all went well.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
bool
VMR9OverlayPlugin::CopyImages(VDPOverlay_WindowId windowId,
                              bool copyImages)
{
   VMR9_OVERLAY_VALIDATE(false, V1);

   const VDPRPC_ChannelContextInterface* iChannelCtx = ChannelContextInterface();
   const VDPRPC_VariantInterface* iVariant = VariantInterface();

   void* messageCtx = NULL;
   if (!CreateMessage(&messageCtx)) {
      LOG("CreateMessage() failed");
      return false;
   }

   iChannelCtx->v1.SetCommand(messageCtx, VMR9_OVERLAY_COPY_IMAGES);

   VDP_RPC_VARIANT var;
   iVariant->v1.VariantInit(&var);
   iVariant->v1.VariantFromInt32(&var, windowId);
   iChannelCtx->v1.AppendParam(messageCtx, &var);
   iVariant->v1.VariantFromChar(&var, copyImages);
   iChannelCtx->v1.AppendParam(messageCtx, &var);

   if (!InvokeMessage(messageCtx)) {
      LOG("InvokeMessage() failed");
      DestroyMessage(messageCtx);
      return false;
   }

   overlayWindow->m_copyImages = copyImages;
   return true;
}


/*
 *----------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::AreImagesCopied --
 *
 * Results:
 *    Returns TRUE if images are being copied.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
bool
VMR9OverlayPlugin::AreImagesCopied(VDPOverlay_WindowId windowId)
{
   VMR9_OVERLAY_VALIDATE(false, V1);

   return overlayWindow->m_copyImages;
}


/*
 *----------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::SetLayoutMode --
 *
 * Results:
 *    Returns TRUE if all went well.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
bool
VMR9OverlayPlugin::SetLayoutMode(VDPOverlay_WindowId windowId,
                                 VDPOverlay_LayoutMode layoutMode)
{
   VMR9_OVERLAY_VALIDATE(false, V1);

   VDPOverlay_Error err =
      OverlayGuestInterface()->v1.SetLayoutMode(windowId, layoutMode);

   if (err != VDP_OVERLAY_ERROR_SUCCESS) {
      LOG("OverlayGuest.v1.SetLayoutMode(0x%x) failed", windowId);
      return false;
   }

   return true;
}


/*
 *----------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::GetLayoutMode --
 *
 * Results:
 *    The current layout mode or VDP_OVERLAY_LAYOUT_MAX is there is an error.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
VDPOverlay_LayoutMode
VMR9OverlayPlugin::GetLayoutMode(VDPOverlay_WindowId windowId)
{
   VMR9_OVERLAY_VALIDATE(VDP_OVERLAY_LAYOUT_MAX, V1);
   VDPOverlay_LayoutMode layoutMode;

   VDPOverlay_Error err =
      OverlayGuestInterface()->v1.GetLayoutMode(windowId, &layoutMode);

   if (err != VDP_OVERLAY_ERROR_SUCCESS) {
      LOG("OverlayGuest.v1.GetLayoutMode(0x%x) failed", windowId);
      return VDP_OVERLAY_LAYOUT_MAX;
   }

   return layoutMode;
}


/*
 *----------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::SetBackgroundColor --
 *
 * Results:
 *    Returns TRUE if all went well.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
bool
VMR9OverlayPlugin::SetBackgroundColor(VDPOverlay_WindowId windowId,
                                      uint32 bgColor)
{
   VMR9_OVERLAY_VALIDATE(false, V4);

   VDPOverlay_Error err =
      OverlayGuestInterface()->v4.SetBackgroundColor(windowId, bgColor);

   if (err != VDP_OVERLAY_ERROR_SUCCESS) {
      LOG("OverlayGuest.v4.SetBackgroundColor(0x%x) failed", windowId);
      return false;
   }

   return true;
}


/*
 *----------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::GetBackgroundColor --
 *
 * Results:
 *    Returns the background color of the overlay window.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
uint32
VMR9OverlayPlugin::GetBackgroundColor(VDPOverlay_WindowId windowId)
{
   VMR9_OVERLAY_VALIDATE(0, V4);

   uint32 bgColor = 0;
   VDPOverlay_Error err =
      OverlayGuestInterface()->v4.GetBackgroundColor(windowId, &bgColor);

   if (err != VDP_OVERLAY_ERROR_SUCCESS) {
      LOG("OverlayGuest.v4.GetBackgroundColor(0x%x) failed", windowId);
      return 0;
   }

   return bgColor;
}


/*
 *----------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::SetAreaRect --
 *
 * Results:
 *    Returns TRUE if all went well.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
bool
VMR9OverlayPlugin::SetAreaRect(VDPOverlay_WindowId windowId,
                               bool enabled,
                               bool clipped,
                               RECT& areaRect)
{
   VMR9_OVERLAY_VALIDATE(false, V4);

   VDPOverlay_Error err =
      OverlayGuestInterface()->v4.SetAreaRect(windowId, enabled, clipped, &areaRect);

   if (err != VDP_OVERLAY_ERROR_SUCCESS) {
      LOG("OverlayGuest.v4.SetAreaRect(0x%x) failed", windowId);
      return false;
   }

   return true;
}


/*
 *----------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::GetAreaRect --
 *
 * Results:
 *    Returns TRUE if all went well.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
bool
VMR9OverlayPlugin::GetAreaRect(VDPOverlay_WindowId windowId,
                               bool* pEnabled,
                               bool* pClipped,
                               RECT* pAreaRect)
{
   VMR9_OVERLAY_VALIDATE(false, V4);

   VDPOverlay_Error err =
      OverlayGuestInterface()->v4.GetAreaRect(windowId,
                                              (Bool*)pEnabled,
                                              (Bool*)pClipped,
                                              pAreaRect);
   if (err != VDP_OVERLAY_ERROR_SUCCESS) {
      LOG("OverlayGuest.v4.GetAreaRect(0x%x) failed", windowId);
      return false;
   }

   return true;
}


/*
 *----------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::GetVideoPath
 *
 * Results:
 *    Returns the name of the video that the client loaded
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
std::string
VMR9OverlayPlugin::GetVideoPath(VDPOverlay_WindowId windowId)
{
   VMR9_OVERLAY_VALIDATE(std::string(""), V1);

   return overlayWindow->m_videoPath.c_str();
}


/*
 *----------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::GetVideoSize
 *
 * Results:
 *    Returns TRUE if all went well.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
bool
VMR9OverlayPlugin::GetVideoSize(VDPOverlay_WindowId windowId,
                                POINT* pVideoSize)
{
   VMR9_OVERLAY_VALIDATE(false, V1);

   if (pVideoSize == NULL) {
      LOG("pVideoSize == NULL");
      return false;
   }

   *pVideoSize = overlayWindow->m_videoSize;
   return true;
}


/*
 *----------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::OnOverlayReady --
 *
 *----------------------------------------------------------------------
 */
void
VMR9OverlayPlugin::OnOverlayReady(void* userData,
                                  VDPOverlay_WindowId windowId,
                                  uint32 response)
{
   VMR9OverlayPlugin* vmr9OverlayPlugin =
      static_cast< VMR9OverlayPlugin* >(userData);

   LOG("Window 0x%x is ready (response = %d)", windowId, response);

   vmr9OverlayPlugin->OnOverlayReady(windowId);
}


/*
 *----------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::OnOverlayRejected --
 *
 *----------------------------------------------------------------------
 */
void
VMR9OverlayPlugin::OnOverlayRejected(void* userData,
                                     VDPOverlay_WindowId windowId,
                                     uint32 reason)
{
   VMR9OverlayPlugin* vmr9OverlayPlugin =
      static_cast< VMR9OverlayPlugin* >(userData);

   /*
    * The window is automatically unregistered
    */
   vmr9OverlayPlugin->m_overlayWindows.erase(windowId);
   LOG("Window 0x%x registration rejected (reason = %d)", windowId, reason);

   vmr9OverlayPlugin->OnOverlayRejected(windowId);
}


/*
 *----------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::OnOverlayCreateError --
 *
 *----------------------------------------------------------------------
 */
void
VMR9OverlayPlugin::OnOverlayCreateError(void* userData,
                                        VDPOverlay_WindowId windowId,
                                        VDPOverlay_Error error)
{
   VMR9OverlayPlugin* vmr9OverlayPlugin =
      static_cast< VMR9OverlayPlugin* >(userData);

   /*
    * The window is automatically unregistered
    */
   vmr9OverlayPlugin->m_overlayWindows.erase(windowId);
   LOG("Window 0x%x registration failed (error = %d)", windowId, error);

   vmr9OverlayPlugin->OnOverlayCreateError(windowId);
}


/*
 *----------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::OnInvoke --
 *
 *----------------------------------------------------------------------
 */
void
VMR9OverlayPlugin::OnInvoke(void* messageCtx)
{
   const VDPRPC_ChannelContextInterface* iChannelCtx = ChannelContextInterface();
   const VDPRPC_VariantInterface* iVariant = VariantInterface();

   VDP_RPC_VARIANT var;
   iVariant->v1.VariantInit(&var);

   FUNCTION_TRACE_MSG("%d", iChannelCtx->v1.GetCommand(messageCtx));
   switch (iChannelCtx->v1.GetCommand(messageCtx))
   {
   case VMR9_OVERLAY_SET_SIZE: {
      iChannelCtx->v1.GetParam(messageCtx, 0, &var);
      OverlayWindow* overlayWindow = WindowIdToOverlay(var.lVal);

      if (overlayWindow == NULL) {
         LOG("WindowId 0x%x is not registered", var.lVal);

      } else {
         iChannelCtx->v1.GetParam(messageCtx, 1, &var);
         overlayWindow->m_videoSize.x = var.lVal;

         iChannelCtx->v1.GetParam(messageCtx, 2, &var);
         overlayWindow->m_videoSize.y = var.lVal;

         if (iChannelCtx->v1.GetParam(messageCtx, 3, &var)) {
            overlayWindow->m_videoPath = var.strVal;
         } else {
            overlayWindow->m_videoPath = "";
         }

         OnVideoReady(overlayWindow->m_windowId);

         FUNCTION_EXIT_MSG("OnVideoReady(): \"%s\" %d x %d",
             overlayWindow->m_videoPath.c_str(),
             overlayWindow->m_videoSize.x,
             overlayWindow->m_videoSize.y);
      }
      break;
     }

   default:
      LOG("Unknown command");
      break;
   }
}
