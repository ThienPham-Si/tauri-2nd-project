/* ********************************************************************************* *
 * Copyright (C) 2011-2021 VMware, Inc.  All rights reserved. -- VMware Confidential *
 * ********************************************************************************* */

/*
 * PingRPCExe.h --
 *
 */

#pragma once

#include "RPCManager.h"

DWORD TcpPingProc(LPVOID data);
#define MAX_RECV_LEN                     65536
#define SELECT_TIMEOUT_SEC               1
#define SELECT_TIMEOUT_USEC              0
#define TCP_SETUP_ERROR                  -1
#define NETWORK_ERROR                    -2


/*
 *----------------------------------------------------------------------
 *
 * Class PingRPCPlugin
 *
 *----------------------------------------------------------------------
 */
class PingRPCPlugin : public RPCPluginInstance
{
public:
   PingRPCPlugin(bool bPostMode, RPCManager* rpcManagerPtr);
   virtual ~PingRPCPlugin();

   bool Ping(int size);
   int cntRecv;
   int cntSent;

   virtual void OnDone(uint32 requestCtxId, void *returnCtx);

   /* Observer interface */
   bool RegisterObserver();
   void UnregisterObserver();
   bool BroadcastToObserver(const void *data);

   /* APIs for Tcp Raw socket */
   void SetTcpTestParam(int cnt, int size, bool comp, bool enc);
   bool TcpSend();
   int  TcpRecv();
   int  TcpPingCnt() const { return cntPing; }

private:

   /* Create string fill with patterned data */
   char* GetStringForPing(int initValue, int size);

   /*
    * In post mode, OnDone() indicates when message is delivered
    * instead of response from peer.
    *
    * For application, there are is not difference at message level.
    */
   bool m_postMode;
   VDPService_ObserverId m_observerId;

   /* Used for tcp raw socket */
   int  cntPing;
   int  pingSize;
   char *recvBuf;
   int  recvLen;
   bool doCompression;
   bool doEncryption;
};
