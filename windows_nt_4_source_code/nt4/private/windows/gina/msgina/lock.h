/****************************** Module Header ******************************\
* Module Name: lock.h
*
* Copyright (c) 1991, Microsoft Corporation
*
* Define apis used to lock the workstation
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/


//
// Exported function prototypes
//


BOOL
CALLBACK
LockedDlgProc(HWND, UINT, WPARAM, LPARAM);


BOOL
CALLBACK
UnlockDlgProc(HWND, UINT, WPARAM, LPARAM);
