/* ********************************************************************************* *
 * Copyright (C) 2011-2021 VMware, Inc.  All rights reserved. -- VMware Confidential *
 * ********************************************************************************* */

/*
 * StdAfx.h --
 *
 */

#pragma once

/*
 * Windows Header Files
 */
#include <windows.h>
#include <objbase.h>

/*
 * C RunTime Header Files
 */
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <time.h>
#include <malloc.h>
#include <memory.h>
#include <comdef.h>
#include <commdlg.h>
#include <intrin.h>
#include <string>
#include <map>

/*
 * DirectShow Header Files
 */
#undef INTSAFE_E_ARITHMETIC_OVERFLOW
#include <streams.h>
#include <d3d9.h>
#include <vmr9.h>


/*
 * This macro is required by the DShow code
 */
#define FAIL_RET_LOG(x) FAIL_RET_LOGFUNC(LOG, x);
#define FAIL_RET_MSG(x) FAIL_RET_LOGFUNC(FUNCTION_EXIT_MSG, x);

#define FAIL_RET_LOGFUNC(logfunc, x)                     \
   do {                                                  \
      if (FAILED(hr=(x))) {                              \
         logfunc("%s (failed with 0x%x)", #x, hr);       \
         return hr;                                      \
      }                                                  \
      else {                                             \
         /* LOG("%s (success with 0x%x)", #x, hr); */    \
      }                                                  \
   } while(0)


#define LOG_BOOL(b) ((b) ? "TRUE" : "FALSE")
#define LOG_WSTR(s) ((s) ? s : L"(null)")


/*
 * These are defined by both Windows and VMware
 * although the Window's header file says they
 * aren't used any more.
 */
#undef LODWORD
#undef HIDWORD


/*
 * Local Header Files
 */
#include "vmware.h"
#include "LogUtils.h"


/*
 * Global variables
 */
extern HMODULE g_hModule;


/*
 * This is the token name shared by the guest application
 * and client plugin so that VDPService knows they go together.
 */
#define VMR9_OVERLAY_TOKEN_NAME "VMR9Overlay"


/*
 * Messages passed between guest and client
 */
typedef enum
{
   // Guest to client events
   VMR9_OVERLAY_FILE_OPEN,
   VMR9_OVERLAY_FILE_CLOSE,
   VMR9_OVERLAY_PLAYBACK_START,
   VMR9_OVERLAY_PLAYBACK_STOP,
   VMR9_OVERLAY_COPY_IMAGES,

   // Client to guest events
   VMR9_OVERLAY_SET_SIZE
} VMR9OverlayMessages;
