
/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    services.c

Abstract:

    Routines to deal with the Windows NT service controller
    and service entries in the registry,

    Externally exposed routines:

        MyCreateService
        MyChangeServiceStart
        MyChangeServiceConfig

Author:

    Ted Miller (tedm) 5-Apr-1995
    adapted from legacy\dll\sc.c

Revision History:

--*/

#include "setupp.h"
#pragma hdrstop

//
// Constants used in logging specific to this module.
//
PCWSTR szOpenSCManager       = L"OpenSCManager";
PCWSTR szCreateService       = L"CreateService";
PCWSTR szChangeServiceConfig = L"ChangeServiceConfig";
PCWSTR szOpenService         = L"OpenService";
PCWSTR szStartService        = L"StartService";
PCWSTR szEnumDependentService= L"EnumDependentService";

PCWSTR szServicesKeyPath         = L"SYSTEM\\CurrentControlSet\\Services";
PCWSTR szDependOnService         = L"DependOnService";
PCWSTR szServicesToRename        = L"ServicesToRename";

BOOL
MyCreateService(
    IN PCWSTR  ServiceName,
    IN PCWSTR  DisplayName,         OPTIONAL
    IN DWORD   ServiceType,
    IN DWORD   StartType,
    IN DWORD   ErrorControl,
    IN PCWSTR  BinaryPathName,
    IN PCWSTR  LoadOrderGroup,      OPTIONAL
    IN PWCHAR  DependencyList,
    IN PCWSTR  ServiceStartName,    OPTIONAL
    IN PCWSTR  Password             OPTIONAL
    )

/*++

Routine Description:

    Stub for calling CreateService. If CreateService fails with
    the error code indicating that the service already exists, this routine
    calls the routine for ChangeServiceConfig to ensure that the
    parameters passed in are reflected in the services database.

Arguments:

    ServiceName - Name of service

    DisplayName - Localizable name of Service or ""

    ServiceType - Service type, e.g. SERVICE_KERNEL_DRIVER

    StartType - Service Start value, e.g. SERVICE_BOOT_START

    ErrorControl - Error control value, e.g. SERVICE_ERROR_NORMAL

    BinaryPathName - Full Path of the binary image containing service

    LoadOrderGroup - Group name for load ordering or ""

    Dependencies - MultiSz list of dependencies for this service. Any dependency
        component having + as the first character is a group dependency.
        The others are service dependencies.

    ServiceStartName - Service Start name (account name in which this service is run).

    Password - Password used for starting the service.

Return value:

    Boolean value indicating outcome.

--*/

{
    SC_HANDLE hSC;
    SC_HANDLE hSCService;
    DWORD dwTag,dw;
    BOOL b;

    //
    // Open a handle to the service controller manager
    //
    hSC = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
    if(hSC == NULL) {
        LogItem2(
            LogSevWarning,
            MSG_LOG_CREATESVC_FAIL,
            ServiceName,
            MSG_LOG_X_RETURNED_WINERR,
            szOpenSCManager,
            GetLastError()
            );
        return(FALSE);
    }

    //
    // Process the optional "" parameters passed in and make them NULL.
    //
    if(DisplayName && !DisplayName[0]) {
        DisplayName = NULL;
    }
    if(LoadOrderGroup && !LoadOrderGroup[0]) {
        LoadOrderGroup = NULL;
    }
    if(ServiceStartName && !ServiceStartName[0]) {
        ServiceStartName = NULL;
    }
    if(Password && !Password[0]) {
        Password = NULL;
    }

    //
    // Create the service.
    //

    hSCService = CreateService(
                     hSC,
                     ServiceName,
                     DisplayName,
                     0,
                     ServiceType,
                     StartType,
                     ErrorControl,
                     BinaryPathName,
                     LoadOrderGroup,
                     LoadOrderGroup ? &dwTag : NULL,
                     DependencyList,
                     ServiceStartName,
                     Password
                     );
    //
    // If we were unable to create the service, check if the service already
    // exists in which case all we need to do is change the configuration
    // parameters in the service.
    //
    if(hSCService) {
        //
        // Note that we won't do anything with the tag.
        //
        CloseServiceHandle(hSCService);
        b = TRUE;
    } else {
        if((dw = GetLastError()) == ERROR_SERVICE_EXISTS) {

            b = MyChangeServiceConfig(
                    ServiceName,
                    ServiceType,
                    StartType,
                    ErrorControl,
                    BinaryPathName,
                    LoadOrderGroup,
                    DependencyList,
                    ServiceStartName,
                    Password,
                    DisplayName
                    );
        } else {
            LogItem2(
                LogSevWarning,
                MSG_LOG_CREATESVC_FAIL,
                ServiceName,
                MSG_LOG_X_RETURNED_WINERR,
                szCreateService,
                dw
                );
            b = FALSE;
        }
    }

    CloseServiceHandle(hSC);
    return(b);
}


BOOL
MyChangeServiceConfig(
    IN PCWSTR ServiceName,
    IN DWORD  ServiceType,
    IN DWORD  StartType,
    IN DWORD  ErrorControl,
    IN PCWSTR BinaryPathName,   OPTIONAL
    IN PCWSTR LoadOrderGroup,   OPTIONAL
    IN PWCHAR DependencyList,
    IN PCWSTR ServiceStartName, OPTIONAL
    IN PCWSTR Password,         OPTIONAL
    IN PCWSTR DisplayName       OPTIONAL
    )

/*++

Routine Description:

    Wrapper for ChangeServiceConfig.

Arguments:

    ServiceName - Name of service

    ServiceType - Service type, e.g. SERVICE_KERNEL_DRIVER

    StartType - Service Start value, e.g. SERVICE_BOOT_START

    ErrorControl - Error control value, e.g. SERVICE_ERROR_NORMAL

    BinaryPathName - Full Path of the binary image containing service

    LoadOrderGroup - Group name for load ordering

    DependencyList - Multisz string having dependencies.  Any dependency
        component having + as the first character is a
        group dependency.  The others are service dependencies.

    ServiceStartName - Service Start name (account name in which this
        service is run).

    Password - Password used for starting the service.

    DisplayName - Localizable name of Service.

Return value:

    Boolean value indicating outcome.

--*/

{
    SC_LOCK sclLock;
    SC_HANDLE hSC;
    SC_HANDLE hSCService;
    DWORD dw;
    BOOL b;

    //
    // Open a handle to the service controller manager
    //
    hSC = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
    if(hSC == NULL) {
        LogItem2(
            LogSevWarning,
            MSG_LOG_CHANGESVC_FAIL,
            ServiceName,
            MSG_LOG_X_RETURNED_WINERR,
            szOpenSCManager,
            GetLastError()
            );
        return(FALSE);
    }

    //
    // Try to lock the database, if possible. If we are not able to lock
    // the database we will still modify the services entry. This is because
    // we are just modifying a single service and chances are very low that
    // anybody else is manipulating the same entry at the same time.
    //
    sclLock = LockServiceDatabase(hSC);

    //
    // Process optional parameters
    //
    if(BinaryPathName && !BinaryPathName[0]) {
        BinaryPathName = NULL;
    }
    if(LoadOrderGroup && !LoadOrderGroup[0]) {
        LoadOrderGroup = NULL;
    }
    if(ServiceStartName && !ServiceStartName[0]) {
        ServiceStartName = NULL;
    }
    if(Password && !Password[0]) {
        Password = NULL;
    }
    if(DisplayName && !DisplayName[0]) {
        DisplayName = NULL;
    }

    //
    // Open the service with SERVICE_CHANGE_CONFIG access
    //
    if(hSCService = OpenService(hSC,ServiceName,SERVICE_CHANGE_CONFIG)) {

        b = ChangeServiceConfig(
                hSCService,
                ServiceType,
                StartType,
                ErrorControl,
                BinaryPathName,
                LoadOrderGroup,
                NULL,
                DependencyList,
                ServiceStartName,
                Password,
                DisplayName
                );

        if(!b) {
            LogItem2(
                LogSevWarning,
                MSG_LOG_CHANGESVC_FAIL,
                ServiceName,
                MSG_LOG_X_RETURNED_WINERR,
                szChangeServiceConfig,
                GetLastError()
                );
        }
        CloseServiceHandle(hSCService);
    } else {
        b = FALSE;
        LogItem2(
            LogSevWarning,
            MSG_LOG_CHANGESVC_FAIL,
            ServiceName,
            MSG_LOG_X_RETURNED_WINERR,
            szOpenService,
            GetLastError()
            );
    }

    //
    // Unlock the database if locked and then close the service controller
    // handle
    //
    if(sclLock) {
        UnlockServiceDatabase(sclLock);
    }

    CloseServiceHandle(hSC);
    return(b);
}


BOOL
MyChangeServiceStart(
    IN PCWSTR ServiceName,
    IN DWORD  StartType
    )

/*++

Routine Description:

    Routine to change the start value of a service. This turns
    around and calls the stub to ChangeServiceConfig.

Arguments:

    ServiceName - Name of service

    StartType - Service Start value, e.g. SERVICE_BOOT_START

Return value:

    Boolean value indicating outcome.

--*/
{
    BOOL b;

    b = MyChangeServiceConfig(
                ServiceName,
                SERVICE_NO_CHANGE,
                StartType,
                SERVICE_NO_CHANGE,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL
                );

    return(b);
}


BOOL
MyStartService(
    IN PCWSTR ServiceName
    )
{
    SC_HANDLE hSC,hSCService;
    BOOL b;
    DWORD d;

    b = FALSE;
    //
    // Open a handle to the service controller manager
    //
    hSC = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
    if(hSC == NULL) {
        LogItem2(
            LogSevWarning,
            MSG_LOG_STARTSVC_FAIL,
            ServiceName,
            MSG_LOG_X_RETURNED_WINERR,
            szOpenSCManager,
            GetLastError()
            );
        return(FALSE);
    }

    if(hSCService = OpenService(hSC,ServiceName,SERVICE_START)) {
        b = StartService(hSCService,0,NULL);
        if(!b && ((d = GetLastError()) == ERROR_SERVICE_ALREADY_RUNNING)) {
            //
            // Service is already running.
            //
            b = TRUE;
        }
        if(!b) {
            LogItem2(
                LogSevWarning,
                MSG_LOG_STARTSVC_FAIL,
                ServiceName,
                MSG_LOG_X_PARAM_RETURNED_WINERR,
                szStartService,
                d,
                ServiceName
                );
        }
        CloseServiceHandle(hSCService);
    } else {
        b = FALSE;
        LogItem2(
            LogSevWarning,
            MSG_LOG_STARTSVC_FAIL,
            ServiceName,
            MSG_LOG_X_PARAM_RETURNED_WINERR,
            szOpenService,
            GetLastError(),
            ServiceName
            );
    }

    CloseServiceHandle(hSC);

    return(b);
}

BOOL
FixServiceDependency(
    IN PCWSTR ServiceName,
    IN PCWSTR OldDependencyName,
    IN PCWSTR NewDependencyName
    )
{
    ULONG     Error;
    HKEY      hKey;
    WCHAR     ServicePath[ MAX_PATH + 1 ];
    PBYTE     OldValueData;
    PBYTE     NewValueData;
    ULONG     OldValueSize;
    ULONG     NewValueSize;
    DWORD     Type;
    PBYTE     p,q;
    BOOL      ChangeDependencyList;

    //
    //  Open the key that describes the service
    //

    lstrcpy( ServicePath, szServicesKeyPath );
    ConcatenatePaths(ServicePath,ServiceName,sizeof( ServicePath )/sizeof( WCHAR ),NULL);

    Error = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                          ServicePath,
                          0,
                          KEY_READ | KEY_WRITE,
                          &hKey );

    if( Error != ERROR_SUCCESS ) {
        LogItem2(
            LogSevWarning,
            MSG_LOG_FIX_SERVICE_FAILED,
            ServiceName,
            MSG_LOG_X_PARAM_RETURNED_WINERR,
            szRegOpenKeyEx,
            Error,
            ServicePath
            );
         return( FALSE );
    }

    //
    //  Allocate a buffer for the old value data
    //

    OldValueSize = 0;
    Error = RegQueryValueEx(hKey,
                            szDependOnService,
                            NULL,
                            &Type,
                            NULL,
                            &OldValueSize);
    if( ( Error != ERROR_SUCCESS ) && ( Error != ERROR_MORE_DATA ) ) {
        LogItem2(
            LogSevWarning,
            MSG_LOG_FIX_SERVICE_FAILED,
            ServiceName,
            MSG_LOG_X_PARAM_RETURNED_WINERR,
            szRegQueryValueEx,
            Error,
            szDependOnService
            );
         RegCloseKey( hKey );
         return( FALSE );
    }

    OldValueData = MyMalloc( OldValueSize );
    if( OldValueData == NULL ) {
        LogItem2(
            LogSevWarning,
            MSG_LOG_FIX_SERVICE_FAILED,
            ServiceName,
            MSG_LOG_OUTOFMEMORY
            );
         RegCloseKey( hKey );
         return( FALSE );
    }

    //
    //  Read the value entry that lists the dependencies
    //

    Error = RegQueryValueEx(hKey,
                            szDependOnService,
                            NULL,
                            &Type,
                            OldValueData,
                            &OldValueSize);
    if( Error != ERROR_SUCCESS ) {
        LogItem2(
            LogSevWarning,
            MSG_LOG_FIX_SERVICE_FAILED,
            ServiceName,
            MSG_LOG_X_PARAM_RETURNED_WINERR,
            szRegQueryValueEx,
            Error,
            szDependOnService
            );
         MyFree( OldValueData );
         RegCloseKey( hKey );
         return( FALSE );
    }

    //
    //  Find out if the OldValueData, explicitly list OldDependencyName.
    //  If not, then the service depends on another service that depends
    //  on OlDependencyName, and in this case there is no need to change
    //  the dependency list.
    //
    p = OldValueData;
    ChangeDependencyList = FALSE;
    while( (ULONG)(p - OldValueData) < OldValueSize ) {
        if( ( lstrcmpi( (PWSTR)p, OldDependencyName ) == 0 ) ) {
            ChangeDependencyList = TRUE;
            break;
        }
        p += (lstrlen( (PWSTR)p ) + 1)*sizeof(WCHAR);
    }
    if( !ChangeDependencyList ) {
         MyFree( OldValueData );
         RegCloseKey( hKey );
         //
         // Let the caller think that the dependency list was fixed
         //
         return( TRUE );
    }

    //
    //  Allocate a buffer for the new value data
    //
    NewValueSize = OldValueSize -
                    ( lstrlen( OldDependencyName ) - lstrlen( NewDependencyName ) )*sizeof(WCHAR);

    NewValueData = MyMalloc( NewValueSize );
    if( NewValueData == NULL ) {
        LogItem2(
            LogSevWarning,
            MSG_LOG_FIX_SERVICE_FAILED,
            ServiceName,
            MSG_LOG_OUTOFMEMORY
            );
         MyFree( OldValueData );
         RegCloseKey( hKey );
         return( FALSE );
    }

    //
    //  Replace the old dependency name with the new one
    //
    p = OldValueData;
    q = NewValueData;

    lstrcpy( (PWSTR)q, NewDependencyName );
    q += (lstrlen( (PWSTR)q ) + 1)*sizeof(WCHAR);
    while( (ULONG)(p - OldValueData) < OldValueSize ) {
        if( ( lstrcmpi( (PWSTR)p, OldDependencyName ) != 0 ) &&
            ( lstrcmpi( (PWSTR)p, NewDependencyName ) != 0 )
          ) {
            lstrcpy( (PWSTR)q, (PWSTR)p );
            q += (lstrlen( (PWSTR)q ) + 1)*sizeof(WCHAR);
        }
        p += (lstrlen( (PWSTR)p ) + 1)*sizeof(WCHAR);
    }

    //
    //  Save the value entry with the new dependency name
    //
    Error = RegSetValueEx( hKey,
                           szDependOnService,
                           0,
                           REG_MULTI_SZ,
                           NewValueData,
                           q-NewValueData // NewValueSize
                         );

    if( Error != ERROR_SUCCESS ) {
        LogItem2(
            LogSevWarning,
            MSG_LOG_FIX_SERVICE_FAILED,
            ServiceName,
            MSG_LOG_X_PARAM_RETURNED_WINERR,
            szRegSetValueEx,
            Error,
            szDependOnService
            );
         MyFree( OldValueData );
         MyFree( NewValueData );
         RegCloseKey( hKey );
         return( FALSE );
    }

    //
    //  Free the allocated buffers
    //

    MyFree( OldValueData );
    MyFree( NewValueData );

    //
    //  Close the key
    //
    RegCloseKey( hKey );
    return( TRUE );
}


BOOL
UpdateServicesDependencies(
    IN HINF InfHandle
    )
{
    INFCONTEXT            InfContext;
    PCWSTR                OldServiceName,NewServiceName;
    BOOL                  b;
    SC_HANDLE             hSC, hSCService;
    LPENUM_SERVICE_STATUS DependentsList;
    DWORD                 BytesNeeded;
    DWORD                 ServicesReturned;
    HKEY                  hKey;
    ULONG                 Error;
    ULONG                 i;

    //
    // Iterate the [ServicesToRename] section in the inf.
    // Each line is the name of a dependecy service that needs to be renamed.
    //
    if(SetupFindFirstLine(InfHandle,szServicesToRename,NULL,&InfContext)) {
        b = TRUE;
    } else {
        LogItem1( LogSevWarning,
                  MSG_LOG_UPDATE_SERVICES_FAILED,
                  MSG_LOG_NO_SECTION,
                  szServicesToRename );
        return(FALSE);
    }

    //
    // Open a handle to the service controller manager
    //
    hSC = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
    if(hSC == NULL) {
        LogItem1(
            LogSevWarning,
            MSG_LOG_UPDATE_SERVICES_FAILED,
            MSG_LOG_X_RETURNED_WINERR,
            szOpenSCManager,
            GetLastError()
            );
        return(FALSE);
    }

    do {
        //
        // Fetch the name of a service that got renamed
        //
        if((OldServiceName = pSetupGetField(&InfContext,0))
        && (NewServiceName = pSetupGetField(&InfContext,1))) {

            //
            //  Create a dummy service that has the same name as the old service
            //  This is necessarey so that we can get a handle to this service,
            //  and pass it to EnumDependentServices to find out the services that
            //  depend on this one.
            //

            if( !MyCreateService( OldServiceName,
                                  NULL,
                                  SERVICE_WIN32_OWN_PROCESS,
                                  SERVICE_DISABLED,
                                  SERVICE_ERROR_NORMAL,
                                  L"%SystemRoot%\\System32\\dummy.exe",
                                  NULL,
                                  L"",
                                  NULL,
                                  NULL ) ) {

                LogItem1(
                    LogSevWarning,
                    MSG_LOG_UPDATE_SERVICES_FAILED,
                    MSG_LOG_CANT_CREATE_DUMMY_SERVICE,
                    OldServiceName
                    );

                b = FALSE;
                continue;
            }

            //
            //  Open the service that was just created
            //

            hSCService = OpenService(hSC,OldServiceName,SERVICE_ENUMERATE_DEPENDENTS | DELETE);
            if( hSCService == NULL) {
                Error = GetLastError();
                DbgPrint( "SYSSETUP: Unable to open service = %ls. Error = %d \n", OldServiceName, Error );
                LogItem1( LogSevWarning,
                          MSG_LOG_UPDATE_SERVICES_FAILED,
                          MSG_LOG_X_PARAM_RETURNED_WINERR,
                          szOpenService,
                          Error,
                          OldServiceName
                        );
                //
                //  Force deletion of the service cretated
                //
                b = FALSE;
                Error = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                      szServicesKeyPath,
                                      0,
                                      MAXIMUM_ALLOWED,
                                      &hKey );
                if( Error == ERROR_SUCCESS ) {
                    RegistryDelnode( hKey, OldServiceName );
                    RegCloseKey( hKey );
                }
                continue;
            }

            //
            //  Determine all services that depend on the service that was renamed
            //

            BytesNeeded = 0;
            ServicesReturned = 0;
            DependentsList = NULL;
            if( !EnumDependentServices( hSCService,
                                        SERVICE_ACTIVE | SERVICE_INACTIVE,
                                        DependentsList,
                                        0,
                                        &BytesNeeded,
                                        &ServicesReturned ) &&
                ( Error = GetLastError()) != ERROR_MORE_DATA ) {

                LogItem2( LogSevWarning,
                          MSG_LOG_UPDATE_SERVICES_PARAM_FAILED,
                          OldServiceName,
                          MSG_LOG_X_RETURNED_WINERR,
                          szEnumDependentService,
                          Error
                        );

                b = FALSE;
                goto delete_dummy_service;
            }

            DependentsList = MyMalloc( BytesNeeded );
            if( DependentsList == NULL ) {

                LogItem2(
                    LogSevWarning,
                    MSG_LOG_UPDATE_SERVICES_PARAM_FAILED,
                    OldServiceName,
                    MSG_LOG_OUTOFMEMORY
                    );

                b = FALSE;
                goto delete_dummy_service;
            }

            if( !EnumDependentServices( hSCService,
                                        SERVICE_ACTIVE | SERVICE_INACTIVE,
                                        DependentsList,
                                        BytesNeeded,
                                        &BytesNeeded,
                                        &ServicesReturned ) ) {

                LogItem2( LogSevWarning,
                          MSG_LOG_UPDATE_SERVICES_PARAM_FAILED,
                          OldServiceName,
                          MSG_LOG_X_RETURNED_WINERR,
                          szEnumDependentService,
                          GetLastError()
                        );

                MyFree( DependentsList );
                b = FALSE;
                goto delete_dummy_service;
            }

            for( i = 0; i < ServicesReturned; i++ ) {
                //
                //  Fix the dependency for this service
                //
                b = b && FixServiceDependency( DependentsList[i].lpServiceName,
                                               OldServiceName,
                                               NewServiceName );
            }
            MyFree( DependentsList );

delete_dummy_service:

            if( !DeleteService(hSCService) &&
                ((Error = GetLastError()) != ERROR_SERVICE_MARKED_FOR_DELETE)
              ) {
                DbgPrint( "SYSSETUP: Unable to delete service %ls. Error = %d \n", OldServiceName, Error );
#if 0
                //
                //  Force deletion of the dummy service
                //
                Error = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                      szServicesKeyPath,
                                      0,
                                      MAXIMUM_ALLOWED,
                                      &hKey );
                if( Error == ERROR_SUCCESS ) {
                    RegistryDelnode( hKey, OldServiceName );
                    RegCloseKey( hKey );
                }
#endif
            }
            CloseServiceHandle(hSCService);


        } else {
            LogItem1( LogSevWarning,
                      MSG_LOG_UPDATE_SERVICES_FAILED,
                      MSG_LOG_NO_SECTION,
                      szServicesToRename );
        }

    } while(SetupFindNextLine(&InfContext,&InfContext));

    CloseServiceHandle(hSC);
    return(b);
}
