/* ********************************************************************************* *
 * Copyright (C) 2011-2021 VMware, Inc.  All rights reserved. -- VMware Confidential *
 * ********************************************************************************* */

/*
 * RegisterDll.cpp --
 *
 * Adding these two functions, and exporting them through the .def file,
 * allows us to use RegSvr32.exe to register the DLL by making the
 * necessary updates to the registry.
 *
 */

#include "stdafx.h"

#define REG_DLL_HIVE          HKEY_LOCAL_MACHINE
#define REG_DLL_KEY           TEXT("SOFTWARE\\VMware, Inc.\\VMware VDPService\\Plugins\\LocalOverlay")
#define REG_DLL_NAME          TEXT("dll")
#define REG_DLL_ERROR_BASE    (HRESULT)0x80040200
#define DLL_NAME              TEXT("LocalOverlayClient.dll")


/*
 *----------------------------------------------------------------------
 *
 * Function DllRegisterServer --
 *
 *    Updates the registry to identify the DLL
 *    as an RDP virtual channel client.
 *
 * Results:
 *    Returns S_OK
 *
 * Side effects:
 *    None
 *
 *----------------------------------------------------------------------
 */
HRESULT __stdcall
DllRegisterServer_(const TCHAR* dllName,      // IN
                   HKEY regHive,             // IN
                   const TCHAR* regDllKey,    // IN
                   const TCHAR* regDllName)   // IN
{
   /*
    * Start by opening/creating the key
    */
   LONG winErr;
   HKEY hRegKey;
   DWORD keyCreated;
   winErr = ::RegCreateKeyEx(regHive,
                             regDllKey,
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             KEY_ALL_ACCESS,
                             NULL,
                             &hRegKey,
                             &keyCreated);
   if (winErr != ERROR_SUCCESS) {
      return REG_DLL_ERROR_BASE+0;
   }


   /*
    * Set the location of the addin in the registry
    */
   HMODULE hMod = ::GetModuleHandle(dllName);
   if (hMod == NULL) {
      ::RegCloseKey(hRegKey);
      return REG_DLL_ERROR_BASE+1;
   }

   TCHAR appPath[MAX_PATH];
   if (!::GetModuleFileName(hMod, appPath, ARRAYSIZE(appPath))) {
      ::RegCloseKey(hRegKey);
      return REG_DLL_ERROR_BASE+2;
   }

   const BYTE* data = (const BYTE*)appPath;
   DWORD dataLen = (DWORD)((::_tcslen(appPath)+1)*sizeof(TCHAR));

   winErr = ::RegSetValueEx(hRegKey, regDllName, 0, REG_SZ, data, dataLen);
   if (winErr != ERROR_SUCCESS) {
      ::RegCloseKey(hRegKey);
      return REG_DLL_ERROR_BASE+3;
   }

   ::RegCloseKey(hRegKey);
   return S_OK;
}

HRESULT __stdcall
DllRegisterServer()
{
   HRESULT regErr1 = S_OK;
   HRESULT regErr2 = S_OK;

   regErr1 = DllRegisterServer_(DLL_NAME,
                                REG_DLL_HIVE,
                                REG_DLL_KEY,
                                REG_DLL_NAME);

#ifdef REG_DLL_KEY2
   regErr2 = DllRegisterServer_(DLL_NAME,
                                REG_DLL_HIVE,
                                REG_DLL_KEY2,
                                REG_DLL_NAME);
#endif

   return regErr1 != S_OK ? regErr1 : regErr2;
}


/*
 *----------------------------------------------------------------------
 *
 * Function DllUnregisterServer --
 *
 *    Removes the registry entries added by DllRegisterServer
 *
 * Results:
 *    Returns S_OK
 *
 * Side effects:
 *    None
 *
 *----------------------------------------------------------------------
 */
HRESULT __stdcall
DllUnregisterServer()
{
   HRESULT regErr1 = S_OK;
   HRESULT regErr2 = S_OK;

   regErr1 = ::RegDeleteKey(REG_DLL_HIVE, REG_DLL_KEY) == ERROR_SUCCESS
           ? S_OK : REG_DLL_ERROR_BASE+0;

#ifdef REG_DLL_KEY2
   regErr2 = ::RegDeleteKey(REG_DLL_HIVE, REG_DLL_KEY2) == ERROR_SUCCESS
           ? S_OK : REG_DLL_ERROR_BASE+1;
#endif

   return regErr1 != S_OK ? regErr1 : regErr2;
}
