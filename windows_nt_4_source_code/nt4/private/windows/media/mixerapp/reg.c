/*****************************************************************************
 *
 *  Component:  sndvol32.exe
 *  File:       reg.c
 *  Purpose:    registry access functions
 * 
 *  Copyright (C) Microsoft Corporation 1985-1995. All rights reserved.
 *
 *****************************************************************************/
#include <windows.h>
#include <regstr.h>

// Note: The following line generates a UNICODE error
//       so it has been replaced by the line after.
//const TCHAR szRegPath[] = REGSTR_PATH_WINDOWSAPPLETS "\\Volume Control";
const TCHAR szRegPath[] = TEXT ("Software\\Microsoft\\Windows\\CurrentVersion\\Applets\\Volume Control");


/* ReadRegistryData
 *
 * Reads information from the registry
 *
 * Parameters:
 *
 *     pEntryNode - The node under Media Player which should be opened
 *         for this data.  If this is NULL, the value is
 *         written directly under szRegPath.
 *
 *     pEntryName - The name of the value under pEntryNode to be retrieved.
 *
 *     pType - Pointer to a buffer to receive type of data read.  May be NULL.
 *
 *     pData - Pointer to a buffer to receive the value data.
 *
 *     Size - Size, in bytes, of the buffer pointed to by pData.
 *
 * Return:
 *
 *     Registry status return (NO_ERROR is good)
 *
 *
 * Andrew Bell (andrewbe) wrote it, 10 September 1992
 *
 */
DWORD ReadRegistryData( LPTSTR pEntryNode,
                        LPTSTR pEntryName,
                        PDWORD pType,
                        LPBYTE pData,
                        DWORD  DataSize )
{
    DWORD  Status;
    HKEY   hkeyRegPath;
    HKEY   hkeyEntryNode;
    DWORD  Size;

    /* Open the top-level node.  For Media Player this is:
     * "Software\\Microsoft\\Windows NT\\CurrentVersion\\Sound Recorder"
     */
    Status = RegOpenKeyEx( HKEY_CURRENT_USER, szRegPath, 0,
                           KEY_READ, &hkeyRegPath );

    if( Status == NO_ERROR )
    {
        /* Open the sub-node:
         */
        if( pEntryNode )
            Status = RegOpenKeyEx( hkeyRegPath, pEntryNode, 0,
                                   KEY_READ, &hkeyEntryNode );
        else
            hkeyEntryNode = hkeyRegPath;

        if( Status == NO_ERROR )
        {
            Size = DataSize;

            /* Read the entry from the registry:
             */
            Status = RegQueryValueEx( hkeyEntryNode,
                                      pEntryName,
                                      0,
                                      pType,
                                      pData,
                                      &Size );

            if( pEntryNode )
                RegCloseKey( hkeyEntryNode );
        }

        RegCloseKey( hkeyRegPath );
    }
    return Status;
}
DWORD QueryRegistryDataSize(
    LPTSTR  pEntryNode,
    LPTSTR  pEntryName,
    DWORD   *pDataSize )
{
    DWORD  Status;
    HKEY   hkeyRegPath;
    HKEY   hkeyEntryNode;
    DWORD  Size;

    /* Open the top-level node.  For Media Player this is:
     * "Software\\Microsoft\\Windows NT\\CurrentVersion\\Sound Recorder"
     */
    Status = RegOpenKeyEx( HKEY_CURRENT_USER, szRegPath, 0,
                           KEY_READ, &hkeyRegPath );

    if( Status == NO_ERROR )
    {
        /* Open the sub-node:
         */
        if( pEntryNode )
            Status = RegOpenKeyEx( hkeyRegPath, pEntryNode, 0,
                                   KEY_READ, &hkeyEntryNode );
        else
            hkeyEntryNode = hkeyRegPath;

        if( Status == NO_ERROR )
        {
            /* Read the entry from the registry:
             */
            Status = RegQueryValueEx( hkeyEntryNode,
                                      pEntryName,
                                      0,
                                      NULL,
                                      NULL,
                                      &Size );
            if (Status == NO_ERROR)
                *pDataSize = Size;

            if( pEntryNode )
                RegCloseKey( hkeyEntryNode );
        }

        RegCloseKey( hkeyRegPath );
    }
    return Status;
}
/* WriteRegistryData
 *
 * Writes a bunch of information to the registry
 *
 * Parameters:
 *
 *     pEntryNode - The node under szRegPath which should be created
 *         or opened for this data.  If this is NULL, the value is
 *         written directly under szRegPath.
 *
 *     pEntryName - The name of the value under pEntryNode to be set.
 *
 *     Type - Type of data to read (e.g. REG_SZ).
 *
 *     pData - Pointer to the value data to be written.  If this is NULL,
 *         the value under pEntryNode is deleted.
 *
 *     Size - Size, in bytes, of the buffer pointed to by pData.
 *
 *
 * This routine is fairly generic, apart from the name of the top-level node.
 *
 * The data are stored in the following registry tree:
 *
 * HKEY_CURRENT_USER
 *  �
 *  �� Software
 *      �
 *      �� Microsoft
 *          �
 *          �� Windows NT
 *              �
 *              �� CurrentVersion
 *                  �
 *                  �� Media Player
 *                      �
 *                      �� AVIVideo
 *                      �
 *                      �� DisplayPosition
 *                      �
 *                      �� SysIni
 *
 *
 * Return:
 *
 *     Registry status return (NO_ERROR is good)
 *
 *
 * Andrew Bell (andrewbe) wrote it, 10 September 1992
 *
 */
DWORD WriteRegistryData( LPTSTR pEntryNode,
                         LPTSTR pEntryName,
                         DWORD  Type,
                         LPBYTE pData,
                         DWORD  Size )
{
    DWORD  Status;
    HKEY   hkeyRegPath;
    HKEY   hkeyEntryNode;

    /* Open or create the top-level node.  For Media Player this is:
     * "Software\\Microsoft\\Windows NT\\CurrentVersion\\Media Player"
     */
    Status = RegCreateKeyEx( HKEY_CURRENT_USER, szRegPath, 0,
                             NULL, 0, KEY_WRITE, NULL, &hkeyRegPath, NULL );

    if( Status == NO_ERROR )
    {
        /* Open or create the sub-node.
         */
        if( pEntryNode )
            Status = RegCreateKeyEx( hkeyRegPath, pEntryNode, 0,
                                     NULL, 0, KEY_WRITE, NULL, &hkeyEntryNode, NULL );
        else
            hkeyEntryNode = hkeyRegPath;

        if( Status == NO_ERROR )
        {
            if( pData )
            {
                Status = RegSetValueEx( hkeyEntryNode,
                                        pEntryName,
                                        0,
                                        Type,
                                        pData,
                                        Size );

            }
            else
            {
                Status = RegDeleteValue( hkeyEntryNode, pEntryName );
            }

            if( pEntryNode )
                RegCloseKey( hkeyEntryNode );
        }

        RegCloseKey( hkeyRegPath );
    }


    return Status;
}
