/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    w3svc.h

    This file contains constants & type definitions shared between the
    W3 Service, Installer, and Administration UI.


    FILE HISTORY:
        KeithMo     10-Mar-1993 Created.
        MuraliK     Redefined service names

*/


#ifndef _W3SVC_H_
#define _W3SVC_H_

#ifdef __cplusplus
extern "C"
{
#endif  // _cplusplus

#if !defined(MIDL_PASS)
#include <winsock.h>
#endif

# include <inetinfo.h>

//
//  Service name.
//

#define IPPORT_W3                      0x50

//
//  Name of the log file, used for logging file accesses.
//

#define W3_LOG_FILE                    TEXT("HTTPSVC.LOG")


//
//  Configuration parameters registry key.
//

#define W3_PARAMETERS_KEY \
            TEXT("System\\CurrentControlSet\\Services\\W3Svc\\Parameters")


//
//  Performance key.
//

#define W3_PERFORMANCE_KEY \
            TEXT("System\\CurrentControlSet\\Services\\W3Svc\\Performance")

//
//  Configuration value names.
//

#define W3_CHECK_FOR_WAISDB            TEXT("CheckForWAISDB")
#define W3_DEBUG_FLAGS                 TEXT("DebugFlags")
#define W3_DIR_BROWSE_CONTROL          TEXT("Dir Browse Control")
#define W3_DIR_ICON                    TEXT("Folder Image")
#define W3_DIR_ICON_W                  L"Folder Image"
#define W3_DEFAULT_FILE                TEXT("Default Load File")
#define W3_DEFAULT_FILE_W              L"Default Load File"
#define W3_SERVER_AS_PROXY             TEXT("ServerAsProxy")
#define W3_CATAPULT_USER               TEXT("CatapultUser")
#define W3_CATAPULT_USER_W             L"CatapultUser"
#define W3_SCRIPT_TIMEOUT              "ScriptTimeout"
#define W3_CACHE_EXTENSIONS            "CacheExtensions"
#define W3_SSI_ENABLED                 "ServerSideIncludesEnabled"
#define W3_SSI_EXTENSION               "ServerSideIncludesExtension"
#define W3_SSI_EXTENSION_W             L"ServerSideIncludesExtension"
#define W3_GLOBAL_EXPIRE               "GlobalExpire"
#define W3_PROVIDER_LIST               "NTAuthenticationProviders"
#define W3_SECURE_PORT                 "SecurePort"
#define W3_ENC_PROVIDER_LIST           "NTEncryptionProviders"
#define W3_ENC_FLAGS                   "EncryptionFlags"
#define W3_ACCESS_DENIED_MSG           "AccessDeniedMessage"
#define W3_DEFAULT_HOST_NAME           "ReturnUrlUsingHostName"
#define W3_ACCEPT_BYTE_RANGES          "AcceptByteRanges"
#define W3_ALLOW_GUEST                 "AllowGuestAccess"
#define W3_LOG_ERRORS                  "LogErrorRequests"
#define W3_LOG_SUCCESS                 "LogSuccessfulRequests"
#define W3_REALM_NAME                  "Realm"
#define IDC_POOL_CONN                  "PoolIDCConnections"
#define IDC_POOL_CONN_TIMEOUT          "PoolIDCConnectionsTimeout"
#define W3_UPLOAD_READ_AHEAD           "UploadReadAhead"
#define W3_USE_POOL_THREAD_FOR_CGI     "UsePoolThreadForCGI"
#define W3_ALLOW_KEEP_ALIVES           "AllowKeepAlives"

//
//  Name of the LSA Secret Object containing the password for
//  anonymous logon.
//

#define W3_ANONYMOUS_SECRET         TEXT("W3_ANONYMOUS_DATA")
#define W3_ANONYMOUS_SECRET_A       "W3_ANONYMOUS_DATA"
#define W3_ANONYMOUS_SECRET_W       L"W3_ANONYMOUS_DATA"

//
//  The set of password/virtual root pairs
//

#define W3_ROOT_SECRET_W            L"W3_ROOT_DATA"

//
//  The password secret for the username to connect to the Catapult gateway if
//  the HTTP server is running as a Catapult proxy client
//

#define W3_PROXY_USER_SECRET_W      L"W3_PROXY_USER_SECRET"

//
//  This is the secret that contains the list of installed SSL keys
//

#define W3_SSL_KEY_LIST_SECRET      L"W3_KEY_LIST"

#ifdef __cplusplus
}
#endif  // _cplusplus


#endif  // _W3SVC_H_
