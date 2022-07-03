/* ********************************************************************************* *
 * Copyright (C) 2011-2021 VMware, Inc.  All rights reserved. -- VMware Confidential *
 * ********************************************************************************* */

/*
 * VMR9OverlayPresenter.h --
 *
 */

#ifndef VMR9OVERLAYPRESENTER_H
#define VMR9OVERLAYPRESENTER_H

#include "vdpOverlay.h"
#include "vdpService.h"
#include "Allocator.h"


/*
 *----------------------------------------------------------------------
 *
 * class OverlayPresenter --
 *
 *----------------------------------------------------------------------
 */
class OverlayPresenter : public CAllocator
{
public:
   OverlayPresenter(HRESULT& hr,
                    HWND hWnd,
                    VDPOverlayClient_ContextId contextId,
                    VDPOverlay_WindowId windowId,
                    IDirect3D9* d3d = NULL,
                    IDirect3DDevice9* d3dd = NULL);

   ~OverlayPresenter();

   void ClearImage();

   void OnOpen();
   void OnStart();
   void OnStop();
   void OnNextImage(void* pImage, int width, int height, int pitch);

   bool CopyImages(void) { return m_copyImages; }
   void CopyImages(bool b) { m_copyImages = b; }

private:
   VDPOverlayClient_ContextId m_contextId;
   VDPOverlay_WindowId        m_windowId;
   bool                       m_copyImages;
};


/*
 *----------------------------------------------------------------------
 *
 * class OverlayImage --
 *
 *----------------------------------------------------------------------
 */
class OverlayImage
{
public:
   OverlayImage();
   virtual ~OverlayImage();

   bool Load(const WCHAR* filename, uint8 bmAlpha);

   void* m_data;
   int32 m_dataSz;
   int32 m_pitch;
   int32 m_width;
   int32 m_height;
   VDPOverlay_ImageFormat m_format;

private:
   ULONG_PTR m_gdiplusToken;
};

#endif // VMR9OVERLAYPRESENTER_H
