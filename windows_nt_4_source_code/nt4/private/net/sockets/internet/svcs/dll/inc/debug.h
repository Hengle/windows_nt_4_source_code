/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    debug.h

Abstract:

    Contains data definitions for debug code.

Author:

    Madan Appiah (madana) 15-Nov-1994

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _DEBUG_
#define _DEBUG_

#ifdef __cplusplus
extern "C" {
#endif

//
// LOW WORD bit mask (0x0000FFFF) for low frequency debug output.
//
#define DEBUG_ERRORS            0x00000001  // hard errors.
#define DEBUG_REGISTRY          0x00000002  // debug registry calls
#define DEBUG_MISC              0x00000004  // misc info.
#define DEBUG_SCAVENGER         0x00000008  // scavenger debug info.

#define DEBUG_SORT              0x00000010  // debug B-TREE functions
#define DEBUG_CONTAINER         0x00000020  // debug container
#define DEBUG_APIS              0x00000040  // debug tcpsvcs apis
#define DEBUG_FILE_VALIDATE     0x00000080 // validate file map file
#define DEBUG_SVCLOC_MESSAGE    0x00000100  // discovery messages



//
// HIGH WORD bit mask (0x0000FFFF) for high frequency debug output.
// ie more verbose.
//

#define DEBUG_TIMESTAMP         0x00010000  // print time stamps
#define DEBUG_MEM_ALLOC         0x00020000 // memory alloc
#define DEBUG_STARTUP_BRK       0x40000000  // breakin debugger during startup.

#if DBG

#define DEBUG_PRINT OutputDebugString

//
// debug functions.
//

extern DWORD GlobalDebugFlag;
extern CRITICAL_SECTION GlobalDebugCritSect;

#define IF_DEBUG(flag) if (GlobalDebugFlag & (DEBUG_ ## flag))
#define TcpsvcsDbgPrint(_x_) TcpsvcsDbgPrintRoutine _x_

VOID
TcpsvcsDbgPrintRoutine(
    IN DWORD DebugFlag,
    IN LPSTR Format,
    ...
    );

VOID
TcpsvcsDbgAssertFailed(
    LPSTR FailedAssertion,
    LPSTR FileName,
    DWORD LineNumber,
    LPSTR Message
    );

#define TcpsvcsDbgAssert(Predicate) \
    { \
        if (!(Predicate)) \
            TcpsvcsDbgAssertFailed( #Predicate, __FILE__, __LINE__, NULL ); \
    }

#else

#define IF_DEBUG(flag) if (FALSE)

#define TcpsvcsDbgPrint(_x_)
#define TcpsvcsDbgAssert(_x_)

#endif // DBG

#ifdef __cplusplus
}
#endif

#endif  // _DEBUG_
