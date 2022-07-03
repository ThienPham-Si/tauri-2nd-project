/* ********************************************************************************* *
 * Copyright (C) 2011-2021 VMware, Inc.  All rights reserved. -- VMware Confidential *
 * ********************************************************************************* */

/*
 * PingRPCExe.cpp --
 *
 */

#include "stdafx.h"
#include "PingRPCExe.h"
#include "param.h"
#include "winsock2.h"

#pragma comment(lib, "Ws2_32.lib")

/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *     main entry point to RPC ping program.
 *
 * Results:
 *     return 0 if succeed.
 *
 * Side Effects:
 *     None.
 *
 *----------------------------------------------------------------------
 */

int
main(int argc, char* argv[])
{
   PingOptions options;

   if (!ParseOptions(options, argc, argv)) {
      // Help page already print out in ParseOptions.
      return -1;
   }

   #ifdef _DEBUG
      printf("Press ENTER to begin\n");  getchar();
   #endif

   LogUtils::LogInit(PINGRPC_TOKEN_NAME, true);
   RPCManager pingRPCManager(PINGRPC_TOKEN_NAME);
   PingRPCPlugin pingRPCPlugin(options.postMode, &pingRPCManager);

   if (!pingRPCManager.ServerInit2(options.sid, options.type,
                                   options.compressEnabled,
                                   options.encryptionEnabled,
                                   &pingRPCPlugin, 5000)) {
      printf("InitServer() failed\n");
      goto done;
   }

   DWORD pingTime = GetTickCount();
   if (options.type != VDPSERVICE_TCPRAW_CHANNEL) {
      for (int i=0;  i < options.n;  ++i) {
         if (!pingRPCPlugin.Ping(options.size)) {
            break;
         }
      }
      pingRPCPlugin.WaitForPendingMessages(options.n * 1000);
   } else {

      /*
       * Tcp Raw socket was optimized for
       * 1) Reduce the number of threadsused(vdpservice need 6 threads for
       *    tcpsidechannel app for each session)
       * 2) Limit network over header per package for application where
       *    message is already packaged as single payload(blob data)
       * 3) Restrict to post-mode only(less network traffic).
       * 4) Agent side optimzation only(less work for client side).
       *
       * Here we also demostrate VDPService_ObserverInterface
       * for communication between two pieces of logic with same process
       * In real world, it could be any threads and any vdpservice plugins
       * within the same process. To make code more portable, tcp raw
       * socket thread is written in BSD APIs.
       *
       * For illustration purpose, agent send timestamp + some gabage data
       * and client send back timestamp for RTT.
       */

      pingRPCPlugin.SetTcpTestParam(options.n, options.size, options.compressEnabled, options.encryptionEnabled);
      pingRPCPlugin.RegisterObserver();

      DWORD id;
      HANDLE hThread;
      hThread = CreateThread(NULL, 0,
                             (LPTHREAD_START_ROUTINE) TcpPingProc,
                             &pingRPCPlugin, 0, &id);

      if (hThread) {
         WaitForSingleObject(hThread, INFINITE);
         CloseHandle(hThread);
      }
      pingRPCPlugin.UnregisterObserver();
   }

   pingTime = GetTickCount() - pingTime;
   pingRPCManager.ServerExit2(options.sid, &pingRPCPlugin);

   char* s = pingRPCPlugin.cntSent == 1 ? "" : "s";
   printf("%d ping%s sent\n", pingRPCPlugin.cntSent, s);

   s = pingRPCPlugin.cntRecv == 1 ? "" : "s";
   printf("%d ping%s received\n", pingRPCPlugin.cntRecv, s);

   double msPing = (double)pingTime / (double)pingRPCPlugin.cntRecv;
   printf("%dms/ping\n", (int32)(msPing + 0.5));

done:
   if (options.delay > 0) {
      ::Sleep(options.delay);
   } else {
      printf("Press ENTER to exit\n");  getchar();
   }

   return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * Class PingRPCPlugin
 *
 *----------------------------------------------------------------------
 */
PingRPCPlugin::PingRPCPlugin(bool bPostMode, RPCManager* rpcManagerPtr)
   : cntRecv(0),
     cntSent(0),
     m_postMode(bPostMode),
     recvBuf(NULL),
     recvLen(0),
     RPCPluginInstance(rpcManagerPtr)
{
}

PingRPCPlugin::~PingRPCPlugin()
{
   if (recvBuf) {
      delete [] recvBuf;
   }
}


/*
 *----------------------------------------------------------------------
 *
 * PingRPCPlugin::Ping --
 *
 *    A timestamp is sent to the client which then bounces the
 *    value back to us so that we can calculate the round-trip time.
 *
 *----------------------------------------------------------------------
 */
bool
PingRPCPlugin::Ping(int size)                       // IN
{
   const VDPRPC_ChannelContextInterface* iChannelCtx = ChannelContextInterface();
   const VDPRPC_VariantInterface* iVariant = VariantInterface();
   RPCManager*              rpcManagerPtr  = GetRPCManager();

   if (!rpcManagerPtr) {
      return false;
   }

   /*
    * Create a message and give it a name
    */
   void* messageCtx = NULL;
   if (!CreateMessage(&messageCtx)) {
      return false;
   }

   iChannelCtx->v1.SetNamedCommand(messageCtx, PINGRPC_MESSAGE);

   /*
    * I'm just going to add one parameter to the message, a timestamp.
    */
   RPCVariant var(this);

   uint32 ms = GetTickCount();
   iVariant->v1.VariantFromUInt32(&var, ms);
   iChannelCtx->v1.AppendParam(messageCtx, &var);

   if (size > 0) {
      char *str = GetStringForPing(cntSent, size);
      iVariant->v1.VariantInit(&var);   // reuse var
      iVariant->v1.VariantFromStr(&var, str);
      iChannelCtx->v1.AppendParam(messageCtx, &var);

      delete []str;
   }

   if (m_postMode) {
      RPCVariant mode(this);
      iVariant->v1.VariantFromUInt32(&mode, 1);
      iChannelCtx->v2.SetOps(messageCtx, VDP_RPC_CHANNEL_CONTEXT_OPT_POST, &mode);
   }

   /*
    * After a successfull call to invoke, the RPC library owns
    * the message context and will destroy it.  We only need to
    * destroy it if InvokeMessage() fails.
    */
   if (!InvokeMessage(messageCtx)) {
      DestroyMessage(messageCtx);
      return false;
   }

   rpcManagerPtr->Poll();
   cntSent++;
   return true;
}


/*
 *----------------------------------------------------------------------
 *
 * PingRPCPlugin::OnDone --
 *
 *    This method is called when the peer has processed a message.
 *
 *----------------------------------------------------------------------
 */
void
PingRPCPlugin::OnDone(uint32 requestCtxId, void *returnCtx)
{
   const VDPRPC_ChannelContextInterface* iChannelCtx = ChannelContextInterface();
   const VDPRPC_VariantInterface* iVariant = VariantInterface();

   /*
    * Make sure the message name matches.
    */
   char cmd[32];
   iChannelCtx->v1.GetNamedCommand(returnCtx, cmd, sizeof cmd);
   if (strcmp(cmd, PINGRPC_MESSAGE) != 0) {
      LOG("Unknown command \"%s\"", cmd);
      return;
   }

   /*
    * Fetch the bounced timestamp and calculate the round-trip time.
    */
   RPCVariant var(this);

   if (m_postMode) {
      iChannelCtx->v1.GetParam(returnCtx, 0, &var);
      uint32 msPing = GetTickCount() - var.ulVal;
      LOG("Ping took %dms to send", msPing);
   } else {
      iChannelCtx->v1.GetReturnVal(returnCtx, 0, &var);
      uint32 msPing = GetTickCount() - var.ulVal;
      LOG("Ping took %dms", msPing);
   }
   cntRecv++;
}


/*
 *----------------------------------------------------------------------
 *
 * PingRPCPlugin::GetStringForPing --
 *
 *    Create string fill with patterned data.
 *
 * Results:
 *    Returns string if succeed.
 *
 * Side Effects:
 *    caller need to delete it.
 *
 *----------------------------------------------------------------------
 */

char *
PingRPCPlugin::GetStringForPing(int initValue,     // IN
                                int size)          // IN
{
   char *str = new char [size + 1];

   if (str) {
      int i;
      int v;
      for (i=0, v=initValue; i<size; i++, v++) {
         str[i] = (char) ((v % 64) + '0');
      }
      str[i] ='\0';
   }

   return str;
}


/*
 *----------------------------------------------------------------------
 *
 * TcpEchoNotification --
 *
 *    Handle tcp echo notification from client.
 *    (increase recv counter and log RTT)
 *
 * Results:
 *    True.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

Bool
TcpEchoNotification(void *context,             // IN
                    const char *sourceToken,   // IN
                    const void *cookie,        // IN
                    const void *data)
{
   PingRPCPlugin *pingRPC = reinterpret_cast<PingRPCPlugin *> (context);
   int n = (int)(LONG_PTR)cookie;
   DWORD origTimestamp = *((DWORD *) data);

   pingRPC->cntRecv++;
   LOG("Tcp recv %d echo %d sent RTT=%d", pingRPC->cntRecv,
       pingRPC->cntSent, GetTickCount() - origTimestamp);

   return TRUE;
}


/*
 *----------------------------------------------------------------------
 *
 * PingRPCPlugin::RegisterObserver --
 *
 *    Register TcpEchoNotification as VDP_TCP_ECHO callback.
 *
 * Results:
 *    true if it is successfully registered.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

bool
PingRPCPlugin::RegisterObserver()
{
   const VDPService_ObserverInterface* iObserver = VdpObserverInterface();

   m_observerId = iObserver->v1.RegisterObserver(VDP_TCP_ECHO,
                                                 this,
                                                 TcpEchoNotification);

   return m_observerId != VDPOBSERVER_INVALID_ID;
}


/*
 *----------------------------------------------------------------------
 *
 * PingRPCPlugin::UnregisterObserver --
 *
 *    Unregister VDP_TCP_ECHO callback.
 *
 * Results:
 *    None.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

void
PingRPCPlugin::UnregisterObserver()
{
   const VDPService_ObserverInterface* iObserver = VdpObserverInterface();

   if (m_observerId != VDPOBSERVER_INVALID_ID) {
      iObserver->v1.UnregisterObserver(m_observerId);
      m_observerId = VDPOBSERVER_INVALID_ID;
   }

   return;
}


/*
 *----------------------------------------------------------------------
 *
 * PingRPCPlugin::BroadcastToObserver --
 *
 *    Broadcast received data to all VDP_TCP_ECHO observers.
 *
 * Results:
 *    trur if it succeeds, otherwise return false.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

bool
PingRPCPlugin::BroadcastToObserver(const void *data)
{
   const VDPService_ObserverInterface* iObserver = VdpObserverInterface();
   void* cookie = (void*)(LONG_PTR)cntRecv;

   return iObserver->v1.Broadcast(VDP_TCP_ECHO, cookie, data) != 0;
}


/*
 *----------------------------------------------------------------------
 *
 * PingRPCPlugin::SetTcpTestParam --
 *
 *    Store tcp test parameters.
 *
 * Results:
 *    None.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

void
PingRPCPlugin::SetTcpTestParam(int cnt,                     // IN
                               int size,                    // IN
                               bool compressionEnabled,     // IN
                               bool encryptionEnabled)      // IN
{
   cntPing = cnt;
   pingSize = size;
   doCompression = compressionEnabled;
   doEncryption = encryptionEnabled;

   if (recvBuf == NULL) {
      recvBuf = new char [MAX_RECV_LEN];
      recvLen = 0;
   }

   return;
}


/*
 *----------------------------------------------------------------------
 *
 * PingRPCPlugin::TcpSend --
 *
 *    Send one RPC command from tcp socket.
 *
 * Results:
 *    true if it succeeds, otherwise return false.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

bool
PingRPCPlugin::TcpSend()
{
   bool rv = true;
   const VDPRPC_StreamDataInterface* iStreamData = StreamDataInterface();
   if (pingSize < sizeof uint32) {
      pingSize = sizeof uint32;
   }

   // Create source data
   char *data = new char [pingSize];
   *((uint32 *) data) = GetTickCount();
   VDP_RPC_BLOB blob = { (uint32)pingSize, data };

   uint32 options = GetChannelObjOptions();
   VM_ASSERT(!doCompression || (options & VDP_RPC_COMP_SNAPPY));
   VM_ASSERT(!doEncryption || (options & VDP_RPC_CRYPTO_AES));

   options = (doCompression ? VDP_RPC_COMP_SNAPPY : 0) |
             (doEncryption ? VDP_RPC_CRYPTO_AES : 0) ;
   static int reqId = 100;
   reqId++;

   // Create RPC payload
   int fd = GetTcpRawSocket();
   VDP_RPC_BLOB payload = {0, NULL};
   iStreamData->v2.GetStreamData(fd, options, &reqId, VDP_PING_CMD, &blob, &payload);
   if (payload.size > 0) {
      int pos = 0;
      int sent;
      while (pos < (int)payload.size) {
         sent = send(fd, payload.blobData + pos, payload.size - pos, 0);
         if (send > 0) {
            pos += sent;
         } else {
            rv = false;
            LOG("Error: tcp send failed.");
            break;
         }
      }
      iStreamData->v2.FreeStreamDataPayload(&payload);
   }

   delete data;
   return rv;
}


/*
 *----------------------------------------------------------------------
 *
 * PingRPCPlugin::TcpRecv --
 *
 *    Handle incoming data from tcp socket
 *
 * Results:
 *    The number of packets are parsed.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

int
PingRPCPlugin::TcpRecv()
{
   int fd = GetTcpRawSocket();
   const VDPService_ObserverInterface* iObserver = VdpObserverInterface();
   const VDPRPC_StreamDataInterface* iStreamData = StreamDataInterface();

   int recved = 0;
   int len = recv(fd, recvBuf + recvLen, MAX_RECV_LEN - recvLen, 0);
   if (len > 0) {
      int pos = 0;
      int reqId;
      int reqCmd;
      int reqType;
      Bool cleanup;
      int packetLen;
      VDP_RPC_BLOB blobData = {0, NULL};
      int  minSize = iStreamData->v1.GetMinimalStreamDataSize(fd);
      recvLen += len;

      while ((pos + minSize) <= recvLen) {
         packetLen = iStreamData->v1.GetStreamDataSize(fd, recvBuf+pos);

         if (pos + packetLen <= recvLen) {
            if (iStreamData->v2.GetStreamDataInfo(fd, recvBuf+pos, &reqId,
                                                  &reqType, &reqCmd,
                                                  &cleanup, &blobData)) {
               LOG("Trace: Recv cmd %d data %d byte", reqCmd, blobData.size);
               BroadcastToObserver(blobData.blobData);
               if (cleanup) {
                  iStreamData->v2.FreeStreamDataPayload(&blobData);
               }
               recved++;
               pos += packetLen;
               continue;
            } else {
               LOG("Error: GetStreamDataInfo(v2) failed!");
            }
         }

         break;
      }

      // recycle bytes already processed
      if (pos > 0) {
         if (pos < recvLen) {
            recvLen -= pos;
            memmove(recvBuf, recvBuf+pos, recvLen);
         } else {
            recvLen = 0;
         }
      }
   }

   return recved;
}


/*
 *----------------------------------------------------------------------
 *
 * TcpPingProc --
 *
 *    Thread start routine for tcp socket ping to vdpservice client
 *
 * Results:
 *    0 if Tcp ping thread exit successfully, or errCode in PingRPCExe.h.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

DWORD
TcpPingProc(LPVOID data)            // IN
{
   int fd;
   PingRPCPlugin *pingRPC = reinterpret_cast<PingRPCPlugin *> (data);
   if (pingRPC == NULL || (fd = pingRPC->GetTcpRawSocket()) == INVALID_SOCKET) {
      LOG("Error: tcp raw socket not succeed.\n");
      return TCP_SETUP_ERROR;
   }

   int rv;
   int nRecv = 0;
   int nSent = 0;
   int total = pingRPC->TcpPingCnt();
   struct fd_set  rfds;
   struct fd_set  wfds;
   struct timeval timeout;

   while (nRecv < total) {
      FD_ZERO(&rfds);
      FD_ZERO(&wfds);
      FD_SET(fd, &rfds);
      if (nSent < total) {
         FD_SET(fd, &wfds);
      }

      timeout.tv_sec = SELECT_TIMEOUT_SEC;
      timeout.tv_usec = SELECT_TIMEOUT_USEC;
      rv = select(1, &rfds, &wfds, NULL, &timeout);

      if (rv > 0) {
         if (FD_ISSET(fd, &wfds)) {
            if (pingRPC->TcpSend()) {
               pingRPC->cntSent = ++nSent;
            } else {
               LOG("Error: Tcp send failed.");
               return NETWORK_ERROR;
            }
         }

         if (FD_ISSET(fd, &rfds)) {
            int n = pingRPC->TcpRecv();
            if (n < 0) {
               LOG("Error: Tcp recv failed.");
               return NETWORK_ERROR;
            } else {
               nRecv += n;
            }
         }
      } else if (rv < 0) {
         LOG("Error: socket select return %d.", rv);
         return NETWORK_ERROR;
      }
   }

   return 0;
}

