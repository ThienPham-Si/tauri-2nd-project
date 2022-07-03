/* ********************************************************************************* *
 * Copyright (C) 2011-2021 VMware, Inc.  All rights reserved. -- VMware Confidential *
 * ********************************************************************************* */

/*
 * PingRPCPlugin.h --
 *
 */
#pragma once

#include "RPCManager.h"


/*
 *----------------------------------------------------------------------
 *
 * class PingRPCPlugin --
 *
 *----------------------------------------------------------------------
 */
class PingRPCPlugin : public RPCPluginInstance
{
public:
   PingRPCPlugin(RPCManager *rpcManagerPtr);
   virtual ~PingRPCPlugin();

   void OnInvoke(void* messageCtx);
};


/*
 *----------------------------------------------------------------------
 *
 * class RPCClient --
 *
 *----------------------------------------------------------------------
 */
class RPCClient : public RPCManager
{
public:
   RPCClient() : RPCManager(PINGRPC_TOKEN_NAME) { }
   virtual RPCPluginInstance* OnCreateInstance() { return new PingRPCPlugin(this); }
};
