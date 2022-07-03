//------------------------------------------------------------------------------
// File: Allocator.cpp
//
// Desc: DirectShow sample code - implementation of the CAllocator class
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "Allocator.h"

/*
 *----------------------------------------------------------------------
 *
 * Class CAllocator --
 *
 *----------------------------------------------------------------------
 */
CAllocator::CAllocator(HRESULT& hr, HWND wnd, IDirect3D9* d3d, IDirect3DDevice9* d3dd)
   : m_refCount(1)
   , m_D3D(d3d)
   , m_D3DDev(d3dd)
   , m_window(wnd)
   , m_pScene(NULL)
   , m_offscreenIndex(0)
   , m_playOverlay(true)
   , m_playDirectX(false)
{
   FUNCTION_TRACE_MSG("Window:0x%x", wnd);

   CAutoLock Lock(&m_ObjectLock);
   hr = E_FAIL;

   if (!IsWindow(wnd)) {
      FUNCTION_EXIT_MSG("!IsWindow()");
      hr = E_INVALIDARG;
      return;
   }

   if (m_D3D == NULL) {
      ASSERT(d3dd == NULL);

      m_D3D.Attach(Direct3DCreate9(D3D_SDK_VERSION));
      if (m_D3D == NULL) {
         FUNCTION_EXIT_MSG("m_D3D == NULL");
         hr = E_FAIL;
         return;
      }
   }

   if(m_D3DDev == NULL) {
      hr = CreateDevice();
   }
}


CAllocator::~CAllocator()
{
   FUNCTION_TRACE_MSG("Window:0x%x", m_window);
   DeleteSurfaces();
}


/*
 *----------------------------------------------------------------------
 *
 * CAllocator::CreateDevice --
 *
 *----------------------------------------------------------------------
 */
HRESULT
CAllocator::CreateDevice()
{
   HRESULT hr;
   m_D3DDev = NULL;
   D3DDISPLAYMODE dm;

   FUNCTION_TRACE_MSG("Window:0x%x", m_window);

   FAIL_RET_LOG(m_D3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &dm));
   D3DPRESENT_PARAMETERS pp;
   ZeroMemory(&pp, sizeof(pp));
   pp.Windowed = TRUE;
   pp.hDeviceWindow = m_window;
   pp.SwapEffect = D3DSWAPEFFECT_COPY;
   pp.BackBufferFormat = dm.Format;

   FAIL_RET_LOG(m_D3D->CreateDevice(D3DADAPTER_DEFAULT,
                                    D3DDEVTYPE_HAL,
                                    m_window,
                                    D3DCREATE_SOFTWARE_VERTEXPROCESSING
                                    | D3DCREATE_MULTITHREADED,
                                    &pp,
                                    &m_D3DDev));

   m_renderTarget = NULL;
   FAIL_RET_MSG(m_D3DDev->GetRenderTarget(0, &m_renderTarget));

   return S_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * CAllocator::DeleteSurfaces --
 *
 *----------------------------------------------------------------------
 */
void
CAllocator::DeleteSurfaces()
{
   FUNCTION_TRACE_MSG("Window:0x%x", m_window);
   CAutoLock Lock(&m_ObjectLock);

   if (m_pScene != NULL) {
      LOG("Deleting scene (0x%x)", m_pScene);
      delete m_pScene;
      m_pScene = NULL;
   }

   for (size_t i = 0;  i < m_offscreenInfo.size();  ++i) {
      if (m_offscreenInfo[i].m_surface != NULL) {
         LOG("Releasing off screen surface (0x%x)", m_offscreenInfo[i].m_surface);
         m_offscreenInfo[i].Unlock();
         m_offscreenInfo[i].m_surface = NULL;
      }
   }

   for (size_t i = 0;  i < m_surfaces.size();  ++i) {
      if (m_surfaces[i] != NULL) {
         LOG("Releasing surface %d (0x%x)", i, m_surfaces[i]);
         m_surfaces[i] = NULL;
      }
   }
}


/*
 *----------------------------------------------------------------------
 *
 * CAllocator::InitializeDevice --
 *
 *----------------------------------------------------------------------
 */
HRESULT
CAllocator::InitializeDevice(DWORD_PTR dwUserID,
                             VMR9AllocationInfo *lpAllocInfo,
                             DWORD *lpNumBuffers)
{
   HRESULT hr = S_OK;
   D3DCAPS9 d3dcaps;
   DWORD dwWidth = 1;
   DWORD dwHeight = 1;
   float fTU = 1.f;
   float fTV = 1.f;

   FUNCTION_TRACE_MSG("Window:0x%x", m_window);

   if (lpNumBuffers == NULL) {
      FUNCTION_EXIT_MSG("lpNumBuffers == NULL");
      return E_POINTER;
   }

   if (m_lpIVMRSurfAllocNotify == NULL) {
      FUNCTION_EXIT_MSG("m_lpIVMRSurfAllocNotify == NULL");
      return E_FAIL;
   }

   DeleteSurfaces();

   FAIL_RET_MSG(m_D3DDev->GetDeviceCaps(&d3dcaps));
   if (d3dcaps.TextureCaps & D3DPTEXTURECAPS_POW2) {

      while (dwWidth < lpAllocInfo->dwWidth) {
         dwWidth = dwWidth << 1;
      }

      while(dwHeight < lpAllocInfo->dwHeight) {
         dwHeight = dwHeight << 1;
      }

      fTU = (float)(lpAllocInfo->dwWidth) / (float)(dwWidth);
      fTV = (float)(lpAllocInfo->dwHeight) / (float)(dwHeight);
      lpAllocInfo->dwWidth = dwWidth;
      lpAllocInfo->dwHeight = dwHeight;
   }

   m_imageSize = lpAllocInfo->szNativeSize;
   LOG("Video size is %dx%d", m_imageSize.cx, m_imageSize.cy);

   // NOTE:
   // we need to make sure that we create textures because
   // surfaces can not be textured onto a primitive.
   lpAllocInfo->dwFlags |= VMR9AllocFlag_TextureSurface;

   LOG("Allocating %d surface%s at %dx%d",
         *lpNumBuffers, (*lpNumBuffers == 1 ? "" : "s"),
          lpAllocInfo->dwWidth, lpAllocInfo->dwHeight);

   m_surfaces.resize(*lpNumBuffers);
   FAIL_RET_MSG(m_lpIVMRSurfAllocNotify->AllocateSurfaceHelper(lpAllocInfo,
                                                               lpNumBuffers,
                                                               &m_surfaces.at(0)));
   for (size_t i = 0;  i < m_surfaces.size();  ++i) {
      LOG("Allocated surface %d (0x%x)", i, m_surfaces[i]);
   }

   DWORD numOffScreenBuffers = max(*lpNumBuffers, 2);
   LOG("Allocating %d off screen surfaces at %dx%d",
         numOffScreenBuffers, lpAllocInfo->dwWidth, lpAllocInfo->dwHeight);

   m_offscreenInfo.resize(numOffScreenBuffers);
   for (size_t i = 0;  i < m_offscreenInfo.size();  ++i) {
      SmartPtr<IDirect3DSurface9> surface;
      FAIL_RET_MSG(m_D3DDev->CreateOffscreenPlainSurface(lpAllocInfo->dwWidth,
                                                         lpAllocInfo->dwHeight,
                                                         D3DFMT_X8R8G8B8,
                                                         D3DPOOL_SYSTEMMEM,
                                                         &surface,
                                                         NULL));

      LOG("Allocated off screen surface (0x%x)", surface);
      m_offscreenInfo[i].m_surface = surface;
   }


   m_pScene = new CPlaneScene;
   m_pScene->SetSrcRect(fTU, fTV);
   FAIL_RET_MSG(m_pScene->Init(m_D3DDev));
   LOG("Created scene (0x%x)", m_pScene);

   OnOpen();
   return S_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * CAllocator::TerminateDevice --
 *
 *----------------------------------------------------------------------
 */
HRESULT
CAllocator::TerminateDevice(DWORD_PTR dwID)
{
   FUNCTION_TRACE_MSG("Window:0x%x", m_window);
   DeleteSurfaces();
   return S_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * CAllocator::GetSurface --
 *
 *----------------------------------------------------------------------
 */
HRESULT
CAllocator::GetSurface(DWORD_PTR dwUserID,
                       DWORD surfaceIndex,
                       DWORD surfaceFlags,
                       IDirect3DSurface9 **lplpSurface)
{
   if (lplpSurface == NULL) {
      LOG("lplpSurface == NULL");
      return E_POINTER;
   }

   if (surfaceIndex >= m_surfaces.size()) {
      LOG("Invalid index %d ", surfaceIndex);
      return E_FAIL;
   }

   CAutoLock Lock(&m_ObjectLock);
   *lplpSurface = m_surfaces[surfaceIndex];
   (*lplpSurface)->AddRef();

   return S_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * CAllocator::AdviseNotify --
 *
 *----------------------------------------------------------------------
 */
HRESULT
CAllocator::AdviseNotify(IVMRSurfaceAllocatorNotify9 *lpIVMRSurfAllocNotify)
{
   FUNCTION_TRACE_MSG("Window:0x%x", m_window);
   CAutoLock Lock(&m_ObjectLock);
   HRESULT hr;

   m_lpIVMRSurfAllocNotify = lpIVMRSurfAllocNotify;

   HMONITOR hMonitor = m_D3D->GetAdapterMonitor(D3DADAPTER_DEFAULT);
   FAIL_RET_MSG(m_lpIVMRSurfAllocNotify->SetD3DDevice(m_D3DDev, hMonitor));

   return hr;
}


/*
 *----------------------------------------------------------------------
 *
 * CAllocator::StartPresenting --
 *
 *----------------------------------------------------------------------
 */
HRESULT
CAllocator::StartPresenting(DWORD_PTR dwUserID)
{
   FUNCTION_TRACE_MSG("Window:0x%x", m_window);
   CAutoLock Lock(&m_ObjectLock);

   ASSERT(m_D3DDev);
   if (m_D3DDev == NULL) {
      FUNCTION_EXIT_MSG("m_D3DDev == NULL");
      return E_FAIL;
   }

   OnStart();
   return S_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * CAllocator::StopPresenting --
 *
 *----------------------------------------------------------------------
 */
HRESULT
CAllocator::StopPresenting(DWORD_PTR dwUserID)
{
   FUNCTION_TRACE_MSG("Window:0x%x", m_window);
   OnStop();
   return S_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * CAllocator::PresentImage --
 *
 *----------------------------------------------------------------------
 */
HRESULT
CAllocator::PresentImage(DWORD_PTR dwUserID,
                         VMR9PresentationInfo *lpPresInfo)
{
   HRESULT hr;
   CAutoLock Lock(&m_ObjectLock);

   /*
    * If we are in the middle of the display change
    */
   if (NeedToHandleDisplayChange()) {
      /*
       * NOTE: this piece of code is left as a user exercise.
       * The D3DDevice here needs to be switched
       * to the device that is using another adapter
       */
   }

   if ((hr = m_D3DDev->TestCooperativeLevel()) != S_OK) {
      LOG("TestCooperativeLevel() returned 0x%x", hr);
   }

   /*
    * IMPORTANT: device can be lost when user changes the resolution or when
    * Ctrl + Alt + Delete is pressed. We need to restore our video memory after that.
    */
   hr = PresentHelper(lpPresInfo);

   if (hr == D3DERR_DEVICELOST) {

      if (m_D3DDev->TestCooperativeLevel() == D3DERR_DEVICENOTRESET) {
         DeleteSurfaces();
         FAIL_RET_LOG(CreateDevice());

         HMONITOR hMonitor = m_D3D->GetAdapterMonitor(D3DADAPTER_DEFAULT);
         FAIL_RET_LOG(m_lpIVMRSurfAllocNotify->ChangeD3DDevice(m_D3DDev, hMonitor));
      }

      hr = S_OK;
   }

   return hr;
}


/*
 *----------------------------------------------------------------------
 *
 * CAllocator::PresentHelper --
 *
 *----------------------------------------------------------------------
 */
HRESULT
CAllocator::PresentHelper(VMR9PresentationInfo *lpPresInfo)
{
   if(lpPresInfo == NULL) {
      return E_POINTER;
   }

   if (lpPresInfo->lpSurf == NULL) {
      return E_POINTER;
   }

   CAutoLock Lock(&m_ObjectLock);
   HRESULT hr = S_OK;

   FAIL_RET_LOG(m_D3DDev->SetRenderTarget(0, m_renderTarget));

   SmartPtr<IDirect3DTexture9> texture;
   FAIL_RET_LOG(lpPresInfo->lpSurf->GetContainer(IID_IDirect3DTexture9,
                                                 (LPVOID*)&texture));

   if (m_playOverlay) {
      int prevIndex = m_offscreenIndex;
      if (++m_offscreenIndex >= (int)m_offscreenInfo.size()) {
         m_offscreenIndex = 0;
      }
      int curIndex = m_offscreenIndex;

      SmartPtr<IDirect3DSurface9> surface;
      FAIL_RET_LOG(texture->GetSurfaceLevel(0, &surface));
      FAIL_RET_LOG(m_D3DDev->GetRenderTargetData(surface, m_offscreenInfo[curIndex].m_surface));
      // LOG("Copied texture (0x%x) to offscreen surface (0x%x)", texture, m_offscreenSurface);

      bool readOnly = false;
      FAIL_RET_LOG(m_offscreenInfo[curIndex].Lock(readOnly));

      OnNextImage(m_offscreenInfo[curIndex].m_rect.pBits,
                  m_imageSize.cx, m_imageSize.cy,
                  m_offscreenInfo[curIndex].m_rect.Pitch);

      FAIL_RET_LOG(m_offscreenInfo[prevIndex].Unlock());
   }

   if (m_playDirectX) {
      FAIL_RET_LOG(m_pScene->DrawScene(m_D3DDev, texture));
      FAIL_RET_LOG(m_D3DDev->Present(NULL, NULL, NULL, NULL));
   }

   return hr;
}


/*
 *----------------------------------------------------------------------
 *
 * CAllocator::NeedToHandleDisplayChange --
 *
 *----------------------------------------------------------------------
 */
bool
CAllocator::NeedToHandleDisplayChange()
{
   if (!m_lpIVMRSurfAllocNotify) {
      return false;
   }

   D3DDEVICE_CREATION_PARAMETERS parameters;
   if (FAILED(m_D3DDev->GetCreationParameters(&parameters))) {
      ASSERT(false);
      return false;
   }

   HMONITOR currentMonitor = m_D3D->GetAdapterMonitor(parameters.AdapterOrdinal);
   HMONITOR hMonitor = m_D3D->GetAdapterMonitor(D3DADAPTER_DEFAULT);
   return hMonitor != currentMonitor;
}


/*
 *----------------------------------------------------------------------
 *
 * CAllocator::QueryInterface --
 *
 *----------------------------------------------------------------------
 */
HRESULT
CAllocator::QueryInterface(REFIID riid, void** ppvObject)
{
   if (ppvObject == NULL) {
      return E_POINTER;
   }

   if (riid == IID_IVMRSurfaceAllocator9) {
      *ppvObject = static_cast<IVMRSurfaceAllocator9*>(this);
      AddRef();
      return S_OK;
   }

   if (riid == IID_IVMRImagePresenter9) {
      *ppvObject = static_cast<IVMRImagePresenter9*>(this);
      AddRef();
      return S_OK;
   }

   if (riid == IID_IUnknown) {
      IVMRSurfaceAllocator9* allocator = static_cast<IVMRSurfaceAllocator9*>(this);
      *ppvObject = static_cast<IUnknown*>(allocator);
      AddRef();
      return S_OK;
   }

   return E_NOINTERFACE;
}


/*
 *----------------------------------------------------------------------
 *
 * CAllocator::AddRef --
 *
 *----------------------------------------------------------------------
 */
ULONG
CAllocator::AddRef()
{
   return InterlockedIncrement(&m_refCount);
}


/*
 *----------------------------------------------------------------------
 *
 * CAllocator::Release --
 *
 *----------------------------------------------------------------------
 */
ULONG
CAllocator::Release()
{
   ULONG ret = InterlockedDecrement(&m_refCount);

   if (ret == 0) {
      delete this;
   }

   return ret;
}
