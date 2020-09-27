//THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
//ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
//THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//
// Copyright  1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//      MODULE:         terminal.c
//
//      PURPOSE:        Terminal screen simulation
//
//      FUNCTIONS:
//              CreateTerminalDlg()
//              TerminalDlgWndProc()
//              TerminalScreenWndProc()
//              OnCommand()
//              GetInput()
//              SendCharacter()
//              AdjustTerminal()
//              TerminalThread()
//

#include "unimdm.h"  // includes common header files and global declarations
#include "umdmspi.h" // includes common header files and global declarations
#include "wndthrd.h" // includes UI declarations
#include "rcids.h"   // includes the resource definitions

//****************************************************************************
// Constants Declaration
//****************************************************************************

#define MAXTITLE               32
#define MAXMESSAGE             256

#define WM_MODEMNOTIFY         (WM_USER + 998)
#define WM_EOLFROMDEVICE       (WM_USER + 999)

#define SIZE_ReceiveBuf        1024
#define SIZE_SendBuf           1

#define Y_MARGIN               4
#define X_SPACING              2
#define MIN_X                  170
#define MIN_Y                  80

#define TERMINAL_BK_COLOR      (RGB( 0, 0, 0 ))
#define TERMINAL_FR_COLOR      (RGB( 255, 255, 255 ))
#define MAXTERMLINE            24

#define READ_EVENT             0
#define STOP_EVENT             1
#define MAX_EVENT              2

//****************************************************************************
// Type Definitions
//****************************************************************************

typedef struct  tagTERMDLG {
    HANDLE   hport;
    HANDLE   hThread;
    HANDLE   hEvent[MAX_EVENT];
    HWND     hwnd;
    PBYTE    pbyteReceiveBuf;
    PBYTE    pbyteSendBuf;
    HBRUSH   hbrushScreenBackground;
    HFONT    hfontTerminal;
    WNDPROC  WndprocOldTerminalScreen;
    DWORD    idLine;
    HWND     ParenthWnd;
}   TERMDLG, *PTERMDLG, FAR* LPTERMDLG;

//****************************************************************************
// Function prototypes
//****************************************************************************

LRESULT FAR PASCAL TerminalDlgWndProc(HWND   hwnd,
                                      UINT   wMsg,
                                      WPARAM wParam,
                                      LPARAM lParam );
LRESULT FAR PASCAL TerminalScreenWndProc(HWND   hwnd,
                                         UINT   wMsg,
                                         WPARAM wParam,
                                         LPARAM lParam );
BOOL NEAR PASCAL OnCommand (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL NEAR PASCAL GetInput  (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
VOID NEAR PASCAL SendCharacter( HWND hwnd, BYTE byte );
VOID NEAR PASCAL AdjustTerminal (HWND hwnd, int wWidth, int wHeight);
void WINAPI      TerminalThread (PTERMDLG  pTerminaldialog);

//****************************************************************************
// HWND CreateTerminalDlg(HWND hwndOwner, DWORD idLine)
//
// Function: creates a modeless terminal dialog box
//
// Returns:  the modeless window handle
//
//****************************************************************************

HWND CreateTerminalDlg(HWND hwndOwner, DWORD idLine)
{
  HANDLE    hComm;
  HWND      hwnd;
  COMMTIMEOUTS commtimeout;
  PTERMDLG  pTerminaldialog;
  DWORD     id;
  int       i;
  int       iRet;
  TERMREQ   TermReq;

  TUISPIDLLCALLBACK   Callback;

  Callback=GetCallbackProc(hwndOwner);

  // Get the terminal parameters
  //
  TermReq.DlgReq.dwCmd   = UI_REQ_TERMINAL_INFO;
  TermReq.DlgReq.dwParam = GetCurrentProcessId();

  (*Callback)(idLine, TUISPIDLL_OBJECT_LINEID,
                    (LPVOID)&TermReq, sizeof(TermReq));                          
  hComm = TermReq.hDevice;

  // Allocate the terminal buffer
  //
  if ((pTerminaldialog = (PTERMDLG)LocalAlloc(LPTR, sizeof(*pTerminaldialog)))
      == NULL)
    return NULL;

  if ((pTerminaldialog->pbyteReceiveBuf = (PBYTE)LocalAlloc(LMEM_FIXED,
							    SIZE_ReceiveBuf
                                                            + SIZE_SendBuf))
      == NULL)
  {
    LocalFree((HLOCAL)pTerminaldialog);
    return NULL;
  };
  pTerminaldialog->pbyteSendBuf = pTerminaldialog->pbyteReceiveBuf + SIZE_ReceiveBuf;

  // Initialize the terminal buffer
  //
  pTerminaldialog->ParenthWnd= hwndOwner;
  pTerminaldialog->hport   = hComm;
  pTerminaldialog->idLine  = idLine;
  pTerminaldialog->hbrushScreenBackground = (HBRUSH)GetStockObject( BLACK_BRUSH );
  pTerminaldialog->hfontTerminal = (HFONT)GetStockObject( SYSTEM_FIXED_FONT );

  // Start receiving from the port
  //
  commtimeout.ReadIntervalTimeout = MAXDWORD;
  commtimeout.ReadTotalTimeoutMultiplier = 0;
  commtimeout.ReadTotalTimeoutConstant   = 0;
  commtimeout.WriteTotalTimeoutMultiplier= 0;
  commtimeout.WriteTotalTimeoutConstant  = 1000;
  SetCommTimeouts(hComm, &commtimeout);

  SetCommMask(hComm, EV_RXCHAR);

  if (TermReq.dwTermType == TERMINAL_PRE) {

#define ECHO_OFF "ATE1\r"

      COMMTIMEOUTS commtimeout;
      HANDLE       hEvent;

      hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);


      if (hEvent != NULL) {

          DWORD        cb;
          OVERLAPPED   ov;
          BOOLEAN      bResult;

          ov.Offset       = 0;
          ov.OffsetHigh   = 0;

          // OR with 1 to prevent it from being posted to the completion port.
          //
          ov.hEvent       = (HANDLE)((DWORD)hEvent | 1);

          bResult=WriteFile(
              hComm,
              ECHO_OFF,
              sizeof(ECHO_OFF)-1,
              &cb,
              &ov
              );


          if (!bResult) {

              DWORD dwResult = GetLastError();

              if (ERROR_IO_PENDING == dwResult) {

                  GetOverlappedResult(
                      hComm,
                      &ov,
                      &cb,
                      TRUE
                      );
              }
          }

          CloseHandle(hEvent);
      }
  }


  // Create read thread and the synchronization objects
  for (i = 0; i < MAX_EVENT; i++)
  {
    pTerminaldialog->hEvent[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
  };

  pTerminaldialog->hThread = CreateThread(NULL, 0,
                                         (LPTHREAD_START_ROUTINE) TerminalThread,
                                         pTerminaldialog, 0, &id);

  // Create the terminal window
  hwnd = CreateDialogParam(ghInstance,
                        MAKEINTRESOURCE(IDD_TERMINALDLG),
                        hwndOwner,
                        (DLGPROC)TerminalDlgWndProc,
                        (LPARAM)pTerminaldialog);

  if (IsWindow(hwnd))
  {
    TCHAR szTitle[MAXTITLE];

    // Set window caption
    //
    LoadString (ghInstance,
                (TermReq.dwTermType == TERMINAL_POST)?
                IDS_POSTTERM_TITLE : IDS_PRETERM_TITLE,
                szTitle, sizeof(szTitle));
    SetWindowText(hwnd, szTitle);
  }
  else
  {
    // The terminal dialog was terminalted, free resources
    //
    SetEvent(pTerminaldialog->hEvent[STOP_EVENT]);

    SetCommMask(hComm, 0);
    WaitForSingleObject(pTerminaldialog->hThread, INFINITE);
    CloseHandle(pTerminaldialog->hThread);

    for (i = 0; i < MAX_EVENT; i++)
    {
      CloseHandle(pTerminaldialog->hEvent[i]);
    };

    LocalFree((HLOCAL)pTerminaldialog->pbyteReceiveBuf);
    LocalFree((HLOCAL)pTerminaldialog);

    hwnd = NULL;
  };

  return hwnd;
}


/*----------------------------------------------------------------------------
** Terminal Window Procedure
**----------------------------------------------------------------------------
*/

LRESULT FAR PASCAL TerminalDlgWndProc(HWND   hwnd,
                                      UINT   wMsg,
                                      WPARAM wParam,
                                      LPARAM lParam )
{
  PTERMDLG pTerminaldialog;
  HWND     hwndScrn;
  RECT     rect;

  switch (wMsg)
  {
    case WM_INITDIALOG:
      pTerminaldialog = (PTERMDLG)lParam;
      SetWindowLong(hwnd, DWL_USER, (LONG)lParam);
      SetForegroundWindow(hwnd);
      pTerminaldialog->hwnd = hwnd;

      // Install subclassed WndProcs.
      //
      hwndScrn = GetDlgItem(hwnd, CID_T_EB_SCREEN);
      pTerminaldialog->WndprocOldTerminalScreen =
          (WNDPROC)SetWindowLong( hwndScrn, GWL_WNDPROC,
                                  (LONG)TerminalScreenWndProc );

      // Set the terminal screen font
      //
      SendMessage(hwndScrn, WM_SETFONT, (WPARAM)pTerminaldialog->hfontTerminal,
                  0L);

      // Adjust the dimension
      //
      GetClientRect(hwnd, &rect);
      AdjustTerminal(hwnd, rect.right-rect.left, rect.bottom-rect.top);

      // Start receiving from the port
      //
      PostMessage(hwnd, WM_MODEMNOTIFY, 0, 0);

      // Set the input focus to the screen
      //
      SetFocus(hwndScrn);
      return 0;

    case WM_CTLCOLOREDIT:
    {
      pTerminaldialog = (PTERMDLG)GetWindowLong(hwnd, DWL_USER);

      /* Set terminal screen colors to TTY-ish green on black.
      */
      if (pTerminaldialog->hbrushScreenBackground)
      {
        SetBkColor( (HDC)wParam,  TERMINAL_BK_COLOR );
        SetTextColor((HDC)wParam, TERMINAL_FR_COLOR );

        return (LRESULT)pTerminaldialog->hbrushScreenBackground;
      }

      break;
    };

    case WM_MODEMNOTIFY:
      return GetInput(hwnd, wMsg, wParam, lParam);

    case WM_COMMAND:

      // Handle the control activities
      //
      return OnCommand(hwnd, wMsg, wParam, lParam);

    case WM_DESTROY:
    {
      DLGREQ  DlgReq;
      int   i;
      TUISPIDLLCALLBACK   Callback;
      
      pTerminaldialog = (PTERMDLG)GetWindowLong(hwnd, DWL_USER);
      SetWindowLong( GetDlgItem(hwnd, CID_T_EB_SCREEN), GWL_WNDPROC,
                     (LONG)pTerminaldialog->WndprocOldTerminalScreen );

      // Destroy the dialog
      //
      DlgReq.dwCmd = UI_REQ_END_DLG;
      DlgReq.dwParam = TERMINAL_DLG;

      Callback=GetCallbackProc(pTerminaldialog->ParenthWnd);

      (*Callback)(pTerminaldialog->idLine, TUISPIDLL_OBJECT_LINEID,
                        (LPVOID)&DlgReq, sizeof(DlgReq));

      // The terminal dialog was terminated, free resources
      //
      SetEvent(pTerminaldialog->hEvent[STOP_EVENT]);

      if (pTerminaldialog->hport != NULL)
      {
        SetCommMask(pTerminaldialog->hport, 0);
      };

      if (pTerminaldialog->hThread != NULL)
      {
        WaitForSingleObject(pTerminaldialog->hThread, INFINITE);
        CloseHandle(pTerminaldialog->hThread);
      };
      CloseHandle(pTerminaldialog->hport);                                                    

      for (i = 0; i < MAX_EVENT; i++)
      {
        CloseHandle(pTerminaldialog->hEvent[i]);
      };

      LocalFree((HLOCAL)pTerminaldialog->pbyteReceiveBuf);
      LocalFree((HLOCAL)pTerminaldialog);
      break;
    }
    case WM_SIZE:
      AdjustTerminal(hwnd, (int)LOWORD(lParam), (int)HIWORD(lParam));
      break;

    case WM_GETMINMAXINFO:
    {
      MINMAXINFO FAR* lpMinMaxInfo = (MINMAXINFO FAR*)lParam;
      DWORD           dwUnit = GetDialogBaseUnits();

      lpMinMaxInfo->ptMinTrackSize.x = (MIN_X*LOWORD(dwUnit))/4;
      lpMinMaxInfo->ptMinTrackSize.y = (MIN_Y*LOWORD(dwUnit))/4;
      break;
    };
  };

  return 0;
}

/*----------------------------------------------------------------------------
** Terminal Screen Subclasses Window Procedure
**----------------------------------------------------------------------------
*/

LRESULT FAR PASCAL TerminalScreenWndProc(HWND   hwnd,
                                         UINT   wMsg,
                                         WPARAM wParam,
                                         LPARAM lParam )
{
  HWND     hwndParent;
  PTERMDLG pTerminaldialog;

  hwndParent      = GetParent(hwnd);
  pTerminaldialog = (PTERMDLG)GetWindowLong(hwndParent, DWL_USER);

  if (wMsg == WM_EOLFROMDEVICE)
  {
    /* Remove the first line if the next line exceeds the maximum line
    */
    if (SendMessage(hwnd, EM_GETLINECOUNT, 0, 0L) == MAXTERMLINE)
    {
      SendMessage(hwnd, EM_SETSEL, 0,
                  SendMessage(hwnd, EM_LINEINDEX, 1, 0L));
      SendMessage(hwnd, EM_REPLACESEL, 0, (LPARAM)(LPTSTR)TEXT(""));
      SendMessage(hwnd, EM_SETSEL, 32767, 32767);
      SendMessage(hwnd, EM_SCROLLCARET, 0, 0);
    };

    /* An end-of-line in the device input was received.  Send a linefeed
    ** character to the window.
    */
    wParam = '\n';
    wMsg = WM_CHAR;
  }
  else
  {
    BOOL fCtrlKeyDown = (GetKeyState( VK_CONTROL ) < 0);
    BOOL fShiftKeyDown = (GetKeyState( VK_SHIFT ) < 0);

    if (wMsg == WM_KEYDOWN)
    {
      /* The key was pressed by the user.
      */
      if (wParam == VK_RETURN && !fCtrlKeyDown && !fShiftKeyDown)
      {
        /* Enter key pressed without Shift or Ctrl is discarded.  This
        ** prevents Enter from being interpreted as "press default
        ** button" when pressed in the edit box.
        */
        return 0;
      }

      if (fCtrlKeyDown && wParam == VK_TAB)
      {
        /* Ctrl+Tab pressed.  Send a tab character to the device.
        ** Pass tab thru to let the edit box handle the visuals.
        ** Ctrl+Tab doesn't generate a WM_CHAR.
        */
        SendCharacter( hwndParent, (BYTE )VK_TAB );
      }

      if (GetKeyState( VK_MENU ) < 0)
      {
        return (CallWindowProc(pTerminaldialog->WndprocOldTerminalScreen, hwnd, wMsg, wParam, lParam ));
      };
    }
    else if (wMsg == WM_CHAR)
    {
      /* The character was typed by the user.
      */
      if (wParam == VK_TAB)
      {
        /* Ignore tabs...Windows sends this message when Tab (leave
        ** field) is pressed but not when Ctrl+Tab (insert a TAB
        ** character) is pressed...weird.
        */
        return 0;
      }

#ifdef UNICODE
      {
        CHAR chAnsi;

        if (WideCharToMultiByte(CP_ACP,
                                0,
                                (LPWSTR)&wParam,
                                1,
                                &chAnsi,
                                1,
                                NULL,
                                NULL))
        {
          SendCharacter( hwndParent, (BYTE )chAnsi );
        }
      }
#else // UNICODE
      SendCharacter( hwndParent, (BYTE )wParam );
#endif // UNICODE

      return 0;
    }
  }

  /* Call the previous window procedure for everything else.
  */
  return (CallWindowProc(pTerminaldialog->WndprocOldTerminalScreen, hwnd, wMsg, wParam, lParam ));
}

/*----------------------------------------------------------------------------
** Terminal Window's Control Handler
**----------------------------------------------------------------------------
*/

BOOL NEAR PASCAL OnCommand (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (GET_WM_COMMAND_ID(wParam, lParam))
  {
    case CID_T_EB_SCREEN:
    {
      switch (HIWORD(wParam))
      {
        case EN_SETFOCUS:
        {
          /* Turn off the default button whenever the terminal
          ** window has the focus.  Pressing [Return] in the
          ** terminal acts like a normal terminal.
          */
          SendDlgItemMessage(hwnd, CID_T_PB_ENTER, BM_SETSTYLE,
                             (WPARAM)BS_DEFPUSHBUTTON, TRUE);

          /* Don't select the entire string on entry.
          */
          SendDlgItemMessage(hwnd, CID_T_EB_SCREEN, EM_SETSEL,
                             32767, 32767);
          SendMessage(hwnd, EM_SCROLLCARET, 0, 0);
          break;
        };
      };

      break;
    };

    case IDOK:
    case IDCANCEL:
    {
      PTERMDLG  pTerminaldialog;

      pTerminaldialog = (PTERMDLG)GetWindowLong(hwnd, DWL_USER);
      EndMdmDialog(pTerminaldialog->ParenthWnd,pTerminaldialog->idLine, TERMINAL_DLG,
                   (GET_WM_COMMAND_ID(wParam, lParam) == IDOK) ?
                   MDM_SUCCESS : MDM_HANGUP);
      break;
    }
  };
  return 0;
}


/*----------------------------------------------------------------------------
** Terminal Input Handler
**----------------------------------------------------------------------------
*/

BOOL NEAR PASCAL GetInput (HWND   hwnd,
                           UINT   usMsg,
                           WPARAM wParam,
                           LPARAM lParam )
{
  PTERMDLG  pTerminaldialog;
  DWORD     cbRead;
  OVERLAPPED ov;
  HANDLE    hEvent;
  COMMTIMEOUTS commtimeout;

  pTerminaldialog = (PTERMDLG)GetWindowLong(hwnd, DWL_USER);

  if ((hEvent = CreateEvent(NULL, TRUE, FALSE, NULL)) == NULL)
  {
    return FALSE;
  }

  // Set write timeout to one second
  //
  commtimeout.ReadIntervalTimeout = MAXDWORD;
  commtimeout.ReadTotalTimeoutMultiplier = 0;
  commtimeout.ReadTotalTimeoutConstant   = 0;
  commtimeout.WriteTotalTimeoutMultiplier= 0;
  commtimeout.WriteTotalTimeoutConstant  = 1000;
  SetCommTimeouts(pTerminaldialog->hport, &commtimeout);

  do
  {
    /* Make sure we still have the comm port
    */
    if (pTerminaldialog->hport == NULL)
      break;

    /* A character has been received from the device.
    */
    ov.Internal     = 0;
    ov.InternalHigh = 0;
    ov.Offset       = 0;
    ov.OffsetHigh   = 0;
    ov.hEvent       = (HANDLE)((DWORD)hEvent | 1);

    cbRead          = 0;

    if (FALSE == ReadFile(pTerminaldialog->hport,
                          pTerminaldialog->pbyteReceiveBuf,
                          SIZE_ReceiveBuf, (LPDWORD)&cbRead, &ov))
    {
      DWORD dwResult = GetLastError();

      if (ERROR_IO_PENDING == dwResult)
      {
        GetOverlappedResult(pTerminaldialog->hport,
                            &ov,
                            &cbRead,
                            TRUE);
      }
      else
      {
        DPRINTF1("ReadFile() in GetInput() failed (0x%8x)!", dwResult);
      }
    };

    SetEvent(pTerminaldialog->hEvent[READ_EVENT]);

    /* Send the device talk to the terminal edit box.
    */
    if (cbRead != 0)
    {
        char  szBuf[ SIZE_ReceiveBuf + 1 ];
#ifdef UNICODE
        WCHAR szUnicodeBuf[ SIZE_ReceiveBuf + 1 ];
#endif // UNICODE
        LPSTR pch = szBuf;
        int   i, cb;
        HWND  hwndScrn = GetDlgItem(hwnd, CID_T_EB_SCREEN);

        cb = cbRead;
        for (i = 0; i < cb; ++i)
        {
            char ch = pTerminaldialog->pbyteReceiveBuf[ i ];

            /* Formatting: Converts CRs to LFs (there seems to be no VK_
            ** for LF) and throws away LFs.  This prevents the user from
            ** exiting the dialog when they press Enter (CR) in the
            ** terminal screen.  LF looks like CRLF in the edit box.  Also,
            ** throw away TABs because otherwise they change focus to the
            ** next control.
            */
            if (ch == VK_RETURN)
            {
                /* Must send whenever end-of-line is encountered because
                ** EM_REPLACESEL doesn't handle VK_RETURN characters well
                ** (prints garbage).
                */
                *pch = '\0';

                /* Turn off current selection, if any, and replace the null
                ** selection with the current buffer.  This has the effect
                ** of adding the buffer at the caret.  Finally, send the
                ** EOL to the window which (unlike EM_REPLACESEL) handles
                ** it correctly.
                */
                SendMessage(hwndScrn, WM_SETREDRAW, (WPARAM )FALSE, 0);

                SendMessage(hwndScrn, EM_SETSEL, 32767, 32767 );
#ifdef UNICODE
                if (MultiByteToWideChar(CP_ACP,
                                        0,
                                        szBuf,
                                        -1,
                                        szUnicodeBuf,
                                        sizeof(szUnicodeBuf)))
                {
                    SendMessage(hwndScrn, EM_REPLACESEL, 0, (LPARAM )szUnicodeBuf );
                }
#else // UNICODE
                SendMessage(hwndScrn, EM_REPLACESEL, 0, (LPARAM )szBuf );
#endif // UNICODE
                SendMessage(hwndScrn, WM_EOLFROMDEVICE, 0, 0 );

                SendMessage(hwndScrn, WM_SETREDRAW, (WPARAM )TRUE, 0);
                SendMessage(hwndScrn, EM_SCROLLCARET, 0, 0);
                InvalidateRect(hwndScrn, NULL, FALSE);

                /* Start afresh on the output buffer.
                */
                pch = szBuf;
                continue;
            }
            else if (ch == '\n' || ch == VK_TAB)
                continue;

            *pch++ = ch;
        }

        *pch = '\0';

        if (pch != szBuf)
        {
            /* Send the last remnant of the line.
            */
            SendMessage(hwndScrn, EM_SETSEL, 32767, 32767);
#ifdef UNICODE
            if (MultiByteToWideChar(CP_ACP,
                                    0,
                                    szBuf,
                                    -1,
                                    szUnicodeBuf,
                                    sizeof(szUnicodeBuf)))
            {
                SendMessage(hwndScrn, EM_REPLACESEL, 0, (LPARAM)szUnicodeBuf );
            }
#else // UNICODE
            SendMessage(hwndScrn, EM_REPLACESEL, 0, (LPARAM)szBuf );
#endif // UNICODE
            SendMessage(hwndScrn, EM_SCROLLCARET, 0, 0);
        }
    }
  }while (cbRead != 0);

  CloseHandle(hEvent);

  return TRUE;
}

/*----------------------------------------------------------------------------
** Terminal Output Handler
**----------------------------------------------------------------------------
*/

VOID NEAR PASCAL SendCharacter( HWND hwnd, BYTE byte )

    /* Send character 'byte' to the device.
    */
{
  PTERMDLG  pTerminaldialog;
  DWORD     cbWrite;
  OVERLAPPED ov;
  HANDLE    hEvent;
  COMMTIMEOUTS commtimeout;

  pTerminaldialog = (PTERMDLG)GetWindowLong(hwnd, DWL_USER);

  /* Make sure we still have the comm port
  */
  if (pTerminaldialog->hport == NULL)
    return;

  /* Send the character to the device.  It is not passed thru
  ** because the device will echo it.
  */
  pTerminaldialog->pbyteSendBuf[ 0 ] = (BYTE )byte;

  // Set write timeout to one second
  //
  commtimeout.ReadIntervalTimeout = MAXDWORD;
  commtimeout.ReadTotalTimeoutMultiplier = 0;
  commtimeout.ReadTotalTimeoutConstant   = 0;
  commtimeout.WriteTotalTimeoutMultiplier= 0;
  commtimeout.WriteTotalTimeoutConstant  = 1000;
  SetCommTimeouts(pTerminaldialog->hport, &commtimeout);

  if ((hEvent = CreateEvent(NULL, TRUE, FALSE, NULL)) != NULL)
  {
    ov.Internal     = 0;
    ov.InternalHigh = 0;
    ov.Offset       = 0;
    ov.OffsetHigh   = 0;
    ov.hEvent       = (HANDLE)((DWORD)hEvent | 1);

    cbWrite = 0;

    if (FALSE == WriteFile(pTerminaldialog->hport,
                           pTerminaldialog->pbyteSendBuf,
                           SIZE_SendBuf, &cbWrite, &ov))
    {
      DWORD dwResult = GetLastError();
      DWORD dwNumBytesWritten;

      if (ERROR_IO_PENDING == dwResult)
      {
        GetOverlappedResult(pTerminaldialog->hport,
                            &ov,
                            &dwNumBytesWritten,
                            TRUE);
        if (dwNumBytesWritten != SIZE_SendBuf)
        {
          DPRINTF1("WriteFile() in SendCharacter() only wrote %d bytes!",
                   dwNumBytesWritten);
        }
      }
      else
      {
        DPRINTF1("WriteFile() in SendCharacter() failed (0x%8x)!", dwResult);
      }
    }

    CloseHandle(hEvent);
  }

  return;
}

/*----------------------------------------------------------------------------
** Terminal Apperance Adjuster
**----------------------------------------------------------------------------
*/

VOID NEAR PASCAL AdjustTerminal (HWND hwnd, int wWidth, int wHeight)
{
  HWND  hwndCtrl;
  RECT  rect;
  SIZE  sizeButton;
  POINT ptPos;
  DWORD dwUnit;

  // Get the sizes of the push buttons
  //
  dwUnit = GetDialogBaseUnits();
  hwndCtrl = GetDlgItem(hwnd, IDOK);
  GetWindowRect(hwndCtrl, &rect);
  sizeButton.cx = rect.right  - rect.left;
  sizeButton.cy = rect.bottom - rect.top;
  ptPos.x   = wWidth/2 - ((X_SPACING*LOWORD(dwUnit))/4)/2 - sizeButton.cx;
  ptPos.y   = wHeight - (sizeButton.cy+((Y_MARGIN*HIWORD(dwUnit))/4));

  // Move the push buttons
  MoveWindow(hwndCtrl, ptPos.x, ptPos.y, sizeButton.cx, sizeButton.cy, TRUE);

  ptPos.x  += ((X_SPACING*LOWORD(dwUnit))/4) + sizeButton.cx;
  MoveWindow(GetDlgItem(hwnd, IDCANCEL), ptPos.x, ptPos.y,
             sizeButton.cx, sizeButton.cy, TRUE);

  // Get the current position of the terminal screen
  hwndCtrl = GetDlgItem(hwnd, CID_T_EB_SCREEN);
  GetWindowRect(hwndCtrl, &rect);
  ScreenToClient(hwnd, (LPPOINT)&rect);
  MoveWindow(hwndCtrl, rect.left, rect.top,
             wWidth - 2*rect.left,
             ptPos.y - rect.top - ((Y_MARGIN*HIWORD(dwUnit))/4),
             TRUE);

  InvalidateRect(hwnd, NULL, TRUE);
  return;
}

/*----------------------------------------------------------------------------
** Terminal read-notification thread
**----------------------------------------------------------------------------
*/

void WINAPI TerminalThread (PTERMDLG  pTerminaldialog)
{
  DWORD     dwEvent;
  DWORD     dwMask;

  while((dwEvent = WaitForMultipleObjects(MAX_EVENT, pTerminaldialog->hEvent,
                                          FALSE, INFINITE))
         < WAIT_OBJECT_0+MAX_EVENT)
  {
    switch (dwEvent)
    {
      case READ_EVENT:

        // If we are stopped already, just get out of here
        //
        if (WaitForSingleObject(pTerminaldialog->hEvent[STOP_EVENT], 0)
            == WAIT_TIMEOUT)
        {
          dwMask = 0;
          WaitCommEvent(pTerminaldialog->hport, &dwMask, NULL);

          if ((dwMask & EV_RXCHAR) && (pTerminaldialog->hwnd != NULL))
          {
            PostMessage(pTerminaldialog->hwnd, WM_MODEMNOTIFY, 0, 0);
          };
          break;
        };

      case STOP_EVENT:
        ExitThread(ERROR_SUCCESS);
        break;
    };
  };
}
