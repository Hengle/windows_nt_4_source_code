/*++

Copyright (c) 1996 Microsoft Corporation

MODULE NAME
    init.c

ABSTRACT
    Initialization for the Autodial helper DLL.

AUTHOR
    Anthony Discolo (adiscolo) 22-Apr-1996

REVISION HISTORY

--*/

#define UNICODE
#define _UNICODE

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <stdlib.h>
#include <windows.h>
#include <stdio.h>
#define DEBUGGLOBALS
#include <debug.h>



BOOLEAN
WINAPI
InitAcsHelperDLL(
    HINSTANCE   hinstDLL,
    DWORD       fdwReason,
    LPVOID      lpvReserved
    )

/*++

DESCRIPTION
    Initialize the DLL.  All we do right now is to initialize
    the debug tracing library.

ARGUMENTS
    hinstDLL:

    fdwReason:

    lpvReserved:

RETURN VALUE
    Always TRUE.

--*/

{
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        DEBUGINIT("RASADHLP");
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:
        DEBUGTERM();
        break;
    }
    return TRUE;
}
