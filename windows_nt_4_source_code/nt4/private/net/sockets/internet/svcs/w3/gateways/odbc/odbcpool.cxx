/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    odbcpool.cxx

Abstract:

    Provides simple ODBC connection pooling for IDC.  The only keys used for
    the connection pooling is the datasource name, the username and
    password.  ODBC options and other connection state are not taken
    into consideration.

Author:

    John Ludeman (johnl)   01-Apr-1996

Revision History:
--*/

#include "dbgutil.h"

#include <w3p.hxx>
#include "tssched.hxx"
#include <tcpdll.hxx>
#include <odbcconn.hxx>
#include <parmlist.hxx>

#include <odbcmsg.h>
#include <odbcreq.hxx>

extern "C"
{
#include <string.h>
}

#if DBG
#define IDC_PRINTF( x )        { char buff[256]; wsprintf x; OutputDebugString( buff ); }
#else
#define IDC_PRINTF( x )
#endif

//
//  Default connection pool timeout
//

#define IDC_POOL_TIMEOUT                30

//
//  Globals
//

CRITICAL_SECTION g_csPoolLock;

LIST_ENTRY       g_PoolList;

DWORD            g_csecPoolTimeout = IDC_POOL_TIMEOUT;

DWORD            g_dwTimeoutID = 0;

//
// Various counters
//

DWORD g_cFree;
DWORD g_cUsed;

//
//  ODBC Connection pool item
//

class ODBC_CONN_POOL
{
public:

    ODBC_CONN_POOL( const CHAR * pszDataSource,
                    const CHAR * pszUsername,
                    const CHAR * pszPassword,
                    const CHAR * pszLoggedOnUser )
        : m_strDataSource( pszDataSource ),
          m_strUsername  ( pszUsername ),
          m_strPassword  ( pszPassword ),
          m_strLogon     ( pszLoggedOnUser ),
          m_fFree        ( TRUE ),
          m_TTL          ( 2 )
    {
    }

    ~ODBC_CONN_POOL()
    {
        m_odbcconn.Close();
    }

    BOOL Open( VOID )
    {
        return m_odbcconn.Open( m_strDataSource.QueryStr(),
                                m_strUsername.QueryStr(),
                                m_strPassword.QueryStr() );
    }

    BOOL IsValid( VOID ) const
        { return m_strDataSource.IsValid() &&
                 m_strUsername.IsValid()   &&
                 m_strPassword.IsValid()   &&
                 m_strLogon.IsValid(); }

    BOOL IsFree( VOID ) const
        { return m_fFree; }

    VOID MarkAsUsed( VOID )
        { m_fFree = FALSE; g_cFree--; g_cUsed++; }

    VOID MarkAsFree( VOID )
        { m_fFree = TRUE; g_cUsed--; g_cFree++; }

    const CHAR * QueryDataSource( VOID ) const
        { return m_strDataSource.QueryStr(); }

    const CHAR * QueryUsername( VOID ) const
        { return m_strUsername.QueryStr(); }

    const CHAR * QueryPassword( VOID ) const
        { return m_strPassword.QueryStr(); }

    const CHAR * QueryLoggedOnUser( VOID ) const
        { return m_strLogon.QueryStr(); }

    ODBC_CONNECTION * QueryOdbcConnection( VOID )
        { return &m_odbcconn; }

    DWORD DecrementTTL( VOID )
        { return --m_TTL; }

    VOID ResetTTL( VOID )
        { m_TTL = 2; }

    LIST_ENTRY m_ListEntry;

private:

    ODBC_CONNECTION m_odbcconn;
    STR             m_strDataSource;
    STR             m_strUsername;
    STR             m_strPassword;
    STR             m_strLogon;         // The NT account this request is using
    BOOL            m_fFree;
    DWORD           m_TTL;

};

VOID
IDCPoolScavenger(
    PVOID pContext
    );

BOOL
InitializeOdbcPool(
    VOID
    )
{
    DWORD  err;
    HKEY   hkey;

    InitializeListHead( &g_PoolList );
    InitializeCriticalSection( &g_csPoolLock );

    err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                        W3_PARAMETERS_KEY,
                        0,
                        KEY_READ,
                        &hkey );

    if ( !err )
    {
        g_csecPoolTimeout = ReadRegistryDword( hkey,
                                               IDC_POOL_CONN_TIMEOUT,
                                               IDC_POOL_TIMEOUT );

        RegCloseKey( hkey );
    }

    if ( g_csecPoolTimeout < 2 )
    {
        g_csecPoolTimeout = 2;
    }

    //
    //  Kick off the pool scavenger
    //

    IDCPoolScavenger( NULL );

    return TRUE;
}

VOID
TerminateOdbcPool(
    VOID
    )
{
    ODBC_CONN_POOL * pOCPool;

    if ( g_dwTimeoutID )
    {
        RemoveWorkItem( g_dwTimeoutID );
        g_dwTimeoutID = 0;
    }

    EnterCriticalSection( &g_csPoolLock );

    while ( !IsListEmpty( &g_PoolList ))
    {
        LIST_ENTRY * pEntry = g_PoolList.Flink;

        RemoveEntryList( pEntry );

        pOCPool = CONTAINING_RECORD( pEntry,
                                     ODBC_CONN_POOL,
                                     m_ListEntry );

        delete pOCPool;
    }

    LeaveCriticalSection( &g_csPoolLock );
    DeleteCriticalSection( &g_csPoolLock );
}

BOOL
OpenConnection(
    IN  ODBC_CONNECTION *   podbcconnNonPooled,
    OUT ODBC_CONNECTION * * ppodbcconnToUse,
    IN  BOOL                fPoolConnection,
    IN  const CHAR *        pszDataSource,
    IN  const CHAR *        pszUsername,
    IN  const CHAR *        pszPassword,
    IN  const CHAR *        pszLoggedOnUser
    )
/*++

Routine Description:

    This function opens an odbc connection, optionally from a pool of
    ODBC connections.



Arguments:

    podbcconnNonPooled - If pooling wasn't requested or the open failed, we
        use this odbc connection object
    ppodbcconnToUse - Receives pointer to either a pooled ODBC connection object
        or podbcconnNonPooled if a pooled object couldn't be used
    fPoolConnection - TRUE if connection pooling should be used, FALSE
        if podbcconnNonPooled should be used
    pszDataSource - ODBC Datasource
    pszUsername - Username for datasource access
    pszPassword - Password for use with this username
    pszLoggedOnUser - The NT account this user is running under

Return Value:

    TRUE if successful, FALSE on error

    ppodbcconnToUse will be set to the ODBC connection to use for the
        request

--*/
{
    LIST_ENTRY *     pEntry;
    ODBC_CONN_POOL * pOCPool;

    //
    //  Don't pool this connection if it wasn't requested
    //

    if ( !fPoolConnection )
    {
        *ppodbcconnToUse = podbcconnNonPooled;

        return podbcconnNonPooled->Open( pszDataSource,
                                         pszUsername,
                                         pszPassword );
    }

    //
    //  Look in the pool cache for an existing connection
    //

    EnterCriticalSection( &g_csPoolLock );

    for ( pEntry  = g_PoolList.Flink;
          pEntry != &g_PoolList;
          pEntry  = pEntry->Flink )
    {
        pOCPool = CONTAINING_RECORD( pEntry,
                                     ODBC_CONN_POOL,
                                     m_ListEntry );

        if ( pOCPool->IsFree() &&
             !_stricmp( pOCPool->QueryDataSource(), pszDataSource ) &&
             !_stricmp( pOCPool->QueryUsername(), pszUsername ) &&
             !_stricmp( pOCPool->QueryLoggedOnUser(), pszLoggedOnUser ) &&
             !strcmp( pOCPool->QueryPassword(),
                      pszPassword ))

        {
            //
            //  We have a match
            //

            pOCPool->MarkAsUsed();
            *ppodbcconnToUse = pOCPool->QueryOdbcConnection();
            pOCPool->ResetTTL();
            LeaveCriticalSection( &g_csPoolLock );

            return TRUE;
        }
    }

    LeaveCriticalSection( &g_csPoolLock );

    //
    //  Allocate a new connection pool and if we connect successfully, put
    //  it in the pool list
    //

    pOCPool = new ODBC_CONN_POOL( pszDataSource,
                                  pszUsername,
                                  pszPassword,
                                  pszLoggedOnUser );


    if ( !pOCPool->Open() )
    {
        delete pOCPool;

        *ppodbcconnToUse = podbcconnNonPooled;

        return podbcconnNonPooled->Open( pszDataSource,
                                         pszUsername,
                                         pszPassword );
    }

    *ppodbcconnToUse = pOCPool->QueryOdbcConnection();

    EnterCriticalSection( &g_csPoolLock );

    //
    //  Account for the new pool item but we have to do it with in
    //  the critical section
    //

    g_cFree++;

    pOCPool->MarkAsUsed();
    InsertHeadList( &g_PoolList, &pOCPool->m_ListEntry );

    LeaveCriticalSection( &g_csPoolLock );

    return TRUE;
}

VOID
CloseConnection(
    IN  ODBC_CONNECTION *   podbcconnPooled,
    IN  BOOL                fDelete
    )
/*++

Routine Description:

    This routine frees an ODBC connection back to the pool, optionally
    deleting it

Arguments:

    podbcconnPooled - ODBC connection that is pooled, can be NULL
    fDelete - TRUE if the item should be delete rather then returned to the
        pool
--*/
{
    LIST_ENTRY *     pEntry;
    ODBC_CONN_POOL * pOCPool;

    if ( !podbcconnPooled )
    {
        return;
    }

    //
    //  Look in the pool list to mark it as free
    //

    EnterCriticalSection( &g_csPoolLock );

    for ( pEntry  = g_PoolList.Flink;
          pEntry != &g_PoolList;
          pEntry  = pEntry->Flink )
    {
        pOCPool = CONTAINING_RECORD( pEntry,
                                     ODBC_CONN_POOL,
                                     m_ListEntry );

        if ( podbcconnPooled == pOCPool->QueryOdbcConnection() )
        {
            pOCPool->MarkAsFree();

            if ( fDelete )
            {
                RemoveEntryList( pEntry );
                g_cFree--;
                delete pOCPool;
            }

            break;
        }
    }

    LeaveCriticalSection( &g_csPoolLock );
}

VOID
IDCPoolScavenger(
    PVOID pContext
    )
/*++

Routine Description:

    Walks the list of pooled connections and removes any that have timed out

--*/
{
    LIST_ENTRY *     pEntry;
    LIST_ENTRY *     pNext;
    ODBC_CONN_POOL * pOCPool;

    //
    //  Look through the list and remove any old items
    //

    EnterCriticalSection( &g_csPoolLock );

    for ( pEntry  = g_PoolList.Flink;
          pEntry != &g_PoolList;
          pEntry  = pNext )
    {
        pNext = pEntry->Flink;

        pOCPool = CONTAINING_RECORD( pEntry,
                                     ODBC_CONN_POOL,
                                     m_ListEntry );

        if ( pOCPool->IsFree() && !pOCPool->DecrementTTL() )
        {
            IDC_PRINTF(( buff,
                         "[IDCPoolScavenger] Removing %s, %s, %s\n",
                         pOCPool->QueryDataSource(),
                         pOCPool->QueryUsername(),
                         pOCPool->QueryLoggedOnUser() ));

            RemoveEntryList( pEntry );
            g_cFree--;
            delete pOCPool;
        }
    }

    IDC_PRINTF(( buff,
                 "[IDCPoolScavenger] Free items in pool %d, used %d\n",
                 g_cFree,
                 g_cUsed ));

    LeaveCriticalSection( &g_csPoolLock );

    //
    //  Schedule the next scavenge period
    //

    g_dwTimeoutID = ScheduleWorkItem( IDCPoolScavenger,
                                      NULL,
                                      g_csecPoolTimeout * 1000 );
}
