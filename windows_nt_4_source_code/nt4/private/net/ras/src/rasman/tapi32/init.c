//****************************************************************************
//
//                     Microsoft NT Remote Access Service
//
//	Copyright (C) 1994-95 Microsft Corporation. All rights reserved.
//
//  Filename: init.c
//
//  Revision History
//
//  Mar  28 1992   Gurdeep Singh Pall	Created
//
//  Description: This file contains init code for TAPI.DLL
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
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include "rastapi.h"


HLINEAPP	RasLine = 0 ;
HINSTANCE	RasInstance = 0 ;
TapiLineInfo	*RasTapiLineInfo ;
DWORD		TotalLines = 0 ;
DWORD		TotalPorts ;
TapiPortControlBlock *RasPorts ;
TapiPortControlBlock *RasPortsEnd ;
//DWORD		NegotiatedApiVersion ;
//DWORD		NegotiatedExtVersion ;
HANDLE		RasTapiMutex ;
BOOL		Initialized = FALSE ;
DWORD       TapiThreadId    ;
HANDLE      TapiThreadHandle;
DWORD       LoaderThreadId;
DWORD	    ValidPorts = 0;
DWORD	    NumberOfRings = 1 ;

HANDLE      ghAsyMac = (HANDLE)0xffffffff ;

//DWORD  EnumerateTapiPorts () ;
DWORD  ReadUsageInfoFromRegistry() ;
TapiLineInfo *FindLineByHandle (HLINE) ;
TapiPortControlBlock *FindPortByRequestId (DWORD) ;
TapiPortControlBlock *FindPortByAddressId (TapiLineInfo *, DWORD) ;
TapiPortControlBlock *FindPortByAddress   (CHAR *) ;
DWORD InitiatePortDisconnection (TapiPortControlBlock *hIOPort) ;
TapiPortControlBlock *FindPortByAddressAndName (CHAR *address, CHAR *name) ;

//* InitTapi()
//
//
//*
BOOL
InitRasTapi (HANDLE hInst, DWORD ul_reason_being_called, LPVOID lpReserved)
{
    STARTUPINFO        startupinfo ;
    DWORD dwErr;
    static BOOLEAN DllInitialized = FALSE ;

    switch (ul_reason_being_called) {

	case DLL_PROCESS_ATTACH:

	if (RasPorts != 0)
	    return 1 ;

	RasInstance = hInst ;

	if (ReadUsageInfoFromRegistry())
	    return 0 ;

	if ((RasTapiMutex = CreateMutex (NULL, FALSE, NULL)) == NULL)
	    return 0 ;

	DllInitialized = TRUE ;

	break ;

    case DLL_PROCESS_DETACH:

	//
	// If DLL did not successfully initialize for this process dont try to clean up
	//
	if (!DllInitialized)
	    break ;

        lineShutdown (RasLine) ;
        PostThreadMessage (TapiThreadId, WM_QUIT, 0, 0) ;
	    break ;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        //
        // If the thread that has created the TAPI
        // message queue thread via EnumerateTapiPorts
        // is exiting, also clean up the TAPI message
        // queue thread at this time.
        //
        // NOTE: We cannot do an explicit WaitForSingleObject()
        // on the TAPI message queue thread because we are
        // in a DLL init proc.  This would cause a deadlock,
        // since the exiting thread has to execute this DLL
        // init proc before exiting.  The TAPI message queue
        // thread actually takes care of the final DLL unload.
        // See EnumerateTapiPorts below.
        //
        if (GetCurrentThreadId() == LoaderThreadId)
           PostThreadMessage (TapiThreadId, WM_QUIT, 0, 0) ;

	break;

    }

    return 1 ;
}



//* EnumerateTapiPorts()
//
//  Function: First we call line initialize and construct a TLI for each line
//	      Then for each line we enumerate addresses and go through each address
//	      If the address is configured to be used with RAS we fill in the
//	      approp. info into the TPCB for the address (now port).
//
//  Return:   GetLastError(), SUCCESS
//
//*
DWORD
EnumerateTapiPorts (HANDLE event)
{
    WORD	    i, k ;
    TapiLineInfo    *nextline ;
    DWORD	    lines = 0 ;
    BYTE	    buffer[800] ;
    LINEADDRESSCAPS *lineaddrcaps ;
    LINEDEVCAPS	    *linedevcaps ;
    CHAR	    address[40] ;
    CHAR	    devicetype[MAX_DEVICETYPE_NAME] = {0};
    DWORD	    devicetypelength;
    CHAR	    devicename[MAX_DEVICE_NAME] = {0};
    DWORD	    devicenamelength;
    CHAR	    szregkey[128];
    LINEEXTENSIONID extensionid ;
    DWORD	    totaladdresses ;
    TapiPortControlBlock *nextport ;
    MSG msg ;
    HINSTANCE hInst;
    TapiPortControlBlock *pports ;
    LINEINITIALIZEEXPARAMS param ;
    DWORD	    version = HIGH_VERSION ;

    memset (&param, 0, sizeof (LINEINITIALIZEEXPARAMS)) ;
    param.dwOptions   = LINEINITIALIZEEXOPTION_USEHIDDENWINDOW ;
    param.dwTotalSize = sizeof(param) ;

    if (lineInitializeEx (&RasLine, RasInstance, (LINECALLBACK) RasTapiCallback, REMOTEACCESS_APP, &lines, &version, &param))
	    goto error ;

	if (lines == 0) {
		goto error;
	}

    nextline = RasTapiLineInfo = LocalAlloc (LPTR, sizeof (TapiLineInfo) * lines) ;

    if (nextline == NULL)
	    goto error ;

    TotalLines = lines ;

    for (i=0; i<lines; i++) {  // for all lines get the addresses -> ports

		if (lineNegotiateAPIVersion(RasLine, i, LOW_VERSION, HIGH_VERSION, &nextline->NegotiatedApiVersion, &extensionid) ) {
			nextline->TLI_LineState = PS_UNINITIALIZED ;
			nextline++ ;
			continue ;
		}

		if (lineNegotiateExtVersion(RasLine,
			                        i,
									nextline->NegotiatedApiVersion,
									LOW_EXT_VERSION,
									HIGH_EXT_VERSION,
			                        &nextline->NegotiatedExtVersion)) {
			nextline->NegotiatedExtVersion = 0;		
		}

		memset (buffer, 0, sizeof(buffer)) ;

		linedevcaps = (LINEDEVCAPS *)buffer ;
		linedevcaps->dwTotalSize = sizeof (buffer) ;

		// Get a count of all addresses across all lines
		//
		if (lineGetDevCaps (RasLine, i, nextline->NegotiatedApiVersion, nextline->NegotiatedExtVersion, linedevcaps)) {
			nextline->TLI_LineState = PS_UNINITIALIZED ;
			nextline++ ;
			continue ;
		}

		nextline->TLI_LineId = i ; // fill TLI struct. id.
		nextline->TLI_LineState	= PS_CLOSED ;

		nextline++ ;
    }

    // Now that we know the number of lines and number of addresses per line
    // we now fillin the TPCB structure per address
    //
    for (i=0, nextline = RasTapiLineInfo; i<TotalLines ; i++, nextline++) {

		BOOL fModem = FALSE ;

		if (RasTapiLineInfo[i].TLI_LineState == PS_UNINITIALIZED)
			continue ;

		memset (buffer, 0, sizeof(buffer)) ;

		linedevcaps = (LINEDEVCAPS *)buffer ;
		linedevcaps->dwTotalSize = sizeof(buffer) ;

		// Get a count of all addresses across all lines
		//
		if (lineGetDevCaps (RasLine, i, nextline->NegotiatedApiVersion, nextline->NegotiatedExtVersion, linedevcaps))
		    goto error ;

		// Figure out if this is a unimodem device or not
		//
		if (nextline->NegotiatedApiVersion == HIGH_VERSION)  {

		    // first convert all nulls in the device class string to non nulls.
		    //
		    DWORD  j ;
		    char *temp ;

		    for (j=0, temp = (CHAR *)linedevcaps+linedevcaps->dwDeviceClassesOffset; j<linedevcaps->dwDeviceClassesSize; j++, temp++)
			if (*temp == '\0')
			    *temp = ' ' ;

		    // select only those devices that have comm/datamodem as a device class
		    //
		    if (strstr((CHAR *)linedevcaps+linedevcaps->dwDeviceClassesOffset, "comm/datamodem") != NULL) {

			DWORD stringlen = (linedevcaps->dwLineNameSize > MAX_DEVICE_NAME - 1 ? MAX_DEVICE_NAME - 1 : linedevcaps->dwLineNameSize) ;

			strcpy (devicetype, DEVICETYPE_UNIMODEM) ;
			strncpy (devicename, (CHAR *)linedevcaps+linedevcaps->dwLineNameOffset, stringlen) ;
			devicename[stringlen] = '\0' ;

			lstrcpynA(szregkey, (CHAR *)linedevcaps+linedevcaps->dwDevSpecificOffset+(2*sizeof(DWORD)), linedevcaps->dwDevSpecificSize);
			szregkey[linedevcaps->dwDevSpecificSize] = '\0';

			fModem = TRUE ;
		    }

		} else {

		    // Provider info is of the following format
		    //	<media name>\0<device name>\0
		    //	where - media name is - ISDN, SWITCH56, FRAMERELAY, etc.
		    //	      device name is  Digiboard PCIMAC, Cirel, Intel, etc.
		    //
		    // Since this format is used only by NDISWAN miniports this may not be present if the TSP
		    // is a non unimodem and a non NDISWAN miniport. The following code (carefully) tries to
		    // parse the
		    //
		    DWORD copylen ;
		    DWORD providerinfosize = linedevcaps->dwProviderInfoSize + 2;
		    BYTE* providerinfo = LocalAlloc (LPTR, providerinfosize) ;
		    memcpy (providerinfo, (CHAR *)linedevcaps+linedevcaps->dwProviderInfoOffset, linedevcaps->dwProviderInfoSize) ;
		    providerinfo[providerinfosize-1] = '\0' ;
		    providerinfo[providerinfosize-2] = '\0' ;

		    if (strlen (providerinfo) > MAX_DEVICETYPE_NAME) {

			//
			//  If this name is longer than specified for a devicetype name then this is not a
			//  wan miniport device. In this case copy the provider info into the devicename
			//
			copylen = (strlen(providerinfo) > MAX_DEVICE_NAME ? MAX_DEVICE_NAME : strlen(providerinfo)) ;

			strcpy (devicetype, "XXXX") ; // put in a dummy device type - this will get skipped.
			strncpy (devicename, providerinfo, copylen) ;

		    } else {

			//
			// treat this case as the the properly formatted name
			//
			strcpy (devicetype, providerinfo) ;
			copylen = (strlen(providerinfo+strlen(providerinfo)+1) > MAX_DEVICE_NAME ? MAX_DEVICE_NAME : strlen(providerinfo+strlen(providerinfo)+1)) ;
			strncpy (devicename, providerinfo+strlen(providerinfo)+1, copylen) ;
			devicename[copylen] = '\0' ;
		    }
		}

		totaladdresses = linedevcaps->dwNumAddresses ;

		for (k=0; k < totaladdresses; k++) {

		    if (!fModem) {

		    	memset (buffer, 0, sizeof(buffer)) ;

		    	lineaddrcaps = (LINEADDRESSCAPS*) buffer ;
		    	lineaddrcaps->dwTotalSize = sizeof (buffer) ;

		    	if (lineGetAddressCaps (RasLine, i, k, nextline->NegotiatedApiVersion, nextline->NegotiatedExtVersion, lineaddrcaps))
		    		goto error ;

		    	memcpy (address, (CHAR *)lineaddrcaps + lineaddrcaps->dwAddressOffset, sizeof (address) -1 ) ;

			if ((nextport = FindPortByAddress(address)) == NULL)
			    continue ; // this address not configured for remoteaccess

		    } else {

			GetAssociatedPortName (szregkey, address) ;

			if ((nextport = FindPortByAddressAndName(address, devicename)) == NULL)
			    continue ; // this address not configured for remoteaccess

		    }


		    // nextport is the TPCB for this address

		    nextport->TPCB_Line	 = &RasTapiLineInfo[i] ;
		    nextport->TPCB_State = PS_CLOSED ;
		    nextport->TPCB_Endpoint = 0xffffffff ;
		    nextport->TPCB_AddressId = k ;

		    // Copy over the devicetype and devicename
		    strcpy (nextport->TPCB_DeviceType, devicetype) ;

		    // For unimodem devices we need to fix up names
		    //
		    if (fModem) {

			    // Device Name is of the form "COM1: Hayes"
			    //
			    strcpy (nextport->TPCB_DeviceName, address) ;
			    strcpy (nextport->TPCB_DeviceName, ":") ;
			    strcpy (nextport->TPCB_DeviceName, devicename) ;

			    // also fix the port name to be the same as address "COM1"
			    //
			    strcpy (nextport->TPCB_Name, address) ;

		    } else {

			    if (devicename[0] != '\0')
			        strcpy (nextport->TPCB_DeviceName, devicename) ; // default

		    }
		}

    }

    //
    // Calculate the number of valid ports
    //
    for (pports = RasPorts, i=0; i < TotalPorts; i++, pports++) {
        if (pports->TPCB_State == PS_UNINITIALIZED)
            continue ;
        ValidPorts++;
    }

    //
    // Increase the reference count on our DLL
    // so it won't get unloaded out from under us.
    //
    hInst = LoadLibrary("rastapi.dll");

    // Notify the api that the initialization is done
    //
    SetEvent (event) ;

    //
    // If there are ports, then we hang around until
    // we are shut down.  Otherwise, we exit immediately.
    //
    if (ValidPorts) {
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg) ;
        }
    }

    lineShutdown (RasLine) ;

    //
    // The following call atomically unloads our
    // DLL and terminates this thread.
    //
    FreeLibraryAndExitThread(hInst, SUCCESS);

error:

	if (RasLine) {
		lineShutdown (RasLine) ;
	}

    RasLine = 0 ;

    SetEvent (event) ;

    return ((DWORD)-1) ;
}



//* ReadUsageInfoFromRegistry()
//
//
//
//
//
//*
DWORD
ReadUsageInfoFromRegistry()
{

    WORD    i ;
    DWORD   size ;
    DWORD   type ;
    PCHAR   addrvalue ;
    PCHAR   fnamevalue ;
    PCHAR   usagevalue ;
    HKEY    hkey ;
    HKEY    hsubkey ;
    CHAR    keyname [50] ;
    CHAR    mediatype [50] ;
    CHAR    provkeyname [200] ;
    PCHAR   nextaddrvalue ;
    PCHAR   nextfnamevalue ;
    PCHAR   nextusagevalue ;
    DWORD   nextkey ;
    TapiPortControlBlock *nextport ;


    if (RegOpenKey (HKEY_LOCAL_MACHINE, REGISTRY_RASMAN_TAPI_KEY, &hkey))
	return FALSE ;

    nextkey = 0 ;

    // Figure out the number of ports
    //
    while (RegEnumKey(hkey, nextkey, keyname, sizeof (keyname)) == ERROR_SUCCESS) {

	nextkey++ ;

	strcpy (provkeyname, REGISTRY_RASMAN_TAPI_KEY) ;
	strcat (provkeyname, "\\") ;
	strcat (provkeyname, keyname) ;

	if (RegOpenKey (HKEY_LOCAL_MACHINE, provkeyname, &hsubkey))
	    return FALSE ;

	size = 0 ;

	RegQueryValueEx (hsubkey, REGISTRY_ADDRESS, NULL, &type, NULL, &size) ;

	addrvalue = (PBYTE) LocalAlloc (LPTR, size+1) ;

	if (addrvalue == NULL)
	    return FALSE ;

	if (RegQueryValueEx (hsubkey, REGISTRY_ADDRESS, NULL, &type, (LPBYTE)addrvalue, &size))
	    return FALSE ;


	size = 0 ;

	RegQueryValueEx (hsubkey, REGISTRY_FRIENDLYNAME, NULL, &type, NULL, &size) ;

	fnamevalue = (PBYTE) LocalAlloc (LPTR, size+1) ;

	if (fnamevalue == NULL)
	    return FALSE ;

	if (RegQueryValueEx (hsubkey, REGISTRY_FRIENDLYNAME, NULL, &type, (LPBYTE)fnamevalue, &size))
	    return FALSE ;


	size = 0 ;

	RegQueryValueEx (hsubkey, REGISTRY_USAGE, NULL, &type, NULL, &size) ;

	usagevalue = (PBYTE) LocalAlloc (LPTR, size+1) ;

	if (usagevalue == NULL)
	    return FALSE ;

	if (RegQueryValueEx (hsubkey, REGISTRY_USAGE, NULL, &type, (LPBYTE)usagevalue, &size))
	    return FALSE ;

	nextaddrvalue  = addrvalue ;
	nextfnamevalue = fnamevalue ;
	nextusagevalue = usagevalue ;

	while (*nextaddrvalue && *nextfnamevalue && *nextusagevalue) {

	    nextaddrvalue += (strlen(nextaddrvalue) + 1) ;
	    nextfnamevalue+= (strlen(nextfnamevalue) + 1) ;
	    nextusagevalue+= (strlen(nextusagevalue) + 1) ;

	    TotalPorts++ ;
	}

	RegCloseKey (hsubkey) ;

    }

    // Allocate storage for the port structures
    //
    nextport = RasPorts = LocalAlloc (LPTR, sizeof (TapiPortControlBlock) * TotalPorts) ;

    // Store a pointer to the end of the allocated block - this is used for an optimization to
    // look up control blocks given a handle.
    //
    RasPortsEnd = RasPorts + (sizeof (TapiPortControlBlock) * TotalPorts) ;

    if (nextport == NULL)
	    return GetLastError() ;

    nextkey = 0 ;

    while (RegEnumKey(hkey, nextkey, keyname, sizeof (keyname)) == ERROR_SUCCESS) {

	nextkey++ ;

	strcpy (provkeyname, REGISTRY_RASMAN_TAPI_KEY) ;
	strcat (provkeyname, "\\") ;
	strcat (provkeyname, keyname) ;

	if (RegOpenKey (HKEY_LOCAL_MACHINE, provkeyname, &hsubkey))
	    return FALSE ;


//	size = sizeof (mediatype) ;
//
//	RegQueryValueEx (hsubkey, REGISTRY_MEDIATYPE, NULL, &type, mediatype, &size) ;


	size = 0 ;

	RegQueryValueEx (hsubkey, REGISTRY_ADDRESS, NULL, &type, NULL, &size) ;

	addrvalue = (PBYTE) LocalAlloc (LPTR, size+1) ;

	if (addrvalue == NULL)
	    return FALSE ;

	if (RegQueryValueEx (hsubkey, REGISTRY_ADDRESS, NULL, &type, (LPBYTE)addrvalue, &size))
	    return FALSE ;


	size = 0 ;

	RegQueryValueEx (hsubkey, REGISTRY_FRIENDLYNAME, NULL, &type, NULL, &size) ;

	fnamevalue = (PBYTE) LocalAlloc (LPTR, size+1) ;

	if (fnamevalue == NULL)
	    return FALSE ;

	if (RegQueryValueEx (hsubkey, REGISTRY_FRIENDLYNAME, NULL, &type, (LPBYTE)fnamevalue, &size))
	    return FALSE ;


	size = 0 ;

	RegQueryValueEx (hsubkey, REGISTRY_USAGE, NULL, &type, NULL, &size) ;

	usagevalue = (PBYTE) LocalAlloc (LPTR, size+1) ;

	if (usagevalue == NULL)
	    return FALSE ;

	if (RegQueryValueEx (hsubkey, REGISTRY_USAGE, NULL, &type, (LPBYTE)usagevalue, &size))
	    return FALSE ;

	nextaddrvalue  = addrvalue ;
	nextfnamevalue = fnamevalue ;
	nextusagevalue = usagevalue ;

	while (*nextaddrvalue && *nextfnamevalue && *nextusagevalue) {
        nextport->TPCB_Signature = CONTROLBLOCKSIGNATURE ;
	    nextport->TPCB_State     = PS_UNINITIALIZED ;
	    strncpy (nextport->TPCB_Address, nextaddrvalue, MAX_PORT_NAME -1 ) ;
	    nextport->TPCB_Address[MAX_PORT_NAME-1] = '\0' ;
	    strncpy (nextport->TPCB_Name, nextfnamevalue, MAX_PORT_NAME -1 ) ;
	    nextport->TPCB_Name[MAX_PORT_NAME-1] = '\0' ;
	    //strcpy (nextport->TPCB_DeviceType, mediatype) ;
	    strcpy (nextport->TPCB_DeviceName, "ISDN") ; // default
	    if (!_stricmp (nextusagevalue, CLIENT_USAGE))
		nextport->TPCB_Usage = CALL_OUT ;
	    else if (!_stricmp (nextusagevalue, SERVER_USAGE))
		nextport->TPCB_Usage = CALL_IN ;
	    else if (!_stricmp (nextusagevalue, CLIENTANDSERVER_USAGE))
		nextport->TPCB_Usage = CALL_IN_OUT ;

	    nextaddrvalue += (strlen(nextaddrvalue) + 1) ;
	    nextfnamevalue+= (strlen(nextfnamevalue) + 1) ;
	    nextusagevalue+= (strlen(nextusagevalue) + 1) ;

	    nextport++ ;
	}

	RegCloseKey (hsubkey) ;

    }

    RegCloseKey (hkey) ;

    //
    // Read the number of rings key from the registry
    //
    if (RegOpenKey (HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Services\\Rasman\\Parameters", &hkey))
	return FALSE ;

    size = sizeof(DWORD) ;
    if (RegQueryValueEx (hkey, "NumberOfRings", NULL, &type, (LPBYTE)&NumberOfRings, &size))
	NumberOfRings = 1 ;

    if ((NumberOfRings < 1) || (NumberOfRings > 20))
	NumberOfRings = 1 ;

    RegCloseKey (hkey) ;

    return SUCCESS ;
}


//* RasTapiCallback()
//
//  Function: Callback entrypoint for TAPI.
//
//  Returns:  Nothing.
//*
VOID FAR PASCAL
RasTapiCallback (HANDLE context, DWORD msg, DWORD instance, DWORD param1, DWORD param2, DWORD param3)
{
    LINECALLINFO    *linecallinfo ;
    BYTE	    buffer [1000] ;
    HCALL	    hcall ;
    HLINE	    linehandle ;
    TapiLineInfo    *line ;
    TapiPortControlBlock *port ;
    DWORD       i ;
    DWORD       retcode ;

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    // DbgPrint ("Msg: %d  Param1: %d\n", msg, param1) ;

    switch (msg) {

	case LINE_CALLSTATE:

	    hcall = (HCALL) context ;
	    line = (TapiLineInfo *) instance ;

	    // If line is closed dont bother
	    //
	    if (line->TLI_LineState == PS_CLOSED)
		break ;

	    memset (buffer, 0, sizeof(buffer)) ;
	    linecallinfo = (LINECALLINFO *) buffer ;
	    linecallinfo->dwTotalSize = sizeof(buffer) ;

	    // If line get call info fails return.
	    //
	    if (lineGetCallInfo (hcall, linecallinfo) > 0x80000000)
		break ;

	    // Locate the ras port for this call
	    //
	    if ((port = FindPortByAddressId (line, linecallinfo->dwAddressID)) == NULL) {

		// Did not find a ras port for the call. Ignore it.
		//
		break ;
	    }

	    // A new call is coming in
	    //
	    if (param1 == LINECALLSTATE_OFFERING) {

		if ((line->TLI_LineState == PS_LISTENING) && (port->TPCB_State == PS_LISTENING)) {

		    port->TPCB_CallHandle = hcall ;

		    //
		    // for unimodem devices wait for the specified number of rings
		    //
		    if (_stricmp (port->TPCB_DeviceType, DEVICETYPE_UNIMODEM) == 0) {

			//
			// call has already been answered by somebody else and is being offered to me
			//
			if (linecallinfo->dwCallStates == LINECALLSTATE_CONNECTED) {

			    port->TPCB_ListenState = LS_COMPLETE ;

			    //
			    // Complete event so that rasman calls DeviceWork to proceed the listen state machine.
			    //
			    SetEvent (port->TPCB_ReqNotificationHandle) ;

			} else {

			    port->TPCB_ListenState = LS_RINGING ;
			    port->TPCB_NumberOfRings = NumberOfRings ;

			}

		    } else {

			//
			// For other devices make transition to next listening state
			//
			port->TPCB_ListenState = LS_ACCEPT ;

			//
			// Complete event so that rasman calls DeviceWork to proceed the listen state machine.
			//
			SetEvent (port->TPCB_ReqNotificationHandle) ;

		    }

		} else {

		    //
		    // We were not expecting the call. Make transition to next listening state
		    //
		    port->TPCB_ListenState = LS_ERROR ;
		    port->TPCB_CallHandle = hcall ;

		    //
		    // not interested in call, drop it.
		    //
		    InitiatePortDisconnection (port) ;

		}

		break ;
	    }


	    // Call connected.
	    //
	    if (param1 == LINECALLSTATE_CONNECTED) {

		if (port->TPCB_State == PS_CONNECTING) {

		    //
		    // We were requesting the call. Complete event so that rasman calls DeviceWork() to complete the
		    // connection process.
		    //
		    SetEvent (port->TPCB_ReqNotificationHandle) ;

		} else {

		    port->TPCB_CallHandle = hcall ;

		    // This is a call we are asnwering. Now we can indicate to rasman that the call has come in.
		    //
		    // Setting listen state to LS_COMPLETE may be redundant but handles the case where the answer
		    // completes *after* the connection is indicated
		    //
		    port->TPCB_ListenState = LS_COMPLETE ;

		    //
		    // Complete event so that rasman knows of incoming call and calls devicework.
		    //
		    SetEvent (port->TPCB_ReqNotificationHandle) ;
		}

	    }

	    //
	    // Failure of sorts.
	    //
	    if ((param1 == LINECALLSTATE_BUSY) || (param1 == LINECALLSTATE_SPECIALINFO)) {

		// If we were connecting, notify rasman to call devicework so that the connection attempt can
		// be gracefully failed.
		//
		if (port->TPCB_State == PS_CONNECTING)
		    SetEvent (port->TPCB_ReqNotificationHandle) ;

		// BUGBUG: Can this be received for the answering case?
		//
	    }

	    //
	    // Disconnection happened
	    //
	    if (param1 == LINECALLSTATE_DISCONNECTED) {

		// If we were connecting, notify rasman to call devicework so that the connection attempt can
		// be gracefully failed.
		//
		if (port->TPCB_State == PS_CONNECTING) {

		    if (param2 == LINEDISCONNECTMODE_BUSY)
			port->TPCB_AsyncErrorCode = ERROR_LINE_BUSY ;
		    else if (param2 == LINEDISCONNECTMODE_NOANSWER)
			port->TPCB_AsyncErrorCode = ERROR_NO_ANSWER ;
		    else if (param2 == LINEDISCONNECTMODE_NODIALTONE)
			port->TPCB_AsyncErrorCode = ERROR_NO_DIALTONE ;
                    else if (param2 == LINEDISCONNECTMODE_CANCELLED)
                        port->TPCB_AsyncErrorCode = ERROR_USER_DISCONNECTION;

		    SetEvent (port->TPCB_ReqNotificationHandle) ;

		} else if (port->TPCB_State != PS_CLOSED) {

		    // If we were connected and got a disconnect notification then this could be hardware failure or
		    // a remote disconnection. Determine this and save the reason away.
		    //
		    if (port->TPCB_State == PS_CONNECTED) {
			LINECALLSTATUS *pcallstatus ;
			BYTE buffer[200] ;

			memset (buffer, 0, sizeof(buffer)) ;
			pcallstatus = (LINECALLSTATUS *) buffer ;
			pcallstatus->dwTotalSize = sizeof (buffer) ;
			lineGetCallStatus (port->TPCB_CallHandle, pcallstatus) ;
			if (pcallstatus->dwCallState == LINECALLSTATE_DISCONNECTED)
			    port->TPCB_DisconnectReason = SS_LINKDROPPED ;
			else
			    port->TPCB_DisconnectReason = SS_HARDWAREFAILURE ;

		    } else
			port->TPCB_DisconnectReason = 0 ;

		    //
		    // This means that we got a disconnect indication in one of the other states (listening, connected, etc.)
		    // We initiate our disconnect state machine.
		    //
		    if (InitiatePortDisconnection (port) != PENDING) {

			// Disconnection succeeded or failed. Both are end states for the disconnect state machine so notify
			// rasman that a disconnection has happened.
			//
			// DbgPrint("SignalDisc: 1\n") ;
			SetEvent (port->TPCB_DiscNotificationHandle) ;
		    }
		}
	    }

	    //
	    // A busy call state - our attempt to dialout failed
	    //
	    if (param1 == LINECALLSTATE_BUSY) {

		if (port->TPCB_State == PS_CONNECTING) {
		port->TPCB_AsyncErrorCode = ERROR_LINE_BUSY ;
		SetEvent (port->TPCB_ReqNotificationHandle) ;
		}
	    }


	    // Idle indication is useful to complete the disconnect state machine.
	    //
	    if (param1 == LINECALLSTATE_IDLE) {

		// DbgPrint ("I, State = %d\n", port->TPCB_State) ;

			if ((port->TPCB_State == PS_DISCONNECTING) && (port->TPCB_RequestId == INFINITE)) {

				// IDLE notification came after LineDrop Succeeded so safe to
				// deallocate the call
				//
				port->TPCB_State = PS_OPEN ;
				lineDeallocateCall (port->TPCB_CallHandle) ;
				// DbgPrint ("D + Signal Disc\n") ;
				port->TPCB_CallHandle = (HCALL) 0xffffffff ;
				line->IdleReceived = FALSE;
				SetEvent (port->TPCB_DiscNotificationHandle) ;
			} else {

				// We have not yet disconnected so do not deallocate call
				// yet.  This will be done when the disconnect completes.
				//
				line->IdleReceived = TRUE;
		    // DbgPrint ("IdleNoAction\n") ;
			}
	    }

	break ;


    case LINE_REPLY:

        // This message is sent to indicate completion of an asynchronous API.
        //
        // DbgPrint ("LineReply: ReqId:%d", param1) ;

	// Find for which port the async request succeeded. This is done by searching for pending
        // request id that is also provided in this message.
	//
	if ((port = FindPortByRequestId (param1)) == NULL) {

            // Not found. Ignore.
            //
            // DbgPrint ("\n") ;
	    break ;

        } else
            ;  // DbgPrint (" State = %d\n", port->TPCB_State) ;

        // Set request id to invalid.
        //
		port->TPCB_RequestId = INFINITE ;

		if (port->TPCB_State == PS_DISCONNECTING) {

		    // lineDrop completed. Note that we ignore the return code in param2. This is
		    // because we cant do anything else.
            //
			if (port->TPCB_Line->IdleReceived) {

				// We received IDLE notification before/during disconnect
				// so deallocate this call
				//
				port->TPCB_Line->IdleReceived = FALSE;
				port->TPCB_State = PS_OPEN ;
				lineDeallocateCall (port->TPCB_CallHandle) ;
				// DbgPrint ("D + SignalDisc\n") ;
				port->TPCB_CallHandle = (HCALL) 0xffffffff ;
                SetEvent (port->TPCB_DiscNotificationHandle) ;

			} else {

                // wait for idle message before signalling disconnect
                //
                ; // DbgPrint ("DropCompNoAction\n") ;
            }

			break ;
		}

        // Some other api completed
        //
		if (param2 == SUCCESS) {

			// Success means take no action - unless we are listening
			// in which case it means move to the next state - we simply
			// set the event that will result in a call to DeviceWork() to
			// make the actual call for the next state
			//
			if (port->TPCB_State != PS_LISTENING)
				break ;

			// Proceed to the next listening sub-state
			//
			if (port->TPCB_ListenState == LS_ACCEPT) {

				port->TPCB_ListenState =  LS_ANSWER ;
				SetEvent (port->TPCB_ReqNotificationHandle) ;

			} else if (port->TPCB_ListenState == LS_ANSWER) {

				port->TPCB_ListenState = LS_COMPLETE ;
			}


		} else {

			// For connecting and listening ports this means the attempt failed
			// because of some error
			//
			if (port->TPCB_State == PS_CONNECTING) {

				port->TPCB_AsyncErrorCode = ERROR_PORT_OR_DEVICE ;
				SetEvent (port->TPCB_ReqNotificationHandle) ;

			} else if (port->TPCB_State == PS_LISTENING) {

			    // Because ACCEPT may not be supported by the device - treat error as success
                //
				if (port->TPCB_ListenState == LS_ACCEPT)
					port->TPCB_ListenState =  LS_ANSWER ;
				else
					port->TPCB_ListenState =  LS_ERROR ;

				SetEvent (port->TPCB_ReqNotificationHandle) ;
			}

			// Some other API failed, we dont know and we dont care. Ignore.
			//
			else if (port->TPCB_State != PS_CLOSED) {

                ; // DbgPrint ("RASTAPI: LINE_REPLY failed. Ignored.\n") ;
            }
		}

		break ;

    case LINE_CLOSE:

        // Typically sent when things go really wrong.
        //
        // DbgPrint ("\nRASTAPI: received LINE_CLOSE message\n") ;

        // Find which line is indication came for.
        //
        line = (TapiLineInfo *) instance ;

        // if line not found or if it is closed just return
        //
        if ((line == NULL) || (line->TLI_LineState == PS_CLOSED))
            break ;

        // For every port that is on the line - open the line again and signal
        // hw failure
        //
        for (port = RasPorts, i = 0; i < TotalPorts; i++, port++) {

            // Skip ports that arent initialized
            //
	        if (port->TPCB_State == PS_UNINITIALIZED)
	            continue ;

            if (port->TPCB_Line == line) {

                if (retcode = lineOpen (RasLine,
	                            port->TPCB_Line->TLI_LineId,
	                            &port->TPCB_Line->TLI_LineHandle,
	                            port->TPCB_Line->NegotiatedApiVersion,
	                            port->TPCB_Line->NegotiatedExtVersion,
	                            (ULONG) port->TPCB_Line,
	                            LINECALLPRIVILEGE_OWNER,
	                            port->TPCB_MediaMode,
	                            NULL))
                    ; // DbgPrint ("LINECLOSE: lineOpen failed with %d\n", retcode) ;

		//
		// Set monitoring of rings
		//
		lineSetStatusMessages (port->TPCB_Line->TLI_LineHandle, LINEDEVSTATE_RINGING, 0) ;

                // If the port is listening - then we do not need to do anything since the listen is
                // posted implicitly,
                //
                if (port->TPCB_State != PS_LISTENING) {
                    //
                    // These settings should cause connections and connect attempts to
                    // fail.
                    //
                    port->TPCB_AsyncErrorCode = ERROR_FROM_DEVICE ;
	                port->TPCB_CallHandle =  (HCALL) 0xffffffff ;
                    port->TPCB_ListenState = LS_ERROR ;

                    // Signal hardware failure to rasman.
                    //
                    // DbgPrint ("SignalDisc: 3\n") ;
                    SetEvent (port->TPCB_DiscNotificationHandle) ;
                }
            }
        }
        break ;

    case LINE_LINEDEVSTATE:

	//
	// we are only interested in ringing message
	//
	if (param1 != LINEDEVSTATE_RINGING)
	    break ;

	// Find which line is indication came for.
        //
        line = (TapiLineInfo *) instance ;

        // if line not found or if it is closed just return
        //
        if ((line == NULL) || (line->TLI_LineState == PS_CLOSED))
            break ;

	// get the port from the line
        //
        for (port = RasPorts, i = 0; i < TotalPorts; i++, port++) {

            // Skip ports that arent initialized
            //
	    if (port->TPCB_State == PS_UNINITIALIZED)
		continue ;

	    if ((port->TPCB_Line == line) && (port->TPCB_State == PS_LISTENING) && (port->TPCB_ListenState == LS_RINGING)) {

		//
		// count down the rings
		//
		port->TPCB_NumberOfRings -= 1 ;
		// DbgPrint ("Ring count = %d\n", port->TPCB_NumberOfRings) ;

		//
		// if the ring count has gone down to zero this means that we should pick up the call.
		//
		if (port->TPCB_NumberOfRings == 0) {

		    port->TPCB_ListenState = LS_ACCEPT ;

		    //
		    // Complete event so that rasman calls DeviceWork to proceed the listen state machine.
		    //
		    SetEvent (port->TPCB_ReqNotificationHandle) ;
		}

		break ;
            }
	}

	break ;

    case LINE_ADDRESSSTATE:
    case LINE_CALLINFO:
    case LINE_CREATE:
    case LINE_DEVSPECIFIC:
    case LINE_DEVSPECIFICFEATURE:
    case LINE_GATHERDIGITS:
    case LINE_GENERATE:

    case LINE_MONITORDIGITS:
    case LINE_MONITORMEDIA:
    case LINE_MONITORTONE:
    case LINE_REQUEST:
	default:
        // All unhandled unsolicited messages.
        //
		;
    }

    // **** Exclusion End ****
	FreeMutex (RasTapiMutex) ;
}



//* FindPortByAddressId()
//
//
//
//
//*
TapiPortControlBlock *
FindPortByAddressId (TapiLineInfo *line, DWORD addid)
{
    DWORD   i ;
    TapiPortControlBlock *port ;

    for (i=0, port=RasPorts; i<TotalPorts; i++, port++) {

	if ((port->TPCB_AddressId == addid) && (port->TPCB_Line == line))
	    return port ;

    }

    return NULL ;
}


//* FindPortByAddress()
//
//
//
//
//*
TapiPortControlBlock *
FindPortByAddress (CHAR *address)
{
    DWORD   i ;
    TapiPortControlBlock *port ;

    for (i=0, port=RasPorts; i<TotalPorts; i++, port++) {

	if (_stricmp (port->TPCB_Address, address) == 0)
	    return port ;

    }

    return NULL ;
}


//* FindPortByAddress()
//
//
//
//
//*
TapiPortControlBlock *
FindPortByAddressAndName (CHAR *address, CHAR *name)
{
    DWORD   i ;
    TapiPortControlBlock *port ;

    for (i=0, port=RasPorts; i<TotalPorts; i++, port++) {

	if ((_stricmp (port->TPCB_Address, address) == 0) &&
	    (_strnicmp (port->TPCB_Name, name, MAX_PORT_NAME-1) == 0))
	    return port ;

    }

    return NULL ;
}


//* FindPortByRequestId()
//
//
//
//
//*
TapiPortControlBlock *
FindPortByRequestId (DWORD reqid)
{
    DWORD   i ;
    TapiPortControlBlock *port ;

    for (i=0, port=RasPorts; i<TotalPorts; i++, port++) {

	if (port->TPCB_RequestId == reqid)
	    return port ;

    }

    return NULL ;
}



//* FindLineByHandle()
//
//
//
//
//*
TapiLineInfo *
FindLineByHandle (HLINE linehandle)
{
    DWORD i ;
    TapiLineInfo *line ;

    for (i=0, line=RasTapiLineInfo; i<TotalLines; i++, line++) {

	if (line->TLI_LineHandle == linehandle)
	    return line ;

    }

    return NULL ;
}



#define VALNAME_ATTACHEDTO "AttachedTo"

BOOL
GetAssociatedPortName(
   char  * szKeyName,
   CHAR * szPortName)
/*
 * Given the registry key name 'szRegistryKeyName' corresponding to the modem entry, fills in
 * 'wszAddress' with the associated port like COM1, ..
 *
 */
{
   HKEY   hKeyModem;
   DWORD  dwType;
   DWORD  cbValueBuf;
#if 0
   char   buf[256];
#endif

#if 0
   wsprintfA(buf,"RegistryKey -> %s \n", szKeyName);
   OutputDebugStringA(buf);
#endif

   if ( RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                     szKeyName,
                     0,
                     KEY_READ,
                     &hKeyModem ) )
   {
       return( FALSE );
   }

   cbValueBuf = 40 ;

   if ( RegQueryValueExA(hKeyModem,
                        VALNAME_ATTACHEDTO,
                        NULL,
                        &dwType,
			(LPBYTE)szPortName,
                        &cbValueBuf ))
   {
      return ( FALSE );
   }

   RegCloseKey( hKeyModem );

   return ( TRUE );
}
