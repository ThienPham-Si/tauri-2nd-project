/* ********************************************************************************* *
 * Copyright (C) 2011-2021 VMware, Inc.  All rights reserved. -- VMware Confidential *
 * ********************************************************************************* */

/*
 * VMR9OverlayGuest.cpp --
 *
 */

#include "stdafx.h"
#include "VMR9OverlayGuest.h"
#define MAX_LOADSTRING 100


/*
 * Global Variables:
 */
HINSTANCE            g_hInstance;                           // Current instance
HWND                 g_hWndMain;                            // Main window
HWND                 g_hWndChild;                           // Child window
COLORREF             g_bgColorMain;                         // Background color of main window
COLORREF             g_bgColorChild;                        // Background color of child window
HBRUSH               g_hBgMain;                             // Background brush for the main window
HBRUSH               g_hBgChild;                            // Background brush for the child window
TCHAR                g_szTitle[MAX_LOADSTRING];             // The title bar text for the main window
TCHAR                g_szWindowClassMain[MAX_LOADSTRING];   // The main window class name
TCHAR                g_szWindowClassChild[MAX_LOADSTRING];  // The child window class name
VDPOverlay_WindowId  g_overlayWindowId = VDP_OVERLAY_WINDOW_ID_NONE;
BOOL                 g_createChildWindow = TRUE;
BOOL                 g_toggleChildWindow = FALSE;
VMR9OverlayApp*      g_vmr9OverlayApp = NULL;
OverlaySettings      g_overlaySettings;


/*
 * Local function prototypes
 */
BOOL             MyRegisterClass(HINSTANCE hInstance);
BOOL             InitInstance(HINSTANCE, int);
void             PaintWindow(HWND, HDC, BOOL, BOOL);
LRESULT CALLBACK WndProcMain(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WndProcChild(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK WndProcAbout(HWND, UINT, WPARAM, LPARAM);
void             SaveOverlaySettings(VDPOverlay_WindowId overlayWindowId);
void             RestoreOverlaySettings(VDPOverlay_WindowId overlayWindowId);
void             UpdateLayoutMenu(VDPOverlay_WindowId overlayWindowId, int layoutId);
void             UpdateBgColorMenu(VDPOverlay_WindowId overlayWindowId, int bgColorId);
void             UpdateAreaMenu(VDPOverlay_WindowId overlayWindowId, int areaId);


/*
 *----------------------------------------------------------------------
 *
 * _tWinMain --
 *
 *    Program entry point
 *
 * Results:
 *    None.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
int APIENTRY
_tWinMain(HINSTANCE hInstance,
          HINSTANCE hPrevInstance,
          LPTSTR    lpCmdLine,
          int       nCmdShow)
{
   UNREFERENCED_PARAMETER(hPrevInstance);
   UNREFERENCED_PARAMETER(lpCmdLine);

   int returnCode = 0;
   MSG msg;
   HACCEL hAccelTable;

   /*
    * First things, first, initialize the logging system
    */
   LogUtils::LogInit(VMR9_OVERLAY_TOKEN_NAME, true);

   /*
    * Initialize the Overlay and RPC APIs
    */
   g_vmr9OverlayApp = new VMR9OverlayApp();

   int32 msTimeout = 5000;
   if (!g_vmr9OverlayApp->Init(msTimeout)) {
      goto Exit;
   }

   /*
    * Initialize global strings
    */
   LoadString(hInstance, IDS_APP_TITLE, g_szTitle, MAX_LOADSTRING);
   LoadString(hInstance, IDC_VMR9OVERLAYGUEST, g_szWindowClassMain, MAX_LOADSTRING);
   LoadString(hInstance, IDC_VMR9OVERLAYGUEST2, g_szWindowClassChild, MAX_LOADSTRING);
   hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_VMR9OVERLAYGUEST));
   MyRegisterClass(hInstance);

   while (true) {
      /*
       * Perform application initialization:
       */
      if (!InitInstance (hInstance, nCmdShow)) {
         goto Exit;
      }


      /*
       * Keep the messages flowing, RPC depends on them
       */
      while (GetMessage(&msg, NULL, 0, 0)) {

         if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
         }
      }

      /*
       * Check if I'm turning the child window on/off
       */
      if (!g_toggleChildWindow) break;
      g_toggleChildWindow = FALSE;
      g_createChildWindow = !g_createChildWindow;
   };

   g_vmr9OverlayApp->Exit();
   returnCode = (int)msg.wParam;

Exit:
   if (g_vmr9OverlayApp != NULL) {
      delete g_vmr9OverlayApp;
      g_vmr9OverlayApp = NULL;
   }

   return returnCode;
}


/*
 *----------------------------------------------------------------------
 *
 * MyRegisterClass --
 *
 * Results:
 *    None.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
BOOL
MyRegisterClass(HINSTANCE hInstance)
{
   ATOM atomMain;
   ATOM atomChild;
   WNDCLASSEX wcex;
   wcex.cbSize = sizeof(WNDCLASSEX);

   g_bgColorMain = RGB(53, 187, 255);
   g_hBgMain = CreateSolidBrush(g_bgColorMain);

   g_bgColorChild = RGB(0, 200, 0);
   g_hBgChild = CreateSolidBrush(g_bgColorChild);

   wcex.style           = CS_HREDRAW | CS_VREDRAW;
   wcex.lpfnWndProc     = WndProcMain;
   wcex.cbClsExtra      = 0;
   wcex.cbWndExtra      = 0;
   wcex.hInstance       = hInstance;
   wcex.hIcon           = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_VMR9OVERLAYGUEST));
   wcex.hCursor         = LoadCursor(NULL, IDC_ARROW);
   wcex.hbrBackground   = g_hBgMain;
   wcex.lpszMenuName    = MAKEINTRESOURCE(IDC_VMR9OVERLAYGUEST);
   wcex.lpszClassName   = g_szWindowClassMain;
   wcex.hIconSm         = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
   atomMain = RegisterClassEx(&wcex);

   wcex.style           = CS_HREDRAW | CS_VREDRAW;;
   wcex.lpfnWndProc     = WndProcChild;
   wcex.cbClsExtra      = 0;
   wcex.cbWndExtra      = 0;
   wcex.hInstance       = hInstance;
   wcex.hIcon           = NULL;
   wcex.hCursor         = LoadCursor(NULL, IDC_ARROW);
   wcex.hbrBackground   = g_hBgChild;
   wcex.lpszMenuName    = NULL;
   wcex.lpszClassName   = g_szWindowClassChild;
   wcex.hIconSm         = NULL;
   atomChild = RegisterClassEx(&wcex);

   return (atomMain != 0) && (atomChild != 0);
}

/*
 *----------------------------------------------------------------------
 *
 * InitInstance --
 *
 *    Saves instance handle and creates main and child windows
 *
 * Results:
 *    TRUE if all went well.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
BOOL
InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   g_hInstance = hInstance;

   g_hWndMain = CreateWindow(g_szWindowClassMain,  // Window class
                             g_szTitle,            // Title
                             WS_OVERLAPPEDWINDOW | // Style
                             WS_CLIPCHILDREN,      //
                             200, 100,             // x, y
                             1024, 768,            // w, h
                             NULL,                 // Parent
                             NULL,                 // Menu
                             hInstance,            // Instance
                             NULL);                // lParam for WM_CREATE
   if (!g_hWndMain) {
      return FALSE;
   }

   if (g_createChildWindow) {
      g_hWndChild = CreateWindow(g_szWindowClassChild, // Window class
                                 _T(""),               // Title
                                 WS_CHILD |            // Style
                                 WS_VISIBLE,           //
                                 0, 0,                 // x, y
                                 1024, 768,            // w, h
                                 g_hWndMain,           // Parent
                                 NULL,                 // Menu
                                 hInstance,            // Instance
                                 NULL);                // lParam for WM_CREATE
      if (!g_hWndChild) {
         DestroyWindow(g_hWndMain);
         g_hWndMain = NULL;
         return FALSE;
      }
   }

   ShowWindow(g_hWndMain, nCmdShow);
   UpdateWindow(g_hWndMain);

   return TRUE;
}


/*
 *----------------------------------------------------------------------
 *
 * WndProcMain --
 *
 *    Message handler for the main window.
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
WndProcMain(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   if (g_vmr9OverlayApp != NULL) {
      switch (message) {
      case WM_COMMAND: {
         int wmId = LOWORD(wParam);
         int wmEvent = HIWORD(wParam);

         switch (wmId) {
         case IDM_ABOUT:
            DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, WndProcAbout);
            break;

         case ID_FILE_OPEN:
            g_vmr9OverlayApp->OpenFile(g_overlayWindowId, "");
            break;

         case ID_FILE_CLOSE:
            g_vmr9OverlayApp->CloseFile(g_overlayWindowId);
            g_overlaySettings.videoPath = "";
            break;

         case ID_PLAYBACK_START:
            g_vmr9OverlayApp->StartVideo(g_overlayWindowId);
            break;

         case ID_PLAYBACK_STOP:
            g_vmr9OverlayApp->StopVideo(g_overlayWindowId);
            break;

         case ID_PLAYBACK_ENABLECHILDWINDOW:
            g_toggleChildWindow = TRUE;
            SaveOverlaySettings(g_overlayWindowId);
            DestroyWindow(hWnd);
            break;

         case ID_LAYOUT_CENTER:
         case ID_LAYOUT_TILE:
         case ID_LAYOUT_SCALE:
         case ID_LAYOUT_SCALE2:
         case ID_LAYOUT_SCALE3:
         case ID_LAYOUT_CROP:
         case ID_LAYOUT_CROP2:
         case ID_LAYOUT_CROP3:
         case ID_LAYOUT_LETTERBOX:
         case ID_LAYOUT_LETTERBOX2:
         case ID_LAYOUT_LETTERBOX3:
            UpdateLayoutMenu(g_overlayWindowId, wmId);
            break;

         case ID_BACKGROUND_DISABLED:
         case ID_BACKGROUND_BLACK:
         case ID_BACKGROUND_WHITE:
         case ID_BACKGROUND_RED:
         case ID_BACKGROUND_BLUE:
         case ID_BACKGROUND_PURPLE:
            UpdateBgColorMenu(g_overlayWindowId, wmId);
            break;

         case ID_AREA_ENABLE:
         case ID_AREA_CLIPTOWINDOW:
         case ID_POSITION_CENTER:
         case ID_POSITION_TOP_LEFT:
         case ID_POSITION_TOP_RIGHT:
         case ID_POSITION_BOTTOM_LEFT:
         case ID_POSITION_BOTTOM_RIGHT:
         case ID_SIZE_16x9:
         case ID_SIZE_4x3:
         case ID_SIZE_1x1:
         case ID_SIZE_VIDEO:
         case ID_SIZE_WINDOW:
            UpdateAreaMenu(g_overlayWindowId, wmId);
            break;

         case ID_PLAYBACK_ENABLEOVERLAY: {
            bool enabled = !(g_vmr9OverlayApp->IsOverlayEnabled(g_overlayWindowId));
            g_vmr9OverlayApp->EnableOverlay(g_overlayWindowId, enabled);

            HMENU hMenu = ::GetMenu(hWnd);
            UINT x = enabled ? MF_CHECKED : MF_UNCHECKED;
            ::CheckMenuItem(hMenu, ID_PLAYBACK_ENABLEOVERLAY, x);
            break;
         }

         case ID_PLAYBACK_COPYIMAGES: {
            bool copyImages = !(g_vmr9OverlayApp->AreImagesCopied(g_overlayWindowId));
            g_vmr9OverlayApp->CopyImages(g_overlayWindowId, copyImages);

            HMENU hMenu = ::GetMenu(hWnd);
            UINT x = copyImages ? MF_CHECKED : MF_UNCHECKED;
            ::CheckMenuItem(hMenu, ID_PLAYBACK_COPYIMAGES, x);
            break;
         }

         case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
         }
         break;
      }

      case WM_CREATE:
         if (!g_createChildWindow) {
            g_overlayWindowId = g_vmr9OverlayApp->CreateOverlay(hWnd);
         }
         break;

      case WM_DESTROY:
         if (!g_createChildWindow) {
            g_vmr9OverlayApp->DestroyOverlay(g_overlayWindowId);
            g_overlayWindowId = VDP_OVERLAY_WINDOW_ID_NONE;
         }
         PostQuitMessage(0);
         break;

      case WM_PAINT: {
         PAINTSTRUCT ps;
         HDC hDC = BeginPaint(hWnd, &ps);
         PaintWindow(hWnd, hDC, ps.fErase, TRUE);
         EndPaint(hWnd, &ps);
         return 0;
      }

      case WM_WINDOWPOSCHANGING:
      case WM_WINDOWPOSCHANGED:
         if (g_hWndChild != NULL) {
            RECT rect;
            GetClientRect(hWnd, &rect);
            int w = rect.right - rect.left;
            int h = rect.bottom - rect.top;
            HWND hWndInsertAfter = NULL;
            UINT flags = SWP_NOZORDER | SWP_NOACTIVATE;
            ::SetWindowPos(g_hWndChild, hWndInsertAfter, 0, 0, w, h, flags);
         }
         break;
      }
   }
   return DefWindowProc(hWnd, message, wParam, lParam);
}

/*
 *----------------------------------------------------------------------
 *
 * WndProcAbout --
 *
 *    Message handler for the about box.
 *
 * Results:
 *    None.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
INT_PTR CALLBACK
WndProcAbout(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
   UNREFERENCED_PARAMETER(lParam);

   switch (message)
   {
   case WM_INITDIALOG:
      return (INT_PTR)TRUE;

   case WM_COMMAND:
      if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
         EndDialog(hDlg, LOWORD(wParam));
         return (INT_PTR)TRUE;
      }
      break;
   }

   return (INT_PTR)FALSE;
}


/*
 *----------------------------------------------------------------------
 *
 * WndProcChild --
 *
 *    Message handler for the child window.
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
WndProcChild(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   if (g_vmr9OverlayApp != NULL) {
      switch (message) {
         case WM_CREATE:
            g_overlayWindowId = g_vmr9OverlayApp->CreateOverlay(hWnd);
            break;

         case WM_DESTROY:
            g_vmr9OverlayApp->DestroyOverlay(g_overlayWindowId);
            g_overlayWindowId = VDP_OVERLAY_WINDOW_ID_NONE;
            break;

         case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hDC = BeginPaint(hWnd, &ps);
            PaintWindow(hWnd, hDC, ps.fErase, FALSE);
            EndPaint(hWnd, &ps);
            return 0;
         }
      }
   }
   return DefWindowProc(hWnd, message, wParam, lParam);
}


/*
 *----------------------------------------------------------------------
 *
 * PaintWindow --
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
PaintWindow(HWND hWnd, HDC hDC, BOOL eraseBG, BOOL isMain)
{
   RECT rect;
   GetClientRect(hWnd, &rect);
   int x = (rect.left + rect.right) / 2;
   int y = (rect.top + rect.bottom) / 2;

   COLORREF bgColor = isMain
                    ? g_bgColorMain
                    : g_bgColorChild;

   TCHAR* msg = isMain
              ? _T("This is the main window")
              : _T("This is the child window");

   if (eraseBG) {
      HBRUSH hBrush = isMain ? g_hBgMain : g_hBgChild;
      FillRect(hDC, &rect, hBrush);
   }

   SetBkColor(hDC, bgColor);
   SetTextAlign(hDC, TA_CENTER|TA_BASELINE);
   TextOut(hDC, x, y, msg, (int)_tcslen(msg));
}


/*
 *----------------------------------------------------------------------
 *
 * SaveOverlaySettings --
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
SaveOverlaySettings(VDPOverlay_WindowId overlayWindowId)
{
   OverlaySettings& settings = g_overlaySettings;

   GetWindowPlacement(g_hWndMain,
                      &settings.mainWindowPlacement);

   settings.videoPath =
      g_vmr9OverlayApp->GetVideoPath(overlayWindowId);

   settings.overlayEnabled =
      g_vmr9OverlayApp->IsOverlayEnabled(overlayWindowId);

   settings.copyImages =
      g_vmr9OverlayApp->AreImagesCopied(overlayWindowId);

   settings.layoutMode =
      g_vmr9OverlayApp->GetLayoutMode(overlayWindowId);

   settings.bgColor =
      g_vmr9OverlayApp->GetBackgroundColor(overlayWindowId);

   g_vmr9OverlayApp->GetAreaRect(overlayWindowId,
                                 &settings.areaEnabled,
                                 &settings.areaClipped,
                                 &settings.areaRect);
}


/*
 *----------------------------------------------------------------------
 *
 * RestoreOverlaySettings --
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
RestoreOverlaySettings(VDPOverlay_WindowId overlayWindowId)
{
   HMENU hMenu = ::GetMenu(g_hWndMain);
   OverlaySettings& settings = g_overlaySettings;

   #define CHECK_MENU_ITEM(id, b) \
      ::CheckMenuItem(hMenu, id, ((b) ? MF_CHECKED : MF_UNCHECKED));

   if (settings.mainWindowPlacement.length != 0) {
      SetWindowPlacement(g_hWndMain, &settings.mainWindowPlacement);
   }

   CHECK_MENU_ITEM(ID_PLAYBACK_ENABLECHILDWINDOW, g_createChildWindow);

   g_vmr9OverlayApp->EnableOverlay(overlayWindowId, settings.overlayEnabled);
   CHECK_MENU_ITEM(ID_PLAYBACK_ENABLEOVERLAY, settings.overlayEnabled);

   g_vmr9OverlayApp->CopyImages(overlayWindowId, settings.copyImages);
   CHECK_MENU_ITEM(ID_PLAYBACK_COPYIMAGES, settings.copyImages);

   g_vmr9OverlayApp->SetLayoutMode(overlayWindowId, settings.layoutMode);
   UpdateLayoutMenu(overlayWindowId, -1);

   g_vmr9OverlayApp->SetBackgroundColor(overlayWindowId, settings.bgColor);
   UpdateBgColorMenu(overlayWindowId, -1);

   g_vmr9OverlayApp->SetAreaRect(overlayWindowId,
                                 settings.areaEnabled,
                                 settings.areaClipped,
                                 settings.areaRect);
   UpdateAreaMenu(overlayWindowId, -1);

   if (!settings.videoPath.empty()) {
      g_vmr9OverlayApp->OpenFile(overlayWindowId, settings.videoPath);
   }
}


/*
 *----------------------------------------------------------------------
 *
 * UpdateLayoutMenu --
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
UpdateLayoutMenu(VDPOverlay_WindowId overlayWindowId, int layoutId)
{
   VDPOverlay_LayoutMode layoutMode;

   if (layoutId == -1) {
      layoutMode = g_vmr9OverlayApp->GetLayoutMode(overlayWindowId);

      if (layoutMode == VDP_OVERLAY_LAYOUT_MAX) {
          layoutMode = VDP_OVERLAY_LAYOUT_DEFAULT;
      }

      layoutId =
         layoutMode == VDP_OVERLAY_LAYOUT_CENTER                ? ID_LAYOUT_CENTER     :
         layoutMode == VDP_OVERLAY_LAYOUT_TILE                  ? ID_LAYOUT_TILE       :
         layoutMode == VDP_OVERLAY_LAYOUT_SCALE                 ? ID_LAYOUT_SCALE      :
         layoutMode == VDP_OVERLAY_LAYOUT_SCALE_SHRINK_ONLY     ? ID_LAYOUT_SCALE2     :
         layoutMode == VDP_OVERLAY_LAYOUT_SCALE_MULTIPLE        ? ID_LAYOUT_SCALE3     :
         layoutMode == VDP_OVERLAY_LAYOUT_CROP                  ? ID_LAYOUT_CROP       :
         layoutMode == VDP_OVERLAY_LAYOUT_CROP_SHRINK_ONLY      ? ID_LAYOUT_CROP2      :
         layoutMode == VDP_OVERLAY_LAYOUT_CROP_MULTIPLE         ? ID_LAYOUT_CROP3      :
         layoutMode == VDP_OVERLAY_LAYOUT_LETTERBOX             ? ID_LAYOUT_LETTERBOX  :
         layoutMode == VDP_OVERLAY_LAYOUT_LETTERBOX_SHRINK_ONLY ? ID_LAYOUT_LETTERBOX2 :
         layoutMode == VDP_OVERLAY_LAYOUT_LETTERBOX_MULTIPLE    ? ID_LAYOUT_LETTERBOX3 : -1;

   } else {
      layoutMode =
         layoutId == ID_LAYOUT_CENTER     ? VDP_OVERLAY_LAYOUT_CENTER                :
         layoutId == ID_LAYOUT_TILE       ? VDP_OVERLAY_LAYOUT_TILE                  :
         layoutId == ID_LAYOUT_SCALE      ? VDP_OVERLAY_LAYOUT_SCALE                 :
         layoutId == ID_LAYOUT_SCALE2     ? VDP_OVERLAY_LAYOUT_SCALE_SHRINK_ONLY     :
         layoutId == ID_LAYOUT_SCALE3     ? VDP_OVERLAY_LAYOUT_SCALE_MULTIPLE        :
         layoutId == ID_LAYOUT_CROP       ? VDP_OVERLAY_LAYOUT_CROP                  :
         layoutId == ID_LAYOUT_CROP2      ? VDP_OVERLAY_LAYOUT_CROP_SHRINK_ONLY      :
         layoutId == ID_LAYOUT_CROP3      ? VDP_OVERLAY_LAYOUT_CROP_MULTIPLE         :
         layoutId == ID_LAYOUT_LETTERBOX  ? VDP_OVERLAY_LAYOUT_LETTERBOX             :
         layoutId == ID_LAYOUT_LETTERBOX2 ? VDP_OVERLAY_LAYOUT_LETTERBOX_SHRINK_ONLY :
         layoutId == ID_LAYOUT_LETTERBOX3 ? VDP_OVERLAY_LAYOUT_LETTERBOX_MULTIPLE    :
                                            VDP_OVERLAY_LAYOUT_DEFAULT               ;

      g_vmr9OverlayApp->SetLayoutMode(overlayWindowId, layoutMode);
   }

   #define UPDATE_LAYOUT_ITEM(id) \
      ::CheckMenuItem(hMenu, ID_LAYOUT_##id, \
                        (ID_LAYOUT_##id == layoutId ? MF_CHECKED : MF_UNCHECKED))

   HMENU hMenu = ::GetMenu(g_hWndMain);
   UPDATE_LAYOUT_ITEM(CENTER);
   UPDATE_LAYOUT_ITEM(TILE);
   UPDATE_LAYOUT_ITEM(SCALE);
   UPDATE_LAYOUT_ITEM(SCALE2);
   UPDATE_LAYOUT_ITEM(SCALE3);
   UPDATE_LAYOUT_ITEM(CROP);
   UPDATE_LAYOUT_ITEM(CROP2);
   UPDATE_LAYOUT_ITEM(CROP3);
   UPDATE_LAYOUT_ITEM(LETTERBOX);
   UPDATE_LAYOUT_ITEM(LETTERBOX2);
   UPDATE_LAYOUT_ITEM(LETTERBOX3);
}


/*
 *----------------------------------------------------------------------
 *
 * UpdateBgColorMenu --
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
UpdateBgColorMenu(VDPOverlay_WindowId overlayWindowId, int bgColorId)
{
   uint32 bgColor;

   if (bgColorId == -1) {
       bgColor = g_vmr9OverlayApp->GetBackgroundColor(overlayWindowId);

       bgColorId =
          bgColor == BGCOLOR_BLACK    ? ID_BACKGROUND_BLACK    :
          bgColor == BGCOLOR_WHITE    ? ID_BACKGROUND_WHITE    :
          bgColor == BGCOLOR_RED      ? ID_BACKGROUND_RED      :
          bgColor == BGCOLOR_BLUE     ? ID_BACKGROUND_BLUE     :
          bgColor == BGCOLOR_PURPLE   ? ID_BACKGROUND_PURPLE   :
          bgColor == BGCOLOR_DISABLED ? ID_BACKGROUND_DISABLED : bgColorId;

   } else {
      bgColor =
         bgColorId == ID_BACKGROUND_BLACK    ? BGCOLOR_BLACK    :
         bgColorId == ID_BACKGROUND_WHITE    ? BGCOLOR_WHITE    :
         bgColorId == ID_BACKGROUND_RED      ? BGCOLOR_RED      :
         bgColorId == ID_BACKGROUND_BLUE     ? BGCOLOR_BLUE     :
         bgColorId == ID_BACKGROUND_PURPLE   ? BGCOLOR_PURPLE   :
         bgColorId == ID_BACKGROUND_DISABLED ? BGCOLOR_DISABLED : BGCOLOR_DEFAULT;

         g_vmr9OverlayApp->SetBackgroundColor(overlayWindowId, bgColor);
   }

   #define UPDATE_BGCOLOR_ITEM(id) \
      ::CheckMenuItem(hMenu, ID_BACKGROUND_##id, \
                      (ID_BACKGROUND_##id == bgColorId ? MF_CHECKED : MF_UNCHECKED))

   HMENU hMenu = ::GetMenu(g_hWndMain);
   UPDATE_BGCOLOR_ITEM(BLACK);
   UPDATE_BGCOLOR_ITEM(WHITE);
   UPDATE_BGCOLOR_ITEM(RED);
   UPDATE_BGCOLOR_ITEM(BLUE);
   UPDATE_BGCOLOR_ITEM(PURPLE);
   UPDATE_BGCOLOR_ITEM(DISABLED);
}


/*
 *----------------------------------------------------------------------
 *
 * UpdateAreaMenu --
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
UpdateAreaMenu(VDPOverlay_WindowId overlayWindowId, int areaId)
{
   HMENU hMenu = ::GetMenu(g_hWndMain);
   OverlaySettings& settings = g_overlaySettings;

   RECT clientRect;
   if (!::GetClientRect(g_hWndMain, &clientRect)) {
      return;
   }

   bool enabled, clipped;
   VDPOverlay_Rect areaRect;
   g_vmr9OverlayApp->GetAreaRect(overlayWindowId, &enabled, &clipped, &areaRect);

   UINT sizeId = settings.areaSizeId;
   UINT posId = settings.areaPosId;
   bool updateAreaSize = false;
   bool updateAreaPos = false;
   bool updateArea = false;

   switch(areaId)
   {
   case ID_AREA_ENABLE:
      enabled = !enabled;
      updateArea = true;
      if (enabled && ::IsRectEmpty(&areaRect)) {
         updateAreaSize = true;
         updateAreaPos = true;
      }
      break;

   case ID_AREA_CLIPTOWINDOW:
      clipped = !clipped;
      updateArea = true;
      break;

   case ID_SIZE_16x9:
   case ID_SIZE_4x3:
   case ID_SIZE_1x1:
   case ID_SIZE_VIDEO:
   case ID_SIZE_WINDOW:
      settings.areaSizeId = sizeId = areaId;
      updateAreaSize = true;
      updateAreaPos = true;
      updateArea = true;
      break;

   case ID_POSITION_CENTER:
   case ID_POSITION_TOP_LEFT:
   case ID_POSITION_TOP_RIGHT:
   case ID_POSITION_BOTTOM_LEFT:
   case ID_POSITION_BOTTOM_RIGHT:
      settings.areaPosId = posId = areaId;
      updateAreaPos = true;
      updateArea = true;
      break;

   case -1:
   default:
      // no changes
      break;
   }


   if (updateArea) {
      LONG xOffset = 0;
      LONG yOffset = 0;
      LONG xMargin = 10;
      LONG yMargin = 10;
      POINT videoSize;

      #define RECT_WIDTH(r) ((r)->right - (r)->left)
      #define RECT_HEIGHT(r) ((r)->bottom - (r)->top)

      if (updateAreaSize) {
         switch(sizeId)
         {
         case ID_SIZE_16x9:
            ::SetRect(&areaRect, 0, 0, 800, 450);
            break;

         case ID_SIZE_4x3:
            ::SetRect(&areaRect, 0, 0, 800, 600);
            break;

         case ID_SIZE_1x1:
            ::SetRect(&areaRect, 0, 0, 600, 600);
            break;

         case ID_SIZE_VIDEO:
            if (g_vmr9OverlayApp->GetVideoSize(overlayWindowId, &videoSize)
                && videoSize.x > 0 && videoSize.y > 0) {
               ::SetRect(&areaRect, 0, 0, videoSize.x, videoSize.y);
               break;
            }
            // DO NOT BREAK

         case ID_SIZE_WINDOW:
            areaRect = clientRect;
            // ::InflateRect(&areaRect, -xMargin/2, -yMargin/2);
            break;
         }
      }

      if (updateAreaPos) {
         switch(posId)
         {
         case ID_POSITION_TOP_LEFT:
            xOffset = xMargin;
            yOffset = yMargin;
            break;

         case ID_POSITION_TOP_RIGHT:
            xOffset = (RECT_WIDTH(&clientRect) - RECT_WIDTH(&areaRect)) - xMargin;
            yOffset = yMargin;
            break;

         case ID_POSITION_BOTTOM_LEFT:
            xOffset = xMargin;
            yOffset = (RECT_HEIGHT(&clientRect) - RECT_HEIGHT(&areaRect)) - yMargin;
            break;

         case ID_POSITION_BOTTOM_RIGHT:
            xOffset = (RECT_WIDTH(&clientRect) - RECT_WIDTH(&areaRect)) - xMargin;
            yOffset = (RECT_HEIGHT(&clientRect) - RECT_HEIGHT(&areaRect)) - yMargin;
            break;

         case ID_POSITION_CENTER:
            xOffset = (RECT_WIDTH(&clientRect) - RECT_WIDTH(&areaRect)) / 2;
            yOffset = (RECT_HEIGHT(&clientRect) - RECT_HEIGHT(&areaRect)) / 2;
            break;
         }

         ::OffsetRect(&areaRect, xOffset - areaRect.left,
                                 yOffset - areaRect.top);
      }

      g_vmr9OverlayApp->SetAreaRect(overlayWindowId, enabled, clipped, areaRect);
   }


   #define AREA_BOOL_CHECKED(b) \
      ((b) ? MF_CHECKED : MF_UNCHECKED)

   #define UPDATE_AREA_BOOL(id, b) \
      ::CheckMenuItem(hMenu, ID_AREA_##id, AREA_BOOL_CHECKED(b))

   #define UPDATE_AREA_POS(pos) \
      ::CheckMenuItem(hMenu, ID_POSITION_##pos, \
                      AREA_BOOL_CHECKED(posId == ID_POSITION_##pos))

   #define UPDATE_AREA_SIZE(size) \
      ::CheckMenuItem(hMenu, ID_SIZE_##size, \
                      AREA_BOOL_CHECKED(sizeId == ID_SIZE_##size))

   UPDATE_AREA_BOOL(ENABLE, enabled);
   UPDATE_AREA_BOOL(CLIPTOWINDOW, clipped);

   UPDATE_AREA_POS(CENTER);
   UPDATE_AREA_POS(TOP_LEFT);
   UPDATE_AREA_POS(TOP_RIGHT);
   UPDATE_AREA_POS(BOTTOM_LEFT);
   UPDATE_AREA_POS(BOTTOM_RIGHT);

   UPDATE_AREA_SIZE(16x9);
   UPDATE_AREA_SIZE(4x3);
   UPDATE_AREA_SIZE(1x1);
   UPDATE_AREA_SIZE(VIDEO);
   UPDATE_AREA_SIZE(WINDOW);
}


/*
 *----------------------------------------------------------------------
 *
 * OnReady --
 *
 *----------------------------------------------------------------------
 */
void
VMR9OverlayApp::OnReady()
{
   LOG("Ready");
   // Wait until the overlay is ready
   // before calling RestoreOverlaySettings()
}


/*
 *----------------------------------------------------------------------
 *
 * OnNotReady --
 *
 *----------------------------------------------------------------------
 */
void
VMR9OverlayApp::OnNotReady()
{
   LOG("Not Ready");
   SaveOverlaySettings(g_overlayWindowId);
}


/*
 *----------------------------------------------------------------------
 *
 * OnOverlayReady --
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
VMR9OverlayApp::OnOverlayReady(VDPOverlay_WindowId overlayWindowId)
{
   LOG("Window 0x%x: overlay is ready", overlayWindowId);
   RestoreOverlaySettings(overlayWindowId);
}


/*
 *----------------------------------------------------------------------
 *
 * OnVideoReady --
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
VMR9OverlayApp::OnVideoReady(VDPOverlay_WindowId overlayWindowId)
{
   LOG("Window 0x%x: video is ready", overlayWindowId);

   bool enabled = false;
   GetAreaRect(overlayWindowId, &enabled, NULL, NULL);
   if (enabled) return;

   POINT videoSize;
   GetVideoSize(overlayWindowId, &videoSize);

   RECT rWindow;
   GetWindowRect(g_hWndMain, &rWindow);

   RECT rClient;
   GetClientRect(g_hWndMain, &rClient);

   LONG wWindow = rWindow.right - rWindow.left;
   LONG hWindow = rWindow.bottom - rWindow.top;

   LONG wClient = rClient.right - rClient.left;
   LONG hClient = rClient.bottom - rClient.top;

   LONG w = videoSize.x + wWindow - wClient;
   LONG h = videoSize.y + hWindow - hClient;

   int x = rWindow.left + (wWindow - w) / 2;
   int y = rWindow.top + (hWindow - h) / 2;

   if (x < 0) x = 0;
   if (y < 0) y = 0;

   HWND hWndInsertAfter = NULL;
   UINT flags = SWP_NOZORDER | SWP_NOACTIVATE;
   ::SetWindowPos(g_hWndMain, hWndInsertAfter, x, y, w, h, flags);
}
