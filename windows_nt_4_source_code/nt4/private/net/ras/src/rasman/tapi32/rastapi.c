//****************************************************************************
//
//                     Microsoft NT Remote Access Service
//
//	Copyright (C) 1994-95 Microsft Corporation. All rights reserved.
//
//  Filename: rastapi.c
//
//  Revision History
//
//  Mar  28 1992   Gurdeep Singh Pall	Created
//
//
//  Description: This file contains all entry points for TAPI.DLL
//
//****************************************************************************

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <tapi.h>
#include <rasndis.h>
#include <wanioctl.h>
#include <rasman.h>
#include <raserror.h>
#include <eventlog.h>

#include <media.h>
#include <device.h>
#include <rasmxs.h>
#include <isdn.h>
#include <serial.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include "rastapi.h"


extern DWORD	    TotalPorts ;
//extern DWORD	    NegotiatedApiVersion ;
//extern DWORD	    NegotiatedExtVersion ;
extern HLINEAPP	    RasLine ;
extern HINSTANCE    RasInstance ;
extern TapiLineInfo *RasTapiLineInfo ;
extern TapiPortControlBlock *RasPorts ;
extern TapiPortControlBlock *RasPortsEnd ;
extern HANDLE	    RasTapiMutex ;
extern BOOL	    Initialized ;
extern DWORD       TapiThreadId    ;
extern HANDLE      TapiThreadHandle;
extern DWORD       LoaderThreadId;
extern DWORD       ValidPorts;
extern HANDLE	   ghAsyMac ;


DWORD GetInfo (TapiPortControlBlock *, BYTE *, WORD *) ;
DWORD SetInfo (TapiPortControlBlock *, RASMAN_PORTINFO *) ;
DWORD GetGenericParams (TapiPortControlBlock *, RASMAN_PORTINFO *, PWORD) ;
DWORD GetIsdnParams (TapiPortControlBlock *, RASMAN_PORTINFO * , PWORD) ;
DWORD GetX25Params (TapiPortControlBlock *, RASMAN_PORTINFO *, PWORD) ;
DWORD FillInX25Params (TapiPortControlBlock *, RASMAN_PORTINFO *) ;
DWORD FillInIsdnParams (TapiPortControlBlock *, RASMAN_PORTINFO *) ;
DWORD FillInGenericParams (TapiPortControlBlock *, RASMAN_PORTINFO *) ;
DWORD FillInUnimodemParams (TapiPortControlBlock *, RASMAN_PORTINFO *) ;
VOID  SetModemParams (TapiPortControlBlock *hIOPort, LINECALLPARAMS *linecallparams) ;
DWORD InitiatePortDisconnection (TapiPortControlBlock *hIOPort) ;
TapiPortControlBlock *LookUpControlBlock (HANDLE hPort) ;
DWORD ValueToNum(RAS_PARAMS *p) ;


//*  Initialization Routine  *************************************************
//

//*  DllEntryPoint
//
// Function:
//
// Returns: TRUE if successful, else FALSE.
//
//*

BOOL APIENTRY
DllEntryPoint(HANDLE hDll, DWORD dwReason, LPVOID pReserved)
{
  static BOOL  bFirstCall = TRUE;

  switch(dwReason)
  {
    case DLL_PROCESS_ATTACH:
      break;
    case DLL_PROCESS_DETACH:
      break;
    case DLL_THREAD_ATTACH:
      break;
    case DLL_THREAD_DETACH:
      break;
  }

  return(TRUE);

  UNREFERENCED_PARAMETER(hDll);
  UNREFERENCED_PARAMETER(pReserved);
}






//*  Serial APIs  ************************************************************
//


//*  PortEnum  ---------------------------------------------------------------
//
// Function: This API returns a buffer containing a PortMediaInfo struct.
//
// Returns: SUCCESS
//          ERROR_BUFFER_TOO_SMALL
//          ERROR_READING_SECTIONNAME
//          ERROR_READING_DEVICETYPE
//          ERROR_READING_DEVICENAME
//          ERROR_READING_USAGE
//          ERROR_BAD_USAGE_IN_INI_FILE
//
//*

DWORD  APIENTRY
PortEnum(BYTE *pBuffer, WORD *pwSize, WORD *pwNumPorts)
{
    PortMediaInfo *pinfo ;
    TapiPortControlBlock *pports ;
    DWORD numports = 0;
    DWORD i ;

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    if (!Initialized) {
		HANDLE	event;

        LoaderThreadId = GetCurrentThreadId();

        event = CreateEvent (NULL, FALSE, FALSE, NULL) ;
        TapiThreadHandle = CreateThread (NULL, 5000, (LPTHREAD_START_ROUTINE) EnumerateTapiPorts,
			   (LPVOID) event,
			   0,
			   &TapiThreadId);
	
        WaitForSingleObject (event, INFINITE) ;

        if (RasLine == 0 || !ValidPorts) {

			//
			// Wait for the thread to go away!
			//
			WaitForSingleObject(TapiThreadHandle, INFINITE);

			CloseHandle (TapiThreadHandle) ;

           	// *** Exclusion End ***
	        FreeMutex (RasTapiMutex) ;

	        return ERROR_TAPI_CONFIGURATION ;
        }

        CloseHandle (event) ;

	    Initialized = TRUE ;
    }

    // calculate the number of valid ports
    //
    for (pports = RasPorts, i=0; i < TotalPorts; i++, pports++) {
	    if (pports->TPCB_State == PS_UNINITIALIZED)
	        continue ;
	    numports++ ;
    }


    if (*pwSize < numports*sizeof(PortMediaInfo)) {

	*pwNumPorts = (WORD) numports ;
	*pwSize = (WORD) *pwNumPorts*sizeof(PortMediaInfo) ;

	// *** Exclusion End ***
	FreeMutex (RasTapiMutex) ;
	return ERROR_BUFFER_TOO_SMALL ;
    }

    *pwNumPorts = 0 ;
    pinfo = (PortMediaInfo *)pBuffer ;

    for (pports = RasPorts, i=0; i < TotalPorts; i++, pports++) {

	if (pports->TPCB_State	== PS_UNINITIALIZED)
	    continue ;

	strcpy (pinfo->PMI_Name, pports->TPCB_Name) ;
	pinfo->PMI_Usage = pports->TPCB_Usage ;
	strcpy (pinfo->PMI_DeviceType, pports->TPCB_DeviceType) ;
	strcpy (pinfo->PMI_DeviceName, pports->TPCB_DeviceName) ;
    pinfo->PMI_LineDeviceId = pports->TPCB_Line->TLI_LineId;
    pinfo->PMI_AddressId = pports->TPCB_AddressId;

	pinfo++ ;
	(*pwNumPorts)++	;
    }

    // *** Exclusion End ***
    FreeMutex (RasTapiMutex) ;

    return(SUCCESS);
}



//*  PortOpen  ---------------------------------------------------------------
//
// Function: This API opens a COM port.  It takes the port name in ASCIIZ
//           form and supplies a handle to the open port.  hNotify is use
//           to notify the caller if the device on the port shuts down.
//
//           PortOpen allocates a SerialPCB and places it at the head of
//           the linked list of Serial Port Control Blocks.
//
// Returns: SUCCESS
//          ERROR_PORT_NOT_CONFIGURED
//          ERROR_DEVICE_NOT_READY
//
//*

DWORD  APIENTRY
PortOpen(char *pszPortName, HANDLE *phIOPort, HANDLE hNotify)
{
    TapiPortControlBlock *pports ;
    DWORD   retcode ;
    DWORD   i ;

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    pports = RasPorts ;

    for (i=0; i < TotalPorts; i++) {
	if (_stricmp(pszPortName, pports->TPCB_Name) == 0)
	    break ;
	pports++ ;
    }

    if (i < TotalPorts) {

	    if (pports->TPCB_State == PS_UNINITIALIZED) {
	        // **** Exclusion END ****
	        FreeMutex (RasTapiMutex) ;
	        return ERROR_TAPI_CONFIGURATION ;
	    }

	    if (pports->TPCB_State != PS_CLOSED) {
	        // **** Exclusion END ****
	        FreeMutex (RasTapiMutex) ;
	        return ERROR_PORT_ALREADY_OPEN ;
	    }

	    if (pports->TPCB_Line->TLI_LineState == PS_CLOSED) { // open line
            LINEDEVCAPS     *linedevcaps ;
            BYTE            buffer[400] ;

            linedevcaps = (LINEDEVCAPS *)buffer ;
            linedevcaps->dwTotalSize = sizeof (buffer) ;

            lineGetDevCaps (RasLine, pports->TPCB_Line->TLI_LineId, pports->TPCB_Line->NegotiatedApiVersion, pports->TPCB_Line->NegotiatedExtVersion, linedevcaps) ;

            // Remove LINEMEDIAMODE_INTERACTIVEVOICE from the media mode since this mode cannot be
            // used for receiving calls.
            //
            pports->TPCB_MediaMode = linedevcaps->dwMediaModes & ~(LINEMEDIAMODE_INTERACTIVEVOICE) ;

	    retcode =
	         lineOpen (RasLine,
	    	       pports->TPCB_Line->TLI_LineId,
	    	       &pports->TPCB_Line->TLI_LineHandle,
	    	       pports->TPCB_Line->NegotiatedApiVersion,
	    	       pports->TPCB_Line->NegotiatedExtVersion,
	    	       (ULONG) pports->TPCB_Line,
	    	       LINECALLPRIVILEGE_OWNER,
	    	       pports->TPCB_MediaMode,
	    	       NULL) ;

	        if (retcode) {

		    // **** Exclusion END ****
		    FreeMutex (RasTapiMutex) ;
		    return retcode ;
	        }

		//
		// Set monitoring of rings
		//
		lineSetStatusMessages (pports->TPCB_Line->TLI_LineHandle, LINEDEVSTATE_RINGING, 0) ;

		//
		//  Always turn off the modem lights incase this is a modem device
		//
		if ((_stricmp (pports->TPCB_DeviceType, DEVICETYPE_UNIMODEM) == 0)) {

		    //
		    // unimodem struct not defined in any header
		    //
		    typedef struct _DEVCFG {
			DWORD	dwSize;
			DWORD	dwVersion;
			WORD	fwOptions;
			WORD	wWaitBong;
		    } DEVCFG;

#define LAUNCH_LIGHTS 8

		    LPVARSTRING var ;
		    BYTE	buffer[1000] ;
		    DEVCFG	*devcfg ;

		    var = (LPVARSTRING)buffer ;
		    var->dwTotalSize  = 1000 ;
		    var->dwStringSize = 0 ;
		    lineGetDevConfig (pports->TPCB_Line->TLI_LineId, var, "comm/datamodem") ;
		    devcfg = (DEVCFG*) (((LPBYTE) var) + var->dwStringOffset) ;
		    devcfg->fwOptions &= ~LAUNCH_LIGHTS ;

		    lineSetDevConfig (pports->TPCB_Line->TLI_LineId, devcfg, var->dwStringSize, "comm/datamodem") ;

		}

	        pports->TPCB_Line->TLI_LineState = PS_OPEN ;
	    }

	    // Initialize the parameters
	    //
	    pports->TPCB_Info[0][0] = '\0' ;
	    pports->TPCB_Info[1][0] = '\0' ;
	    pports->TPCB_Info[2][0] = '\0' ;
	    pports->TPCB_Info[3][0] = '\0' ;
	    pports->TPCB_Info[4][0] = '\0' ;
	    strcpy (pports->TPCB_Info[ISDN_CONNECTBPS_INDEX], "64000") ;

	    pports->TPCB_Line->TLI_OpenCount++ ;
	    pports->TPCB_DiscNotificationHandle = hNotify ;

	    // DbgPrint ("RASTAPI: TPCB_DiscNotificationHandle == %x\n", pports->TPCB_DiscNotificationHandle) ;

	    pports->TPCB_State = PS_OPEN ;
	    pports->TPCB_DisconnectReason = 0 ;
	    pports->TPCB_CommHandle = INVALID_HANDLE_VALUE ;

	    *phIOPort = (HANDLE) pports ;

	    // **** Exclusion END ****
	    FreeMutex (RasTapiMutex) ;

	    return(SUCCESS);

    }

   // **** Exclusion END ****
   FreeMutex (RasTapiMutex) ;

   return ERROR_PORT_NOT_CONFIGURED ;


}


//*  PortClose  --------------------------------------------------------------
//
// Function: This API closes the COM port for the input handle.  It also
//           finds the SerialPCB for the input handle, removes it from
//           the linked list, and frees the memory for it
//
// Returns: SUCCESS
//          Values returned by GetLastError()
//
//*

DWORD  APIENTRY
PortClose (HANDLE hIOPort)
{
    TapiPortControlBlock *pports = (TapiPortControlBlock *) hIOPort ;

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    pports->TPCB_Line->TLI_OpenCount-- ;
    pports->TPCB_State = PS_CLOSED ;

    if (pports->TPCB_DevConfig)
        LocalFree (pports->TPCB_DevConfig) ;
    pports->TPCB_DevConfig = NULL ;

    if (pports->TPCB_Line->TLI_OpenCount == 0) {
	    pports->TPCB_Line->TLI_LineState = PS_CLOSED ;
	    lineClose (pports->TPCB_Line->TLI_LineHandle) ;
    }

    // **** Exclusion END ****
    FreeMutex (RasTapiMutex) ;

    return(SUCCESS);
}


//*  PortGetInfo  ------------------------------------------------------------
//
// Function: This API returns a block of information to the caller about
//           the port state.  This API may be called before the port is
//           open in which case it will return inital default values
//           instead of actual port values.
//
//           hIOPort can be null in which case use portname to give information
//           hIOPort may be the actual file handle or the hIOPort returned in port open.
//
// Returns: SUCCESS
//
//*

DWORD  APIENTRY
PortGetInfo(HANDLE hIOPort, TCHAR *pszPortName, BYTE *pBuffer, WORD *pwSize)
{
    DWORD i ;
    DWORD retcode = ERROR_FROM_DEVICE ;

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    // hIOPort or pszPortName must be valid:
    //
    for (i=0; i < TotalPorts; i++) {
	    if (!_stricmp(RasPorts[i].TPCB_Name, pszPortName) || (hIOPort == (HANDLE) &RasPorts[i]) || (hIOPort == RasPorts[i].TPCB_CommHandle)) {
	        hIOPort = (HANDLE) &RasPorts[i] ;
	        retcode = GetInfo ((TapiPortControlBlock *) hIOPort, pBuffer, pwSize) ;
	        break ;
	    }
    }

    // **** Exclusion END ****
    FreeMutex (RasTapiMutex) ;

    return (retcode);
}



//*  PortSetInfo  ------------------------------------------------------------
//
// Function: The values for most input keys are used to set the port
//           parameters directly.  However, the carrier BPS and the
//           error conrol on flag set fields in the Serial Port Control
//           Block only, and not the port.
//
//           hIOPort may the port handle returned in portopen or the actual file handle.
//
// Returns: SUCCESS
//          ERROR_WRONG_INFO_SPECIFIED
//          Values returned by GetLastError()
//*

DWORD  APIENTRY
PortSetInfo(HANDLE hIOPort, RASMAN_PORTINFO *pInfo)
{
    DWORD retcode ;

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    hIOPort = LookUpControlBlock(hIOPort) ;

    retcode = SetInfo ((TapiPortControlBlock *) hIOPort, pInfo) ;

    // **** Exclusion END ****
    FreeMutex (RasTapiMutex) ;

    return (retcode);
}


//*  PortTestSignalState  ----------------------------------------------------
//
// Function: Really only has meaning if the call was active. Will return
//
// Returns: SUCCESS
//          Values returned by GetLastError()
//
//*

DWORD  APIENTRY
PortTestSignalState(HANDLE hPort, DWORD *pdwDeviceState)
{
    BYTE    buffer[200] ;
    LINECALLSTATUS *pcallstatus ;
    DWORD   retcode = SUCCESS ;
    TapiPortControlBlock *hIOPort = (TapiPortControlBlock *) hPort;

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    *pdwDeviceState = 0 ;

    memset (buffer, 0, sizeof(buffer)) ;

    pcallstatus = (LINECALLSTATUS *) buffer ;
    pcallstatus->dwTotalSize = sizeof (buffer) ;

    // First check if we have a disconnect reason stored away. if so return that.
    //
    if (hIOPort->TPCB_DisconnectReason) {

        *pdwDeviceState = hIOPort->TPCB_DisconnectReason ;

    } else if (hIOPort->TPCB_State != PS_CLOSED) {

        // Only in case of CONNECTING or CONNECTED do we care how the link dropped
        //
        if (hIOPort->TPCB_State == PS_CONNECTING || hIOPort->TPCB_State == PS_CONNECTED) {

	        retcode = lineGetCallStatus (hIOPort->TPCB_CallHandle, pcallstatus) ;

	        if (retcode)
	    	    ;
	        else if (pcallstatus->dwCallState == LINECALLSTATE_DISCONNECTED)
	    	    *pdwDeviceState = SS_LINKDROPPED ;
	        else if (pcallstatus->dwCallState == LINECALLSTATE_IDLE)
	    	     *pdwDeviceState = SS_HARDWAREFAILURE ;
	        else if (pcallstatus->dwCallState == LINECALLSTATE_SPECIALINFO)
	    	     *pdwDeviceState = SS_HARDWAREFAILURE ;

        } else

            *pdwDeviceState = SS_LINKDROPPED | SS_HARDWAREFAILURE ;
    }

    // **** Exclusion END ****
    FreeMutex (RasTapiMutex) ;

    // DbgPrint ("TestSigState: State=%d LINKDROP=%d, HWFAIL=%d\n", hIOPort->TPCB_State, *pdwDeviceState & SS_LINKDROPPED,*pdwDeviceState & SS_HARDWAREFAILURE) ;

    return retcode ;
}


//*  PortConnect  ------------------------------------------------------------
//
// Function: This API is called when a connection has been completed.
//           It in turn calls the asyncmac device driver in order to
//           indicate to asyncmac that the port and the connection
//           over it are ready for commumication.
//
//
// Returns: SUCCESS
//          ERROR_PORT_NOT_OPEN
//          ERROR_NO_CONNECTION
//          Values returned by GetLastError()
//*
DWORD  APIENTRY
PortConnect(HANDLE	        hPort,
            BOOL            bLegacyFlagNotUsed,
	        DWORD	        *endpoint)
{
    DCB		 DCB ;
    LINECALLINFO linecall ;
    ASYMAC_OPEN	AsyMacOpen;
    ASYMAC_DCDCHANGE    A ;
    DWORD		dwBytesReturned ;
    TapiPortControlBlock *hIOPort = (TapiPortControlBlock *) hPort;

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    // We must be connected to process this
    //
    if (hIOPort->TPCB_State != PS_CONNECTED) {
	// **** Exclusion END ****
	FreeMutex (RasTapiMutex) ;
	return ERROR_PORT_DISCONNECTED ;
    }

    // get the actual line speed at which we connected
    //
    memset (&linecall, 0, sizeof (linecall)) ;
    linecall.dwTotalSize = sizeof (linecall) ;
    lineGetCallInfo (hIOPort->TPCB_CallHandle, &linecall) ;
    _ltoa(linecall.dwRate, hIOPort->TPCB_Info[CONNECTBPS_INDEX], 10) ;

    if (_stricmp (hIOPort->TPCB_DeviceType, DEVICETYPE_UNIMODEM) == 0) {

	//
	// Before we send IOCTL to asyncmac - sanitize the DCB in case some app left
	// databits, stopbits, parity set to bad values.
	//
	if (!GetCommState(hIOPort->TPCB_CommHandle, &DCB)) {
        // **** Exclusion END ****
        FreeMutex (RasTapiMutex) ;
	    return(GetLastError());
    }

	DCB.ByteSize = 8 ;
	DCB.StopBits = ONESTOPBIT ;
	DCB.Parity   = NOPARITY ;

	if (!SetCommState(hIOPort->TPCB_CommHandle, &DCB)) {
        // **** Exclusion END ****
        FreeMutex (RasTapiMutex) ;
	    return(GetLastError());
    }

        AsyMacOpen.hNdisEndpoint = INVALID_HANDLE_VALUE ;
	AsyMacOpen.LinkSpeed  = linecall.dwRate ;
	AsyMacOpen.FileHandle = hIOPort->TPCB_CommHandle ;
        AsyMacOpen.QualOfConnect = (UINT)NdisWanErrorControl;

	if (ghAsyMac == (HANDLE) 0xffffffff)
             ghAsyMac = CreateFileW(L"\\\\.\\ASYNCMAC",
				GENERIC_READ | GENERIC_WRITE,
				FILE_SHARE_READ | FILE_SHARE_WRITE,
				NULL,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
				NULL);

	if (ghAsyMac == INVALID_HANDLE_VALUE) {
	    // *** Exclusion END ****
	    FreeMutex (RasTapiMutex) ;
	    return(GetLastError());
	}

        {
            OVERLAPPED overlapped ;
            memset (&overlapped, 0, sizeof(OVERLAPPED)) ;
            if (!DeviceIoControl(ghAsyMac,
                                 IOCTL_ASYMAC_OPEN,
                                 &AsyMacOpen,
                                 sizeof(AsyMacOpen),
                                 &AsyMacOpen,
                                 sizeof(AsyMacOpen),
                                 &dwBytesReturned,
				 &overlapped)) {
		// *** Exclusion END ****
		FreeMutex (RasTapiMutex) ;
		return(GetLastError());
	    }
        }

        hIOPort->TPCB_Endpoint = *endpoint = (DWORD) AsyMacOpen.hNdisEndpoint ;

    } else {

	*endpoint = hIOPort->TPCB_Endpoint ;

    }

    // **** Exclusion END ****
    FreeMutex (RasTapiMutex) ;

    return(SUCCESS);
}



//*  PortDisconnect  ---------------------------------------------------------
//
// Function: This API is called to drop a connection and close AsyncMac.
//
// Returns: SUCCESS
//          PENDING
//          ERROR_PORT_NOT_OPEN
//
//*
DWORD  APIENTRY
PortDisconnect(HANDLE hPort)
{
    DWORD retcode = SUCCESS ;
    TapiPortControlBlock *hIOPort = (TapiPortControlBlock *) hPort;

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    // DbgPrint ("PortDisconnect, state = %d\n", hIOPort->TPCB_State) ;

    if ((hIOPort->TPCB_State == PS_CONNECTED)  ||
	(hIOPort->TPCB_State == PS_CONNECTING) ||
	((hIOPort->TPCB_State == PS_LISTENING) && (hIOPort->TPCB_ListenState != LS_WAIT))) {

        retcode = InitiatePortDisconnection (hIOPort) ;

        // If we had saved away the device config then we restore it here.
        //
        if (hIOPort->TPCB_DefaultDevConfig) {
            lineSetDevConfig (hIOPort->TPCB_Line->TLI_LineId, hIOPort->TPCB_DefaultDevConfig, hIOPort->TPCB_DefaultDevConfigSize, "comm/datamodem") ;
            LocalFree (hIOPort->TPCB_DefaultDevConfig) ;
            hIOPort->TPCB_DefaultDevConfig = NULL ;
        }


    } else if (hIOPort->TPCB_State == PS_LISTENING) {

		hIOPort->TPCB_State = PS_OPEN ; // for LS_WAIT listen state case
        retcode = SUCCESS ;

    } else if (hIOPort->TPCB_State == PS_DISCONNECTING) {

		retcode = PENDING ;
    }

    // **** Exclusion END ****
    FreeMutex (RasTapiMutex) ;

    return retcode ;
}



//*  PortInit  ---------------------------------------------------------------
//
// Function: This API re-initializes the com port after use.
//
// Returns: SUCCESS
//          ERROR_PORT_NOT_CONFIGURED
//          ERROR_DEVICE_NOT_READY
//
//*

DWORD  APIENTRY
PortInit(HANDLE hIOPort)
{
  return(SUCCESS);
}



//*  PortSend  ---------------------------------------------------------------
//
// Function: This API sends a buffer to the port.  This API is
//           asynchronous and normally returns PENDING; however, if
//           WriteFile returns synchronously, the API will return
//           SUCCESS.
//
// Returns: SUCCESS
//          PENDING
//          Return code from GetLastError
//
//*

DWORD
PortSend(HANDLE hPort, BYTE *pBuffer, DWORD dwSize, HANDLE hAsyncEvent)
{
    TapiPortControlBlock *hIOPort = (TapiPortControlBlock *) hPort;
    DWORD retcode ;
    DWORD pdwBytesWritten;
    BOOL  bIODone;

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    if (_stricmp (hIOPort->TPCB_DeviceType, DEVICETYPE_UNIMODEM) == 0) {

        // Send Buffer to Port
        //
        hIOPort->TPCB_WriteOverlapped.hEvent = hAsyncEvent;

        bIODone = WriteFile(hIOPort->TPCB_CommHandle,
                            pBuffer,
                            dwSize,
                            &pdwBytesWritten,         //pdwBytesWritten is not used
                            &(hIOPort->TPCB_WriteOverlapped));

        if (bIODone)
            retcode = PENDING;

        else if ((retcode = GetLastError()) == ERROR_IO_PENDING)
            retcode = PENDING ;

    } else

        retcode = SUCCESS ;         // return SUCCESS for all non unimodem devices

    // **** Exclusion END ****
    FreeMutex (RasTapiMutex) ;

    return retcode ;
}



//*  PortReceive  ------------------------------------------------------------
//
// Function: This API reads from the port.  This API is
//           asynchronous and normally returns PENDING; however, if
//           ReadFile returns synchronously, the API will return
//           SUCCESS.
//
// Returns: SUCCESS
//          PENDING
//          Return code from GetLastError
//
//*

DWORD
PortReceive(HANDLE hPort,
            BYTE   *pBuffer,
            DWORD  dwSize,
            DWORD  dwTimeOut,
            HANDLE hAsyncEvent)
{
    TapiPortControlBlock *hIOPort = (TapiPortControlBlock *) hPort;
    COMMTIMEOUTS  CT;
    DWORD         pdwBytesRead;
    BOOL          bIODone;
    DWORD         retcode ;

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    if (_stricmp (hIOPort->TPCB_DeviceType, DEVICETYPE_UNIMODEM) == 0) {

        // Set Read Timeouts

        CT.ReadIntervalTimeout = 0;
        CT.ReadTotalTimeoutMultiplier = 0;
        CT.ReadTotalTimeoutConstant = dwTimeOut;

	if (!SetCommTimeouts(hIOPort->TPCB_CommHandle, &CT)) {
	    // **** Exclusion END ****
	    FreeMutex (RasTapiMutex) ;
	    return(GetLastError());
	}

        // Read from Port
        //
        hIOPort->TPCB_ReadOverlapped.hEvent = hAsyncEvent;

        bIODone = ReadFile(hIOPort->TPCB_CommHandle,
                           pBuffer,
                           dwSize,
                           &pdwBytesRead,               //pdwBytesRead is not used
                           &(hIOPort->TPCB_ReadOverlapped));

        if (bIODone)
            retcode = PENDING;

        else if ((retcode = GetLastError()) == ERROR_IO_PENDING)
            retcode = PENDING;

    } else

        retcode = SUCCESS ;         // return SUCCESS for all non unimodem devices

    // **** Exclusion END ****
    FreeMutex (RasTapiMutex) ;

    return retcode ;
}


//*  PortReceiveComplete ------------------------------------------------------
//
// Function: Completes a read  - if still PENDING it cancels it - else it returns the bytes read.
//           PortClearStatistics.
//
// Returns: SUCCESS
//
DWORD
PortReceiveComplete (HANDLE hPort, PDWORD bytesread)
{
    TapiPortControlBlock *hIOPort = (TapiPortControlBlock *) hPort;
    DWORD retcode ;

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    if (!GetOverlappedResult(hIOPort->TPCB_CommHandle,
			                 &(hIOPort->TPCB_ReadOverlapped),
			                 bytesread,
			                 FALSE)) {
        retcode = GetLastError() ;
	    PurgeComm (hIOPort->TPCB_CommHandle, PURGE_RXABORT) ;
	    *bytesread = 0 ;

    } else
        retcode = SUCCESS ;

    // **** Exclusion END ****
    FreeMutex (RasTapiMutex) ;

    return retcode ;
}



//*  PortCompressionSetInfo  -------------------------------------------------
//
// Function: This API selects Asyncmac compression mode by setting
//           Asyncmac's compression bits.
//
// Returns: SUCCESS
//          Return code from GetLastError
//
//*

DWORD
PortCompressionSetInfo(HANDLE hIOPort)
{
  return SUCCESS;
}



//*  PortClearStatistics  ----------------------------------------------------
//
// Function: This API is used to mark the beginning of the period for which
//           statistics will be reported.  The current numbers are copied
//           from the MAC and stored in the Serial Port Control Block.  At
//           the end of the period PortGetStatistics will be called to
//           compute the difference.
//
// Returns: SUCCESS
//          ERROR_PORT_NOT_OPEN
//*

DWORD
PortClearStatistics(HANDLE hIOPort)
{
  return SUCCESS;
}



//*  PortGetStatistics  ------------------------------------------------------
//
// Function: This API reports MAC statistics since the last call to
//           PortClearStatistics.
//
// Returns: SUCCESS
//          ERROR_PORT_NOT_OPEN
//*

DWORD
PortGetStatistics(HANDLE hIOPort, RAS_STATISTICS *pStat)
{

  return(SUCCESS);
}


//*  PortSetFraming	-------------------------------------------------------
//
// Function: Sets the framing type with the mac
//
// Returns: SUCCESS
//
//*
DWORD  APIENTRY
PortSetFraming(HANDLE hIOPort, DWORD SendFeatureBits, DWORD RecvFeatureBits,
	      DWORD SendBitMask, DWORD RecvBitMask)
{

    return(SUCCESS);
}



//*  PortGetPortState  -------------------------------------------------------
//
// Function: This API is used in MS-DOS only.
//
// Returns: SUCCESS
//
//*

DWORD  APIENTRY
PortGetPortState(char *pszPortName, DWORD *pdwUsage)
{
  return(SUCCESS);
}





//*  PortChangeCallback  -----------------------------------------------------
//
// Function: This API is used in MS-DOS only.
//
// Returns: SUCCESS
//
//*

DWORD  APIENTRY
PortChangeCallback(HANDLE hIOPort)
{
  return(SUCCESS);
}


//*  PortGetIOHandle()
//
// Function: For the given hIOPort this returns the file handle for the connection
//
// Returns: SUCCESS
//
//*
DWORD  APIENTRY
PortGetIOHandle(HANDLE hPort, HANDLE *FileHandle)
{
    DWORD retcode ;
    TapiPortControlBlock *hIOPort = (TapiPortControlBlock *) hPort;

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    if (hIOPort->TPCB_State == PS_CONNECTED) {
        *FileHandle = hIOPort->TPCB_CommHandle ;
        retcode = SUCCESS ;
    } else
        retcode = ERROR_PORT_NOT_OPEN ;

    // **** Exclusion Begin ****
    FreeMutex (RasTapiMutex) ;
    return retcode ;
}


//*  DeviceEnum()  -----------------------------------------------------------
//
// Function: Enumerates all devices in the device INF file for the
//           specified DevictType.
//
// Returns: Return codes from RasDevEnumDevices
//
//*

DWORD APIENTRY
DeviceEnum (char  *pszDeviceType,
            WORD  *pcEntries,
            BYTE  *pBuffer,
            WORD  *pwSize)
{
    *pwSize    = 0 ;
    *pcEntries = 0 ;

    return(SUCCESS);
}



//*  DeviceGetInfo()  --------------------------------------------------------
//
// Function: Returns a summary of current information from the InfoTable
//           for the device on the port in Pcb.
//
// Returns: Return codes from GetDeviceCB, BuildOutputTable
//*

DWORD APIENTRY
DeviceGetInfo(HANDLE hPort,
              char   *pszDeviceType,
              char   *pszDeviceName,
              BYTE   *pInfo,
              WORD   *pwSize)
{
    DWORD retcode ;
    TapiPortControlBlock *hIOPort = LookUpControlBlock(hPort);

    if (!hIOPort)
        return ERROR_PORT_NOT_FOUND ;

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    retcode = GetInfo (hIOPort, pInfo, pwSize) ;


    // **** Exclusion End ****
    FreeMutex (RasTapiMutex) ;

    return(retcode);
}



//*  DeviceSetInfo()  --------------------------------------------------------
//
// Function: Sets attributes in the InfoTable for the device on the
//           port in Pcb.
//
// Returns: Return codes from GetDeviceCB, UpdateInfoTable
//*

DWORD APIENTRY
DeviceSetInfo(HANDLE		hPort,
              char              *pszDeviceType,
              char              *pszDeviceName,
              RASMAN_DEVICEINFO *pInfo)
{
    DWORD retcode ;
    TapiPortControlBlock *hIOPort = LookUpControlBlock(hPort);

    if (!hIOPort)
        return ERROR_PORT_NOT_FOUND ;


    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    retcode = SetInfo (hIOPort, (RASMAN_PORTINFO*) pInfo) ;

    // **** Exclusion End ****
    FreeMutex (RasTapiMutex) ;

    return (retcode);
}



//*  DeviceConnect()  --------------------------------------------------------
//
// Function: Initiates the process of connecting a device.
//
// Returns: Return codes from ConnectListen
//*

DWORD APIENTRY
DeviceConnect(HANDLE hPort,
              char   *pszDeviceType,
              char   *pszDeviceName,
              HANDLE hNotifier)
{
    LINECALLPARAMS *linecallparams ;
    LPVARSTRING var ;
    BYTE	   buffer [2000] ;
    BYTE	   *nextstring ;
    TapiPortControlBlock *hIOPort = LookUpControlBlock(hPort);

    if (!hIOPort)
        return ERROR_PORT_NOT_FOUND ;


    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    // if dev config has been set for this device we should call down and set it.
    //
    if ((hIOPort->TPCB_DevConfig) && (_stricmp (hIOPort->TPCB_DeviceType, DEVICETYPE_UNIMODEM) == 0)) {

        // Before the write this - save away the current setting for the device.
        //
        var = (LPVARSTRING)buffer ;
        var->dwTotalSize  = 2000 ;
        var->dwStringSize = 0 ;
        lineGetDevConfig (hIOPort->TPCB_Line->TLI_LineId, var, "comm/datamodem") ;

        // Alloc mem for the returned info
        //
        hIOPort->TPCB_DefaultDevConfig = LocalAlloc (LPTR, var->dwStringSize) ;
        hIOPort->TPCB_DefaultDevConfigSize = var->dwStringSize ;
        memcpy (hIOPort->TPCB_DefaultDevConfig, (CHAR*)var+var->dwStringOffset, var->dwStringSize) ;

        lineSetDevConfig (hIOPort->TPCB_Line->TLI_LineId, hIOPort->TPCB_DevConfig, hIOPort->TPCB_SizeOfDevConfig, "comm/datamodem") ;

    }

    memset (buffer, 0, sizeof(buffer)) ;
    linecallparams = (LINECALLPARAMS *) buffer ;
    nextstring = (buffer + sizeof (LINECALLPARAMS)) ;
    linecallparams->dwTotalSize = sizeof(buffer) ;

    strcpy (nextstring, hIOPort->TPCB_Address) ;
    linecallparams->dwOrigAddressSize = strlen (nextstring) ;
    linecallparams->dwOrigAddressOffset = (nextstring - buffer) ;

    linecallparams->dwAddressMode = LINEADDRESSMODE_DIALABLEADDR ;

    nextstring += linecallparams->dwOrigAddressSize ;

    if (_stricmp (hIOPort->TPCB_DeviceType, DEVICETYPE_ISDN) == 0)
	    SetIsdnParams (hIOPort, linecallparams) ;

    else if (_stricmp (hIOPort->TPCB_DeviceType, DEVICETYPE_X25) == 0) {

	    if (*hIOPort->TPCB_Info[X25_USERDATA_INDEX] != '\0') {

	        strcpy (nextstring, hIOPort->TPCB_Info[X25_USERDATA_INDEX]) ;
	        linecallparams->dwUserUserInfoSize	 = strlen (nextstring) ;
	        linecallparams->dwUserUserInfoOffset = (nextstring - buffer) ;
	        nextstring += linecallparams->dwUserUserInfoSize ;

	    }

	    if (*hIOPort->TPCB_Info[X25_FACILITIES_INDEX] != '\0') {

	        strcpy (nextstring, hIOPort->TPCB_Info[X25_FACILITIES_INDEX]) ;
	        linecallparams->dwDevSpecificSize	 = strlen (nextstring) ;
	        linecallparams->dwDevSpecificOffset = (nextstring - buffer) ;
	        nextstring += linecallparams->dwDevSpecificSize ;
	    }

	    // Diagnostic key is ignored.

    } else if (_stricmp (hIOPort->TPCB_DeviceType, DEVICETYPE_UNIMODEM) == 0) {

	    SetModemParams (hIOPort, linecallparams) ;

    }


    hIOPort->TPCB_RequestId = INFINITE ;     // mark request id as unused
    hIOPort->TPCB_CallHandle = (HCALL) INFINITE ;	// set call handle to bogus value
    hIOPort->TPCB_AsyncErrorCode = SUCCESS ; // initialize

    if ((hIOPort->TPCB_RequestId =
	    lineMakeCall (hIOPort->TPCB_Line->TLI_LineHandle, &hIOPort->TPCB_CallHandle, hIOPort->TPCB_Info[ADDRESS_INDEX], 0, linecallparams)) > 0x80000000 ) {

	// **** Exclusion End ****
	FreeMutex (RasTapiMutex) ;

	// DbgPrint ("RASTAPI: lineMakeCall failed -> returned %x\n", hIOPort->TPCB_RequestId) ;

	if (hIOPort->TPCB_RequestId == LINEERR_INUSE)
	    return ERROR_PORT_NOT_AVAILABLE ;

	return ERROR_FROM_DEVICE ;

    }

    ResetEvent (hNotifier) ;

    hIOPort->TPCB_ReqNotificationHandle = hNotifier ;

    hIOPort->TPCB_State = PS_CONNECTING ;

    hIOPort->TPCB_DisconnectReason = 0 ;

    // **** Exclusion End ****
    FreeMutex (RasTapiMutex) ;

    return (PENDING);
}


//*
//
//
//
//
//*
VOID
SetIsdnParams (TapiPortControlBlock *hIOPort, LINECALLPARAMS *linecallparams)
{
    WORD    numchannels ;
    WORD    fallback ;

    // Line type
    //
    if (_stricmp (hIOPort->TPCB_Info[ISDN_LINETYPE_INDEX], ISDN_LINETYPE_STRING_64DATA) == 0) {
	linecallparams->dwBearerMode = LINEBEARERMODE_DATA ;
	linecallparams->dwMinRate = 64000 ;
	linecallparams->dwMaxRate = 64000 ;
	linecallparams->dwMediaMode = LINEMEDIAMODE_DIGITALDATA ;

    } else if (_stricmp (hIOPort->TPCB_Info[ISDN_LINETYPE_INDEX], ISDN_LINETYPE_STRING_56DATA) == 0) {
	linecallparams->dwBearerMode = LINEBEARERMODE_DATA ;
	linecallparams->dwMinRate = 56000 ;
	linecallparams->dwMaxRate = 56000 ;
	linecallparams->dwMediaMode = LINEMEDIAMODE_DIGITALDATA ;

    } else if (_stricmp (hIOPort->TPCB_Info[ISDN_LINETYPE_INDEX], ISDN_LINETYPE_STRING_56VOICE) == 0) {
	linecallparams->dwBearerMode = LINEBEARERMODE_VOICE ;
	linecallparams->dwMinRate = 56000 ;
	linecallparams->dwMaxRate = 56000 ;
	linecallparams->dwMediaMode = LINEMEDIAMODE_UNKNOWN ;
    } else {  // default
	linecallparams->dwBearerMode = LINEBEARERMODE_DATA ;
	linecallparams->dwMinRate = 64000 ;
	linecallparams->dwMaxRate = 64000 ;
	linecallparams->dwMediaMode = LINEMEDIAMODE_DIGITALDATA ;
    }

    if (hIOPort->TPCB_Info[ISDN_CHANNEL_AGG_INDEX][0] != '\0')
	numchannels = atoi(hIOPort->TPCB_Info[ISDN_CHANNEL_AGG_INDEX]) ;
    else
	numchannels = 1 ; // default

    if (hIOPort->TPCB_Info[ISDN_FALLBACK_INDEX] != '\0')
	fallback = atoi(hIOPort->TPCB_Info[ISDN_FALLBACK_INDEX]) ;
    else
	fallback = 1 ;	  // default

    if (fallback)
	linecallparams->dwMinRate = 56000 ; // always allow the min
    else
	linecallparams->dwMinRate = numchannels * linecallparams->dwMaxRate ;

    linecallparams->dwMaxRate = numchannels * linecallparams->dwMaxRate ;

}


//*
//
//
//
//
//*
VOID
SetModemParams (TapiPortControlBlock *hIOPort, LINECALLPARAMS *linecallparams)
{
    WORD    numchannels ;
    WORD    fallback ;
    BYTE    buffer[800] ;
    LINEDEVCAPS     *linedevcaps ;

	memset (buffer, 0, sizeof(buffer)) ;

	linedevcaps = (LINEDEVCAPS *)buffer ;
	linedevcaps->dwTotalSize = sizeof(buffer) ;

	// Get a count of all addresses across all lines
	//
	if (lineGetDevCaps (RasLine, hIOPort->TPCB_Line->TLI_LineId,  hIOPort->TPCB_Line->NegotiatedApiVersion, hIOPort->TPCB_Line->NegotiatedExtVersion, linedevcaps))
        linecallparams->dwBearerMode = LINEBEARERMODE_VOICE ;   // in case of failure try the common case - modems

    if (linedevcaps->dwBearerModes & LINEBEARERMODE_VOICE)
        linecallparams->dwBearerMode = LINEBEARERMODE_VOICE ;
    else
        linecallparams->dwBearerMode = LINEBEARERMODE_DATA ;

    //
    // do not dial without dialtone
    //
    linecallparams->dwCallParamFlags |= LINECALLPARAMFLAGS_IDLE ;

    linecallparams->dwMinRate = 2400 ;
    linecallparams->dwMaxRate = 115200 ;
    linecallparams->dwMediaMode = LINEMEDIAMODE_DATAMODEM ;
}


//*  DeviceListen()  ---------------------------------------------------------
//
// Function: Initiates the process of listening for a remote device
//           to connect to a local device.
//
// Returns: Return codes from ConnectListen
//*

DWORD APIENTRY
DeviceListen(HANDLE hPort,
             char   *pszDeviceType,
             char   *pszDeviceName,
             HANDLE hNotifier)
{
    DWORD retcode ;
    TapiPortControlBlock *hIOPort = LookUpControlBlock(hPort);

    if (!hIOPort)
        return ERROR_PORT_NOT_FOUND ;

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    // DbgPrint ("DevListen, State = %d\n", hIOPort->TPCB_State) ;

    // If the state is DISCONNECTING (this could happen since rasman waits only 10 seconds
    // for the lower layers to complete a disconnect request), then we have no option but
    // to close and open the line.
    //
    if (hIOPort->TPCB_State == PS_DISCONNECTING) {

	// DbgPrint ("DevListen: Hit code path where device is still disconnecting\n") ;

        lineClose (hIOPort->TPCB_Line->TLI_LineHandle) ;

        Sleep (30L) ;   // allow a "reasonable" time to allow clean up.

        retcode = lineOpen (RasLine,
	                  hIOPort->TPCB_Line->TLI_LineId,
	                  &hIOPort->TPCB_Line->TLI_LineHandle,
	                  hIOPort->TPCB_Line->NegotiatedApiVersion,
	                  hIOPort->TPCB_Line->NegotiatedExtVersion,
	                  (ULONG) hIOPort->TPCB_Line,
	                  LINECALLPRIVILEGE_OWNER,
	                  hIOPort->TPCB_MediaMode,
	                  NULL) ;

	    if (retcode) {
	        // **** Exclusion End ****
	        FreeMutex (RasTapiMutex) ;
	    // DbgPrint ("DevListen: lineOpen failed with %d \n", retcode) ;
            return ERROR_FROM_DEVICE ;
	}

	//
	// Set monitoring of rings
	//
	lineSetStatusMessages (hIOPort->TPCB_Line->TLI_LineHandle, LINEDEVSTATE_RINGING, 0) ;
    }

    if (hIOPort->TPCB_Line->TLI_LineState != PS_LISTENING)
	    hIOPort->TPCB_Line->TLI_LineState = PS_LISTENING ;

    hIOPort->TPCB_State = PS_LISTENING ;
    hIOPort->TPCB_ListenState = LS_WAIT ;
    hIOPort->TPCB_DisconnectReason = 0 ;

    ResetEvent (hNotifier) ;

    hIOPort->TPCB_ReqNotificationHandle = hNotifier ;

    hIOPort->TPCB_CallHandle = INVALID_HANDLE_VALUE ;

    // **** Exclusion End ****
    FreeMutex (RasTapiMutex) ;

    return (PENDING);
}



//*  DeviceDone()  -----------------------------------------------------------
//
// Function: Informs the device dll that the attempt to connect or listen
//           has completed.
//
// Returns: nothing
//*

VOID APIENTRY
DeviceDone(HANDLE hPort)
{
    TapiPortControlBlock *hIOPort = LookUpControlBlock(hPort);

    if (!hIOPort)
        return ;

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    hIOPort->TPCB_ReqNotificationHandle = NULL ; // no more needed.

    // **** Exclusion End ****
    FreeMutex (RasTapiMutex) ;

}



//*  DeviceWork()  -----------------------------------------------------------
//
// Function: This function is called following DeviceConnect or
//           DeviceListen to further the asynchronous process of
//           connecting or listening.
//
// Returns: ERROR_DCB_NOT_FOUND
//          ERROR_STATE_MACHINES_NOT_STARTED
//          Return codes from DeviceStateMachine
//*

DWORD APIENTRY
DeviceWork(HANDLE hPort,
           HANDLE hNotifier)
{
    LINECALLSTATUS *callstatus ;
    BYTE	   buffer [1000] ;
    DWORD      retcode = ERROR_FROM_DEVICE ;
    TapiPortControlBlock *hIOPort = LookUpControlBlock(hPort);

    if (!hIOPort)
        return ERROR_PORT_NOT_FOUND ;

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    memset (buffer, 0, sizeof(buffer)) ;

    callstatus = (LINECALLSTATUS *)buffer ;
    callstatus->dwTotalSize = sizeof(buffer) ;

    if (hIOPort->TPCB_State == PS_CONNECTING) {

	    if (hIOPort->TPCB_AsyncErrorCode != SUCCESS) {

		retcode = hIOPort->TPCB_AsyncErrorCode ;
		hIOPort->TPCB_AsyncErrorCode = SUCCESS ;

	    } else if (lineGetCallStatus (hIOPort->TPCB_CallHandle, callstatus))
	        retcode =  ERROR_FROM_DEVICE ;

	    else if (callstatus->dwCallState == LINECALLSTATE_CONNECTED) {
	        hIOPort->TPCB_State = PS_CONNECTED ;
	        retcode =  SUCCESS ;

	    } else if (callstatus->dwCallState == LINECALLSTATE_DISCONNECTED) {
	        retcode = ERROR_FROM_DEVICE ;
	        if (callstatus->dwCallStateMode == LINEDISCONNECTMODE_BUSY)
	            retcode = ERROR_LINE_BUSY ;
	        else if (callstatus->dwCallStateMode == LINEDISCONNECTMODE_NOANSWER)
	            retcode = ERROR_NO_ANSWER ;
            else if (callstatus->dwCallStateMode == LINEDISCONNECTMODE_CANCELLED)
                retcode = ERROR_USER_DISCONNECTION;

	    } else if ((callstatus->dwCallState == LINECALLSTATE_SPECIALINFO) &&
	        	   (callstatus->dwCallStateMode == LINESPECIALINFO_NOCIRCUIT)) {
	            retcode = ERROR_NO_ACTIVE_ISDN_LINES ;
	    }
    }

    if (hIOPort->TPCB_State == PS_LISTENING) {

	    if (hIOPort->TPCB_ListenState == LS_ERROR)
	        retcode = ERROR_FROM_DEVICE ;

	    else if (hIOPort->TPCB_ListenState == LS_ACCEPT) {
	        hIOPort->TPCB_RequestId = lineAccept (hIOPort->TPCB_CallHandle, NULL, 0) ;
	        if (hIOPort->TPCB_RequestId > 0x80000000 ) // ERROR or SUCCESS
	    	    hIOPort->TPCB_ListenState = LS_ANSWER ;
	        else if (hIOPort->TPCB_RequestId == 0)
	    	    hIOPort->TPCB_ListenState = LS_ANSWER ;

	        retcode = PENDING ;
	    }

	    if (hIOPort->TPCB_ListenState == LS_ANSWER) {
	        hIOPort->TPCB_RequestId = lineAnswer (hIOPort->TPCB_CallHandle, NULL, 0) ;
	        if (hIOPort->TPCB_RequestId > 0x80000000 )
	    	    retcode = ERROR_FROM_DEVICE ;
	        else if (hIOPort->TPCB_RequestId)
	    	    retcode = PENDING ;
	        else  // SUCCESS
	    	    hIOPort->TPCB_ListenState = LS_COMPLETE ;
	    }

	    if (hIOPort->TPCB_ListenState == LS_COMPLETE) {

	        if (hIOPort->TPCB_CallHandle == INVALID_HANDLE_VALUE) {

	         	retcode = ERROR_FROM_DEVICE ;

	        } else {

		        hIOPort->TPCB_State = PS_CONNECTED ;
		        retcode = SUCCESS ; //
	        }
        }

    }

    // If we have connected, then get the com port handle for use in terminal modem i/o
    //
    if (hIOPort->TPCB_State == PS_CONNECTED) {

        VARSTRING	*varstring ;
        BYTE	    buffer [100] ;

        // get the cookie to realize tapi and ndis endpoints
        //
        varstring = (VARSTRING *) buffer ;
        varstring->dwTotalSize = sizeof(buffer) ;

        // Unimodem/asyncmac linegetid returns a comm port handle. Other medias give the endpoint itself back in linegetid
        // This has to do with the fact that modems/asyncmac are not a miniport.
        //
        if (_stricmp (hIOPort->TPCB_DeviceType, DEVICETYPE_UNIMODEM) == 0) {

	        if (lineGetID (hIOPort->TPCB_Line->TLI_LineHandle, hIOPort->TPCB_AddressId, hIOPort->TPCB_CallHandle, LINECALLSELECT_CALL, varstring, "comm/datamodem")) {
		        // **** Exclusion End ****
		        FreeMutex (RasTapiMutex) ;
		        return ERROR_FROM_DEVICE ;
	        }

	        hIOPort->TPCB_CommHandle =	(HANDLE) (*((DWORD *) ((BYTE *)varstring+varstring->dwStringOffset))) ;

            // Initialize the port for approp. buffers
            //
            SetupComm (hIOPort->TPCB_CommHandle, 1514, 1514) ;

	    } else {

	        if (lineGetID (hIOPort->TPCB_Line->TLI_LineHandle, hIOPort->TPCB_AddressId, hIOPort->TPCB_CallHandle, LINECALLSELECT_CALL, varstring, "NDIS")) {
	    	    // **** Exclusion End ****
	    	    FreeMutex (RasTapiMutex) ;
	    	    return ERROR_FROM_DEVICE ;
	        }

            hIOPort->TPCB_Endpoint = *((DWORD *) ((BYTE *)varstring+varstring->dwStringOffset)) ;
        }

	// DbgPrint ("L\n") ;
    }

    if (retcode == PENDING)
	    ResetEvent (hNotifier) ;

    // **** Exclusion End ****
    FreeMutex (RasTapiMutex) ;
    return(retcode);
}


//* DeviceSetDevConfig()
//
//  Function: Called to set an opaque blob of data to configure a device.
//
//  Returns:  LocalAlloc returned values.
//
DWORD
DeviceSetDevConfig (HANDLE hPort, PBYTE devconfig, DWORD sizeofdevconfig)
{
    TapiPortControlBlock *hIOPort = LookUpControlBlock(hPort);

    if (!hIOPort)
        return ERROR_PORT_NOT_FOUND ;

    if (_stricmp (hIOPort->TPCB_DeviceType, DEVICETYPE_UNIMODEM))
        return SUCCESS ;

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    if (hIOPort->TPCB_DevConfig != NULL)
        LocalFree (hIOPort->TPCB_DevConfig) ;

    if ((hIOPort->TPCB_DevConfig = LocalAlloc(LPTR, sizeofdevconfig)) == NULL) {

        // **** Exclusion End ****
        FreeMutex (RasTapiMutex) ;
        return(GetLastError());
    }

    memcpy (hIOPort->TPCB_DevConfig, devconfig, sizeofdevconfig) ;
    hIOPort->TPCB_SizeOfDevConfig = sizeofdevconfig ;

    // **** Exclusion End ****
    FreeMutex (RasTapiMutex) ;
    return (SUCCESS);
}


//* DeviceGetDevConfig()
//
//  Function: Called to set an opaque blob of data to configure a device.
//
//  Returns:  LocalAlloc returned values.
//
DWORD
DeviceGetDevConfig (char *name, PBYTE devconfig, DWORD *sizeofdevconfig)
{
    TapiPortControlBlock *hIOPort = NULL;
    DWORD i ;
    BYTE buffer[2000] ;
    LPVARSTRING var ;
    PBYTE configptr ;
    DWORD configsize ;
    DWORD retcode ;

    // hIOPort or pszPortName must be valid:
    //
    for (i=0; i < TotalPorts; i++) {
	    if (!_stricmp(RasPorts[i].TPCB_Name, name)) {
	        hIOPort = (HANDLE) &RasPorts[i] ;
	        break ;
	    }
    }

    if (!hIOPort)
        return ERROR_PORT_NOT_FOUND ;

    if (_stricmp (hIOPort->TPCB_DeviceType, DEVICETYPE_UNIMODEM)) {
        *sizeofdevconfig = 0 ;
        return SUCCESS ;
    }

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    if (hIOPort->TPCB_DevConfig != NULL) {

        configptr  = hIOPort->TPCB_DevConfig ;
        configsize = hIOPort->TPCB_SizeOfDevConfig ;

    } else {

        // Make var string
        //
        var = (LPVARSTRING)buffer ;
        var->dwTotalSize  = 2000 ;
        var->dwStringSize = 0 ;
        lineGetDevConfig (hIOPort->TPCB_Line->TLI_LineId, var, "comm/datamodem") ;
        configptr  = ((CHAR *)var + var->dwStringOffset) ;
        configsize = var->dwStringSize  ;
    }

    if (*sizeofdevconfig > configsize) {
        memcpy (devconfig, configptr, configsize) ;
        retcode = SUCCESS ;
    } else
        retcode = ERROR_BUFFER_TOO_SMALL ;

    *sizeofdevconfig = configsize ;

    // **** Exclusion End ****
    FreeMutex (RasTapiMutex) ;
    return (retcode);
}


//*
//
//
//
//*
DWORD
GetInfo (TapiPortControlBlock *hIOPort, BYTE *pBuffer, WORD *pwSize)
{
    if (_stricmp (hIOPort->TPCB_DeviceType, DEVICETYPE_ISDN) == 0)
	GetIsdnParams (hIOPort, (RASMAN_PORTINFO *) pBuffer, pwSize) ;
    else if (_stricmp (hIOPort->TPCB_DeviceType, DEVICETYPE_X25) == 0)
	GetX25Params (hIOPort, (RASMAN_PORTINFO *) pBuffer, pwSize) ;
    else
	GetGenericParams (hIOPort, (RASMAN_PORTINFO *) pBuffer, pwSize) ;

    return SUCCESS ;
}


//* SetInfo()
//
//
//
//*
DWORD
SetInfo (TapiPortControlBlock *hIOPort, RASMAN_PORTINFO *pBuffer)
{

    if (_stricmp (hIOPort->TPCB_DeviceType, DEVICETYPE_UNIMODEM) == 0)
	FillInUnimodemParams (hIOPort, pBuffer) ;
    else if (_stricmp (hIOPort->TPCB_DeviceType, DEVICETYPE_ISDN) == 0)
	FillInIsdnParams (hIOPort, pBuffer) ;
    else if (_stricmp (hIOPort->TPCB_DeviceType, DEVICETYPE_X25) == 0)
	FillInX25Params (hIOPort, pBuffer) ;
    else
	FillInGenericParams (hIOPort, pBuffer) ;

    return SUCCESS ;
}


//* FillInUnimodemParams()
//
//  Function:	We do more than fill in the params if the params are ones that are required to be set
//		right then.
//
//  Returns:	ERROR_WRONG_INFO_SPECIFIED.
//		Comm related Win32 errors
//		SUCCESS.
//*
DWORD
FillInUnimodemParams (TapiPortControlBlock *hIOPort, RASMAN_PORTINFO *pInfo)
{
    RAS_PARAMS *p;
    WORD	i;
    DWORD	index = 0xfefefefe ;
    DCB 	DCB ;
#define INITIALIZED_VALUE  0xde
    BYTE	DCBByteSize = INITIALIZED_VALUE ;
    BYTE	DCBParity   = INITIALIZED_VALUE ;
    BYTE	DCBStopBits = INITIALIZED_VALUE ;
    BOOL	DCBProcessingRequired = FALSE ;

    for (i=0, p=pInfo->PI_Params; i<pInfo->PI_NumOfParams; i++, p++) {

	if (_stricmp(p->P_Key, SER_DATABITS_KEY) == 0) {
	    DCBByteSize = (BYTE) ValueToNum(p);
	    DCBProcessingRequired = TRUE ;
	} else if (_stricmp(p->P_Key, SER_PARITY_KEY) == 0) {
	    DCBParity = (BYTE) ValueToNum(p);
	    DCBProcessingRequired = TRUE ;
	} else if (_stricmp(p->P_Key, SER_STOPBITS_KEY) == 0) {
	    DCBStopBits = (BYTE) ValueToNum(p);
	    DCBProcessingRequired = TRUE ;
	}

	//
	// The fact we use ISDN_PHONENUMBER_KEY is not a bug. This is just a define.
	//
	else if (_stricmp(p->P_Key, ISDN_PHONENUMBER_KEY) == 0)
	    index = ADDRESS_INDEX ;
	else if (_stricmp(p->P_Key, CONNECTBPS_KEY) == 0)
	    index = CONNECTBPS_INDEX ;
	else
	    return(ERROR_WRONG_INFO_SPECIFIED);

    if (index != 0xfefefefe) {
	    strncpy (hIOPort->TPCB_Info[index], p->P_Value.String.Data, p->P_Value.String.Length);
	    hIOPort->TPCB_Info[index][p->P_Value.String.Length] = '\0' ;
    }
    }


    //
    // For parameters that should be set right away - check that the port handle is still valid
    // if so set the parameters.
    //
    if (DCBProcessingRequired && hIOPort->TPCB_CommHandle != INVALID_HANDLE_VALUE) {

	//
	// Get a Device Control Block with current port values
	//
	if (!GetCommState(hIOPort->TPCB_CommHandle, &DCB))
	    return(GetLastError());

	if (DCBByteSize != INITIALIZED_VALUE)
	    DCB.ByteSize = DCBByteSize ;
	if (DCBParity	!= INITIALIZED_VALUE)
	    DCB.Parity	 = DCBParity ;
	if (DCBStopBits != INITIALIZED_VALUE)
	    DCB.StopBits = DCBStopBits ;

	//
	// Send DCB to Port
	//
	if (!SetCommState(hIOPort->TPCB_CommHandle, &DCB))
	    return(GetLastError());

    }

    return SUCCESS ;
}


//* FillInIsdnParams()
//
//
//
//*
DWORD
FillInIsdnParams (TapiPortControlBlock *hIOPort, RASMAN_PORTINFO *pInfo)
{
    RAS_PARAMS *p;
    WORD	i;
    DWORD	index ;

    for (i=0, p=pInfo->PI_Params; i<pInfo->PI_NumOfParams; i++, p++) {

	if (_stricmp(p->P_Key, ISDN_LINETYPE_KEY) == 0)
	    index = ISDN_LINETYPE_INDEX ;

	else if (_stricmp(p->P_Key, ISDN_FALLBACK_KEY) == 0)
	    index = ISDN_FALLBACK_INDEX ;

	else if (_stricmp(p->P_Key, ISDN_COMPRESSION_KEY) == 0)
	    index = ISDN_COMPRESSION_INDEX ;

	else if (_stricmp(p->P_Key, ISDN_CHANNEL_AGG_KEY) == 0)
	    index = ISDN_CHANNEL_AGG_INDEX ;

	else if (_stricmp(p->P_Key, ISDN_PHONENUMBER_KEY) == 0)
	    index = ADDRESS_INDEX ;

	else if (_stricmp(p->P_Key, CONNECTBPS_KEY) == 0)
	    index = ISDN_CONNECTBPS_INDEX ;

	else
	    return(ERROR_WRONG_INFO_SPECIFIED);

	strncpy (hIOPort->TPCB_Info[index], p->P_Value.String.Data, p->P_Value.String.Length);
	hIOPort->TPCB_Info[index][p->P_Value.String.Length] = '\0' ;
    }

    strcpy (hIOPort->TPCB_Info[ISDN_CONNECTBPS_INDEX], "64000") ; // initialize connectbps to a
							       // reasonable default

    return SUCCESS ;
}

//*
//
//
//
//*
DWORD
FillInX25Params (TapiPortControlBlock *hIOPort, RASMAN_PORTINFO *pInfo)
{
    RAS_PARAMS *p;
    WORD	i;
    DWORD	index ;

    for (i=0, p=pInfo->PI_Params; i<pInfo->PI_NumOfParams; i++, p++) {

	if (_stricmp(p->P_Key, MXS_DIAGNOSTICS_KEY) == 0)
	    index = X25_DIAGNOSTICS_INDEX ;

	else if (_stricmp(p->P_Key, MXS_USERDATA_KEY) == 0)
	    index = X25_USERDATA_INDEX ;

	else if (_stricmp(p->P_Key, MXS_FACILITIES_KEY) == 0)
	    index = X25_FACILITIES_INDEX;

	else if (_stricmp(p->P_Key, MXS_X25ADDRESS_KEY) == 0)
	    index = ADDRESS_INDEX ;

	else if (_stricmp(p->P_Key, CONNECTBPS_KEY) == 0)
	    index = X25_CONNECTBPS_INDEX ;
	else
	    return(ERROR_WRONG_INFO_SPECIFIED);

	strncpy (hIOPort->TPCB_Info[index], p->P_Value.String.Data, p->P_Value.String.Length);
	hIOPort->TPCB_Info[index][p->P_Value.String.Length] = '\0' ;
    }

    strcpy (hIOPort->TPCB_Info[X25_CONNECTBPS_INDEX], "9600") ; // initialize connectbps to a
							      // reasonable default

    return SUCCESS ;
}




//*
//
//
//
//*
DWORD
FillInGenericParams (TapiPortControlBlock *hIOPort, RASMAN_PORTINFO *pInfo)
{
    RAS_PARAMS *p;
    WORD	i;
    DWORD	index ;

    for (i=0, p=pInfo->PI_Params; i<pInfo->PI_NumOfParams; i++, p++) {

	if (_stricmp(p->P_Key, ISDN_PHONENUMBER_KEY) == 0)
	    index = ADDRESS_INDEX ;
	else if (_stricmp(p->P_Key, CONNECTBPS_KEY) == 0)
	    index = CONNECTBPS_INDEX ;
	else
	    return(ERROR_WRONG_INFO_SPECIFIED);

	strncpy (hIOPort->TPCB_Info[index], p->P_Value.String.Data, p->P_Value.String.Length);
	hIOPort->TPCB_Info[index][p->P_Value.String.Length] = '\0' ;
    }

    return SUCCESS ;
}



//*
//
//
//
//*
DWORD
GetGenericParams (TapiPortControlBlock *hIOPort, RASMAN_PORTINFO *pBuffer , PWORD pwSize)
{
    RAS_PARAMS	*pParam;
    CHAR	*pValue;
    WORD	wAvailable ;
    DWORD dwStructSize = sizeof(RASMAN_PORTINFO) + sizeof(RAS_PARAMS) * 2;

    wAvailable = *pwSize;
    *pwSize = (WORD) (dwStructSize + strlen (hIOPort->TPCB_Info[ADDRESS_INDEX])
				   + strlen (hIOPort->TPCB_Info[CONNECTBPS_INDEX])
				   + 1L) ;

    if (*pwSize > wAvailable)
      return(ERROR_BUFFER_TOO_SMALL);

    // Fill in Buffer

    ((RASMAN_PORTINFO *)pBuffer)->PI_NumOfParams = 2;

    pParam = ((RASMAN_PORTINFO *)pBuffer)->PI_Params;
    pValue = (CHAR*)pBuffer + dwStructSize;

    strcpy(pParam->P_Key, MXS_PHONENUMBER_KEY);
    pParam->P_Type = String;
    pParam->P_Attributes = 0;
    pParam->P_Value.String.Length = strlen (hIOPort->TPCB_Info[ADDRESS_INDEX]);
    pParam->P_Value.String.Data = pValue;
    strcpy(pParam->P_Value.String.Data, hIOPort->TPCB_Info[ADDRESS_INDEX]);
    pParam++;

    strcpy(pParam->P_Key, CONNECTBPS_KEY);
    pParam->P_Type = String;
    pParam->P_Attributes = 0;
    pParam->P_Value.String.Length = strlen (hIOPort->TPCB_Info[ISDN_CONNECTBPS_INDEX]);
    pParam->P_Value.String.Data = pValue;
    strcpy(pParam->P_Value.String.Data, hIOPort->TPCB_Info[ISDN_CONNECTBPS_INDEX]);

    return(SUCCESS);
}



//*
//
//
//
//*
DWORD
GetIsdnParams (TapiPortControlBlock *hIOPort, RASMAN_PORTINFO *pBuffer , PWORD pwSize)
{
    RAS_PARAMS	*pParam;
    CHAR	*pValue;
    WORD	wAvailable ;
    DWORD dwStructSize = sizeof(RASMAN_PORTINFO) + sizeof(RAS_PARAMS) * 5;

    wAvailable = *pwSize;
    *pwSize = (WORD) (dwStructSize + strlen (hIOPort->TPCB_Info[ADDRESS_INDEX])
				   + strlen (hIOPort->TPCB_Info[ISDN_LINETYPE_INDEX])
				   + strlen (hIOPort->TPCB_Info[ISDN_FALLBACK_INDEX])
				   + strlen (hIOPort->TPCB_Info[ISDN_COMPRESSION_INDEX])
				   + strlen (hIOPort->TPCB_Info[ISDN_CHANNEL_AGG_INDEX])
				   + strlen (hIOPort->TPCB_Info[ISDN_CONNECTBPS_INDEX])
				   + 1L) ;

    if (*pwSize > wAvailable)
      return(ERROR_BUFFER_TOO_SMALL);

    // Fill in Buffer

    ((RASMAN_PORTINFO *)pBuffer)->PI_NumOfParams = 6;

    pParam = ((RASMAN_PORTINFO *)pBuffer)->PI_Params;
    pValue = (CHAR*)pBuffer + dwStructSize;


    strcpy(pParam->P_Key, ISDN_PHONENUMBER_KEY);
    pParam->P_Type = String;
    pParam->P_Attributes = 0;
    pParam->P_Value.String.Length = strlen (hIOPort->TPCB_Info[ADDRESS_INDEX]);
    pParam->P_Value.String.Data = pValue;
    strcpy(pParam->P_Value.String.Data, hIOPort->TPCB_Info[ADDRESS_INDEX]);
    pValue += pParam->P_Value.String.Length + 1;
    pParam++;


    strcpy(pParam->P_Key, ISDN_LINETYPE_KEY);
    pParam->P_Type = String;
    pParam->P_Attributes = 0;
    pParam->P_Value.String.Length = strlen (hIOPort->TPCB_Info[ISDN_LINETYPE_INDEX]);
    pParam->P_Value.String.Data = pValue;
    strcpy(pParam->P_Value.String.Data, hIOPort->TPCB_Info[ISDN_LINETYPE_INDEX]);
    pValue += pParam->P_Value.String.Length + 1;
    pParam++;


    strcpy(pParam->P_Key, ISDN_FALLBACK_KEY);
    pParam->P_Type = String;
    pParam->P_Attributes = 0;
    pParam->P_Value.String.Length = strlen (hIOPort->TPCB_Info[ISDN_FALLBACK_INDEX]);
    pParam->P_Value.String.Data = pValue;
    strcpy(pParam->P_Value.String.Data, hIOPort->TPCB_Info[ISDN_FALLBACK_INDEX]);
    pValue += pParam->P_Value.String.Length + 1;
    pParam++;


    strcpy(pParam->P_Key, ISDN_COMPRESSION_KEY);
    pParam->P_Type = String;
    pParam->P_Attributes = 0;
    pParam->P_Value.String.Length = strlen (hIOPort->TPCB_Info[ISDN_COMPRESSION_INDEX]);
    pParam->P_Value.String.Data = pValue;
    strcpy(pParam->P_Value.String.Data, hIOPort->TPCB_Info[ISDN_COMPRESSION_INDEX]);
    pValue += pParam->P_Value.String.Length + 1;
    pParam++;


    strcpy(pParam->P_Key, ISDN_CHANNEL_AGG_KEY);
    pParam->P_Type = String;
    pParam->P_Attributes = 0;
    pParam->P_Value.String.Length = strlen (hIOPort->TPCB_Info[ISDN_CHANNEL_AGG_INDEX]);
    pParam->P_Value.String.Data = pValue;
    strcpy(pParam->P_Value.String.Data, hIOPort->TPCB_Info[ISDN_CHANNEL_AGG_INDEX]);
    pValue += pParam->P_Value.String.Length + 1;
    pParam++;

    strcpy(pParam->P_Key, CONNECTBPS_KEY);
    pParam->P_Type = String;
    pParam->P_Attributes = 0;
    pParam->P_Value.String.Length = strlen (hIOPort->TPCB_Info[ISDN_CONNECTBPS_INDEX]);
    pParam->P_Value.String.Data = pValue;
    strcpy(pParam->P_Value.String.Data, hIOPort->TPCB_Info[ISDN_CONNECTBPS_INDEX]);


    return(SUCCESS);
}



//*
//
//
//
//*
DWORD
GetX25Params (TapiPortControlBlock *hIOPort, RASMAN_PORTINFO *pBuffer ,PWORD pwSize)
{
    RAS_PARAMS	*pParam;
    CHAR	*pValue;
    WORD	wAvailable ;
    DWORD dwStructSize = sizeof(RASMAN_PORTINFO) + sizeof(RAS_PARAMS) * 4 ;

    wAvailable = *pwSize;
    *pwSize = (WORD) (dwStructSize + strlen (hIOPort->TPCB_Info[ADDRESS_INDEX])
				   + strlen (hIOPort->TPCB_Info[X25_DIAGNOSTICS_INDEX])
				   + strlen (hIOPort->TPCB_Info[X25_USERDATA_INDEX])
				   + strlen (hIOPort->TPCB_Info[X25_FACILITIES_INDEX])
				   + strlen (hIOPort->TPCB_Info[X25_CONNECTBPS_INDEX])
				   + 1L) ;

    if (*pwSize > wAvailable)
      return(ERROR_BUFFER_TOO_SMALL);

    // Fill in Buffer

    ((RASMAN_PORTINFO *)pBuffer)->PI_NumOfParams = 5 ;

    pParam = ((RASMAN_PORTINFO *)pBuffer)->PI_Params;
    pValue = (CHAR*)pBuffer + dwStructSize;

    strcpy(pParam->P_Key, MXS_X25ADDRESS_KEY);
    pParam->P_Type = String;
    pParam->P_Attributes = 0;
    pParam->P_Value.String.Length = strlen (hIOPort->TPCB_Info[ADDRESS_INDEX]);
    pParam->P_Value.String.Data = pValue;
    strcpy(pParam->P_Value.String.Data, hIOPort->TPCB_Info[ADDRESS_INDEX]);
    pValue += pParam->P_Value.String.Length + 1;
	pParam++;

    strcpy(pParam->P_Key, MXS_DIAGNOSTICS_KEY);
    pParam->P_Type = String;
    pParam->P_Attributes = 0;
    pParam->P_Value.String.Length = strlen (hIOPort->TPCB_Info[X25_DIAGNOSTICS_INDEX]);
    pParam->P_Value.String.Data = pValue;
    strcpy(pParam->P_Value.String.Data, hIOPort->TPCB_Info[X25_DIAGNOSTICS_INDEX]);
    pValue += pParam->P_Value.String.Length + 1;
	pParam++;

    strcpy(pParam->P_Key, MXS_USERDATA_KEY);
    pParam->P_Type = String;
    pParam->P_Attributes = 0;
    pParam->P_Value.String.Length = strlen (hIOPort->TPCB_Info[X25_USERDATA_INDEX]);
    pParam->P_Value.String.Data = pValue;
    strcpy(pParam->P_Value.String.Data, hIOPort->TPCB_Info[X25_USERDATA_INDEX]);
    pValue += pParam->P_Value.String.Length + 1;
    pParam++;

    strcpy(pParam->P_Key, MXS_FACILITIES_KEY);
    pParam->P_Type = String;
    pParam->P_Attributes = 0;
    pParam->P_Value.String.Length = strlen (hIOPort->TPCB_Info[X25_FACILITIES_INDEX]);
    pParam->P_Value.String.Data = pValue;
    strcpy(pParam->P_Value.String.Data, hIOPort->TPCB_Info[X25_FACILITIES_INDEX]);
    pValue += pParam->P_Value.String.Length + 1;
    pParam++;

    strcpy(pParam->P_Key, CONNECTBPS_KEY);
    pParam->P_Type = String;
    pParam->P_Attributes = 0;
    pParam->P_Value.String.Length = strlen (hIOPort->TPCB_Info[X25_CONNECTBPS_INDEX]);
    pParam->P_Value.String.Data = pValue;
    strcpy(pParam->P_Value.String.Data, hIOPort->TPCB_Info[X25_CONNECTBPS_INDEX]);


    return(SUCCESS);
}


//* GetMutex
//
//
//
//*
VOID
GetMutex (HANDLE mutex, DWORD to)
{
    if (WaitForSingleObject (mutex, to) == WAIT_FAILED) {
	GetLastError() ;
	DbgBreakPoint() ;
    }
}



//* FreeMutex
//
//
//
//*
VOID
FreeMutex (HANDLE mutex)
{
    if (!ReleaseMutex(mutex)) {
	GetLastError () ;
	DbgBreakPoint() ;
    }
}


//* InitiatePortDisconnection()
//
// Function: Starts the disconnect process. Note even though this covers SYNC completion of lineDrop this
//           is not per TAPI spec.
//
// Returns:
//*
DWORD
InitiatePortDisconnection (TapiPortControlBlock *hIOPort)
{
    DWORD retcode ;

    hIOPort->TPCB_RequestId = INFINITE ; // mark requestid as unused

    // For asyncmac/unimodem give a close indication to asyncmac if the endpoint is still valid
    //
    if (_stricmp (hIOPort->TPCB_DeviceType, DEVICETYPE_UNIMODEM) == 0) {

        // tell asyncmac to close the link
        //
        if (hIOPort->TPCB_Endpoint != 0xffffffff) {

            ASYMAC_CLOSE  AsyMacClose;
            OVERLAPPED overlapped ;
            DWORD       dwBytesReturned ;

            memset (&overlapped, 0, sizeof(OVERLAPPED)) ;

            AsyMacClose.MacAdapter = NULL;
            AsyMacClose.hNdisEndpoint = (HANDLE) hIOPort->TPCB_Endpoint ;

            DeviceIoControl(ghAsyMac,
                      IOCTL_ASYMAC_CLOSE,
                      &AsyMacClose,
                      sizeof(AsyMacClose),
                      &AsyMacClose,
                      sizeof(AsyMacClose),
                      &dwBytesReturned,
	                  &overlapped);

            hIOPort->TPCB_Endpoint = 0xffffffff ;

        }

        // Close the handle given by lineGetId on unimodem ports
	    //
        if (hIOPort->TPCB_CommHandle != INVALID_HANDLE_VALUE) {
            CloseHandle (hIOPort->TPCB_CommHandle) ;
            hIOPort->TPCB_CommHandle = INVALID_HANDLE_VALUE ;
        }
    }

    // Handle the case where lineMakeCall is not yet complete and the callhandle is invalid
    //
    if (hIOPort->TPCB_CallHandle == (HCALL) INFINITE) {

        lineClose (hIOPort->TPCB_Line->TLI_LineHandle) ;

        Sleep (30L) ;   // arbitrary sleep time to allow cleanup in lower layers

        retcode = lineOpen (RasLine,
	                  hIOPort->TPCB_Line->TLI_LineId,
	                  &hIOPort->TPCB_Line->TLI_LineHandle,
	                  hIOPort->TPCB_Line->NegotiatedApiVersion,
	                  hIOPort->TPCB_Line->NegotiatedExtVersion,
	                  (ULONG) hIOPort->TPCB_Line,
	                  LINECALLPRIVILEGE_OWNER,
	                  hIOPort->TPCB_MediaMode,
	                  NULL) ;

        if (retcode)
	     DbgPrint ("InitiateDisconnection: lineOpen failed with %d\n", retcode) ;

	//
	// Set monitoring of rings
	//
	lineSetStatusMessages (hIOPort->TPCB_Line->TLI_LineHandle, LINEDEVSTATE_RINGING, 0) ;

        return SUCCESS ;
    }


    // Initiate disconnection.
    //
    if ((hIOPort->TPCB_RequestId = lineDrop (hIOPort->TPCB_CallHandle, NULL, 0)) > 0x80000000 ) {

		//
		// Error issuing the linedrop.  Should we try to deallocate anyway?
		//
		hIOPort->TPCB_State = PS_OPEN ;
		hIOPort->TPCB_RequestId = INFINITE ;
		lineDeallocateCall (hIOPort->TPCB_CallHandle) ;

		// DbgPrint ("D\n") ;

		return ERROR_DISCONNECTION ; // generic disconnect message

	} else if (hIOPort->TPCB_RequestId) {

		//
		// The linedrop is completeing async
		//
		hIOPort->TPCB_State = PS_DISCONNECTING ;

		// DbgPrint ("InitiatePortDisconnection: ReqId:%d\n", hIOPort->TPCB_RequestId) ;

		return PENDING ;

	} else { // SUCCESS

		//
		// The linedrop completed sync
		//
		hIOPort->TPCB_RequestId = INFINITE ;
		if (hIOPort->TPCB_Line->IdleReceived) {

			hIOPort->TPCB_Line->IdleReceived = FALSE;
			hIOPort->TPCB_State = PS_OPEN ;
			lineDeallocateCall (hIOPort->TPCB_CallHandle) ;

			 // DbgPrint ("D\n") ;

			hIOPort->TPCB_CallHandle = (HCALL) 0xffffffff ;
		     return SUCCESS ;

		} else {

		    //
		    // Wait for IdleReceived
		    //
		    hIOPort->TPCB_State = PS_DISCONNECTING ;
		    return PENDING ;
        }
    }

}


// LookUpControlBlock()
//
// Function: This function uses the given handle to find which TPCB is it refering to. This handle can be
//           either a pointer to TPCB itself (in case of non unimodem devices) or it is the CommHandle
//           for the unimodem port.
//
// Consider: Adding a cache for lookup speeding.
//
// Returns:  Nothing.
//
TapiPortControlBlock *
LookUpControlBlock (HANDLE hPort)
{
    DWORD i ;
    TapiPortControlBlock *pports ;

    // hPort is the TPCB pointer
    //
    if (((TapiPortControlBlock *)hPort >= RasPorts)    &&
        ((TapiPortControlBlock *)hPort < RasPortsEnd)  &&
        (((TapiPortControlBlock *)hPort)->TPCB_Signature == CONTROLBLOCKSIGNATURE))
        return (TapiPortControlBlock *)hPort ;

    // hPort is not the TPCB pointer - see if this matches any of the CommHandles
    //
    for (pports = RasPorts, i=0; i < TotalPorts; i++, pports++) {
        if (pports->TPCB_CommHandle == hPort)
            return pports ;
    }

    return NULL ;
}


//*  ValueToNum  -------------------------------------------------------------
//
// Function: Converts a RAS_PARAMS P_Value, which may be either a DWORD or
//           a string, to a DWORD.
//
// Returns: The numeric value of the input as a DWORD.
//
//*

DWORD
ValueToNum(RAS_PARAMS *p)
{
    CHAR szStr[RAS_MAXLINEBUFLEN];


    if (p->P_Type == String) {

	strncpy(szStr, p->P_Value.String.Data, p->P_Value.String.Length);
	szStr[p->P_Value.String.Length] = '\0';
	return(atol(szStr));

    } else

	return(p->P_Value.Number);
}




/*

// MapRasParamsForUnimodem()
//
// Function:    Given RASMAN_DEVICEINFO structure and a Unimodem PDEVCFG structure this routine
//              first sanitizes all values in DEVCFG so that they are suitable for a RAS connection
//              and then sets all the relevant fields in it that are specified in RASMAN_DEVICEINFO.
//
// Returns:     Nothing.
//
VOID
MapRasParamsForUnimodem (RASMAN_DEVICEINFO *rasinfo, PDEVCFG uniconfig)
{
    DWORD i ;
    DWORD number ;
    MODEMSETTINGS *modemsettings = uniconfig->commconfig + uniconfig->commconfig.wcProviderOffset ;

    // Sanitize settings for RAS
    //
    uniconfig->dwVersion = 0x00010003 ;                     // version for which this struct is valid.
    uniconfig->fwOptions = 0 ;                              // no pre/post connect terminal.
    uniconfig->wWaitBong = 1 ;                              // wait for 2 seconds for the credit card bong.
    uniconfig->commconfig.dcb.BaudRate          = 9600 ;    // initial default.
    uniconfig->commconfig.dcb.fBinary           = TRUE ;
    uniconfig->commconfig.dcb.fParity           = TRUE ;
    uniconfig->commconfig.dcb.fOutxCtsFlow      = FALSE ;
    uniconfig->commconfig.dcb.fOutxDsrFlow      = FALSE ;
    uniconfig->commconfig.dcb.fDtrControl       = DTR_CONTROL_DISABLE ;
    uniconfig->commconfig.dcb.fDsrSensitivity   = TRUE ;
    uniconfig->commconfig.dcb.fTxContinueOnXoff = FALSE ;
    uniconfig->commconfig.dcb.fOutX             = FALSE ;
    uniconfig->commconfig.dcb.fInX              = FALSE ;
    uniconfig->commconfig.dcb.fErrorChar        = FALSE ;
    uniconfig->commconfig.dcb.fNull             = FALSE ;
    uniconfig->commconfig.dcb.fRtsControl       = RTS_CONTROL_DISABLE ;
    uniconfig->commconfig.dcb.fAbortOnError     = FALSE ;
    uniconfig->commconfig.dcb.wReserved         = 0 ;
    uniconfig->commconfig.dcb.XonLim            = 0 ;
    uniconfig->commconfig.dcb.XoffLim           = 0 ;
    uniconfig->commconfig.dcb.ByteSize          = 8 ;
    uniconfig->commconfig.dcb.Parity            = NOPARITY ;
    uniconfig->commconfig.dcb.StopBits          = ONESTOPBIT ;
    uniconfig->commconfig.dcb.XonChar           = 0 ;
    uniconfig->commconfig.dcb.XoffChar          = 0 ;
    uniconfig->commconfig.dcb.ErrorChar         = 0 ;
    uniconfig->commconfig.dcb.EofChar           = 0 ;
    uniconfig->commconfig.dcb.EvtChar           = 0 ;
    modemsettings->dwCallSetupFailTimer         = 55 ;              // 55 seconds to wait for call to establish.
    modemsettings->dwInactivityTimeout          = 0xefffffff ;      // should this be 0?
    modemsettings->dwSpeakerVolume              = MDMVOL_MEDIUM ;   // modem should be a medium volume initially.
    modemsettings->dwSpeakerMode                = MDMSPKR_ON ;      // turn modem on by default.

    // Now update any values that are specified in RAS Params
    //
    for (i=0; i < rasinfo->DI_NumOfParams; i++) {

        if (!_stricmp(rasinfo->DI_Params[i].P_Key,  MXS_SPEAKER_KEY)) {

            number = GetValue (&rasinfo->DI_Params[i]) ;

            if (number) {

                modemsettings->dwSpeakerMode = MDMSPKR_ON ;
                modemsettings->dwSpeakerVolume = number >> 1 ; // This is funky but the levels are the possible values shifted over.

            else {

                modemsettings->dwSpeakerMode = MDMSPKR_OFF ;
            }
        }

        else if (!_stricmp(rasinfo->DI_Params[i].P_Key,  MXS_HDWFLOWCONTROL_KEY)) {

            number = GetValue (&rasinfo->DI_Params[i]) ;

            if (number) {
                uniconfig->commconfig.dcb.fOutxCtsFlow = TRUE ;
                modemsettings->dwPreferredModemOptions |= MDM_FLOWCONTROL_HARD ;
            } else
                modemsettings->dwPreferredModemOptions & ~MDM_FLOWCONTROL_HARD ;
        }

        else if (!_stricmp(rasinfo->DI_Params[i].P_Key,  MXS_PROTOCOL_KEY)) {

            number = GetValue (&rasinfo->DI_Params[i]) ;

            if (number)
                modemsettings->dwPreferredModemOptions |= MDM_ERROR_CONTROL ;
            else
                modemsettings->dwPreferredModemOptions & ~MDM_ERROR_CONTROL ;
        }

        else if (!_stricmp(rasinfo->DI_Params[i].P_Key,  MXS_COMPRESSION_KEY)) {

            number = GetValue (&rasinfo->DI_Params[i]) ;

            if (number)
                modemsettings->dwPreferredModemOptions |= MDM_COMPRESSION ;
            else
                modemsettings->dwPreferredModemOptions & ~MDM_COMPRESSION ;
        }

        else if (!_stricmp(rasinfo->DI_Params[i].P_Key,  MXS_CONNECTBPS_KEY)) {

            number = GetValue (&rasinfo->DI_Params[i]) ;

            uniconfig->commconfig.dcb.BaudRate = number ;
        }

    }

}


//* GetValue()
//
// Function: Reads the Data portion of RAS_PARAMS struct and returns it. This may involve
//           conversion from a string to a numeric value.
//
// Returns:  Number (DWORD)
//
DWORD
GetValue (RAS_PARAMS *param)
{
    DWORD number ;
    CHAR  numberstring[20] ;

    if (param->P_Type == String) {

      memcpy(numberstring, param->P_Value.String.Data, rasinfo->DI_Params[i].P_Value.String.Length);

      numberstring[param->P_Value.String.Length] = '\0';

      number = atoi(numberstring);

    }  else {

      number = param->P_Value.Number;

    }

    return number ;
}

    struct UNIMODEMINFO {

        BOOLEAN     HwFlowControl ;

        BOOLEAN     ErrorControl ;

        BOOLEAN     Compression ;

        DWORD       ConnectBps ;

        DWORD       ModemVolumeLevels ;

    } ;

    typedef struct UNIMODEMINFO UNIMODEMINFO ;



//* GetUnimodemParamsForRas()
//
//  Function: Gets ras relevant info from unimodem DEVCFG structure
//
//  Returns:  Nothing.
//*
VOID
GetUnimodemParamsForRas (PDEVCFG uniconfig, UNIMODEMINFO *uinfo)
{
    MODEMSETTINGS *modemsettings = uniconfig->commconfig + uniconfig->commconfig.wcProviderOffset ;

    uinfo->ModemVolumeLevels = modemsettings->dwSpeakerVolume ; // FLAGS:  MDMVOLFLAG_LOW (0x00000001) |  MDMVOLFLAG_MEDIUM (0x00000002) | MDMVOLFLAG_HIGH (0x00000004)

    uinfo->HwFlowControl = (modemsettings->dwPreferredModemOptions & MDM_FLOWCONTROL_HARD ? TRUE : FALSE) ;

    uinfo->Compression   = (modemsettings->dwPreferredModemOptions & MDM_COMPRESSION ? TRUE : FALSE) ;

    uinfo->ErrorControl  = (modemsettings->dwPreferredModemOptions & MDM_ERROR_CONTROL ? TRUE : FALSE) ;

    uinfo->ConnectBps    = uniconfig->commconfig.dcb.BaudRate ;
}

*/
