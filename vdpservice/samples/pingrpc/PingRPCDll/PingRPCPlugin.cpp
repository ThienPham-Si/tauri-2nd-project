/* ********************************************************************************* *
 * Copyright (C) 2011-2021 VMware, Inc.  All rights reserved. -- VMware Confidential *
 * ********************************************************************************* */

/*
 * PingRPCPlugin.cpp --
 *
 */

#include "stdafx.h"
#include "PingRPCPlugin.h"


/*
 * The compiler/linker will ignore this file since it doesn't see any code paths
 * that enter here since it's done through the static variable below. The below
 * exported function causes the linker to force the file to be loaded in.
 */
#ifdef _WIN32
void __declspec(dllexport) ForceLoad() { }
#endif

/*
 * This is the single RPCManager object.  It must be declared
 * in global scope so that the constructor is executed before
 * VDPService_PluginGetTokenName() is called.
 */
RPCClient rpcClient;

VDP_SERVICE_CREATE_INTERFACE(PINGRPC_TOKEN_NAME, rpcClient)

/*
 *----------------------------------------------------------------------------
 *
 * class PingRPCPlugin --
 *
 *    An object will be created for each instance of our server side application
 *
 *----------------------------------------------------------------------------
 */
PingRPCPlugin::PingRPCPlugin(RPCManager *rpcManagerPtr)
   : RPCPluginInstance(rpcManagerPtr)
{
}

PingRPCPlugin::~PingRPCPlugin()
{
}


/*
 *----------------------------------------------------------------------
 *
 * PingRPCPlugin::OnInvoke --
 *
 *    This method is called when a peer has sent a message.
 *
 *    All the parameters that the server sent us are bounced back.
 *
 *----------------------------------------------------------------------
 */
void
PingRPCPlugin::OnInvoke(void* messageCtx)
{
   const VDPRPC_ChannelContextInterface* iChannelCtx = ChannelContextInterface();
   const VDPRPC_VariantInterface* iVariant = VariantInterface();

   if (GetChannelType() != VDPSERVICE_TCPRAW_CHANNEL) {
      char cmd[32];
      iChannelCtx->v1.GetNamedCommand(messageCtx, cmd, sizeof cmd);
      if (strcmp(cmd, PINGRPC_MESSAGE) != 0) {
         LOG("Unknown command \"%s\"", cmd);
         return;
      }

      RPCVariant var(this);
      for (int i=0;  i < iChannelCtx->v1.GetParamCount(messageCtx);  ++i) {
         iChannelCtx->v1.GetParam(messageCtx, i, &var);
         iChannelCtx->v1.AppendReturnVal(messageCtx, &var);
      }
   } else {
      uint32 cmd = iChannelCtx->v1.GetCommand(messageCtx);
      if (cmd == VDP_PING_CMD) {
         RPCVariant var(this);
         iChannelCtx->v1.GetParam(messageCtx, 0, &var);

         void *echoCtx;
         if (!CreateMessage(&echoCtx)) {
            LOG("Error: cannot create channelCtx to send channel type.");
            return;
         }

         // Echo back.
         iChannelCtx->v1.SetCommand(echoCtx, VDP_PING_ECHO);
         iChannelCtx->v1.AppendParam(echoCtx, &var);

         if (!InvokeMessage(echoCtx, true)) {
            DestroyMessage(echoCtx);
            return;
         }
      } else {
         LOG("Unknown command [%d]", cmd);
         return;
      }
   }
}
