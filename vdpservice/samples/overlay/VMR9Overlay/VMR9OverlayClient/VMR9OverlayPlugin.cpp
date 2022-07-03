/* ********************************************************************************* *
 * Copyright (C) 2011-2021 VMware, Inc.  All rights reserved. -- VMware Confidential *
 * ********************************************************************************* */

/*
 * VMR9OverlayPlugin.cpp --
 *
 */

#include "stdafx.h"
#include "VMR9OverlayPlugin.h"


/*
 *----------------------------------------------------------------------
 *
 * Class VMR9OverlayManager --
 *
 *    A single RPCManager object must be declared in global
 *    scope so that the the constructor is executed before
 *    VDPService_PluginGetTokenName() is called.
 *
 *----------------------------------------------------------------------
 */
class VMR9OverlayManager : public RPCManager
{
public:
   VMR9OverlayManager() : RPCManager(VMR9_OVERLAY_TOKEN_NAME) { }
   virtual RPCPluginInstance* OnCreateInstance() { return new VMR9OverlayPlugin(this); }
};

VMR9OverlayManager vmr9OverlayManager;

VDP_SERVICE_CREATE_INTERFACE(VMR9_OVERLAY_TOKEN_NAME, vmr9OverlayManager)

/*
 *----------------------------------------------------------------------------
 *
 * Class VMR9OverlayPlugin --
 *
 *----------------------------------------------------------------------------
 */
VMR9OverlayPlugin::PluginContexts VMR9OverlayPlugin::s_pluginContexts;

VMR9OverlayPlugin::VMR9OverlayPlugin(RPCManager *rpcManagerPtr)
   : m_contextId(VDP_OVERLAY_CLIENT_CONTEXT_ID_NONE),
     RPCPluginInstance(rpcManagerPtr)
{
   FUNCTION_TRACE;
   CreatePlugin();
}

VMR9OverlayPlugin::~VMR9OverlayPlugin()
{
   FUNCTION_TRACE;
   DestroyPlugin();
}


/*
 *----------------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::GetPlugin --
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------------
 */
bool
VMR9OverlayPlugin::GetPlugin(VDPOverlayClient_ContextId contextId,   // IN
                             VMR9OverlayPlugin** pPluginContext)     // OUT
{
   PluginContexts::iterator it = s_pluginContexts.find(contextId);
   if (it == s_pluginContexts.end()) {
      return false;
   }

   if (pPluginContext != NULL) {
      *pPluginContext = it->second;
   }

   return true;
}


/*
 *----------------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::CreatePlugin --
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------------
 */
bool
VMR9OverlayPlugin::CreatePlugin()
{
   FUNCTION_TRACE;

   VDPOverlayClient_Sink clientSink = {
      VDP_OVERLAY_CLIENT_SINK_V1,
      {
         OnWindowRegistered,
         OnWindowUnregistered,
         OnOverlayEnabled,
         OnOverlayDisabled,
         OnWindowPositionChanged,
         OnWindowSizeChanged,
         OnWindowObscured,
         OnWindowVisible,
         OnLayoutModeChanged,
         NULL
      }
   };


   /*
    * Initialize the OverlayClient API
    */
   VDPOverlayClient_ContextId contextId;

   VDPOverlay_Error err =
      OverlayClientInterface()->v1.Init(&clientSink,
                                        (void*)this,
                                        &contextId);

   if (err != VDP_OVERLAY_ERROR_SUCCESS) {
      FUNCTION_EXIT_MSG("OverlayClient.v1.Init() failed");
      return false;
   }


   /*
    * Set the context ID on the plugin
    */
   m_contextId = contextId;
   s_pluginContexts[contextId] = this;

   /*
    * Initialize the other member variables
    */
   m_visible = true;
   m_started = false;

   /*
    * Load an image to put into the overlay
    * while a video is not playing
    */
   WCHAR bgFilename[1024] = L"";
   HMODULE hModule = GetModuleHandleA("VMR9OverlayClient.dll");
   GetModuleFileNameW(hModule, bgFilename, ARRAYSIZE(bgFilename));
   WCHAR* ext = wcsrchr(bgFilename, L'\\');
   if (ext != NULL) {
      size_t pathLen = ++ext - bgFilename;
      size_t len = ARRAYSIZE(bgFilename) - pathLen;
      wcscpy_s(ext, len, L"VMR9Overlay.img");
      m_bgImage.Load(bgFilename, 0xFF);
   }

   FUNCTION_EXIT_MSG("Plugin%d - Created", contextId);
   return true;
}


/*
 *----------------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::DestroyPlugin --
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------------
 */
bool
VMR9OverlayPlugin::DestroyPlugin(void)
{
   FUNCTION_TRACE_MSG("Plugin%d", m_contextId);

   PluginContexts::iterator itPlugin = s_pluginContexts.find(m_contextId);
   if (itPlugin == s_pluginContexts.end()) {
      FUNCTION_EXIT_MSG("Plugin%d - not found", m_contextId);
      return false;
   }

   PlayerMap playersCopy = m_players;
   PlayerMap::iterator itPlayer = playersCopy.begin();
   while (itPlayer != playersCopy.end()) {
      DestroyPlayer(itPlayer++->first);
   }

   if (m_contextId != VDP_OVERLAY_CLIENT_CONTEXT_ID_NONE) {
      LOG("Plugin%d - Calling OverlayClient.v1.Exit()", m_contextId);
      OverlayClientInterface()->v1.Exit(m_contextId);
   }

   s_pluginContexts.erase(itPlugin);

   FUNCTION_EXIT_MSG("Plugin%d - Destroyed", m_contextId);
   m_contextId = VDP_OVERLAY_CLIENT_CONTEXT_ID_NONE;
   return true;
}


/*
 *----------------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::GetPlayer --
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------------
 */
bool
VMR9OverlayPlugin::GetPlayer(VDPOverlay_WindowId windowId,     // IN
                             OverlayPlayer** pOverlayPlayer)   // IN
{
   PlayerMap::iterator it = m_players.find(windowId);
   if (it == m_players.end()) {
      return false;
   }

   if (pOverlayPlayer != NULL) {
      *pOverlayPlayer = it->second;
   }
   return true;
}


/*
 *----------------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::CreatePlayer --
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------------
 */
bool
VMR9OverlayPlugin::CreatePlayer(VDPOverlay_WindowId windowId)  // IN
{
   FUNCTION_TRACE_MSG("Window 0x%x", windowId);
   DestroyPlayer(windowId);

   m_players[windowId] =
         new OverlayPlayer((HINSTANCE)g_hModule, m_contextId, windowId);

   FUNCTION_EXIT_MSG("Plugin%d - Created player for window 0x%x", m_contextId, windowId);
   return true;
}


/*
 *----------------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::DestroyPlayer --
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------------
 */
bool
VMR9OverlayPlugin::DestroyPlayer(VDPOverlay_WindowId windowId)  // IN
{
   FUNCTION_TRACE_MSG("Window 0x%x", windowId);

   OverlayPlayer* overlayPlayer = NULL;
   if (!GetPlayer(windowId, &overlayPlayer)) {
      FUNCTION_EXIT_MSG("Plugin%d - Can't find player for window 0x%x", m_contextId, windowId);
      return false;
   }

   bool ok = overlayPlayer->StopThread();
   m_players.erase(windowId);
   delete overlayPlayer;

   FUNCTION_EXIT_MSG("Plugin%d - Destroyed player for window 0x%x", m_contextId, windowId);
   return ok;
}


/*
 *----------------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::OpenPlayer --
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------------
 */
bool
VMR9OverlayPlugin::OpenPlayer(VDPOverlay_WindowId windowId, BSTR path) // IN
{
   FUNCTION_TRACE_MSG("Window 0x%x  path %ls", windowId, LOG_WSTR(path));

   OverlayPlayer* overlayPlayer = NULL;
   if (!GetPlayer(windowId, &overlayPlayer)) {
      FUNCTION_EXIT_MSG("Plugin%d - Can't find player for window 0x%x", m_contextId, windowId);
      return false;
   }

   if (path != NULL && *path != L'\0' &&
       ::GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES) {
      path =  SysAllocString(path);
   } else {
      path = OverlayPlayer::GetMoviePath();
   }

   if (path == NULL) {
      FUNCTION_EXIT_MSG("Plugin%d - path == NULL", m_contextId);
      return false;
   }

   if (!ClosePlayer(windowId)) {
      FUNCTION_EXIT_MSG("Plugin%d - ClosePlayer() failed", m_contextId);
      return false;
   }
   m_started = false;

   overlayPlayer->SetMoviePath(path);
   if (!overlayPlayer->StartThread()) {
      FUNCTION_EXIT_MSG("Plugin%d - StartThread() failed", m_contextId);
      return false;
   }
   m_started = true;

   return true;
}


/*
 *----------------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::ClosePlayer --
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------------
 */
bool
VMR9OverlayPlugin::ClosePlayer(VDPOverlay_WindowId windowId)   // IN
{
   FUNCTION_TRACE_MSG("Window 0x%x", windowId);

   OverlayPlayer* overlayPlayer = NULL;
   if (!GetPlayer(windowId, &overlayPlayer)) {
      FUNCTION_EXIT_MSG("Plugin%d - Can't find player for window 0x%x", m_contextId, windowId);
      return false;
   }

   if (!overlayPlayer->StopThread()) {
      FUNCTION_EXIT_MSG("Plugin%d - StopThread() failed", m_contextId);
      return false;
   }

   return true;
}


/*
 *----------------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::StartPlayer --
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------------
 */
bool
VMR9OverlayPlugin::StartPlayer(VDPOverlay_WindowId windowId)   // IN
{
   FUNCTION_TRACE_MSG("Window 0x%x", windowId);
   m_started = true;
   return UpdatePlayback(windowId);
}


/*
 *----------------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::StopPlayer --
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------------
 */
bool
VMR9OverlayPlugin::StopPlayer(VDPOverlay_WindowId windowId) // IN
{
   FUNCTION_TRACE_MSG("Window 0x%x", windowId);
   m_started = false;
   return UpdatePlayback(windowId);
}


/*
 *----------------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::UpdatePlayback --
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------------
 */
bool
VMR9OverlayPlugin::UpdatePlayback(VDPOverlay_WindowId windowId)
{
   FUNCTION_TRACE_MSG("Window 0x%x  started(%d)  visible(%d)",
                      windowId, m_started, m_visible);

   OverlayPlayer* overlayPlayer = NULL;
   if (!GetPlayer(windowId, &overlayPlayer)) {
      FUNCTION_EXIT_MSG("Plugin%d - Can't find player for window 0x%x", m_contextId, windowId);
      return false;
   }

   if (m_visible && m_started) {
      if (!overlayPlayer->IsStarted()) {
         if (!overlayPlayer->StartVideo()) {
            FUNCTION_EXIT_MSG("Plugin%d - StartVideo() failed", m_contextId);
            return false;
         }
         FUNCTION_EXIT_MSG("Plugin%d - StartVideo() [OK]", m_contextId);
      }
   } else {
      if (overlayPlayer->IsStarted()) {
         if (!overlayPlayer->StopVideo()) {
            FUNCTION_EXIT_MSG("Plugin%d - StopVideo() failed", m_contextId);
            return false;
         }
         FUNCTION_EXIT_MSG("Plugin%d - StopVideo() [OK]", m_contextId);
      }
   }

   return true;
}


/*
 *----------------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::SetPlayerSize --
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------------
 */
bool
VMR9OverlayPlugin::SetPlayerSize(VDPOverlay_WindowId windowId, // IN
                                 int32 w,                      // IN
                                 int32 h)                      // IN
{
   FUNCTION_TRACE_MSG("Window 0x%x %dx%d", windowId, w, h);

   OverlayPlayer* overlayPlayer = NULL;
   if (!GetPlayer(windowId, &overlayPlayer)) {
      FUNCTION_EXIT_MSG("Plugin%d - Can't find player for window 0x%x", m_contextId, windowId);
      return false;
   }

   const VDPRPC_ChannelContextInterface* iChannelCtx = ChannelContextInterface();
   const VDPRPC_VariantInterface* iVariant = VariantInterface();

   void* messageCtx = NULL;
   if (!CreateMessage(&messageCtx)) {
      FUNCTION_EXIT_MSG("Plugin%d - CreateMessage() failed", m_contextId);
      return false;
   }

   iChannelCtx->v1.SetCommand(messageCtx, VMR9_OVERLAY_SET_SIZE);

   VDP_RPC_VARIANT var;
   iVariant->v1.VariantInit(&var);
   iVariant->v1.VariantFromInt32(&var, windowId);
   iChannelCtx->v1.AppendParam(messageCtx, &var);

   iVariant->v1.VariantFromInt32(&var, w);
   iChannelCtx->v1.AppendParam(messageCtx, &var);

   iVariant->v1.VariantFromInt32(&var, h);
   iChannelCtx->v1.AppendParam(messageCtx, &var);

   _bstr_t moviePath(overlayPlayer->MoviePath());
   iVariant->v1.VariantFromStr(&var, (char*)moviePath);
   iChannelCtx->v1.AppendParam(messageCtx, &var);

   if (!InvokeMessage(messageCtx)) {
      FUNCTION_EXIT_MSG("Plugin%d - InvokeMessage() failed", m_contextId);
      DestroyMessage(messageCtx);
      return false;
   }

   return true;
}


/*
 *----------------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::UpdateImage --
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------------
 */
bool
VMR9OverlayPlugin::UpdateImage(VDPOverlay_WindowId windowId,   // IN
                               void* image,                    // IN
                               int32 w,                        // IN
                               int32 h,                        // IN
                               int32 pitch,                    // IN
                               VDPOverlay_ImageFormat format,  // IN
                               bool copyImage)                 // IN
{
   OverlayPlayer* overlayPlayer = NULL;
   if (!GetPlayer(windowId, &overlayPlayer)) {
      LOG("Plugin%d - Can't find player for window 0x%x", m_contextId, windowId);
      return false;
   }

   static const uint32 VDP_OVERLAY_LAYOUT_MULTIPLE_ANY =
                        VDP_OVERLAY_LAYOUT_MULTIPLE_CENTER |
                        VDP_OVERLAY_LAYOUT_MULTIPLE_CORNER;

   if ((m_layoutMode & VDP_OVERLAY_LAYOUT_MULTIPLE_ANY) &&
       format == VDP_OVERLAY_BGRX) {

      BYTE* pRow = (BYTE*)image;
      for (int y=0;  y < h;  ++y) {
         DWORD* pPixels = (DWORD*)pRow;
         for (int x=0;  x < w;  ++x) {
            *pPixels++ |= 0xff000000;
         }
         pRow += pitch;
      }
      format = VDP_OVERLAY_BGRA;
   }


   VDPOverlay_Error err;
   char versionNum = ' ';

   if (OverlayClientInterface()->version >= VDP_OVERLAY_CLIENT_INTERFACE_V2) {
      uint32 flags = copyImage
                   ? VDP_OVERLAY_UPDATE_FLAG_COPY_IMAGE
                   : VDP_OVERLAY_UPDATE_FLAG_NONE;

      versionNum = '2';
      err = OverlayClientInterface()->v2.Update(m_contextId, windowId,
                                                image, w, h, pitch,
                                                format, flags);
   } else {
      versionNum = '1';
      err = OverlayClientInterface()->v1.Update(m_contextId, windowId,
                                                image, w, h, pitch, copyImage);
   }

   if (err != VDP_OVERLAY_ERROR_SUCCESS) {
      LOG("OverlayClient.v%c.Update() failed", versionNum);
      return false;
   }

   // LOG("Plugin%d - Window 0x%x  %dx%d  copyImage(%s)",
   //     m_contextId, windowId, w, h, LOG_BOOL(copyImage));
   return true;
}


/*
 *----------------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::DisplayBgImage --
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------------
 */
bool
VMR9OverlayPlugin::DisplayBgImage(VDPOverlay_WindowId windowId)   // IN
{
   OverlayPlayer* overlayPlayer = NULL;
   if (!GetPlayer(windowId, &overlayPlayer)) {
      LOG("Plugin%d - Can't find player for window 0x%x", m_contextId, windowId);
      return false;
   }

   UpdateImage(windowId,
               m_bgImage.m_data,
               m_bgImage.m_width,
               m_bgImage.m_height,
               m_bgImage.m_pitch,
               m_bgImage.m_format,
               false);  // copyImage
   return true;
}


/*
 *----------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::CopyImages --
 *
 * Results:
 *    Return the value of the player's 'copyImages' property
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
bool
VMR9OverlayPlugin::CopyImages(VDPOverlay_WindowId windowId)
{
   FUNCTION_TRACE_MSG("Window 0x%x", windowId);

   OverlayPlayer* overlayPlayer = NULL;
   if (!GetPlayer(windowId, &overlayPlayer)) {
      FUNCTION_EXIT_MSG("Plugin%d - Can't find player for window 0x%x", m_contextId, windowId);
      return false;
   }

   bool copyImages = overlayPlayer->CopyImages();
   FUNCTION_EXIT_MSG("copyImages(%s)", LOG_BOOL(copyImages));
   return copyImages;
}



/*
 *----------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::CopyImages --
 *
 * Results:
 *    None.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
void
VMR9OverlayPlugin::CopyImages(VDPOverlay_WindowId windowId, // IN
                              bool copyImages)              // IN
{
   FUNCTION_TRACE_MSG("Window 0x%x copyImages(%s)",
                      windowId, LOG_BOOL(copyImages));

   OverlayPlayer* overlayPlayer = NULL;
   if (!GetPlayer(windowId, &overlayPlayer)) {
      FUNCTION_EXIT_MSG("Plugin%d - Can't find player for window 0x%x", m_contextId, windowId);
      return;
   }

   overlayPlayer->CopyImages(copyImages);
}


/*
 *----------------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::OnWindowRegistered --
 *
 *----------------------------------------------------------------------------
 */
void
VMR9OverlayPlugin::OnWindowRegistered(
         void* userData,
         VDPOverlayClient_ContextId contextId,
         VDPOverlay_WindowId windowId,
         VDPOverlay_UserArgs userArgs,
         Bool* pReject,
         uint32* pResponse)
{
   FUNCTION_TRACE_MSG("Plugin%d  Window 0x%x", contextId, windowId);
   VMR9OverlayPlugin* pluginContext = (VMR9OverlayPlugin*)userData;
   pluginContext->CreatePlayer(windowId);
   pluginContext->DisplayBgImage(windowId);
}


/*
 *----------------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::OnWindowUnregistered --
 *
 *----------------------------------------------------------------------------
 */
void
VMR9OverlayPlugin::OnWindowUnregistered(
         void* userData,
         VDPOverlayClient_ContextId contextId,
         VDPOverlay_WindowId windowId,
         VDPOverlay_UserArgs userArgs)
{
   FUNCTION_TRACE_MSG("Plugin%d  Window 0x%x", contextId, windowId);
   VMR9OverlayPlugin* pluginContext = (VMR9OverlayPlugin*)userData;
   pluginContext->DestroyPlayer(windowId);
}


/*
 *----------------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::OnOverlayEnabled --
 *
 *----------------------------------------------------------------------------
 */
void
VMR9OverlayPlugin::OnOverlayEnabled(
         void* userData,
         VDPOverlayClient_ContextId contextId,
         VDPOverlay_WindowId windowId,
         VDPOverlay_UserArgs userArgs)
{
   LOG("Plugin%d  Window 0x%x", contextId, windowId);
}


/*
 *----------------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::OnOverlayDisabled --
 *
 *----------------------------------------------------------------------------
 */
void
VMR9OverlayPlugin::OnOverlayDisabled(
         void* userData,
         VDPOverlayClient_ContextId contextId,
         VDPOverlay_WindowId windowId,
         VDPOverlay_UserArgs userArgs)
{
   LOG("Plugin%d  Window 0x%x", contextId, windowId);
}


/*
 *----------------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::OnWindowPositionChanged --
 *
 *----------------------------------------------------------------------------
 */
void
VMR9OverlayPlugin::OnWindowPositionChanged(
         void* userData,
         VDPOverlayClient_ContextId contextId,
         VDPOverlay_WindowId windowId,
         int32 x, int32 y)
{
}


/*
 *----------------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::OnWindowSizeChanged --
 *
 *----------------------------------------------------------------------------
 */
void
VMR9OverlayPlugin::OnWindowSizeChanged(
         void* userData,
         VDPOverlayClient_ContextId contextId,
         VDPOverlay_WindowId windowId,
         int32 width, int32 height)
{
}


/*
 *----------------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::OnWindowObscured --
 *
 *----------------------------------------------------------------------------
 */
void
VMR9OverlayPlugin::OnWindowObscured(
         void* userData,
         VDPOverlayClient_ContextId contextId,
         VDPOverlay_WindowId windowId)
{
   FUNCTION_TRACE_MSG("Plugin%d  Window 0x%x", contextId, windowId);
   VMR9OverlayPlugin* pluginContext = (VMR9OverlayPlugin*)userData;
   pluginContext->m_visible = false;
   pluginContext->UpdatePlayback(windowId);
}


/*
 *----------------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::OnWindowVisible --
 *
 *----------------------------------------------------------------------------
 */
void
VMR9OverlayPlugin::OnWindowVisible(
         void* userData,
         VDPOverlayClient_ContextId contextId,
         VDPOverlay_WindowId windowId)
{
   FUNCTION_TRACE_MSG("Plugin%d  Window 0x%x", contextId, windowId);
   VMR9OverlayPlugin* pluginContext = (VMR9OverlayPlugin*)userData;
   pluginContext->m_visible = true;
   pluginContext->UpdatePlayback(windowId);
}


/*
 *----------------------------------------------------------------------------
 *
 * VMR9OverlayPlugin::OnLayoutModeChanged --
 *
 *----------------------------------------------------------------------------
 */
void
VMR9OverlayPlugin::OnLayoutModeChanged(
         void* userData,
         VDPOverlayClient_ContextId contextId,
         VDPOverlay_WindowId windowId,
         VDPOverlay_LayoutMode layoutMode)
{
   FUNCTION_TRACE_MSG("Plugin%d  Window 0x%x  layoutMode(%d)",
                      contextId, windowId, layoutMode);

   VMR9OverlayPlugin* pluginContext = NULL;
   if (!GetPlugin(contextId, &pluginContext)) {
      FUNCTION_EXIT_MSG("Plugin%d - GetPlugin() failed", contextId);
      return;
   }

   pluginContext->m_layoutMode = layoutMode;
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

   FUNCTION_TRACE_MSG("Received message %d", iChannelCtx->v1.GetCommand(messageCtx));

   VDP_RPC_VARIANT var;
   iVariant->v1.VariantInit(&var);

   if (iChannelCtx->v1.GetParamCount(messageCtx) < 1) {
      FUNCTION_EXIT_MSG("Missing window ID parameter");
      return;
   }

   iChannelCtx->v1.GetParam(messageCtx, 0, &var);
   VDPOverlay_WindowId windowId = var.lVal;

   switch (iChannelCtx->v1.GetCommand(messageCtx))
   {
   case VMR9_OVERLAY_FILE_OPEN: {
      LOG("VMR9_OVERLAY_FILE_OPEN");
      iChannelCtx->v1.GetParam(messageCtx, 1, &var);
      _bstr_t moviePath(var.strVal);
      OpenPlayer(windowId, moviePath);
      break;
     }

   case VMR9_OVERLAY_FILE_CLOSE:
      LOG("VMR9_OVERLAY_FILE_CLOSE");
      ClosePlayer(windowId);
      break;

   case VMR9_OVERLAY_PLAYBACK_START:
      LOG("VMR9_OVERLAY_PLAYBACK_START");
      StartPlayer(windowId);
      break;

   case VMR9_OVERLAY_PLAYBACK_STOP:
      LOG("VMR9_OVERLAY_PLAYBACK_STOP");
      StopPlayer(windowId);
      break;

   case VMR9_OVERLAY_COPY_IMAGES:
      LOG("VMR9_OVERLAY_COPY_IMAGES");
      iChannelCtx->v1.GetParam(messageCtx, 1, &var);
      CopyImages(windowId, var.cVal != 0);
      break;

   default:
      FUNCTION_EXIT_MSG("Unknown command");
      break;
   }
}
