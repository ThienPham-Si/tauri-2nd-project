/* ********************************************************************************* *
 * Copyright (C) 2011-2021 VMware, Inc.  All rights reserved. -- VMware Confidential *
 * ********************************************************************************* */

/*
 * LogUtils.h --
 *
 */

#pragma once

namespace LogUtils
{
   void LogInit(const char* filename, bool isServer);
   void Log(const char* funcName, const char* fmt, ...);
   void vLog(const char* funcName, const char* fmt, va_list args);
};


#ifdef _WIN32
#define LOG_FUNC_NAME __FUNCTION__ "(): "
#else
#define LOG_FUNC_NAME __FUNCTION__
#endif
#define LOG(...) LogUtils::Log(LOG_FUNC_NAME, __VA_ARGS__)


/*
 *----------------------------------------------------------------------
 *
 * Class FunctionTrace
 * Macro FUNCTION_TRACE
 * Macro FUNCTION_TRACE_MSG
 * Macro FUNCTION_EXIT_MSG
 *
 * Liberal use of the FUNCTION_TRACE macro can help you learn the flow of the code.
 *
 *----------------------------------------------------------------------
 */
class FunctionTrace
{
public:
   FunctionTrace(const char* funcName);
   FunctionTrace(const char* funcName, const char* fmt, ...);
   virtual ~FunctionTrace();

   void SetExitMsg(const char* fmt, ...);
   const char* m_funcName;
   char m_exitMsg[128];
};

#define FUNCTION_TRACE \
   FunctionTrace functionTraceTmp (LOG_FUNC_NAME)

#define FUNCTION_TRACE_MSG(...) \
   FunctionTrace functionTraceTmp (LOG_FUNC_NAME, __VA_ARGS__)

#define FUNCTION_EXIT_MSG(...) \
   functionTraceTmp.SetExitMsg(__VA_ARGS__)


/*
 *----------------------------------------------------------------------
 *
 * Macro CASE_RETURN --
 *
 *    Helper macro used to return a string based on the value of
 *    case symbol.
 *
 * Results:
 *    Returns a string.
 *
 * Side effects:
 *    The invoking function is exited if the case value matches.
 *
 *----------------------------------------------------------------------
 */
#define CASE_RETURN(a,b) case a##b: return #b;
