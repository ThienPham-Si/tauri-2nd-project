/* ********************************************************************************* *
 * Copyright (C) 2011-2021 VMware, Inc.  All rights reserved. -- VMware Confidential *
 * ********************************************************************************* */

/*
 * dllmain.cpp --
 *
 */

#include "stdafx.h"

HMODULE g_hModule;

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD  reason,
                      LPVOID lpReserved)
{
   switch (reason)
   {
   case DLL_PROCESS_ATTACH:
      g_hModule = hModule;
      break;

   case DLL_THREAD_ATTACH:
   case DLL_THREAD_DETACH:
   case DLL_PROCESS_DETACH:
      break;
   }

   return TRUE;
}
