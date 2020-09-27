/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :

      mimeutil.cxx

   Abstract:

      This module defines MIME utility functions: Initialize and Cleanup
        of global MimeMap. Also provides function for obtaining the MimeType
        for given file extension

   Author:

       Murali R. Krishnan    ( MuraliK )     23-Jan-1995

   Environment:

       Win32

   Project:

       TCP Internet services common dll

   Functions Exported:

       BOOL InitializeMimeMap( VOID)
       BOOL CleanupMimeMap( VOID)


   Revision History:

--*/


/************************************************************
 *     Include Headers
 ************************************************************/
# include <tcpdllp.hxx>

# include <tchar.h>
# include "mimemap.hxx"
# include "mainsupp.hxx"
# include "tsvcinfo.hxx"

# define   PSZ_MIME_MAP       TEXT( "MimeMap")

PMIME_MAP     g_pMimeMap = NULL;


/************************************************************
 *    Functions
 ************************************************************/


BOOL
InitializeMimeMap( IN LPCTSTR  pszRegEntry)
/*++

  Creates a new mime map object and loads the registry entries from
    under this entry from  \\MimeMap.


--*/
{
    BOOL fReturn = FALSE;

    DBG_ASSERT( g_pMimeMap == NULL);

    g_pMimeMap = new MIME_MAP();

    if ( g_pMimeMap != NULL) {

        if ( pszRegEntry == NULL ||
            _tcslen( pszRegEntry) + _tcslen( PSZ_MIME_MAP) >= MAX_PATH) {

            SetLastError( ERROR_INSUFFICIENT_BUFFER);
        } else {

            DWORD dwError;
            TCHAR  szMimeMapEntry[MAX_PATH];

            wsprintf( szMimeMapEntry, TEXT("%s\\%s"),
                      pszRegEntry, PSZ_MIME_MAP);
#ifdef CHICAGO
            //
            // JRA: This was #ifdef CHICAGO1. I assumed it's a typo (!)
            //

            dwError = g_pMimeMap->InitFromRegistry(TEXT("MIME\\Database\\Content Type"));
#else
            dwError = g_pMimeMap->InitFromRegistry( szMimeMapEntry);
#endif

            fReturn = ( dwError == NO_ERROR);
            if ( !fReturn) {
                SetLastError( dwError);
            }
        }
    }

    IF_DEBUG( MIME_MAP ) {

        DBGPRINTF( ( DBG_CONTEXT, "InitializeMimeMap() from Reg %s. returns %d."
                    " Error = %d\n",
                     PSZ_MIME_MAP, fReturn, GetLastError()));
    }

    return ( fReturn);
} // InitializeMimeMap()


BOOL
CleanupMimeMap( VOID)
{
    BOOL fReturn = TRUE;

    if ( g_pMimeMap != NULL) {

         delete g_pMimeMap;
         g_pMimeMap = NULL;
   }

   return ( fReturn);
} // CleanupMimeMap()




BOOL
SelectMimeMappingForFileExt(
    IN const LPTSVC_INFO   pTsvcInfo,
    IN const TCHAR *       pchFilePath,
    OUT STR *              pstrMimeType,          // optional
    OUT STR *              pstrIconFile)          // optional
/*++
   Locks and obtains the mime type and/or icon file
    for file based on the file extension.

  pTsvcInfo       pointer to service's tsvcinfo object
  pchFilePath     pointer to path for the given file
  pstrMimeType    pointer to string to store the mime type on return
                   ( if ! NULL)
  pstrIconFile    pointer to string to store the icon file name on return
                   ( if ! NULL)

  Returns:
     TRUE on success and
     FALSE if there is any error.

--*/
{
    BOOL  fReturn = TRUE;

    if ( pstrIconFile != NULL || pstrMimeType != NULL) {

        PMIME_MAP  pMm;
        PCMIME_MAP_ENTRY   pMmeMatch;

        DBG_ASSERT( pTsvcInfo);
        pMm = pTsvcInfo->QueryMimeMap();

        DBG_ASSERT( pMm != NULL);

        // No need to unlock/lock since nobody changes mime type on the fly.
        // pMm->LockThisForRead();

        pMmeMatch = pMm->LookupMimeEntryForFileExt( pchFilePath);
        DBG_ASSERT( pMmeMatch != NULL);

        if ( pstrIconFile != NULL) {

            fReturn = fReturn &&
                    pstrIconFile->Copy( pMmeMatch->QueryIconFile());
        }

        if ( pstrMimeType != NULL) {

            fReturn = fReturn &&
                    pstrMimeType->Copy( pMmeMatch->QueryMimeType());
        }

        // pMm->UnlockThis();
    }

    return ( fReturn);
} // SelectMimeMappingForFileExt()



/************************ End of File ***********************/
