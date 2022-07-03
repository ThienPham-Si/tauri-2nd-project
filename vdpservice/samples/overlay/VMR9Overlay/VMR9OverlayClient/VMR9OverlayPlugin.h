/* ********************************************************************************* *
 * Copyright (C) 2011-2021 VMware, Inc.  All rights reserved. -- VMware Confidential *
 * ********************************************************************************* */

/*
 * VMR9OverlayPlugin.h --
 *
 */
#ifndef VMR9OVERLAYPLUGIN_H
#define VMR9OVERLAYPLUGIN_H

#include "RPCManager.h"
#include "VMR9OverlayPlayer.h"


/*
 *----------------------------------------------------------------------
 *
 * class VMR9OverlayPlugin --
 *
 *----------------------------------------------------------------------
 */
class VMR9OverlayPlugin : public RPCPluginInstance
{
public:
   VMR9OverlayPlugin(RPCManager *rpcManagerPtr);
   virtual ~VMR9OverlayPlugin();

   static int32 NumPlugins(void) { return (int32)s_pluginContexts.size(); }

   static bool GetPlugin(VDPOverlayClient_ContextId contextId,
                         VMR9OverlayPlugin** pPluginContext=NULL);

   VDPOverlayClient_ContextId  ContextId(void) { return m_contextId; }

   bool StartPlayer(VDPOverlay_WindowId windowId);
   bool StopPlayer(VDPOverlay_WindowId windowId);
   bool OpenPlayer(VDPOverlay_WindowId windowId, BSTR path = NULL);
   bool ClosePlayer(VDPOverlay_WindowId windowId);
   bool UpdatePlayback(VDPOverlay_WindowId windowId);

   bool SetPlayerSize(VDPOverlay_WindowId windowId, int32 w, int32 h);

   bool DisplayBgImage(VDPOverlay_WindowId windowId);

   bool UpdateImage(VDPOverlay_WindowId windowId,
                    void* image, int32 w, int32 h, int32 pitch,
                    VDPOverlay_ImageFormat format, bool copyImage);

   bool CopyImages(VDPOverlay_WindowId windowId);
   void CopyImages(VDPOverlay_WindowId windowId, bool copyImages);

private:
   VDPOverlayClient_ContextId  m_contextId;
   VDPOverlay_LayoutMode m_layoutMode;
   OverlayImage m_bgImage;
   bool m_visible;
   bool m_started;

   bool CreatePlugin();
   bool DestroyPlugin();

   typedef std::map<VDPOverlayClient_ContextId, VMR9OverlayPlugin*> PluginContexts;
   static PluginContexts s_pluginContexts;

   bool CreatePlayer(VDPOverlay_WindowId windowId);
   bool DestroyPlayer(VDPOverlay_WindowId windowId);

   bool GetPlayer(VDPOverlay_WindowId windowId, OverlayPlayer** pOverlayPlayer);
   typedef std::map<VDPOverlay_WindowId, OverlayPlayer*> PlayerMap;
   PlayerMap m_players;

   void OnInvoke(void* messageCtx);

   static void OnWindowRegistered(
            void* userData,
            VDPOverlayClient_ContextId contextId,
            VDPOverlay_WindowId windowId,
            VDPOverlay_UserArgs userArgs,
            Bool* pReject,
            uint32* pResponse);

   static void OnWindowUnregistered(
            void* userData,
            VDPOverlayClient_ContextId contextId,
            VDPOverlay_WindowId windowId,
            VDPOverlay_UserArgs userArgs);

   static void OnOverlayEnabled(
            void* userData,
            VDPOverlayClient_ContextId contextId,
            VDPOverlay_WindowId windowId,
            VDPOverlay_UserArgs userArgs);

   static void OnOverlayDisabled(
            void* userData,
            VDPOverlayClient_ContextId contextId,
            VDPOverlay_WindowId windowId,
            VDPOverlay_UserArgs userArgs);

   static void OnWindowPositionChanged(
            void* userData,
            VDPOverlayClient_ContextId contextId,
            VDPOverlay_WindowId windowId,
            int32 x, int32 y);

   static void OnWindowSizeChanged(
            void* userData,
            VDPOverlayClient_ContextId contextId,
            VDPOverlay_WindowId windowId,
            int32 width, int32 height);

   static void OnWindowObscured(
            void* userData,
            VDPOverlayClient_ContextId contextId,
            VDPOverlay_WindowId windowId);

   static void OnWindowVisible(
            void* userData,
            VDPOverlayClient_ContextId contextId,
            VDPOverlay_WindowId windowId);

   static void OnLayoutModeChanged(
            void* userData,
            VDPOverlayClient_ContextId contextId,
            VDPOverlay_WindowId windowId,
            VDPOverlay_LayoutMode layoutMode);
};

#endif // VMR9OVERLAYPLUGIN_H
