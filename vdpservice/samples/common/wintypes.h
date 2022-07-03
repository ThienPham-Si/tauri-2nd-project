/* ********************************************************************************* *
 * Copyright (C) 2011-2021 VMware, Inc.  All rights reserved. -- VMware Confidential *
 * ********************************************************************************* */

/*
 * wintypes.h --
 *
 */

#ifndef WINTYPES_H
#define WINTYPES_H

#include <wchar.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>

#define TRUE 1
#define FALSE 0

#ifndef ARRAYSIZE
#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))
#endif

typedef unsigned char BYTE;
typedef BYTE *PBYTE;
typedef int BOOL;
typedef BYTE BOOLEAN;
typedef char CHAR;
typedef char CCHAR;
typedef long LONG;

#ifndef USE_WIN_DWORD_RANGE
#ifdef __APPLE__
typedef uint32_t      DWORD;
#else
typedef unsigned long DWORD;
#endif
#else
typedef unsigned int DWORD;
#endif

typedef void* HMODULE;
typedef void* HANDLE;
typedef int INT;
typedef char* LPSTR;
typedef void* LPVOID;
typedef DWORD * LPDWORD;
typedef CHAR * LPSTR;
typedef char* PCHAR;
typedef unsigned long* PULONG;
typedef void* PVOID;
typedef short SHORT;
typedef unsigned char UCHAR;
typedef UCHAR *PUCHAR;
typedef unsigned int UINT;
typedef unsigned long long ULONG64;
typedef unsigned int ULONG32;
typedef unsigned long ULONG;
typedef ULONG HRESULT;
typedef unsigned int UINT32;
typedef UINT32* PUINT32;
typedef int INT32;
typedef unsigned short UINT16;
typedef unsigned char UINT8;
typedef unsigned short USHORT;
typedef void VOID;
typedef const CHAR *LPCSTR;
typedef void* HMODULE;
typedef unsigned short WORD;

typedef DWORD ACCESS_MASK;
typedef ACCESS_MASK* PACCESS_MASK;

/* Check whether the compiler turns on the -fshort-wchar flag.
 * In Android WCHAR_MAX is defined as INT_MAX in wchar.h, the ndk compiler
 * won't use the value of INT_MAX for the pre-compile condition check, so add
 * check for __ANDROID__ macro since it's known that wchar_t is 4-Byte in
 * Android.
 */
#if defined __ANDROID__ || WCHAR_MAX > 0xFFFFu
/*
 * The wchar_t i.e. widechar implementation is platform and compiler
 * dependent. The wchar_t is 2-byte for VC++ & 4-Byte for G++.
 * Since few WTS functions (QuerySessionInformationA, QuerySessionInformationW)
 * expect input in widechar format and since the server is Windows-based,
 * it is the responsibility of client to construct a widechar string as
 * expected by Windows and then send it over pcoip.
 * We use a custom datatype - LWSTR - which is unsigned short (2-byte) for G++.
 */
typedef unsigned short LWSTR;
typedef unsigned short WCHAR;
typedef WCHAR *PWSTR;
typedef LWSTR* LPWSTR;

#else // for WCHAR

typedef wchar_t WCHAR;
typedef WCHAR *PWSTR;
typedef WCHAR LWSTR;
typedef WCHAR *LPWSTR;

#endif // for WCHAR

#define FAR
#define CALLBACK

typedef unsigned long long __int64;
#define __int32 uint32_t
#define uint32 uint32_t
#define int32 int32_t

#if defined(_WIN64)
 typedef unsigned __int64 UINT_PTR;
#else
 typedef unsigned int UINT_PTR;
#endif

#if defined(_WIN64)
 typedef unsigned __int64 ULONG_PTR;
#else
 typedef unsigned long ULONG_PTR;
#endif

typedef ULONG_PTR SIZE_T;

#if defined(_WIN64)
 typedef __int64 LONG_PTR;
#else
 typedef long LONG_PTR;
#endif

typedef HANDLE HKEY;
typedef HANDLE HWND;

typedef UINT_PTR WPARAM;
typedef LONG_PTR LPARAM;

/*
 * Definition of LPTSTR from WinNT.h
 */
#ifdef UNICODE
 typedef LPWSTR LPTSTR;
#else
 typedef LPSTR LPTSTR;
#endif

#ifdef UNICODE
 typedef LPCWSTR LPCTSTR;
#else
 typedef LPCSTR LPCTSTR;
#endif

#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((HANDLE) -1)
#endif

#ifndef WTS_CURRENT_SESSION
#define WTS_CURRENT_SESSION ((DWORD)-1)
#endif

#define VCAPITYPE

#define _ASSERT(expr) assert(expr)

#define __cdecl

#define NTAPI
#define WINAPI
#define IN
#define OUT

#define WAIT_OBJECT_0    0
#define WAIT_TIMEOUT     258L
#define WAIT_FAILED      UINT_MAX
#define INFINITE         UINT_MAX

#define MAX_COMPUTERNAME_LENGTH 64

typedef LONG NTSTATUS;
#define STATUS_SUCCESS                 0x00000000
#define STATUS_NOTIFY_ENUM_DIR         0x0000010C
#define STATUS_DEVICE_BUSY             0x80000011
#define STATUS_NO_MORE_FILES           0x80000006
#define STATUS_UNSUCCESSFUL            0xC0000001
#define STATUS_NOT_IMPLEMENTED         0xC0000002
#define STATUS_INVALID_HANDLE          0xC0000008
#define STATUS_INVALID_PARAMETER       0xC000000D
#define STATUS_NO_SUCH_FILE            0xC000000F
#define STATUS_END_OF_FILE             0xC0000011
#define STATUS_NO_MEMORY               0xC0000017
#define STATUS_ACCESS_DENIED           0xC0000022
#define STATUS_BUFFER_TOO_SMALL        0xC0000023
#define STATUS_OBJECT_NAME_NOT_FOUND   0xC0000034
#define STATUS_OBJECT_NAME_COLLISION   0xC0000035
#define STATUS_LOCK_NOT_GRANTED        0xC0000055
#define STATUS_DISK_FULL               0xC000007F
#define STATUS_FILE_IS_A_DIRECTORY     0xC00000BA
#define STATUS_NOT_A_DIRECTORY         0xC0000103
#define STATUS_CANCELLED               0xC0000120
#define STATUS_DEVICE_REMOVED          0xC00002B6
#define STATUS_DIRECTORY_NOT_EMPTY     0xC0000101

#ifndef MAX_PATH
#define MAX_PATH 256
#endif

#ifndef ERROR_SUCCESS
#define ERROR_SUCCESS 0
#endif

#endif

