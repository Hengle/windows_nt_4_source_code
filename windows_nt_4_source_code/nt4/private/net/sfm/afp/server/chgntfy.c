/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	chgntfy.c

Abstract:

	This module contains the code for processing change notifies.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	15 Jun 1995 JameelH	Seperated the change notify code from idindex.c

Notes:		Tab stop: 4

--*/

#define IDINDEX_LOCALS
#define	FILENUM	FILE_CHGNTFY

#include <afp.h>
#include <scavengr.h>
#include <fdparm.h>
#include <pathmap.h>
#include <afpinfo.h>
#include <access.h>	// for AfpWorldId

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, afpVerifyDFE)
#pragma alloc_text(PAGE, afpAddDfEntryAndCacheInfo)
#pragma alloc_text(PAGE, afpReadIdDb)
#pragma alloc_text(PAGE, AfpProcessChangeNotify)
#pragma alloc_text(PAGE, afpProcessPrivateNotify)
#pragma alloc_text(PAGE, AfpQueuePrivateChangeNotify)
#pragma alloc_text(PAGE, AfpCacheDirectoryTree)
#pragma alloc_text(PAGE, AfpQueueOurChange)
#pragma alloc_text(PAGE, AfpDequeueOurChange)
#endif


/***	afpVerifyDFE
 *
 *	Check if our view of this item in our data-base matches whats on disk. If not
 *  update our view with whats on disk.
 */
VOID
afpVerifyDFE(
	IN	struct _VolDesc *			pVolDesc,
	IN	PDFENTRY					pDfeParent,
	IN	PUNICODE_STRING				pUName,			// munged unicode name
	IN	PFILESYSHANDLE				pfshParentDir,	// open handle to parent directory
	IN	PFILE_BOTH_DIR_INFORMATION	pFBDInfo,		// from enumerate
	IN	PUNICODE_STRING				pNotifyPath,	// to filter out our own AFP_AfpInfo change notifies
	IN	PDFENTRY	*				ppDfEntry
)
{
	PDFENTRY	pDfEntry = *ppDfEntry;

	if (pFBDInfo->LastWriteTime.QuadPart > pDfEntry->dfe_LastModTime.QuadPart)
	{
		FILESYSHANDLE	fshAfpInfo, fshData;
		AFPINFO			AfpInfo;
        DWORD			crinfo, openoptions = 0;
		BOOLEAN			SeenComment, WriteBackROAttr = False;
        PSTREAM_INFO	pStreams = NULL, pCurStream;
		NTSTATUS		Status = STATUS_SUCCESS;
		BOOLEAN			IsDir;

		// Our view is stale, update it
		DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
				("afpVerifyDFE: Updating stale database with fresh info\n\t%Z\n", pNotifyPath));

		IsDir = (pFBDInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? True : False;

		ASSERT (IS_VOLUME_NTFS(pVolDesc));

		ASSERT (!(DFE_IS_DIRECTORY(pDfEntry) ^ IsDir));

		// Update DFE from FBDInfo first
		pDfEntry->dfe_CreateTime = AfpConvertTimeToMacFormat(&pFBDInfo->CreationTime);
		pDfEntry->dfe_LastModTime = pFBDInfo->LastWriteTime;
		pDfEntry->dfe_NtAttr = (USHORT)(pFBDInfo->FileAttributes & FILE_ATTRIBUTE_VALID_FLAGS);
		if (!IsDir)
		{
			pDfEntry->dfe_DataLen = pFBDInfo->EndOfFile.LowPart;
		}

		// Open/Create the AfpInfo stream
		fshAfpInfo.fsh_FileHandle = NULL;
		fshData.fsh_FileHandle	= NULL;

		do
		{
			// open or create the AfpInfo stream
			if (!NT_SUCCESS(AfpCreateAfpInfoWithNodeName(pfshParentDir,
														 pUName,
														 pNotifyPath,
														 pVolDesc,
														 &fshAfpInfo,
														 &crinfo)))
			{
				if (!(pFBDInfo->FileAttributes & FILE_ATTRIBUTE_READONLY))
				{
					// What other reason is there that we could not open
					// this stream except that this file/dir is readonly?
					Status = STATUS_UNSUCCESSFUL;
					break;
				}

				openoptions = IsDir ? FILEIO_OPEN_DIR : FILEIO_OPEN_FILE;
				Status = STATUS_UNSUCCESSFUL;	// Assume failure
				if (NT_SUCCESS(AfpIoOpen(pfshParentDir,
										 AFP_STREAM_DATA,
										 openoptions,
										 pUName,
										 FILEIO_ACCESS_NONE,
										 FILEIO_DENY_NONE,
										 False,
										 &fshData)))
				{
					if (NT_SUCCESS(AfpExamineAndClearROAttr(&fshData,
															&WriteBackROAttr,
															pVolDesc,
															pNotifyPath)))
					{
						if (NT_SUCCESS(AfpCreateAfpInfo(&fshData,
														&fshAfpInfo,
														&crinfo)))
						{
							Status = STATUS_SUCCESS;
						}
					}
				}

				if (!NT_SUCCESS(Status))
				{
					// Skip this entry if you cannot get to the AfpInfo, cannot
					// clear the RO attribute or whatever.
					break;
				}
			}

			// We successfully opened or created the AfpInfo stream.  If
			// it existed, then validate the ID, otherwise create all new
			// Afpinfo for this file/dir.
			if ((crinfo == FILE_OPENED) &&
				(NT_SUCCESS(AfpReadAfpInfo(&fshAfpInfo, &AfpInfo))))
			{
				BOOLEAN	fSuccess;

				if ((AfpInfo.afpi_Id != pDfEntry->dfe_AfpId) &&
					(pDfEntry->dfe_AfpId != AFP_ID_ROOT))
				{
					PDFENTRY	pDFE;

					DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
							("afpVerifyDFE: IdDb Id does not match the AfpInfo Id!!!\n"));

					// Unlink it from the hash-bucket since we have the wrong id.
					AfpUnlinkDouble(pDfEntry, dfe_NextOverflow, dfe_PrevOverflow);

					// If some other entity has this id, then assign a new one. Else
					// use the one from the AfpInfo stream.
					pDFE = AfpFindDfEntryById(pVolDesc, AfpInfo.afpi_Id, DFE_ANY);
					if (pDFE != NULL)
					{
						pDfEntry->dfe_AfpId = afpGetNextId(pVolDesc);
					}
					else
					{
						pDfEntry->dfe_AfpId = AfpInfo.afpi_Id;
					}

					// Re-insert it with the new id
                    afpInsertDFEInHashBucket(pVolDesc, pDfEntry, IsDir, &fSuccess);
				}

				// NOTE: should we set the finder invisible bit if the
				// hidden attribute is set so system 6 will obey the
				// hiddenness in finder?
				pDfEntry->dfe_FinderInfo = AfpInfo.afpi_FinderInfo;
				pDfEntry->dfe_BackupTime = AfpInfo.afpi_BackupTime;
				pDfEntry->dfe_AfpAttr = AfpInfo.afpi_Attributes;
			}
			else
			{
				// AfpInfo stream was newly created, or we could not read
				// the existing one because it was corrupt.  Create new
				// info for this file/dir. Trust the version from the IdDb
				AfpInitAfpInfo(&AfpInfo, pDfEntry->dfe_AfpId, IsDir, pDfEntry->dfe_BackupTime);
				AfpInfo.afpi_FinderInfo = pDfEntry->dfe_FinderInfo;
				AfpInfo.afpi_Attributes = pDfEntry->dfe_AfpAttr;
				if (IsDir)
				{
					// Keep track of see files vs. see folders
					AfpInfo.afpi_AccessOwner = DFE_OWNER_ACCESS(pDfEntry);
					AfpInfo.afpi_AccessGroup = DFE_GROUP_ACCESS(pDfEntry);
					AfpInfo.afpi_AccessWorld = DFE_WORLD_ACCESS(pDfEntry);
				}
				else
				{
					AfpProDosInfoFromFinderInfo(&AfpInfo.afpi_FinderInfo,
												&AfpInfo.afpi_ProDosInfo);
					pDfEntry->dfe_RescLen = 0;	// Assume no resource fork

					// if this is a Mac application, make sure there is an APPL
					// mapping for it.
					if (AfpInfo.afpi_FinderInfo.fd_TypeD == *(PDWORD)"APPL")
					{
						AfpAddAppl(pVolDesc,
								   pDfEntry->dfe_FinderInfo.fd_CreatorD,
								   0,
								   pDfEntry->dfe_AfpId,
								   True,
								   pDfEntry->dfe_Parent->dfe_AfpId);
					}
				}

				Status = AfpWriteAfpInfo(&fshAfpInfo, &AfpInfo);
				if (!NT_SUCCESS(Status))
				{
					// We failed to write the AfpInfo stream;
					Status = STATUS_UNSUCCESSFUL;
					break;
				}
			}

			// Check for comment and resource stream
			pStreams = AfpIoQueryStreams(&fshAfpInfo);
			if (pStreams == NULL)
			{
				Status = STATUS_NO_MEMORY;
				break;
			}

			for (pCurStream = pStreams, SeenComment = False;
				 pCurStream->si_StreamName.Buffer != NULL;
				 pCurStream++)
			{
				if (IS_COMMENT_STREAM(&pCurStream->si_StreamName))
				{
					DFE_SET_COMMENT(pDfEntry);
					SeenComment = True;
					if (IsDir)
						break;	// Scan no further for directories
				}
				else if (!IsDir && IS_RESOURCE_STREAM(&pCurStream->si_StreamName))
				{
					pDfEntry->dfe_RescLen = pCurStream->si_StreamSize.LowPart;
					if (SeenComment)
						break;	// We have all we need to
				}
			}

		} while (False);

		if (fshData.fsh_FileHandle != NULL)
		{
			AfpPutBackROAttr(&fshData, WriteBackROAttr);
			AfpIoClose(&fshData);
		}

		if (fshAfpInfo.fsh_FileHandle != NULL)
			AfpIoClose(&fshAfpInfo);

		if (pStreams != NULL)
			AfpFreeMemory(pStreams);

		if (!NT_SUCCESS(Status))
		{
			*ppDfEntry = NULL;
		}
		else
		{
			// If this is the root directory, make sure we do not blow away the
			// AFP_VOLUME_HAS_CUSTOM_ICON bit for NTFS Volume.
			if (DFE_IS_ROOT(pDfEntry) &&
				(pVolDesc->vds_Flags & AFP_VOLUME_HAS_CUSTOM_ICON))
			{
				// Don't bother writing back to disk since we do not
				// try to keep this in sync in the permanent afpinfo
				// stream with the actual existence of the icon<0d> file.
				pDfEntry->dfe_FinderInfo.fd_Attr1 |= FINDER_FLAG_HAS_CUSTOM_ICON;
			}
		}
	}
}


/***	afpAddDfEntryAndCacheInfo
 *
 *	During the initial sync with disk on volume startup, add each entry
 *  we see during enumerate to the id index database.
 */
VOID
afpAddDfEntryAndCacheInfo(
	IN	PVOLDESC					pVolDesc,
	IN	PDFENTRY					pParentDfe,
	IN	PUNICODE_STRING 			pUName,			// munged unicode name
	IN  PFILESYSHANDLE				pfshEnumDir,	// open handle to parent directory
    IN	PFILE_BOTH_DIR_INFORMATION	pFBDInfo,		// from enumerate
	IN	PUNICODE_STRING				pNotifyPath, 	// For Afpinfo Stream
	IN	PDFENTRY	*				ppDfEntry,
	IN	BOOLEAN						CheckDuplicate
)
{
	BOOLEAN			IsDir, SeenComment, WriteBackROAttr = False;
	NTSTATUS		Status = STATUS_SUCCESS;
	FILESYSHANDLE		fshAfpInfo, fshData;
	DWORD			crinfo, openoptions = 0;
	PDFENTRY		pDfEntry;
	AFPINFO			AfpInfo;
	FINDERINFO		FinderInfo;
	PSTREAM_INFO		pStreams = NULL, pCurStream;
    DWORD           NTAttr = 0;
    TIME            ModTime;
    DWORD           ModMacTime;


	PAGED_CODE();

	fshAfpInfo.fsh_FileHandle = NULL;
	fshData.fsh_FileHandle	= NULL;

	IsDir = (pFBDInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? True : False;

	do
	{
		if (IS_VOLUME_NTFS(pVolDesc))
		{
			if (IsDir || CheckDuplicate)
			{
				// Make sure we don't already have this item in our database.
				// Multiple notifies for the same item could possibly occur if
				// the PC is renaming or moving items around on the disk, or
				// copying trees while we are trying to cache it, since we are
				// also queueing up private notifies for directories.

				afpFindDFEByUnicodeNameInSiblingList_CS(pVolDesc,
														pParentDfe,
														pUName,
														&pDfEntry,
														IsDir ? DFE_DIR : DFE_FILE);
				if (pDfEntry != NULL)
				{
					Status = AFP_ERR_OBJECT_EXISTS;
					DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_ERR,
							("afpAddDfEntryAndCacheInfo: Attempt to add a duplicate entry: %Z\n", pUName));
					break;
				}
			}

			openoptions = IsDir ? FILEIO_OPEN_DIR : FILEIO_OPEN_FILE;
			if (NT_SUCCESS(AfpIoOpen(pfshEnumDir,
									 AFP_STREAM_DATA,
									 openoptions,
									 pUName,
									 FILEIO_ACCESS_NONE,
									 FILEIO_DENY_NONE,
									 False,
									 &fshData)))
			{
                // save the LastModify time on the data stream of the file: we need to restore it
                AfpIoQueryTimesnAttr(&fshData,
                                     NULL,
                                     &ModTime,
                                     &NTAttr);

                ModMacTime = AfpConvertTimeToMacFormat(&ModTime);
            }
            else
            {
                // if we can't open the data file, just skip this entry!
                Status = STATUS_UNSUCCESSFUL;
	            DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_ERR,
	                ("Couldn't open data stream for %Z\n", pUName));
                break;
            }

			// open or create the AfpInfo stream
			if (!NT_SUCCESS(AfpCreateAfpInfoWithNodeName(pfshEnumDir,
														 pUName,
														 pNotifyPath,
														 pVolDesc,
														 &fshAfpInfo,
														 &crinfo)))
			{
				if (!(pFBDInfo->FileAttributes & FILE_ATTRIBUTE_READONLY))
				{
					// What other reason is there that we could not open
					// this stream except that this file/dir is readonly?
					Status = STATUS_UNSUCCESSFUL;
					break;
				}

				Status = STATUS_UNSUCCESSFUL;	// Assume failure
				if (NT_SUCCESS(AfpExamineAndClearROAttr(&fshData,
														&WriteBackROAttr,
														pVolDesc,
														pNotifyPath)))
				{
					if (NT_SUCCESS(AfpCreateAfpInfo(&fshData,
													&fshAfpInfo,
													&crinfo)))
					{
						Status = STATUS_SUCCESS;
					}
				}

				if (!NT_SUCCESS(Status))
				{
					// Skip this entry if you cannot get to the AfpInfo, cannot
					// clear the RO attribute or whatever.
					break;
				}
			}

			// We successfully opened or created the AfpInfo stream.  If
			// it existed, then validate the ID, otherwise create all new
			// Afpinfo for this file/dir.
			if ((crinfo == FILE_OPENED) &&
				(NT_SUCCESS(AfpReadAfpInfo(&fshAfpInfo, &AfpInfo))))
			{
				// the file/dir had an AfpInfo stream on it
				afpCheckDfEntry(pVolDesc,
								AfpInfo.afpi_Id,
								pUName,
								IsDir,
								pParentDfe,
								&pDfEntry);

				if (pDfEntry == NULL)
				{
					Status = STATUS_UNSUCCESSFUL;
					break;
				}

				if (pDfEntry->dfe_AfpId != AfpInfo.afpi_Id)
				{
					// Write out the AFP_AfpInfo with the new AfpId
					// and uninitialized icon coordinates
					AfpInfo.afpi_Id = pDfEntry->dfe_AfpId;
					AfpInfo.afpi_FinderInfo.fd_Location[0] =
					AfpInfo.afpi_FinderInfo.fd_Location[1] =
					AfpInfo.afpi_FinderInfo.fd_Location[2] =
					AfpInfo.afpi_FinderInfo.fd_Location[3] = 0xFF;
					AfpInfo.afpi_FinderInfo.fd_Attr1 &= ~FINDER_FLAG_SET;

					if (!NT_SUCCESS(AfpWriteAfpInfo(&fshAfpInfo, &AfpInfo)))
					{
						// We failed to write the AfpInfo stream;
						// delete the thing from the database
						AfpDeleteDfEntry(pVolDesc, pDfEntry);
						Status = STATUS_UNSUCCESSFUL;
						break;
					}
				}

				// NOTE: should we set the finder invisible bit if the
				// hidden attribute is set so system 6 will obey the
				// hiddenness in finder?
				pDfEntry->dfe_FinderInfo = AfpInfo.afpi_FinderInfo;
				pDfEntry->dfe_BackupTime = AfpInfo.afpi_BackupTime;
				pDfEntry->dfe_AfpAttr = AfpInfo.afpi_Attributes;
			}
			else
			{
				// AfpInfo stream was newly created, or we could not read
				// the existing one because it was corrupt.  Create new
				// info for this file/dir
				pDfEntry = AfpAddDfEntry(pVolDesc,
										pParentDfe,
										pUName,
										IsDir,
										0);
				if (pDfEntry == NULL)
				{
					Status = STATUS_UNSUCCESSFUL;
					break;
				}

				if (NT_SUCCESS(AfpSlapOnAfpInfoStream(pVolDesc,
													  pNotifyPath,
													  NULL,
													  &fshAfpInfo,
													  pDfEntry->dfe_AfpId,
													  IsDir,
													  pUName,
													  &AfpInfo)))
				{
					// NOTE: should we set the finder invisible bit if the
					// hidden attribute is set so system 6 will obey the
					// hiddenness in finder?
					pDfEntry->dfe_FinderInfo = AfpInfo.afpi_FinderInfo;
					pDfEntry->dfe_BackupTime = AfpInfo.afpi_BackupTime;
					pDfEntry->dfe_AfpAttr = AfpInfo.afpi_Attributes;
				}
				else
				{
					// We failed to write the AfpInfo stream;
					// delete the thing from the database
					AfpDeleteDfEntry(pVolDesc, pDfEntry);
					Status = STATUS_UNSUCCESSFUL;
					break;
				}
			}

			ASSERT(pDfEntry != NULL);

			if (IsDir)
			{
				// Keep track of see files vs. see folders
				DFE_OWNER_ACCESS(pDfEntry) = AfpInfo.afpi_AccessOwner;
				DFE_GROUP_ACCESS(pDfEntry) = AfpInfo.afpi_AccessGroup;
				DFE_WORLD_ACCESS(pDfEntry) = AfpInfo.afpi_AccessWorld;
			}
			else
			{
				// it's a file

				pDfEntry->dfe_RescLen = 0;	// Assume no resource fork

				// if this is a Mac application, make sure there is an APPL
				// mapping for it.
				if (AfpInfo.afpi_FinderInfo.fd_TypeD == *(PDWORD)"APPL")
				{
					AfpAddAppl(pVolDesc,
							   AfpInfo.afpi_FinderInfo.fd_CreatorD,
							   0,
							   pDfEntry->dfe_AfpId,
							   True,
							   pDfEntry->dfe_Parent->dfe_AfpId);
				}
			}

			// Check for comment and resource stream
			pStreams = AfpIoQueryStreams(&fshAfpInfo);
			if (pStreams == NULL)
			{
				Status = STATUS_NO_MEMORY;
				break;
			}

			for (pCurStream = pStreams, SeenComment = False;
				 pCurStream->si_StreamName.Buffer != NULL;
				 pCurStream++)
			{
				if (IS_COMMENT_STREAM(&pCurStream->si_StreamName))
				{
					DFE_SET_COMMENT(pDfEntry);
					SeenComment = True;
					if (IsDir)
						break;	// Scan no further for directories
				}
				else if (!IsDir && IS_RESOURCE_STREAM(&pCurStream->si_StreamName))
				{
					pDfEntry->dfe_RescLen = pCurStream->si_StreamSize.LowPart;
					if (SeenComment)
						break;	// We have all we need to
				}
			}
			AfpFreeMemory(pStreams);
		}
		else // CDFS
		{
			pDfEntry = AfpAddDfEntry(pVolDesc,
									 pParentDfe,
									 pUName,
									 IsDir,
									 0);

            if (pDfEntry == NULL)
            {
                Status = STATUS_UNSUCCESSFUL;
                break;
            }

            RtlZeroMemory(&FinderInfo, sizeof(FINDERINFO));
            RtlZeroMemory(&pDfEntry->dfe_FinderInfo, sizeof(FINDERINFO));
			pDfEntry->dfe_BackupTime = BEGINNING_OF_TIME;
			pDfEntry->dfe_AfpAttr = 0;

			if (IsDir)
			{
				DFE_OWNER_ACCESS(pDfEntry) = (DIR_ACCESS_SEARCH | DIR_ACCESS_READ);
				DFE_GROUP_ACCESS(pDfEntry) = (DIR_ACCESS_SEARCH | DIR_ACCESS_READ);
				DFE_WORLD_ACCESS(pDfEntry) = (DIR_ACCESS_SEARCH | DIR_ACCESS_READ);
			}

			if (IS_VOLUME_CD_HFS(pVolDesc))
			{
    		    Status = AfpIoOpen(pfshEnumDir,
				            AFP_STREAM_DATA,
				            openoptions,
				            pUName,
				            FILEIO_ACCESS_NONE,
				            FILEIO_DENY_NONE,
				            False,
				            &fshData);
			    if (!NT_SUCCESS(Status))
			    {
				    DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_ERR,
				      ("afpAddDfEntryAndCacheInfo: AfpIoOpen on %Z failed (%lx) for CD_HFS\n", pUName,Status));
			        break;
			    }

			    if (NT_SUCCESS(Status = AfpReadCdHfsAfpInfo(&fshData,
					        (PHFS_INFORMATION_BUFFER)&FinderInfo)))
			    {
    			    pDfEntry->dfe_FinderInfo = FinderInfo;
			        pDfEntry->dfe_AfpAttr = (FinderInfo.fd_Attr1 << 8) | (FinderInfo.fd_Attr2);

			        if (pVolDesc->vds_Flags & AFP_VOLUME_HAS_CUSTOM_ICON)
			        {
        			    pDfEntry->dfe_FinderInfo.fd_Attr1 |= FINDER_FLAG_HAS_CUSTOM_ICON;
			        }
			    }
			    else
			    {
    			    DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_ERR,
			            ("AfpReadCdHfsInfo failed with %lx for CD_HFS: no finder info\n", Status));
			    }


			    pDfEntry->dfe_RescLen = 0;

			    // if it's a file...

                // BUGBUG: can there be comments on directories on a CD?  If so, Logicraft needs
                // to implement that.
                if (!IsDir)
                {
    		        // if this is a Mac application, make sure there is an APPL
    			    // mapping for it.
    			    if (FinderInfo.fd_TypeD == *(PDWORD)"APPL")
			        {
    			        AfpAddAppl( pVolDesc,
				    	            FinderInfo.fd_CreatorD,
				                    0,
    				                pDfEntry->dfe_AfpId,
	    			                True,
		    		                pDfEntry->dfe_Parent->dfe_AfpId);
			        }

			        // Check for resource stream
			        pStreams = AfpIoQueryStreams(&fshData);
			        if (pStreams == NULL)
			        {
        			    Status = STATUS_NO_MEMORY;
			            break;
			        }

    			    for (pCurStream = pStreams;
        			    pCurStream->si_StreamName.Buffer != NULL;
		    	        pCurStream++)
			        {
    			        if (IS_RESOURCE_STREAM(&pCurStream->si_StreamName))
			            {
    			            pDfEntry->dfe_RescLen = pCurStream->si_StreamSize.LowPart;
				            break;
			            }
			        }

			        AfpFreeMemory(pStreams);
                }
			}
			// BUGBUG: if CdHfs doesn't have finder info, should we check for
			// that and set it ourselves using AfpSetFinderInfoByExtension?

			else
			{
    		    AfpSetFinderInfoByExtension(pUName, &pDfEntry->dfe_FinderInfo);
			    pDfEntry->dfe_RescLen = 0;
			}
			
		}

		// Record common NTFS & CDFS information
		pDfEntry->dfe_CreateTime = AfpConvertTimeToMacFormat(&pFBDInfo->CreationTime);
		pDfEntry->dfe_LastModTime = pFBDInfo->LastWriteTime;
		pDfEntry->dfe_NtAttr = (USHORT)pFBDInfo->FileAttributes & FILE_ATTRIBUTE_VALID_FLAGS;

        // if this is an HFS volume, check if Finder says file should be invisible
		if (IS_VOLUME_CD_HFS(pVolDesc))
        {
            if (pDfEntry->dfe_FinderInfo.fd_Attr1 & FINDER_FLAG_INVISIBLE)
            {
		        pDfEntry->dfe_NtAttr |= FILE_ATTRIBUTE_HIDDEN;
            }
        }

		if (!IsDir)
		{
			pDfEntry->dfe_DataLen = pFBDInfo->EndOfFile.LowPart;
		}

		ASSERT(pDfEntry != NULL);
	} while (False); // error handling loop

	if (fshAfpInfo.fsh_FileHandle != NULL)
    {
		AfpIoClose(&fshAfpInfo);

        // NTFS will set the LastModify time to current time on file since we modified the
        // AfpInfo stream: restore the original time
		if (!IsDir)
        {
            // force ntfs to "flush" any pending time updates before we reset the time
            AfpIoChangeNTModTime(&fshData,
                                 &ModTime);

            AfpIoSetTimesnAttr(&fshData,
                               NULL,
                               &ModMacTime,
                               0,
                               0,
                               NULL,
                               NULL);
        }
    }

	if (fshData.fsh_FileHandle != NULL)
	{
		if (IS_VOLUME_NTFS(pVolDesc))
		{
		    AfpPutBackROAttr(&fshData, WriteBackROAttr);
		}
		AfpIoClose(&fshData);
	}


	if (!NT_SUCCESS(Status))
	{
		pDfEntry = NULL;
	}

	*ppDfEntry = pDfEntry;
}


/***	afpReadIdDb
 *
 *	This is called when a volume is added, if an existing Afp_IdIndex stream
 *	is found on the root of the volume.  The stream is is read in, the
 *	VolDesc is initialized with the header image on disk, and the
 *	IdDb sibling tree/hash tables are created from the data on disk.
 *
**/
LOCAL NTSTATUS FASTCALL
afpReadIdDb(
	IN	PVOLDESC		pVolDesc,
	IN	PFILESYSHANDLE	pfshIdDb
)
{
	PBYTE					pReadBuf;
	PIDDBHDR				pIdDbHdr;
	NTSTATUS				Status;
	LONG					SizeRead = 0, Count;
	FORKOFFST				ForkOffst;
	UNICODE_STRING			uName;
	UNALIGNED DISKENTRY	*	pCurDiskEnt = NULL;
	DWORD					NameLen, CurEntSize, SizeLeft;
	LONG					i, NumRead = 0;
	PDFENTRY				pParentDfe = NULL, pCurDfe = NULL;
	BOOLEAN					LastBuf = False, ReadDb = False;

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
			("afpReadIdDb: Reading existing Id Database header stream...\n") );

	do
	{
		if ((pReadBuf = AfpAllocPANonPagedMemory(IDDB_UPDATE_BUFLEN)) == NULL)
		{
			Status=STATUS_INSUFFICIENT_RESOURCES;
			break;
		}

		pIdDbHdr = (PIDDBHDR)pReadBuf;

		// read in the header
		Status = AfpIoRead(pfshIdDb,
						   &LIZero,
						   IDDB_UPDATE_BUFLEN,
						   &SizeRead,
						   (PBYTE)pIdDbHdr);

		if (!NT_SUCCESS(Status)									||
			(SizeRead < sizeof(IDDBHDR))						||
			(pIdDbHdr->idh_Signature != AFP_SERVER_SIGNATURE)	||
			(pIdDbHdr->idh_LastId < AFP_ID_NETWORK_TRASH))
		{
			DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_ERR,
					("afpreadIdDb: Bad signature\n"));
			// just recreate the stream
			Status = AFP_ERR_BAD_VERSION;
			AfpIoSetSize(pfshIdDb, 0);
		}
		else
		{
			if (pIdDbHdr->idh_Version == AFP_IDDBHDR_VERSION)
			{
				AfpIdDbHdrToVolDesc(pIdDbHdr, pVolDesc);
				if (SizeRead < (sizeof(IDDBHDR) + sizeof(DWORD)))
				{
					DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_ERR,
							("afpreadIdDb: Size incorrect\n"));
					Count = 0;
					Status = STATUS_END_OF_FILE;
				}
				else
				{
					Count = *(PDWORD)(pReadBuf + sizeof(IDDBHDR));
					if (Count != 0)
						ReadDb = True;
					else
					{
						Status = STATUS_END_OF_FILE;
						DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_ERR,
								("afpreadIdDb: Count incorrect\n"));
					}
				}
			}
			else if ((pIdDbHdr->idh_Version != AFP_IDDBHDR_VERSION1) &&
					 (pIdDbHdr->idh_Version != AFP_IDDBHDR_VERSION2))
			{
				DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_ERR,
						("afpreadIdDb: Bad Version\n"));
				// just recreate the stream
				AfpIoSetSize(pfshIdDb, 0);
				Status = AFP_ERR_BAD_VERSION;
			}
			else
			{
				DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_ERR,
						("afpreadIdDb: Downlevel Version\n"));

				Status = STATUS_END_OF_FILE;

				AfpIdDbHdrToVolDesc(pIdDbHdr, pVolDesc);

				// Dont blow away the header
				AfpIoSetSize(pfshIdDb, sizeof(IDDBHDR));
			}
		}

		if (!NT_SUCCESS(Status) || !ReadDb || (Count == 0))
		{
			break;
		}

		// read the count of entries from the stream
		ForkOffst.QuadPart = SizeRead;
		SizeLeft = SizeRead - (sizeof(IDDBHDR) + sizeof(DWORD));
		pCurDiskEnt = (UNALIGNED DISKENTRY *)(pReadBuf + (sizeof(IDDBHDR) + sizeof(DWORD)));

		// Start the database with the PARENT_OF_ROOT
		// At this point there is only the ParentOfRoot DFE in the volume.
		// Just access it rather can calling AfpFindDfEntryById
		pParentDfe = pVolDesc->vds_pDfeDirBuckets[AFP_ID_PARENT_OF_ROOT];
		ASSERT (pParentDfe != NULL);
		ASSERT (pParentDfe->dfe_AfpId == AFP_ID_PARENT_OF_ROOT);

		DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
				("afpreadIdDb: Number of entries %d\n", Count));

		while ((NumRead < Count) &&
			   ((pVolDesc->vds_Flags & (VOLUME_STOPPED | VOLUME_DELETED)) == 0))
		{
			//
			// get the next entry
			//

			// We ensure that there are no partial entries. If an entry does not fit at
			// the end, we write an invalid entry (AfpId == 0) and skip to the next
			// buffer.
			if ((SizeLeft < (sizeof(DISKENTRY)))  || (pCurDiskEnt->dsk_AfpId == 0))
			{
				if (LastBuf) // we have already read to the end of file
				{
					DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_ERR,
							("afpreadIdDb: Reached EOF\n"));
					Status = STATUS_UNSUCCESSFUL;
					break;
				}

				// Skip to the next page and continue with the next entry
				Status = AfpIoRead(pfshIdDb,
								   &ForkOffst,
								   IDDB_UPDATE_BUFLEN,
								   &SizeRead,
								   pReadBuf);

				if (!NT_SUCCESS(Status))
				{
					DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_ERR,
							("afpreadIdDb: Read Eror %lx\n", Status));
					break;
				}

				ForkOffst.QuadPart += SizeRead;
				// if we read less than we asked for, then we reached EOF
				LastBuf = (SizeRead < IDDB_UPDATE_BUFLEN) ? True : False;
				SizeLeft = SizeRead;
				pCurDiskEnt = (UNALIGNED DISKENTRY *)pReadBuf;
				continue;
			}

			//
			// check dsk_Signature for signature, just to be sure you are
			// still aligned on a structure and not off in la-la land
			//
			if (pCurDiskEnt->dsk_Signature != AFP_DISKENTRY_SIGNATURE)
			{
				DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_ERR,
						("afpreadIdDb: Signature mismatch\n"));
				Status = STATUS_DATATYPE_MISALIGNMENT;
				break;
			}

			ASSERT(pCurDiskEnt->dsk_AfpId != AFP_ID_NETWORK_TRASH);

			// add current entry to database
			if (pCurDiskEnt->dsk_AfpId != AFP_ID_ROOT)
			{
				AfpInitUnicodeStringWithNonNullTerm(&uName,
													(pCurDiskEnt->dsk_Flags & DFE_FLAGS_NAMELENBITS) * sizeof(WCHAR),
													(PWSTR)(pCurDiskEnt->dsk_Name));
			}
			else
			{
				// In case someone has reused a directory with an existing
				// AFP_IdIndex stream for a different volume name
				uName = pVolDesc->vds_Name;
			}

			DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
					("afpReadIdDb: Read %d: %Z, flags %x\n", pCurDiskEnt->dsk_AfpId,
					&uName, pCurDiskEnt->dsk_Flags));

			if ((pCurDfe = AfpAddDfEntry(pVolDesc,
										 pParentDfe,
										 &uName,
										 (BOOLEAN)((pCurDiskEnt->dsk_Flags & DFE_FLAGS_DIR) == DFE_FLAGS_DIR),
										 pCurDiskEnt->dsk_AfpId)) == NULL)
			{
				DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_ERR,
						("afpreadIdDb: AfpAddDfEntry failed for %Z (ParentId %x)\n",
						 &uName, pParentDfe->dfe_AfpId));
				Status = STATUS_UNSUCCESSFUL;
				break;
			}

			// Initialize pointer to root.
			if (DFE_IS_ROOT(pCurDfe))
			{
				pVolDesc->vds_pDfeRoot = pCurDfe;
			}

			afpUpdateDfeWithSavedData(pCurDfe, pCurDiskEnt);

			NameLen = (DWORD)((pCurDiskEnt->dsk_Flags & DFE_FLAGS_NAMELENBITS)*sizeof(WCHAR));
			CurEntSize = DWLEN(FIELD_OFFSET(DISKENTRY, dsk_Name) + NameLen);

			NumRead ++;
			SizeLeft -= CurEntSize;
			pCurDiskEnt = (UNALIGNED DISKENTRY *)((PBYTE)pCurDiskEnt + CurEntSize);

			// figure out who the next parent should be
			if (pCurDfe->dfe_Flags & DFE_FLAGS_HAS_CHILD)
			{
				DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
						("afpreadIdDb: Moving down %Z\n", &pCurDfe->dfe_UnicodeName));
				pParentDfe = pCurDfe;
			}
			else if (!(pCurDfe->dfe_Flags & DFE_FLAGS_HAS_SIBLING))
			{
				if (DFE_IS_PARENT_OF_ROOT(pParentDfe))
				{
					DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
							("afpreadIdDb: Done, NumRead %d, Count %d\n", NumRead, Count));
					ASSERT(NumRead == Count);
					break;
				}
				while (!(pParentDfe->dfe_Flags & DFE_FLAGS_HAS_SIBLING))
				{
					if (DFE_IS_ROOT(pParentDfe))
					{
						break;
					}
					pParentDfe = pParentDfe->dfe_Parent;
					DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
							("afpreadIdDb: Moving up\n"));
				}
				pParentDfe = pParentDfe->dfe_Parent;
				if (DFE_IS_PARENT_OF_ROOT(pParentDfe))
				{
					DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
							("afpreadIdDb: Reached Id 1\n"));
					ASSERT(NumRead == Count);
					break;
				}
			}
		} // while

		DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
				("\tRead %ld entries\n", NumRead) );
		if (!NT_SUCCESS(Status))
		{
			// Free everything and start over
			AfpFreeIdIndexTables(pVolDesc);
			afpInitializeIdDb(pVolDesc);
		}

	} while (False);

	if (pReadBuf != NULL)
		AfpFreePANonPagedMemory(pReadBuf, IDDB_UPDATE_BUFLEN);

	if (!NT_SUCCESS(Status))
	{
		AFPLOG_ERROR(AFPSRVMSG_INIT_IDDB,
					 Status,
					 NULL,
					 0,
					 &pVolDesc->vds_Name);
	}

	return Status;
}


/***	AfpFlushIdDb
 *
 *	Initiated by the scavenger thread. If it is determined that the Afp_IdIndex
 *	stream for this volume is to be flushed to disk, then this is called to
 *	do the job. The swmr access is locked for read while the update is in
 *	progress. The vds_cScvgrIdDb and vds_cChangesIdDb are reset if there is
 *	no error writing the entire database.  An initial count of zero is written
 *	to the database before it is updated, just in case an error occurs that
 *	prevents us from writing the entire database.  When the entire thing is
 *	written, then the actual count of entries is written to disk.  The
 *	parent-of-root entry is never saved on disk, nor is the network trash
 *	subtree of the database. The iddb header is written whether it is dirty
 *	or not.
 *
 *	LOCKS:			vds_VolLock (SPIN)
 *	LOCKS_ASSUMED:	vds_idDbAccessLock (SWMR, Shared)
 *	LOCK_ORDER:		vds_VolLock after vds_IdDbAccessLock
 */
VOID FASTCALL
AfpFlushIdDb(
	IN	PVOLDESC			pVolDesc,
	IN	PFILESYSHANDLE		phIdDb
)
{
	PBYTE					pWriteBuf;
	NTSTATUS				Status;
	BOOLEAN					WriteEntireHdr = False;
	PDFENTRY				pCurDfe;
	DWORD					SizeLeft;	// the number of free bytes left in the writebuffer
	DWORD					CurEntSize, NumWritten = 0;
	FORKOFFST				ForkOffst;
	UNALIGNED DISKENTRY	*	pCurDiskEnt = NULL;
	PIDDBHDR				pIdDbHdr;
	BOOLEAN					HdrDirty = False;
	KIRQL					OldIrql;
	DWORD					fbi = 0, CreateInfo;
#ifdef	PROFILING
	TIME		TimeS, TimeE, TimeD;

	INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_IdIndexUpdCount);
	AfpGetPerfCounter(&TimeS);
#endif

	DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,("\tWriting out IdDb...\n") );
	ASSERT(VALID_VOLDESC(pVolDesc));

	// Take the Swmr so that nobody can initiate changes to the IdDb
	AfpSwmrAcquireShared(&pVolDesc->vds_IdDbAccessLock);

	do
	{
		if ((pWriteBuf = AfpAllocPANonPagedMemory(IDDB_UPDATE_BUFLEN)) == NULL)
		{
			Status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}

		pIdDbHdr = (PIDDBHDR)pWriteBuf;

		// Snapshot the IdDbHdr and dirty bit
		ACQUIRE_SPIN_LOCK(&pVolDesc->vds_VolLock, &OldIrql);

		AfpVolDescToIdDbHdr(pVolDesc, pIdDbHdr);

		if (pVolDesc->vds_Flags & VOLUME_IDDBHDR_DIRTY)
		{
			HdrDirty = True;
			// Clear the dirty bit
			pVolDesc->vds_Flags &= ~VOLUME_IDDBHDR_DIRTY;
		}

		RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock, OldIrql);

		// Set the count of entries to zero. Once we are done, we'll then
		// overwrite it with the right value
		*(PDWORD)(pWriteBuf + sizeof(IDDBHDR)) = 0;

   		// Set the pointer to the first entry we'll write past the header and
		// the count of entries. Adjust space remaining.
		pCurDiskEnt = (UNALIGNED DISKENTRY *)(pWriteBuf + sizeof(IDDBHDR) + sizeof(DWORD));
		SizeLeft = IDDB_UPDATE_BUFLEN - (sizeof(IDDBHDR) + sizeof(DWORD));
		ForkOffst.QuadPart = 0;

		// start with the root (don't write out the parent of root)
		pCurDfe = pVolDesc->vds_pDfeRoot;
		ASSERT(pCurDfe != NULL);

		Status = STATUS_SUCCESS;

		do
		{
			ASSERT(!DFE_IS_NWTRASH(pCurDfe));

			// The size of the current entry is:
			//	DISKENTRY structure + namelen padded to 4*N
			CurEntSize = DWLEN(FIELD_OFFSET(DISKENTRY, dsk_Name) + pCurDfe->dfe_UnicodeName.Length);
			if (CurEntSize > SizeLeft)
			{
				// If there is atleast a DWORD left, write a ZERO there to
				// indicate that the rest of the buffer is to be skipped.
				// ZERO is an invalid AfpId.
				// NOTE: dsk_AfpId NEEDS TO BE THE FIRST FIELD IN THE STRUCT
				ASSERT(FIELD_OFFSET(DISKENTRY, dsk_AfpId) == 0);

				if (SizeLeft > 0)
				{
					RtlZeroMemory(pWriteBuf + IDDB_UPDATE_BUFLEN - SizeLeft,
								  SizeLeft);
				}

				// write out the buffer and start at the beginning of buffer
				Status = AfpIoWrite(phIdDb,
									&ForkOffst,
									IDDB_UPDATE_BUFLEN,
									pWriteBuf);
				if (!NT_SUCCESS(Status))
				{
					// Reset the dirty bit if need be
					if (HdrDirty && (ForkOffst.QuadPart == 0))
					{
						AfpInterlockedSetDword(&pVolDesc->vds_Flags,
												VOLUME_IDDBHDR_DIRTY,
												&pVolDesc->vds_VolLock);
					}
					break;
				}
				ForkOffst.QuadPart += IDDB_UPDATE_BUFLEN;
				SizeLeft = IDDB_UPDATE_BUFLEN;
				pCurDiskEnt = (UNALIGNED DISKENTRY *)pWriteBuf;
			}

			NumWritten ++;
			afpSaveDfeData(pCurDfe, pCurDiskEnt);

			if (DFE_IS_DIRECTORY(pCurDfe))
			{
				PDIRENTRY	pDirEntry = pCurDfe->dfe_pDirEntry;

				pCurDiskEnt->dsk_RescLen = 0;
				pCurDiskEnt->dsk_Access = *(PDWORD)(&pDirEntry->de_Access);
				ASSERT (pCurDfe->dfe_pDirEntry != NULL);
				if ((pCurDfe->dfe_FileOffspring > 0)	||
					(pCurDfe->dfe_DirOffspring > 1)		||
					 ((pCurDfe->dfe_DirOffspring > 0) &&
					 !DFE_IS_NWTRASH(pDirEntry->de_ChildDir)))
				{
					pCurDiskEnt->dsk_Flags |= DFE_FLAGS_HAS_CHILD;
				}

				if (pCurDfe->dfe_NextSibling != NULL)
				{
					// Make sure we ignore the nwtrash folder
					if (!DFE_IS_NWTRASH(pCurDfe->dfe_NextSibling) ||
						(pCurDfe->dfe_NextSibling->dfe_NextSibling != NULL))
					{
						pCurDiskEnt->dsk_Flags |= DFE_FLAGS_HAS_SIBLING;
					}
				}
			}
			else
			{
				BOOLEAN	fHasSibling;

				pCurDiskEnt->dsk_RescLen = pCurDfe->dfe_RescLen;
				pCurDiskEnt->dsk_DataLen = pCurDfe->dfe_DataLen;
				DFE_FILE_HAS_SIBLING(pCurDfe, fbi, &fHasSibling);
				if (fHasSibling)
				{
					pCurDiskEnt->dsk_Flags |= DFE_FLAGS_HAS_SIBLING;
				}
			}

			// stick the current entry into the write buffer
			DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
					("AfpFlushIdDb: Writing %s %d: %Z, flags %x\n",
					DFE_IS_DIRECTORY(pCurDfe) ? "Dir" : "File", pCurDfe->dfe_AfpId,
					&pCurDfe->dfe_UnicodeName, pCurDiskEnt->dsk_Flags));

			SizeLeft -= CurEntSize;
			pCurDiskEnt = (UNALIGNED DISKENTRY *)((PBYTE)pCurDiskEnt + CurEntSize);

			/*
			 * Get to the next DFE. The entire tree under the network-trash
			 * folder is ignored. The Next DFE is:
			 *
			 * if (the current DFE is a file)
			 * {
			 *		if (there is another file sibling)
			 *		{
			 *			CurDfe = file Sibling;
			 *		}
			 *		else if (there is dir sibling)
			 *		{
			 *			CurDfe = dir Sibling;
			 *		}
			 *		else
			 *		{
			 *			CurDfe = Parent's next sibling directory;
			 * 		}
			 * }
			 * else
			 * {
			 *		if (dir has any file children)
			 *		{
			 *			CurDfe = First file child;
			 *		}
			 *		else if (dir has any dir children)
			 *		{
			 *			CurDfe = First dir child;
			 *		}
			 *		if (there is a sibling)
			 *		{
			 *			CurDfe = dir Sibling;
			 *		}
			 *		else
			 *		{
			 *			CurDfe = Parent's next sibling directory;
			 * 		}
			 * }
			 * When we hit the root, we are done
			 *
			 */
			if (DFE_IS_FILE(pCurDfe))
			{
				if (pCurDfe->dfe_NextSibling != NULL)
				{
					pCurDfe = pCurDfe->dfe_NextSibling;
				}
				else
				{
					PDIRENTRY	pDirEntry;

					pDirEntry = pCurDfe->dfe_Parent->dfe_pDirEntry;

					fbi++;
					while ((fbi < MAX_CHILD_HASH_BUCKETS) &&
						   (pDirEntry->de_ChildFile[fbi] == NULL))
					{
						fbi++;
					}

					if (fbi < MAX_CHILD_HASH_BUCKETS)
					{
						pCurDfe = pDirEntry->de_ChildFile[fbi];
						continue;
					}

					// There are no more files from this parent. Try its children
					// next. Ignore the NWTRASH folder.
					fbi = 0;
					if (pDirEntry->de_ChildDir != NULL)
					{
	                    if (!DFE_IS_NWTRASH(pDirEntry->de_ChildDir))
						{
							pCurDfe = pDirEntry->de_ChildDir;
							continue;
						}

						if (pDirEntry->de_ChildDir->dfe_NextSibling != NULL)
						{
							pCurDfe = pDirEntry->de_ChildDir->dfe_NextSibling;
							continue;
						}
					}

					// No more 'dir' siblings, move on up.
					if (!DFE_IS_ROOT(pCurDfe))
					{
						goto move_up;
					}
					else
					{
						// we've reached the root: we're done: get out of the loop
						pCurDfe = NULL;
					}
				}
			}
			else
			{
				PDIRENTRY	pDirEntry;

				// Reset ChildFileBucket index. First check for and handle
				// all file children of this directory and then move on to
				// its children or siblings
				fbi = 0;
				pDirEntry = pCurDfe->dfe_pDirEntry;
				while ((fbi < MAX_CHILD_HASH_BUCKETS) &&
					   (pDirEntry->de_ChildFile[fbi] == NULL))
				{
					fbi++;
				}
				if (fbi < MAX_CHILD_HASH_BUCKETS)
				{
					pCurDfe = pDirEntry->de_ChildFile[fbi];
					continue;
				}

				if (pDirEntry->de_ChildDir != NULL)
				{
					pCurDfe = pDirEntry->de_ChildDir;

					// don't bother writing out the network trash tree
					if (DFE_IS_NWTRASH(pCurDfe))
					{
						// could be null, if so, we're done
						pCurDfe = pCurDfe->dfe_NextSibling;
					}
				}
				else if (pCurDfe->dfe_NextSibling != NULL)
				{
					pCurDfe = pCurDfe->dfe_NextSibling;
					// don't bother writing out the network trash tree
					if (DFE_IS_NWTRASH(pCurDfe))
					{
						// could be null, if so, we're done
						pCurDfe = pCurDfe->dfe_NextSibling;
					}
				}
				else if (!DFE_IS_ROOT(pCurDfe))
				{
				  move_up:
					while (pCurDfe->dfe_Parent->dfe_NextSibling == NULL)
					{
						if (DFE_IS_ROOT(pCurDfe->dfe_Parent))
						{
							break;
						}
						pCurDfe = pCurDfe->dfe_Parent;
					}

                    // if ROOT then you get NULL
					pCurDfe = pCurDfe->dfe_Parent->dfe_NextSibling; // if ROOT then you get NULL

					// Make sure we ignore the nwtrash folder
					if ((pCurDfe != NULL) && DFE_IS_NWTRASH(pCurDfe))
					{
						pCurDfe = pCurDfe->dfe_NextSibling; // Could be null
					}
				}
				else break;
			}
		} while ((pCurDfe != NULL) && NT_SUCCESS(Status));

		if (NT_SUCCESS(Status))
		{
			DWORD	LastWriteSize, SizeRead;

			LastWriteSize = ROUND_TO_PAGES(IDDB_UPDATE_BUFLEN - SizeLeft);
			if (SizeLeft != IDDB_UPDATE_BUFLEN)
			{
				// write out the last bunch of the entries. Zero out unused space
				RtlZeroMemory(pWriteBuf + IDDB_UPDATE_BUFLEN - SizeLeft,
							  LastWriteSize - (IDDB_UPDATE_BUFLEN - SizeLeft));
				Status = AfpIoWrite(phIdDb,
									&ForkOffst,
									LastWriteSize,
									pWriteBuf);
				if (!NT_SUCCESS(Status))
				{
					// Reset the dirty bit if need be
					if (HdrDirty && (ForkOffst.QuadPart == 0))
					{
						AfpInterlockedSetDword(&pVolDesc->vds_Flags,
												VOLUME_IDDBHDR_DIRTY,
												&pVolDesc->vds_VolLock);
					}
					break;
				}
			}

			// set the file length to IdDb plus count plus header
			Status = AfpIoSetSize(phIdDb,
								  ForkOffst.LowPart + LastWriteSize);
			ASSERT(Status == AFP_ERR_NONE);

			// write out the count of entries written to the file. Do a read-modify-write
			// of the first page
			ForkOffst.QuadPart = 0;
			Status = AfpIoRead(phIdDb,
							   &ForkOffst,
							   PAGE_SIZE,
							   &SizeRead,
							   pWriteBuf);
			ASSERT (NT_SUCCESS(Status) && (SizeRead == PAGE_SIZE));
			if (!NT_SUCCESS(Status))
			{
				// set the file length to header plus count of zero entries
				AfpIoSetSize(phIdDb,
							 sizeof(IDDBHDR)+sizeof(DWORD));
				break;
			}

			*(PDWORD)(pWriteBuf + sizeof(IDDBHDR)) = NumWritten;
			Status = AfpIoWrite(phIdDb,
								&ForkOffst,
								PAGE_SIZE,
								pWriteBuf);
			if (!NT_SUCCESS(Status))
			{
				// set the file length to header plus count of zero entries
				AfpIoSetSize(phIdDb,
							 sizeof(IDDBHDR)+sizeof(NumWritten));
				break;
			}

			// not protected since scavenger is the only consumer of this field
			pVolDesc->vds_cScvgrIdDb = 0;

			DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
					("\tWrote %ld entries\n", NumWritten) );
			ASSERT(Status == AFP_ERR_NONE);
		}
	} while (False);

	AfpSwmrRelease(&pVolDesc->vds_IdDbAccessLock);

	if (!NT_SUCCESS(Status))
	{
		AFPLOG_ERROR(AFPSRVMSG_WRITE_IDDB,
					 Status,
					 NULL,
					 0,
					 &pVolDesc->vds_Name);
	}

	if (pWriteBuf != NULL)
	{
		AfpFreePANonPagedMemory(pWriteBuf, IDDB_UPDATE_BUFLEN);
	}
#ifdef	PROFILING
	AfpGetPerfCounter(&TimeE);
	TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;
	INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_IdIndexUpdTime,
								 TimeD,
								 &AfpStatisticsLock);
#endif
}


/***	AfpChangeNotifyThread
 *
 *	Handle change notify requests queued by the notify completion routine.
 */
VOID
AfpChangeNotifyThread(
	IN	PVOID	pContext
)
{
	PKQUEUE		NotifyQueue;
	PLIST_ENTRY	pList, pNotifyList;
	LIST_ENTRY	TransitionList;
	PVOL_NOTIFY	pVolNotify;
	PVOLDESC	pVolDesc;
	ULONG		BasePriority;
	PLONG		pNotifyQueueCount;
	KIRQL		OldIrql;
	NTSTATUS	Status;
#ifdef	PROFILING
	TIME		TimeS, TimeE, TimeD;
#endif

	DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO,
			("AfpChangeNotifyThread: Starting %ld\n", pContext));

    IoSetThreadHardErrorMode( FALSE );

	// Boost our priority to just below low realtime.
	// The idea is get the work done fast and get out of the way.
	BasePriority = LOW_REALTIME_PRIORITY;
	Status = NtSetInformationThread(NtCurrentThread(),
									ThreadBasePriority,
									&BasePriority,
									sizeof(BasePriority));
	ASSERT(NT_SUCCESS(Status));

	pNotifyList = &AfpVolumeNotifyList[(LONG)pContext];
	NotifyQueue = &AfpVolumeNotifyQueue[(LONG)pContext];
	pNotifyQueueCount = &AfpNotifyQueueCount[(LONG)pContext];
	AfpThreadPtrsW[(LONG)pContext] = PsGetCurrentThread();

	do
	{
		AFPTIME		CurrentTime;

		// Wait for a change notify request to process or timeout
		pList = KeRemoveQueue(NotifyQueue, KernelMode, &TwoSecTimeOut);

		ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

		// We either have something to process or we timed out. In the latter case
		// see if it is time to move some stuff from the list to the queue.
		if ((NTSTATUS)pList != STATUS_TIMEOUT)
		{
			pVolNotify = CONTAINING_RECORD(pList, VOL_NOTIFY, vn_List);

			if (pVolNotify == &AfpTerminateNotifyThread)
			{
				// NOTE: what if we are holding a volume's iddb swmr??
				// Can this ever happen since afpFsdUnloadServer should
				// only be called after the volumes have gone away

				// If these assertions fail, then there will be extra ref
				// counts on some volumes due to unprocessed notifies,
				// and the volumes will never go away.
				ASSERT((*pNotifyQueueCount == 0)	&&
						(AfpNotifyListCount[(LONG)pContext] == 0) &&
						IsListEmpty(pNotifyList));
				break;			// Asked to quit, so do so.
			}

#ifdef	PROFILING
			AfpGetPerfCounter(&TimeS);
#endif
			pVolDesc = pVolNotify->vn_pVolDesc;
			(*pNotifyQueueCount) --;

			ASSERT(VALID_VOLDESC(pVolDesc));

#ifdef BLOCK_MACS_DURING_NOTIFYPROC
			ASSERT(pVolDesc->vds_QueuedNotifyCount > 0);
#endif

			// The volume is already referenced for the notification processing.
			// Dereference after finishing processing.
			if ((pVolDesc->vds_Flags & (VOLUME_DELETED | VOLUME_STOPPED)) == 0)
			{
#ifndef BLOCK_MACS_DURING_NOTIFYPROC
				AfpSwmrAcquireExclusive(&pVolDesc->vds_IdDbAccessLock);
#endif
				(*pVolNotify->vn_Processor)(pVolNotify);
#ifndef BLOCK_MACS_DURING_NOTIFYPROC
				AfpSwmrRelease(&pVolDesc->vds_IdDbAccessLock);
#endif
			}

			// If there are no more buffers in the notify queue that belong
			// to this volume, release the swmr.
#ifdef BLOCK_MACS_DURING_NOTIFYPROC
			pVolDesc->vds_QueuedNotifyCount --;
			if (pVolDesc->vds_QueuedNotifyCount == 0)
			{
				AfpSwmrRelease(&pVolDesc->vds_IdDbAccessLock);
			}
#endif
			AfpVolumeDereference(pVolDesc);
			AfpFreeMemory(pVolNotify);

#ifdef	PROFILING
			AfpGetPerfCounter(&TimeE);
			TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;
			INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_ChangeNotifyTime,
										 TimeD,
										 &AfpStatisticsLock);
			INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_ChangeNotifyCount);
#endif
			if (*pNotifyQueueCount > 0)
				continue;
		}

		// If we timed out because there was nothing in the queue, or we
		// just processed the last thing in the queue, then see if there are
		// more things that can be moved into the queue.
		InitializeListHead(&TransitionList);

		// Look at the list to see if some stuff should move to the
		// Queue now i.e. if the delta has elapsed since we were notified of this change
		AfpGetCurrentTimeInMacFormat(&CurrentTime);

		ACQUIRE_SPIN_LOCK(&AfpVolumeListLock, &OldIrql);

		while (!IsListEmpty(pNotifyList))
		{
			pList = RemoveHeadList(pNotifyList);
			pVolNotify = CONTAINING_RECORD(pList, VOL_NOTIFY, vn_List);
			pVolDesc = pVolNotify->vn_pVolDesc;

			if ((pVolNotify->vn_TimeStamp == AFP_QUEUE_NOTIFY_IMMEDIATELY) ||
				((CurrentTime - pVolNotify->vn_TimeStamp) >= VOLUME_NTFY_DELAY) ||
				(pVolDesc->vds_Flags & (VOLUME_STOPPED | VOLUME_DELETED)))
			{
				AfpNotifyListCount[(LONG)pContext] --;
				(*pNotifyQueueCount) ++;
				// Build up a list of items to queue up outside the spinlock
				// since we will want to take the IdDb swmr for any volume
				// which has notifies that we are about to process.
				// Make sure you add things so they are processed in the
				// order they came in with!!
				InsertTailList(&TransitionList, pList);
			}
			else
			{
				// Put it back where we we took it from - its time has not come yet
				// And we are done now since the list is ordered in time.
				InsertHeadList(pNotifyList, pList);
				break;
			}
		}

		RELEASE_SPIN_LOCK(&AfpVolumeListLock, OldIrql);

		while (!IsListEmpty(&TransitionList))
		{
			pList = RemoveHeadList(&TransitionList);
			pVolNotify = CONTAINING_RECORD(pList, VOL_NOTIFY, vn_List);
			pVolDesc = pVolNotify->vn_pVolDesc;
			AfpVolumeQueueChangeNotify(pVolNotify, NotifyQueue);

#ifdef BLOCK_MACS_DURING_NOTIFYPROC
			if (pVolDesc->vds_QueuedNotifyCount == 0)
			{
				// While we process the changes already pending
				// Block out mac clients from making changes to this volume
				AfpSwmrAcquireExclusive(&pVolDesc->vds_IdDbAccessLock);
			}
			pVolDesc->vds_QueuedNotifyCount ++;
#endif
		}
	} while (True);

	DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO,
			("AfpChangeNotifyThread: Quitting %ld\n", pContext));

	AfpThreadPtrsW[(LONG)pContext] = NULL;
	KeSetEvent(&AfpStopConfirmEvent, IO_NETWORK_INCREMENT, False);
}


/***	AfpProcessChangeNotify
 *
 * A change item has been dequeued by one of the notify processing threads.
 *
 *	LOCKS_ASSUMED: vds_IdDbAccessLock (SWMR, Exclusive)
 */
VOID FASTCALL
AfpProcessChangeNotify(
	IN	PVOL_NOTIFY			pVolNotify
)
{
	PVOLDESC					pVolDesc;
	UNICODE_STRING				UName, UParent, UTail;
	PFILE_NOTIFY_INFORMATION	pFNInfo;
	BOOLEAN						CleanupHandle;
	PLIST_ENTRY					pList;
	NTSTATUS					status;
	PDFENTRY					pDfEntry;
	FILESYSHANDLE				handle;
	DWORD						afpChangeAction;
	DWORD						StreamId;
	PFILE_BOTH_DIR_INFORMATION	pFBDInfo = NULL;
	LONG						infobuflen;
        LONGLONG                        infobuf[(sizeof(FILE_BOTH_DIR_INFORMATION) + (AFP_LONGNAME_LEN + 1) * sizeof(WCHAR))/sizeof(LONGLONG) + 1];
#if DBG
	static PBYTE	Action[] = { "",
								 "ADDED",
								 "REMOVED",
								 "MODIFIED",
								 "RENAMED OLD",
								 "RENAMED NEW",
								 "STREAM ADDED",
								 "STREAM REMOVED",
								 "STREAM MODIFIED"};
#endif

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_INFO,
			("AfpProcessChangeNotify: entered...\n"));

	pFNInfo = (PFILE_NOTIFY_INFORMATION)(pVolNotify + 1);

	ASSERT((pFNInfo->Action & AFP_ACTION_PRIVATE) == 0);

    pVolDesc = pVolNotify->vn_pVolDesc;
	ASSERT (VALID_VOLDESC(pVolDesc));

	INTERLOCKED_DECREMENT_LONG(&pVolDesc->vds_cOutstandingNotifies);

	StreamId = pVolNotify->vn_StreamId;

	if ( (pFNInfo->Action == FILE_ACTION_REMOVED) ||
		 (pFNInfo->Action == FILE_ACTION_RENAMED_OLD_NAME) )
	{
		ASSERT(!IsListEmpty(&pVolDesc->vds_ChangeNotifyLookAhead));

		DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_INFO,
				("AfpProcessChangeNotify: removing %s LookAhead change\n",
				Action[pFNInfo->Action]));

		pList = ExInterlockedRemoveHeadList(&pVolDesc->vds_ChangeNotifyLookAhead,
											&pVolDesc->vds_VolLock);

		ASSERT(pList == &(pVolNotify->vn_DelRenLink));
	}

	// Process each of the entries in the list for this volume
	while (True)
	{
		CleanupHandle = False;
		status = STATUS_SUCCESS;

		AfpInitUnicodeStringWithNonNullTerm(&UName,
											(USHORT)pFNInfo->FileNameLength,
											pFNInfo->FileName);
		UName.MaximumLength += (AFP_LONGNAME_LEN+1)*sizeof(WCHAR);

		ASSERT(IS_VOLUME_NTFS(pVolDesc) && !EXCLUSIVE_VOLUME(pVolDesc));

		DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_INFO,
				("AfpProcessChangeNotify: Action: %s Name: %Z\n",
				Action[pFNInfo->Action], &UName));

		do
		{
			DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_INFO,
					 ("AfpProcessChangeNotify: !!!!Processing Change!!!!\n"));

			// We have the idindex write lock, look up the entry by path,
			// open a handle to the item, query the appropriate info,
			// cache the info in the DFE.  Where necessary, open a handle
			// to the parent directory and update its cached ModTime.

		  Lookup_Entry:
			pDfEntry = afpFindEntryByNtPath(pVolDesc,
											pFNInfo->Action,
											&UName,
											&UParent,
											&UTail);
			if (pDfEntry != NULL)
			{
				// Open a handle to parent or entity relative to the volume root handle
				CleanupHandle = True;
				status = AfpIoOpen(&pVolDesc->vds_hRootDir,
								   StreamId,
								   FILEIO_OPEN_EITHER,
								   ((pFNInfo->Action == FILE_ACTION_ADDED) ||
									(pFNInfo->Action == FILE_ACTION_REMOVED) ||
									(pFNInfo->Action == FILE_ACTION_RENAMED_OLD_NAME)) ?
										&UParent : &UName,
								   FILEIO_ACCESS_NONE,
								   FILEIO_DENY_NONE,
								   False,
								   &handle);
				if (!NT_SUCCESS(status))
				{
					DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_INFO,
							("AfpProcessChangeNotify: Failed to open (0x%lx)\n", status));
					CleanupHandle = False;
					break;
				}

				switch (pFNInfo->Action)
				{
					case FILE_ACTION_ADDED:
					{
						UNICODE_STRING	UEntity;
						PDFENTRY		pDfeNew;
						FILESYSHANDLE	fshEnumDir;
						BOOLEAN			firstEnum = True;

						// Update timestamp of parent dir.
						AfpIoQueryTimesnAttr(&handle,
											 NULL,
											 &pDfEntry->dfe_LastModTime,
											 NULL);

						// enumerate parent handle for this entity to get
						// the file/dir info, then add entry to the IDDB
						if ((UTail.Length/sizeof(WCHAR)) <= AFP_LONGNAME_LEN)
						{
							pFBDInfo = (PFILE_BOTH_DIR_INFORMATION)infobuf;
							infobuflen = sizeof(infobuf);
						}
						else
						{
							infobuflen = sizeof(FILE_BOTH_DIR_INFORMATION) +
											  (MAXIMUM_FILENAME_LENGTH * sizeof(WCHAR));
							if ((pFBDInfo = (PFILE_BOTH_DIR_INFORMATION)
											AfpAllocNonPagedMemory(infobuflen)) == NULL)
							{
								status = STATUS_NO_MEMORY;
								break; // out of case FILE_ACTION_ADDED
							}
						}

						do
						{
							status = AfpIoQueryDirectoryFile(&handle,
															 pFBDInfo,
															 infobuflen,
															 FileBothDirectoryInformation,
															 True,
															 True,
															 firstEnum ? &UTail : NULL);
							if ((status == STATUS_BUFFER_TOO_SMALL) ||
								(status == STATUS_BUFFER_OVERFLOW))
							{
								// Since we queue our own ACTION_ADDED for directories when
								// caching a tree, we may have the case where we queued it
								// by shortname because it actually had a name > 31 chars.
								// Note that a 2nd call to QueryDirectoryFile after a buffer
								// Overflow must send a null filename, since if the name is
								// not null, it will override the restartscan parameter
								// which means the scan will not restart from the beginning
								// and we will not find the file name we are looking for.

								ASSERT((PBYTE)pFBDInfo == (PBYTE)infobuf);

								// This should never happen, but if it does...
								if ((PBYTE)pFBDInfo != (PBYTE)infobuf)
								{
									status = STATUS_UNSUCCESSFUL;
									break;
								}

								firstEnum = False;

								infobuflen = sizeof(FILE_BOTH_DIR_INFORMATION) +
													(MAXIMUM_FILENAME_LENGTH * sizeof(WCHAR));

								if ((pFBDInfo = (PFILE_BOTH_DIR_INFORMATION)
													AfpAllocNonPagedMemory(infobuflen)) == NULL)
									status = STATUS_NO_MEMORY;
							}
						} while ((status == STATUS_BUFFER_TOO_SMALL) ||
								 (status == STATUS_BUFFER_OVERFLOW));

						if (status == STATUS_SUCCESS)
						{
							// If this file was created in a directory that has
							// not been looked at by a mac, just ignore it.
							// If this was not a FILE_ACTION_ADDED we would not
							// have found it in the database at all if it was
							// a file and the parent has not had its file
							// children cached in, so we will ignore those
							// notifies by default anyway since the DFE would
							// come back as NULL from afpFindEntryByNtPath.
							// We *do* want to process changes for directories
							// even if the parent has not had its
							// file children brought in since directories
							// are only cached in once at volume startup.
							if (((pFBDInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) &&
								!DFE_CHILDREN_ARE_PRESENT(pDfEntry))
							{
								break;
							}

							// figure out which name to use
							// If NT name > AFP_LONGNAME_LEN, use the NT shortname for
							// Mac longname on NTFS, any other file system the shortname
							// will be null, so ignore the file
							if ((pFBDInfo->FileNameLength/sizeof(WCHAR)) <= AFP_LONGNAME_LEN)
							{
								AfpInitUnicodeStringWithNonNullTerm(&UEntity,
																	(USHORT)pFBDInfo->FileNameLength,
																	pFBDInfo->FileName);
							}
							else if (pFBDInfo->ShortNameLength > 0)
							{
								AfpInitUnicodeStringWithNonNullTerm(&UEntity,
																	(USHORT)pFBDInfo->ShortNameLength,
																	pFBDInfo->ShortName);
							}
							else
							{
								DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_ERR,
										("AfpProcessChangeNotify: Name is > 31 with no short name ?\n") );
								status = STATUS_UNSUCCESSFUL;
							}

							if (NT_SUCCESS(status))
							{
								// add the entry
								afpAddDfEntryAndCacheInfo(pVolDesc,
														  pDfEntry,
														  &UEntity,
														  &handle,
														  pFBDInfo,
														  &UName,
														  &pDfeNew,
														  True);

								if (pDfeNew == NULL)
								{
									DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_ERR,
											("AfpProcessChangeNotify: Could not add DFE for %Z\n", &UName));
								}
								else if (DFE_IS_DIRECTORY(pDfeNew))
								{
									// if a directory was added, we must see if it has
									// children that we must cache as well
									if (NT_SUCCESS(status = AfpIoOpen(&pVolDesc->vds_hRootDir,
																	  AFP_STREAM_DATA,
																	  FILEIO_OPEN_DIR,
																	  &UName,
																	  FILEIO_ACCESS_NONE,
																	  FILEIO_DENY_NONE,
																	  False,
																	  &fshEnumDir)))
									{
										status = AfpCacheDirectoryTree(pVolDesc,
																	   pDfeNew,
																	   GETENTIRETREE | REENUMERATE,
																	   &fshEnumDir,
																	   &UName);
										AfpIoClose(&fshEnumDir);
										if (!NT_SUCCESS(status))
										{
											DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_ERR,
													("AfpProcessChangeNotify: Could not cache dir tree for %Z\n",
													&UName) );
										}
									}
									else
									{
										DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_ERR,
												("AfpProcessChangeNotify: Added dir %Z but couldn't open it for enumerating\n", &UName) );
									}
								}
							}
						}

						if (((PBYTE)pFBDInfo != NULL) &&
							((PBYTE)pFBDInfo != (PBYTE)infobuf))
						{
							AfpFreeMemory(pFBDInfo);
							pFBDInfo = NULL;
						}
					}
					break;

					case FILE_ACTION_REMOVED:
					{
						// update timestamp of parent dir.
						AfpIoQueryTimesnAttr(&handle,
											 NULL,
											 &pDfEntry->dfe_Parent->dfe_LastModTime,
											 NULL);
						// remove entry from IDDb.
						AfpDeleteDfEntry(pVolDesc, pDfEntry);
					}
					break;

					case FILE_ACTION_MODIFIED:
					{
						FORKSIZE forklen;
						DWORD	 NtAttr;

						// NOTE: if a file is SUPERSEDED or OVERWRITTEN,
						// you will only get a MODIFIED notification.  Is
						// there a way to check the creation date against
						// what we have cached in order to figure out if
						// this is what happened?

						// query for times and attributes.  If its a file,
						// also query for the data fork length.
						if (NT_SUCCESS(AfpIoQueryTimesnAttr(&handle,
															&pDfEntry->dfe_CreateTime,
															&pDfEntry->dfe_LastModTime,
															&NtAttr)))
						{
							pDfEntry->dfe_NtAttr = (USHORT)NtAttr &
														 FILE_ATTRIBUTE_VALID_FLAGS;
						}
						if (DFE_IS_FILE(pDfEntry) &&
							NT_SUCCESS(AfpIoQuerySize(&handle, &forklen)))
						{
							pDfEntry->dfe_DataLen = forklen.LowPart;
						}
					}
					break;

					case FILE_ACTION_RENAMED_OLD_NAME:
					{
						UNICODE_STRING				UNewname;
						PFILE_NOTIFY_INFORMATION	pFNInfo2;

						status = STATUS_SUCCESS;

						// The next item in the change buffer better be the
						// new name -- consume this entry so we don't find
						// it next time thru the loop.  If there is none,
						// then throw the whole thing out and assume the
						// rename aborted in NTFS
						if (pFNInfo->NextEntryOffset == 0)
							break; // from switch

						// If the next change in the buffer is not the
						// new name, we don't want to advance over it,
						// we want it to be processed next time thru
						// the loop. Note we are assuming it is valid.
						(PBYTE)pFNInfo2 = (PBYTE)pFNInfo + pFNInfo->NextEntryOffset;
						ASSERT(pFNInfo2->Action == FILE_ACTION_RENAMED_NEW_NAME);
						if (pFNInfo2->Action != FILE_ACTION_RENAMED_NEW_NAME)
						{
							DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_ERR,
									("AfpProcessChangeNotify: Rename did not come with new name!!!\n"));
							break; // from switch
						}

						pFNInfo = pFNInfo2;

						// update timestamp of parent dir.
						AfpIoQueryTimesnAttr(&handle,
											 NULL,
											 &pDfEntry->dfe_Parent->dfe_LastModTime,
											 NULL);

						// get the entity name from the path (subtract the
						// parent path length from total length), if it is
						// > 31 chars, we have to get the shortname by
						// enumerating the parent for this item, since we
						// already have a handle to the parent
						AfpInitUnicodeStringWithNonNullTerm(&UNewname,
															(USHORT)pFNInfo->FileNameLength,
															pFNInfo->FileName);
						if (UParent.Length > 0)
						{
							// if the rename is not in the volume root,
							// get rid of the path separator before the name
							UNewname.Length -= UParent.Length + sizeof(WCHAR);
							UNewname.Buffer = (PWCHAR)((PBYTE)UNewname.Buffer + UParent.Length + sizeof(WCHAR));
						}

						if (UNewname.Length/sizeof(WCHAR) > AFP_LONGNAME_LEN)
						{
							infobuflen = sizeof(FILE_BOTH_DIR_INFORMATION) +
									  (MAXIMUM_FILENAME_LENGTH * sizeof(WCHAR));
							if ((pFBDInfo = (PFILE_BOTH_DIR_INFORMATION)
									AfpAllocNonPagedMemory(infobuflen)) == NULL)
							{
								status = STATUS_NO_MEMORY;
								break; // out of case FILE_ACTION_RENAMED
							}

							status = AfpIoQueryDirectoryFile(&handle,
															 pFBDInfo,
															 infobuflen,
															 FileBothDirectoryInformation,
															 True,
															 True,
															 &UNewname);
							if (status == STATUS_SUCCESS)
							{
								// figure out which name to use
								// If NT name > AFP_LONGNAME_LEN, use the NT shortname for
								// Mac longname on NTFS, any other file system the shortname
								// will be null, so ignore the file
								if ((pFBDInfo->FileNameLength/sizeof(WCHAR)) <= AFP_LONGNAME_LEN)
								{
									AfpInitUnicodeStringWithNonNullTerm(&UNewname,
																		(USHORT)pFBDInfo->FileNameLength,
																		pFBDInfo->FileName);
								}
								else if (pFBDInfo->ShortNameLength > 0)
								{
									AfpInitUnicodeStringWithNonNullTerm(&UNewname,
																		(USHORT)pFBDInfo->ShortNameLength,
																		pFBDInfo->ShortName);
								}
								else
								{
									DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_ERR,
											("AfpProcessChangeNotify: Name is > 31 with no short name ?\n") );
									status = STATUS_UNSUCCESSFUL;
								}
							}
						}

						// rename the entry
						if (NT_SUCCESS(status))
						{
							AfpRenameDfEntry(pVolDesc, pDfEntry, &UNewname);
						}

						if ((PBYTE)pFBDInfo != NULL)
						{
							AfpFreeMemory(pFBDInfo);
							pFBDInfo = NULL;
						}
					}
					break;

					case FILE_ACTION_MODIFIED_STREAM:
					{
						FORKSIZE forklen;

						// If it is the AFP_Resource stream on a file,
						// cache the resource fork length.
						if ((StreamId == AFP_STREAM_RESC) &&
							DFE_IS_FILE(pDfEntry) &&
							NT_SUCCESS(AfpIoQuerySize(&handle, &forklen)))
						{
							pDfEntry->dfe_RescLen = forklen.LowPart;
						}
						else if (StreamId == AFP_STREAM_INFO)
						{
							AFPINFO		afpinfo;
							FILEDIRPARM fdparms;

							// Read the afpinfo stream.  If the file ID in
							// the DfEntry does not match the one in the
							// stream, write back the ID we know it by.
							// Update our cached FinderInfo.
							if (NT_SUCCESS(AfpReadAfpInfo(&handle, &afpinfo)))
							{
								pDfEntry->dfe_FinderInfo = afpinfo.afpi_FinderInfo;
								pDfEntry->dfe_BackupTime = afpinfo.afpi_BackupTime;

								if (pDfEntry->dfe_AfpId != afpinfo.afpi_Id)
								{
									// munge up a fake FILEDIRPARMS structure
                                    AfpInitializeFDParms(&fdparms);
									fdparms._fdp_Flags = pDfEntry->dfe_Flags;
									fdparms._fdp_AfpId = pDfEntry->dfe_AfpId;
									AfpConvertMungedUnicodeToAnsi(&pDfEntry->dfe_UnicodeName,
																  &fdparms._fdp_LongName);

									// NOTE: can we open a handle to afpinfo
									// relative to a afpinfo handle??
									AfpSetAfpInfo(&handle,
												  FILE_BITMAP_FILENUM,
												  &fdparms,
												  NULL,
												  NULL);
								}
							}
						}
					}
					break; // from switch

					default:
						ASSERTMSG("AfpProcessChangeNotify: Unexpected Action\n", False);
						break;
				} // switch
			}
			else
			{
				PFILE_NOTIFY_INFORMATION	pFNInfo2;

				DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_WARN,
						("AfpProcessChangeNotify: Could not find DFE for %Z\n", &UName));

				// This item may have been deleted, renamed or moved
				// by someone else in the interim, ignore this change
				// if it was not a rename.  If it was a rename, then
				// try to add the item using the new name.
				if ((pFNInfo->Action == FILE_ACTION_RENAMED_OLD_NAME) &&
					(pFNInfo->NextEntryOffset != 0))
				{
					// If the next change in the buffer is not the
					// new name, we don't want to advance over it,
					// we want it to be processed next time thru
					// the loop. Note we are assuming it is valid.
					(PBYTE)pFNInfo2 = (PBYTE)pFNInfo + pFNInfo->NextEntryOffset;
					ASSERT(pFNInfo2->Action == FILE_ACTION_RENAMED_NEW_NAME);
					if (pFNInfo2->Action != FILE_ACTION_RENAMED_NEW_NAME)
					{
						DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_ERR,
								("AfpProcessChangeNotify: Rename did not come with new name!!! (no-DFE case)\n"));
						break; // from error loop
					}

					pFNInfo = pFNInfo2;

					pFNInfo->Action = FILE_ACTION_ADDED;

					AfpInitUnicodeStringWithNonNullTerm(&UName,
														(USHORT)pFNInfo->FileNameLength,
														pFNInfo->FileName);

					DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_INFO,
							("AfpProcessChangeNotify: Converting RENAME into Action: %s for Name: %Z\n",
							Action[pFNInfo->Action], &UName));

					goto Lookup_Entry;
				}
			}
		} while (False);

		if (CleanupHandle)
			AfpIoClose(&handle);

		// Advance to the next entry in the change buffer
		if (pFNInfo->NextEntryOffset == 0)
		{
			break;
		}
		(PBYTE)pFNInfo += pFNInfo->NextEntryOffset;
		DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_WARN,
				("More than one entry in notify ?\n"));
	}

	// Get new values for Free space on disk and update volume time
	AfpUpdateVolFreeSpace(pVolDesc);
}


/***	afpProcessPrivateNotify
 *
 *	Similar to AfpProcessChangeNotify but optimized/special-cased for private notifies.
 *
 *	LOCKS_ASSUMED: vds_idDbAccessLock (SWMR, Exclusive)
 */
VOID FASTCALL
afpProcessPrivateNotify(
	IN	PVOL_NOTIFY			pVolNotify
)
{
	PVOLDESC					pVolDesc;
	UNICODE_STRING				UName;
	PFILE_NOTIFY_INFORMATION	pFNInfo;
	NTSTATUS					status;
	PDFENTRY					pParentDFE;
	BOOLEAN						CloseParentHandle = False, Verify = True;
	PFILE_BOTH_DIR_INFORMATION	pFBDInfo;
        LONGLONG                        infobuf[(sizeof(FILE_BOTH_DIR_INFORMATION) + (AFP_LONGNAME_LEN + 1) * sizeof(WCHAR))/sizeof(LONGLONG) + 1];


	PAGED_CODE( );

	pFNInfo = (PFILE_NOTIFY_INFORMATION)(pVolNotify + 1);
	ASSERT (pFNInfo->Action & AFP_ACTION_PRIVATE);

	pVolDesc = pVolNotify->vn_pVolDesc;
	ASSERT (VALID_VOLDESC(pVolDesc));

	INTERLOCKED_DECREMENT_LONG(&pVolDesc->vds_cPrivateNotifies);

	pFBDInfo = (PFILE_BOTH_DIR_INFORMATION)infobuf;
	AfpInitUnicodeStringWithNonNullTerm(&UName,
										(USHORT)pFNInfo->FileNameLength,
										pFNInfo->FileName);
	// We always allocate extra space for notifies
	UName.MaximumLength += (AFP_LONGNAME_LEN+1)*sizeof(WCHAR);

	DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_INFO,
			("afpProcessPrivateNotify: ParentId %d, Path: %Z\n",
			pVolNotify->vn_ParentId, &UName));

	// Lookup the parent DFE using the AfpId stored in the notify
	// structure, and setup the UParent and UTail appropriately
	pParentDFE = AfpFindDfEntryById(pVolDesc,
									pVolNotify->vn_ParentId,
									DFE_DIR);
	if (pParentDFE != NULL)
	{
		FILESYSHANDLE	ParentHandle, DirHandle;

		status = STATUS_SUCCESS;

		// Open a handle to the directory relative to the volume root handle
		// Special case volume root
		ASSERT((UName.Length != 0) || (pVolNotify->vn_ParentId == AFP_ID_ROOT));

		do
		{
			PDFENTRY		pDfeNew;
			UNICODE_STRING	UParent, UTail;

			if (pVolNotify->vn_ParentId == AFP_ID_ROOT)
			{
				AfpSetEmptyUnicodeString(&UParent, 0, NULL);
				UTail = UName;
			}
			else
			{
				UParent.MaximumLength =
				UParent.Length = UName.Length - (USHORT)pVolNotify->vn_TailLength - sizeof(WCHAR);
				UParent.Buffer = UName.Buffer;

				UTail.MaximumLength =
				UTail.Length = (USHORT)pVolNotify->vn_TailLength;
				UTail.Buffer = (PWCHAR)((PBYTE)UName.Buffer + UParent.Length + sizeof(WCHAR));
			}

			if (UName.Length != 0)
			{
				// Open a handle to parent relative to the volume root handle
				status = AfpIoOpen(&pVolDesc->vds_hRootDir,
								   AFP_STREAM_DATA,
								   FILEIO_OPEN_DIR,
								   &UParent,
								   FILEIO_ACCESS_NONE,
								   FILEIO_DENY_NONE,
								   False,
								   &ParentHandle);
				if (!NT_SUCCESS(status))
				{
					DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_INFO,
							("afpProcessPrivateNotify: Failed to open parent: %Z (0x%lx)\n",
							&UParent, status));
					break;
				}
				CloseParentHandle = True;

				status = AfpIoQueryDirectoryFile(&ParentHandle,
												 pFBDInfo,
												 sizeof(infobuf),
												 FileBothDirectoryInformation,
												 True,
												 True,
												 &UTail);
				if (!NT_SUCCESS(status))
				{
					DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_INFO,
							("afpProcessPrivateNotify: AfpIoQueryDirectoryFile failed: %Z (0x%lx)\n",
							&UTail, status));
					break;
				}

				// Lookup this entry in the data-base. If not there then we need to add
				// it. If its there, we need to verify it.
				// NOTE: Use DFE_ANY here and not DFE_DIR !!!
				pDfeNew = AfpFindEntryByUnicodeName(pVolDesc,
													&UTail,
													AFP_LONGNAME,
													pParentDFE,
													DFE_ANY);
				if (pDfeNew == NULL)
					Verify = False;
			}
			else
			{
				FILE_BASIC_INFORMATION	FBasInfo;

				status = AfpIoQueryBasicInfo(&pVolDesc->vds_hRootDir,
											 &FBasInfo);
				if (!NT_SUCCESS(status))
				{
					DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_ERR,
							("afpProcessPrivateNotify: Could not get basic information for root\n"));
					status = STATUS_UNSUCCESSFUL;
					break;
				}

				afpConvertBasicToBothDirInfo(&FBasInfo, pFBDInfo);
				pDfeNew = pParentDFE;
				ParentHandle = pVolDesc->vds_hRootDir;

				// Root directory needs special casing. The reason here is that we have
				// no parent directory. Also we need to handle the AFP_HAS_CUSTOM_ICON
				// bit in the volume descriptor since the finderinfo on the root volume
				// doesn't have this bit saved.
				if (pVolDesc->vds_Flags & AFP_VOLUME_HAS_CUSTOM_ICON)
				{
					pDfeNew->dfe_FinderInfo.fd_Attr1 |= FINDER_FLAG_HAS_CUSTOM_ICON;
				}
			}

			if (!Verify)
			{
				ASSERT(pDfeNew == NULL);
				afpAddDfEntryAndCacheInfo(pVolDesc,
										  pParentDFE,
										  &UTail,
										  &ParentHandle,
										  pFBDInfo,
										  &UName,
										  &pDfeNew,
										  False);
			}
			else if (pFBDInfo->LastWriteTime.QuadPart > pDfeNew->dfe_LastModTime.QuadPart)
			{
				afpVerifyDFE(pVolDesc,
							 pParentDFE,
							 &UTail,
							 &ParentHandle,
							 pFBDInfo,
							 &UName,
							 &pDfeNew);
			}

			if (pDfeNew == NULL)
			{
				DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_ERR,
						("afpProcessPrivateNotify: Could not add DFE for %Z\n", &UName));
				status = STATUS_UNSUCCESSFUL;
				break;
			}
			pParentDFE = pDfeNew;

			//
			// Now open the directory itself so that it can be enumerated
			// Open the directory relative to its parent since we already
			// have the parent handle open.
			//
			status = AfpIoOpen(&ParentHandle,
							   AFP_STREAM_DATA,
							   FILEIO_OPEN_DIR,
							   &UTail,
							   FILEIO_ACCESS_NONE,
							   FILEIO_DENY_NONE,
							   False,
							   &DirHandle);
		} while (False);

		if (CloseParentHandle)
		{
			AfpIoClose(&ParentHandle);
		}

		if (NT_SUCCESS(status))
		{
			DWORD	Method;

			// Always get the root level files
			Method = (Verify && (pVolNotify->vn_ParentId != AFP_ID_ROOT)) ?
									GETDIRSKELETON : (GETENTIRETREE | REENUMERATE);
			status = AfpCacheDirectoryTree(pVolDesc,
										   pParentDFE,
										   Method,
										   &DirHandle,
										   &UName);
			if (!NT_SUCCESS(status))
			{
				DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_ERR,
						("afpProcessPrivateNotify: Could not cache dir tree for %Z\n", &UName));
			}
			AfpIoClose(&DirHandle);
		}

	}

	afpActivateVolume(pVolDesc);
}


/***	afpActivateVolume
 *
 *	If we just finished caching in the directory structure, activate the volume now.
 *	This is keyed off the AFP_INITIAL_CACHE bit in the volume flags.
 */
VOID FASTCALL
afpActivateVolume(
	IN	PVOLDESC			pVolDesc
)
{
	BOOLEAN	fCdfs;
	KIRQL	OldIrql;
	NTSTATUS	Status = STATUS_SUCCESS;

	ACQUIRE_SPIN_LOCK(&pVolDesc->vds_VolLock, &OldIrql);

	if ((pVolDesc->vds_cPrivateNotifies == 0) &&
		(pVolDesc->vds_Flags & VOLUME_INITIAL_CACHE))
	{
		// mark the volume as 'officially' available to the clients
		pVolDesc->vds_Flags |=  VOLUME_SCAVENGER_RUNNING;
		pVolDesc->vds_Flags &= ~VOLUME_INITIAL_CACHE;

		RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock, OldIrql);

        // Set the type of the ICON<0d> file to 0's so that
		// mac apps will not list it in their file-open dialogs
		fCdfs = !IS_VOLUME_NTFS(pVolDesc);
		if (!fCdfs)
		{
			PDFENTRY		pdfetmp;
			UNICODE_STRING	iconstr;
			WCHAR			iconname[5] = AFPSERVER_VOLUME_ICON_FILE_ANSI;

			AfpInitUnicodeStringWithNonNullTerm(&iconstr,
												sizeof(iconname),
												iconname);

			if ((pdfetmp = AfpFindEntryByUnicodeName(pVolDesc,
													 &iconstr,
													 AFP_LONGNAME,
													 pVolDesc->vds_pDfeRoot,
													 DFE_FILE)) != NULL)
			{
				pdfetmp->dfe_FinderInfo.fd_TypeD = 0;
			}

			// Kick off the OurChange scavenger scheduled routine
			// Explicitly reference this for the scavenger routine
			AfpVolumeReference(pVolDesc);

			// Schedule the scavenger to run periodically. This scavenger
			// is queued since it acquires a SWMR.
			AfpScavengerScheduleEvent(AfpOurChangeScavenger,
									  pVolDesc,
									  VOLUME_OURCHANGE_AGE,
									  False);

		}

        //
        // for CD's, get creation,mod time for the volume by asking the same for
        // root dir
        //
        if (fCdfs)
        {
		    Status = AfpIoQueryTimesnAttr(&pVolDesc->vds_hRootDir,
									      &pVolDesc->vds_pDfeRoot->dfe_CreateTime,
									      &pVolDesc->vds_pDfeRoot->dfe_LastModTime,
									      NULL);

            pVolDesc->vds_CreateTime = pVolDesc->vds_pDfeRoot->dfe_CreateTime;

            if (NT_SUCCESS(Status))
            {
                pVolDesc->vds_ModifiedTime =
                    AfpConvertTimeToMacFormat(&pVolDesc->vds_pDfeRoot->dfe_LastModTime);
            }
            else
            {
                pVolDesc->vds_ModifiedTime = pVolDesc->vds_pDfeRoot->dfe_CreateTime;
            }
        }

		// Kick off the scavenger thread scheduled routine
		// Explicitly reference this for the scavenger routine
		AfpVolumeReference(pVolDesc);

		// Schedule the scavenger to run periodically. Always make the
		// scavenger use the worker thread for CD-ROM volumes since we
		// 'nudge' it every invocation to see if the CD is in the drive
		AfpScavengerScheduleEvent(AfpVolumeScavenger,
								  pVolDesc,
								  fCdfs ?
									VOLUME_CDFS_SCAVENGER_INTERVAL :
									VOLUME_NTFS_SCAVENGER_INTERVAL,
								  fCdfs);

	    DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_ERR,
			("afpActivateVolume: volume %Z activated %s\n",
                    &pVolDesc->vds_Name,
                    (pVolDesc->vds_Flags & AFP_VOLUME_SUPPORTS_CATSRCH) ?
                     " " : "(CatSearch disabled)"));
	}
	else
	{
		RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock, OldIrql);
	}
}

/***	AfpShouldWeIgnoreThisNotification
 *
 *	Check to see if this notification should be ignored. The following events are
 *	ignored.
 *
 *		(((Action == FILE_ACTION_MODIFIED_STREAM) &&
 *		  (Stream != AFP_RESC_STREAM)			  &&
 *		  (Stream != AFP_INFO_STREAM))				||
 *		 (Its one of our own changes))
 *
 *	LOCKS:	vds_VolLock (SPIN)
 */
BOOLEAN FASTCALL
AfpShouldWeIgnoreThisNotification(
	IN	PVOL_NOTIFY		pVolNotify
)
{
	PFILE_NOTIFY_INFORMATION pFNInfo;
	PVOLDESC				 pVolDesc;
	UNICODE_STRING			 UName;
	BOOLEAN					 ignore = False;

	pFNInfo = (PFILE_NOTIFY_INFORMATION)(pVolNotify + 1);
	pVolDesc = pVolNotify->vn_pVolDesc;
	AfpInitUnicodeStringWithNonNullTerm(&UName,
										(USHORT)pFNInfo->FileNameLength,
										pFNInfo->FileName);

	DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_INFO,
			("AfpShouldWeIgnoreThisNotification: Action: %d Name: %Z\n", pFNInfo->Action, &UName));

	pVolNotify->vn_StreamId = AFP_STREAM_DATA;
	if (pFNInfo->Action == FILE_ACTION_MODIFIED_STREAM)
	{
		UNICODE_STRING	UStream;

		ignore = True;
		UStream.Length = UStream.MaximumLength = AfpInfoStream.Length;
		UStream.Buffer = (PWCHAR)((PBYTE)UName.Buffer +
								  UName.Length - AfpInfoStream.Length);

		if (EQUAL_UNICODE_STRING(&UStream, &AfpInfoStream, False))
		{
			pVolNotify->vn_StreamId = AFP_STREAM_INFO;
			pFNInfo->FileNameLength -= AfpInfoStream.Length;
			UName.Length -= AfpInfoStream.Length;
			ignore = False;
		}
		else
		{
			UStream.Length = UStream.MaximumLength = AfpResourceStream.Length;
			UStream.Buffer = (PWCHAR)((PBYTE)UName.Buffer +
									  UName.Length - AfpResourceStream.Length);

			if (EQUAL_UNICODE_STRING(&UStream, &AfpResourceStream, False))
			{
				pVolNotify->vn_StreamId = AFP_STREAM_RESC;
				pFNInfo->FileNameLength -= AfpResourceStream.Length;
				UName.Length -= AfpResourceStream.Length;
				ignore = False;
			}
		}
	}

	if (!ignore)
	{
		PLIST_ENTRY		pList, pListHead;
		POUR_CHANGE		pChange;
		DWORD			afpChangeAction;
		KIRQL			OldIrql;

		afpChangeAction = AFP_CHANGE_ACTION(pFNInfo->Action);

		ASSERT(afpChangeAction <= AFP_CHANGE_ACTION_MAX);

		ACQUIRE_SPIN_LOCK(&pVolDesc->vds_VolLock, &OldIrql);

		// point to the head of the appropriate change action list
		pListHead = &pVolDesc->vds_OurChangeList[afpChangeAction];

		for (pList = pListHead->Flink;
			 pList != pListHead;
			 pList = pList->Flink)
		{
			pChange = CONTAINING_RECORD(pList, OUR_CHANGE, oc_Link);

			// do a case *sensitive* unicode string compare
			if (EQUAL_UNICODE_STRING_CS(&UName, &pChange->oc_Path))
			{
				RemoveEntryList(&pChange->oc_Link);
				AfpFreeMemory(pChange);

				// We were notified of our own change
				ignore = True;

				if (pFNInfo->Action == FILE_ACTION_RENAMED_OLD_NAME)
				{
					// consume the RENAMED_NEW_NAME if it exists
					if (pFNInfo->NextEntryOffset != 0)
					{
						PFILE_NOTIFY_INFORMATION	pFNInfo2;

						(PBYTE)pFNInfo2 = (PBYTE)pFNInfo + pFNInfo->NextEntryOffset;
						ASSERT(pFNInfo2->Action == FILE_ACTION_RENAMED_NEW_NAME);
						if (pFNInfo2->Action == FILE_ACTION_RENAMED_NEW_NAME)
						{
							ASSERT(pFNInfo2->NextEntryOffset == 0);
						}
						else
						{
							DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_ERR,
									("AfpShouldWeIgnoreThisNotification: Our Rename did not come with new name!!!\n"));
						}
					}
					else
					{
						DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_ERR,
								("AfpShouldWeIgnoreThisNotification: Our Rename did not come with new name!!!\n"));

					}
				} // if rename
				else
				{
					// We are ignoring this notify. Make sure its not a multiple
					ASSERT(pFNInfo->NextEntryOffset == 0);
				}

				break;
			}
		} // while there are more of our changes to look thru

		RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock, OldIrql);
	}
	else
	{
		// We are ignoring this notify. Make sure its not a multiple
		ASSERT(pFNInfo ->NextEntryOffset == 0);
	}

	if (!ignore)
	{
		INTERLOCKED_INCREMENT_LONG(&pVolDesc->vds_cOutstandingNotifies);
		DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_INFO,
				("ShouldWe: Action: %d Name: \t\t\t\t\t\n%Z\n", pFNInfo->Action, &UName));
	}

	return ignore;
}


/***	AfpCacheDirectoryTree
 *
 *	Scan a directory tree and build the idindex database from this information.
 *	Do a breadth-first search. On volume start, this will cache the tree
 *	beginning at the root directory. For Directory ADDED notifications, this
 *	will cache the subtree beginning at the added directory, since a PC user can
 *  potentially move an entire subtree into a mac volume, but we will only
 *  be notified of the one directory addition.
 *
 *	Only the first level is actually handled here. Sub-directories are queued up
 *	as faked notifies and handled that way.
 *
 *  Method:
 *  REENUMERATE:		In case we need to reenumerate just
 *						this level in the tree in order to
 *						get rid of any dead wood that a PC
 *						removed by its 'other' name
 *
 *	GETDIRSKELETON:		When we want to bring in only the skeleton
 *						of the tree by adding directories only
 *
 *	GETFILES:			When we need to fill in the files for this
 *						level of the tree because a mac has accessed
 *						a dir for the first time.
 *
 *  GetDirSkeletonAndFiles:
 *						A Combination of the above two.
 *						When we want to bring in both files and directories.
 *						This is used when adding a volume, and we want the
 *						files in the root directory cached in, but no others.
 *						Also this will be used if we are rebuilding the
 *						Desktop database APPLs while caching the disk tree.
 *						The private ChangeNotifies we queue up for ADDED dirs
 *						will also read in the files if the volume is marked
 *						for rebuilding of the desktop.
 *
 *	GETENTIRETREE:		When we want to cache in the entire tree
 *
 *	GetEntireTreeAndReEnumerate:
 *						Combines GETENTIRETREE and REENUMERATE
 *
 *  LOCKS_ASSUMED: vds_IdDbAccessLock (SWMR, Exclusive)
 */
NTSTATUS
AfpCacheDirectoryTree(
	IN	PVOLDESC			pVolDesc,
	IN	PDFENTRY			pDFETreeRoot,		// DFE of the tree root directory
	IN	DWORD				Method,				// See explanation in procedure comment
	IN	PFILESYSHANDLE		phRootDir OPTIONAL, // open handle to tree root directory
	IN	PUNICODE_STRING		pDirPath  OPTIONAL
)
{
	UNICODE_STRING				UName, Path;
	PDFENTRY					pDFE;
	NTSTATUS					Status = STATUS_SUCCESS;
	PBYTE						enumbuf;
	PFILE_BOTH_DIR_INFORMATION	pNextEntry;
	FILESYSHANDLE				fshEnumDir;
	USHORT						SavedPathLength;
#ifdef	PROFILING
	TIME						TimeS, TimeE, TimeD;
	DWORD						NumScanned = 0;

	AfpGetPerfCounter(&TimeS);
#endif

	PAGED_CODE( );

	ASSERT (VALID_DFE(pDFETreeRoot));
	ASSERT (DFE_IS_DIRECTORY(pDFETreeRoot));

	ASSERT((Method != GETFILES) || !DFE_CHILDREN_ARE_PRESENT(pDFETreeRoot));

	// allocate the buffer that will hold enumerated files and dirs
	if ((pVolDesc->vds_EnumBuffer == NULL) &&
		((pVolDesc->vds_EnumBuffer = (PBYTE)AfpAllocPANonPagedMemory(AFP_ENUMBUF_SIZE)) == NULL))
	{
		return STATUS_NO_MEMORY;
	}

	do
	{
		fshEnumDir.fsh_FileHandle = NULL;
		enumbuf = pVolDesc->vds_EnumBuffer;

		// Get the volume root relative path to the directory tree being scanned
		// Get extra space for one more entry to tag on for queuing notifies.
		// In case we already have the path corres. to directory we are attempting
		// to cache, get it from there. Note that in this case we are always
		// guaranteed that extra space is available
		if (ARGUMENT_PRESENT(pDirPath))
		{
			Path = *pDirPath;
		}
		else
		{
			AfpSetEmptyUnicodeString(&Path, 0, NULL);
			Status = AfpHostPathFromDFEntry(pDFETreeRoot,
											(AFP_LONGNAME_LEN+1)*sizeof(WCHAR),
											&Path);

			if (!NT_SUCCESS(Status))
			{
				break;
			}
		}

		DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
				("AfpCacheDirectoryTree: ParentId %d, Path %Z\n",
				 pDFETreeRoot->dfe_AfpId, &Path));

		if (!ARGUMENT_PRESENT(phRootDir))
		{
			// Need to open a handle to the directory in order to enumerate
			if (NT_SUCCESS(Status = AfpIoOpen(&pVolDesc->vds_hRootDir,
											  AFP_STREAM_DATA,
											  FILEIO_OPEN_DIR,
											  &Path,
											  FILEIO_ACCESS_NONE,
											  FILEIO_DENY_NONE,
											  False,
											  &fshEnumDir)))
			{
				phRootDir = &fshEnumDir;
			}
			else
			{
				break;
			}
		}

		SavedPathLength = Path.Length;

		DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
				("AfpCacheDirectoryTree: Scanning Tree: %Z\n", &Path));

		if (Method & REENUMERATE)
		{
			afpMarkAllChildrenUnseen(pDFETreeRoot);
		}

		while (True)
		{
			// keep enumerating till we get all the entries
			Status = AfpIoQueryDirectoryFile(phRootDir,
											 (PFILE_BOTH_DIR_INFORMATION)enumbuf,
											 AFP_ENUMBUF_SIZE,
											 FileBothDirectoryInformation,
											 False, // return multiple entries
											 False, // don't restart scan
											 NULL);

			ASSERT(Status != STATUS_PENDING);

			if (Status != STATUS_SUCCESS)
			{
				if ((Status == STATUS_NO_MORE_FILES) ||
					(Status == STATUS_NO_SUCH_FILE))
				{
					Status = STATUS_SUCCESS;
					break; // that's it, we've seen everything there is
				}
				else
				{
					AFPLOG_HERROR(AFPSRVMSG_ENUMERATE,
								  Status,
								  NULL,
								  0,
								  phRootDir->fsh_FileHandle);
					DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_ERR,
							("AfpCacheDirectoryTree: dir enum failed %lx\n", Status));
					break;	// enumerate failed, bail out
				}
			}

			// process the enumerated files and dirs in the current enumbuf
			pNextEntry = (PFILE_BOTH_DIR_INFORMATION)enumbuf;
			while (True)
			{
				BOOLEAN						IsDir, WriteBackROAttr, FixIt;
				WCHAR						wc;
				PFILE_BOTH_DIR_INFORMATION	pCurrEntry;

				if (pNextEntry == NULL)
				{
					Status = STATUS_NO_MORE_ENTRIES;
					break;
				}
				WriteBackROAttr = False;
				IsDir = False;

				// Move the structure to the next entry or NULL if we hit the end
				pCurrEntry = pNextEntry;
				(PBYTE)pNextEntry += pCurrEntry->NextEntryOffset;
				if (pCurrEntry->NextEntryOffset == 0)
				{
					pNextEntry = NULL;
				}

				if (pCurrEntry->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					// Ignore dirs if we are only getting files for this level
					if (Method == GETFILES)
					{
						continue;
					}
					IsDir = True;
				}
				else if (Method == GETDIRSKELETON)
				{
					// Ignore files if we are only getting the dir skeleton
					continue;
				}

				// If NT name > AFP_LONGNAME_LEN, use the NT shortname for
				// Mac longname on NTFS, any other file system the shortname
				// will be null, so ignore the file
				if (pCurrEntry->FileNameLength <= (AFP_LONGNAME_LEN*sizeof(WCHAR)))
				{
					AfpInitUnicodeStringWithNonNullTerm(&UName,
														(USHORT)pCurrEntry->FileNameLength,
														pCurrEntry->FileName);
				}
				else if (pCurrEntry->ShortNameLength > 0)
				{
					AfpInitUnicodeStringWithNonNullTerm(&UName,
														(USHORT)pCurrEntry->ShortNameLength,
														pCurrEntry->ShortName);
				}
				else
				{
					DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_ERR,
							("AfpCacheDirectoryTree: Name is > 31 with no short name ?\n"));
					continue;
				}

				if (IsDir &&
					(EQUAL_UNICODE_STRING_CS(&Dot, &UName) ||
					 EQUAL_UNICODE_STRING_CS(&DotDot, &UName)))
				{
					continue;
				}

				// Check if this entry is an invalid win32 name i.e. it has either
				// a period or a space at end, if so convert it to the new format.
				// NOTE: can we construct a path to use to catch our own changes ?
				wc = UName.Buffer[(UName.Length - 1)/sizeof(WCHAR)];
				if ((wc == UNICODE_SPACE) || (wc == UNICODE_PERIOD))
				{
                    // BUGBUG: MacCD driver should fix this??
                    if (IS_VOLUME_NTFS(pVolDesc))
                    {
                        afpRenameInvalidWin32Name(phRootDir, IsDir, &UName);
                    }
				}

#ifdef	PROFILING
				NumScanned++;
#endif
				pDFE = NULL;
				FixIt = False;
				if (Method & REENUMERATE)
				{
					// If we have this item in our DB, just mark it as seen.
					// Use DFE_ANY here since a mismatch is fatal.
					afpFindDFEByUnicodeNameInSiblingList(pVolDesc,
														 pDFETreeRoot,
														 &UName,
														 &pDFE,
														 DFE_ANY);
					if (pDFE != NULL)
					{
						// If we have a wrong type, fix it.
						if (IsDir ^ DFE_IS_DIRECTORY(pDFE))
						{
							AfpDeleteDfEntry(pVolDesc, pDFE);
							pDFE = NULL;
							FixIt = True;
						}
						else
						{
							DFE_MARK_AS_SEEN(pDFE);
						}
					}
				}

				if ((Method != REENUMERATE) || FixIt)
				{
					// add this entry to the idindex database, and cache all the required
					// information, but only for files since the directories are queued
					// back and added at that time.
					if (!IsDir)
					{
						// Construct a full path to the file in order to filter our
						// own changes to AFP_AfpInfo stream when adding the file
						if (Path.Length > 0)
						{
							// Append a path separator
							Path.Buffer[Path.Length / sizeof(WCHAR)] = L'\\';
							Path.Length += sizeof(WCHAR);
						}
						ASSERT(Path.Length + UName.Length <= Path.MaximumLength);
						RtlAppendUnicodeStringToString(&Path, &UName);

						if (pDFE == NULL)
						{
							afpAddDfEntryAndCacheInfo(pVolDesc,
													  pDFETreeRoot,
													  &UName,
													  phRootDir,
													  pCurrEntry,
													  &Path,
													  &pDFE,
													  True);
						}
						else if (pCurrEntry->LastWriteTime.QuadPart > pDFE->dfe_LastModTime.QuadPart)
						{
							afpVerifyDFE(pVolDesc,
										 pDFETreeRoot,
										 &UName,
										 phRootDir,
										 pCurrEntry,
										 &Path,
										 &pDFE);
						}


						// Restore the original length of the path to enum dir
						Path.Length = SavedPathLength;

						if (pDFE == NULL)
						{
							// one reason this could fail is if we encounter pagefile.sys
							// if our volume is rooted at the drive root
							DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_ERR,
									("AfpCacheDirectoryTree: AddDfEntry failed \n"));
							continue;
						}
						else if (Method == (GETENTIRETREE | REENUMERATE))
						{
							DFE_MARK_AS_SEEN(pDFE);
						}
					}
					else
					{
						ASSERT(IsDir);
						ASSERT ((Method != GETFILES) &&
								(Method != REENUMERATE));

						// queue this directory as a simulated Notify of a directory add.
						AfpQueuePrivateChangeNotify(pVolDesc,
													&UName,
													&Path,
													pDFETreeRoot->dfe_AfpId);
					}
				}
			} // while more entries in the enumbuf

			if (!NT_SUCCESS(Status) && (Status != STATUS_NO_MORE_ENTRIES))
			{
				break;
			}
		} // while there are more files to enumerate

		if (NT_SUCCESS(Status))
		{
			if (Method & REENUMERATE)
			{
				afpPruneUnseenChildren(pVolDesc, pDFETreeRoot);
			}

			DFE_MARK_CHILDREN_PRESENT(pDFETreeRoot);
		}
		else
		{
			DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_ERR,
					("AfpCacheDirectoryTree: Status %lx\n", Status));
		}
	} while (False);

	ASSERT (enumbuf != NULL);
	if ((pVolDesc->vds_cPrivateNotifies == 0) &&
		(pVolDesc->vds_cOutstandingNotifies == 0))
	{
		AfpFreePANonPagedMemory(enumbuf, AFP_ENUMBUF_SIZE);
		pVolDesc->vds_EnumBuffer = NULL;
	}


	ASSERT (Path.Buffer != NULL);
	if (!ARGUMENT_PRESENT(pDirPath) && (Path.Buffer != NULL))
	{
		AfpFreeMemory(Path.Buffer);
	}

	if (fshEnumDir.fsh_FileHandle != NULL)
	{
		AfpIoClose(&fshEnumDir);
	}

#ifdef	PROFILING
	AfpGetPerfCounter(&TimeE);
	TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;
	INTERLOCKED_ADD_ULONG(&AfpServerProfile->perf_ScanTreeCount,
						  NumScanned,
						  &AfpStatisticsLock);
	INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_ScanTreeTime,
								TimeD,
								&AfpStatisticsLock);
#endif

	return Status;
}



/***	AfpQueuePrivateChangeNotify
 *
 *	LOCKS_ASSUMED: vds_idDbAccessLock (SWMR, Exclusive)
 */
VOID
AfpQueuePrivateChangeNotify(
	IN	PVOLDESC			pVolDesc,
	IN	PUNICODE_STRING		pName,
	IN	PUNICODE_STRING		pPath,
	IN	DWORD				ParentId
)
{
	DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_INFO,
			("AfpQueuePrivateChangeNotify: ParentId %d, Path %Z, Name %Z\n",
			ParentId, pPath, pName));

	// Reference the volume for Notify processing
	if (AfpVolumeReference(pVolDesc))
	{
		PVOL_NOTIFY					pVolNotify;
		PFILE_NOTIFY_INFORMATION	pNotifyInfo;

		DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_INFO,
				("AfpQueuePrivateChangeNotify: Queuing directory %Z\\%Z\n", pPath, pName));

		// Allocate an extra component worths
		pVolNotify = (PVOL_NOTIFY)AfpAllocNonPagedMemory(sizeof(VOL_NOTIFY) +
														 sizeof(FILE_NOTIFY_INFORMATION) +
														 pPath->Length +
														 pName->Length +
														 (AFP_LONGNAME_LEN+1)*sizeof(WCHAR)+
														 sizeof(WCHAR));
		if (pVolNotify != NULL)
		{
			LONG	Offset = 0;

			INTERLOCKED_INCREMENT_LONG(&pVolDesc->vds_cPrivateNotifies);

			pVolNotify->vn_pVolDesc = pVolDesc;
			pVolNotify->vn_Processor = afpProcessPrivateNotify;
			pVolNotify->vn_TimeStamp = AFP_QUEUE_NOTIFY_IMMEDIATELY;
			pVolNotify->vn_ParentId = ParentId;
			pVolNotify->vn_TailLength = pName->Length;
			pVolNotify->vn_StreamId = AFP_STREAM_DATA;
			pNotifyInfo = (PFILE_NOTIFY_INFORMATION)((PBYTE)pVolNotify + sizeof(VOL_NOTIFY));
			pNotifyInfo->NextEntryOffset = 0;
			pNotifyInfo->Action = FILE_ACTION_ADDED | AFP_ACTION_PRIVATE;
			pNotifyInfo->FileNameLength = pName->Length + pPath->Length;
			if (pPath->Length > 0)
			{
				RtlCopyMemory(pNotifyInfo->FileName,
							  pPath->Buffer,
							  pPath->Length);

				pNotifyInfo->FileName[pPath->Length/sizeof(WCHAR)] = L'\\';
				pNotifyInfo->FileNameLength += sizeof(WCHAR);
				Offset = pPath->Length + sizeof(WCHAR);
			}
			if (pName->Length > 0)
			{
				RtlCopyMemory((PBYTE)pNotifyInfo->FileName + Offset,
							  pName->Buffer,
							  pName->Length);
			}

			AfpVolumeInsertChangeNotifyList(pVolNotify, pVolDesc);
		}
		else
		{
			DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_ERR,
					("AfpQueuePrivateChangeNotify: Queuing of directory %Z\\%Z failed\n",
					pPath, pName));
			AfpVolumeDereference(pVolDesc);
		}
	}
}


/***	AfpQueueOurChange
 *
 * 	LOCKS:	vds_VolLock (SPIN)
 */
VOID
AfpQueueOurChange(
	IN PVOLDESC				pVolDesc,
	IN DWORD				Action,		// NT FILE_ACTION_XXX (ntioapi.h)
	IN PUNICODE_STRING		pPath,
	IN PUNICODE_STRING		pParentPath	OPTIONAL // queues a ACTION_MODIFIED
)
{
	POUR_CHANGE pchange = NULL;
	KIRQL		OldIrql;
#if DBG
	static PBYTE	ActionStrings[] =
					{	"",
						"ADDED",
						"REMOVED",
						"MODIFIED",
						"RENAMED OLD",
						"RENAMED NEW",
						"STREAM ADDED",
						"STREAM REMOVED",
						"STREAM MODIFIED"
					};
#endif

	PAGED_CODE( );
	ASSERT(IS_VOLUME_NTFS(pVolDesc) && !EXCLUSIVE_VOLUME(pVolDesc));

	pchange = (POUR_CHANGE)AfpAllocNonPagedMemory(sizeof(OUR_CHANGE) + pPath->Length);

	if (pchange != NULL)
	{
		DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_INFO,
				 ("AfpQueueOurChange: Queueing a %s for %Z\n", ActionStrings[Action], pPath));
		AfpGetCurrentTimeInMacFormat(&pchange->oc_Time);
		AfpInitUnicodeStringWithNonNullTerm(&pchange->oc_Path,
											pPath->Length,
											(PWCHAR)((PBYTE)pchange + sizeof(OUR_CHANGE)));
		RtlCopyMemory(pchange->oc_Path.Buffer,
					  pPath->Buffer,
					  pPath->Length);

		ExInterlockedInsertTailList(&pVolDesc->vds_OurChangeList[AFP_CHANGE_ACTION(Action)],
								    &pchange->oc_Link,
									&pVolDesc->vds_VolLock);
	}

	if (ARGUMENT_PRESENT(pParentPath))
	{
		pchange = (POUR_CHANGE)AfpAllocNonPagedMemory(sizeof(OUR_CHANGE) + pParentPath->Length);

		if (pchange != NULL)
		{
			DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_INFO,
					 ("AfpQueueOurChange: Queueing (parent) %s for %Z\n",
					 ActionStrings[FILE_ACTION_MODIFIED], pParentPath));
			AfpGetCurrentTimeInMacFormat(&pchange->oc_Time);
			AfpInitUnicodeStringWithNonNullTerm(&pchange->oc_Path,
												pParentPath->Length,
												(PWCHAR)((PBYTE)pchange + sizeof(OUR_CHANGE)));
			RtlCopyMemory(pchange->oc_Path.Buffer,
						  pParentPath->Buffer,
						  pParentPath->Length);

			ExInterlockedInsertTailList(&pVolDesc->vds_OurChangeList[AFP_CHANGE_ACTION(FILE_ACTION_MODIFIED)],
										&pchange->oc_Link,
										&pVolDesc->vds_VolLock);
		}
	}
}


/***	AfpDequeueOurChange
 *
 * 	LOCKS: LOCKS:	vds_VolLock (SPIN)
 */
VOID
AfpDequeueOurChange(
	IN PVOLDESC				pVolDesc,
	IN DWORD				Action,				// NT FILE_ACTION_XXX (ntioapi.h)
	IN PUNICODE_STRING		pPath,
	IN PUNICODE_STRING		pParentPath	OPTIONAL// queues a ACTION_MODIFIED
)
{
	POUR_CHANGE pChange;
	PLIST_ENTRY	pList, pListHead;
	KIRQL		OldIrql;
#if DBG
	static PBYTE	ActionStrings[] =
					{	"",
						"ADDED",
						"REMOVED",
						"MODIFIED",
						"RENAMED OLD",
						"RENAMED NEW",
						"STREAM ADDED",
						"STREAM REMOVED",
						"STREAM MODIFIED"
					};
#endif

	ASSERT(IS_VOLUME_NTFS(pVolDesc) && !EXCLUSIVE_VOLUME(pVolDesc));

	ACQUIRE_SPIN_LOCK(&pVolDesc->vds_VolLock, &OldIrql);

	// point to the head of the appropriate change action list
	pListHead = &pVolDesc->vds_OurChangeList[AFP_CHANGE_ACTION(Action)];

	for (pList = pListHead->Flink;
		 pList != pListHead;
		 pList = pList->Flink)
	{
		pChange = CONTAINING_RECORD(pList, OUR_CHANGE, oc_Link);

		// do a case *sensitive* unicode string compare
		if (EQUAL_UNICODE_STRING_CS(pPath, &pChange->oc_Path))
		{
			DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_INFO,
					 ("AfpDequeueOurChange: Dequeueing a %s for %Z\n",
					 ActionStrings[Action], pPath));

			RemoveEntryList(&pChange->oc_Link);
			AfpFreeMemory(pChange);
			break;
		}
	}

	if (ARGUMENT_PRESENT(pParentPath))
	{
		// point to the head of the appropriate change action list
		pListHead = &pVolDesc->vds_OurChangeList[FILE_ACTION_MODIFIED];
	
		for (pList = pListHead->Flink;
			 pList != pListHead;
			 pList = pList->Flink)
		{
			pChange = CONTAINING_RECORD(pList, OUR_CHANGE, oc_Link);
	
			// do a case *sensitive* unicode string compare
			if (EQUAL_UNICODE_STRING_CS(pParentPath, &pChange->oc_Path))
			{
				DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_INFO,
						 ("AfpDequeueOurChange: Dequeueing (parent) %s for %Z\n",
						 ActionStrings[FILE_ACTION_MODIFIED], pParentPath));

				RemoveEntryList(&pChange->oc_Link);
				AfpFreeMemory(pChange);
				break;
			}
		}
	}
	RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock, OldIrql);
}

/***	AfpOurChangeScavenger
 *
 *  This runs in a worker thread context since it takes an swmr.
 *
 *  LOCKS:	vds_VolLock (SPIN)
 */
AFPSTATUS FASTCALL
AfpOurChangeScavenger(
	IN PVOLDESC pVolDesc
)
{
	AFPTIME		Now;
	KIRQL		OldIrql;
	int 		i;
	BOOLEAN		DerefVol = False;
#if DBG
	static PBYTE	Action[] = { "",
								 "ADDED",
								 "REMOVED",
								 "MODIFIED",
								 "RENAMED OLD",
								 "RENAMED NEW",
								 "STREAM ADDED",
								 "STREAM REMOVED",
								 "STREAM MODIFIED"};
#endif

	DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_INFO,
			("AfpOurChangeScavenger: OurChange scavenger for volume %Z entered...\n",
			 &pVolDesc->vds_Name));

	// If this volume is going away, do not requeue this scavenger routine
	// We don't take the volume lock to check these flags since they are
	// one-way, i.e. once set they are never cleared.
	if (pVolDesc->vds_Flags & (VOLUME_DELETED | VOLUME_STOPPED))
	{
		DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_INFO,
				("AfpOurChangeScavenger: OurChange scavenger for volume %Z: Final run\n",
				 &pVolDesc->vds_Name));
		DerefVol = True;
	}

  CleanTurds:

	AfpGetCurrentTimeInMacFormat(&Now);

	ACQUIRE_SPIN_LOCK(&pVolDesc->vds_VolLock, &OldIrql);

	for (i = 0; i < NUM_AFP_CHANGE_ACTION_LISTS; i++)
	{
		PLIST_ENTRY	pList, pHead;
		POUR_CHANGE	pChange;

		pHead = &pVolDesc->vds_OurChangeList[i];
		while (!IsListEmpty(pHead))
		{
			pList = pHead->Flink;
			pChange = CONTAINING_RECORD(pList, OUR_CHANGE, oc_Link);

			if (((Now - pChange->oc_Time) > OURCHANGE_AGE) || DerefVol)
			{
				RemoveHeadList(pHead);

				DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_WARN,
						("AfpOurChangeScavenger: freeing %Z (%s)\n",
						&pChange->oc_Path, &Action[i]));
				AfpFreeMemory(pChange);
			}
			else
			{
				// All subsequent items in list will have later times so
				// don't bother checking them
				break;
			}
		}
	}

	RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock, OldIrql);

	// Check again if this volume is going away and if so do not requeue this
	// scavenger routine.  Note that while we were running, the volume may
	// have been deleted but this scavenger event could not be killed because
	// it wasn't found on the list. We don't want to requeue this routine again
	// because it will take AFP_OURCHANGE_AGE minutes for the volume to go
	// away otherwise. This closes the window more, but does not totally
	// eliminate it from happening.
	if (!DerefVol)
	{
		if (pVolDesc->vds_Flags & (VOLUME_DELETED | VOLUME_STOPPED))
		{
			DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_INFO,
					("AfpOurChangeScavenger: OurChanges scavenger for volume %Z: Final Run\n",
					 &pVolDesc->vds_Name));
			DerefVol = True;
			goto CleanTurds;
		}
	}
	else
	{
		AfpVolumeDereference(pVolDesc);
		return AFP_ERR_NONE;
	}

	return AFP_ERR_REQUEUE;
}





