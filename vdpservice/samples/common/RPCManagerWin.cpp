/* ********************************************************************************* *
 * Copyright (C) 2012-2021 VMware, Inc.  All rights reserved. -- VMware Confidential *
 * ********************************************************************************* */

/*
 * RPCManagerWin.cpp --
 *
 */

#include "stdafx.h"
#include "RPCManager.h"


/*
 *----------------------------------------------------------------------
 *
 * RPCManager::AllowRunAsServer --
 *
 *    Whether the RPCManager is allowed to run as server.
 *
 * Results:
 *    Returns TRUE (Windows Agent supported).
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

bool
RPCManager::AllowRunAsServer()
{
   return true;
}


/*
 *----------------------------------------------------------------------
 *
 * RPCManager::RMWaitForEvent --
 *
 *    This method waits for the given event to become signaled.  While
 *    waiting it ensures that RPC is being given timeslices so that it
 *    can do its work.
 *
 * Results:
 *    Returns true if the event was signaled.
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

bool
RPCManager::RMWaitForEvent(HANDLE hEvent,    // IN
                           uint32 msTimeout) // IN
{
   uint32 msBegin = GetTickCount();
   uint32 msCurrent = 0;

   while (msCurrent < max(msTimeout,1)) {
      m_iChannel.v1.Poll();

      uint32 msSleep = msTimeout - msCurrent;
      if (msSleep > 100) msSleep = 100;

      if (hEvent != NULL) {
         DWORD rc = WaitForSingleObject(hEvent, msSleep);
         if (rc != WAIT_TIMEOUT) {
            return rc == WAIT_OBJECT_0;
         }
      } else {
         Sleep(msSleep);
      }

      uint32 msCurrent2 = GetTickCount() - msBegin;
      msCurrent = msCurrent2 >= msCurrent ? msCurrent2 : ~0;
   }

   return false;
}


/*
 *----------------------------------------------------------------------
 *
 * RPCPluginInstance::InitializeEventsAndMutexes --
 *
 *    Creates and initializes the Events and Mutexes required.
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
RPCPluginInstance::InitializeEventsAndMutexes()
{
   m_hReadyEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
   m_pendingMsgMutex = CreateMutex(NULL, FALSE, NULL);
   m_pendingMsgEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
}


/*
 *----------------------------------------------------------------------
 *
 * RPCPluginInstance::CloseEventsAndMutexes --
 *
 *    Closes the Events and Mutexes used by this instance.
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
RPCPluginInstance::CloseEventsAndMutexes()
{
   if (m_hReadyEvent != NULL) {
      CloseHandle(m_hReadyEvent);
      m_hReadyEvent = NULL;
   }

   if (m_pendingMsgEvent != NULL) {
      CloseHandle(m_pendingMsgEvent);
      m_pendingMsgEvent = NULL;
   }

   if (m_pendingMsgMutex != NULL) {
      CloseHandle(m_pendingMsgMutex);
      m_pendingMsgMutex = NULL;
   }
}


/*
 *----------------------------------------------------------------------
 *
 * RPCPluginInstance::RMLockMutex --
 *
 *    Locks the given mutex.
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
RPCPluginInstance::RMLockMutex(HANDLE hMutex) // IN
{
   WaitForSingleObject(hMutex, INFINITE);
}


/*
 *----------------------------------------------------------------------
 *
 * RPCPluginInstance::RMUnlockMutex --
 *
 *    Unlocks the given mutex.
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
RPCPluginInstance::RMUnlockMutex(HANDLE hMutex) // IN
{
   ReleaseMutex(hMutex);
}


/*
 *----------------------------------------------------------------------
 *
 * RPCPluginInstance::RMSetEvent --
 *
 *    Sets the given event.
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
RPCPluginInstance::RMSetEvent(HANDLE hEvent) // IN
{
   SetEvent(hEvent);
}


/*
 *----------------------------------------------------------------------
 *
 * RPCPluginInstance::RMResetEvent --
 *
 *    Resets the given event.
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
RPCPluginInstance::RMResetEvent(HANDLE hEvent) // IN
{
   ResetEvent(hEvent);
}
