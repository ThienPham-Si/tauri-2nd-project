/* ********************************************************************************* *
 * Copyright (C) 2011-2021 VMware, Inc.  All rights reserved. -- VMware Confidential *
 * ********************************************************************************* */

/*
 * VMR9OverlayPlayer.cpp --
 *
 */

#include "stdafx.h"
#include "resource.h"
#include "VMR9OverlayPlayer.h"
#include "vmrutil.h"


/*
 * Class statics
 */
bool OverlayPlayer::s_classRegistered = false;


/*
 *----------------------------------------------------------------------
 *
 * Class OverlayPlayer --
 *
 *----------------------------------------------------------------------
 */
OverlayPlayer::OverlayPlayer(HINSTANCE hInstance,                       // IN
                             VDPOverlayClient_ContextId overlayCtxId,   // IN
                             VDPOverlay_WindowId overlayWndId)          // IN
   : m_hInstance(hInstance),
     m_overlayCtxId(overlayCtxId),
     m_overlayWndId(overlayWndId),
     m_userId(0xACDCACDC),
     m_moviePath(),
     m_hWnd(NULL),
     m_wndTimer(NULL),
     m_wndTimerId(5),
     m_graphRotId(0),
     m_hThread(NULL),
     m_threadId(0),
     m_hExitEvent(NULL),
     m_overlayPresenter(NULL)
{
   LOG("");
}

OverlayPlayer::~OverlayPlayer()
{
   LOG("");
   _ASSERT(m_hThread == NULL);

   if (m_moviePath != NULL) {
      SysFreeString(m_moviePath);
      m_moviePath = NULL;
   }
}


/*
 *----------------------------------------------------------------------
 *
 * OverlayPlayer::StartThread --
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
OverlayPlayer::StartThread(void)
{
   FUNCTION_TRACE;

   m_hExitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
   m_hThread = CreateThread(NULL, 0, ThreadFunc, (void*)this, 0, &m_threadId);

   if (m_hThread == NULL) {
      FUNCTION_EXIT_MSG("Failed to create playback thread");
      return false;
   }

   FUNCTION_EXIT_MSG("Created new playback thread %d", m_threadId);
   return true;
}


/*
 *----------------------------------------------------------------------
 *
 * OverlayPlayer::StopThread --
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
OverlayPlayer::StopThread(DWORD msTimeout)
{
   FUNCTION_TRACE;
   bool ret = true;

   if (m_hThread == NULL) {
      FUNCTION_EXIT_MSG("m_hThread == NULL");
      return true;
   }

   if (msTimeout == INFINITE) {
      LOG("Waiting for thread %d to exit", m_threadId);

   } else {
      LOG("Waiting %dms for thread %d to exit", msTimeout, m_threadId);
   }

   SetEvent(m_hExitEvent);
   DWORD rc = WaitForSingleObject(m_hThread, msTimeout);

   if (rc == WAIT_OBJECT_0) {
      FUNCTION_EXIT_MSG("Thread %d has exited", m_threadId);
      ret = true;

   } else if (rc == WAIT_TIMEOUT) {
      FUNCTION_EXIT_MSG("Thread %d didn't exit after %dms", m_threadId, msTimeout);
      ret = false;

   } else {
      FUNCTION_EXIT_MSG("Error %d while waiting for thread %d to exit", rc, m_threadId);
      ret = false;
   }

   if (m_overlayPresenter != NULL) {
      m_overlayPresenter->ClearImage();
   }

   CloseHandle(m_hThread);
   m_hThread = NULL;
   m_threadId = 0;

   CloseHandle(m_hExitEvent);
   m_hExitEvent = NULL;
   return ret;
}


/*
 *----------------------------------------------------------------------
 *
 * OverlayPlayer::CopyImages --
 *
 * Results:
 *    Return the value of the presenter's 'copyImages' property
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
bool
OverlayPlayer::CopyImages(void)
{
   FUNCTION_TRACE;
   bool copyImages = false;

   if (m_overlayPresenter != NULL) {
      copyImages = m_overlayPresenter->CopyImages();
   }

   FUNCTION_EXIT_MSG("copyImages = %s", LOG_BOOL(copyImages));
   return copyImages;
}



/*
 *----------------------------------------------------------------------
 *
 * OverlayPlayer::CopyImages --
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
OverlayPlayer::CopyImages(bool copyImages)
{
   FUNCTION_TRACE_MSG("copyImages(%s)", LOG_BOOL(copyImages));

   if (m_overlayPresenter != NULL) {
      m_overlayPresenter->CopyImages(copyImages);
   }
}



/*
 *----------------------------------------------------------------------
 *
 * OverlayPlayer::StartVideo --
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
OverlayPlayer::StartVideo(void)
{
   FUNCTION_TRACE;

   if (m_mediaControl == NULL) {
      FUNCTION_EXIT_MSG("m_mediaControl == NULL");
      return false;
   }

   HRESULT hr = m_mediaControl->Run();
   FUNCTION_EXIT_MSG("m_mediaControl->Run() return 0x%x", hr);
   return !FAILED(hr);
}


/*
 *----------------------------------------------------------------------
 *
 * OverlayPlayer::StopVideo --
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
OverlayPlayer::StopVideo(void)
{
   FUNCTION_TRACE;

   if (m_mediaControl == NULL) {
      FUNCTION_EXIT_MSG("m_mediaControl == NULL");
      return false;
   }

   HRESULT hr = m_mediaControl->Pause();
   FUNCTION_EXIT_MSG("m_mediaControl->Pause() return 0x%x", hr);
   return !FAILED(hr);
}


/*
 *----------------------------------------------------------------------
 *
 * OverlayPlayer::IsStarted --
 *
 * Results:
 *    None.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
bool
OverlayPlayer::IsStarted()
{
   bool isStarted = false;

   if (m_mediaControl != NULL) {
      OAFilterState state;
      HRESULT hr = m_mediaControl->GetState(0, &state);
      if (SUCCEEDED(hr)) {
         isStarted = (state == State_Running);
      }
   }

   return isStarted;
}


/*
 *----------------------------------------------------------------------
 *
 * ThreadFunc --
 *
 *----------------------------------------------------------------------
 */
DWORD WINAPI
OverlayPlayer::ThreadFunc(void* userData)
{
   FUNCTION_TRACE;
   OverlayPlayer* overlayPlayer = (OverlayPlayer*)userData;
   overlayPlayer->InitPlayer();
   return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * OverlayPlayer::InitPlayer --
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
OverlayPlayer::InitPlayer(void)
{
   LOG("Enter");
   CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

   /*
    *   Verify that the VMR9 is present on this system
    */
   if(!VerifyVMR9())
   {
      LOG("Exit - VerifyVMR9() failed");
      CoUninitialize();
      return false;
   }

   __try {
      /*
       * Perform application initialization:
       */
      if (!InitInstance()) {
         LOG("InitInstance() failed");
         __leave;
      }

      HACCEL hAccelTable = LoadAccelerators(m_hInstance, (LPCTSTR)IDC_ALLOCATOR9);

      /*
       * Go straight to loading a video
       */
      if (FAILED(StartGraph())) {
         LOG("StartGraph() failed");
         __leave;
      }

      while (true) {
         MSG msg;
         int nEvents = 0;
         HANDLE hEvents[1] = { NULL };
         hEvents[nEvents++] = m_hExitEvent;
         DWORD msTimeout = INFINITE;

         DWORD rc = MsgWaitForMultipleObjects(nEvents, hEvents, FALSE, msTimeout, QS_ALLINPUT);
         if (rc != WAIT_OBJECT_0+nEvents) {
            LOG("Exiting loop (rc=%d)", rc);
            break;
         }

         while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {

            if (msg.message == WM_QUIT) {
               LOG("WM_QUIT received");
               SetEvent(m_hExitEvent);
               break;
            }

            if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
               TranslateMessage(&msg);
               DispatchMessage(&msg);
            }
         }
      }
   } __finally {
      /*
       *  Make sure to release everything at the end
       *  regardless of what's happening
       */
      CloseGraph();
      CoUninitialize();
   }

   LOG("Exit");
   return true;
}


/*
 *----------------------------------------------------------------------
 *
 * OverlayPlayer::GetMoviePath --
 *
 * Results:
 *    Returns the path of the movie to load, NULL on error.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
BSTR
OverlayPlayer::GetMoviePath(void)
{
    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    TCHAR szBuffer[MAX_PATH] = _T("");

    static const TCHAR szFilter[]
                            = TEXT("Video Files\0*.ASF;*.AVI;*.MP4;*.MPG;*.MPEG;*.VOB;*.QT;*.WMV\0")
                              TEXT("All Files\0*.*\0\0");
    ofn.lStructSize         = sizeof(OPENFILENAME);
    ofn.hwndOwner           = NULL;
    ofn.hInstance           = NULL;
    ofn.lpstrFilter         = szFilter;
    ofn.nFilterIndex        = 1;
    ofn.lpstrCustomFilter   = NULL;
    ofn.nMaxCustFilter      = 0;
    ofn.lpstrFile           = szBuffer;
    ofn.nMaxFile            = MAX_PATH;
    ofn.lpstrFileTitle      = NULL;
    ofn.nMaxFileTitle       = 0;
    ofn.lpstrInitialDir     = NULL;
    ofn.lpstrTitle          = TEXT("Select a video file to play...");
    ofn.Flags               = OFN_HIDEREADONLY;
    ofn.nFileOffset         = 0;
    ofn.nFileExtension      = 0;
    ofn.lpstrDefExt         = TEXT("AVI");
    ofn.lCustData           = 0L;
    ofn.lpfnHook            = NULL;
    ofn.lpTemplateName  = NULL;

    if (GetOpenFileName(&ofn))
    {
        return SysAllocString( szBuffer );
    }

   return NULL;
}


/*
 *----------------------------------------------------------------------
 *
 * OverlayPlayer::StartGraph --
 *
 * Results:
 *    Returns S_OK if all went well.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
HRESULT
OverlayPlayer::StartGraph(void)
{
   FUNCTION_TRACE;

   if (m_graph != NULL) {
      FUNCTION_EXIT_MSG("Already playing %ls", m_moviePath);
      return S_OK;
   }

   HRESULT hr = CreateGraph();
   if (FAILED(hr)) {
      LOG("Failed to create graph");
      CloseGraph();
      return hr;
   }

   UpdateStartStop(false);
   UpdatePlayOverlay(false);
   UpdatePlayDirectX(false);
   SetWindowText(m_hWnd, m_moviePath);

   FUNCTION_EXIT_MSG("Graph is ready");
   return hr;
}


/*
 *----------------------------------------------------------------------
 *
 * OverlayPlayer::CreateGraph --
 *
 * Results:
 *    Returns S_OK if all went well.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
HRESULT
OverlayPlayer::CreateGraph(void)
{
   FUNCTION_TRACE_MSG("Loading %ls", m_moviePath);

   SmartPtr<IVMRFilterConfig9> filterConfig;
   HRESULT hr = S_OK;

   FAIL_RET_MSG(CloseGraph());

   FAIL_RET_MSG(CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
                                       IID_IGraphBuilder, (void**)&m_graph));

   FAIL_RET_MSG(CoCreateInstance(CLSID_VideoMixingRenderer9, NULL, CLSCTX_INPROC_SERVER,
                                                   IID_IBaseFilter, (void**)&m_filter));

   FAIL_RET_MSG(m_filter->QueryInterface(IID_IVMRFilterConfig9, reinterpret_cast<void**>(&filterConfig)));

   FAIL_RET_MSG(filterConfig->SetRenderingMode(VMR9Mode_Renderless));

   FAIL_RET_MSG(filterConfig->SetNumberOfStreams(2));

   FAIL_RET_MSG(SetAllocatorPresenter());

   FAIL_RET_MSG(m_graph->AddFilter(m_filter, L"Video Mixing Renderer 9"));

   FAIL_RET_MSG(m_graph->QueryInterface(IID_IMediaControl, reinterpret_cast<void**>(&m_mediaControl)));

   FAIL_RET_MSG(m_graph->QueryInterface(IID_IMediaEvent, reinterpret_cast<void**>(&m_mediaEvent)));

   FAIL_RET_MSG(m_graph->QueryInterface(IID_IMediaPosition, reinterpret_cast<void**>(&m_mediaPosition)));

   FAIL_RET_MSG(m_graph->QueryInterface(IID_IMediaSeeking, reinterpret_cast<void**>(&m_mediaSeeking)));

   FAIL_RET_MSG(m_graph->RenderFile(m_moviePath, NULL));

   FAIL_RET_MSG(m_mediaControl->Run());

   // FAIL_RET_MSG(AddGraphToRot(m_graph, &m_graphRotId));

   return hr;
}


/*
 *----------------------------------------------------------------------
 *
 * OverlayPlayer::CloseGraph --
 *
 * Results:
 *    Returns S_OK if all went well.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
HRESULT
OverlayPlayer::CloseGraph(void)
{
   HRESULT hr = S_OK;

   if (m_graph == NULL) {
      return S_OK;
   }

   FUNCTION_TRACE;

   if (m_mediaControl != NULL)  {
      OAFilterState state;
      hr = m_mediaControl->GetState(0, &state);
      if (FAILED(hr)) {
         LOG("m_mediaControl->GetState() failed (0x%x)", hr);
      } else {
         LOG("Graph's current state is %d", state);
      }

      if (state != State_Stopped) {
         LOG("Requesting for graph to stop");
         hr = m_mediaControl->Stop();
         if (FAILED(hr)) {
            LOG("m_mediaControl->Stop() failed (0x%x)", hr);
         }

         LOG("Waiting for graph to stop");
         while (state != State_Stopped) {
            hr = m_mediaControl->GetState(0, &state);
            if (FAILED(hr)) {
               LOG("m_mediaControl->GetState() failed (0x%x)", hr);
            } else {
               LOG("Graph's current state is %d", state);
            }
         }
      }

      LOG("Graph is stopped");
   }

   if (m_graphRotId != 0) {
      RemoveGraphFromRot(m_graphRotId);
      m_graphRotId = 0;
   }

   m_overlayPresenter = NULL;
   LOG("m_overlayPresenter released");

   m_allocator = NULL;
   LOG("m_allocator released");

   m_mediaControl = NULL;
   LOG("m_mediaControl released");

   m_mediaEvent = NULL;
   LOG("m_mediaEvent released");

   m_mediaSeeking = NULL;
   LOG("m_mediaSeeking released");

   m_mediaPosition = NULL;
   LOG("m_mediaPosition released");

   m_filter = NULL;
   LOG("m_filter released");

   m_graph = NULL;
   LOG("m_graph released");

   InvalidateRect(m_hWnd, NULL, true);
   SetWindowText(m_hWnd, m_szTitle);
   UpdateStartStop(false);
   UpdatePlayOverlay(false);
   UpdatePlayDirectX(false);

   return S_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * OverlayPlayer::SetAllocatorPresenter --
 *
 * Results:
 *    Returns S_OK if all went well.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
HRESULT
OverlayPlayer::SetAllocatorPresenter(void)
{
   FUNCTION_TRACE;
   HRESULT hr = S_OK;

   SmartPtr<IVMRSurfaceAllocatorNotify9> lpIVMRSurfAllocNotify;
   FAIL_RET_MSG(m_filter->QueryInterface(IID_IVMRSurfaceAllocatorNotify9,
                                         reinterpret_cast<void**>(&lpIVMRSurfAllocNotify)));

   // create our custom allocator/presenter
   m_overlayPresenter = new OverlayPresenter(hr, m_hWnd, m_overlayCtxId, m_overlayWndId);
   m_allocator.Attach(m_overlayPresenter);
   if (FAILED(hr)) {
      FUNCTION_EXIT_MSG("m_allocator.Attach() (failed with 0x%x)", hr);
      m_allocator = NULL;
      return hr;
   }

   // let the allocator and the notify know about each other
   FAIL_RET_MSG(lpIVMRSurfAllocNotify->AdviseSurfaceAllocator(m_userId, m_allocator));
   FAIL_RET_MSG(m_allocator->AdviseNotify(lpIVMRSurfAllocNotify));

   return hr;
}


/*
 *----------------------------------------------------------------------
 *
 * OverlayPlayer::HandleGraphEvents --
 *
 * Results:
 *    Returns S_OK if all went well.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
void
OverlayPlayer::HandleGraphEvents(void)
{
   HRESULT hr;
   long evCode;
   LONG_PTR p1, p2;
   bool logEnterExit = false;

   if (logEnterExit) {
      LOG("Enter");
   }

   if(m_mediaEvent == NULL) {
      if (logEnterExit) {
         LOG("m_mediaEvent == NULL");
      }
      return;
   }

   while (!FAILED(m_mediaEvent->GetEvent(&evCode, &p1, &p2, 1))) {
      LOG("Processing %d", evCode);

      if (evCode == EC_COMPLETE) {
         LOG("Handling EC_COMPLETE");

         if (m_mediaControl != NULL && m_mediaPosition != NULL) {

            hr = m_mediaControl->Stop();
            LOG("m_mediaControl->Stop() return 0x%x", hr);

            hr = m_mediaPosition->put_CurrentPosition(0);
            LOG("m_mediaPosition->put_CurrentPosition() return 0x%x", hr);

            hr = m_mediaControl->Run();
            LOG("m_mediaControl->Run() return 0x%x", hr);
         }
      }

      m_mediaEvent->FreeEventParams(evCode, p1, p2);
   }

   if (logEnterExit) {
      LOG("Exit");
   }
}


/*
 *----------------------------------------------------------------------
 *
 * OverlayPlayer::InitInstance --
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
OverlayPlayer::InitInstance(void)
{
   /*
    * Initialize global strings
    */
   LoadString(m_hInstance, IDS_APP_TITLE, m_szTitle, ARRAYSIZE(m_szTitle));
   LoadString(m_hInstance, IDC_ALLOCATOR9, m_szWindowClass, ARRAYSIZE(m_szWindowClass));

   /*
    * Set the members of the window class structure.
    *
    * Don't provide a background brush, because we process the WM_PAINT
    * messages in OnPaint().  If a movie is active, we tell the VMR to
    * repaint the window; otherwise, we repaint with COLOR_WINDOW+1.
    * If a background brush is provided, you will see a white flicker
    * whenever you resize the main application window, because Windows
    * will repaint the window before the application also repaints.
    */
   if (!s_classRegistered) {

      WNDCLASS wc;
      ZeroMemory(&wc, sizeof(wc));

      wc.hInstance     = m_hInstance;
      wc.lpfnWndProc   = WndProc;
      wc.lpszClassName = m_szWindowClass;
      wc.lpszMenuName  = (LPCTSTR)IDC_ALLOCATOR9;
      wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
      wc.hbrBackground = NULL; // No background brush
      wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
      wc.hIcon         = LoadIcon(m_hInstance, MAKEINTRESOURCE(IDI_ALLOCATOR9));

      if(!RegisterClass(&wc)) {
         LOG("RegisterClass() failed");
         return false;
      }

      s_classRegistered = true;
   }


   m_hWnd = CreateWindow(m_szWindowClass,       // Window class
                         m_szTitle,             // Title
                         WS_OVERLAPPEDWINDOW,   // Style
                         950, 100,              // x, y
                         600, 500,              // w, h
                         NULL,                  // Parent
                         NULL,                  // Menu
                         m_hInstance,           // Instance
                         (LPVOID)this);         // lParam for WM_CREATE
   if (m_hWnd == NULL) {
      LOG("CreateWindow() failed");
      return false;
   }

   SetWindowLongPtr(m_hWnd, GWLP_USERDATA, (LONG_PTR)this);
   UpdateStartStop(false);
   UpdatePlayOverlay(false);
   UpdatePlayDirectX(false);

   m_wndTimer = SetTimer(m_hWnd, m_wndTimerId, 250, NULL);
   return true;
}


/*
 *----------------------------------------------------------------------
 *
 * OverlayPlayer::PaintWindow --
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
OverlayPlayer::PaintWindow(void)
{
   bool bNeedPaint = true;

   PAINTSTRUCT ps;
   HDC hdc = BeginPaint(m_hWnd, &ps);

   if (m_overlayPresenter != NULL) {
      if (m_overlayPresenter->PlayDirectX()) {

         /*
          * If we have a movie loaded then we only
          * need to repaint when the graph is stopped
          */
         if (m_mediaControl != NULL) {
            OAFilterState state;
            if (SUCCEEDED(m_mediaControl->GetState(0, &state))) {
               bNeedPaint = (state == State_Stopped);
            }
         }
      }
   }

   if (bNeedPaint) {
      RECT rc2;
      GetClientRect(m_hWnd, &rc2);
      FillRect(hdc, &rc2, (HBRUSH)(COLOR_WINDOW+1));
   }

   EndPaint(m_hWnd, &ps);
}


/*
 *----------------------------------------------------------------------
 *
 * OverlayPlayer::UpdateStartStop --
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
OverlayPlayer::UpdateStartStop(bool toggle)
{
   bool running = IsStarted();

   if (m_mediaControl != NULL) {
      HRESULT hr;

      if (toggle) {
         if (running) {
            hr = m_mediaControl->Pause();
            running = false;
         } else {
            hr = m_mediaControl->Run();
            running = true;
         }
      }
   }

   HMENU hMenu = GetMenu(m_hWnd);
   TCHAR* txt = running ? _T("Stop") : _T("Start");
   ModifyMenu(hMenu, ID_PLAYBACK_STARTSTOP, MF_STRING, ID_PLAYBACK_STARTSTOP, txt);

   UINT x = m_mediaControl == NULL ? MF_GRAYED :MF_ENABLED;
   EnableMenuItem(hMenu, ID_PLAYBACK_STARTSTOP, x);
}


/*
 *----------------------------------------------------------------------
 *
 * OverlayPlayer::UpdatePlayOverlay --
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
OverlayPlayer::UpdatePlayOverlay(bool toggle)
{
   bool b = false;

   if (m_overlayPresenter != NULL) {
      b = m_overlayPresenter->PlayOverlay();

      if (toggle) {
         m_overlayPresenter->PlayOverlay(b = !b);
      }
   }

   HMENU hMenu = GetMenu(m_hWnd);
   UINT x = b ? MF_CHECKED : MF_UNCHECKED;
   CheckMenuItem(hMenu, ID_PLAYBACK_OVERLAY, x);

   x = m_overlayPresenter == NULL ? MF_GRAYED :MF_ENABLED;
   EnableMenuItem(hMenu, ID_PLAYBACK_OVERLAY, x);
}


/*
 *----------------------------------------------------------------------
 *
 * OverlayPlayer::UpdatePlayDirectX --
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
OverlayPlayer::UpdatePlayDirectX(bool toggle)
{
   bool b = false;

   if (m_overlayPresenter != NULL) {
      b = m_overlayPresenter->PlayDirectX();

      if (toggle) {
         m_overlayPresenter->PlayDirectX(b = !b);
      }

      if (!b) {
         InvalidateRect(m_hWnd, NULL, true);
      }
   }

   HMENU hMenu = GetMenu(m_hWnd);
   UINT x = b ? MF_CHECKED : MF_UNCHECKED;
   CheckMenuItem(hMenu, ID_PLAYBACK_DIRECTX, x);

   x = m_overlayPresenter == NULL ? MF_GRAYED :MF_ENABLED;
   EnableMenuItem(hMenu, ID_PLAYBACK_DIRECTX, x);
}


/*
 *----------------------------------------------------------------------
 *
 * OverlayPlayer::WndProc --
 *
 * Results:
 *    None.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
LRESULT CALLBACK
OverlayPlayer::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   OverlayPlayer* overlayPlayer =
      (OverlayPlayer*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

   if (overlayPlayer == NULL) {
      return DefWindowProc(hWnd, message, wParam, lParam);
   }

   switch (message)
   {
   case WM_COMMAND: {
      int wmId    = LOWORD(wParam);
      int wmEvent = HIWORD(wParam);

      // Parse the menu selections:
      switch (wmId)
      {
      case IDM_PLAY_FILE:
         overlayPlayer->StartGraph();
         break;

      case ID_FILE_CLOSE:
         overlayPlayer->CloseGraph();
         break;

      case ID_PLAYBACK_STARTSTOP:
         overlayPlayer->UpdateStartStop(true);
         break;

      case ID_PLAYBACK_OVERLAY:
         overlayPlayer->UpdatePlayOverlay(true);
         break;

      case ID_PLAYBACK_DIRECTX:
         overlayPlayer->UpdatePlayDirectX(true);
         break;

      case IDM_ABOUT:
         DialogBox(overlayPlayer->m_hInstance,
                   (LPCTSTR)IDD_ABOUTBOX,
                   overlayPlayer->m_hWnd,
                   (DLGPROC)WndProcAbout);
         break;

      case IDM_EXIT:
         DestroyWindow(hWnd);
         break;

      default:
         return DefWindowProc(hWnd, message, wParam, lParam);
      }
      break;
    }

   case WM_PAINT:
      overlayPlayer->PaintWindow();
      break;

   case WM_TIMER:
      overlayPlayer->HandleGraphEvents();
      break;

   case WM_DESTROY:
      PostQuitMessage(0);
      break;

   default:
      return DefWindowProc(hWnd, message, wParam, lParam);
   }

   return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * OverlayPlayer::WndProcAbout --
 *
 * Results:
 *    None.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
LRESULT CALLBACK
OverlayPlayer::WndProcAbout(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
   switch (message)
   {
   case WM_INITDIALOG:
      return TRUE;

   case WM_COMMAND:
      if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
         EndDialog(hDlg, LOWORD(wParam));
         return TRUE;
      }
      break;
   }

   return FALSE;
}


/*
 *----------------------------------------------------------------------
 *
 * OverlayPlayer::AddGraphToRot --
 *
 *    Adds a DirectShow filter graph to the Running Object Table,
 *    allowing GraphEdit to load a remote filter graph.
 *
 * Results:
 *    None.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
HRESULT
OverlayPlayer::AddGraphToRot(IUnknown *pUnkGraph, DWORD *pdwRegister)
{
   SmartPtr<IMoniker> pMoniker;
   SmartPtr<IRunningObjectTable> pROT;
   WCHAR wsz[128];
   HRESULT hr;

   if (!pUnkGraph || !pdwRegister) {
      LOG("Invalid parameters");
      return E_POINTER;
   }

   hr = GetRunningObjectTable(0, &pROT);
   if (FAILED(hr)) {
      LOG("GetRunningObjectTable() (failed with 0x%x)", hr);
      return hr;
   }

   // Format the string for the registration
   hr = StringCchPrintfW(wsz, NUMELMS(wsz), L"FilterGraph %08x pid %08x\0",
                           (DWORD_PTR)pUnkGraph, GetCurrentProcessId());
   if (FAILED(hr)) {
      LOG("StringCchPrintfW() (failed with 0x%x)", hr);
      return hr;
   }

   // Create the moniker
   hr = CreateItemMoniker(L"!", wsz, &pMoniker);
   if (FAILED(hr)) {
      LOG("CreateItemMoniker() (failed with 0x%x)", hr);
      return hr;
   }

   // Use the ROTFLAGS_REGISTRATIONKEEPSALIVE to ensure a strong reference
   // to the object.  Using this flag will cause the object to remain
   // registered until it is explicitly revoked with the Revoke() method.
   //
   // Not using this flag means that if GraphEdit remotely connects
   // to this graph and then GraphEdit exits, this object registration
   // will be deleted, causing future attempts by GraphEdit to fail until
   // this application is restarted or until the graph is registered again.
   hr = pROT->Register(ROTFLAGS_REGISTRATIONKEEPSALIVE,
                       pUnkGraph, pMoniker, pdwRegister);
   if (FAILED(hr)) {
      LOG("Register() (failed with 0x%x)", hr);
      return hr;
   }

   LOG("[OK]");
   return S_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * OverlayPlayer::RemoveGraphFromRot --
 *
 *    Removes a filter graph from the Running Object Table.
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
OverlayPlayer::RemoveGraphFromRot(DWORD dwRegister)
{
   SmartPtr<IRunningObjectTable> pROT;

   if (SUCCEEDED(GetRunningObjectTable(0, &pROT))) {
      pROT->Revoke(dwRegister);
      LOG("[OK]");
   }
}
