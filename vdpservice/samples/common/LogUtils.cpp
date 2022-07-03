/* ********************************************************************************* *
 * Copyright (C) 2011-2021 VMware, Inc.  All rights reserved. -- VMware Confidential *
 * ********************************************************************************* */

/*
 * LogUtils.cpp --
 *
 */

#include "stdafx.h"
#include <stdio.h>
#include <time.h>
#include <mutex>

#include "helpers.h"

#ifdef _WIN32
#include <sys\timeb.h>
#else
#include <sys/time.h>
#endif


/*
 *----------------------------------------------------------------------
 *
 * Class AutoCS()
 *
 *----------------------------------------------------------------------
 */
class AutoCS
{
public:
   AutoCS() { s_mutex.lock(); }
   virtual ~AutoCS() { s_mutex.unlock(); }

private:
   static std::recursive_mutex s_mutex;
};

std::recursive_mutex AutoCS::s_mutex;


/*
 *----------------------------------------------------------------------
 *
 * OS specific definitions
 *
 *----------------------------------------------------------------------
 */
#ifdef _WIN32
   #define LOGUTILS_OPEN_FILE(_FILE, _pchar, _mode) \
      (::fopen_s(&_FILE, _pchar, _mode) == 0)

   #define USER_MSG(msg) \
      ::MessageBoxA(NULL, (msg), __FUNCTION__, MB_OK)

#else

   #if defined(__APPLE__) || defined(__linux__)
      #define Posix_Fopen fopen
   #endif

   #define LOGUTILS_OPEN_FILE(_FILE, _pchar, _mode) \
      ((_FILE = Posix_Fopen(_pchar, _mode)) != NULL)

   #define USER_MSG(msg) \
      fprintf(stdout, "%s(): %s\n", __FUNCTION__, (msg))

   #define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))
#endif

static int LogUtilsGetLocalTime(struct tm *local);


/*
 *----------------------------------------------------------------------
 *
 * Function LogInit --
 *
 *----------------------------------------------------------------------
 */
static char         g_logFilename[1024];
static bool         g_isServer = false;
static bool         g_logFirst = true;
static VMThreadID   g_prevTID = 0;

#ifdef _WIN32
void
LogUtils::LogInit(const char* filename,   // IN
                  bool isServer)          // IN
{
   char username[MAX_PATH];
   char tempDir[MAX_PATH];

   AutoCS lock;

   if (*g_logFilename != '\0') {
      return;
   }

   g_isServer = isServer;

   int userLen = GetEnvironmentVariableA("USERNAME", username, ARRAYSIZE(username));
   if (userLen <= 0 || userLen >= ARRAYSIZE(username)) {
      USER_MSG("USERNAME environment variable undefined or too long");
      return;
   }

   int tempLen = GetEnvironmentVariableA("TEMP", tempDir, ARRAYSIZE(tempDir));
   if (tempLen <= 0 || tempLen >= ARRAYSIZE(tempDir)) {
      USER_MSG("TEMP environment variable undefined or too long");
      return;
   }

   /*
    * Remove a trailing backslash
    */
   if (tempDir[--tempLen] == '\\') {
      tempDir[tempLen--] = '\0';
   }

   /*
    * Windows server OS will place a session ID at the end
    * of the temp folder name that needs to be removed
    */
   for (int len=tempLen;  len > 0;  --len) {
      if (tempDir[len] == '\\') {
         tempDir[len] = '\0';
         break;
      }

      if (tempDir[len] < '0' ||
          tempDir[len] > '9') {
         break;
      }
   }

   _snprintf_s(g_logFilename, ARRAYSIZE(g_logFilename), _TRUNCATE, "%s\\vmware-%s\\%s-%s-%d.log",
               tempDir, username, filename, (g_isServer?"Server":"Client"), GetCurrentProcessId());

   // USER_MSG(g_logFilename);
}
#else
void
LogUtils::LogInit(const char* filename,   // IN
                  bool isServer)          // IN
{
   AutoCS lock;

   if (*g_logFilename != '\0') {
      return;
   }

   g_isServer = isServer;

   const char* user = getenv("USER");
   if (user == NULL) {
      user = getenv("USERNAME");
   }
   if (user == NULL) {
      USER_MSG("USER/USERNAME is undefined");
      return;
   }

   snprintf(g_logFilename, ARRAYSIZE(g_logFilename),
            "/tmp/vmware-%s/%s-client-%d.log", user, filename, getpid());

   // USER_MSG(g_logFilename);
}
#endif


/*
 *----------------------------------------------------------------------
 *
 * Function Log/vLog --
 *
 *----------------------------------------------------------------------
 */
void
LogUtils::Log(const char* funcName,   // IN
              const char* fmt, ...)   // IN
{
   va_list args;
   va_start(args, fmt);
   vLog(funcName, fmt, args);
   va_end(args);
}

void
LogUtils::vLog(const char* funcName,  // IN
               const char* fmt,       // IN
               va_list args)          // IN
{
   AutoCS lock;
   VMThreadID tid = GetCurrentThreadId();
   DWORD pid = GetCurrentProcessId();

   /*
    * Try to open the file
    */
   FILE* fp = NULL;
   const char* mode = g_logFirst ? "w" : "a";
   if (!LOGUTILS_OPEN_FILE(fp, g_logFilename, mode)) {
      return;
   }

   /*
    * Put a separator in the logs when there's a thread switch
    */
   if (g_logFirst) {
      g_prevTID = tid;
      g_logFirst = false;

   } else if (g_prevTID != tid) {
      g_prevTID = tid;
      ::fprintf(fp, "-------------------------------------\n");
   }

   /*
    * Generate the log message with the some useful information
    */
   struct tm tm64;
   int ms = LogUtilsGetLocalTime(&tm64);

#ifdef _WIN32
   ::fprintf(fp, "%04d-%02d-%02d %2d:%02d:%02d.%03d <%04X> [%04X] - %s",
             tm64.tm_year+1900, tm64.tm_mon+1, tm64.tm_mday,
             tm64.tm_hour, tm64.tm_min, tm64.tm_sec, ms,
             tid, pid, funcName);
#else
   ::fprintf(fp, "%04d-%02d-%02d %2d:%02d:%02d.%03d <%08lX> [%04X] - %s",
             tm64.tm_year+1900, tm64.tm_mon+1, tm64.tm_mday,
             tm64.tm_hour, tm64.tm_min, tm64.tm_sec, ms,
             (unsigned long)tid, pid, funcName);
#endif

   ::vfprintf (fp, fmt, args);
   ::putc('\n', fp);
   ::fclose(fp);
}


/*
 *----------------------------------------------------------------------
 *
 * LogUtilsGetLocalTime --
 *
 *    Fills the given tm struct with local time info.
 Returns the
 *    milliseconds past the current second as well.
 *
 * Results:
 *    Milliseconds.
 *
 * Side effects:
 *    None
 *
 *----------------------------------------------------------------------
 */

#ifdef _WIN32
static int
LogUtilsGetLocalTime(struct tm *local) // OUT
{
   struct __timeb64 tb64;
   _ftime64_s(&tb64);
   _localtime64_s(local, &tb64.time);
   return tb64.millitm;
}
#else
static int
LogUtilsGetLocalTime(struct tm *local) // OUT
{
   time_t seconds;
   struct tm *tmp;
   struct timeval tv;

   time(&seconds);
   tmp = localtime(&seconds);

   *local = *tmp;

   gettimeofday(&tv, NULL);
   return tv.tv_usec / 1000;
}
#endif


/*
 *----------------------------------------------------------------------
 *
 * Class FunctionTrace --
 *
 *    Logs an "Enter" message when the object is created and an "Exit"
 *    message when the object is deleted.  This makes it easy to track
 *    the enter/exit of a function or scope.
 *
 *----------------------------------------------------------------------
 */
FunctionTrace::FunctionTrace(const char* funcName) // IN
{
   m_funcName = funcName;
   m_exitMsg[0] = '\0';
   LogUtils::Log(m_funcName, "Enter");
}

FunctionTrace::FunctionTrace(const char* funcName, // IN
                             const char* fmt, ...) // IN
{
   m_funcName = funcName;
   m_exitMsg[0] = '\0';

   char msg[sizeof m_exitMsg];
   va_list args;
   va_start(args, fmt);
   _vsnprintf_s(msg, sizeof msg, _TRUNCATE, fmt, args);
   va_end(args);

   LogUtils::Log(funcName, "Enter - %s", msg);
}

FunctionTrace::~FunctionTrace()
{
   if (m_exitMsg[0] == '\0') {
      LogUtils::Log(m_funcName, "Exit");
   }
   else {
      LogUtils::Log(m_funcName, "Exit  - %s", m_exitMsg);
   }
}

void
FunctionTrace::SetExitMsg(const char* fmt, ...) // IN
{
   va_list args;
   va_start(args, fmt);
   _vsnprintf_s(m_exitMsg, sizeof m_exitMsg, _TRUNCATE, fmt, args);
   va_end(args);
}
