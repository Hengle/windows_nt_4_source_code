/* VWCAPT_W.C 18/03/94 10.03.40 */
BOOL VWHandleDrawGraphicCaption (XVIEWINFO ViewInfo, LPSCCDDRAWGRAPHIC
	 lpDrawGraphic);
BOOL VWDrawCaption (XVIEWINFO ViewInfo, LPSCCDDRAWGRAPHIC lpDrawGraphic, LPBYTE
	 lpCaption);
IOERR VWOpenEmbedding (LPSCCVWOPENEMBED lpOpen, XVIEWINFO ViewInfo,
	 PSOOBJECTLOC lpLoc, BOOL bStream);
IOERR VWCloseEmbedding (LPSCCVWOPENEMBED lpOpen, XVIEWINFO ViewInfo);
