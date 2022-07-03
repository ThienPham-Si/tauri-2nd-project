/* ********************************************************************************* *
 * Copyright (C) 2012-2021 VMware, Inc.  All rights reserved. -- VMware Confidential *
 * ********************************************************************************* */

/* **************************************************************************
 * How to build VMR9Overlay
 * **************************************************************************/
1) DirectX SDK
   a) VMR9Overlay is built with the DirectX SDK June 2010.  We suggest using
      the same version to build VMR9Overlay.

   b) DirectX SDK June 2010 can be downloaded from here:
      http://www.microsoft.com/download/en/details.aspx?id=6812

   c) Installing the SDK will automatically create the global environment
      variable DXSDK_DIR.  The Visual Studio projects will reference this
      variable to find the DirectX headers and libraries.

   d) This sample makes use of the VMR9 Overlay that is part of D3D9.  Run
      the DXDiag utility and verify that the D3D9 Overlay is supported or
      video playback will fail.  See this webpage for more information.
      https://support.microsoft.com/en-us/help/3089258/video-playback-may-fail-when-directx-9-overlays-are-required-in-window

   e) Load VMR9Overlay.sln and build.  You can build it as either a 32-bit
      or a 64-bit application.

   f) Because of the dependancy on DirectX, this sample is not supported
      on other platforms.


/* **************************************************************************
 * How to install VMR9Overlay
 * **************************************************************************/
   1) On local client
      a) Install VMware Horizon View Client.

      b) Copy the correct version (32-bit or 64-bit) VMR9OverlayClient.dll
         to a folder of your choice.

         The bit level (i.e. 32-bit/64-bit) of VMR9OverlayClient.dll
         must match the bit level of the VMware Horizon View Client
         that is installed.

      c) Run "regsvr32 VMR9OverlayClient.dll".  This registers
         VMR9OverlayClient.dll as a vdpservice plugin in the registry.
         The registration may fail unless you run the command "As Administrator".

      d) You may run "regsvr32 -u VMR9OverlayClient.dll" to unregister the
         plugin and remove the registry entries.


   2) On the remote desktop
      a) Install VMware Horizon View Agent.

      b) Copy the compiled VMR9OverlayGuest.exe application to a folder
         of your choice.

      c) Previous versions of the SDK included vdpService.dll.  This DLL is
         no longer included as part of the SDK because it is included with
         VMware Horizon View and your application must use the version of
         vdpService.dll that ships with VMware Horizon View.  Using a
         different version of vdpService.dll may cause problems in the
         future because this DLL is matched with other DLLs that ship with
         View.  The SDK now includes vdpService_import.cpp as a replacement
         for the import library, which also knows how to find and load the
         correct version of vdpService.dll.

      d) If the Virtual Channel Security feature is enabled you will need to
         apply VMR9OverlayGuest.reg.  This is just a template file, you must
         modify the file to include the full path to VMR9OverlayGuest.exe
         before applying it.  You must also log out and log back in to make
         sure the virtual channel security settings have been updated.


/* **************************************************************************
 * How to run VMR9Overlay
 * **************************************************************************/
   1) Launch VMware Horizon View Client to connect to the remote desktop.

   2) Run LocalOverlayGuest.exe in the remote desktop, this opens a window on
      the remote desktop.

   3) Select File/Open, this will open a file selector dialog on the local client.

   4) Select a video file, the video must exist on the local client.

   5) The video will decode and playback on the local client but will appear to
      be playing in the window that was opened on the remote desktop.
