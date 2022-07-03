/* ********************************************************************************* *
 * Copyright (C) 2013-2021 VMware, Inc.  All rights reserved. -- VMware Confidential *
 * ********************************************************************************* */

/*
 * stubs.c --
 *
 */


/*
 *----------------------------------------------------------------------
 *
 * Panic --
 *
 *    Stub for Panic to get Bora string library to be quiet.
 *
 * Results:
 *    None
 *
 * Side effects:
 *    None
 *
 *----------------------------------------------------------------------
 */

void
Panic(const char *fmt, // IN
      ...)             // IN
{
}


/*
 *----------------------------------------------------------------------
 *
 * Log --
 *
 *    Stub for Log.
 *    XXX: Map to LogUtils
 *
 * Results:
 *    None
 *
 * Side effects:
 *    None
 *
 *----------------------------------------------------------------------
 */

void
Log(const char *fmt, // IN
    ...)             // IN
{
}


/*
 *----------------------------------------------------------------------
 *
 * Warning --
 *
 *    Stub for Warning.
 *    XXX: Map to WarningUtils
 *
 * Results:
 *    None
 *
 * Side effects:
 *    None
 *
 *----------------------------------------------------------------------
 */

void
Warning(const char *fmt, // IN
        ...)             // IN
{
}
