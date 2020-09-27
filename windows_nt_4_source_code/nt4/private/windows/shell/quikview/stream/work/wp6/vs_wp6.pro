/* VS_WP6.C 14/12/94 13.07.38 */
VW_ENTRYSC SHORT VW_ENTRYMOD VwStreamOpenFunc (SOFILE fp, SHORT wFileId, BYTE
	 VWPTR *pFileName, SOFILTERINFO VWPTR *pFilterInfo, HPROC hProc);
VW_LOCALSC WORD VW_LOCALMOD MatchBadWord (BYTE VWPTR *PrevWord, BYTE VWPTR *
	LastWord, HPROC hProc);
VW_LOCALSC VOID VW_LOCALMOD PreprocessFontName (BYTE VWPTR *FontName, HPROC
	 hProc);
VW_ENTRYSC SHORT VW_ENTRYMOD VwStreamSectionFunc (SOFILE fp, HPROC hProc);
VW_ENTRYSC VOID VW_ENTRYMOD VwStreamCloseFunc (SOFILE hFile, HPROC hProc);
VW_LOCALSC WORD VW_LOCALMOD GetWPG2Val (BYTE Prec, HPROC hProc);
VW_LOCALSC SHORT VW_LOCALMOD GetWpg2Size (DWORD VWPTR *WidthIn, DWORD VWPTR *
	HeightIn, HPROC hProc);
VW_LOCALSC WORD VW_LOCALMOD AllocateMemory (HANDLE VWPTR *h, LPBYTE VWPTR *lp,
	 WORD Size, WORD *Ok, HPROC hProc);
VW_LOCALSC WORD VW_LOCALMOD ReAllocateMemory (HANDLE VWPTR *h, LPBYTE VWPTR *
	lp, WORD Size, WORD VWPTR *OldSize, HPROC hProc);
VW_LOCALSC WORD VW_LOCALMOD GetWord (register HPROC hProc);
VW_LOCALSC DWORD VW_LOCALMOD GetLong (register HPROC hProc);
VW_LOCALSC LONG VW_LOCALMOD WPU (LONG dwVal, register HPROC hProc);
VW_LOCALSC SHORT VW_LOCALMOD ValidityCheckByte (BYTE VWPTR *pGroup, WORD
	 wTokenSize, HPROC hProc);
VW_LOCALSC VOID VW_LOCALMOD ParseEOLSubfunction (BYTE VWPTR *pData, register
	 HPROC hProc);
VW_LOCALSC VOID VW_LOCALMOD ParseBoxHeaderPacket (WORD pId, TOKEN VWPTR *t,
	 register HPROC hProc);
VW_LOCALSC VOID VW_LOCALMOD ParseBoxOverrideData (BYTE VWPTR *pData, TOKEN
	 VWPTR *t, register HPROC hProc);
VW_LOCALSC VOID VW_LOCALMOD InitStruct (register WP6_SAVE *Wp6Save, HPROC hProc
	);
VW_LOCALSC SHORT VW_LOCALMOD PutCharHeight (HPROC hProc);
VW_LOCALSC SHORT VW_LOCALMOD AttributeHandler (BYTE Attribute, WORD SoVal,
	 HPROC hProc);
VW_LOCALSC VOID VW_LOCALMOD ReadTabstops (BYTE VWPTR *pData, HPROC hProc);
VW_LOCALSC SHORT VW_LOCALMOD PutTabstops (HPROC hProc);
VW_LOCALSC DWORD VW_LOCALMOD PreviousTabstopPosition (LONG LinePosition, HPROC
	 hProc);
VW_LOCALSC DWORD VW_LOCALMOD NextTabstopPosition (LONG LinePosition, HPROC
	 hProc);
VW_LOCALSC VOID VW_LOCALMOD DefineBorders (SOBORDER VWPTR *Border, WORD
	 PacketIndex, WORD Priority, BYTE OverrideColors, BYTE r, BYTE g, BYTE
	 b, HPROC hProc);
VW_ENTRYSC VOID VW_ENTRYMOD GiveRowInformation (BYTE EndTable, HPROC hProc);
VW_LOCALSC VOID VW_LOCALMOD StartCellPidHandler (TOKEN VWPTR *t, HPROC hProc);
VW_LOCALSC VOID VW_LOCALMOD CellAttributeHandler (WORD SoVal, HPROC hProc);
VW_LOCALSC VOID VW_LOCALMOD StartCellAttributeHandler (HPROC hProc);
VW_LOCALSC VOID VW_LOCALMOD EndCellParaAttributeHandler (WORD cCell, register
	 HPROC hProc);
VW_LOCALSC VOID VW_LOCALMOD EndCellSymbolAttributeHandler (register HPROC hProc
	);
VW_LOCALSC VOID VW_LOCALMOD HandleFirstSymbol (register HPROC hProc);
VW_LOCALSC VOID VW_LOCALMOD SendParaIndents (register HPROC hProc);
VW_LOCALSC VOID VW_LOCALMOD HandleHardBreak (register HPROC hProc);
VW_LOCALSC SHORT VW_LOCALMOD AdjustMemoryBuffer (LPBYTE VWPTR *pGroup, LPBYTE
	 VWPTR *EndGroup, WORD TokenSize, register HPROC hProc);
VW_LOCALSC VOID VW_LOCALMOD WPMapChar (WORD Page, WORD Char, register HPROC
	 hProc);
VW_ENTRYSC SHORT VW_ENTRYMOD VwStreamReadFunc (SOFILE fp, register HPROC hProc
	);
VW_ENTRYSC SHORT VW_ENTRYMOD VwStreamTellFunc (SOFILE fp, HPROC hProc);
VW_ENTRYSC SHORT VW_ENTRYMOD VwStreamSeekFunc (SOFILE hFile, HPROC hProc);
