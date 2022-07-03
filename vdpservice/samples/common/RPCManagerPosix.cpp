/* ********************************************************************************* *
 * Copyright (C) 2012-2021 VMware, Inc.  All rights reserved. -- VMware Confidential *
 * ********************************************************************************* */

/*
 * RPCManagerPosix.cpp --
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
 *    Returns false (non-Windows Agent not supported).
 *
 * Side Effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

bool
RPCManager::AllowRunAsServer()
{
   return false;
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
   m_hReadyEvent = NULL;
   m_pendingMsgMutex = NULL;
   m_pendingMsgEvent = NULL;
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
}
