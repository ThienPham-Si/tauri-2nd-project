/* ********************************************************************************* *
 * Copyright (C) 2011-2021 VMware, Inc.  All rights reserved. -- VMware Confidential *
 * ********************************************************************************* */

/*
 * VMR9OverlayInterface.h --
 *
 */

#ifndef VMR9OVERLAYINTERFACE_H
#define VMR9OVERLAYINTERFACE_H

#include "vmware.h"
#include "RPCManager.h"

/*
 *----------------------------------------------------------------------
 *
 * Class OverlayWindow
 *
 *    Used to store user properties for each overlay window.
 *
 *----------------------------------------------------------------------
 */
class OverlayWindow
{
public:
   OverlayWindow()
   {
      m_hWnd = NULL;
      m_windowId = VDP_OVERLAY_WINDOW_ID_NONE;
      m_copyImages = false;
      m_videoSize.x = 0;
      m_videoSize.y = 0;
   }

   HWND                    m_hWnd;
   VDPOverlay_WindowId     m_windowId;
   bool                    m_copyImages;
   std::string             m_videoPath;
   POINT                   m_videoSize;
};


/*
 *----------------------------------------------------------------------
 *
 * Class VMR9OverlayPlugin
 *
 *
 *----------------------------------------------------------------------
 */
class VMR9OverlayPlugin : public RPCPluginInstance
{
public:
   VMR9OverlayPlugin(RPCManager *rpcManagerPtr);
   virtual ~VMR9OverlayPlugin();

   bool  Init(int32 msTimeout);
   bool  Exit();

   VDPOverlay_WindowId CreateOverlay(HWND hWnd);
   bool                DestroyOverlay(VDPOverlay_WindowId windowId);

   bool  OpenFile(VDPOverlay_WindowId windowId, const std::string& path);
   bool  CloseFile(VDPOverlay_WindowId windowId);
   bool  StartVideo(VDPOverlay_WindowId windowId);
   bool  StopVideo(VDPOverlay_WindowId windowId);

   bool  EnableOverlay(VDPOverlay_WindowId windowId, bool enabled);
   bool  IsOverlayEnabled(VDPOverlay_WindowId windowId);

   bool  CopyImages(VDPOverlay_WindowId windowId, bool copyImages);
   bool  AreImagesCopied(VDPOverlay_WindowId windowId);

   bool  SetLayoutMode(VDPOverlay_WindowId windowId, VDPOverlay_LayoutMode layoutMode);
   VDPOverlay_LayoutMode GetLayoutMode(VDPOverlay_WindowId windowId);

   bool   SetBackgroundColor(VDPOverlay_WindowId windowId, uint32 bgColor);
   uint32 GetBackgroundColor(VDPOverlay_WindowId windowId);

   bool  SetAreaRect(VDPOverlay_WindowId windowId, bool enabled, bool clipped, VDPOverlay_Rect& areaRect);
   bool  GetAreaRect(VDPOverlay_WindowId windowId, bool* pEnabled, bool* pClipped, VDPOverlay_Rect* pAreaRect);

   std::string  GetVideoPath(VDPOverlay_WindowId windowId);
   bool         GetVideoSize(VDPOverlay_WindowId windowId, POINT* pVideoSize);

   virtual void OnVideoReady(VDPOverlay_WindowId windowId) { }
   virtual void OnOverlayReady(VDPOverlay_WindowId windowId) { }
   virtual void OnOverlayRejected(VDPOverlay_WindowId windowId) { }
   virtual void OnOverlayCreateError(VDPOverlay_WindowId windowId) { }

private:
   void OnInvoke(void* messageCtx);

   static void OnOverlayReady(
            void* userData,
            VDPOverlay_WindowId windowId,
            uint32 response);

   static void OnOverlayRejected(
            void* userData,
            VDPOverlay_WindowId windowId,
            uint32 reason);

   static void OnOverlayCreateError(
            void* userData,
            VDPOverlay_WindowId windowId,
            VDPOverlay_Error error);

   bool  m_overlayInit;

   typedef std::map< VDPOverlay_WindowId, OverlayWindow > OverlayWindowMap;
   OverlayWindowMap m_overlayWindows;

   OverlayWindow* WindowIdToOverlay(VDPOverlay_WindowId windowId)
   {
      OverlayWindowMap::iterator it = m_overlayWindows.find(windowId);
      return it != m_overlayWindows.end() ? &it->second : NULL;
   }
};

#endif // VMR9OVERLAYINTERFACE_H
