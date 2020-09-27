/*
 *  Detection routines for modems.
 *
 *  Microsoft Confidential
 *  Copyright (c) Microsoft Corporation 1993-1994
 *  All rights reserved
 *
 */

#include "proj.h"

#include <stdarg.h>

#define CR '\r'        
#define LF '\n'        

#define RESPONSE_RCV_DELAY      5000    // A long time (5 secs) because 
                                        // once we have acquired the modem 
                                        // we can afford the wait.

#define MAX_QUERY_RESPONSE_LEN  100
#define MAX_SHORT_RESPONSE_LEN  30      // echo of ATE0Q0V1<cr> and 
                                        // <cr><lf>ERROR<cr><lf> by a 
                                        // little margin

#define ATI0_LEN                30      // amount of the ATI0 query that 
                                        // we will save

#define ATI0                    0       // we will use this result completely
#define ATI4                    4       // we will use this result completely, 
                                        // if it matches the Hayes format 
                                        // (check for 'a' at beginning)

// Return values for the FindModem function
//
#define RESPONSE_USER_CANCEL    (-4)    // user requested cancel
#define RESPONSE_UNRECOG        (-3)    // got some chars, but didn't 
                                        //  understand them
#define RESPONSE_NONE           (-2)    // didn't get any chars
#define RESPONSE_FAILURE        (-1)    // internal error or port error
#define RESPONSE_OK             0       // matched with index of <cr><lf>OK<cr><lf>
#define RESPONSE_ERROR          1       // matched with index of <cr><lf>ERROR<cr><lf>

#ifdef WIN32
typedef HANDLE  HPORT;          // variable type used in FindModem
#else
typedef int     HPORT;          // variable type used in FindModem
#endif

#define IN_QUEUE_SIZE           8192
#define OUT_QUEUE_SIZE          256

#define RCV_DELAY               2000
#define CHAR_DELAY              100

#define CBR_HACK_115200         0xff00  // This is how we set 115,200 on 
                                        //  Win 3.1 because of a stupid bug.



#pragma data_seg(DATASEG_READONLY)

TCHAR const FAR c_szPortPrefix[]    = TEXT("\\\\.\\%s");  // "\\.\" in ASCII

char const FAR c_szModemIdPrefix[] = "UNIMODEM";
char const FAR c_szNoEcho[] = "ATE0Q0V1\r";
char const FAR c_szReset[] = "ATZ\r";
char const FAR c_szATPrefix[] = "AT";
char const FAR c_szATSuffix[] = "\r";
char const FAR c_szBlindOnCheck[] = "X3";
char const FAR c_szBlindOnCheckAlternate[] = "X0";
char const FAR c_szBlindOffCheck[] = "X4";

// WARNING!  If you change these, you will have to change ALL of your
// CompatIDs!!!
char const FAR *c_aszQueries[] = { "ATI0\r", "ATI1\r", "ATI2\r",  "ATI3\r",
                                 "ATI4\r", "ATI5\r", "ATI6\r",  "ATI7\r",
                                 "ATI8\r", "ATI9\r", "ATI10\r", "AT%V\r" };

// these are mostly for #'s.  If a numeric is adjoining one of these, it 
// will not be treated as special.
// Warning: Change any of these and you have to redo all of the CRCs!!!!  
// Case insensitive compares
char const FAR *c_aszIncludes[] = { "300",
                                  "1200",
                                  "2400",                         "2,400",
                                  "9600",    "96",     "9.6",     "9,600",
                                  "12000",   "120",    "12.0",    "12,000",
                                  "14400",   "144",    "14.4",    "14,400",
                                  "16800",   "168",    "16.8",    "16,800",
                                  "19200",   "192",    "19.2",    "19,200",
                                  "21600",   "216",    "21.6",    "21,600",
                                  "24000",   "240",    "24.0",    "24,000",
                                  "26400",   "264",    "26.4",    "26,400",
                                  "28800",   "288",    "28.8",    "28,800",
                                  "31200",   "312",    "31.2",    "31,200",
                                  "33600",   "336",    "33.6",    "33,600",
                                  "36000",   "360",    "36.0",    "36,000",
                                  "38400",   "384",    "38.4",    "38,400",
                                  "9624",    "32bis",  "42bis",   "V32",
                                  "V.32",    "V.FC",   "FAST",    "FAX",
                                  "DATA",    "VOICE",  "" };

// Matches will be case-insensitive
char const FAR *c_aszExcludes[] = { "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                                    "JUL", "AUG", "SEP", "OCT", "NOV", "DEC", 
                                    "" };

// case sensitive matching
char const FAR *c_aszBails[] = { "CONNECT", "RING", "NO CARRIER", 
                                 "NO DIALTONE", "BUSY", "NO ANSWER", "=" };

// start after CBR_9600
UINT const FAR c_auiUpperBaudRates[] = { CBR_19200, CBR_38400, CBR_56000, 
                                         CBR_HACK_115200 }; 

char const FAR *c_aszResponses[] = { "\r\nOK\r\n", "\r\nERROR\r\n" };

// Some MultiTech's send 0<cr> in response to AT%V (they go 
// into numeric mode)
char const FAR *c_aszNumericResponses[] = { "0\r", "4\r" };  

char const FAR c_szHex[] = "0123456789abcdef";

struct DCE {
    char  pszStr[4];
    DWORD dwDce;
    DWORD dwAlternateDce;
} DCE_Table[] = {
    "384", 38400, 300,   // Some PDI's will report 38400, and this won't work for them.  Screw 'em.
    "360", 36000, 300,
    "336", 33600, 300,
    "312", 31200, 300,
    "288", 28800, 2400,
    "264", 26400, 2400,
    "240", 24000, 2400,
    "216", 21600, 2400,
    "192", 19200, 1200,
    "168", 16800, 1200,
    "14",  14400, 1200,
    "120", 12000, 1200,
    "9",   9600,  300,
    "2",   2400,  300,
    "1",   1200,  300,
    "3",   300,   0
};

#pragma data_seg()



// BUGBUG - WARNING: Not for DBCS usage - is not a real bugbug since modems aren't DBCS.
//#define isupper(ch) (((ch) >= 'A' && (ch) <= 'Z') ? TRUE : FALSE)
//#define islower(ch) (((ch) >= 'a' && (ch) <= 'z') ? TRUE : FALSE)
//#define isalpha(ch) ((toupper(ch) >= 'A' && toupper(ch) <= 'Z') ? TRUE : FALSE)
#define toupper(ch) (islower(ch) ? (ch) - 'a' + 'A' : (ch))
#define ishex(ch)   ((toupper(ch) >= 'A' && toupper(ch) <= 'F') ? TRUE : FALSE)
#define isnum(num)  ((num >= '0' && num <= '9') ? TRUE : FALSE)

#define MAX_TEST_TRIES 4


#define MAX_LOG_PRINTF_LEN 256
void _cdecl LogPrintf(HANDLE hLog, UINT uResourceFmt, ...);

DWORD NEAR PASCAL FindModem(PDETECTCALLBACK pdc, HPORT hPort);

#ifdef DEBUG
void HexDump( TCHAR *, LPBYTE lpBuf, DWORD cbLen);
#define	HEXDUMP(_a, _b, _c) HexDump(_a, _b, _c)
#else // !DEBUG
#define	HEXDUMP(_a, _b, _c) ((void) 0)
#endif

DWORD 
PRIVATE
IdentifyModem(
    IN  PDETECTCALLBACK pdc,
    IN  HPORT   hPort, 
    OUT LPTSTR  pszModemName, 
    IN  HANDLE  hLog, 
    OUT LPSTR   lpszATI0Result);

BOOL 
PRIVATE
TestBaudRate(
    IN  HPORT hPort, 
    IN  UINT uiBaudRate, 
    IN  DWORD dwRcvDelay, 
    IN  PDETECTCALLBACK pdc, 
    OUT BOOL FAR *lpfCancel);

DWORD 
NEAR PASCAL 
SetPortBaudRate(
    HPORT hPort, 
    UINT BaudRate);
int    
NEAR PASCAL 
ReadResponse(
    HPORT hPort, 
    LPBYTE lpvBuf, 
    UINT uRead, 
    BOOL fMulti, 
    DWORD dwRcvDelay, 
    PDETECTCALLBACK pdc);
UINT
NEAR PASCAL 
ReadPort(
    HPORT hPort, 
    LPBYTE lpvBuf, 
    UINT uRead, 
    DWORD dwRcvDelay,
    int FAR *lpiError, 
    PDETECTCALLBACK pdc, 
    BOOL FAR *lpfCancel);

DWORD 
PRIVATE
ConstructGenericInf(
    PDETECTCALLBACK pdc,
    HPORT hPort, 
    LPCSTR lpszATI0Result,
    HANDLE hLog,
    PMODEM_DETECT_SIG pmds);

DWORD NEAR PASCAL CBR_To_Decimal(UINT uiCBR);
LPSTR NEAR ConvertToPrintable(LPCSTR pszIn, LPSTR pszOut, UINT uOut);

// Does a printf to a log, using a resource string as the format.
// WARNING: Do not try to print large strings.
void _cdecl LogPrintf(HANDLE hLog, UINT uResourceFmt, ...)
{
    char pFmt[MAX_LOG_PRINTF_LEN];
    char pOutput[MAX_LOG_PRINTF_LEN];
    UINT uCount, uWritten;
    va_list vArgs;

    if (hLog != INVALID_HANDLE_VALUE)
    {
        if (LoadStringA(g_hinst, uResourceFmt, pFmt, MAX_LOG_PRINTF_LEN))
        {
            va_start(vArgs, uResourceFmt);
            uCount = wvsprintfA(pOutput, pFmt, vArgs);
            va_end(vArgs);

            WriteFile(hLog, (LPCVOID)pOutput, uCount, &uWritten, NULL);
        }
    }
}

int FAR PASCAL mylstrncmp(LPCSTR pchSrc, LPCSTR pchDest, int count)
{
    for ( ; count && *pchSrc == *pchDest; pchSrc++, pchDest++, count--) {
        if (*pchSrc == '\0')
            return 0;
    }
    return count;
}

int FAR PASCAL mylstrncmpi(LPCSTR pchSrc, LPCSTR pchDest, int count)
{
    for ( ; count && toupper(*pchSrc) == toupper(*pchDest); pchSrc++, pchDest++, count--) {
        if (*pchSrc == '\0')
            return 0;
    }
    return count;
}

#ifdef WIN32

DWORD 
PRIVATE
MyWriteComm(
    HANDLE hPort, 
    LPBYTE lpBuf, 
    DWORD cbLen)
{
    COMMTIMEOUTS cto;
    DWORD        cbLenRet;

    HEXDUMP	(TEXT("Write"), lpBuf, cbLen);
    // Set comm timeout
    if (!GetCommTimeouts(hPort, &cto))
    {
      ZeroMemory(&cto, sizeof(cto));
    };

    // Allow a constant write timeout
    cto.WriteTotalTimeoutMultiplier = 0;
    cto.WriteTotalTimeoutConstant   = 1000; // 1 second
    SetCommTimeouts(hPort, &cto);

    // Synchronous write
    WriteFile(hPort, lpBuf, cbLen, &cbLenRet, NULL);
    return cbLenRet;
}

#define MyFlushComm     PurgeComm
#define MyCloseComm     CloseHandle

#else   // WIN32

#define MyWriteComm     WriteComm
#define MyCloseComm     CloseComm

#ifndef PURGE_TXCLEAR
#define PURGE_TXCLEAR   0x00000001
#endif
#ifndef PURGE_RXCLEAR
#define PURGE_RXCLEAR   0x00000002
#endif

BOOL
PRIVATE
MyFlushComm(
    HANDLE  hport,
    DWORD   dwAction)
    {
    if (IsFlagSet(dwAction, PURGE_TXCLEAR))
        {
        FlushComm((int)hport, 0);
        }

    if (IsFlagSet(dwAction, PURGE_RXCLEAR))
        {
        FlushComm((int)hport, 1);
        }

    return TRUE;
    }

#endif  // WIN32


/*----------------------------------------------------------
Purpose: Open the modem detection log.

Returns: handle to the open file
         NULL if the file could not be opened
Cond:    --
*/
HANDLE
PUBLIC
OpenDetectionLog()
    {
    TCHAR szLogPath[MAX_PATH];
    UINT cch;
    HANDLE hLog;

    // open the log file
    cch = GetWindowsDirectory(szLogPath, SIZECHARS(szLogPath));
    if (0 == cch)
        {
        hLog = INVALID_HANDLE_VALUE;
        }
    else
        {                                                      
        if (*CharPrev(szLogPath, szLogPath + cch) != TEXT('\\'))
            {
            szLogPath[cch++] = (TCHAR)'\\';
            }
        LoadString(g_hinst, IDS_DET_LOG_NAME, &szLogPath[cch],
                   SIZECHARS(szLogPath) - (cch - 1));

        // error return will be HFILE_ERROR, so no need to check since 
        // we will handle that during writes
        TRACE_MSG(TF_DETECT, "Opening detection log file '%s'", (LPTSTR)szLogPath);

        hLog = CreateFile(szLogPath, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                          FILE_ATTRIBUTE_NORMAL, NULL);

        if (hLog == INVALID_HANDLE_VALUE)
            {
            TRACE_MSG(TF_ERROR, "Modem CreateFile() failed!");
            }
        }

    return hLog;
    }


/*----------------------------------------------------------
Purpose: Closes the detection log file.

Returns: --
Cond:    --
*/
void
PUBLIC
CloseDetectionLog(
    IN  HANDLE hLog)
    {
    if (INVALID_HANDLE_VALUE != hLog)
        {
        TRACE_MSG(TF_DETECT, "Closing detection log");
        CloseHandle(hLog);
        }
    }    


/*----------------------------------------------------------
Purpose: Set the current port we're updating in the progress
         bar.
Returns: --
Cond:    --
*/
void 
PRIVATE 
DetectSetPort(
    PDETECTCALLBACK pdc,
    LPCTSTR lpcszName)
    {
    if (pdc && pdc->pfnCallback)
        {
        try
            {
            pdc->pfnCallback(DSPM_SETPORT, (LPARAM)lpcszName, pdc->lParam);
            }
        except (EXCEPTION_EXECUTE_HANDLER)
            {
            ;
            }
        }
    }


/*----------------------------------------------------------
Purpose: Set the current msg we're updating in the Detect wizard page.
Returns: --
Cond:    --
*/
void 
PUBLIC 
DetectSetStatus(
    PDETECTCALLBACK pdc,
    DWORD           nStatus)
    {
    if (pdc && pdc->pfnCallback)
        {
        try
            {
            pdc->pfnCallback(DSPM_SETSTATUS, (LPARAM)nStatus, pdc->lParam);
            }
        except (EXCEPTION_EXECUTE_HANDLER)
            {
            ;
            }
        }
    }


/*----------------------------------------------------------
Purpose: Query's whether we are supposed to cancel the detection.  Also
         yields.
Returns: TRUE if we should cancel.  FALSE otherwise.
Cond:    --
*/
BOOL 
PRIVATE 
DetectQueryCancel(
    PDETECTCALLBACK pdc)
    {
    BOOL bRet = FALSE;

    if (pdc && pdc->pfnCallback)
        {
        try
            {
            bRet = pdc->pfnCallback(DSPM_QUERYCANCEL, 0, pdc->lParam);
            }
        except (EXCEPTION_EXECUTE_HANDLER)
            {
            bRet = FALSE;
            }
        }
    return bRet;
    }


/*----------------------------------------------------------
Purpose: This function queries the given port to find a legacy
         modem.

         If a modem is detected and we recognize it (meaning 
         we have the hardware ID in our INF files), or if we
         successfully create a generic hardware ID and 
         inf file, then this function also creates the phantom
         device instance of this modem.

         NOTE (scotth):  in Win95, this function only detected 
         the modem and returned the hardware ID and device 
         description.  For NT, this function also creates the 
         device instance.  I made this change because it is
         faster.

Returns: NO_ERROR
         ERROR_PORT_INACCESSIBLE
         ERROR_NO_MODEM
         ERROR_ACCESS_DENIED
         ERROR_CANCELLED

Cond:    --
*/
DWORD 
PUBLIC
DetectModemOnPort(
    IN  HDEVINFO            hdi,
    IN  PDETECTCALLBACK     pdc,
    IN  HANDLE              hLog, 
    IN  LPCTSTR             pszPort,
    OUT PMODEM_DETECT_SIG   pmds,
    OUT PSP_DEVINFO_DATA    pdevDataOut)
{
    DWORD dwRet;
    HPORT hPort;
    HCURSOR hCursor;
    DWORD cbLen;
    char szATI0Result[ATI0_LEN];
    char szASCIIPort[LINE_LEN];
	TCHAR szLocalHardwareID[MAX_MODEM_ID_LEN+1];

#if defined(WIN32)
    TCHAR szPrefixedPort[MAX_BUF + sizeof(c_szPortPrefix)];
#endif
    
    DBG_ENTER(DetectModemOnPort);

    ASSERT(pszPort);
    ASSERT(pmds);

	*szLocalHardwareID=0;

#ifdef  UNICODE
    // Convert the port name to ASCII
    WideCharToMultiByte(CP_ACP, 0, pszPort, -1, szASCIIPort, SIZECHARS(szASCIIPort),
                        NULL, NULL);
#else
    lstrcpyA(szASCIIPort, pszPort);
#endif  // UNICODE


#ifdef SKIP_MOUSE_PORT
    // Is this port used by a serial mouse?
    if (0 == lstrcmpi(g_szMouseComPort, pszPort))
        {
        // Yes; skip it
        TRACE_MSG(TF_ERROR, "Serial mouse on this port, skipping");
        dwRet = ERROR_NO_MODEM;
        goto Cleanup;
        }
#endif
    
    // Open the port

    DetectSetPort(pdc, pszPort);

#if !defined(WIN32)
    hPort = OpenComm(pszPort, IN_QUEUE_SIZE, OUT_QUEUE_SIZE);
#else
    wsprintf(szPrefixedPort, c_szPortPrefix, pszPort);
    hPort = CreateFile(szPrefixedPort, 
                       GENERIC_WRITE | GENERIC_READ,
                       0, NULL,
                       OPEN_EXISTING, 0, NULL);
#endif

    if (hPort == INVALID_HANDLE_VALUE) {
        dwRet = GetLastError();
        if (dwRet == ERROR_ACCESS_DENIED) {
            TRACE_MSG(TF_ERROR, "Port is in use by another app");
            LogPrintf(hLog, IDS_DET_INUSE, szASCIIPort);
        }
        else  {
            TRACE_MSG(TF_ERROR, "Couldn't open port");
            LogPrintf(hLog, IDS_DET_COULDNT_OPEN, szASCIIPort);
        }
    }
    else {
        SetupComm (hPort, IN_QUEUE_SIZE, OUT_QUEUE_SIZE);
    
        TRACE_MSG(TF_DETECT, "Opened Port");
        
        // Check for a modem on the port
        
        hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
        
        dwRet = FindModem(pdc, hPort);
        
        if (dwRet == NO_ERROR) 
        {
            // We have a modem.  No matter what, we must return 
            // NO_ERROR (unless the user cancels).
            LogPrintf(hLog, IDS_DET_FOUND, szASCIIPort);

            // Initialize the detection signature
            DetectSig_Init(pmds, 0, NULL, pszPort);

            // Could we identify the modem?
            dwRet = IdentifyModem(pdc, hPort, szLocalHardwareID, hLog, szATI0Result);
            if (NO_ERROR != dwRet)
                {
                if (ERROR_CANCELLED != dwRet)
                    {
                    // No; attempt to construct a generic INF
                    TRACE_MSG(TF_DETECT, "Couldn't identify modem due to some kind of error.  Will build one.");
                    goto DoConstructGenericInf;  // save a little code space
                    }
                }
            else
                {
                // Yes; we now have the hardware ID for this modem.  
				// Copy it into pmds, truncating it to 16characters (including
				// null)
				lstrcpyn(
					pmds->szHardwareID,
					szLocalHardwareID,
					sizeof(pmds->szHardwareID)/sizeof(TCHAR)
					);
                SetFlag(pmds->dwMask, MDSM_HARDWAREID);
                
                DetectSetStatus(pdc, DSS_CHECK_FOR_COMPATIBLE);

                // Is there a device that is compatible with this
                // hardware ID?  If there is, this function will also
                // create a phantom device instance with a working
                // set of compatible drivers.
                if (CplDiCreateCompatibleDeviceInfo(hdi,
                                                 szLocalHardwareID, 
                                                 NULL,
                                                 pmds->szDeviceDesc,
                                                 SIZECHARS(pmds->szDeviceDesc),
                                                 pdevDataOut))
                    {
                    // Yes; a device instance was created!
                    SetFlag(pmds->dwMask, MDSM_DEVICEDESC);
                    }
                else
                    {
                    // Doh!  No matching inf for this compat id.  Must create a generic one...
                    TRACE_MSG(TF_DETECT, "No compatible infs found.  Will build one.");                

DoConstructGenericInf:
                    dwRet = ConstructGenericInf(pdc, hPort, szATI0Result, hLog, pmds);

                    if (NO_ERROR == dwRet)
                        {
                        LoadString(g_hinst, IDS_GENERIC_DESC, pmds->szDeviceDesc, SIZECHARS(pmds->szDeviceDesc));
                        SetFlag(pmds->dwMask, MDSM_DEVICEDESC);
                        
                        DetectSetStatus(pdc, DSS_CHECK_FOR_COMPATIBLE);

                        // Try creating a device that is compatible with
                        // the generic modem
                        if ( !CplDiCreateCompatibleDeviceInfo(hdi, 
                                                              pmds->szHardwareID,
                                                              pmds->szDeviceDesc,
                                                              NULL, 0,
                                                              pdevDataOut) )
                            {
                            // This still failed.  Give up.
                            dwRet = GetLastError();
                            ASSERT(NO_ERROR != dwRet);
                            // BUG BUG: we should do this or set this to some
                            // other error (and clean up the detected modem instance!)
                            //dwRet = ERROR_CANCELLED;
                            }
                        }
                    }

                if (DetectQueryCancel(pdc))
                    {
                    TRACE_MSG(TF_DETECT, "User pressed cancel.");
                    dwRet = ERROR_CANCELLED;
                    }
                }

            // Reset
            cbLen = lstrlenA(c_szReset);
            if (MyWriteComm(hPort, (LPBYTE)c_szReset, cbLen) == cbLen &&
                ERROR_CANCELLED != dwRet)
            {
                // Now read the result of the write and ignore it
                if (RESPONSE_OK != ReadResponse(hPort, NULL, 
                                                MAX_SHORT_RESPONSE_LEN, 
                                                FALSE, 0, pdc)) 
                {
                    TRACE_MSG(TF_ERROR, "Reset result failed");
                }
            }
            else 
            {
                TRACE_MSG(TF_ERROR, "Couldn't write Reset string");
            }
        }
        else
        {
            if (ERROR_CANCELLED != dwRet)
            {
                LogPrintf(hLog, IDS_DET_NOT_FOUND, szASCIIPort);
            }
        }

        SetCursor(hCursor);
        
        // Flush before closing becuase if there are characters stuck in the queue,
        // serial.386 will take 30 seconds to time out.
        
        MyFlushComm(hPort, PURGE_RXCLEAR | PURGE_TXCLEAR);
        EscapeCommFunction(hPort, CLRDTR);
        MyCloseComm(hPort);

    }  // hPort < 0

#ifdef SKIP_MOUSE_PORT
Cleanup:
#endif
    DBG_EXIT_DWORD(DetectModemOnPort, dwRet);
    return dwRet;
}

#define HAYES_COMMAND_LEN 40

// Try sending the requested command and return whether it works or not
// Try MAX_TEST_TRIES
// Returns: TRUE on SUCCESS
//          FALSE on failure (including user cancels)
BOOL 
NEAR PASCAL 
TestCommand(
    HPORT hPort, 
    PDETECTCALLBACK pdc, 
    BOOL FAR *lpfCancel, 
    LPCSTR szCommand)
{
    DWORD cbLen;
    int iTries = MAX_TEST_TRIES;
    char szFullCommand[HAYES_COMMAND_LEN + 1];

    lstrcpyA(szFullCommand, c_szATPrefix);
    lstrcatA(szFullCommand, szCommand);
    lstrcatA(szFullCommand, c_szATSuffix);
    cbLen = lstrlenA(szFullCommand);

    TRACE_MSG(TF_DETECT, "Testing '%s'", (LPTSTR)szFullCommand);

    *lpfCancel = FALSE;

    while (iTries--)
    {

        // clear the read queue, there shouldn't be anything there
        MyFlushComm(hPort, PURGE_RXCLEAR);  

        if (MyWriteComm(hPort, szFullCommand, cbLen) == cbLen) 
        {
            switch(ReadResponse(hPort, NULL, MAX_SHORT_RESPONSE_LEN, FALSE, RESPONSE_RCV_DELAY, pdc))
            {
            case RESPONSE_OK:
                TRACE_MSG(TF_DETECT, "Command returned OK!");
                return TRUE;

            case RESPONSE_ERROR:
                TRACE_MSG(TF_DETECT, "Command returned ERROR!");
                return FALSE;

            case RESPONSE_USER_CANCEL:
                *lpfCancel = TRUE;
                return FALSE;
            }
        }                                                                
    }
    return FALSE;
}

// Checks various AT commands to see if they work.  Updates the modem
// detection signature (*pms) as necessary.
// Returns FALSE on error or user cancel
BOOL 
PRIVATE
CheckCommands(
    IN  HPORT hPort, 
    IN  PDETECTCALLBACK pdc,
    OUT BOOL FAR *lpfCancel, 
    OUT PMODEM_DETECT_SIG pmds)
{
    // assume failures
    pmds->szBlindOn[0] = 0;
    pmds->szBlindOff[0] = 0;

    //
    // Blind On/Off - check blind off last so that the modem is in a state we prefer.
    //

    // test X3
    if (TestCommand(hPort, pdc, lpfCancel, c_szBlindOnCheck))
    {
        ASSERT(SIZECHARS(pmds->szBlindOn) > lstrlenA(c_szBlindOnCheck)); // '>' accounts for null
        lstrcpynA(pmds->szBlindOn, c_szBlindOnCheck, SIZECHARS(pmds->szBlindOn));
    }
    else
    {
        if (*lpfCancel)
        {
            return FALSE;
        }

        // X3 failed so try X0
        if (TestCommand(hPort, pdc, lpfCancel, c_szBlindOnCheckAlternate))
        {
            ASSERT(SIZECHARS(pmds->szBlindOn) > lstrlenA(c_szBlindOnCheckAlternate)); // '>' accounts for null
            lstrcpynA(pmds->szBlindOn, c_szBlindOnCheckAlternate, SIZECHARS(pmds->szBlindOn));
        }
        else
        {
            if (*lpfCancel)
            {
                return FALSE;
            }
        }
    }

    // test X4
    if (TestCommand(hPort, pdc, lpfCancel, c_szBlindOffCheck))
    {
        ASSERT(SIZECHARS(pmds->szBlindOff) > lstrlenA(c_szBlindOffCheck)); // '>' accounts for null
        lstrcpynA(pmds->szBlindOff, c_szBlindOffCheck, SIZECHARS(pmds->szBlindOff));
    }
    else
    {
        if (*lpfCancel)
        {
            return FALSE;
        }
    }

    return TRUE;
}

// Returns the max dte for this modem in numeric form (ie. 57600, NOT CBR_56000)
// Returns 0 on error or user cancel
DWORD 
PRIVATE
FindMaxDTE(
    HPORT hPort, 
    PDETECTCALLBACK pdc,
    BOOL FAR *lpfCancel)
{
    UINT uiWorkingBaudRate, uiMaxBaudRate;
    int  iBaudRateIndex;
    DCB DCB;

    *lpfCancel = FALSE;

    // Get current baud rate
    if (GetCommState(hPort, &DCB) < 0) 
    {
        TRACE_MSG(TF_ERROR, "GetCommState failed");
        return 0;
    }
    uiWorkingBaudRate = uiMaxBaudRate = DCB.BaudRate;

    // Should we try even faster (are we at 9600 now?)?
    if (uiWorkingBaudRate == CBR_9600)
    {
        for (iBaudRateIndex = 0; iBaudRateIndex < ARRAYSIZE(c_auiUpperBaudRates); iBaudRateIndex++)
        {
            // if baud rate works then break out and use it!
            if (TestBaudRate(hPort, c_auiUpperBaudRates[iBaudRateIndex], 0, pdc, lpfCancel))
            {
                uiMaxBaudRate = c_auiUpperBaudRates[iBaudRateIndex];
            }
            else
            {
                if (*lpfCancel)
                {
                    return 0;
                }
                break;                
            }
        }            

        // we now have our maximum baud rate and a good baud rate for interacting with the modem
        // go back to our safe baud rate (safe w/respect to computer overruns)
        if (!TestBaudRate(hPort, uiWorkingBaudRate, 0, pdc, lpfCancel))
        {
            if (*lpfCancel)
            {
                return 0;
            }
            TRACE_MSG(TF_ERROR, "return to safe baudrate failed!");
        }

    }
    return CBR_To_Decimal(uiMaxBaudRate);
}

// Returns the max dce for this modem in numeric form (ie. 9600, NOT CBR_9600)
// Returns 0 on error
DWORD NEAR PASCAL FindMaxDCE(HPORT hPort, LPCSTR lpszATI0Result, DWORD dwMaxDTE)
{
    int i;
    DWORD dwMaxDCE = 0;
    char FAR *lpszPtr = (char FAR *)lpszATI0Result;

    // First we try to deduce our max dce by examining the ATI0 result
    while (*lpszPtr) // skip past <cr> and <lf> and any other non numeric chars
    {
        if (isnum(*lpszPtr))
        {
            break;
        }
        lpszPtr++;
    }

    // did we hit the null-terminator?
    if (*lpszPtr) 
    {
        // nope! let's rock and roll, uh I mean, do some comparisons...
        TRACE_MSG(TF_DETECT, "Analyzing ATI0 result '%s'", (LPTSTR)lpszPtr);
        
        for (i = 0; i < ARRAYSIZE(DCE_Table); i++)
        {
            // do we have a match?
            if (!mylstrncmp(lpszPtr, DCE_Table[i].pszStr, lstrlenA(DCE_Table[i].pszStr)))
            {
                // yes.  is there an alternate that is more feasible?
                // since DTE shouldn't be lower than DCE, let's check that...
                if (DCE_Table[i].dwAlternateDce && dwMaxDTE < DCE_Table[i].dwDce)
                {
                    dwMaxDCE = DCE_Table[i].dwAlternateDce;
                    TRACE_MSG(TF_DETECT, "Using alternate dce of %ld", dwMaxDCE);
                }
                else
                {
                    dwMaxDCE = DCE_Table[i].dwDce;
                    TRACE_MSG(TF_DETECT, "A.I. dce of %ld", dwMaxDCE);
                }
                break;
            }
        }
    }
        
    // If that fails, make a guess based on the MaxDTE.
    if (!dwMaxDCE)
    {
        TRACE_MSG(TF_DETECT, "Guessing at DCE speed");
        switch (dwMaxDTE)
        {
        case 300:
            dwMaxDCE = 300;
            break;        
        case 1200:
            dwMaxDCE = 1200;
            break;        
        case 2400:
            dwMaxDCE = 2400;
            break;        
        case 9600:
        case 19200:
        case 38400:
            dwMaxDCE = 9600;
            break;        
        case 57600:
            dwMaxDCE = 14400;
            break;        
        case 115200:
            dwMaxDCE = 28800;
            break;        
        default:
            ASSERT(0);
            TRACE_MSG(TF_WARNING, "Hit a default in FindMaxDCE");
            break;        
        }
    }
    
    // Check the logic of the DTE and DCE.  Verify that it makes sense.
    if (dwMaxDCE > dwMaxDTE)
    {
        dwMaxDCE = dwMaxDTE;
        TRACE_MSG(TF_DETECT, "Auto-correcting Max DCE to be %ld bps", dwMaxDCE);
    }

    ASSERT(dwMaxDCE);
    return dwMaxDCE;
}

// When we get there, the modem is open and in the ATE0Q0V1 state.
// Even if we can't talk to the modem, we have to return a generic inf.
// We are only called when a modem is seen on a port.  (it may be gone now)
// Return NO_ERROR unles there was a user cancel (ERROR_CANCELLED)
DWORD 
PRIVATE
ConstructGenericInf(
    IN  PDETECTCALLBACK pdc,
    IN  HPORT hPort, 
    IN  LPCSTR lpszATI0Result,
    IN  HANDLE hLog,
    OUT PMODEM_DETECT_SIG pmds)
{
    DWORD dwMaxDTE;
    DWORD dwMaxDCE;
    BOOL  fCancel;

    ASSERT(pmds);

    // our fallback
    lstrcpy(pmds->szHardwareID, TEXT("MDMGEN"));

    // Non-dte changing stuff first.
    if (!CheckCommands(hPort, pdc, &fCancel, pmds))
    {
        if (fCancel)
        {
            return ERROR_CANCELLED;
        }
        else
        {
            TRACE_MSG(TF_ERROR, "CheckCommands failed");
            return NO_ERROR;
        }
    }

    if (!(dwMaxDTE = FindMaxDTE(hPort, pdc, &fCancel)))
    {
        if (fCancel)
        {
            return ERROR_CANCELLED;
        }
        else
        {
            TRACE_MSG(TF_ERROR, "FindMaxDTE failed");
            return NO_ERROR;
        }
    }
    TRACE_MSG(TF_DETECT, "Max DTE = %ld bps", dwMaxDTE);
    LogPrintf(hLog, IDS_DET_DTE, dwMaxDTE);

    if (!(dwMaxDCE = FindMaxDCE(hPort, lpszATI0Result, dwMaxDTE)))
    {
        TRACE_MSG(TF_ERROR, "FindMaxDCE failed");
        return NO_ERROR;
    }
    TRACE_MSG(TF_DETECT, "Hypothesized Max DCE = %ld bps", dwMaxDCE);

    SetFlag(pmds->dwFlags, MDSF_UPDATE_DEVCAPS);
    pmds->dwMaxDTE = dwMaxDTE;
    pmds->dwMaxDCE = dwMaxDCE;

    return NO_ERROR;
}


// Switch to requested baud rate and try sending ATE0Q0V1 and return whether it works or not
// Try MAX_TEST_TRIES
// Returns: TRUE on SUCCESS
//          FALSE on failure (including user cancels)
BOOL 
PRIVATE
TestBaudRate(
    IN  HPORT hPort, 
    IN  UINT uiBaudRate, 
    IN  DWORD dwRcvDelay, 
    IN  PDETECTCALLBACK pdc,
    OUT BOOL FAR *lpfCancel)
{
    DWORD cbLen;
    int   iTries = MAX_TEST_TRIES;

    DBG_ENTER(TestBaudRate);
    
    *lpfCancel = FALSE;

    while (iTries--)
    {
        // try new baud rate
        if (SetPortBaudRate(hPort, uiBaudRate) == NO_ERROR) 
        {
            cbLen = lstrlenA(c_szNoEcho); // Send an ATE0Q0V1<cr>

            // clear the read queue, there shouldn't be anything there
            PurgeComm(hPort, PURGE_RXCLEAR);
            if (MyWriteComm(hPort, (LPBYTE)c_szNoEcho, cbLen) == cbLen) 
            {
                switch(ReadResponse(hPort, NULL, MAX_SHORT_RESPONSE_LEN, FALSE, dwRcvDelay, pdc))
                {
                case RESPONSE_OK:
		    DBG_EXIT(TestBaudRate);
                    return TRUE;

                case RESPONSE_USER_CANCEL:
                    *lpfCancel = TRUE;
		    DBG_EXIT(TestBaudRate);
                    return FALSE;
                }
            }                                                                
        }
    }
    DBG_EXIT(TestBaudRate);
    return FALSE;
}


// Tries to figure out if there is a modem on the port.  If there is, it
// will try to find a good speed to talk to it at (300,1200,2400,9600).
// Modem will be set to echo off, result codes on, and verbose result codes. (E0Q0V1)
DWORD 
PRIVATE
FindModem(
    PDETECTCALLBACK pdc,
    HPORT hPort)
{
    UINT uGoodBaudRate;
    BOOL fCancel = FALSE;

    DBG_ENTER(FindModem);
    
    Sleep(500); // Wait, give time for modem to spew junk if any.

    DetectSetStatus(pdc, DSS_LOOKING);

    if (TestBaudRate(hPort, CBR_9600, 500, pdc, &fCancel))
    {
        uGoodBaudRate = CBR_9600;
    }
    else
    {
        if (!fCancel && TestBaudRate(hPort, CBR_2400, 500, pdc, &fCancel))
        {
            uGoodBaudRate = CBR_2400;
        }
        else
        {
            if (!fCancel && TestBaudRate(hPort, CBR_1200, 500, pdc, &fCancel))
            {
                uGoodBaudRate = CBR_1200;
            }
            else
            {
                // Hayes Accura 288 needs this much at 300bps
                if (!fCancel && TestBaudRate(hPort, CBR_300, 1000, pdc, &fCancel))  
                {
                    uGoodBaudRate = CBR_300;
                }
                else
                {
                    uGoodBaudRate = 0;
                }
            }
        }
    }

    if (fCancel)
    {
        return ERROR_CANCELLED;
    }

    if (uGoodBaudRate)
    {
        DetectSetStatus(pdc, DSS_FOUND_MODEM);
        DBG_EXIT(FindModem);
        return NO_ERROR;
    }
    else
    {
        DetectSetStatus(pdc, DSS_FOUND_NO_MODEM);
        DBG_EXIT(FindModem);
        return ERROR_NO_MODEM;
    }
}

DWORD NEAR PASCAL SetPortBaudRate(HPORT hPort, UINT BaudRate)
{
    DCB DCB;

    DBG_ENTER_UL(SetPortBaudRate, CBR_To_Decimal(BaudRate));

    // Get a Device Control Block with current port values

    if (GetCommState(hPort, &DCB) < 0) {
        TRACE_MSG(TF_ERROR, "GetCommState failed");
        DBG_EXIT(SetPortBaudRate);
        return ERROR_PORT_INACCESSIBLE;
    }

    DCB.BaudRate = BaudRate;
    DCB.ByteSize = 8;
    DCB.Parity = 0;
    DCB.StopBits = 0;
    DCB.fBinary = 1;
    DCB.fParity = 0;
// BUGBUG we should probably set flow control to off here.

    if (SetCommState(hPort, &DCB) < 0) {
        TRACE_MSG(TF_ERROR, "SetCommState failed");
        DBG_EXIT(SetPortBaudRate);
        return ERROR_PORT_INACCESSIBLE;
    }
    TRACE_MSG(TF_DETECT, "SetBaud rate to %lu", BaudRate);

    DBG_EXIT(SetPortBaudRate);
    return NO_ERROR;
}

#define MAX_RESPONSE_BURST_SIZE 8192
#define MAX_NUM_RESPONSE_READ_TRIES 30 // digicom scout needs this much + some safety
#define MAX_NUM_MULTI_TRIES 3   // Maximum number of 'q's to be sent when we aren't getting any response

// Read in response.  Handle multi-pagers.  Return a null-terminated string.
// Also returns response code.
// If lpvBuf == NULL
//      cbRead indicates the max amount to read.  Bail if more than this.
// Else
//      cbRead indicates the size of lpvBuf
// This can not be a state driven (ie. char by char) read because we
// must look for responses from the end of a sequence of chars backwards.
// This is because "ATI2" on some modems will return 
// "<cr><lf>OK<cr><lf><cr><lf>OK<cr><lf>" and we only want to pay attention
// to the final OK.  Yee haw!
// Returns:  RESPONSE_xxx
int 
PRIVATE
ReadResponse(
    HPORT hPort, 
    LPBYTE lpvBuf, 
    UINT cbRead, 
    BOOL fMulti, 
    DWORD dwRcvDelay, 
    PDETECTCALLBACK pdc)
{
    int  iRet = RESPONSE_UNRECOG;
    LPBYTE pszBuffer;
    BOOL fDoCopy = TRUE;
    UINT uBufferLen, uResponseLen;
    UINT uReadTries = MAX_NUM_RESPONSE_READ_TRIES;
    UINT i;
    UINT uOutgoingBufferCount = 0;
    UINT uAllocSize = lpvBuf ? MAX_RESPONSE_BURST_SIZE : cbRead;
    UINT uTotalReads = 0;
    UINT uNumMultiTriesLeft = MAX_NUM_MULTI_TRIES;
    int  iError;
    BOOL fCancel;
    BOOL fHadACommError = FALSE;

    ASSERT(cbRead);

    // do we need to adjust cbRead?
    if (lpvBuf)
    {
        cbRead--;  // preserve room for terminator
    }

    // Allocate buffer
    if (!(pszBuffer = (LPBYTE)LocalAlloc(LMEM_FIXED, uAllocSize)))
    {
        TRACE_MSG(TF_ERROR, "couldn't allocate memory.\n");
        return RESPONSE_FAILURE;
    }

    while (uReadTries--)
    {
        // Read response into buffer
        uBufferLen = ReadPort(hPort, pszBuffer, uAllocSize, dwRcvDelay, &iError, pdc, &fCancel);

        // Did the user request a cancel?
        if (fCancel)
        {
            iRet = RESPONSE_USER_CANCEL;
            goto Exit;
        }

        // any errors?
        if (iError)
        {
            // BUGBUG - Were screwed if we get an error during a multi-pager.
            fHadACommError = TRUE;
#ifdef DEBUG
            if (iError & CE_RXOVER)   TRACE_MSG(TF_DETECT, "CE_RXOVER");
            if (iError & CE_OVERRUN)  TRACE_MSG(TF_DETECT, "CE_OVERRUN");
            if (iError & CE_RXPARITY) TRACE_MSG(TF_DETECT, "CE_RXPARITY");
            if (iError & CE_FRAME)    TRACE_MSG(TF_DETECT, "CE_FRAME");
            if (iError & CE_BREAK)    TRACE_MSG(TF_DETECT, "CE_BREAK");
            //if (iError & CE_CTSTO)    TRACE_MSG(TF_DETECT, "CE_CTSTO");
            //if (iError & CE_DSRTO)    TRACE_MSG(TF_DETECT, "CE_DSRTO");
            //if (iError & CE_RLSDTO)   TRACE_MSG(TF_DETECT, "CE_RLSDTO");
            if (iError & CE_TXFULL)   TRACE_MSG(TF_DETECT, "CE_TXFULL");
            if (iError & CE_PTO)      TRACE_MSG(TF_DETECT, "CE_PTO");
            if (iError & CE_IOE)      TRACE_MSG(TF_DETECT, "CE_IOE");
            if (iError & CE_DNS)      TRACE_MSG(TF_DETECT, "CE_DNS");
            if (iError & CE_OOP)      TRACE_MSG(TF_DETECT, "CE_OOP");
            if (iError & CE_MODE)     TRACE_MSG(TF_DETECT, "CE_MODE");
#endif // DEBUG
        }

        // Did we not get any chars?
        if (uBufferLen)
        {
            uNumMultiTriesLeft = MAX_NUM_MULTI_TRIES; // reset num multi tries left, since we got some data
            uTotalReads += uBufferLen;
            HEXDUMP(TEXT("Read"), pszBuffer, uBufferLen);
            if (lpvBuf)
            {
                // fill outgoing buffer if there is room
                for (i = 0; i < uBufferLen; i++)
                {
                    if (uOutgoingBufferCount < cbRead)
                    {
                        lpvBuf[uOutgoingBufferCount++] = pszBuffer[i];
                    }
                    else
                    {
                        break;
                    }
                }
                // null terminate what we have so far
                lpvBuf[uOutgoingBufferCount] = 0;
            }
            else
            {
                if (uTotalReads >= cbRead)
                {
                    TRACE_MSG(TF_WARNING, "Bailing ReadResponse because we exceeded our maximum read allotment.");
                    goto Exit;
                }
            }

            // try to find a matching response (crude but quick)
            for (i = 0; i < ARRAYSIZE(c_aszResponses); i++)
            {
                // Verbose responses
                uResponseLen = lstrlenA(c_aszResponses[i]);

                // enough read to match this response?
                if (uBufferLen >= uResponseLen)
                {
                    if (!mylstrncmp(c_aszResponses[i], pszBuffer + uBufferLen - uResponseLen, uResponseLen))
                    {
                        iRet = i;
                        goto Exit;
                    }
                }

                // Numeric responses, for cases like when a MultiTech interprets AT%V to mean "go into numeric response mode"
                uResponseLen = lstrlenA(c_aszNumericResponses[i]);

                // enough read to match this response?
                if (uBufferLen >= uResponseLen)
                {
                    if (!mylstrncmp(c_aszNumericResponses[i], pszBuffer + uBufferLen - uResponseLen, uResponseLen))
                    {
                        DCB DCB;

                        TRACE_MSG(TF_WARNING, "went into numeric response mode inadvertantly.  Setting back to verbose.");

                        // Get current baud rate
                        if (GetCommState(hPort, &DCB) == 0) 
                        {
                            // Put modem back into Verbose response mode
                            if (!TestBaudRate(hPort, DCB.BaudRate, 0, pdc, &fCancel))
                            {
                                if (fCancel)
                                {
                                    iRet = RESPONSE_USER_CANCEL;
                                    goto Exit;
                                }
                                else
                                {
                                    TRACE_MSG(TF_ERROR, "couldn't recover contact with the modem.");
                                    // don't return error on failure, we have good info
                                }
                            }
                        }
                        else
                        {
                            TRACE_MSG(TF_ERROR, "GetCommState failed");
                            // don't return error on failure, we have good info
                        }

                        iRet = i;
                        goto Exit;
                    }
                }
            }
        }
        else
        {
            // have we received any chars at all (ie. from this or any previous reads)?
            if (uTotalReads)
            {
                if (fMulti && uNumMultiTriesLeft)
                {   // no match found, so assume it is a multi-pager, send a 'q'
                    // 'q' will catch those pagers that will think 'q' means quit.
                    // else, we will work with the pages that just need any ole' char.  
                    uNumMultiTriesLeft--;
                    TRACE_MSG(TF_DETECT, "sending a 'q' because of a multi-pager.");
                    if (MyWriteComm(hPort, "q", 1) != 1)
                    {
                        TRACE_MSG(TF_ERROR, "WriteComm failed");
                        iRet = RESPONSE_FAILURE;
                        goto Exit;
                    }
                    continue;
                }
                else
                {   // we got a response, but we didn't recognize it
                    ASSERT(iRet == RESPONSE_UNRECOG);   // check initial setting
                    goto Exit;
                }
            }
            else
            {   // we didn't get any kind of response
                iRet = RESPONSE_NONE;
                goto Exit;
            }
        }
    } // while

Exit:
    // Free local buffer
    LocalFree((HLOCAL)pszBuffer);
    if (fHadACommError && RESPONSE_USER_CANCEL != iRet)
    {
        iRet = RESPONSE_FAILURE;
    }
    return iRet;
}


// WARNING - DO NOT CHANGE THIS FUNCTION!!!!!!  YOU WILL HAVE TO DO A LOT OF WORK IF YOU DO!!!
// WARNING - DO NOT CHANGE THIS FUNCTION!!!!!!  YOU WILL HAVE TO DO A LOT OF WORK IF YOU DO!!!
// WARNING - DO NOT CHANGE THIS FUNCTION!!!!!!  YOU WILL HAVE TO DO A LOT OF WORK IF YOU DO!!!
// WARNING - DO NOT CHANGE THIS FUNCTION!!!!!!  YOU WILL HAVE TO DO A LOT OF WORK IF YOU DO!!!
// WARNING - DO NOT CHANGE THIS FUNCTION!!!!!!  YOU WILL HAVE TO DO A LOT OF WORK IF YOU DO!!!
// WARNING - DO NOT CHANGE THIS FUNCTION!!!!!!  YOU WILL HAVE TO DO A LOT OF WORK IF YOU DO!!!
// You will have to change all of the inf files if you change the CRC results.
//
// Traverse lpszIn and copy "pure" chars to lpszOut.
// Remove any "impurities" such as:
//   - "bails" - find one of these and cancel the rest of the line
//   - numerics/hexadecimal on any line but ATI0 and possibly ATI4, and
//     not including the "includes".  Includes are only used if they
//     aren't adjoining another #.
//
void NEAR CleanseResponse(int iQueryNumber, LPSTR lpszIn, LPSTR lpszOut)
{
    LPSTR lpszSrc = lpszIn;
    LPSTR lpszDest = lpszOut;
    LPSTR FAR *lppsz;
    BOOL fBail = FALSE;
    BOOL fInclude = FALSE;
    BOOL fExclude = FALSE;
    BOOL fInBody = FALSE;
    BOOL fCopyAll;
    int j, iLen;

    // Is this query exempt?
    fCopyAll = (iQueryNumber == ATI0) ? TRUE : FALSE;

    while (*lpszSrc)
    {
        // use any CRs or LFs we get before non-CRs/no-LFs.
        if (*lpszSrc == CR || *lpszSrc == LF)
        {
            if (fInBody)
            {
                break;
            }
            else
            {
                *lpszDest++ = *lpszSrc++;
                continue;
            }
        }

        // is this the first char of the body?
        if (!fInBody)
        {
            fInBody = TRUE; // indicate that the next CR or LF means termination.
            if (iQueryNumber == ATI4 && *lpszSrc == 'a')  // Hayes format capabilities string
            {
                fCopyAll = TRUE;
                *lpszDest++ = *lpszSrc++;
                continue;
            }
        }

        if (fCopyAll) // are we jammin?  (happens for ATI0 and ATI4 when first char is 'a')
        {
            // Only do a verbatim copy of the first word of the ATI0 response.
            if (iQueryNumber == ATI0 && *lpszSrc == ' ')
            {
                fCopyAll = FALSE;
            }
            else
            {
                *lpszDest++ = *lpszSrc++;
            }
            continue;
        }

        // Do Bails
        for (j = 0; j < ARRAYSIZE(c_aszBails); j++)
        {
            if (!mylstrncmp(lpszSrc, c_aszBails[j], lstrlenA(c_aszBails[j])))
            {
                fBail = TRUE;
                break;
            }
        }        
        if (fBail)  // should we bail?
        {
            TRACE_MSG(TF_DETECT, "early bail due to Bail '%s'", (LPTSTR)c_aszBails[j]);
            break;
        }

        // Do Includes
        lppsz = (LPSTR FAR *)c_aszIncludes;
        while (**lppsz)
        {
            iLen = lstrlenA(*lppsz);
            if (!mylstrncmpi(lpszSrc, *lppsz, iLen))
            {
                // check before and after to make sure they aren't numbers.
                // catches  33489600394, 9600 won't be exempted from certain death in this case
                if (!isnum(lpszSrc[-1]) && !isnum(lpszSrc[iLen]))
                {
                    fInclude = TRUE;
                    break;
                }
                else
                {
                    TRACE_MSG(TF_DETECT, "skipped an include because it was adjoined by numbers.");
                }
            }
            lppsz++;
        }             
        if (fInclude) // should we do the include?
        {
            fInclude = FALSE;
            TRACE_MSG(TF_DETECT, "include ('%s' len = %d)", (LPTSTR)*lppsz, iLen);
            CopyMemory(lpszDest, lpszSrc, (DWORD) iLen);
            lpszSrc += iLen;
            lpszDest += iLen;
            continue;
        }   

        // Do Excludes
        lppsz = (LPSTR FAR *)c_aszExcludes;
        while (**lppsz)
        {
            iLen = lstrlenA(*lppsz);
            if (!mylstrncmpi(lpszSrc, *lppsz, iLen))
            {
                fExclude = TRUE;
                break;
            }
            lppsz++;
        }             
        if (fExclude) // should we do the exclude?
        {
            fExclude = FALSE;
            TRACE_MSG(TF_DETECT, "exclude ('%s' len = %d)", (LPTSTR)*lppsz, iLen);
            lpszSrc += iLen;
            continue;
        }   

        // Remove numbers
        if (isnum(*lpszSrc))
        {
            lpszSrc++;
            continue;
        }

        // Remove hex digits (keep only if adjoining 1 or 2 non-hex letters)
        if (ishex(*lpszSrc))
        {
            // we know there is a char or null ahead of us...
            if ((lpszSrc[1] >= 'g' && lpszSrc[1] <= 'z') ||
                (lpszSrc[1] >= 'G' && lpszSrc[1] <= 'Z'))
            {
                *lpszDest++ = *lpszSrc++;
                continue;
            }

            // is there a char before us?
            if (lpszSrc > lpszIn)
            {
                if ((lpszSrc[-1] >= 'g' && lpszSrc[-1] <= 'z') ||
                    (lpszSrc[-1] >= 'G' && lpszSrc[-1] <= 'Z'))
                {
                    *lpszDest++ = *lpszSrc++;
                    continue;
                }
            }

            // we get here if we don't want to copy the hex digit
            lpszSrc++;
            continue;
        }
        
        // Remove lone letters (ex. 4M4 - reject, 4MM - accept)
        if (isalpha(*lpszSrc))
        {
            if (!isalpha(lpszSrc[-1]) && !isalpha(lpszSrc[1]))
            {
                lpszSrc++;
                continue;
            }
        }

        // Remove certain punctuation: periods, commas, and spaces
        // Will protect against things like "1992, 1993." -> "1992, 1993, 1994"
        if (*lpszSrc == '.' || *lpszSrc == ',' || *lpszSrc == ' ')
        {
            lpszSrc++;
            continue;
        }

        // whatever's left is okay to copy
        *lpszDest++ = *lpszSrc++;
     }

    *lpszSrc = 0;  // for log comparison sake
    *lpszDest = 0;
}

#define MAX_RESPONSE_FAILURES 5

// When we get here, we have found a modem on the hPort.  Our job is 
// to interogate the modem and return a hardware ID.

// returns:
//  NO_ERROR and a PnP id in pszModemName
//  ERROR_PORT_INACCESSIBLE
//  result of ATI0 query in lpszATI0Result
DWORD 
PRIVATE
IdentifyModem(
    IN  PDETECTCALLBACK pdc,
    IN  HPORT   hPort, 
    OUT LPTSTR  pszModemName, 
    IN  HANDLE  hLog, 
    OUT LPSTR   lpszATI0Result)
{
    DWORD cbLen;
    char  pszReadBuf[MAX_QUERY_RESPONSE_LEN];
    char  pszCRCBuf[MAX_QUERY_RESPONSE_LEN];
    LPSTR lpszPtr;
    char  pszPrintableBuf[MAX_QUERY_RESPONSE_LEN];
    int   iRet, i, j;
    int   iCurQuery;
    int   iResponseFailureCount;
    ULONG ulCrcTable[256], ulCrc;
    char  szASCIIModem[MAX_MODEM_ID_LEN+1];
    
    ASSERT(pszModemName);
    ASSERT(lpszATI0Result);
    *lpszATI0Result = (TCHAR)0; // null-terminate in case we fail

    DBG_ENTER(IdentifyModem);
    
    // Build CRC table
    for (i = 0; i < 256; i++)
    {
        ulCrc = i;
        for (j = 8; j > 0; j--) 
        {
            if (ulCrc & 1) 
            {
                ulCrc = (ulCrc >> 1) ^ 0xEDB88320L;
            } 
            else
            {
                ulCrc >>= 1;
            }
        }
        ulCrcTable[i] = ulCrc;
    }

    // Init ulCrc
    ulCrc = 0xFFFFFFFF;
        
    // Do each query.
    for (iCurQuery = 0, iResponseFailureCount = 0;
         iCurQuery < ARRAYSIZE(c_aszQueries); iCurQuery++)
    {
        DetectSetStatus(pdc, DSS_QUERYING_RESPONSES);
#ifndef PROFILE_MASSINSTALL
        TRACE_MSG(TF_DETECT, "sending query '%s'.",
                 ConvertToPrintable(c_aszQueries[iCurQuery],
                                    pszPrintableBuf,
                                    sizeof(pszPrintableBuf)));
#endif
        cbLen = lstrlenA(c_aszQueries[iCurQuery]);
        if (MyWriteComm(hPort, (LPBYTE)c_aszQueries[iCurQuery], cbLen) != cbLen) 
        {
            TRACE_MSG(TF_ERROR, "WriteComm failed");
            iRet = RESPONSE_FAILURE;  // spoof ReadResponse return for following switch handler
        }
        else
        {
            // Read in response.  Handle multi-pagers.  Return a null-terminated
            // string containing all or part of the response.  Return response
            // code.
            iRet = ReadResponse(hPort, (LPBYTE)pszReadBuf, sizeof(pszReadBuf), TRUE,
                                RESPONSE_RCV_DELAY, pdc);
#ifdef DEBUG
            switch (iRet)
            {
            case RESPONSE_FAILURE:
                TRACE_MSG(TF_DETECT, "ReadResponse returned RESPONSE_FAILURE");
                break;
            case RESPONSE_UNRECOG:
                TRACE_MSG(TF_DETECT, "ReadResponse returned RESPONSE_UNRECOG");
                break;
            case RESPONSE_NONE:
                TRACE_MSG(TF_DETECT, "ReadResponse returned RESPONSE_NONE");
                break;
            case RESPONSE_USER_CANCEL:
                TRACE_MSG(TF_DETECT, "ReadResponse returned RESPONSE_USER_CANCEL");
                break;
            }
#endif // DEBUG
        }

        switch (iRet)
        {
        case RESPONSE_USER_CANCEL:
            return ERROR_CANCELLED;

        case RESPONSE_FAILURE:
        case RESPONSE_UNRECOG:
        case RESPONSE_NONE:
            iResponseFailureCount++;
            if (iResponseFailureCount >= MAX_RESPONSE_FAILURES)
            {
                TRACE_MSG(TF_ERROR, "had %d failed responses, aborting IdentifyModem()", iResponseFailureCount);
                return ERROR_PORT_INACCESSIBLE;
            }
            else
            {
                DCB DCB;
                BOOL fCancel;

                // Get current baud rate
                if (GetCommState(hPort, &DCB) < 0) 
                {
                    TRACE_MSG(TF_ERROR, "GetCommState failed");
                    return ERROR_PORT_INACCESSIBLE;
                }

                if (!TestBaudRate(hPort, DCB.BaudRate, 0, pdc, &fCancel))  // attempt to recover friendship with the modem
                {
                    if (fCancel)
                    {
                        return ERROR_CANCELLED;
                    }
                    else
                    {
                        TRACE_MSG(TF_ERROR, "couldn't recover contact with the modem.");
                        return ERROR_PORT_INACCESSIBLE;
                    }
                }
                iCurQuery--;    // try the same query again
            }
            break;

        case RESPONSE_OK:
        case RESPONSE_ERROR:
            CleanseResponse(iCurQuery, pszReadBuf, pszCRCBuf);
            
            if (ATI0 == iCurQuery)
            {
                ASSERT(ATI0_LEN <= sizeof(pszCRCBuf));  // make sure we are doing a legal copy
                CopyMemory(lpszATI0Result, pszCRCBuf, ATI0_LEN);
            }

            lpszPtr = (LPSTR) pszCRCBuf;

            while (*lpszPtr)
            {
                ulCrc = ((ulCrc >> 8) & 0x00FFFFFF) ^ ulCrcTable[(ulCrc ^ *lpszPtr++) & 0xFF];
            }


            LogPrintf(hLog, IDS_DET_OK_1,
                      ConvertToPrintable(c_aszQueries[iCurQuery],
                                         pszPrintableBuf,
                                         sizeof(pszPrintableBuf)));
            LogPrintf(hLog, IDS_DET_OK_2,
                      ConvertToPrintable(pszReadBuf,
                                         pszPrintableBuf,
                                         sizeof(pszPrintableBuf)));
#ifndef PROFILE_MASSINSTALL
            TRACE_MSG(TF_DETECT, "response (len=%d): %s", lstrlenA(pszReadBuf),
                     ConvertToPrintable(pszReadBuf,
                                        pszPrintableBuf,
                                        sizeof(pszPrintableBuf)));
#endif
            LogPrintf(hLog, IDS_DET_OK_1,
                      ConvertToPrintable(c_aszQueries[iCurQuery],
                                         pszPrintableBuf,
                                         sizeof(pszPrintableBuf)));
            LogPrintf(hLog, IDS_DET_OK_2,
                      ConvertToPrintable(pszCRCBuf,
                                         pszPrintableBuf,
                                         sizeof(pszPrintableBuf)));
#ifndef PROFILE_MASSINSTALL
            TRACE_MSG(TF_DETECT, "converted form   : %s",
                     ConvertToPrintable(pszCRCBuf,
                                        pszPrintableBuf,
                                        sizeof(pszPrintableBuf)));
#endif
            iResponseFailureCount = 0;  // reset count of failed responses to 0 for upcoming query
            break;
            
        default:
            TRACE_MSG(TF_ERROR, "hit a default it shouldn't have hit.");
            ASSERT(0);
            return ERROR_PORT_INACCESSIBLE;
        }
    }

    // Finish up CRC
    ulCrc ^= 0xFFFFFFFF;

    lstrcpyA(szASCIIModem, c_szModemIdPrefix);
    j = lstrlenA(szASCIIModem);

    // Convert CRC into hex text.
    for (i = 0; i < 8; i++)
    {
        szASCIIModem[i+j] = "0123456789ABCDEF"[(ulCrc>>((7-i)<<2))&0xf];
    }
    szASCIIModem[i+j] = 0; // null-terminate

    DBG_EXIT(IdentifyModem);
    TRACE_MSG(TF_DETECT, "final CRC = 0x%8lx (ascii = %s)", ulCrc, szASCIIModem);

    LogPrintf(hLog, IDS_DET_ID, szASCIIModem);

#ifdef  UNICODE
    MultiByteToWideChar(CP_ACP, 0, szASCIIModem, -1, pszModemName, MAX_MODEM_ID_LEN+1);
    // match lstrcpyn behaviour of always null-terminating the line.
    pszModemName[MAX_MODEM_ID_LEN]=0;
#else
    lstrcpynA(pszModemName, szASCIIModem, MAX_MODEM_ID_LEN+1);
#endif  // UNICODE
                          
    return NO_ERROR;
}

// returns buffer full o' data and an int.
// if dwRcvDelay is NULL, default RCV_DELAY will be used, else
// dwRcvDelay (miliseconds) will be used
// *lpfCancel will be true if we are exiting because of a user requested cancel.
UINT 
PRIVATE
ReadPort(
    HPORT   hPort, 
    LPBYTE  lpvBuf, 
    UINT    uRead, 
    DWORD   dwRcvDelay, 
    int FAR *lpiError, 
    PDETECTCALLBACK pdc, 
    BOOL FAR *lpfCancel)
{
    DWORD cb, cbLenRet;
    UINT uTotal = 0;
    DWORD tStart;
    DWORD dwDelay;
    COMSTAT comstat;
    COMMTIMEOUTS cto;
    DWORD   dwError;
    DWORD cbLeft;
#ifdef DEBUG
    DWORD dwZeroCount = 0;
#endif // DEBUG

    ASSERT(lpvBuf);
    ASSERT(uRead);
    ASSERT(lpiError);

    *lpiError = 0;
    *lpfCancel = FALSE;
    
    tStart = GetTickCount();
    dwDelay = dwRcvDelay ? dwRcvDelay : RCV_DELAY;
    
    // save space for terminator
    uRead--;
    cbLeft=uRead;


    // Set comm timeout
    if (!GetCommTimeouts(hPort, &cto))
    {
      ZeroMemory(&cto, sizeof(cto));
    };
    // Allow a constant write timeout
    cto.ReadIntervalTimeout        = 0;
    cto.ReadTotalTimeoutMultiplier = 0;
    cto.ReadTotalTimeoutConstant   = 25; 
    SetCommTimeouts(hPort, &cto);

    do
    {
        cb = 0;
        while(  cbLeft
                && ReadFile(hPort, lpvBuf + uTotal + cb, 1, &cbLenRet, NULL)
                && (cbLenRet))
        {
          ASSERT(cbLenRet==1);
          cb ++;
          cbLeft--;
        };

#ifdef DEBUG
        if (cb)
        {
           // TRACE_MSG(TF_DETECT, "ReadComm returned %d (zero count = %d)", cb, dwZeroCount);    
            dwZeroCount = 0;
        }
        else
        {
            dwZeroCount++;
        }
#endif // DEBUG

        {
            MSG msg;

            while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                DispatchMessage(&msg);
            };
        }

        if (cb == 0)  // possible error?
        {
            //*lpiError |= GetCommError(hPort, &comstat);
            dwError = 0;
            ClearCommError(hPort, &dwError, &comstat);
            *lpiError |= dwError;
#ifdef DEBUG
            if (dwError)
            {
              TRACE_MSG(TF_DETECT, "ReadComm returned %d, comstat: status = %hx, in = %u, out = %u",
                                  cb, dwError, comstat.cbInQue, comstat.cbOutQue);
            };
#endif // DEBUG
        }

        if (cb)
        {
            // successful read - add to total and reset delay
            uTotal += cb;

            if (uTotal >= uRead)
            {
                ASSERT(uTotal == uRead);
                break;
            }
            tStart = GetTickCount();
            dwDelay = CHAR_DELAY;
        }
        else
        {
            if (DetectQueryCancel(pdc))
            {
                TRACE_MSG(TF_DETECT, "User pressed cancel.");
                *lpfCancel = TRUE;
                break;
            }
        }

     // While read is successful && time since last read < delay allowed)       
    } while (cbLeft && (GetTickCount() - tStart) < dwDelay);
               
    *(lpvBuf+uTotal) = 0;
    
#ifndef PROFILE_MASSINSTALL
    TRACE_MSG(TF_DETECT, "ReadPort returning %d", uTotal);    
#endif
    return uTotal;
}

// Convert CBR format speeds to decimal.  Returns 0 on error
DWORD NEAR PASCAL CBR_To_Decimal(UINT uiCBR)
{
    DWORD dwBaudRate;

    switch (uiCBR)
    {
    case CBR_300:
        dwBaudRate = 300L;
        break;
    case CBR_1200:
        dwBaudRate = 1200L;
        break;
    case CBR_2400:
        dwBaudRate = 2400L;
        break;
    case CBR_9600:
        dwBaudRate = 9600L;
        break;
    case CBR_19200:
        dwBaudRate = 19200L;
        break;
    case CBR_38400:
        dwBaudRate = 38400L;
        break;
    case CBR_56000:
        dwBaudRate = 57600L;
        break;
    case CBR_HACK_115200:
        dwBaudRate = 115200L;
        break;
//    case CBR_110:
//    case CBR_600:
//    case CBR_4800:
//    case CBR_14400:
//    case CBR_128000:
//    case CBR_256000:
    default:
        TRACE_MSG(TF_ERROR, "An unsupported CBR_x value was used.");
        dwBaudRate = 0;
        break;
    }
    return dwBaudRate;
}

// Convert pszIn to a printable pszOut, not using more than cbOut bytes.
// WARNING: Not a DBCS function.
// Returns a pointer to the output buffer.  Always successful.
LPSTR NEAR ConvertToPrintable(LPCSTR lpszIn, LPSTR lpszOut, UINT uOut)
{
    LPSTR lpszReturn = lpszOut;

    ASSERT(lpszOut);
    ASSERT(lpszIn);
    ASSERT(uOut);

    uOut--;  // save space for the null-terminator

    while (*lpszIn)
    {
        // ascii printable chars are between 0x20 and 0x7e, inclusive                                    
        if (*lpszIn >= 0x20 && *lpszIn <= 0x7e)
        {
            // printable text
            if (uOut)
            {
                uOut--;
                *lpszOut++ = *lpszIn;
            }
            else
            {
                break;
            }
        }
        else
        {
            // binary
            if (uOut >= 4)
            {
                uOut -= 4;
                switch (*lpszIn)
                {
                case CR:
                    *lpszOut++ = '<'; *lpszOut++ = 'c'; *lpszOut++ = 'r'; *lpszOut++ = '>';
                    break;
                case LF:
                    *lpszOut++ = '<'; *lpszOut++ = 'l'; *lpszOut++ = 'f'; *lpszOut++ = '>';
                    break;
                default:
                    *lpszOut++ = '<';
                    *lpszOut++ = c_szHex[(*lpszIn>>4) & 0xf];
                    *lpszOut++ = c_szHex[*lpszIn & 0xf];
                    *lpszOut++ = '>';
                }
            }
            else
            {
                break;
            }
        }
        lpszIn++;
    }

    *lpszOut = 0; // make sure we are null-terminated

    return lpszReturn;
}


//-----------------------------------------------------------------------------------
//  Detection Signature structure functions
//-----------------------------------------------------------------------------------


/*----------------------------------------------------------
Purpose: This function initializes a MODEM_DETECT_SIG signature
         structure.

Returns: TRUE on success
Cond:    --
*/
BOOL
PUBLIC
DetectSig_Init(
    IN  PMODEM_DETECT_SIG   pmds,
    IN  DWORD               dwFlags,
    IN  LPCTSTR             pszHardwareID,  OPTIONAL
    IN  LPCTSTR             pszPort)        OPTIONAL
    {
    BOOL bRet;

    ASSERT(pmds);

    if ( !pmds )
        {
        bRet = FALSE;
        }
    else
        {
        ZeroInit(pmds);
        pmds->cbSize = sizeof(*pmds);
        pmds->dwMask = 0;
        pmds->dwFlags = dwFlags;

        if (pszHardwareID)
            {
            SetFlag(pmds->dwMask, MDSM_HARDWAREID);
            lstrcpyn(pmds->szHardwareID, pszHardwareID, SIZECHARS(pmds->szHardwareID));
            }

        if (pszPort)
            {
            SetFlag(pmds->dwMask, MDSM_PORT);
            lstrcpyn(pmds->szPort, pszPort, SIZECHARS(pmds->szPort));
            }
        }

    return bRet;
    }


/*----------------------------------------------------------
Purpose: This function returns TRUE if the modem detection
         signature is valid.

Returns: see above
Cond:    --
*/
BOOL
PUBLIC
DetectSig_Validate(
    IN PMODEM_DETECT_SIG    pmds)
    {
    return (pmds &&
            sizeof(*pmds) == pmds->cbSize &&
            0 == (pmds->dwMask & ~MDSM_ALL) &&
            0 == (pmds->dwFlags & ~MDSF_ALL));
    }


/*----------------------------------------------------------
Purpose: If the detection signature's port field does not
         match what the device's AttachedTo property says,
         then update it.

Returns: TRUE   if the detection signature needs to be set
         FALSE  if the detection signature is okay as it is

Cond:    --
*/
BOOL
PRIVATE
UpdateAttachedPort(
    IN     HDEVINFO             hdi,
    IN     PSP_DEVINFO_DATA     pdevData,
    IN OUT PMODEM_DETECT_SIG    pmds)
    {
    BOOL bRet;
    MODEM_PRIV_PROP mpp;

    // Get the attached port
    mpp.cbSize = sizeof(mpp);
    mpp.dwMask = MPPM_PORT;

    bRet = CplDiGetPrivateProperties(hdi, pdevData, &mpp);
    if (bRet)
        {
        // Assume that the signature does not need changing
        bRet = FALSE;

        if (IsFlagSet(mpp.dwMask, MPPM_PORT))
            {
            // Do we need to update the signature?
            if (IsFlagClear(pmds->dwMask, MDSM_PORT) ||
                IsFlagSet(pmds->dwMask, MDSM_PORT) && 
                    !IsSzEqual(pmds->szPort, mpp.szPort))
                {
                // Yes
                SetFlag(pmds->dwMask, MDSM_PORT);
                lstrcpyn(pmds->szPort, mpp.szPort, SIZECHARS(pmds->szPort));
                bRet = TRUE;
                }
            }
        }

    return bRet;
    }
    

/*----------------------------------------------------------
Purpose: This function compares two modem detection signatures
         and determines if they are identical.

Returns: NO_ERROR
         ERROR_DUPLICATE_FOUND  if the modem signatures match
         other errors

Cond:    --
*/
DWORD
CALLBACK
DetectSig_Compare(
    IN HDEVINFO         hdi,
    IN PSP_DEVINFO_DATA pdevDataNew,
    IN PSP_DEVINFO_DATA pdevDataExisting,
    IN PVOID            lParam)             OPTIONAL
    {
    DWORD dwRet;
    PDETECTSIG_PARAMS pparams = (PDETECTSIG_PARAMS)lParam;

    ASSERT(hdi && INVALID_HANDLE_VALUE != hdi);
    ASSERT(pdevDataNew);
    ASSERT(pdevDataExisting);

    try 
        {
        MODEM_DETECT_SIG mdsNew;
        MODEM_DETECT_SIG mdsExisting;
        DWORD dwComparedMask = 0;
        DWORD dwMatchingMask = 0;

        // Assume the signatures don't match
        dwRet = NO_ERROR;

        mdsNew.cbSize = sizeof(mdsNew);
        mdsExisting.cbSize = sizeof(mdsExisting);

        // Get the detection signatures for both device instances
        if (CplDiGetDetectSignature(hdi, pdevDataNew, &mdsNew) &&
            CplDiGetDetectSignature(hdi, pdevDataExisting, &mdsExisting))
            {
            // Do the sizes look like they are valid detection signatures?
            if (sizeof(MODEM_DETECT_SIG) == mdsNew.cbSize &&
                sizeof(MODEM_DETECT_SIG) == mdsExisting.cbSize)
                {
                // Yes; make sure the detection signature is up-to-date
                DWORD cFieldsCompared = 0;
                DWORD cMatches = 0;
                
                // Since the user can attach the modem to any port
                // after first installing it, let's go check what the
                // existing modem is attached to currently.
                if (UpdateAttachedPort(hdi, pdevDataExisting, &mdsExisting))
                    {
                    CplDiSetDetectSignature(hdi, pdevDataExisting, &mdsExisting);
                    }
                

                // Compare each of the valid fields

                // Count the number of fields that can be compared, and
                // the number of matching fields.  If these two numbers
                // are the same and the number of fields that can be
                // compared is not 0, then we have a duplicate device.

                if (IsFlagSet(mdsNew.dwMask, MDSM_HARDWAREID) &&
                    IsFlagSet(mdsExisting.dwMask, MDSM_HARDWAREID))
                    {
                    SetFlag(dwComparedMask, MDSM_HARDWAREID);
                    cFieldsCompared++;

                    if (IsSzEqual(mdsNew.szHardwareID, mdsExisting.szHardwareID))
                        {
                        SetFlag(dwMatchingMask, MDSM_HARDWAREID);
                        cMatches++;
                        }
                    }

                if (IsFlagSet(mdsNew.dwMask, MDSM_PORT) &&
                    IsFlagSet(mdsExisting.dwMask, MDSM_PORT))
                    {
                    SetFlag(dwComparedMask, MDSM_PORT);
                    cFieldsCompared++;

                    if (IsSzEqual(mdsNew.szPort, mdsExisting.szPort))
                        {
                        SetFlag(dwMatchingMask, MDSM_PORT);
                        cMatches++;
                        }
                    }

                // There are some minimum requirements for two devices
                // to be duplicates.  
                //
                //   1)  Both must have a matching hardware ID
                //   2)  Both must be on the same port
                //
                // Anything else is frosting on the cake.

                // Did the hardware ID and port match?
                if ( !(2 == cFieldsCompared && 2 == cMatches) )
                    {
                    // No; don't waste anymore of our time
                    goto Leave;
                    }

                if (IsFlagSet(mdsNew.dwMask, MDSM_DEVICEDESC) &&
                    IsFlagSet(mdsExisting.dwMask, MDSM_DEVICEDESC))
                    {
                    SetFlag(dwComparedMask, MDSM_DEVICEDESC);
                    cFieldsCompared++;

                    if (IsSzEqual(mdsNew.szDeviceDesc, mdsExisting.szDeviceDesc))
                        {
                        SetFlag(dwMatchingMask, MDSM_DEVICEDESC);
                        cMatches++;
                        }
                    }


                // Do we have a duplicate device?
                if (0 < cFieldsCompared && cFieldsCompared == cMatches)
                    {
                    // Yes
                    dwRet = ERROR_DUPLICATE_FOUND;
                    }
                }
            }


Leave:
        // Return to the caller some information about which parts
        // of the detection signature matched.
        if (pparams)
            {
            pparams->dwComparedMask = dwComparedMask;
            pparams->dwMatchingMask = dwMatchingMask;
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER)
        {
        dwRet = ERROR_INVALID_PARAMETER;
        }

    return dwRet;
    }

#ifdef DEBUG
void HexDump(TCHAR *ptchHdr, LPBYTE lpBuf, DWORD cbLen)
{
    TCHAR rgch[10000];
	TCHAR *pc = rgch;
	TCHAR *pcMore = TEXT("");

	if (DisplayDebug(TF_DETECT))
    {
		pc += wsprintf(pc, TEXT("HEX DUMP(%s,%lu): ["), ptchHdr, cbLen);
		if (cbLen>1000) {pcMore = TEXT(", ..."); cbLen=1000;}

		for(;cbLen--; lpBuf++)
		{
			pc += wsprintf(pc, TEXT(" %02lx"), (unsigned long) *lpBuf);
			if (!((cbLen+1)%20))
			{
				pc += wsprintf(pc, TEXT("\r\n"));
			}
		}
		pc += wsprintf(pc, TEXT("]\r\n"));

		OutputDebugString(rgch);
	}
}
#endif // DEBUG
