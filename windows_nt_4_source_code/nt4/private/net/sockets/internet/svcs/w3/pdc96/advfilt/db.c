/*++

Copyright (c) 1996  Microsoft Corporation

This program is released into the public domain for any purpose.


Module Name:

    db.c

Abstract:

    This module implements the database routines for the authentication filter.

--*/

#include <windows.h>
#include <httpfilt.h>
#include <authfilt.h>

//
//  This is the name of the file that contains the username/password pairs.
//  It should be in the \inetsrv\server installation directory.
//

#define USER_LIST_FILE     "c:\\inetsrv\\userdb.txt"

//
//  Globals
//

CHAR * pszUserFile = NULL;


BOOL
ValidateUser(
    IN OUT CHAR * pszUserName,
    IN OUT CHAR * pszPassword,
    OUT    BOOL * pfValid
    )
/*++

Routine Description:

    Looks up the username and confirms the user is allowed access to the
    server

Arguments:

    pszUserName - The username to validate, will contain the mapped username
                  on return.  Must be at least SF_MAX_USERNAME
    pszPassword - The password for this user.  Will contain the mapped
                  password on return.  Must be at least SF_MAX_PASSWORD
    pfValid     - Set to TRUE if the user should be logged on.

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    BOOL fFound;
    CHAR achPassword[SF_MAX_PASSWORD];
    CHAR achNTUser[SF_MAX_USERNAME];
    CHAR achNTUserPassword[SF_MAX_PASSWORD];

    //
    //  Assume we're going to fail validation
    //

    *pfValid = FALSE;

    //
    //  Lookup the user in the cache, if that fails, get the user from the
    //  database and add the retrieved user to the cache
    //

    if ( !LookupUserInCache( pszUserName,
                             &fFound,
                             achPassword,
                             achNTUser,
                             achNTUserPassword ))
    {
        return FALSE;
    }

    if ( !fFound )
    {
        if ( !LookupUserInDb( pszUserName,
                              &fFound,
                              achPassword,
                              achNTUser,
                              achNTUserPassword ))
        {
            return FALSE;
        }

        if ( fFound )
        {
            AddUserToCache( pszUserName,
                            achPassword,
                            achNTUser,
                            achNTUserPassword );
        }
    }

    if ( !fFound )
    {
        DbgWrite(( DEST,
                   "[ValidateUser] Failed to find user %s\n",
                   pszUserName ));

        return TRUE;
    }

    //
    //  Do the passwords match?
    //

    if ( !strcmp( pszPassword, achPassword ))
    {
        //
        //  We have a match, map to the NT user and password
        //

        strcpy( pszUserName, achNTUser );
        strcpy( pszPassword, achNTUserPassword );
        *pfValid = TRUE;
    }

    return TRUE;
}

BOOL
InitializeUserDatabase(
    VOID
    )
/*++

Routine Description:

    Retrieves the userlist from the file.  If the users were coming from a
    database, this routine would connect to the database.

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    HANDLE hFile;
    DWORD  cbFile;
    DWORD  cMaxUsers;
    DWORD  cUsers;
    DWORD  cbRead;

    //
    //  Open and read the file.  The System account must have access to the
    //  file.
    //

    hFile = CreateFile( USER_LIST_FILE,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL );

    if ( hFile == INVALID_HANDLE_VALUE )
    {
        DbgWrite(( DEST,
                   "[InitializeUserDatabase] Error %d openning %s\n",
                   GetLastError(),
                   USER_LIST_FILE ));

        return FALSE;
    }

    cbFile = GetFileSize( hFile, NULL );

    if ( cbFile == (DWORD) -1 )
    {
        CloseHandle( hFile );
        return FALSE;
    }

    pszUserFile = LocalAlloc( LPTR, cbFile + 1 );

    if ( !pszUserFile )
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        CloseHandle( hFile );
        return FALSE;
    }

    if ( !ReadFile( hFile,
                    pszUserFile,
                    cbFile,
                    &cbRead,
                    NULL ))
    {
        CloseHandle( hFile );
        LocalFree( pszUserFile );

        return FALSE;
    }

    CloseHandle( hFile );

    //
    //  Zero terminate the file data
    //

    pszUserFile[cbRead] = '\0';

    return TRUE;
}

BOOL
LookupUserInDb(
    IN CHAR * pszUser,
    OUT BOOL * pfFound,
    OUT CHAR * pszPassword,
    OUT CHAR * pszNTUser,
    OUT CHAR * pszNTUserPassword
    )
/*++

Routine Description:

    Looks up the username in the database and returns the other attributes
    associated with this user

    The file data is not sorted to simulate the cost of an external database
    lookup.

Arguments:

    pszUserName - The username to find in the database (case insensitive)
    pfFound     - Set to TRUE if the specified user name was found in the
                  database
    pszPassword - The external password for the found user.  Buffer must be
                  at least SF_MAX_PASSWORD bytes.
    pszNTUser   - The NT username associated with this user, Buffer must be at
                  least SF_MAX_USERNAME bytes
    pszNTUserPassword - The password for NTUser. Buffer must be at least
                  SF_MAX_PASSWORD

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    CHAR * pch = pszUserFile;
    CHAR * pchEnd;
    DWORD  cchUser = strlen( pszUser );
    DWORD  cch;

    *pfFound = FALSE;

    //
    //  Find the external username.  We're expecting one user per line in
    //  the form:
    //
    //      username:password, NTUser:NTUserPassword
    //

    while ( pch && *pch )
    {
        while ( ISWHITE( *pch ) )
            pch++;

        if ( toupper( *pch ) == toupper( *pszUser ) &&
             !strnicmp( pszUser, pch, cchUser )     &&
             pch[cchUser] == ':' )
        {
            pch += cchUser + 1;
            goto Found;
        }

        pch = strchr( pch+1, '\n' );
    }

    //
    //  Not found
    //

    return TRUE;

Found:

    //
    //  Break out the external username
    //

    if ( !(pchEnd = strchr( pch, ',' )))
    {
        SetLastError( ERROR_INVALID_PASSWORDNAME );
        return FALSE;
    }

    cch = pchEnd - pch;

    if ( cch+1 > SF_MAX_PASSWORD )
    {
        SetLastError( ERROR_INVALID_PASSWORDNAME );
        return FALSE;
    }

    memcpy( pszPassword, pch, cch );
    pszPassword[cch] = '\0';

    pch = pchEnd + 1;

    //
    //  Get the NT username from the file
    //

    while ( ISWHITE( *pch ) )
        pch++;

    if ( !(pchEnd = strchr( pch, ':' )))
    {
        SetLastError( ERROR_BAD_USERNAME );
        return FALSE;
    }

    cch = pchEnd - pch;

    if ( cch+1 > SF_MAX_USERNAME )
    {
        SetLastError( ERROR_BAD_USERNAME );
        return FALSE;
    }

    memcpy( pszNTUser, pch, cch );
    pszNTUser[cch] = '\0';

    pch = pchEnd + 1;

    //
    //  Get the NT password from the file, look for a '\r' or '\n'
    //

    pchEnd = pch;

    while ( *pchEnd && *pchEnd != '\r' && *pchEnd != '\n' )
        pchEnd++;

    cch = pchEnd - pch;

    if ( cch+1 > SF_MAX_PASSWORD )
    {
        SetLastError( ERROR_INVALID_PASSWORDNAME );
        return FALSE;
    }

    memcpy( pszNTUserPassword, pch, cch );
    pszNTUserPassword[cch] = '\0';

    *pfFound = TRUE;

    return TRUE;
}

