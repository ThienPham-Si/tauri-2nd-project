/* **************************************************************************** *
 * Copyright (C) 2011-2021 VMware, Inc.  All rights reserved. -- VMware Confidential *
 * **************************************************************************** */

/*
 * param.cpp --
 *
 *     win32 Command line parameter parser(mimic GNU getopt).
 */

#include "stdafx.h"
#include "RPCManager.h"
#include "param.h"

/* static (global) variables that are specified as exported by getopt() */
static char *optarg = NULL;    /* pointer to the start of the option argument  */
static int optind = 1;         /* number of the next argv[] to be evaluated    */
static int opterr = 1;
static int optopt;

/* handle possible future character set concerns by putting this in a macro */
#define _next_char(string)  (char)(*(string+1))

/*
 *----------------------------------------------------------------------
 *
 * getopt --
 *
 *     command line option parser.
 *
 * Results:
 *     Returns the next option character in argv.
 *     EOF if all options have been processed.
 *
 * Side Effects:
 *     None.
 *
 *----------------------------------------------------------------------
 */

int
getopt(int argc,             // IN
       char **argv,          // IN
       char const *opstring) // IN
{
   static char *pIndexPosition = NULL; /* place inside current argv string */
   char *pArgString = NULL;            /* where to start from next */
   const char *pOptString;             /* the string in our program */

   if (pIndexPosition != NULL) {
      /* we last left off inside an argv string */
      if (*(++pIndexPosition)) {
         /* there is more to come in the most recent argv */
         pArgString = pIndexPosition;
      }
   }

   if (pArgString == NULL) {
      /* we didn't leave off in the middle of an argv string */
      if (optind >= argc) {
         /* more command-line arguments than the argument count */
         pIndexPosition = NULL;  /* not in the middle of anything */
         optind = 1;

         return EOF;             /* used up all command-line arguments */
      }

      /*---------------------------------------------------------------------
       * If the next argv[] is not an option, there can be no more options.
       *-------------------------------------------------------------------*/
      pArgString = argv[optind++]; /* set this to the next argument ptr */

      if (('/' != *pArgString) && /* doesn't start with a slash or a dash? */
          ('-' != *pArgString)) {
         --optind;               /* point to current arg once we're done */
         optarg = NULL;          /* no argument follows the option */
         pIndexPosition = NULL;  /* not in the middle of anything */

         return EOF;             /* used up all the command-line flags */
      }

      /* check for special end-of-flags markers */
      if ((strcmp(pArgString, "-") == 0) ||
          (strcmp(pArgString, "--") == 0)) {
         optarg = NULL;          /* no argument follows the option */
         pIndexPosition = NULL;  /* not in the middle of anything */

         return EOF;             /* encountered the special flag */
      }

      pArgString++;               /* look past the / or - */
   }

   if (':' == *pArgString) {       /* is it a colon? */
      /*---------------------------------------------------------------------
       * Rare case
       *-------------------------------------------------------------------*/
      if (opterr) {
      }
      optopt = ':';

      return '?';
   }

   if ((pOptString = strchr(opstring, *pArgString)) == 0) {
      /*---------------------------------------------------------------------
       * The letter on the command-line wasn't any good.
       *-------------------------------------------------------------------*/
      optarg = NULL;              /* no argument follows the option */
      pIndexPosition = NULL;      /* not in the middle of anything */
      if (opterr) {
      }
      optopt = (int)*pArgString;

      return '?';
   }

   /*---------------------------------------------------------------------
    * The letter on the command-line matches one we expect to see
    *-------------------------------------------------------------------*/
   if (':' == _next_char(pOptString)) { /* is the next letter a colon? */
      /* It is a colon.  Look for an argument string. */
      if ('\0' != _next_char(pArgString)) {  /* argument in this argv? */
         optarg = &pArgString[1];   /* Yes, it is */
      } else {
         /*-------------------------------------------------------------
          * The argument string must be in the next argv.
          * But, what if there is none (bad input from the user)?
          * In that case, return the letter, and optarg as NULL.
          *-----------------------------------------------------------*/
         if (optind < argc) {
            optarg = argv[optind++];
         } else {
            optarg = NULL;
            if (opterr) {
            }
            optopt = (int)*pArgString;

            return '?';
         }
      }
      pIndexPosition = NULL;  /* not in the middle of anything */
   } else {
      /* it's not a colon, so just return the letter */
      optarg = NULL;          /* no argument follows the option */
      pIndexPosition = pArgString;    /* point to the letter we're on */
   }

   return (int)*pArgString;    /* return the letter that matched */
}


/*
 *----------------------------------------------------------------------
 *
 * Usage --
 *
 *     Print out help page.
 *
 * Results:
 *     None
 *
 * Side Effects:
 *     None.
 *
 *----------------------------------------------------------------------
 */

static void
Usage(void)
{
   printf("Usage: PingRPCExe [-h] [-t type] [-s size] [-n count] [-d delay]\n");
   printf("                  [-i sessionId] [-c] [-e]\n");
   printf("Options:\n");
   printf("    -t       specify channel type.\n");
   printf("             main    -- ping send via main channel.(default)\n");
   printf("             tcp     -- ping send via tcp sidechannel.\n");
   printf("             tcpRaw  -- ping send via tcp streamData mode.\n");
   printf("             vchan   -- ping send via virtual sidechannel.\n");
   printf("    -s       specify ping packet size.\n");
   printf("    -n       Number of ping request. (default 1)\n");
   printf("    -d       Number of millisecond need to wait before exit.\n");
   printf("    -i       send ping packet to which windows session.\n");
   printf("             default is current session.\n");
   printf("             Request high priviledge for cross session communication.\n");
   printf("    -c       Packet will be compressed.(not for type=main)\n");
   printf("    -e       Packet will be encrypted.(Only for tcp and tcpRaw)\n");
   printf("    -p       Ping run in \"post\" mode. (No ack/OnDone needed from peer)\n");
   printf("    -h       Print usage\n");
   printf("Examples:\n");
   printf("    PingRPCExe -h\n");
   printf("    PingRPCExe -n 5\n");
   printf("    PingRPCExe -t -s 1280 -p\n");
   printf("\n");
}


/*
 *----------------------------------------------------------------------
 *
 * ParseOptions --
 *
 *     Parse command line options.
 *
 * Results:
 *     True if everything goes well.
 *
 * Side Effects:
 *     None.
 *
 *----------------------------------------------------------------------
 */

bool
ParseOptions(PingOptions& options,           // OUT
             int  argc,                      // IN
             char **argv)                    // IN
{
   int opt;
   bool ret = true;
   const char *channelType;

   // Set default value
   options.type = VDPSERVICE_MAIN_CHANNEL;
   options.size = 0;
   options.n = PING_MIN_NUMBER;
   options.delay = 0;
   options.sid = CURRENT_SESSION;
   options.compressEnabled = false;
   options.encryptionEnabled = false;
   options.postMode = false;
   channelType = "main channel";

   // Print help page and run ping with default parametr.
   if (argc <= 1) {
      Usage();
      return ret;
   }

   while ((opt = getopt(argc, argv, "t:s:n:d:i:ceph")) != EOF) {
      switch (opt) {
      case 't':
         if (_stricmp(optarg, "vchan") == 0) {
            channelType = "virtual sidechannel";
            options.type = VDPSERVICE_VCHAN_CHANNEL;
         } else if (_stricmp(optarg, "tcp") == 0) {
            channelType = "tcp sidechannel";
            options.type = VDPSERVICE_TCP_CHANNEL;
         } else if (_stricmp(optarg, "tcpRaw") == 0) {
            channelType = "tcp raw socket";
            options.type = VDPSERVICE_TCPRAW_CHANNEL;
         }
         break;
      case 's':
         options.size = atoi(optarg);
         if (options.size < 0) {
            printf("Warning : Input ping size is too small\n");
            options.size = 0;
         }
         break;
      case 'n':
         options.n = atoi(optarg);
         if (options.n < PING_MIN_NUMBER) {
            options.n = PING_MIN_NUMBER;
            printf("Warning : Invalid number, set to %d\n", PING_MIN_NUMBER);
         }
         break;
      case 'd':
         options.delay = atoi(optarg);
         break;
      case 'i':
         options.sid = atoi(optarg);
         break;
      case 'c':
         options.compressEnabled = true;
         break;
      case 'e':
         options.encryptionEnabled = true;
         break;
      case 'p':
         options.postMode = true;
         break;
      case 'h':
      case '?':
         Usage();
         ret = false;
         break;
      }
   }

   // validate user input parameter
   if (ret) {
      if (options.compressEnabled && options.type == VDPSERVICE_MAIN_CHANNEL) {
         printf("Warning: Compression is not support on main channel.\n");
         options.compressEnabled = false;
      }

      if (options.encryptionEnabled &&
          (options.type == VDPSERVICE_MAIN_CHANNEL ||
           options.type == VDPSERVICE_VCHAN_CHANNEL)) {
         printf("Warning: Encryption is not support on non-tcp channels.\n");
         options.encryptionEnabled = false;
      }

      printf("\nPing %d bytes %d times via %s in %s mode\n"
             "(Encryption : %s   Compression : %s)\n",
             options.size, options.n, channelType,
             options.postMode ? "post" : "request",
             options.encryptionEnabled ? "on" : "off",
             options.compressEnabled ? "on" : "off");
   }

   return ret;
}