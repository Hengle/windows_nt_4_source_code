//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       debug.c
//
//  Contents:   Debugging support functions
//
//  Classes:
//
//  Functions:
//
//  Note:       This file is not compiled for retail builds
//
//  History:    4-29-93   RichardW   Created
//
//----------------------------------------------------------------------------

//
//  For ease of debugging the SPMgr, all the debug support functions have
//  been stuck here.  Basically, we read info from win.ini, since that allows
//  us to configure the debug level via a text file (and DOS, for example).
//
//  Format is:
//
//  win.ini
//
//  [Gina]
//      DebugFlags=<Flag>[<,Flag>]*
//
//  WHERE:
//      Flag is one of the following:
//          Error, Warning, Trace
//

#if DBG         // NOTE:  This file not compiled for retail builds

#include "sslsspi.h"
#include <stdio.h>
#include <wchar.h>

DWORD   SslInfoLevel = 3;



// Debugging support functions.

// These two functions do not exist in Non-Debug builds.  They are wrappers
// to the commnot functions (maybe I should get rid of that as well...)
// that echo the message to a log file.

char   szSection[] = "SslSspi";
char * DebLevel[] = {"Ssl-Error", "Ssl-Warn", "Ssl-Trace"
                    };

typedef struct _DebugKeys {
    char *  Name;
    DWORD   Value;
} DebugKeys, *PDebugKeys;

DebugKeys   DebugKeyNames[] = {
                {"Error",       DEB_ERROR},
                {"Warning",     DEB_WARN},
                {"Trace",       DEB_TRACE},
                {"Message",     DEB_TRACE_MESSAGES},
                {"Cache",       DEB_TRACE_CACHE},
                {"Perf",        DEB_TRACE_PERF}
                };

#define NUM_DEBUG_KEYS  sizeof(DebugKeyNames) / sizeof(DebugKeys)
#define NUM_BREAK_KEYS  sizeof(BreakKeyNames) / sizeof(DebugKeys)

//+---------------------------------------------------------------------------
//
//  Function:   LogEvent
//
//  Synopsis:   Logs an event to the console and, optionally, a file.
//
//  Effects:
//
//  Arguments:  [Mask]   --
//              [Format] --
//              [Format] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    4-29-93   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void
LogEvent(   long            Mask,
            const char *    Format,
            ...)
{
    va_list ArgList;
    int     Level = 0;
    int     PrefixSize = 0;
    char    szOutString[256];
    long    OriginalMask = Mask;


    if (Mask & SslInfoLevel)
    {
        while (!(Mask & 1))
        {
            Level++;
            Mask >>= 1;
        }
        if (Level >= (sizeof(DebLevel) / sizeof(char *)) )
        {
            Level = (sizeof(DebLevel) / sizeof(char *)) - 1;
        }


        //
        // Make the prefix first:  "Process.Thread> GINA-XXX"
        //

        PrefixSize = sprintf(szOutString, "%d.%d> %s: ",
                GetCurrentProcessId(), GetCurrentThreadId(), DebLevel[Level]);


        va_start(ArgList, Format);

        if (_vsnprintf(&szOutString[PrefixSize], sizeof(szOutString) - PrefixSize,
                            Format, ArgList) < 0)
        {
            //
            // Less than zero indicates that the string could not be
            // fitted into the buffer.  Output a special message indicating
            // that:
            //

            OutputDebugStringA("GINA!LogEvent:  Could not pack string into 256 bytes\n");

        }
        else
        {
            OutputDebugStringA(szOutString);
        }


    }

}


DWORD
GetDebugKeyValue(
    PDebugKeys      KeyTable,
    int             cKeys,
    LPSTR           pszKey)
{
    int     i;

    for (i = 0; i < cKeys ; i++ )
    {
        if (_stricmp(KeyTable[i].Name, pszKey) == 0)
        {
            return(KeyTable[i].Value);
        }
    }
    return(0);
}

//+---------------------------------------------------------------------------
//
//  Function:   LoadDebugParameters
//
//  Synopsis:   Loads debug parameters from win.ini
//
//  Effects:
//
//  Arguments:  (none)
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    4-29-93   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------


void
LoadDebugParameters(void)
{
    char    szVal[128];
    char *  pszDebug;
    int     cbVal;

    cbVal = GetProfileStringA(szSection, "DebugFlags", "Error,Warning", szVal, sizeof(szVal));

    pszDebug = strtok(szVal, ", \t");
    while (pszDebug)
    {
        SslInfoLevel |= GetDebugKeyValue(DebugKeyNames, NUM_DEBUG_KEYS, pszDebug);
        pszDebug = strtok(NULL, ", \t");
    }

}

//+---------------------------------------------------------------------------
//
//  Function:   InitDebugSupport
//
//  Synopsis:   Initializes debugging support for the GINAgr
//
//  Effects:
//
//  Arguments:  (none)
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    4-29-93   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------


void
InitDebugSupport(void)
{
    LoadDebugParameters();

}



#else // DBG

#pragma warning(disable:4206)   // Disable the empty transation unit
                                // warning/error

#endif  // NOTE:  This file not compiled for retail builds

