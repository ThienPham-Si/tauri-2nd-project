/* ********************************************************************************* *
 * Copyright (C) 2011-2021 VMware, Inc.  All rights reserved. -- VMware Confidential *
 * ********************************************************************************* */

/*
 * VMR9OverlayPlayer.h --
 *
 */

#ifndef VMR9OVERLAYPLAYER_H
#define VMR9OVERLAYPLAYER_H

#include "VMR9OverlayPresenter.h"


/*
 *----------------------------------------------------------------------
 *
 * Class OverlayPlayer
 *
 *----------------------------------------------------------------------
 */
class OverlayPlayer
{
public:
   OverlayPlayer(HINSTANCE, VDPOverlayClient_ContextId, VDPOverlay_WindowId);
   virtual ~OverlayPlayer();

   bool                       StartThread(void);
   bool                       StopThread(DWORD msTimeout=INFINITE);

   bool                       StartVideo(void);
   bool                       StopVideo(void);
   bool                       IsStarted(void);

   BSTR                       MoviePath() { return m_moviePath; }
   void                       SetMoviePath(BSTR path) { m_moviePath = path; }
   static BSTR                GetMoviePath(void);

   bool                       CopyImages(void);
   void                       CopyImages(bool copyImages);

private:
   bool                       InitPlayer(void);
   HRESULT                    StartGraph(void);
   HRESULT                    CreateGraph(void);
   HRESULT                    CloseGraph(void);
   HRESULT                    SetAllocatorPresenter(void);
   void                       HandleGraphEvents(void);

   bool                       InitInstance(void);

   void                       PaintWindow(void);
   void                       UpdateStartStop(bool toggle);
   void                       UpdatePlayOverlay(bool toggle);
   void                       UpdatePlayDirectX(bool toggle);
   HRESULT                    AddGraphToRot(IUnknown *pUnkGraph, DWORD *pdwRegister);
   void                       RemoveGraphFromRot(DWORD dwRegister);

   static LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
   static LRESULT CALLBACK    WndProcAbout(HWND, UINT, WPARAM, LPARAM);

   void                       WaitForThread(DWORD msTimeout = INFINITE);
   static DWORD WINAPI        ThreadFunc(void* userData);

   HINSTANCE                        m_hInstance;
   TCHAR                            m_szTitle[100];
   TCHAR                            m_szWindowClass[100];
   static bool                      s_classRegistered;

   DWORD_PTR                        m_userId;
   HWND                             m_hWnd;
   UINT_PTR                         m_wndTimer;
   UINT_PTR                         m_wndTimerId;
   VDPOverlay_WindowId              m_overlayWndId;
   VDPOverlayClient_ContextId       m_overlayCtxId;

   BSTR                             m_moviePath;
   SmartPtr<IGraphBuilder>          m_graph;
   SmartPtr<IBaseFilter>            m_filter;
   SmartPtr<IMediaEvent>            m_mediaEvent;
   SmartPtr<IMediaControl>          m_mediaControl;
   SmartPtr<IMediaSeeking>          m_mediaSeeking;
   SmartPtr<IMediaPosition>         m_mediaPosition;
   SmartPtr<IVMRSurfaceAllocator9>  m_allocator;
   DWORD                            m_graphRotId;

   HANDLE                           m_hThread;
   DWORD                            m_threadId;
   HANDLE                           m_hExitEvent;

   OverlayPresenter*                m_overlayPresenter;
};

#endif // VMR9OVERLAYPLAYER_H
