/* ********************************************************************************* *
 * Copyright (C) 2018-2021 VMware, Inc.  All rights reserved. -- VMware Confidential *
 * ********************************************************************************* */

/*
 * helpers.cpp --
 *
 */

#include "stdafx.h"
#include "helpers.h"


/*
 *----------------------------------------------------------------------
 *
 * Str_Strcpy --
 *
 *    Wrapper for strcpy that checks for buffer overruns.
 *
 * Results:
 *    Same as strcpy.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

char *
Str_Strcpy(char *buf,       // OUT
           const char *src, // IN
           size_t maxSize)  // IN
{
   size_t len = 0;
   if (buf == NULL || src == NULL || (len = strlen(src)) >= maxSize) {
      return NULL;
   }
   return (char *)memcpy(buf, src, len + 1);
}
