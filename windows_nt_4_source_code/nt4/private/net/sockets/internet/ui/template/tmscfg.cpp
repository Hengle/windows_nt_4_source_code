///////////////////////////////////////////////////////////////////////////////
//
// File:    tmscfg.cpp
// Purpose: API Entry points for service ui
//

#include "stdafx.h"

#define DLL_BASED __declspec(dllexport)

#include "tmscfg.h"
#include "tmservic.h"
#include "tmsessio.h"

// ===========================================================================
// Standard configuration Information
// ===========================================================================

//
// Is this server discoverable by INETSLOC?
//
#define INETSLOC_DISCOVERY          FALSE

#if (INETSLOC_DISCOVERY) && !defined(_SVCLOC_)
    #error You must include svcloc.h.
#endif

//
// If INETSLOC_DISCOVERY == TRUE, define the discovery MASK here.
//
#if (INETSLOC_DISCOVERY) 
    #define INETSLOC_MASK               
#else  // (!INETSLOC_DISCOVERY) 
    #define INETSLOC_MASK           (ULONGLONG)(0x00000000)
#endif // (INETSLOC_DISCOVERY) 

//
// Do we use the service control manager to control
// the service we administer?
//
#define USE_SC_MANAGER              FALSE

//
// Can we change the service state (start/pause/continue)?
//
#define CAN_CHANGE_SERVICE_STATE    FALSE

//
// Can we pause this service?
//
#define CAN_PAUSE_SERVICE           FALSE

#if (USE_SC_MANAGER)
    //
    // Name used for this service by the service controller manager.
    //
    #define SERVICE_SC_NAME         _T("TEMPLATE")
#endif // USE_SC_MANAGER

//
// Short descriptive name of the service.  This
// is what will show up as the name of the service
// in the internet manager tool.
//
// Issue: I'm assuming here that this name does NOT
//        require localisation.
//
#define SERVICE_SHORT_NAME      _T("Template")

//
// Longer name.  This is the text that shows up in
// the tooltips text on the internet manager
// tool.  This probably should be localised.
//

#define SERVICE_LONG_NAME      _T("Template Service")

//
// Use normal colour mapping.
//
#define NORMAL_TB_MAPPING          TRUE

//
// Toolbar button background mask. This is
// the colour that gets masked out in
// the bitmap file and replaced with the
// actual button background.  This setting
// is automatically assumed to be lt. gray
// if NORMAL_TB_MAPPING (above) is TRUE
//
#define BUTTON_BMP_BACKGROUND       RGB(192, 192, 192)      // Lt. Gray

//
// Resource ID of the toolbar button bitmap.
//
// The bitmap must be 17x17
//
#define BUTTON_BMP_ID               IDB_TEMPLATE

//
// Similar to BUTTON_BMP_BACKGROUND, this is the
// background mask for the service ID
//
#define SERVICE_BMP_BACKGROUND      BUTTON_BMP_BACKGROUND

//
// Bitmap id which is used in the service view
// of the service manager.  
//
// The bitmap must be 17x17, and can be the
// same as BUTTON_BMP_ID bitmap.
//
#define SERVICE_BMP_ID              BUTTON_BMP_ID

// ===========================================================================
// End Of Standard configuration Information
// ===========================================================================
 
//
// Global DLL instance
//
HINSTANCE hInstance;

// ============================================================================
// ISM API Functions
// ============================================================================

///////////////////////////////////////////////////////////////////////////////
//
// Return service-specific information back to
// to the application.  This function is called
// by the service manager immediately after
// LoadLibary();  The size element must be
// set prior to calling this API.
//
DLL_BASED DWORD  APIENTRY
ISMQueryServiceInfo(
    ISMSERVICEINFO * psi        // Service information returned.
    )
{
    if ( psi == NULL
      || psi->dwSize < ISMSERVICEINFO_SIZE
       )
    {
        TRACEEOLID(_T("ISMQueryServiceInfo: ISMSERVICEINFO invalid"));
        ASSERT(0);
        return ERROR_INVALID_PARAMETER;
    }

#ifdef _DEBUG

    if (psi->dwSize != ISMSERVICEINFO_SIZE)
    {
        TRACEEOLID(_T("Warning: internet manager is newer than DLL"));
    }

#endif // _DEBUG

    psi->dwSize = ISMSERVICEINFO_SIZE;
    psi->dwVersion = ISM_VERSION;

    psi->flServiceInfoFlags = 0
#if (INETSLOC_DISCOVERY)
        | ISMI_INETSLOCDISCOVER
#endif 
#if (CAN_CHANGE_SERVICE_STATE)
        | ISMI_CANCONTROLSERVICE   
#endif 
#if (CAN_PAUSE_SERVICE)
        | ISMI_CANPAUSESERVICE
#endif 
#if (NORMAL_TB_MAPPING)
        | ISMI_NORMALTBMAPPING
#endif
        ; /**/

    ASSERT(::lstrlen(SERVICE_LONG_NAME) <= MAX_LNLEN);
    ASSERT(::lstrlen(SERVICE_SHORT_NAME) <= MAX_SNLEN);

    psi->ullDiscoveryMask = INETSLOC_MASK;
    psi->rgbButtonBkMask = BUTTON_BMP_BACKGROUND;
    psi->nButtonBitmapID = BUTTON_BMP_ID;
    psi->rgbServiceBkMask = SERVICE_BMP_BACKGROUND;
    psi->nServiceBitmapID = SERVICE_BMP_ID;
    ::lstrcpy(psi->atchShortName, SERVICE_SHORT_NAME);
    ::lstrcpy(psi->atchLongName, SERVICE_LONG_NAME);

    return ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
//
// Discover machines running this service.  This is
// only necessary for services not discovered with
// inetscloc (which don't give a mask)
//
DLL_BASED DWORD APIENTRY
ISMDiscoverServers(
    ISMSERVERINFO * psi,        // Server info buffer.
    DWORD * pdwBufferSize,      // Size required/available.  
    int * cServers              // Number of servers in buffer.
    )
{
#if (INETSLOC_DISCOVERY)
    *cServers = 0;
    *pdwBufferSize = 0L;

    TRACEEOLID(_T("Warning: service manager called bogus ISMDiscoverServers"));

    return ERROR_SUCCESS;

#else

    //#error Service specific discovery function must be provided.

    //
    // Dummy example, which discovers 2 servers running our service
    //
    int cServersFound = 2;
    DWORD dwRequiredSize = sizeof(ISMSERVERINFO) * cServersFound;

    if (*pdwBufferSize < dwRequiredSize || psi == NULL)
    {
        *pdwBufferSize = dwRequiredSize;
        *cServers = cServersFound;

        return ERROR_MORE_DATA;
    }

    psi[0].dwSize = ISMSERVERINFO_SIZE;
    ::lstrcpy(psi[0].atchServerName, _T("\\FOO"));
    ::lstrcpy(psi[0].atchComment, _T("Dummy Entry 1"));
    psi[0].nState = INetServiceUnknown;

    psi[1].dwSize = ISMSERVERINFO_SIZE;
    ::lstrcpy(psi[1].atchServerName, _T("\\BAR"));
    ::lstrcpy(psi[1].atchComment, _T("Dummy Entry 2"));
    psi[1].nState = INetServiceUnknown;

    return ERROR_SUCCESS;

#endif // (INETSLOC_DISCOVERY)
}

///////////////////////////////////////////////////////////////////////////////
//
// Get information about a specific server with
// regards to this service.  Usually this uses
// the service control manager
//
DLL_BASED DWORD APIENTRY
ISMQueryServerInfo(
    LPCTSTR lpstrServerName,    // Name of server.
    ISMSERVERINFO * psi         // Server information returned.
    )
{
    if ( psi == NULL
      || psi->dwSize < ISMSERVERINFO_SIZE
      || ::lstrlen(lpstrServerName) > MAX_SERVERNAME_LEN
       )
    {
        TRACEEOLID(_T("ISMQueryServerInfo: bad parameters"));
        ASSERT(0);
        return ERROR_INVALID_PARAMETER;
    }

#ifdef _DEBUG

    if (psi->dwSize != ISMSERVERINFO_SIZE)
    {
        TRACEEOLID(_T("Warning internet manager is newer than DLL"));
    }

#endif // _DEBUG

    psi->dwSize = ISMSERVERINFO_SIZE;
    ::lstrcpy(psi->atchServerName, lpstrServerName);

#if (USE_SC_MANAGER)
    SC_HANDLE hScManager;

    hScManager = ::OpenSCManager(lpstrServerName, NULL, SC_MANAGER_ENUMERATE_SERVICE);

    if (hScManager == NULL)
    {
        return ::GetLastError();
    }

    DWORD err = ERROR_SUCCESS;

    SC_HANDLE hService = ::OpenService(hScManager, 
        SERVICE_SC_NAME, SERVICE_QUERY_STATUS);

    if (hService == NULL)
    {
        err = ::GetLastError();
    }
    else
    {
        SERVICE_STATUS ss;

        VERIFY(::QueryServiceStatus(hService, &ss));
        switch(ss.dwCurrentState)
        {
        case SERVICE_STOPPED:
        case SERVICE_STOP_PENDING:
            psi->nState = INetServiceStopped;
            break;

        case SERVICE_RUNNING:
        case SERVICE_START_PENDING:
        case SERVICE_CONTINUE_PENDING:
            psi->nState = INetServiceRunning;
            break;

        case SERVICE_PAUSE_PENDING:
        case SERVICE_PAUSED:
            psi->nState = INetServicePaused;
            break;

        default:
            psi->nState = INetServiceUnknown;
        }

        ::CloseServiceHandle(hService);
    }

    ::CloseServiceHandle(hScManager);

    return err;

#else

    //#error Non SC QueryServerInfo must be defined!

    ::lstrcpy(psi->atchComment, _T("Dummy Entry"));
    psi->nState = INetServiceUnknown;

    return ERROR_SUCCESS;

#endif // USE_SC_MANAGER
}

///////////////////////////////////////////////////////////////////////////////
//
// Change the service state of the servers (to paused/continue, started,
// stopped, etc)
//
DLL_BASED DWORD APIENTRY
ISMChangeServiceState(
    int nNewState,              // INetService* definition.
    int * pnCurrentState,       // Ptr to current state (will be changed
    DWORD dwReserved,           // Reserved: must be 0
    LPCTSTR lpstrServers        // Double NULL terminated list of servers.
    )
{
    if ( dwReserved != 0L
      || nNewState < INetServiceStopped 
      || nNewState > INetServicePaused
       )
    {
        TRACEEOLID(_T("ISMChangeServiceState: Invalid information passed"));
        ASSERT(0);
        return ERROR_INVALID_PARAMETER;
    }

#if (USE_SC_MANAGER)
    //
    // BUGBUG: SINGLE SELECTION ONLY!
    //
    SC_HANDLE hService;
    SC_HANDLE hScManager;
    hScManager = ::OpenSCManager(lpstrServers, NULL, SC_MANAGER_ALL_ACCESS);
                        
    if (hScManager == NULL)
    {
        return ::GetLastError();
    }
    hService = OpenService(hScManager, SERVICE_SC_NAME, SERVICE_ALL_ACCESS);
    if (hService == NULL)
    {
        return ::GetLastError();
    }

    SERVICE_STATUS ss;
    BOOL fSuccess;

    switch(nNewState)
    {
    case INetServiceStopped:
        fSuccess = ::ControlService(hService, SERVICE_CONTROL_STOP, &ss);
        break;

    case INetServiceRunning:
        if (*pnCurrentState == INetServicePaused)
        {
            fSuccess = ::ControlService(hService, SERVICE_CONTROL_CONTINUE, &ss);
        }
        else
        {
            fSuccess = ::StartService(hService, 0, NULL);
        }
        break;
    case INetServicePaused:
        fSuccess = ::ControlService(hService, SERVICE_CONTROL_PAUSE, &ss);
        break;
    }

    //
    // Update state information
    //    
    VERIFY(::QueryServiceStatus(hService, &ss));
    switch(ss.dwCurrentState)
    {
    case SERVICE_STOPPED:
    case SERVICE_STOP_PENDING:
        *pnCurrentState = INetServiceStopped;
        break;

    case SERVICE_RUNNING:
    case SERVICE_START_PENDING:
    case SERVICE_CONTINUE_PENDING:
        *pnCurrentState = INetServiceRunning;
        break;

    case SERVICE_PAUSE_PENDING:
    case SERVICE_PAUSED:
        *pnCurrentState = INetServicePaused;
        break;

    default:
        *pnCurrentState = INetServiceUnknown;
    }

    ::CloseServiceHandle(hService);
    ::CloseServiceHandle(hScManager);

    if (!fSuccess)
    {
        return ::GetLastError();
    }
     
    return ERROR_SUCCESS;

#else

    //#error Non-SC ISMChangeServiceState must be defined

    return ERROR_SERVICE_DOES_NOT_EXIST;

#endif // USE_SC_MANAGER
}

///////////////////////////////////////////////////////////////////////////////
//
// Display configuration property sheet.
//
DLL_BASED DWORD APIENTRY
ISMConfigureServers(
    HWND hWnd,                  // Main app window handle
    DWORD dwReserved,           // Reserved: must be 0
    LPCTSTR lpstrServers        // Double NULL terminated list of servers
    )
{
    DWORD err;

    if ( dwReserved != 0L )
    {
        TRACEEOLID(_T("ISMConfigureServers: Invalid information passed"));
        ASSERT(0);
        return ERROR_INVALID_PARAMETER;
    }

    //
    // convert the list of servers to a 
    // more manageable CStringList.
    //
    LPCTSTR pBuf = lpstrServers;
    CStringList strlServers;
    while ( *pBuf != TCHAR('\0' ))
    {
        CString strTmp = pBuf;
        strlServers.AddTail( strTmp );
        pBuf += strTmp.GetLength() + 1;
    }

    if ( strlServers.GetCount() == 0 )
    {
        TRACEEOLID(_T("Error: strlServers Count == 0."));
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Save the resource handle.
    //
    HINSTANCE hOldInstance = ::AfxGetResourceHandle();
    ASSERT( hInstance != NULL );

    //
    // Load resources out of our own resource segment
    //
    ::AfxSetResourceHandle( hInstance );

    CString strCaption;

    if (strlServers.GetCount() == 1)
    {
        CString str;

        VERIFY(str.LoadString(IDS_CAPTION));
        strCaption.Format(str, INETSVC_DISPLAYNAME(lpstrServers));
    }
    else // Multiple server caption
    {
        VERIFY(strCaption.LoadString(IDS_CAPTION_MULTIPLE));
    }

    ::AfxMessageBox(_T("Initialisation happens here"));

    //
    // Didn't do anything, but what we did, we did
    // perfectly.
    //
    err = ERROR_SUCCESS;

    //
    // reset the resource handle to the application's
    // resource segment.
    //
    ::AfxSetResourceHandle( hOldInstance );

    return err;
}

// ============================================================================
// End of ISM API Functions
// ============================================================================

//
// Perform additional initialisation as necessary
//
void
InitializeDLL()
{
    #ifdef _DEBUG
        afxMemDF |= checkAlwaysMemDF;
    #endif // _DEBUG
}

//
// DLL Main entry point
//
DLL_BASED BOOL WINAPI 
LibMain(
    HINSTANCE hDll,
    DWORD dwReason,
    LPVOID lpReserved
    )
{
    BOOL bResult = TRUE ;

    switch ( dwReason )
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
        ASSERT( hDll != NULL );
        hInstance = hDll;
        InitializeDLL();
        break ;

    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        //
        // termination
        //
        break ;
    }

    ASSERT( hInstance != NULL );

    return bResult ;
}
