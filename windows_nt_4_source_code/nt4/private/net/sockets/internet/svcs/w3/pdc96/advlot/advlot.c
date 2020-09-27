/*++

Copyright (c) 1996  Microsoft Corporation

This program is released into the public domain for any purpose.

Module Name:

    advlot.c

Abstract:

    This module is an example of an effective multi-threaded ISAPI Application.

Revision History:

--*/

#include <windows.h>
#include <httpext.h>

#include <lottery.h>

//
//  Constants
//

//
//  This is the maximum number of threads we'll allow in the pool
//

#define MAX_THREADS                 8

//
//  This is the number of threads per processor to create.  If the threads
//  are heavily IO bound (waiting on network connections for example), a higher
//  number might be appropriate.  If the threads are CPU bound, a lower number
//  would be appropriate.
//

#define THREADS_PER_PROCESSOR       2

//
//  This is the maximum number of items we'll allow on the work queue.  If
//  this number is exceeded.
//

#define MAX_WORK_QUEUE_ITEMS        100

//
//  The text to display when there are too many outstanding work items
//

#define SERVER_TOO_BUSY_TEXT        "<head><title>Server too busy</title></head>" \
                                    "<body><h2>The server is too busy to give" \
                                    "your lucky lottery number right now."     \
                                    "Please try again later.\n"

//
//  Definitions
//

//
//  This is the structure of a work queue item
//

typedef struct _WORK_QUEUE_ITEM
{
    LIST_ENTRY                ListEntry;
    HANDLE                    hImpersonationToken;
    EXTENSION_CONTROL_BLOCK * pecb;
} WORK_QUEUE_ITEM;

//
//  Globals
//

//
//  Protects the work queue and free queue
//

CRITICAL_SECTION csQueueLock;

//
//  List of work items in a doubly linked circular list
//

LIST_ENTRY WorkQueueList;

//
//  List of free WORK_QUEUE_ITEM structures
//

LIST_ENTRY FreeQueueList;

//
//  Number of items on the work queue
//

DWORD cQueueItems = 0;

//
//  Array of thread handles for shutting down
//

HANDLE ahThread[ MAX_THREADS ];

//
//  Use a semaphore to indicate there's work to be performed.  We use a
//  semaphore rather then an event because a semaphore tracks how many times
//  it has been signalled
//

HANDLE hWorkSem = NULL;

//
//  Prototypes
//

DWORD
WINAPI
PoolThread(
    LPDWORD lpParams
    );

WORK_QUEUE_ITEM *
AllocateWorkItem(
    VOID
    );

VOID
FreeWorkItem(
    WORK_QUEUE_ITEM * pwqi
    );

BOOL
SendError(
    EXTENSION_CONTROL_BLOCK * pecb,
    CHAR *                    pszStatus,
    CHAR *                    pszMessage
    );

//
// Functions
//

BOOL
WINAPI
DllLibMain(
     IN HINSTANCE hinstDll,
     IN DWORD     fdwReason,
     IN LPVOID    lpvContext OPTIONAL
     )
/*++

 Routine Description:

   This function DllLibMain() is the main initialization function for
    this DLL. It initializes local variables and prepares it to be invoked
    subsequently.

 Arguments:

   hinstDll          Instance Handle of the DLL
   fdwReason         Reason why NT called this DLL
   lpvReserved       Reserved parameter for future use.

 Return Value:

    Returns TRUE is successful; otherwise FALSE is returned.
--*/
{
    BOOL        fReturn = TRUE;
    SYSTEM_INFO si;
    DWORD       i;
    DWORD       dwThreadId;

    switch (fdwReason )
    {
    case DLL_PROCESS_ATTACH:

        //
        // Initialize various data and modules.
        //

        if ( !InitializeLottery() )
        {
            fReturn = FALSE;
            break;
        }

        WorkQueueList.Flink = WorkQueueList.Blink = &WorkQueueList;
        FreeQueueList.Flink = FreeQueueList.Blink = &FreeQueueList;

        memset( ahThread, 0, sizeof( ahThread ));

        hWorkSem = CreateSemaphore( NULL,
                                    0,          // Not signalled initially
                                    0x7fffffff, // Max reference count
                                    NULL );

        if ( !hWorkSem )
        {
            return FALSE;
        }

        InitializeCriticalSection( &csQueueLock );

        //
        //  We don't care about thread attach/detach notifications
        //

        DisableThreadLibraryCalls( hinstDll );

        //
        //  Create our thread pool, two times the number of processors
        //

        GetSystemInfo( &si );

        for ( i = 0;
              i < THREADS_PER_PROCESSOR * si.dwNumberOfProcessors &&
              i < MAX_THREADS;
              i++ )
        {
            ahThread[i] = CreateThread( NULL,
                                        0,
                                        (LPTHREAD_START_ROUTINE) PoolThread,
                                        NULL,
                                        0,
                                        &dwThreadId );

            if ( !ahThread[i] )
            {
                CloseHandle( hWorkSem );
                DeleteCriticalSection( &csQueueLock );
                return FALSE;
            }

            CloseHandle( ahThread[i] );
        }
        break;

    case DLL_PROCESS_DETACH:
        {

            //
            // Only cleanup when we are called because of a FreeLibrary().
            //  i.e., when lpvContext == NULL
            // If we are called because of a process termination,
            //  dont free anything. System will free resources and memory for us.
            //

            if ( lpvContext != NULL)
            {
                TerminateLottery();

                //
                //  The only termination notice is process detach, so care
                //  must be taken when cleaning up threads.
                //
                //  Delete any pool threads.  We know all of the pool threads
                //  are waiting for work items so calling TerminateThread is
                //  safe.
                //
                //  Alternately, you could just do a LoadLibrary on this dll
                //  and let the terminate process cleanup.
                //

                i = 0;
                while ( ahThread[i] != NULL )
                {
                    if ( !TerminateThread( ahThread[i], 0 ) )
                    {
                        OutputDebugString( "Warning: Failed to terminate thread\n");
                    }
                }

                DeleteCriticalSection( &csQueueLock );
                CloseHandle( hWorkSem );
            }

            break;
        } /* case DLL_PROCESS_DETACH */

    default:
        break;
    }   /* switch */

    return ( fReturn);
}  /* DllLibMain() */



BOOL
GetExtensionVersion(
    HSE_VERSION_INFO * pver
    )
/*++

Routine Description:
    This is the first function that is called when this ISAPI DLL is loaded.
    We should fill in the version information in the structure passed in.

Arguments:
    pVer - pointer to Server Extension Version Information structure.

Returns:
    TRUE for success and FALSE for failure.
    On success the valid version information is stored in *pVer.
--*/
{
    pver->dwExtensionVersion = MAKELONG( HSE_VERSION_MINOR, HSE_VERSION_MAJOR );

    strcpy( pver->lpszExtensionDesc,
            "Multi-threaded ISAPI Application example, v 1.0" );

    return TRUE;
}

DWORD
WINAPI
HttpExtensionProc(
    EXTENSION_CONTROL_BLOCK * pecb
    )
/*++

Routine Description:
    This is the main function that is called for this ISAPI Extension.
    This function processes the request and sends out appropriate response.

Arguments:
    pecb  - pointer to EXTENSION_CONTROL_BLOCK, which contains most of the
            required variables for the extension called. In addition,
    it contains the various callbacks as appropriate.

Returns:
    HSE_STATUS code indicating the succes/failure of this call.
--*/
{
    WORK_QUEUE_ITEM * pwqi;
    DWORD             cb;
    BOOL              fRet;
    HANDLE            hImpersonationToken;

    //
    //  Is the list too long?  If so, tell the user to come back later
    //

    if ( cQueueItems + 1 > MAX_WORK_QUEUE_ITEMS )
    {
        fRet = SendError( pecb,
                          "503 Server too busy",
                          SERVER_TOO_BUSY_TEXT );

        pecb->dwHttpStatusCode = 501;

        return fRet ? HSE_STATUS_SUCCESS : HSE_STATUS_ERROR;
    }

    //
    //  Capture the current impersonation token so we can impersonate this
    //  user in the other thread
    //

    if ( !OpenThreadToken( GetCurrentThread(),
                           TOKEN_QUERY | TOKEN_IMPERSONATE,
                           TRUE,
                           &hImpersonationToken ))
    {
        fRet = SendError( pecb,
                          "500 Failed to open thread token",
                          "Failed to open thread token" );

        pecb->dwHttpStatusCode = 500;

        return fRet ? HSE_STATUS_SUCCESS : HSE_STATUS_ERROR;
    }


    //
    //  Take the queue lock, get a queue item and put it on the queue
    //

    EnterCriticalSection( &csQueueLock );

    pwqi = AllocateWorkItem();

    if ( !pwqi )
    {
        fRet = SendError( pecb,
                          "500 Not enough memory",
                          "Failed to allocate work queue item" );

        pecb->dwHttpStatusCode = 500;

        LeaveCriticalSection( &csQueueLock );
        CloseHandle( hImpersonationToken );
        return fRet ? HSE_STATUS_SUCCESS : HSE_STATUS_ERROR;
    }

    //
    //  Initialize the work queue item and put it at the end of the list
    //

    pwqi->pecb = pecb;
    pwqi->hImpersonationToken = hImpersonationToken;

    pwqi->ListEntry.Flink = &WorkQueueList;
    pwqi->ListEntry.Blink = WorkQueueList.Blink;

    WorkQueueList.Blink->Flink = &pwqi->ListEntry;
    WorkQueueList.Blink        = &pwqi->ListEntry;

    cQueueItems++;

    LeaveCriticalSection( &csQueueLock );

    //
    //  Signal the pool threads there is work to be done
    //

    ReleaseSemaphore( hWorkSem, 1, NULL );

    return HSE_STATUS_PENDING;
}

DWORD
WINAPI
PoolThread(
    LPDWORD lpParams
    )
/*++

Routine Description:

    This is an ISAPI pool thread

--*/
{
    WORK_QUEUE_ITEM * pwqi;
    DWORD             res;

    while ( TRUE )
    {
        res = WaitForSingleObject( hWorkSem, INFINITE );

        if ( res == WAIT_OBJECT_0 )
        {
            //
            //  There's work to do, grab the queue lock and get the next
            //  work item
            //

            EnterCriticalSection( &csQueueLock );

            if ( WorkQueueList.Flink != &WorkQueueList )
            {
                pwqi = CONTAINING_RECORD( WorkQueueList.Flink,
                                          WORK_QUEUE_ITEM,
                                          ListEntry );

                pwqi->ListEntry.Flink->Blink = &WorkQueueList;
                WorkQueueList.Flink          = pwqi->ListEntry.Flink;

                cQueueItems--;
            }
            else
            {
                pwqi = NULL;
            }

            LeaveCriticalSection( &csQueueLock );

            if ( !pwqi )
                continue;

            //
            //  Impersonate the specified user so security is maintained
            //  accessing system resources
            //

            ImpersonateLoggedOnUser( pwqi->hImpersonationToken );

            SendLotteryNumber( pwqi->pecb );

            RevertToSelf();

            //
            //  Cleanup this work item
            //

            pwqi->pecb->ServerSupportFunction( pwqi->pecb->ConnID,
                                               HSE_REQ_DONE_WITH_SESSION,
                                               NULL,
                                               NULL,
                                               NULL );

            CloseHandle( pwqi->hImpersonationToken );
            FreeWorkItem( pwqi );
        }
    }

    return 0;
}

BOOL
SendError(
    EXTENSION_CONTROL_BLOCK * pecb,
    CHAR *                    pszStatus,
    CHAR *                    pszMessage
    )
/*++

Routine Description:

    Sends the specified error to the client

Arguments:

    pecb  - pointer to EXTENSION_CONTROL_BLOCK
    pszStatus - Status line of response ("501 Server busy")
    pszMessage - HTML message explaining the failure

Returns:

    TRUE on success, FALSE on failure
--*/
{
    BOOL  fRet;
    DWORD cb;

    //
    //  Send the headers
    //

    fRet = pecb->ServerSupportFunction( pecb->ConnID,
                                        HSE_REQ_SEND_RESPONSE_HEADER,
                                        pszStatus,
                                        NULL,
                                        (LPDWORD) "Content-Type: text/html\r\n\r\n" );

    //
    //  If that succeeded, send the message
    //

    if ( fRet )
    {
        cb = strlen( pszMessage );

        fRet = pecb->WriteClient( pecb->ConnID,
                                  pszMessage,
                                  &cb,
                                  0 );
    }

    return fRet;
}

WORK_QUEUE_ITEM *
AllocateWorkItem(
    VOID
    )
/*++

Routine Description:

    Allocates a work queue item by either retrieving one from the free list
    or allocating it from the heap.

    Note: THE QUEUE LOCK MUST BE TAKEN BEFORE CALLING THIS ROUTINE!

Returns:

    Work queue item on success, NULL on failure
--*/
{
    WORK_QUEUE_ITEM * pwqi;

    //
    //  If the list is not empty, take a work item off the list
    //

    if ( FreeQueueList.Flink != &FreeQueueList )
    {
        pwqi = CONTAINING_RECORD( FreeQueueList.Flink,
                                  WORK_QUEUE_ITEM,
                                  ListEntry );

        pwqi->ListEntry.Flink->Blink = &FreeQueueList;
        FreeQueueList.Flink          = pwqi->ListEntry.Flink;
    }
    else
    {
        pwqi = LocalAlloc( LPTR, sizeof( WORK_QUEUE_ITEM ));
    }

    return pwqi;
}

VOID
FreeWorkItem(
    WORK_QUEUE_ITEM * pwqi
    )
/*++

Routine Description:

    Frees the passed work queue item to the free list

    Note: This routine takes the queue lock.

Arguments:

    pwqi - Work queue item to free

--*/
{
    //
    //  Take the queue lock and put on the free list
    //

    EnterCriticalSection( &csQueueLock );

    pwqi->ListEntry.Flink = FreeQueueList.Flink;
    pwqi->ListEntry.Blink = &FreeQueueList;

    FreeQueueList.Flink->Blink = &pwqi->ListEntry;
    FreeQueueList.Flink        = &pwqi->ListEntry;

    LeaveCriticalSection( &csQueueLock );
}

