/* SCCIO_SS.C 08/02/95 08.18.06 */
WORD IOOLE2ReadWORD (BYTE FAR *pBuf, WORD ByteOrder);
LONG IOOLE2ReadLONG (BYTE FAR *pBuf, WORD ByteOrder);
IOERR IOIsOLE2RootStorage (HIOFILE hFile);
void IOOLE2RootStorageFreeAlloc (PIOROOTSTORAGE pRoot);
IOERR IOOpenOLE2RootStorage (HIOFILE FAR *phFile, HIOSPEC hSpec, DWORD dwFlags
	);
IOERR IO_ENTRYMOD IOOLE2RootStgReadSector (PIOROOTSTORAGE pRoot, LONG Sector,
	 BYTE FAR *pBuf);
IOERR IO_ENTRYMOD IOOLE2RootStgReadConsecutiveSector (PIOROOTSTORAGE pRoot,
	 LONG StartSector, LONG SectorNum, BYTE FAR *pBuf);
LONG IOOLE2RootStgGetNextSector (PIOROOTSTORAGE pRoot, LONG Sector);
IOERR IO_ENTRYMOD IOOLE2RootStgReadMiniSector (PIOROOTSTORAGE pRoot, LONG
	 MiniSector, BYTE FAR *pBuf);
IOERR IO_ENTRYMOD IOOLE2RootStgReadConsecutiveMiniSector (PIOROOTSTORAGE pRoot
	, LONG StartMiniSector, LONG SectorNum, BYTE FAR *pBuf);
LONG IOOLE2RootStgGetNextMiniSector (PIOROOTSTORAGE pRoot, LONG MiniSector);
IOERR IO_ENTRYMOD IOOLE2RootStgGetDirEntry (PIOROOTSTORAGE pRoot, LONG DirEntry
	, BYTE FAR *pDirBuf);
SHORT IOOLE2DirNameCmp (BYTE FAR *pDirBuf, WORD ByteOrder, BYTE FAR *pDirName,
	 WORD NameLength);
IOERR IOOLE2RootStgFindChildEntry (HIOFILE hRefStorage, BYTE FAR *pChildName,
	 BYTE FAR *pDirEntryBuf, LONG FAR *pDirEntry, PIOROOTSTORAGE FAR *
	ppRoot);
IOERR IO_ENTRYMOD IORootStgClose (HIOFILE hFile);
IOERR IO_ENTRYMOD IORootStgRead (HIOFILE hFile, BYTE FAR *pData, DWORD dwSize,
	 DWORD FAR *pCount);
IOERR IO_ENTRYMOD IORootStgWrite (HIOFILE hFile, BYTE FAR *pData, DWORD dwSize
	, DWORD FAR *pCount);
IOERR IO_ENTRYMOD IORootStgSeek (HIOFILE hFile, WORD wFrom, LONG lOffset);
IOERR IO_ENTRYMOD IORootStgTell (HIOFILE hFile, DWORD FAR *pOffset);
IOERR IO_ENTRYMOD IORootStgGetInfo (HIOFILE hFile, DWORD dwInfoId, VOID FAR *
	pInfo);
IOERR IOOpenOLE2SubStorage (HIOFILE FAR *phFile, HIOSPEC hSpec, DWORD dwFlags);
IOERR IO_ENTRYMOD IOSubStgClose (HIOFILE hFile);
IOERR IO_ENTRYMOD IOSubStgRead (HIOFILE hFile, BYTE FAR *pData, DWORD dwSize,
	 DWORD FAR *pCount);
IOERR IO_ENTRYMOD IOSubStgWrite (HIOFILE hFile, BYTE FAR *pData, DWORD dwSize,
	 DWORD FAR *pCount);
IOERR IO_ENTRYMOD IOSubStgSeek (HIOFILE hFile, WORD wFrom, LONG lOffset);
IOERR IO_ENTRYMOD IOSubStgTell (HIOFILE hFile, DWORD FAR *pOffset);
IOERR IO_ENTRYMOD IOSubStgGetInfo (HIOFILE hFile, DWORD dwInfoId, VOID FAR *
	pInfo);
IOERR IOOpenOLE2SubStream (HIOFILE FAR *phFile, HIOSPEC hSpec, DWORD dwFlags);
IOERR IO_ENTRYMOD IOSubStrRead (HIOFILE hFile, BYTE FAR *pData, DWORD dwSize,
	 DWORD FAR *pCount);
IOERR IO_ENTRYMOD IOSubStrWrite (HIOFILE hFile, BYTE FAR *pData, DWORD dwSize,
	 DWORD FAR *pCount);
IOERR IO_ENTRYMOD IOSubStrSeek (HIOFILE hFile, WORD wFrom, LONG lOffset);
IOERR IO_ENTRYMOD IOSubStrTell (HIOFILE hFile, DWORD FAR *pOffset);
IOERR IO_ENTRYMOD IOSubStrGetInfo (HIOFILE hFile, DWORD dwInfoId, VOID FAR *
	pInfo);
IOERR IO_ENTRYMOD IOSubStrClose (HIOFILE hFile);
