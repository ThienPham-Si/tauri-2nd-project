/* ********************************************************************************* *
 * Copyright (C) 2011-2021 VMware, Inc.  All rights reserved. -- VMware Confidential *
 * ********************************************************************************* */

/*
 * VMR9OverlayPresenter.cpp --
 *
 */

#include "stdafx.h"
#include "VMR9OverlayPresenter.h"
#include "VMR9OverlayPlugin.h"


/*
 *----------------------------------------------------------------------
 *
 * Class OverlayPresenter --
 *
 *----------------------------------------------------------------------
 */
OverlayPresenter::OverlayPresenter(HRESULT& hr,                            // OUT
                                   HWND hWnd,                              // IN
                                   VDPOverlayClient_ContextId contextId,   // IN
                                   VDPOverlay_WindowId windowId,           // IN
                                   IDirect3D9* d3d,                        // IN
                                   IDirect3DDevice9* d3dd)                 // IN
   : m_contextId(contextId),
     m_windowId(windowId),
     m_copyImages(false),
     CAllocator (hr, hWnd, d3d, d3dd)
{
   LOG("contextId:%d  windowId:0x%x", m_contextId, m_windowId);
}

OverlayPresenter::~OverlayPresenter()
{
   LOG("contextId:%d  windowId:0x%x", m_contextId, m_windowId);
   ClearImage();
}


/*
 *----------------------------------------------------------------------
 *
 * OverlayPresenter::ClearImage --
 *
 *----------------------------------------------------------------------
 */
void
OverlayPresenter::ClearImage()
{
   LOG("contextId:%d  windowId:0x%x", m_contextId, m_windowId);
   VMR9OverlayPlugin* vmr9OverlayPlugin = NULL;

   if (VMR9OverlayPlugin::GetPlugin(m_contextId, &vmr9OverlayPlugin)) {
      vmr9OverlayPlugin->DisplayBgImage(m_windowId);
   }
}


/*
 *----------------------------------------------------------------------
 *
 * OverlayPresenter::OnOpen --
 *
 *----------------------------------------------------------------------
 */
void
OverlayPresenter::OnOpen()
{
   LOG("contextId:%d  windowId:0x%x", m_contextId, m_windowId);
   VMR9OverlayPlugin* vmr9OverlayPlugin = NULL;

   if (VMR9OverlayPlugin::GetPlugin(m_contextId, &vmr9OverlayPlugin)) {
      vmr9OverlayPlugin->SetPlayerSize(m_windowId, m_imageSize.cx, m_imageSize.cy);
   }
}


/*
 *----------------------------------------------------------------------
 *
 * OverlayPresenter::OnStart --
 *
 *----------------------------------------------------------------------
 */
void
OverlayPresenter::OnStart()
{
   LOG("contextId:%d  windowId:0x%x", m_contextId, m_windowId);
}


/*
 *----------------------------------------------------------------------
 *
 * OverlayPresenter::OnStop --
 *
 *----------------------------------------------------------------------
 */
void
OverlayPresenter::OnStop()
{
   LOG("contextId:%d  windowId:0x%x", m_contextId, m_windowId);
}


/*
 *----------------------------------------------------------------------
 *
 * OverlayPresenter::OnNextImage --
 *
 *----------------------------------------------------------------------
 */
void
OverlayPresenter::OnNextImage(void* pImage,  // IN
                              int width,     // IN
                              int height,    // IN
                              int pitch)     // IN
{
   VMR9OverlayPlugin* vmr9OverlayPlugin = NULL;

   if (VMR9OverlayPlugin::GetPlugin(m_contextId, &vmr9OverlayPlugin)) {
      vmr9OverlayPlugin->UpdateImage(m_windowId, pImage, width, height, pitch,
                                                 VDP_OVERLAY_BGRX, m_copyImages);
   }
}


/*
 *----------------------------------------------------------------------
 *
 * Class OverlayImage --
 *
 *----------------------------------------------------------------------
 */
#include <gdiplus.h>
using namespace Gdiplus;
#pragma comment(lib, "Gdiplus.lib")

OverlayImage::OverlayImage()
{
   GdiplusStartupInput gdiplusStartupInput;
   GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);

   m_width  = 1;
   m_height = 1;
   m_pitch  = m_width * sizeof(uint32);
   m_dataSz = m_height * m_pitch;
   m_data   = calloc(1, m_dataSz);
   m_format = VDP_OVERLAY_BGRA;
}

OverlayImage::~OverlayImage()
{
   free(m_data);
   GdiplusShutdown(m_gdiplusToken);
}


/*
 *----------------------------------------------------------------------
 *
 * OverlayImage::Load --
 *
 *----------------------------------------------------------------------
 */
bool
OverlayImage::Load(const WCHAR* filename, uint8 bmAlpha)
{
   /*
    * Load the bitmap and get access to the pixels
    */
   Bitmap* pBitmap = Bitmap::FromFile(filename);
   if (pBitmap == NULL) {
      LOG("Bitmap::FromFile(%ls) failed", filename);
      return false;
   }

   BitmapData bitmapData;
   Rect bitmapRect = Rect(0, 0 , pBitmap->GetWidth(), pBitmap->GetHeight());
   Status status = pBitmap->LockBits(&bitmapRect, ImageLockModeRead,
                                       PixelFormat32bppARGB, &bitmapData);
   if (status != Ok) {
      LOG("Bitmap::LockBits(%ls) failed (status=%d)", filename, status);
      return false;
   }

   /*
    * Make a copy of the pixels
    */
   int32 width  = bitmapData.Width;
   int32 height = bitmapData.Height;
   int32 pitch  = bitmapData.Stride;
   int32 dataSz = pitch * height;

   void* data = realloc(m_data, dataSz);
   if (data == NULL) {
      LOG("malloc(%d) failed", dataSz);
      return false;
   }

   m_data   = data;
   m_dataSz = dataSz;
   m_width  = width;
   m_height = height;
   m_pitch  = pitch;
   m_format = VDP_OVERLAY_BGRA;

   memcpy(m_data, bitmapData.Scan0, m_dataSz);

   /*
    * The image might be bottom-up
    */
   if (m_height < 0) {
      m_height *= -1;
   }

   /*
    * Apply an alpha to the image.  The pixels
    * need to be pre-multiplied by the alpha value.
    */
   if (bmAlpha != 255) {
      uint8 a = bmAlpha;
      double alpha = a / 255.0;
      char* image = (char*)m_data;

      for (int32 h=0;  h < m_height;  ++h) {
         uint32* imageRow = (uint32*)image;
         for (int32 w=0;  w < m_width;  ++w) {
            uint32 pixel = imageRow[w];

            uint8 r = (pixel >> 16) & 0xff;
            uint8 g = (pixel >>  8) & 0xff;
            uint8 b = (pixel >>  0) & 0xff;

            r = (uint8)(r * alpha + 0.5);
            g = (uint8)(g * alpha + 0.5);
            b = (uint8)(b * alpha + 0.5);

            pixel = ((uint32)a << 24) |
                    ((uint32)r << 16) |
                    ((uint32)g <<  8) |
                    ((uint32)b <<  0) ;

            imageRow[w] = pixel;
         }
         image += m_pitch;
      }
   }

   /*
    * Unlock the bitmap data and delete the bitmap
    */
   status = pBitmap->UnlockBits(&bitmapData);
   if (status != Ok) {
      LOG("Bitmap::UnlockBits(%ls) failed (status=%d)", filename, status);
   }

   delete pBitmap;

   LOG("%ls [OK]", filename);
   return true;
}
