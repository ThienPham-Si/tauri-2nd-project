/* **************************************************************************** *
 * Copyright (C) 2011-2021 VMware, Inc.  All rights reserved. -- VMware Confidential *
 * **************************************************************************** */

/*
 * usage.h --
 *
 */

#pragma once

#define PING_MIN_NUMBER     1
#define CURRENT_SESSION     -1

typedef struct {
   VdpServiceChannelType type;    // channel type
   int size;                      // ping packet data size
   int n;                         // number of ping requests
   int delay;                     // Number of millisecond
   int sid;                       // windows session id
   bool compressEnabled;          // is compression enabled
   bool encryptionEnabled;        // is encryption enabled
   bool postMode;                 // message in post mode
} PingOptions;

// Parse commandline options
bool ParseOptions(PingOptions& options, int argc, char **argv);
