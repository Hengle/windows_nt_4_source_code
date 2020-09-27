/* VS_EXE2.C 10/01/95 13.02.42 */
VW_ENTRYSC SHORT VW_ENTRYMOD
VwStreamOpen (
	SOFILE fp,
	SHORT FileId,
	BYTE VWPTR *FileName,
	SOFILTERINFO VWPTR *FilterInfo,
	HPROC hProc
	);

VW_ENTRYSC SHORT VW_ENTRYMOD
VwStreamSection (
	SOFILE fp,
	HPROC hProc
	);

VW_ENTRYSC SHORT VW_ENTRYMOD
VwStreamRead (
	SOFILE fp,
	HPROC hProc
	);

VW_ENTRYSC SHORT VW_ENTRYMOD
VwStreamTell (
	SOFILE fp,
	HPROC hProc
	);

VW_ENTRYSC SHORT VW_ENTRYMOD
VwStreamSeek (
	SOFILE fp,
	HPROC hProc
	);

VW_LOCALSC SHORT VW_LOCALMOD
ReadExeOldHeader (
	SOFILE fp,
	OLDEXEHDR VWPTR * pOldHdr,
	HPROC hProc
	);

VW_LOCALSC SHORT VW_LOCALMOD
ReadExePeHeader (
	SOFILE fp,
	PE_IMAGE_FILE_HDR VWPTR *pPeHdr,
	HPROC hProc
	);

VW_LOCALSC SHORT VW_LOCALMOD
ReadExeExportTable (
	SOFILE fp,
	PE_EXPORT_TABLE * pPeExportTable,
	HPROC hProc
	);

VW_LOCALSC SHORT VW_LOCALMOD
ReadExeImportTable (
	SOFILE fp,
	PE_IMPORT_TABLE * pPeImportTable,
	HPROC hProc
	);

VW_LOCALSC SHORT VW_LOCALMOD
ReadExePeSectionTable (
	SOFILE fp,
	PE_IMAGE_SECTION_HDR VWPTR *pPeSec,
	WORD sec_count,
	HPROC hProc
	);

VW_LOCALSC SHORT VW_LOCALMOD
ReadExeNewHeader (
	SOFILE fp,
	NEWEXEHDR VWPTR * pNewHdr,
	OLDEXEHDR VWPTR * pOldHdr,
	HPROC hProc
	);

VW_LOCALSC VOID VW_LOCALMOD
HandleSectionAttributes (
	HPROC hProc
	);

VW_LOCALSC VOID VW_LOCALMOD
DisplaySectionHeading (
	WORD HeadingIndex,
	HPROC hProc
	);

VW_LOCALSC VOID VW_LOCALMOD
DisplaySubHeading (
	WORD HeadingIndex,
	HPROC hProc
	);

VW_LOCALSC VOID VW_LOCALMOD
PutString (
	WORD idResource,
	HPROC hProc
	);

VW_LOCALSC VOID VW_LOCALMOD
OldPutString (
	BYTE VWPTR *pString,
	HPROC hProc
	);

VW_LOCALSC VOID VW_LOCALMOD
my_putvalue (
	WORD wData,
	SHORT iBase,
	SHORT iMin,
	HPROC hProc
	);

VW_LOCALSC WORD VW_LOCALMOD
GetWord (
	SOFILE fp,
	HPROC hProc
	);

VW_LOCALSC DWORD VW_LOCALMOD
GetDouble (
	SOFILE fp,
	HPROC hProc
	);

VW_LOCALSC WORD VW_LOCALMOD
ReadAndPutPascalString (
	SOFILE fp,
	DWORD offset,
	HPROC hProc
	);

VW_LOCALSC BYTE VWPTR * VW_LOCALMOD
SULoadString (
	WORD idResource,
	HPROC hProc
	);

VW_LOCALSC WORD VW_LOCALMOD
ReadPascalString (
	SOFILE fp,
	BYTE VWPTR *pString,
	HPROC hProc
	);
