/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    Resource.c

Abstract:

    This module implements the executive functions to acquire and release
    a shared resource.

Author:

    Mark Lucovsky       (markl)     04-Aug-1989

Environment:

    These routines are statically linked in the caller's executable and
    are callable in only from user mode.  They make use of Nt system
    services.

Revision History:

--*/

#include <tcpdllp.hxx>
#include <tsres.hxx>

//
//  The semaphore wait time before retrying the wait
//

#define INET_RES_TIMEOUT            (2 * 60 * 1000)



BOOL
InetInitializeResource(
    IN PRTL_RESOURCE Resource
    )

/*++

Routine Description:

    This routine initializes the input resource variable

Arguments:

    Resource - Supplies the resource variable being initialized

Return Value:

    None

--*/

{
    //
    //  Initialize the lock fields, the count indicates how many are waiting
    //  to enter or are in the critical section, LockSemaphore is the object
    //  to wait on when entering the critical section.  SpinLock is used
    //  for the add interlock instruction.
    //

    InitializeCriticalSection( &Resource->CriticalSection );

#ifndef CHICAGO
    Resource->CriticalSection.DebugInfo->Type = RTL_RESOURCE_TYPE;
#endif
    Resource->DebugInfo = NULL;

    //
    //  Initialize flags so there is a default value.
    //  (Some apps may set RTL_RESOURCE_FLAGS_LONG_TERM to affect timeouts.)
    //

    Resource->Flags = 0;


    //
    //  Initialize the shared and exclusive waiting counters and semaphore.
    //  The counters indicate how many are waiting for access to the resource
    //  and the semaphores are used to wait on the resource.  Note that
    //  the semaphores can also indicate the number waiting for a resource
    //  however there is a race condition in the alogrithm on the acquire
    //  side if count if not updated before the critical section is exited.
    //

     Resource->SharedSemaphore = CreateSemaphore( NULL,
                                                  0,
                                                  MAXLONG,
                                                  NULL );

    if ( !Resource->SharedSemaphore ) {
        return FALSE;
    }

    Resource->NumberOfWaitingShared = 0;

    Resource->ExclusiveSemaphore = CreateSemaphore( NULL,
                                                    0,
                                                    MAXLONG,
                                                    NULL );
    if ( !Resource->ExclusiveSemaphore ){
        CloseHandle( Resource->SharedSemaphore );

        return FALSE;
    }

    Resource->NumberOfWaitingExclusive = 0;

    //
    //  Initialize the current state of the resource
    //

    Resource->NumberOfActive = 0;

    Resource->ExclusiveOwnerThread = NULL;

    return TRUE;
}


BOOL
InetAcquireResourceShared(
    IN PRTL_RESOURCE Resource,
    IN BOOL          Wait
    )

/*++

Routine Description:

    The routine acquires the resource for shared access.  Upon return from
    the procedure the resource is acquired for shared access.

Arguments:

    Resource - Supplies the resource to acquire

    Wait - Indicates if the call is allowed to wait for the resource
        to become available for must return immediately

Return Value:

    BOOL - TRUE if the resource is acquired and FALSE otherwise

--*/

{
    DWORD          ret;
    ULONG          TimeoutCount = 0;
    DWORD          TimeoutTime  = INET_RES_TIMEOUT;
    //
    //  Enter the critical section
    //

    EnterCriticalSection(&Resource->CriticalSection);

    //
    //  If it is not currently acquired for exclusive use then we can acquire
    //  the resource for shared access.  Note that this can potentially
    //  starve an exclusive waiter however, this is necessary given the
    //  ability to recursively acquire the resource shared.  Otherwise we
    //  might/will reach a deadlock situation where a thread tries to acquire
    //  the resource recusively shared but is blocked by an exclusive waiter.
    //
    //  The test to reanable not starving an exclusive waiter is:
    //
    //      if ((Resource->NumberOfWaitingExclusive == 0) &&
    //          (Resource->NumberOfActive >= 0)) {
    //

    if (Resource->NumberOfActive >= 0) {

        //
        //  The resource is ours, so indicate that we have it and
        //  exit the critical section
        //

        Resource->NumberOfActive += 1;

        LeaveCriticalSection(&Resource->CriticalSection);

    //
    //  Otherwise check to see if this thread is the one currently holding
    //  exclusive access to the resource.  And if it is then we change
    //  this shared request to an exclusive recusive request and grant
    //  access to the resource.
    //

    } else if (Resource->ExclusiveOwnerThread == (HANDLE) GetCurrentThreadId()) {

        //
        //  The resource is ours (recusively) so indicate that we have it
        //  and exit the critial section
        //

        Resource->NumberOfActive -= 1;

        LeaveCriticalSection(&Resource->CriticalSection);

    //
    //  Otherwise we'll have to wait for access.
    //

    } else {

        //
        //  Check if we are allowed to wait or must return immedately, and
        //  indicate that we didn't acquire the resource
        //

        if (!Wait) {

            LeaveCriticalSection(&Resource->CriticalSection);

            return FALSE;

        }

        //
        //  Otherwise we need to wait to acquire the resource.
        //  To wait we will increment the number of waiting shared,
        //  release the lock, and wait on the shared semaphore
        //

        Resource->NumberOfWaitingShared += 1;

        LeaveCriticalSection(&Resource->CriticalSection);

rewait:
        if ( Resource->Flags & RTL_RESOURCE_FLAG_LONG_TERM ) {
            TimeoutTime = INFINITE;
        }
        ret = WaitForSingleObject(
                    Resource->SharedSemaphore,
                    TimeoutTime
                    );

        if ( ret == WAIT_TIMEOUT ) {
            OutputDebugString( "[InetAcquireResourceShared] Sem timeout\n" );

            TimeoutCount++;
            if ( TimeoutCount > 2 ) {
#if DBG
                DebugBreak();
#endif
                }
            OutputDebugString("[InetAcquireResourceShared]: Re-Waiting\n");
            goto rewait;
        } else if ( ret != WAIT_OBJECT_0 ) {
            OutputDebugString("[InetAcquireResourceShared]: WaitForSingleObjectFailed\n");
        }
    }

    //
    //  Now the resource is ours, for shared access
    //

    return TRUE;

}


BOOL
InetAcquireResourceExclusive(
    IN PRTL_RESOURCE Resource,
    IN BOOL Wait
    )

/*++

Routine Description:

    The routine acquires the resource for exclusive access.  Upon return from
    the procedure the resource is acquired for exclusive access.

Arguments:

    Resource - Supplies the resource to acquire

    Wait - Indicates if the call is allowed to wait for the resource
        to become available for must return immediately

Return Value:

    BOOL - TRUE if the resource is acquired and FALSE otherwise

--*/

{
    ULONG TimeoutCount = 0;
    DWORD TimeoutTime  = INET_RES_TIMEOUT;
    DWORD ret;

    //
    //  Loop until the resource is ours or exit if we cannot wait.
    //

    while (TRUE) {

        //
        //  Enter the critical section
        //

        EnterCriticalSection(&Resource->CriticalSection);

        //
        //  If there are no shared users and it is not currently acquired for
        //  exclusive use then we can acquire the resource for exclusive
        //  access.  We also can acquire it if the resource indicates exclusive
        //  access but there isn't currently an owner.
        //

        if ((Resource->NumberOfActive == 0)

                ||

            ((Resource->NumberOfActive == -1) &&
             (Resource->ExclusiveOwnerThread == NULL))) {

            //
            //  The resource is ours, so indicate that we have it and
            //  exit the critical section
            //

            Resource->NumberOfActive = -1;

            Resource->ExclusiveOwnerThread = (HANDLE) GetCurrentThreadId();

            LeaveCriticalSection(&Resource->CriticalSection);

            return TRUE;

        }

        //
        //  Otherwise check to see if we already have exclusive access to the
        //  resource and can simply recusively acquire it again.
        //

        if (Resource->ExclusiveOwnerThread == (HANDLE) GetCurrentThreadId()) {

            //
            //  The resource is ours (recusively) so indicate that we have it
            //  and exit the critial section
            //

            Resource->NumberOfActive -= 1;

            LeaveCriticalSection(&Resource->CriticalSection);

            return TRUE;

        }

        //
        //  Check if we are allowed to wait or must return immedately, and
        //  indicate that we didn't acquire the resource
        //

        if (!Wait) {

            LeaveCriticalSection(&Resource->CriticalSection);

            return FALSE;

        }

        //
        //  Otherwise we need to wait to acquire the resource.
        //  To wait we will increment the number of waiting exclusive,
        //  release the lock, and wait on the exclusive semaphore
        //

        Resource->NumberOfWaitingExclusive += 1;

        LeaveCriticalSection(&Resource->CriticalSection);

rewait:
        if ( Resource->Flags & RTL_RESOURCE_FLAG_LONG_TERM ) {
            TimeoutTime = INFINITE;
        }
        ret = WaitForSingleObject(
                    Resource->ExclusiveSemaphore,
                    TimeoutTime
                    );

        if ( ret == WAIT_TIMEOUT ) {
            OutputDebugString( "[InetAcquireResourceExclusive] Sem timeout\n" );

            TimeoutCount++;
            if ( TimeoutCount > 2 ) {
#if DBG
                DebugBreak();
#endif
                }
            OutputDebugString("[InetAcquireResourceExclusive]: Re-Waiting\n");
            goto rewait;
        } else if ( ret != WAIT_OBJECT_0 ) {
            OutputDebugString("[InetAcquireResourceExclusive]: WaitForSingleObject Failed\n");
        }
    }

    return TRUE;
}


BOOL
InetReleaseResource(
    IN PRTL_RESOURCE Resource
    )

/*++

Routine Description:

    This routine release the input resource.  The resource can have been
    acquired for either shared or exclusive access.

Arguments:

    Resource - Supplies the resource to release

Return Value:

    None.

--*/

{
    LONG PreviousCount;

    //
    //  Enter the critical section
    //

    EnterCriticalSection(&Resource->CriticalSection);

    //
    //  Test if the resource is acquired for shared or exclusive access
    //

    if (Resource->NumberOfActive > 0) {

        //
        //  Releasing shared access to the resource, so decrement
        //  the number of shared users
        //

        Resource->NumberOfActive -= 1;

        //
        //  If the resource is now available and there is a waiting
        //  exclusive user then give the resource to the waiting thread
        //

        if ((Resource->NumberOfActive == 0) &&
            (Resource->NumberOfWaitingExclusive > 0)) {

            //
            //  Set the resource state to exclusive (but not owned),
            //  decrement the number of waiting exclusive, and release
            //  one exclusive waiter
            //

            Resource->NumberOfActive = -1;
            Resource->ExclusiveOwnerThread = NULL;

            Resource->NumberOfWaitingExclusive -= 1;

            if ( !ReleaseSemaphore(
                         Resource->ExclusiveSemaphore,
                         1,
                         &PreviousCount
                         )) {
                return FALSE;
            }
        }

    } else if (Resource->NumberOfActive < 0) {

        //
        //  Releasing exclusive access to the resource, so increment the
        //  number of active by one.  And continue testing only
        //  if the resource is now available.
        //

        Resource->NumberOfActive += 1;

        if (Resource->NumberOfActive == 0) {

            //
            //  The resource is now available.  Remove ourselves as the
            //  owner thread
            //

            Resource->ExclusiveOwnerThread = NULL;

            //
            //  If there is another waiting exclusive then give the resource
            //  to it.
            //

            if (Resource->NumberOfWaitingExclusive > 0) {

                //
                //  Set the resource to exclusive, and its owner undefined.
                //  Decrement the number of waiting exclusive and release one
                //  exclusive waiter
                //

                Resource->NumberOfActive = -1;
                Resource->NumberOfWaitingExclusive -= 1;

                if ( !ReleaseSemaphore(
                             Resource->ExclusiveSemaphore,
                             1,
                             &PreviousCount
                             )) {
                    return FALSE;
                }

            //
            //  Check to see if there are waiting shared, who should now get
            //  the resource
            //

            } else if (Resource->NumberOfWaitingShared > 0) {

                //
                //  Set the new state to indicate that all of the shared
                //  requesters have access and there are no more waiting
                //  shared requesters, and then release all of the shared
                //  requsters
                //

                Resource->NumberOfActive = Resource->NumberOfWaitingShared;

                Resource->NumberOfWaitingShared = 0;

                if ( !ReleaseSemaphore(
                             Resource->SharedSemaphore,
                             Resource->NumberOfActive,
                             &PreviousCount
                             )) {
                    return FALSE;
                }
            }
        }

#if DBG
    } else {

        //
        //  The resource isn't current acquired, there is nothing to release
        //  so tell the user the mistake
        //


        OutputDebugString("[InetReleaseResource] - Resource released too many times!\n");
        DebugBreak();
#endif
    }

    //
    //  Exit the critical section, and return to the caller
    //

    LeaveCriticalSection(&Resource->CriticalSection);

    return TRUE;
}


BOOL
InetConvertSharedToExclusive(
    IN PRTL_RESOURCE Resource
    )

/*++

Routine Description:

    This routine converts a resource acquired for shared access into
    one acquired for exclusive access.  Upon return from the procedure
    the resource is acquired for exclusive access

Arguments:

    Resource - Supplies the resource to acquire for shared access, it
        must already be acquired for shared access

Return Value:

    None

--*/

{
    DWORD ret;
    DWORD TimeoutTime  = INET_RES_TIMEOUT;
    ULONG TimeoutCount = 0;

    //
    //  Enter the critical section
    //

    EnterCriticalSection(&Resource->CriticalSection);

    //
    //  If there is only one shared user (it's us) and we can acquire the
    //  resource for exclusive access.
    //

    if (Resource->NumberOfActive == 1) {

        //
        //  The resource is ours, so indicate that we have it and
        //  exit the critical section, and return
        //

        Resource->NumberOfActive = -1;

        Resource->ExclusiveOwnerThread = (HANDLE) GetCurrentThreadId();

        LeaveCriticalSection(&Resource->CriticalSection);

        return TRUE;
    }

    //
    //  If the resource is currently acquired exclusive and it's us then
    //  we already have exclusive access
    //

    if ((Resource->NumberOfActive < 0) &&
        (Resource->ExclusiveOwnerThread == (HANDLE) GetCurrentThreadId())) {

        //
        //  We already have exclusive access to the resource so we'll just
        //  exit the critical section and return
        //

        LeaveCriticalSection(&Resource->CriticalSection);

        return TRUE;
    }

    //
    //  If the resource is acquired by more than one shared then we need
    //  to wait to get exclusive access to the resource
    //

    if (Resource->NumberOfActive > 1) {

        //
        //  To wait we will decrement the fact that we have the resource for
        //  shared, and then loop waiting on the exclusive lock, and then
        //  testing to see if we can get exclusive access to the resource
        //

        Resource->NumberOfActive -= 1;

        while (TRUE) {

            //
            //  Increment the number of waiting exclusive, exit and critical
            //  section and wait on the exclusive semaphore
            //

            Resource->NumberOfWaitingExclusive += 1;

            LeaveCriticalSection(&Resource->CriticalSection);
rewait:
        if ( Resource->Flags & RTL_RESOURCE_FLAG_LONG_TERM ) {
            TimeoutTime = INFINITE;
        }
        ret = WaitForSingleObject(
                    Resource->ExclusiveSemaphore,
                    TimeoutTime
                    );

        if ( ret == WAIT_TIMEOUT ) {
            OutputDebugString( "[InetConverSharedToExclusive] Sem timeout\n" );

            TimeoutCount++;
            if ( TimeoutCount > 2 ) {
#if DBG
                DebugBreak();
#endif
                }
            OutputDebugString("[InetConvertSharedToExclusive]: Re-Waiting\n");
            goto rewait;
        } else if ( ret != WAIT_OBJECT_0 ) {
            OutputDebugString("[InetConvertSharedToExclusive]: WaitForSingleObject Failed\n");

            return FALSE;
        }

            //
            //  Enter the critical section
            //

            EnterCriticalSection(&Resource->CriticalSection);

            //
            //  If there are no shared users and it is not currently acquired
            //  for exclusive use then we can acquire the resource for
            //  exclusive access.  We can also acquire it if the resource
            //  indicates exclusive access but there isn't currently an owner
            //

            if ((Resource->NumberOfActive == 0)

                    ||

                ((Resource->NumberOfActive == -1) &&
                 (Resource->ExclusiveOwnerThread == NULL))) {

                //
                //  The resource is ours, so indicate that we have it and
                //  exit the critical section and return.
                //

                Resource->NumberOfActive = -1;

                Resource->ExclusiveOwnerThread = (HANDLE) GetCurrentThreadId();

                LeaveCriticalSection(&Resource->CriticalSection);

                return TRUE;
            }

            //
            //  Otherwise check to see if we already have exclusive access to
            //  the resource and can simply recusively acquire it again.
            //

            if (Resource->ExclusiveOwnerThread == (HANDLE) GetCurrentThreadId()) {

                //
                //  The resource is ours (recusively) so indicate that we have
                //  it and exit the critical section and return.
                //

                Resource->NumberOfActive -= 1;

                LeaveCriticalSection(&Resource->CriticalSection);

                return TRUE;
            }
        }

    }

    //
    //  The resource is not currently acquired for shared so this is a
    //  spurious call
    //

#if DBG
    OutputDebugString("[InetConvertSharedToExclusive]:  Failed error - SHARED_RESOURCE_CONV_ERROR\n");
    DebugBreak();
#endif

    return FALSE;
}


BOOL
InetConvertExclusiveToShared(
    IN PRTL_RESOURCE Resource
    )

/*++

Routine Description:

    This routine converts a resource acquired for exclusive access into
    one acquired for shared access.  Upon return from the procedure
    the resource is acquired for shared access

Arguments:

    Resource - Supplies the resource to acquire for shared access, it
        must already be acquired for exclusive access

Return Value:

    None

--*/

{
    LONG PreviousCount;

    //
    //  Enter the critical section
    //

    EnterCriticalSection(&Resource->CriticalSection);

    //
    //  If there is only one shared user (it's us) and we can acquire the
    //  resource for exclusive access.
    //

    if (Resource->NumberOfActive == -1) {

        Resource->ExclusiveOwnerThread = NULL;

        //
        //  Check to see if there are waiting shared, who should now get the
        //  resource along with us
        //

        if (Resource->NumberOfWaitingShared > 0) {

            //
            //  Set the new state to indicate that all of the shared requesters
            //  have access including us, and there are no more waiting shared
            //  requesters, and then release all of the shared requsters
            //

            Resource->NumberOfActive = Resource->NumberOfWaitingShared + 1;

            Resource->NumberOfWaitingShared = 0;

            if ( !ReleaseSemaphore(
                         Resource->SharedSemaphore,
                         Resource->NumberOfActive - 1,
                         &PreviousCount
                         )) {
                return FALSE;
            }
        } else {

            //
            //  There is no one waiting for shared access so it's only ours
            //

            Resource->NumberOfActive = 1;

        }

        LeaveCriticalSection(&Resource->CriticalSection);

        return TRUE;

    }

    //
    //  The resource is not currently acquired for exclusive, or we've
    //  recursively acquired it, so this must be a spurious call
    //

#if DBG
    OutputDebugString("[InetConvertExclusiveToShared] Failed error - SHARED_RESOURCE_CONV_ERROR\n");
    DebugBreak();
#endif

    return FALSE;
}


VOID
InetDeleteResource (
    IN PRTL_RESOURCE Resource
    )

/*++

Routine Description:

    This routine deletes (i.e., uninitializes) the input resource variable


Arguments:

    Resource - Supplies the resource variable being deleted

Return Value:

    None

--*/

{
    DeleteCriticalSection( &Resource->CriticalSection );
    CloseHandle(Resource->SharedSemaphore);
    CloseHandle(Resource->ExclusiveSemaphore);

    RtlZeroMemory( Resource, sizeof( *Resource ) );

    return;
}

