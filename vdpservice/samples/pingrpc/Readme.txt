/* ********************************************************************************* *
 * Copyright (C) 2012-2021 VMware, Inc.  All rights reserved. -- VMware Confidential *
 * ********************************************************************************* */

/* **************************************************************************
 * How to build PingRPC
 * **************************************************************************/
   1) Load PingRPC.sln and build.  You can build it as either a 32-bit or a
      64-bit application.


/* **************************************************************************
 * How to install PingRPC
 * **************************************************************************/
   1) On local client machine
      a) Install Horizon VMware View Client.

      b) Copy the correct version (32-bit or 64-bit) PingRPCDll.dll to
         a folder on the client

         The bit level (i.e. 32-bit/64-bit) of the client side plugin must
         match the bit level of the Horizon View Client that is installed.

      c) Run "regsvr32 PingRPCDll.dll".  This registers PingRPCDll.dll as
         a vdpservice plugin in the registry.  The registration may fail
         unless you run the command "As Administrator".

      d) You may run "regsvr32 -u PingRPCDll.dll" to unregister the plugin
         and remove the registry entries.


   2) On the remote desktop
      a) Install VMware Horizon View Agent.

      b) Copy the compiled PingRPCExe.exe application to a folder of your choice.

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
         apply PingRPCExe.reg.  This is just a template file, you must modify
         the file to include the full path to PingRPCExe.exe before applying
         it.  You must also log out and log back in to make sure the virtual
         channel security settings have been updated.


/* **************************************************************************
 * How to run PingRPC
 * **************************************************************************/
   1) Launch VMware Horizon View Client to connect to the remote desktop.

   2) Run "PingRPCExe -n 4" in the remote desktop to send 4 pings.

   3) Run "PingRPCExe -h" for detailed commandline options.




/* **************************************************************************
 * Linux
 * **************************************************************************/

/* **************************************************************************
 * How to build PingRPC
 * **************************************************************************/
   1) Only the client side component, libPingRPC.so, can be built on Linux.
      There is a Makefile in the PingRPCDll folder which will build it.  The
      Makefile will also detect if the VMware Horizon View Client is installed
      on the system, and if so, automatically copy the binary to the correct
      folder.


/* **************************************************************************
 * How to install PingRPC
 * **************************************************************************/
   1) On local client
      a) Install VMware Horizon View Client.

      b) Copy libPingRPC.so to /usr/lib/vmware/view/vdpService.
         note: The Makefile might have already performed this step.

   2) On the remote desktop
      a) Follow the instructions under the Windows section.


/* **************************************************************************
 * How to run LocalOverlay
 * **************************************************************************/
   1) Follow steps under the Windows section.
