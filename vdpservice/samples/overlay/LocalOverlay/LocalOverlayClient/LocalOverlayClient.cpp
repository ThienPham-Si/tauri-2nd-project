/* ********************************************************************************* *
 * Copyright (C) 2011-2021 VMware, Inc.  All rights reserved. -- VMware Confidential *
 * ********************************************************************************* */

/*
 * LocalOverlayClient.cpp --
 *
 */

#include "stdafx.h"
#include "RPCManager.h"


/*
 *----------------------------------------------------------------------
 *
 * Function DllMain --
 *
 *----------------------------------------------------------------------
 */
#ifdef _MANAGED
#pragma managed(push, off)
#endif

#ifdef _WIN32
BOOL APIENTRY
DllMain (HMODULE hModule,
         DWORD   reason,
         LPVOID  lpReserved)
{
   switch(reason)
   {
   case DLL_PROCESS_ATTACH:
   case DLL_THREAD_ATTACH:
   case DLL_THREAD_DETACH:
   case DLL_PROCESS_DETACH:
      break;
   }

   return TRUE;
}
#endif

#ifdef _MANAGED
#pragma managed(pop)
#endif


#define LOG_DEBUG(...) LOG(__VA_ARGS__)
#define LOG_INFO(...) LOG(__VA_ARGS__)

/*
 *----------------------------------------------------------------------
 *
 * Class LocalOverlayPlugin --
 *
 *----------------------------------------------------------------------
 */
class LocalOverlayPlugin : public RPCPluginInstance
{
public:
   const VDPOverlayClient_Interface* m_iOverlay;
   VDPOverlayClient_ContextId m_pluginId;
   VDPOverlay_OverlayId m_overlayId;
   VDPOverlay_ImageFormat m_format;
   void* m_image;
   int32 m_width;
   int32 m_height;
   int32 m_pitch;
   int32 m_overlayW;
   int32 m_overlayH;
   uint32 m_imageSz;
   uint32 m_color;
   uint32 m_updateFlags;

   /*
    *----------------------------------------------------------------------
    *
    * Method LocalOverlayPlugin --
    *
    *----------------------------------------------------------------------
    */
   LocalOverlayPlugin(RPCManager* rpcManagerPtr) : RPCPluginInstance(rpcManagerPtr)
   {
      m_iOverlay = OverlayClientInterface();
      m_pluginId = VDP_OVERLAY_CLIENT_CONTEXT_ID_NONE;
      m_overlayId = VDP_OVERLAY_WINDOW_ID_NONE;

      m_image = NULL;
      m_imageSz = 0;
      m_width = 100;
      m_height = 100;
      m_pitch = 0;
      m_overlayW = m_width;
      m_overlayH = m_height;
      m_color = 0xff000000;
      m_format = VDP_OVERLAY_BGRX;
      m_updateFlags = VDP_OVERLAY_UPDATE_FLAG_COPY_IMAGE;

      VDPOverlayClient_Sink sink = { VDP_OVERLAY_CLIENT_SINK_V1 };
      if (m_iOverlay->v1.Init(&sink, (void*)this, &m_pluginId) != VDP_OVERLAY_ERROR_SUCCESS) {
         LOG_DEBUG("iOverlay->v1.Init() failed");
         return;
      }

      if (m_iOverlay->version < VDP_OVERLAY_CLIENT_INTERFACE_V2)
      {
         LOG_DEBUG("Overlay version %d detected; version %d required",
            m_iOverlay->version, VDP_OVERLAY_CLIENT_INTERFACE_V2);
         return;
      }

      VDPOverlay_Error err =
         m_iOverlay->v2.CreateOverlay(m_pluginId, &m_overlayId);

      if (err != VDP_OVERLAY_ERROR_SUCCESS) {
         LOG_DEBUG("iOverlay->v2.CreateOverlay() failed");
         return;
      }

      LOG_DEBUG("iOverlay->v2.CreateOverlay(0x%x) [OK]", m_overlayId);
   }


   /*
    *----------------------------------------------------------------------
    *
    * Method ~LocalOverlayPlugin --
    *
    *----------------------------------------------------------------------
    */
   virtual ~LocalOverlayPlugin()
   {
      GetInfo();

      if (m_overlayId != VDP_OVERLAY_WINDOW_ID_NONE) {
         VDPOverlay_Error err =
            m_iOverlay->v2.DestroyOverlay(m_pluginId, m_overlayId);

         if (err != VDP_OVERLAY_ERROR_SUCCESS) {
            LOG_DEBUG("iOverlay->v2.DestroyOverlay(0x%x) failed", m_overlayId);
         } else {
            LOG_DEBUG("iOverlay->v2.DestroyOverlay(0x%x) [OK]", m_overlayId);
         }

         m_overlayId = VDP_OVERLAY_WINDOW_ID_NONE;
      }

      if (m_pluginId != VDP_OVERLAY_CLIENT_CONTEXT_ID_NONE) {
         VDPOverlay_Error err =
            m_iOverlay->v1.Exit(m_pluginId);

         if (err != VDP_OVERLAY_ERROR_SUCCESS) {
            LOG_DEBUG("iOverlay->v1.Exit(0x%x) failed", m_overlayId);
         } else {
            LOG_DEBUG("iOverlay->v1.Exit(0x%x) [OK]", m_overlayId);
         }

         m_pluginId = VDP_OVERLAY_CLIENT_CONTEXT_ID_NONE;
      }

      if (m_image != NULL) {
         free(m_image);
         m_image = NULL;
      }
   }


   /*
    *----------------------------------------------------------------------
    *
    * Method OnInvoke --
    *
    *----------------------------------------------------------------------
    */
   void OnInvoke(void* messageCtx)
   {
      const VDPRPC_ChannelContextInterface* iChannelCtx = ChannelContextInterface();
      RPCVariant var(this);

      uint32 cmd = iChannelCtx->v1.GetCommand(messageCtx);
      switch (cmd)
      {
      case LOCAL_OVERLAY_ENABLE:
         UpdateImage(m_overlayW, m_overlayH, m_format, m_color);
         Enable();
         break;

      case LOCAL_OVERLAY_DISABLE:
         Disable();
         break;

      case LOCAL_OVERLAY_SET_FORMAT:
         iChannelCtx->v1.GetParam(messageCtx, 0, &var);
         SetFormat((VDPOverlay_ImageFormat)var.ullVal);
         break;

      case LOCAL_OVERLAY_SET_COLOR:
         iChannelCtx->v1.GetParam(messageCtx, 0, &var);
         SetColor(var.ulVal);
         break;

      case LOCAL_OVERLAY_SET_LAYOUT_MODE:
         iChannelCtx->v1.GetParam(messageCtx, 0, &var);
         SetLayoutMode((VDPOverlay_LayoutMode)var.ullVal);
         break;

      case LOCAL_OVERLAY_SET_LAYER:
         iChannelCtx->v1.GetParam(messageCtx, 0, &var);
         SetLayer(var.ulVal);
         break;

      case LOCAL_OVERLAY_SET_POSITION: {
         iChannelCtx->v1.GetParam(messageCtx, 0, &var);  int32 x = var.lVal;
         iChannelCtx->v1.GetParam(messageCtx, 1, &var);  int32 y = var.lVal;
         SetPosition(x, y);
         break;
        }

      case LOCAL_OVERLAY_SET_SIZE: {
         iChannelCtx->v1.GetParam(messageCtx, 0, &var);  int32 w = var.lVal;
         iChannelCtx->v1.GetParam(messageCtx, 1, &var);  int32 h = var.lVal;
         SetSize(w, h);
         break;
        }

      default:
         LOG_DEBUG("Unknown command %d", cmd);
         break;
      }
   }


   /*
    *----------------------------------------------------------------------
    *
    * Method UpdateImage --
    *
    *----------------------------------------------------------------------
    */
   void UpdateImage(int32 width, int32 height, VDPOverlay_ImageFormat format, uint32 color)
   {
      bool newImage = m_image == NULL;

      /*
       * If the parameters changed then allocate a new image buffer
       */
      if (newImage || m_width != width || m_height != height || m_format != format) {
         int32 pitch, imageSz;

         if (VDP_OVERLAY_FORMAT_IS_RGB(format)) {
            pitch = width * 4;
            imageSz = pitch * height;
         }
         else
         if (VDP_OVERLAY_FORMAT_IS_YUV(format)) {
            pitch    = (width + 1) & ~1;
            imageSz  = pitch   * height;    // Y plane
            imageSz += pitch/2 * height/2;  // V plane
            imageSz += pitch/2 * height/2;  // U plane
         } else {
            LOG_DEBUG("iOverlay 0x%x: invalid format %d", m_overlayId, format);
            return;
         }

         if (imageSz != m_imageSz) {
            void* image = realloc(m_image, imageSz);
            if (image == NULL) {
               LOG_DEBUG("iOverlay 0x%x: image size set to %s %d x %d failed",
                  m_overlayId, VDP_OVERLAY_FORMAT_STR(format), width, height);
               return;
            }
            m_image   = image;
            m_imageSz = imageSz;
         }

         m_width   = width;
         m_height  = height;
         m_pitch   = pitch;
         m_format  = format;
         newImage  = true;

         LOG_DEBUG("iOverlay 0x%x: image set to %s %d x %d [OK]",
            m_overlayId, VDP_OVERLAY_FORMAT_STR(m_format), m_width, m_height);
      }


      /*
       * If necessary, fill the image buffer with the color.
       */
      if (newImage || m_color != color) {
         if (m_format == VDP_OVERLAY_BGRX) {
            /*
             * Simple just fill the buffer with the color
             */
            #ifdef _WIN32
            __stosd((DWORD*)m_image, color, m_imageSz / 4);
            #else
            uint32 *buf = (uint32*)m_image;
            for(int i=0; i < m_imageSz / 4; ++i ) {
               buf[i] = color;
            }
            #endif

         } else if (m_format == VDP_OVERLAY_BGRA) {
            /*
             * VDPService expects the image to be pre-multiplied
             * so I have to multiply the RGB values by the alpha
             */
            uint8 a = (color >> 24) & 0xff;
            uint8 r = (color >> 16) & 0xff;
            uint8 g = (color >>  8) & 0xff;
            uint8 b = (color >>  0) & 0xff;

            double alpha = a / 255.0;
            r = (uint8)(r * alpha + 0.5);
            g = (uint8)(g * alpha + 0.5);
            b = (uint8)(b * alpha + 0.5);

            uint32 preMulColor = (a << 24) | (r << 16) | (g << 8) | b;
            #ifdef _WIN32
            __stosd((DWORD*)m_image, preMulColor, m_imageSz / 4);
            #else
            uint32 *buf = (uint32*)m_image;
            for(int i=0; i < m_imageSz / 4; ++i ) {
               buf[i] = preMulColor;
            }
            #endif

         } else if (m_format == VDP_OVERLAY_YV12) {
            /*
             * Convert the RGB color to YUV
             */
            uint8 r = (color >> 16) & 0xff;
            uint8 g = (color >>  8) & 0xff;
            uint8 b = (color >>  0) & 0xff;

            uint8 y = (( 66 * r + 129 * g +  25 * b + 128) >> 8) +  16;
            uint8 u = ((-38 * r -  74 * g + 112 * b + 128) >> 8) + 128;
            uint8 v = ((112 * r -  94 * g -  18 * b + 128) >> 8) + 128;

            char* yData = (char*)m_image;
            size_t yDataSz = m_pitch * m_height;
            memset(yData, y, yDataSz);

            char* vData = yData + yDataSz;
            size_t vDataSz = yDataSz / 4;
            memset(vData, v, vDataSz);

            char* uData = vData + vDataSz;
            size_t uDataSz = yDataSz / 4;
            memset(uData, u, uDataSz);

         } else {
            memset(m_image, 0, m_imageSz);
         }

         m_color = color;
         LOG_DEBUG("iOverlay 0x%x: color set to 0x%08x [OK]", m_overlayId, m_color);
      }


      /*
       * Update the overlay with the new image
       */
      if (m_overlayId == VDP_OVERLAY_WINDOW_ID_NONE) {
         LOG_DEBUG("No overlay");
         return;
      }

      VDPOverlay_Error err =
         m_iOverlay->v2.Update(m_pluginId, m_overlayId, m_image,
                               m_width, m_height, m_pitch,
                               m_format, m_updateFlags);

      if (err != VDP_OVERLAY_ERROR_SUCCESS) {
         LOG_DEBUG("iOverlay->v1.Update(0x%x) failed", m_overlayId);
         return;
      }

      LOG_INFO("iOverlay->v1.Update(0x%x) [OK]", m_overlayId);
   }


   /*
    *----------------------------------------------------------------------
    *
    * Method GetInfo --
    *
    *----------------------------------------------------------------------
    */
   void GetInfo()
   {
      if (m_overlayId == VDP_OVERLAY_WINDOW_ID_NONE) {
         LOG_DEBUG("No overlay");
         return;
      }

      VDPOverlayClient_OverlayInfo info = { VDP_OVERLAY_INFO_V1_SIZE };

      VDPOverlay_Error err =
         m_iOverlay->v2.GetInfo(m_pluginId, m_overlayId, &info);

      if (err != VDP_OVERLAY_ERROR_SUCCESS) {
         LOG_DEBUG("iOverlay->v2.GetInfo(0x%x) failed", m_overlayId);
         return;
      }

      LOG_INFO("version  = %d", info.version);
      LOG_INFO("windowId = 0x%x", info.v1.windowId);
      LOG_INFO("format   = %s", VDP_OVERLAY_FORMAT_STR(info.v2.imageFormat));
      LOG_INFO("position = %d,%d", info.v1.xUI, info.v1.yUI);
      LOG_INFO("size     = %dx%d", info.v1.width, info.v1.height);
      LOG_INFO("enabled  = %d", info.v1.enabled);
      LOG_INFO("visible  = %d", info.v1.visible);
      LOG_INFO("layer    = %d", info.v2.layer);
      LOG_INFO("colorkey = 0x%08x", info.v2.colorkey);
   }


   /*
    *----------------------------------------------------------------------
    *
    * Method Enable --
    *
    *----------------------------------------------------------------------
    */
   void Enable()
   {
      if (m_overlayId == VDP_OVERLAY_WINDOW_ID_NONE) {
         LOG_DEBUG("No overlay");
         return;
      }

      VDPOverlay_Error err =
         m_iOverlay->v2.EnableOverlay(m_pluginId, m_overlayId);

      if (err != VDP_OVERLAY_ERROR_SUCCESS) {
         LOG_DEBUG("iOverlay->v2.EnableOverlay(0x%x) failed", m_overlayId);
         return;
      }

      LOG_INFO("iOverlay->v2.EnableOverlay(0x%x) [OK]", m_overlayId);
   }


   /*
    *----------------------------------------------------------------------
    *
    * Method Disable --
    *
    *----------------------------------------------------------------------
    */
   void Disable()
   {
      if (m_overlayId == VDP_OVERLAY_WINDOW_ID_NONE) {
         LOG_DEBUG("No overlay");
         return;
      }

      VDPOverlay_Error err =
         m_iOverlay->v2.DisableOverlay(m_pluginId, m_overlayId);

      if (err != VDP_OVERLAY_ERROR_SUCCESS) {
         LOG_DEBUG("iOverlay->v2.DisableOverlay(0x%x) failed", m_overlayId);
         return;
      }

      LOG_INFO("iOverlay->v2.DisableOverlay(0x%x) [OK]", m_overlayId);
   }


   /*
    *----------------------------------------------------------------------
    *
    * Method SetFormat --
    *
    *----------------------------------------------------------------------
    */
   void SetFormat(VDPOverlay_ImageFormat format)
   {
      UpdateImage(m_width, m_height, format, m_color);
   }


   /*
    *----------------------------------------------------------------------
    *
    * Method SetColor --
    *
    *----------------------------------------------------------------------
    */
   void SetColor(uint32 color)
   {
      UpdateImage(m_width, m_height, m_format, color);
   }


   /*
    *----------------------------------------------------------------------
    *
    * Method SetLayoutMode --
    *
    *----------------------------------------------------------------------
    */
   void SetLayoutMode(VDPOverlay_LayoutMode layoutMode)
   {
      if (m_overlayId == VDP_OVERLAY_WINDOW_ID_NONE) {
         LOG_DEBUG("No overlay");
         return;
      }

      VDPOverlay_Error err =
         m_iOverlay->v2.SetLayoutMode(m_pluginId, m_overlayId, layoutMode);

      if (err != VDP_OVERLAY_ERROR_SUCCESS) {
         LOG_DEBUG("iOverlay->v2.SetLayoutMode(0x%x) %d failed", m_overlayId, layoutMode);
         return;
      }

      LOG_INFO("iOverlay->v2.SetLayoutMode(0x%x) %d [OK]", m_overlayId, layoutMode);
   }


   /*
    *----------------------------------------------------------------------
    *
    * Method SetLayer --
    *
    *----------------------------------------------------------------------
    */
   void SetLayer(uint32 layer)
   {
      if (m_overlayId == VDP_OVERLAY_WINDOW_ID_NONE) {
         LOG_DEBUG("No overlay");
         return;
      }

      VDPOverlay_Error err =
         m_iOverlay->v2.SetLayer(m_pluginId, m_overlayId, layer);

      if (err != VDP_OVERLAY_ERROR_SUCCESS) {
         LOG_DEBUG("iOverlay->v2.SetLayer(0x%x) %d failed", m_overlayId, layer);
         return;
      }

      LOG_INFO("iOverlay->v2.SetLayer(0x%x) %d [OK]", m_overlayId, layer);
   }


   /*
    *----------------------------------------------------------------------
    *
    * Method SetPosition --
    *
    *----------------------------------------------------------------------
    */
   void SetPosition(int32 x, int32 y)
   {
      if (m_overlayId == VDP_OVERLAY_WINDOW_ID_NONE) {
         LOG_DEBUG("No overlay");
         return;
      }

      VDPOverlay_Error err =
         m_iOverlay->v2.SetPosition(m_pluginId, m_overlayId, x, y);

      if (err != VDP_OVERLAY_ERROR_SUCCESS) {
         LOG_DEBUG("iOverlay->v2.SetPosition(0x%x) %d,%d failed", m_overlayId, x, y);
         return;
      }

      LOG_INFO("iOverlay->v2.SetPosition(0x%x) %d,%d [OK]", m_overlayId, x, y);
   }


   /*
    *----------------------------------------------------------------------
    *
    * Method SetSize --
    *
    *----------------------------------------------------------------------
    */
   void SetSize(int32 w, int32 h)
   {
      if (m_overlayId == VDP_OVERLAY_WINDOW_ID_NONE) {
         LOG_DEBUG("No overlay");
         return;
      }

      VDPOverlay_Error err =
         m_iOverlay->v2.SetSize(m_pluginId, m_overlayId, w, h);

      if (err != VDP_OVERLAY_ERROR_SUCCESS) {
         LOG_DEBUG("iOverlay->v2.SetSize(0x%x) %d,%d failed", m_overlayId, w, h);
         return;
      }

      LOG_INFO("iOverlay->v2.SetSize(0x%x) %d,%d [OK]", m_overlayId, w, h);

      m_overlayW = w;
      m_overlayH = h;
   }
};


/*
 *----------------------------------------------------------------------
 *
 * Class LocalOverlayManager --
 *
 *    A single RPCManager object must be declared in global
 *    scope so that the the constructor is executed before
 *    VDPService_PluginGetTokenName() is called.
 *
 *----------------------------------------------------------------------
 */
class LocalOverlayManager : public RPCManager
{
public:
   LocalOverlayManager() : RPCManager(LOCAL_OVERLAY_TOKEN_NAME) { }
   virtual RPCPluginInstance* OnCreateInstance() { return new LocalOverlayPlugin(this); }
   virtual void OnDestroyInstance(RPCPluginInstance *plugin) { delete plugin; plugin = NULL; }
};

LocalOverlayManager localOverlayManager;

VDP_SERVICE_CREATE_INTERFACE(LOCAL_OVERLAY_TOKEN_NAME, localOverlayManager)
