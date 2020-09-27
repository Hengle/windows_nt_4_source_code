/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       REGEDIT.H
*
*  VERSION:     4.0
*
*  AUTHOR:      Tracy Sharpe
*
*  DATE:        21 Nov 1993
*
*  Common header file for the Registry Editor.  Precompiled header.
*
********************************************************************************
*
*  CHANGE LOG:
*
*  DATE        REV DESCRIPTION
*  ----------- --- -------------------------------------------------------------
*  21 Nov 1993 TCS Original implementation.
*
*******************************************************************************/

#ifndef _INC_REGEDIT
#define _INC_REGEDIT

//  Class name of main application window.
extern const CHAR g_RegEditClassName[];

#define IDC_KEYTREE                     1
#define IDC_VALUELIST                   2
#define IDC_STATUSBAR                   3

typedef struct _REGEDITDATA {
    HWND hKeyTreeWnd;
    HWND hValueListWnd;
    HWND hStatusBarWnd;
    HWND hFocusWnd;
    int xPaneSplit;
    HIMAGELIST hImageList;
    HKEY hCurrentSelectionKey;
    int SelChangeTimerState;
    int StatusBarShowCommand;
    PSTR pDefaultValue;
    PSTR pValueNotSet;
    PSTR pEmptyBinary;
    PSTR pCollapse;
    PSTR pModify;
    PSTR pNewKeyTemplate;
    PSTR pNewValueTemplate;
    BOOL fAllowLabelEdits;
    HMENU hMainMenu;
    BOOL fMainMenuInited;
    BOOL fHaveNetwork;
    BOOL fProcessingFind;
}   REGEDITDATA, *PREGEDITDATA;

extern REGEDITDATA g_RegEditData;

#define SCTS_TIMERCLEAR                 0
#define SCTS_TIMERSET                   1
#define SCTS_INITIALIZING               2

BOOL
PASCAL
RegisterRegEditClass(
    VOID
    );

HWND
PASCAL
CreateRegEditWnd(
    VOID
    );

VOID
PASCAL
RegEdit_OnCommand(
    HWND hWnd,
    int DlgItem,
    HWND hControlWnd,
    UINT NotificationCode
    );

VOID
PASCAL
RegEdit_SetNewObjectEditMenuItems(
    HMENU hPopupMenu
    );

VOID
PASCAL
RegEdit_SetWaitCursor(
    BOOL fSet
    );

#define REM_UPDATESTATUSBAR             (WM_USER + 1)

#endif // _INC_REGEDIT
