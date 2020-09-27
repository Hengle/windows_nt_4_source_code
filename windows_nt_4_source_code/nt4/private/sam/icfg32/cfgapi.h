//*******************************************************************
//
//  Copyright(c) Microsoft Corporation, 1996
//
//  FILE: CFGAPI.H
//
//  PURPOSE:  Contains API's exported from icfg32.dll and structures
//            required by those functions.
//
//*******************************************************************

#ifndef _CFGAPI_H_
#define _CFGAPI_H_

// Maximum buffer size for error messages.
#define MAX_ERROR_TEXT  512

// Flags for dwfOptions

// install TCP (if needed)
#define ICFG_INSTALLTCP            0x00000001

// install RAS (if needed)
#define ICFG_INSTALLRAS            0x00000002

// install exchange and internet mail
#define ICFG_INSTALLMAIL           0x00000004


// DRIVERTYPE_ defines for TCP/IP configuration apis
#define DRIVERTYPE_NET  0x0001
#define DRIVERTYPE_PPP  0x0002


#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus


//*******************************************************************
//
//  FUNCTION:   IcfgNeedInetComponents
//
//  PURPOSE:    Detects whether the specified system components are
//              installed or not.
//
//  PARAMETERS: dwfOptions - a combination of ICFG_ flags that specify
//              which components to detect as follows:
//
//                ICFG_INSTALLTCP - is TCP/IP needed?
//                ICFG_INSTALLRAS - is RAS needed?
//                ICFG_INSTALLMAIL - is exchange or internet mail needed?
//
//              lpfNeedComponents - TRUE if any specified component needs
//              to be installed.
//
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//
//*******************************************************************

HRESULT IcfgNeedInetComponents(DWORD dwfOptions, LPBOOL lpfNeedComponents);


//*******************************************************************
//
//  FUNCTION:   IcfgInstallInetComponents
//
//  PURPOSE:    Install the specified system components.
//
//  PARAMETERS: hwndParent - Parent window handle.
//              dwfOptions - a combination of ICFG_ flags that controls
//              the installation and configuration as follows:
//
//                ICFG_INSTALLTCP - install TCP/IP (if needed)
//                ICFG_INSTALLRAS - install RAS (if needed)
//                ICFG_INSTALLMAIL - install exchange and internet mail
//
//              lpfNeedsRestart - if non-NULL, then on return, this will be
//              TRUE if windows must be restarted to complete the installation.
//
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//
//*******************************************************************

HRESULT IcfgInstallInetComponents(HWND hwndParent, DWORD dwfOptions,
  LPBOOL lpfNeedsRestart);


//*******************************************************************
//
//  FUNCTION:   IcfgGetLastInstallErrorText
//
//  PURPOSE:    Get a text string that describes the last installation
//              error that occurred.  The string should be suitable
//              for display in a message box with no further formatting.
//
//  PARAMETERS: lpszErrorDesc - points to buffer to receive the string.
//              cbErrorDesc - size of buffer.
//
//  RETURNS:    The length of the string returned.
//
//*******************************************************************

DWORD IcfgGetLastInstallErrorText(LPSTR lpszErrorDesc, DWORD cbErrorDesc);


//*******************************************************************
//
//  FUNCTION:   IcfgSetInstallSourcePath
//
//  PURPOSE:    Sets the path where windows looks when installing files.
//
//  PARAMETERS: lpszSourcePath - full path of location of files to install.
//              If this is NULL, default path is used.
//
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//
//*******************************************************************

HRESULT IcfgSetInstallSourcePath(LPCSTR lpszSourcePath);


//*******************************************************************
//
//  FUNCTION:   IcfgIsGlobalDNS
//
//  PURPOSE:    Determines whether there is Global DNS set.
//
//  PARAMETERS: lpfGlobalDNS - TRUE if global DNS is set, FALSE otherwise.
//
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//              NOTE:  This function is for Windows 95 only, and
//              should always return ERROR_SUCCESS and set lpfGlobalDNS
//              to FALSE in Windows NT.
//
//*******************************************************************

HRESULT IcfgIsGlobalDNS(LPBOOL lpfGlobalDNS);


//*******************************************************************
//
//  FUNCTION:   IcfgRemoveGlobalDNS
//
//  PURPOSE:    Removes global DNS info from registry.
//
//  PARAMETERS: None.
//
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//              NOTE:  This function is for Windows 95 only, and
//              should always return ERROR_SUCCESS in Windows NT.
//
//*******************************************************************

HRESULT IcfgRemoveGlobalDNS(void);


//*******************************************************************
//
//  FUNCTION:   IcfgIsFileSharingTurnedOn
//
//  PURPOSE:    Determines if file server (VSERVER) is bound to TCP/IP
//              for specified driver type (net card or PPP).
//
//  PARAMETERS: dwfDriverType - a combination of DRIVERTYPE_ flags
//              that specify what driver type to check server-TCP/IP
//              bindings for as follows:
//
//                DRIVERTYPE_NET  - net card
//                DRIVERTYPE_PPP        - PPPMAC
//
//              lpfSharingOn - TRUE if bound once or more, FALSE if not bound
//
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//
//*******************************************************************

HRESULT IcfgIsFileSharingTurnedOn(DWORD dwfDriverType, LPBOOL lpfSharingOn);


//*******************************************************************
//
//  FUNCTION:   IcfgTurnOffFileSharing
//
//  PURPOSE:    Unbinds file server (VSERVER) from TCP/IP for
//              specified driver type (net card or PPP).
//
//  PARAMETERS: dwfDriverType - a combination of DRIVERTYPE_ flags
//              that specify what driver type to remove server-TCP/IP
//              bindings for as follows:
//
//                DRIVERTYPE_NET  - net card
//                DRIVERTYPE_PPP        - PPPMAC
//
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//
//*******************************************************************

HRESULT IcfgTurnOffFileSharing(DWORD dwfDriverType);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif //_CFGAPI_H_
