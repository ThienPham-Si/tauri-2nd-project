/* ********************************************************************************* *
 * Copyright (C) 2017-2021 VMware, Inc.  All rights reserved. -- VMware Confidential *
 * ********************************************************************************* */

/*
 * vdpService_import.cpp
 *
 *    This file is a replacement for the import library vdpService.lib
 *    It is completely self contained, just include it in your project.
 *
 *    The difference between this code and the import library is that it
 *    knows where to find vdpService.dll (it does not rely on the PATH).
 *    Another difference is that if vdpService.dll can not be found your
 *    program will still load but the vdpService API functions will return
 *    an error.
 */

#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <tchar.h>

#include <vdpService.h>


/*
 * Define function pointer types for all the functions in vdpService.dll
 */
typedef Bool (*tServerInit)(
                  const char *token,
                  VDP_SERVICE_QUERY_INTERFACE *qi,
                  void **channelHandle);

typedef Bool (*tServerInitEx)(
                  const char *token,
                  const char *params,
                  VDP_SERVICE_QUERY_INTERFACE *qi,
                  void **channelHandle);

typedef Bool (*tServerInitLP)(
                  const char *token,
                  const char *params,
                  VDP_SERVICE_QUERY_INTERFACE *qi,
                  void **channelHandle);

typedef Bool (*tServerExit)();

typedef Bool (*tServerInit2)(
                  unsigned long sessionId,
                  const char *token,
                  VDP_SERVICE_QUERY_INTERFACE *qi,
                  void **channelHandle);

typedef Bool (*tServerInitEx2)(
                  unsigned long sessionId,
                  const char *token,
                  const char *params,
                  VDP_SERVICE_QUERY_INTERFACE *qi,
                  void **channelHandle);

typedef Bool (*tServerExit2)(
                  unsigned long sessionId);

typedef Bool (*tMultiServerInit)(
                  unsigned long sessionId,
                  const char *token,
                  const char *params,
                  VDP_SERVICE_QUERY_INTERFACE *qi,
                  VdpMultiServerID *phMultiServerId);

typedef Bool (*tMultiServerExit)(
                  VdpMultiServerID hMultiServerId,
                  void             *reserved);

/*
 *----------------------------------------------------------------------
 *
 * class VdpService
 *
 *----------------------------------------------------------------------
 */
class VdpService
{
public:
   VdpService();
   ~VdpService();

   void  Load();
   void  Unload();
   void  Reset();

   bool  AlreadyLoaded();
   bool  LoadFromRxAgentPath();

   HMODULE               m_hVdpDll;
   tServerInit           m_fServerInit;
   tServerInitEx         m_fServerInitEx;
   tServerInitLP         m_fServerInitLP;
   tServerExit           m_fServerExit;
   tServerInit2          m_fServerInit2;
   tServerInitEx2        m_fServerInitEx2;
   tServerExit2          m_fServerExit2;
   tMultiServerInit      m_fMultiServerInit;
   tMultiServerExit      m_fMultiServerExit;
};


/*
 * Create a global instance.
 * This will load vdpService.dll when the program loads.
 *
 * With a little effort you can delay load vdpService.dll,
 * but then you have to worry about locking during initialization.
 */
static VdpService g_vdpService;


/*
 *----------------------------------------------------------------------
 *
 * VdpService::VdpService
 *
 *----------------------------------------------------------------------
 */
VdpService::VdpService()
{
   Reset();
   Load();
}

VdpService::~VdpService()
{
   Unload();
}


/*
 *----------------------------------------------------------------------
 *
 * VdpService::Load
 *
 *    You must use the version of the vdpService.dll that ships
 *    with View to ensure compatibility with previous and future
 *    releases of View.  This is why the DLL is no longer provided
 *    as part of the SDK.
 *
 *----------------------------------------------------------------------
 */
void
VdpService::Load()
{
   if (m_hVdpDll != NULL) {
      return;
   }

   if (!AlreadyLoaded() &&
       !LoadFromRxAgentPath()) {
      return;
   }

   #define GET_VDP_ADDRESS(fname) \
      m_f##fname = (t##fname)::GetProcAddress(m_hVdpDll, "VDPService_" #fname)

   GET_VDP_ADDRESS(ServerInit);
   GET_VDP_ADDRESS(ServerInitEx);
   GET_VDP_ADDRESS(ServerInitLP);
   GET_VDP_ADDRESS(ServerExit);
   GET_VDP_ADDRESS(ServerInit2);
   GET_VDP_ADDRESS(ServerInitEx2);
   GET_VDP_ADDRESS(ServerExit2);
   GET_VDP_ADDRESS(MultiServerInit);
   GET_VDP_ADDRESS(MultiServerExit);
}


/*
 *----------------------------------------------------------------------
 *
 * VdpService::AlreadyLoaded
 *
 *    Check if there is already a copy of vdpService.dll loaded.
 *    This can happen if this code is called from the client side.
 *
 *    Note that even in the special case when the agent is also installed,
 *    you must use the version that is already loaded or you will get two
 *    copies of the DLL in memory which will cause problems.
 *
 *----------------------------------------------------------------------
 */
bool
VdpService::AlreadyLoaded()
{
   return GetModuleHandleEx(0, _T("vdpService.dll"), &m_hVdpDll) != 0;
}


/*
 *----------------------------------------------------------------------
 *
 * VdpService::LoadFromRxAgentPath
 *
 *----------------------------------------------------------------------
 */
bool
VdpService::LoadFromRxAgentPath()
{
   /*
    * This is where View places all the Remote Experience (RX) dlls.
    */
   static const HKEY   RX_AGENT_PATH_REG_HIVE = HKEY_LOCAL_MACHINE;
   static const TCHAR* RX_AGENT_PATH_REG_KEY  = _T("Software\\VMware, Inc.")
                                                _T("\\VMware VDM\\RemoteExperienceAgent");
   static const TCHAR* RX_AGENT_PATH_REG_NAME = _T("InstallPath");

   /*
    * Determine if I'm being compiled as 32 or 64 bit.
    * This is normally done with a preprocessor symbol
    * but I don't want this code to depend on such symbols
    * so that it can be easily added to other projects.
    */
   bool is32bit = sizeof(void*) == 4;

   /*
    * You need to look in the registry to find where the
    * dll is installed. For 32-bit appliations make sure
    * to look on non-wow side of the registry.
    */
   HKEY hKey = NULL;
   REGSAM regOpenMode = KEY_QUERY_VALUE
                      | (is32bit ? KEY_WOW64_64KEY : 0);

   LONG err = ::RegOpenKeyEx(RX_AGENT_PATH_REG_HIVE,
                             RX_AGENT_PATH_REG_KEY,
                             0, regOpenMode, &hKey);
   if (err != ERROR_SUCCESS) return false;

   TCHAR dllPath[MAX_PATH];
   DWORD dllPathSz = ARRAYSIZE(dllPath);
   DWORD dllPathType = REG_NONE;

   err = ::RegQueryValueEx(hKey, RX_AGENT_PATH_REG_NAME, NULL,
                           &dllPathType, (LPBYTE)dllPath, &dllPathSz);
   RegCloseKey(hKey);
   if (err != ERROR_SUCCESS) return false;
   if (dllPathType != REG_SZ) return false;

   if (!is32bit) {
      _tcscat_s(dllPath, ARRAYSIZE(dllPath), _T("\\x64"));
   }

   _tcscat_s(dllPath, ARRAYSIZE(dllPath), _T("\\vdpService.dll"));

   m_hVdpDll = ::LoadLibrary(dllPath);
   return m_hVdpDll != NULL;
}


/*
 *----------------------------------------------------------------------
 *
 * VdpService::Unload
 *
 *----------------------------------------------------------------------
 */
void
VdpService::Unload()
{
   if (m_hVdpDll != NULL) {
      ::FreeLibrary(m_hVdpDll);
   }

   Reset();
}


/*
 *----------------------------------------------------------------------
 *
 * VdpService::Reset
 *
 *    Clears out all the member variables
 *
 *----------------------------------------------------------------------
 */
void
VdpService::Reset()
{
   m_hVdpDll             = NULL;
   m_fServerInit         = NULL;
   m_fServerInitEx       = NULL;
   m_fServerInitLP       = NULL;
   m_fServerExit         = NULL;
   m_fServerInit2        = NULL;
   m_fServerInitEx2      = NULL;
   m_fServerExit2        = NULL;
   m_fMultiServerInit    = NULL;
   m_fMultiServerExit    = NULL;
}


/*
 *----------------------------------------------------------------------
 *
 * All the vdpService API functions pass through this macro.
 *
 * CALL_VDP_FUNCTION
 *    Calls a function in vdpService.dll if it's available.
 *
 *----------------------------------------------------------------------
 */
#define CALL_VDP_FUNCTION(fname, ...)                    \
   if (g_vdpService.m_f##fname != NULL) {                \
      return g_vdpService.m_f##fname(__VA_ARGS__);       \
   }                                                     \
   return FALSE                                          \


/*
 *----------------------------------------------------------------------
 *
 * VDP_SERVICE_API
 *
 *    Attributes applied to all the API functions
 *
 *----------------------------------------------------------------------
 */
#define VDP_SERVICE_API extern "C"


/*
 *----------------------------------------------------------------------
 *
 * Function VDPService_ServerInit
 *
 *----------------------------------------------------------------------
 */
VDP_SERVICE_API Bool
VDPService_ServerInit(const char *token,                 // IN
                      VDP_SERVICE_QUERY_INTERFACE *qi,   // OUT
                      void **channelHandle)              // OUT
{
   CALL_VDP_FUNCTION(ServerInit, token, qi, channelHandle);
}


/*
 *----------------------------------------------------------------------
 *
 * Function VDPService_ServerInitEx
 *
 *----------------------------------------------------------------------
 */
VDP_SERVICE_API Bool
VDPService_ServerInitEx(const char *token,               // IN
                        const char *params,              // IN
                        VDP_SERVICE_QUERY_INTERFACE *qi, // OUT
                        void **channelHandle)            // OUT
{
   CALL_VDP_FUNCTION(ServerInitEx, token, params, qi, channelHandle);
}


/*
 *----------------------------------------------------------------------
 *
 * Function VDPService_ServerInitLP
 *
 *----------------------------------------------------------------------
 */
VDP_SERVICE_API Bool
VDPService_ServerInitLP(const char *token,               // IN
                        const char *params,              // IN
                        VDP_SERVICE_QUERY_INTERFACE *qi, // OUT
                        void **channelHandle)            // OUT
{
   CALL_VDP_FUNCTION(ServerInitLP, token, params, qi, channelHandle);
}


/*
 *----------------------------------------------------------------------
 *
 * Function VDPService_ServerExit
 *
 *----------------------------------------------------------------------
 */
VDP_SERVICE_API Bool
VDPService_ServerExit()
{
   CALL_VDP_FUNCTION(ServerExit);
}


/*
 *----------------------------------------------------------------------
 *
 * Function VDPService_ServerInit2
 *
 *----------------------------------------------------------------------
 */
VDP_SERVICE_API Bool
VDPService_ServerInit2(unsigned long sessionId,          // IN
                       const char *token,                // IN
                       VDP_SERVICE_QUERY_INTERFACE *qi,  // OUT
                       void **channelHandle)             // OUT
{
   CALL_VDP_FUNCTION(ServerInit2, sessionId, token, qi, channelHandle);
}


/*
 *----------------------------------------------------------------------
 *
 * Function VDPService_ServerInitEx2
 *
 *----------------------------------------------------------------------
 */
VDP_SERVICE_API Bool
VDPService_ServerInitEx2(unsigned long sessionId,           // IN
                         const char *token,                 // IN
                         const char *params,                // IN
                         VDP_SERVICE_QUERY_INTERFACE *qi,   // OUT
                         void **channelHandle)              // OUT
{
   CALL_VDP_FUNCTION(ServerInitEx2, sessionId, token, params, qi, channelHandle);
}


/*
 *----------------------------------------------------------------------
 *
 * Function VDPService_ServerExit2
 *
 *----------------------------------------------------------------------
 */
VDP_SERVICE_API Bool
VDPService_ServerExit2(unsigned long sessionId)    // IN
{
   CALL_VDP_FUNCTION(ServerExit2, sessionId);
}


/*
 *----------------------------------------------------------------------
 *
 * Function VDPService_MultiServerInit
 *
 *----------------------------------------------------------------------
 */
VDP_SERVICE_API Bool
VDPService_MultiServerInit(unsigned long sessionId,            // IN
                           const char *token,                  // IN
                           const char *params,                 // IN
                           VDP_SERVICE_QUERY_INTERFACE *qi,    // OUT
                           VdpMultiServerID *phMultiServerId)  // OUT
{
   CALL_VDP_FUNCTION(MultiServerInit, sessionId, token, params, qi, phMultiServerId);
}


/*
 *----------------------------------------------------------------------
 *
 * Function VDPService_MultiServerExit
 *
 *----------------------------------------------------------------------
 */
VDP_SERVICE_API Bool
VDPService_MultiServerExit(VdpMultiServerID hMultiServerId,    // IN
                           void             *reserved)         // IN
{
   CALL_VDP_FUNCTION(MultiServerExit, hMultiServerId, reserved);
}
