/* ********************************************************************************* *
 * Copyright (C) 2011-2021 VMware, Inc.  All rights reserved. -- VMware Confidential *
 * ********************************************************************************* */

/*
 * LocalOverlayGuest.cpp --
 *
 */

#include "stdafx.h"
#include "RPCManager.h"


/*
 *----------------------------------------------------------------------
 *
 * Class LocalOverlayRPCPlugin
 *
 *    The server side RPCPluginInstance which takes care of sending
 *    and receiving messages
 *
 *----------------------------------------------------------------------
 */
class LocalOverlayRPCPlugin : public RPCPluginInstance
{
public:
   LocalOverlayRPCPlugin(RPCManager* rpcManagerPtr) : RPCPluginInstance(rpcManagerPtr) { }
   virtual ~LocalOverlayRPCPlugin() { }

   /*
    *----------------------------------------------------------------------
    *
    * Method SetEnable
    *
    *----------------------------------------------------------------------
    */
   bool SetEnable(bool enable)
   {
      return enable ? Enable() : Disable();
   }


   /*
    *----------------------------------------------------------------------
    *
    * Method Enable
    *
    *----------------------------------------------------------------------
    */
   bool Enable()
   {
      const VDPRPC_ChannelContextInterface* iChannelCtx = ChannelContextInterface();
      const VDPRPC_VariantInterface* iVariant = VariantInterface();

      void* messageCtx = NULL;
      if (!CreateMessage(&messageCtx)) {
         return false;
      }

      iChannelCtx->v1.SetCommand(messageCtx, LOCAL_OVERLAY_ENABLE);

      if (!InvokeMessage(messageCtx)) {
         DestroyMessage(messageCtx);
         return false;
      }

      return true;
   }


   /*
    *----------------------------------------------------------------------
    *
    * Method Disable
    *
    *----------------------------------------------------------------------
    */
   bool Disable()
   {
      const VDPRPC_ChannelContextInterface* iChannelCtx = ChannelContextInterface();
      const VDPRPC_VariantInterface* iVariant = VariantInterface();

      void* messageCtx = NULL;
      if (!CreateMessage(&messageCtx)) {
         return false;
      }

      iChannelCtx->v1.SetCommand(messageCtx, LOCAL_OVERLAY_DISABLE);

      if (!InvokeMessage(messageCtx)) {
         DestroyMessage(messageCtx);
         return false;
      }

      return true;
   }


   /*
    *----------------------------------------------------------------------
    *
    * Method SetLayoutMode
    *
    *----------------------------------------------------------------------
    */
   bool SetLayoutMode(VDPOverlay_LayoutMode layoutMode)
   {
      const VDPRPC_ChannelContextInterface* iChannelCtx = ChannelContextInterface();
      const VDPRPC_VariantInterface* iVariant = VariantInterface();

      void* messageCtx = NULL;
      if (!CreateMessage(&messageCtx)) {
         return false;
      }

      iChannelCtx->v1.SetCommand(messageCtx, LOCAL_OVERLAY_SET_LAYOUT_MODE);

      RPCVariant var(this);
      iVariant->v1.VariantFromUInt32(&var, layoutMode);
      iChannelCtx->v1.AppendParam(messageCtx, &var);

      if (!InvokeMessage(messageCtx)) {
         DestroyMessage(messageCtx);
         return false;
      }

      return true;
   }


   /*
    *----------------------------------------------------------------------
    *
    * Method SetPosition
    *
    *----------------------------------------------------------------------
    */
   bool SetPosition(int32 x, int32 y)
   {
      const VDPRPC_ChannelContextInterface* iChannelCtx = ChannelContextInterface();
      const VDPRPC_VariantInterface* iVariant = VariantInterface();

      void* messageCtx = NULL;
      if (!CreateMessage(&messageCtx)) {
         return false;
      }

      iChannelCtx->v1.SetCommand(messageCtx, LOCAL_OVERLAY_SET_POSITION);

      RPCVariant var(this);
      iVariant->v1.VariantFromInt32(&var, x);
      iChannelCtx->v1.AppendParam(messageCtx, &var);
      iVariant->v1.VariantFromInt32(&var, y);
      iChannelCtx->v1.AppendParam(messageCtx, &var);

      if (!InvokeMessage(messageCtx)) {
         DestroyMessage(messageCtx);
         return false;
      }

      return true;
   }


   /*
    *----------------------------------------------------------------------
    *
    * Method SetSize
    *
    *----------------------------------------------------------------------
    */
   bool SetSize(int32 w, int32 h)
   {
      const VDPRPC_ChannelContextInterface* iChannelCtx = ChannelContextInterface();
      const VDPRPC_VariantInterface* iVariant = VariantInterface();

      void* messageCtx = NULL;
      if (!CreateMessage(&messageCtx)) {
         return false;
      }

      iChannelCtx->v1.SetCommand(messageCtx, LOCAL_OVERLAY_SET_SIZE);

      RPCVariant var(this);
      iVariant->v1.VariantFromInt32(&var, w);
      iChannelCtx->v1.AppendParam(messageCtx, &var);
      iVariant->v1.VariantFromInt32(&var, h);
      iChannelCtx->v1.AppendParam(messageCtx, &var);

      if (!InvokeMessage(messageCtx)) {
         DestroyMessage(messageCtx);
         return false;
      }

      return true;
   }


   /*
    *----------------------------------------------------------------------
    *
    * Method SetColor
    *
    *----------------------------------------------------------------------
    */
   bool SetColor(uint32 color)
   {
      const VDPRPC_ChannelContextInterface* iChannelCtx = ChannelContextInterface();
      const VDPRPC_VariantInterface* iVariant = VariantInterface();

      void* messageCtx = NULL;
      if (!CreateMessage(&messageCtx)) {
         return false;
      }

      iChannelCtx->v1.SetCommand(messageCtx, LOCAL_OVERLAY_SET_COLOR);

      RPCVariant var(this);
      iVariant->v1.VariantFromUInt32(&var, color);
      iChannelCtx->v1.AppendParam(messageCtx, &var);

      if (!InvokeMessage(messageCtx)) {
         DestroyMessage(messageCtx);
         return false;
      }

      printf("color  = 0x%08x\n", color);
      return true;
   }


   /*
    *----------------------------------------------------------------------
    *
    * Method SetFormat
    *
    *----------------------------------------------------------------------
    */
   bool SetFormat(VDPOverlay_ImageFormat format)
   {
      const VDPRPC_ChannelContextInterface* iChannelCtx = ChannelContextInterface();
      const VDPRPC_VariantInterface* iVariant = VariantInterface();

      void* messageCtx = NULL;
      if (!CreateMessage(&messageCtx)) {
         return false;
      }

      iChannelCtx->v1.SetCommand(messageCtx, LOCAL_OVERLAY_SET_FORMAT);

      RPCVariant var(this);
      iVariant->v1.VariantFromUInt32(&var, format);
      iChannelCtx->v1.AppendParam(messageCtx, &var);

      if (!InvokeMessage(messageCtx)) {
         DestroyMessage(messageCtx);
         return false;
      }

      printf("format = %s\n", VDP_OVERLAY_FORMAT_STR(format));
      return true;
   }


   /*
    *----------------------------------------------------------------------
    *
    * Method SetLayer
    *
    *----------------------------------------------------------------------
    */
   bool SetLayer(uint32 layer)
   {
      const VDPRPC_ChannelContextInterface* iChannelCtx = ChannelContextInterface();
      const VDPRPC_VariantInterface* iVariant = VariantInterface();

      void* messageCtx = NULL;
      if (!CreateMessage(&messageCtx)) {
         return false;
      }

      iChannelCtx->v1.SetCommand(messageCtx, LOCAL_OVERLAY_SET_LAYER);

      RPCVariant var(this);
      iVariant->v1.VariantFromUInt32(&var, layer);
      iChannelCtx->v1.AppendParam(messageCtx, &var);

      if (!InvokeMessage(messageCtx)) {
         DestroyMessage(messageCtx);
         return false;
      }

      printf("layer  = %d\n", layer);
      return true;
   }
};


/*
 *----------------------------------------------------------------------
 *
 * Function _tmain
 *
 *----------------------------------------------------------------------
 */
int _tmain(int argc, _TCHAR* argv[])
{
   int i, n = argc > 1 ? ::atoi(argv[1]) : 10;
   bool pressEnterToExit = true;

   #ifdef _DEBUG
      printf("Press ENTER to begin\n");  getchar();
   #endif

   LogUtils::LogInit(LOCAL_OVERLAY_TOKEN_NAME, true);
   RPCManager rpcManager(LOCAL_OVERLAY_TOKEN_NAME);
   LocalOverlayRPCPlugin rpc(&rpcManager);

   if (!rpcManager.ServerInit(&rpc, 5000)) {
      printf("InitServer() failed\n");
      goto done;
   }

   bool enabled = true;
   uint32 layer = 100;
   int x=100, y=150, w=400, h=200;

   uint32 colors[] = { 0xc0ffffff, 0xb0ff0000, 0xa000ff00, 0x900000ff,
                       0x80ff00ff, 0x70ffff00, 0x6000ffff, 0x50000000 };
   uint32 colorInx = 0, colorMax = ARRAYSIZE(colors);

   // note: VDP_OVERLAY_YV12 has been deprecated
   VDPOverlay_ImageFormat formats[] = { VDP_OVERLAY_BGRX, VDP_OVERLAY_BGRA };
   uint32 formatInx = 0, formatMax = ARRAYSIZE(formats);

   rpc.SetLayoutMode(VDP_OVERLAY_LAYOUT_SCALE);
   rpc.SetFormat(formats[formatInx]);
   rpc.SetColor(colors[colorInx]);
   rpc.SetLayer(layer);
   rpc.SetPosition(x, y);
   rpc.SetSize(w, h);
   rpc.Enable();

   for (i=0;  i < n;  ++i) {
      rpc.SetPosition(++x, ++y);
      rpc.SetSize(++w, --h);
      rpcManager.Poll(100);
   }

   printf("Use these keys to control the overlay\n");
   printf("   1 - move overlay down/left\n");
   printf("   2 - move overlay down\n");
   printf("   3 - move overlay down/right\n");
   printf("   4 - move overlay left\n");
   printf("   6 - move overlay right\n");
   printf("   7 - move overlay up/left\n");
   printf("   8 - move overlay up\n");
   printf("   9 - move overlay up/right\n");
   printf("   0 - change overlay color\n");
   printf("   * - change overlay format\n");
   printf("   / - enable/disable overlay\n");
   printf("   + - increase overlay layer\n");
   printf("   - - decrease overlay layer\n");
   printf("   . - exit program\n");
   printf("===============================\n");

   char c = '\0';
   while (c != '.') {

      while (!_kbhit()) {
         rpcManager.Poll(10);
      }

      int posAdj = 10;
      switch(c = _getch())
      {
      case '/':
         rpc.SetEnable(enabled ^= 1);
         break;

      case '+':
         rpc.SetLayer(++layer);
         break;

      case '-':
         rpc.SetLayer(--layer);
         break;

      case '*':
         rpc.SetFormat(formats[++formatInx % formatMax]);
         break;

      case '5':
         rpc.SetLayoutMode(VDP_OVERLAY_LAYOUT_SCALE);
         break;

      case '0':
         rpc.SetColor(colors[++colorInx % colorMax]);
         break;

      case '1':
         rpc.SetPosition(x-=posAdj, y+=posAdj);
         break;

      case '2':
         rpc.SetPosition(x, y+=posAdj);
         break;

      case '3':
         rpc.SetPosition(x+=posAdj, y+=posAdj);
         break;

      case '4':
         rpc.SetPosition(x-=posAdj, y);
         break;

      case '6':
         rpc.SetPosition(x+=posAdj, y);
         break;

      case '7':
         rpc.SetPosition(x-=posAdj, y-=posAdj);
         break;

      case '8':
         rpc.SetPosition(x, y-=posAdj);
         break;

      case '9':
         rpc.SetPosition(x+=posAdj, y-=posAdj);
         break;
      }
   }

   rpcManager.Poll(1000);
   rpcManager.ServerExit(&rpc);
   pressEnterToExit = false;

done:
   if (pressEnterToExit) {
      printf("Press ENTER to exit\n");  getchar();
   }

   return 0;
}
