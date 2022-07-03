/* ********************************************************************************* *
 * Copyright (C) 2011-2021 VMware, Inc.  All rights reserved. -- VMware Confidential *
 * ********************************************************************************* */

/*
 * vmware.h -- common VMware definitions used in vdpService header files
 *
 */

#ifndef _VMWARE_H_
#define _VMWARE_H_

#include <stddef.h>

#include <cinttypes>
typedef uint64_t    uint64;
typedef  int64_t     int64;
typedef uint32_t    uint32;
typedef  int32_t     int32;
typedef uint16_t    uint16;
typedef  int16_t     int16;
typedef  uint8_t     uint8;
typedef   int8_t      int8;


typedef char           Bool;
#ifndef FALSE
#define FALSE          0
#endif
#ifndef TRUE
#define TRUE           1
#endif


typedef struct VMPoint {
   int x, y;
} VMPoint;

#if defined _WIN32
struct tagRECT;
typedef struct tagRECT VMRect;
#else
typedef struct VMRect {
   int left;
   int top;
   int right;
   int bottom;
} VMRect;
#endif

#endif // ifndef _VMWARE_H_
