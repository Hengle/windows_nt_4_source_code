/*	File: D:\WACKER\cncttapi\dialdlg.c (Created: 23-Mar-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1.14 $
 *	$Date: 1996/03/01 11:16:17 $
 */

#include <tapi.h>
#pragma hdrstop

#include <time.h>

#include <tdll\stdtyp.h>
#include <tdll\session.h>
#include <tdll\tdll.h>
#include <tdll\misc.h>
#include <tdll\assert.h>
#include <tdll\cnct.h>
#include <tdll\globals.h>
#include <term\res.h>

#include "cncttapi.hh"
#include "cncttapi.h"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	DialingDlg
 *
 * DESCRIPTION:
 *	Dialing dialog.
 *
 * ARGUMENTS:
 *	Standard dialog arguments
 *
 * RETURNS:
 *	BOOL
 *
 */
BOOL CALLBACK DialingDlg(HWND hwnd, UINT uMsg, WPARAM wPar, LPARAM lPar)
	{
	#define TB_SESSION	  103
	#define TB_TELEPHONE  104
	#define TB_MODEM	  105
	#define TB_DIALICON   101
	#define TB_STATUS	  110

	HHDRIVER hhDriver;
	TCHAR ach[256];
    TCHAR achFmt[100];

	switch (uMsg)
		{
	case WM_INITDIALOG:
		SetWindowLong(hwnd, DWL_USER, lPar);
		hhDriver = (HHDRIVER)lPar;
		mscCenterWindowOnWindow(hwnd, sessQueryHwnd(hhDriver->hSession));

		SendDlgItemMessage(hwnd, TB_DIALICON, STM_SETICON,
				(WPARAM)sessQueryIcon(hhDriver->hSession), 0);

		sessQueryName(hhDriver->hSession, ach, sizeof(ach));
		SetDlgItemText(hwnd, TB_SESSION, ach);

		SetDlgItemText(hwnd, TB_TELEPHONE, hhDriver->achDisplayableDest);
		SetDlgItemText(hwnd, TB_MODEM, hhDriver->achLineName);
		break;

	case WM_SHOWWINDOW:
		if (wPar == TRUE)
			EnableDialNow(hwnd, FALSE);
		break;

	case WM_USER+0x100: // Got connection.  Close the dialog.
		EndModelessDialog(hwnd);
		break;

	case WM_USER+0x101: // Dialing message.  Display requested string in status.
		hhDriver = (HHDRIVER)GetWindowLong(hwnd, DWL_USER);

		if (LoadString(glblQueryDllHinst(), wPar, ach,
				sizeof(ach) / sizeof(TCHAR)) == 0)
			{
			assert(FALSE);
            break;
			}

        DbgOutStr("%s\r\n", ach, 0, 0, 0, 0);
		SetDlgItemText(hhDriver->hwndCnctDlg, TB_STATUS, ach);

		break;

	case WM_COMMAND:
		switch (wPar)
			{
		case IDOK: // Dial Now
			hhDriver = (HHDRIVER)GetWindowLong(hwnd, DWL_USER);
            hhDriver->iRedialCnt = 0;
            KillTimer(hwnd, 1);

			cnctdrvDisconnect(hhDriver,
				CNCT_DIALNOW | CNCT_NOCONFIRM | DISCNCT_NOBEEP);

			break;

		case IDCANCEL:
			hhDriver = (HHDRIVER)GetWindowLong(hwnd, DWL_USER);
			cnctdrvDisconnect(hhDriver, DISCNCT_NOBEEP);
			EndModelessDialog(hwnd);
			hhDriver->hwndCnctDlg = 0;	// important, so we create another
            KillTimer(hwnd, 1);
			break;

		default:
			return FALSE;
			}
		break;

    case WM_TIMER:
		hhDriver = (HHDRIVER)GetWindowLong(hwnd, DWL_USER);

        if (--hhDriver->iRedialSecsRemaining > 0)
            {
    		LoadString(glblQueryDllHinst(), IDS_DIAL_REDIAL_IN, achFmt,
	    	    sizeof(achFmt) / sizeof(TCHAR));

            wsprintf(ach, achFmt, hhDriver->iRedialSecsRemaining);
          	SetDlgItemText(hhDriver->hwndCnctDlg, TB_STATUS, ach);
            }

        else
            {
		    PostMessage(sessQueryHwnd(hhDriver->hSession), WM_CNCT_DIALNOW,
			    hhDriver->uDiscnctFlags, 0);
            }

        break;

	default:
		return FALSE;
		}

	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	DialingMessage
 *
 * DESCRIPTION:
 *	Gets the given string ID from term's resource file and displays it
 *	in the dialing dialog.
 *
 * ARGUMENTS:
 *	hhDriver	- private driver handle
 *	resID		- ID of resource.
 *
 * RETURNS:
 *	void
 *
 */
void DialingMessage(const HHDRIVER hhDriver, const int resID)
	{
	if (!IsWindow(hhDriver->hwndCnctDlg))
		return;

	PostMessage(hhDriver->hwndCnctDlg, WM_USER+0x101, (WPARAM)resID, 0);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	EnableDialNow
 *
 * DESCRIPTION:
 *	Enables/Disables dial now button.
 *
 * ARGUMENTS:
 *	hwndDlg - dial dialog window handle
 *	fEnable - TRUE/FALSE
 *
 * RETURNS:
 *	void
 *
 */
void EnableDialNow(const HWND hwndDlg, const int fEnable)
	{
	if (IsWindow(hwndDlg))
		{
		EnableWindow(GetDlgItem(hwndDlg, IDOK), fEnable);

		if (fEnable == FALSE)
			SetFocus(GetDlgItem(hwndDlg,IDCANCEL));
		else
			SetFocus(GetDlgItem(hwndDlg, IDOK));
		}

	return;
	}
