/* VW_W.C 18/12/94 15.29.06 */
DECALLBACK_ENTRYSC VOID DECALLBACK_ENTRYMOD VWBeginDraw (LPSCCDGENINFO
	 lpGenInfo);
DECALLBACK_ENTRYSC VOID DECALLBACK_ENTRYMOD VWEndDraw (LPSCCDGENINFO lpGenInfo
	);
WIN_ENTRYSC LRESULT WIN_ENTRYMOD SccVwNoFileWndProc (HWND hWnd, UINT message,
	 WPARAM wParam, LPARAM lParam);
VOID VWPaintNoFileWnd (HWND hWnd, HDC hDC, XVIEWINFO ViewInfo);
VOID VWHandleSize (XVIEWINFO ViewInfo, WORD wWidth, WORD wHeight);
WIN_ENTRYSC LRESULT WIN_ENTRYMOD SccVwDisplayWndProc (HWND hWnd, UINT message,
	 WPARAM wParam, LPARAM lParam);
LONG VWGetDisplayInfo (XVIEWINFO ViewInfo, LPSCCVWDISPLAYINFO lpDisplayInfo);
VOID VWAddSectionNameToMenuNP (XVIEWINFO ViewInfo, WORD wSection);
VOID VWDisplaySectionMenuNP (XVIEWINFO ViewInfo);
VOID VWPaintWnd (XVIEWINFO ViewInfo);
VOID VWLeftButtonDown (XVIEWINFO ViewInfo, WORD wX, WORD wY);
VOID VWHandleCommand (XVIEWINFO ViewInfo, HWND hControl, WORD wCommand, WORD
	 wId);
VOID VWSetFocus (XVIEWINFO ViewInfo, HWND hOldWnd);
WIN_ENTRYSC LRESULT WIN_ENTRYMOD SccSecListWndProc (HWND hWnd, UINT message,
	 WPARAM wParam, LPARAM lParam);
WIN_ENTRYSC VOID WIN_ENTRYMOD SccVwTimerFunc (HWND hWnd, UINT wMsg, UINT nEvent
	, DWORD dwTime);
VOID VWFirstSection (XVIEWINFO ViewInfo);
VOID VWLastSection (XVIEWINFO ViewInfo);
VOID VWNextSection (XVIEWINFO ViewInfo);
VOID VWPriorWalterSection (XVIEWINFO ViewInfo);
