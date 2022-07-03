/* ********************************************************************************* *
 * Copyright (C) 2012-2021 VMware, Inc.  All rights reserved. -- VMware Confidential *
 * ********************************************************************************* */

/*
 * helpers.h --
 *
 */

#pragma once

#ifndef __HELPERS_H__
#define __HELPERS_H__

#include <stdio.h>


#define VM_ASSERT(cond)                            \
   do {                                            \
      if (!(cond)) {                               \
         fprintf(stderr, "%s(%d)@%s: ASSERT(%s)\n",\
                  __FILE__, __LINE__,              \
                  __FUNCTION__, #cond);            \
         exit(1);                                  \
      }                                            \
   } while(false)


char *Str_Strcpy(char *buf, const char *src, size_t maxSize);


#ifdef _WIN32

typedef DWORD VMThreadID;


#else // !_WIN32

typedef pthread_t VMThreadID;
#define GetCurrentThreadId() (pthread_self())
#define GetCurrentProcessId() ((unsigned int)getpid())
#define Sleep(_interval) usleep(_interval * 1000);

#define _strnicmp strncasecmp
#define _stricmp strcasecmp
#define _wcsicmp wcscasecmp
#define _snprintf snprintf
#define _vsnprintf vsnprintf
#define _strdup strdup

#define HELPER_ARGS(...) , ##__VA_ARGS__

#ifndef strncpy_s
   #define strncpy_s(_dest, _n, _source, _count) { \
      strncpy(_dest, _source, (_n) - 1); \
      _dest[(_n) - 1] = '\0'; \
     }
#endif

#ifndef strcpy_s
   #define strcpy_s(_dest, _n, _source) \
      Str_Strcpy(_dest, _source, _n)
#endif

#ifndef strcat_s
   #define strcat_s(_dest, _n, _source) \
      strcat(_dest, _source)
#endif

#ifndef _snprintf_s
   #define _snprintf_s(_msgp, _msgl, _cnt, _fmt, ...) \
      snprintf(_msgp, _msgl, _fmt HELPER_ARGS(__VA_ARGS__))
#endif

#ifndef _vsnprintf_s
   #define _vsnprintf_s(_msgp, _msgl, _cnt, _fmt, _va) \
      vsnprintf(_msgp, _msgl, _fmt, _va)
#endif

#endif // ifdef _WIN32

#endif // __HELPERS_H__
