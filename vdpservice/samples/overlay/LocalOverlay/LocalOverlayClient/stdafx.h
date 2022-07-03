/* ********************************************************************************* *
 * Copyright (C) 2011-2021 VMware, Inc.  All rights reserved. -- VMware Confidential *
 * ********************************************************************************* */

/*
 * stdafx.h --
 *
 */

#pragma once

#ifdef _WIN32
   #ifndef WIN32_LEAN_AND_MEAN
      #define WIN32_LEAN_AND_MEAN
   #endif

   #include "targetver.h"
   #include <windows.h>
   #include <tchar.h>
   #include <intrin.h>
   #include <conio.h>

#else // _WIN32

   #ifndef USE_WIN_DWORD_RANGE
      #define USE_WIN_DWORD_RANGE
   #endif

   #include "wintypes.h"
#endif // _WIN32

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "helpers.h"
#include "LogUtils.h"

#define LOCAL_OVERLAY_TOKEN_NAME   "LocalOverlay"

enum {
   LOCAL_OVERLAY_ENABLE = 1,
   LOCAL_OVERLAY_DISABLE,
   LOCAL_OVERLAY_SET_LAYOUT_MODE,
   LOCAL_OVERLAY_SET_LAYER,
   LOCAL_OVERLAY_SET_COLOR,
   LOCAL_OVERLAY_SET_POSITION,
   LOCAL_OVERLAY_SET_SIZE,
   LOCAL_OVERLAY_SET_FORMAT
};
