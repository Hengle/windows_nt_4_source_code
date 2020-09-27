/****************************** Module Header ******************************\
* Module Name: winutil.h
*
* Copyright (c) 1991, Microsoft Corporation
*
* Define windows utility functions
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/


//
// Exported function prototypes
//

HWND
CreateAniOnControl(
    HWND        hDlg,
    DWORD       ControlId,
    DWORD       AniId);

VOID
SetupSystemMenu(
    HWND    hwnd
    );


VOID
CentreWindow(
    HWND    hwnd
    );

BOOL
SetPasswordFocus(
    HWND    hDlg
    );

VOID
SetupCursor(
    BOOL    fWait
    );

BOOL
FormatTime(
   IN PTIME Time,
   OUT PWCHAR Buffer,
   IN ULONG BufferLength,
   IN USHORT Flags
   );

//
// Define flags used by FormatTime routine
//
#define FT_TIME 1
#define FT_DATE 2


BOOL
DuplicateUnicodeString(
    PUNICODE_STRING OutString,
    PUNICODE_STRING InString
    );

LPTSTR
UnicodeStringToString(
    PUNICODE_STRING UnicodeString
    );

BOOL
StringToUnicodeString(
    PUNICODE_STRING UnicodeString,
    LPTSTR String
    );

BOOL
PaintBitmapWindow(
    HWND    hDlg,
    HINSTANCE   hInstance,
    int     ControlId,
    WORD    BitmapId
    );

BOOL
OpenIniFileUserMapping(
    PGLOBALS pGlobals
    );

VOID
CloseIniFileUserMapping(
    PGLOBALS pGlobals
    );

LPTSTR
AllocAndGetDlgItemText(
    HWND hDlg,
    int  iItem
    );

BOOL
HandleComboBoxOK(
    HWND    hDlg,
    int     ComboBoxId
    );

LPTSTR
AllocAndGetPrivateProfileString(
    LPCTSTR lpAppName,
    LPCTSTR lpKeyName,
    LPCTSTR lpDefault,
    LPCTSTR lpFileName
    );

#define AllocAndGetProfileString(App, Key, Def) \
            AllocAndGetPrivateProfileString(App, Key, Def, NULL)


BOOL
WritePrivateProfileInt(
    LPCTSTR lpAppName,
    LPCTSTR lpKeyName,
    UINT Value,
    LPCTSTR lpFileName
    );

#define WriteProfileInt(App, Key, Value) \
            WritePrivateProfileInt(App, Key, Value, NULL)


LPTSTR
AllocAndExpandEnvironmentStrings(
    LPCTSTR lpszSrc
    );

LPTSTR
AllocAndRegEnumKey(
    HKEY hKey,
    DWORD iSubKey
    );

LPTSTR
AllocAndRegQueryValueEx(
    HKEY hKey,
    LPTSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType
    );

BOOL
SetEnvironmentULong(
    LPTSTR Variable,
    ULONG Value
    );

BOOL
SetEnvironmentLargeInt(
    LPTSTR Variable,
    LARGE_INTEGER Value
    );

LPWSTR
EncodeMultiSzW(
    IN LPWSTR MultiSz
    );


//
// Memory macros
//

#define Alloc(c)        ((PVOID)LocalAlloc(LPTR, c))
#define ReAlloc(p, c)   ((PVOID)LocalReAlloc(p, c, LPTR | LMEM_MOVEABLE))
#define Free(p)         ((VOID)LocalFree(p))


//
// Define a winlogon debug print routine
//

#define WLPrint(s)  KdPrint(("WINLOGON: ")); \
                    KdPrint(s);            \
                    KdPrint(("\n"));

DLG_RETURN_TYPE TimeoutMessageBox(
    HWND hwnd,
    UINT IdText,
    UINT IdCaption,
    UINT wType,
    TIMEOUT Timeout);


DLG_RETURN_TYPE TimeoutMessageBoxlpstr(
    HWND hwnd,
    LPTSTR Text,
    LPTSTR Caption,
    UINT wType,
    TIMEOUT Timeout);

#define TIMEOUT_VALUE_MASK  (0x0fffffff)
#define TIMEOUT_NOTIFY_MASK (0x10000000)

#define TIMEOUT_VALUE(t)    (t & TIMEOUT_VALUE_MASK)
#define TIMEOUT_NOTIFY(t)   (t & TIMEOUT_NOTIFY_MASK)

#define TIMEOUT_SS_NOTIFY   (TIMEOUT_NOTIFY_MASK)
#define TIMEOUT_CURRENT     (TIMEOUT_VALUE_MASK)    // Use existing timeout
#define TIMEOUT_NONE        (0)                     // Disable input timeout


PWSTR
DupString(PWSTR pszString);

PWSTR
DupUnicodeString(PUNICODE_STRING pString);
