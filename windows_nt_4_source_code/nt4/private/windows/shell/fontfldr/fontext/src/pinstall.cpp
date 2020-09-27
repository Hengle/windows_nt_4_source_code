///////////////////////////////////////////////////////////////////////////////
//
// pinstal.cpp
//      Explorer Font Folder extension routines
//     Install files into \<windows>\fonts directory
//
//
// History:
//      31 May 95 SteveCat
//          Ported to Windows NT and Unicode, cleaned up
//
//
// NOTE/BUGS
//
//  Copyright (C) 1992-1995 Microsoft Corporation
//
///////////////////////////////////////////////////////////////////////////////

//==========================================================================
//                              Include files
//==========================================================================

#include "priv.h"
#include "globals.h"

#include <shsemip.h>

#include "cpanel.h"
#include "resource.h"


/*****************************************************/
/******************** locals *************************/
/*****************************************************/

static DWORD NEAR PASCAL dwGetSetFileAttr( LPTSTR lpFileName, WORD wAttr );
static int   NEAR PASCAL iMsgRecoverable( DWORD dwVIF, PTSTR pFile );

/*****************************************************/
/******************** Defines ************************/
/*****************************************************/

#define RECOVERABLEERROR    (VIF_SRCOLD     |  VIF_DIFFLANG |  \
                             VIF_DIFFCODEPG |  VIF_DIFFTYPE)

#define UNRECOVERABLEERROR (VIF_FILEINUSE    | VIF_OUTOFSPACE   | \
                            VIF_CANNOTCREATE | VIF_OUTOFMEMORY  | \
                            VIF_CANNOTRENAME | VIF_CANNOTDELETE | \
                            VIF_CANNOTREADDST)
#define READONLY (1)

#ifndef DBCS
#define IsDBCSLeadByte(x) (FALSE)
#endif

/*****************************************************/
/******************** Functions **********************/
/*****************************************************/


int NEAR PASCAL iMsgRecoverable( DWORD dwVIF, PTSTR pszFile )
{
    // BGK - finish this one

#define NUMFLAGS 3

    TCHAR  cszErr[ NUMFLAGS ][ 128 ] = { TEXT( "" ), TEXT( "" ), TEXT( "" ) };

    static DWORD dwFlags[ NUMFLAGS ] =
                   { VIF_SRCOLD, VIF_DIFFLANG | VIF_DIFFCODEPG, VIF_DIFFTYPE };


    for( int nTemp = 0; nTemp < NUMFLAGS; ++nTemp )
        if( dwVIF & dwFlags[ nTemp ])

            // cszErr[ nTemp ].LoadString( INSTALL5+nTemp );

            LoadString( g_hInst, INSTALL5 + nTemp, cszErr[ nTemp ], 128 );

    return iUIMsgBox( INSTALL8, IDS_MSG_CAPTION,
                      MB_YESNOCANCEL|MB_DEFBUTTON2|MB_ICONEXCLAMATION,
                      pszFile, cszErr[ 0 ], cszErr[ 1 ], cszErr[ 2 ]);
}


DWORD FFInstallFile( DWORD   dwFlags,
                     LPCTSTR szFromName,
                     LPCTSTR szToName,
                     LPCTSTR szFromDir,
                     LPCTSTR szToDir,
                     LPCTSTR szWinDir,
                     LPTSTR  szTmpFile,
                     UINT   *puTmpLen )
{
    FullPathName_t szFrom;
    FullPathName_t szTo;

    PathCombine( szFrom, szFromDir, szFromName );
    PathCombine( szTo  , szToDir  , szToName  );

    OFSTRUCT of;

    HFILE hFrom = LZOpenFile( szFrom, &of, OF_READ );

    if (hFrom < 0)
    {
        return( VIF_CANNOTREADSRC );
    }

    HFILE hTo = LZOpenFile( szTo, &of, OF_CREATE | OF_WRITE );

    if (hTo < 0)
    {
        LZClose( hFrom );
        return( VIF_ACCESSVIOLATION );
    }

    LONG lCopy = LZCopy( hFrom, hTo );

    LZClose( hTo );

    LZClose( hFrom );

    switch( lCopy )
    {
    case LZERROR_WRITE:
        return( VIF_OUTOFSPACE );

    case LZERROR_GLOBALLOC:
    case LZERROR_GLOBLOCK:
        return( VIF_OUTOFMEMORY );

    default:
        if( lCopy < 0 )
        {
            //
            //  Return some unknown error
            //

            return( UNRECOVERABLEERROR );
        }
        break;
    }

    return( 0 );
}


//
//  This installs files into the system or windows directory
//
//  There is a lot of dead code here, since this is only used for installing
//  fonts now.  Oh well.
//

BOOL FAR PASCAL bCPInstallFile( LPTSTR lpszDir, LPTSTR lpszFrom, LPTSTR lpszTo )
{
    FullPathName_t  szShareDir;
    FullPathName_t  szAttrPath;
    FILENAME        szTmpFile;
    FILENAME        szFile;
                    
    TCHAR   szClass[ 40 ];

    UINT    wClass  = ARRAYSIZE( szClass );
    UINT    wTmpLen = ARRAYSIZE( szShareDir );


    WORD    wFlags;
    UINT    wFindStatus;
    DWORD   dwInsStatus;

    BOOL    bSuccess       = FALSE;
    BOOL    bToastTempFile = FALSE;

    static    FullPathName_t s_szWinDir;

    static    BOOL  s_bFirst = TRUE;


    if( s_bFirst )
    {
        GetWindowsDirectory( s_szWinDir, ARRAYSIZE( s_szWinDir ) );
        lpCPBackSlashTerm  ( s_szWinDir );
        s_bFirst = FALSE;
    }

    lstrcpy( e_szDirOfSrc, lpszDir );

    vCPStripBlanks( e_szDirOfSrc );

    //
    //  Find the file to see if it is running or currently installed.
    //

    //
    //  Interrogate the installation system to determine where we
    //  need to put the file.  We pass in szFile - the name of the
    //  file we're going to be creating.  This has no path specified.
    //  We know where we want to put the file, and therefore don't look
    //  at the recommended destination
    //

    lstrcpy( szFile, lpszTo );
    CharUpper( szFile );

    *szShareDir = 0;

    FullPathName_t szTempDir;

    GetFontsDirectory( szTempDir, ARRAYSIZE( szTempDir ) );
   
    wFindStatus = VerFindFile( VFFF_ISSHAREDFILE, szFile, NULL, szTempDir,
                                      szClass, &wClass, szShareDir, &wTmpLen );

    //
    //  Force an install in the fonts directory.
    //

    lstrcpy( szShareDir, szTempDir );

    //
    //  If the destination file's being used by windows, we can't do much.
    //  report and bug out.
    //

    if( wFindStatus & VFF_FILEINUSE )
    {
        iUIMsgInfo( INSTALL1, szFile );
        goto Backout;
    }

    //
    //  If the current file is in the windows directory, be sure to delete it
    //  Or else a file installed in the system dir will not be found
    //

    lpCPBackSlashTerm( szShareDir );

    if( lstrcmpi( szShareDir, s_szWinDir ) == 0 )
        wFlags = 0;
    else
        wFlags = VIFF_DONTDELETEOLD;

    lstrcpy( szFile, lpszFrom );

    CharUpper( szFile );

DoInstall:

    //
    //  input buffer should be empty
    //

    *szTmpFile = 0;

    wTmpLen = ARRAYSIZE( szTmpFile );

    dwInsStatus = FFInstallFile( wFlags, szFile, lpszTo, e_szDirOfSrc,
                                 szShareDir, s_szWinDir, szTmpFile,
                                 &wTmpLen );
    //
    //  Note if we created a temporary file, which we'll have to delete in
    //  the event of failure
    //

    bToastTempFile = ( dwInsStatus & VIF_TEMPFILE ) != 0;

    if( dwInsStatus & VIF_CANNOTREADSRC )
        iUIMsgInfo( INSTALL9 );

    //
    //  If the install failed because the file was write-protected, ask the
    //  user what to do.    A no means on to the next, a yes says reset our
    //  file attributes and try again
    //

    else if( dwInsStatus & VIF_WRITEPROT )
        switch( iUIMsgBox( INSTALL0, IDS_MSG_CAPTION,
                           MB_YESNOCANCEL | MB_DEFBUTTON2 | MB_ICONEXCLAMATION,
                           szFile ) )
        {
        case IDYES:
            lstrcpy( szAttrPath, szShareDir );

            lstrcat( szAttrPath, szFile );

            SetFileAttributes( szAttrPath, GetFileAttributes( szAttrPath ) 
                                              & ~FILE_ATTRIBUTE_READONLY );
            goto DoInstall;
        } // switch

    //
    //  If we had a recoverable error and we're not retrying ask the user
    //  what to do.    A no means go on to the next, and a yes means reset
    //  our force flag and retry.
    //

    else if( ( dwInsStatus & RECOVERABLEERROR )
               && !(wFlags & VIFF_FORCEINSTALL ) )
        switch( iMsgRecoverable( dwInsStatus, szFile ) )
        {
        case IDYES:
            //
            //  Try again, forcing
            //

            wFlags |= VIFF_FORCEINSTALL;
            goto DoInstall;
        }

    //
    //  These other states all need errors reported, then bug out
    //

    else if( dwInsStatus & VIF_FILEINUSE )
        iUIMsgInfo( INSTALL1, szFile );

    else if( dwInsStatus & VIF_OUTOFSPACE )
        iUIMsgInfo( INSTALL2, szFile );

    else if( dwInsStatus & VIF_OUTOFMEMORY )
        iUIMsgInfo( INSTALL3, szFile );

    else if( dwInsStatus & UNRECOVERABLEERROR )
        iUIMsgInfo( INSTALL4, szFile );

    else if( dwInsStatus )
        iUIMsgInfo( INSTALL9 );

    //
    //  We've reached this point because of a good status, so we don't have
    //  to delete the temporary file.
    //

    else
    {
        bToastTempFile = FALSE;
        bSuccess = TRUE;
    }

Backout:

    if( bToastTempFile )
        vCPDeleteFromSharedDir( szTmpFile );

    bToastTempFile = FALSE;

    return bSuccess;
}

