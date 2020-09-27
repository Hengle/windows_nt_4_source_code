/*** 
*dspcalc2.r
*
*  Copyright (C) 1992-1994, Microsoft Corporation.  All Rights Reserved.
*
*Purpose:
*  Resource script for dspcalc2.
*
*
*Implementation Notes:
*
*****************************************************************************/

#include "types.r"
#include "resource.h"

#if defined (_PPCMAC)
include "cfrg.rsc";
#endif

/* we use an MBAR resource to conveniently load all the menus */

resource 'MBAR' (rMenuBar, preload) {
    { mApple, mFile, mEdit }
};

resource 'MENU' (mApple, preload) {
    mApple,
    textMenuProc,
    0b11111111111111111111111111111101,
    enabled,
    apple,
    {
	"About DspCalc2\311",
	    noicon, nokey, nomark, plain;
	"-",
	    noicon, nokey, nomark, plain
    }
};

resource 'MENU' (mFile, preload) {
    mFile,
    textMenuProc,
    0b00000000000000000000100000000000,
    enabled,
    "File",
    {
	"New",
	    noicon, "N", nomark, plain;
	"Open",
	    noicon, "O", nomark, plain;
	"-",
	    noicon, nokey, nomark, plain;
	"Close",
	    noicon, "W", nomark, plain;
	"Save",
	    noicon, "S", nomark, plain;
	"Save As\311",
	    noicon, nokey, nomark, plain;
	"Revert",
	    noicon, nokey, nomark, plain;
	"-",
	    noicon, nokey, nomark, plain;
	"Page Setup\311",
	    noicon, nokey, nomark, plain;
	"Print\311",
	    noicon, nokey, nomark, plain;
	"-",
	    noicon, nokey, nomark, plain;
	"Quit",
	    noicon, "Q", nomark, plain
    }
};

resource 'MENU' (mEdit, preload) {
    mEdit,
    textMenuProc,
    0b00000000000000000000000000000000,
    enabled,
    "Edit",
    {
	"Undo",
	    noicon, "Z", nomark, plain;
	"-",
	    noicon, nokey, nomark, plain;
	"Cut",
	    noicon, "X", nomark, plain;
	"Copy",
	    noicon, "C", nomark, plain;
	"Paste",
	    noicon, "V", nomark, plain;
	"Clear",
	    noicon, nokey, nomark, plain
    }
};

resource 'DLOG' (rCalc) {
    {100, 100, 240, 220},
    rDocProc,
    visible,
    noGoAway,
    0x0,
    rCalc,
    "DspCalc2"
};

resource 'DITL' (rCalc) {
    {
        {110,10,130,30},  Button   { enabled, "0" } /* 1 */
      , {85,10,105,30},   Button   { enabled, "1" } /* 2 */
      , {85,35,105,55},   Button   { enabled, "2" } /* 3 */
      , {85,60,105,80},   Button   { enabled, "3" } /* 4 */
      , {60,10,80,30},    Button   { enabled, "4" } /* 5 */
      , {60,35,80,55},    Button   { enabled, "5" } /* 6 */
      , {60,60,80,80},    Button   { enabled, "6" } /* 7 */
      , {35,10,55,30},    Button   { enabled, "7" } /* 8 */
      , {35,35,55,55},    Button   { enabled, "8" } /* 9 */
      , {35,60,55,80},    Button   { enabled, "9" } /* 10 */

      , {35,85,55,105},   Button   { enabled, "+" } /* 11 */
      , {60,85,80,105},   Button   { enabled, "-" } /* 12 */
      , {85,85,105,105},  Button   { enabled, "*" } /* 13 */
      , {110,85,130,105}, Button   { enabled, "/" } /* 14 */

      , {110,35,130,55},  Button   { enabled, "C" } /* 15 */
      , {110,60,130,80},  Button   { enabled, "=" } /* 16 */

      , {10,10,25,105},   EditText { disabled, ""  } /* 17 */
    }
};

/* this ALRT and DITL are used as an About screen */

resource 'ALRT' (rAboutAlert, purgeable) {
    {40, 20, 160, 290},
    rAboutAlert,
    {
	/* [1] */
	OK, visible, silent,
	/* [2] */
	OK, visible, silent,
	/* [3] */
	OK, visible, silent,
	/* [4] */
	OK, visible, silent
    }
};

resource 'DITL' (rAboutAlert, purgeable) {
    { /* array DITLarray: 5 elements */
	/* [1] */
	{88, 180, 108, 260},
	Button {
	    enabled,
	    "OK"
	},
	/* [2] */
	{8, 8, 24, 214},
	StaticText {
	    disabled,
	    "The IDispatch Calculator"
	}
    }
};


/* this ALRT and DITL are used as an error screen */

resource 'ALRT' (rUserAlert, purgeable) {
	{40, 20, 120, 260},
	rUserAlert,
	{ /* array: 4 elements */
	    /* [1] */
	    OK, visible, silent,
	    /* [2] */
	    OK, visible, silent,
	    /* [3] */
	    OK, visible, silent,
	    /* [4] */
	    OK, visible, silent
	}
};


resource 'DITL' (rUserAlert, purgeable) {
    {
	/* [1] */
	{50, 150, 70, 230},
	Button {
	    enabled,
	    "OK"
	},
	/* [2] */
	{10, 60, 30, 230},
	StaticText {
	    disabled,
	    "Error. ^0"
	},
	/* [3] */
	{8, 8, 40, 40},
	Icon {
	    disabled,
	    2
	}
    }
};


resource 'SIZE' (-1) {
    dontSaveScreen,
    acceptSuspendResumeEvents,
    enableOptionSwitch,
    canBackground,	
    multiFinderAware,
    backgroundAndForeground,
    dontGetFrontClicks,
    ignoreChildDiedEvents,
    is32BitCompatible,
    isHighLevelEventAware,
    localAndRemoteHLEvents,
    reserved,
    reserved,
    reserved,
    reserved,
    reserved,
    kPrefSize * 1024,
    kMinSize * 1024	
};

resource 'BNDL' (130) {
	'DCL2',
	0,
	{	/* array TypeArray: 2 elements */
		/* [1] */
		'FREF',
		{	/* array IDArray: 1 elements */
			/* [1] */
			0, 130
		},
		/* [2] */
		'ICN#',
		{	/* array IDArray: 1 elements */
			/* [1] */
			0, 130
		}
	}
};

resource 'FREF' (130) {
	'APPL',
	0,
	""
};

data 'ICN#' (130) {
	$"1E3F FFFF 3340 0001 63C0 0001 03CF FFF9"            /* .?��3@..c�...��� */
	$"0748 0009 0E48 0009 3C48 0009 704F FFF9"            /* .H.�.H.�<H.�pO�� */
	$"7FC0 0001 0040 0001 004F FFF9 0048 8889"            /* .�...@...O��.H�� */
	$"0048 8889 0048 8889 004F FFF9 0040 0001"            /* .H��.H��.O��.@.. */
	$"004F FFF9 0048 8889 0048 8889 004C 8889"            /* .O��.H��.H��.L�� */
	$"005F FFF9 0078 0001 01FF FFF9 00F8 8889"            /* ._��.x...���.��� */
	$"0068 8889 0168 8889 064F FFF9 1E40 0001"            /* .h��.h��.O��.@.. */
	$"7C40 0001 3C7F FFFF 183F FFFF 0800 0000"            /* |@..<.��.?��.... */
	$"1E3F FFFF 337F FFFF 63FF FFFF 03FF FFFF"            /* .?��3.��c���.��� */
	$"077F FFFF 0E7F FFFF 3C7F FFFF 707F FFFF"            /* ..��..��<.��p.�� */
	$"7FFF FFFF 007F FFFF 007F FFFF 007F FFFF"            /* .���..��..��..�� */
	$"007F FFFF 007F FFFF 007F FFFF 007F FFFF"            /* ..��..��..��..�� */
	$"007F FFFF 007F FFFF 007F FFFF 007F FFFF"            /* ..��..��..��..�� */
	$"007F FFFF 007F FFFF 01FF FFFF 00FF FFFF"            /* ..��..��.���.��� */
	$"007F FFFF 017F FFFF 067F FFFF 1E7F FFFF"            /* ..��..��..��..�� */
	$"7C7F FFFF 3C7F FFFF 183F FFFF 0800 0000"            /* |.��<.��.?��.... */
};

data 'icl4' (130) {
	$"000F FFF0 00FF FFFF FFFF FFFF FFFF FFFF"            /* ..��.����������� */
	$"00FF 00FF 0F00 0000 0000 0000 0000 000F"            /* .�.�............ */
	$"0FF0 00FF FF0C CCCC CCCC CCCC CCCC CCDF"            /* .�.��.���������� */
	$"0000 00FF FF0C DDDD DDDD DDDD DDDD DCDF"            /* ...��.���������� */
	$"0000 0FFF 0F0C DCCC CCCC CCCC CCCC 0CDF"            /* ...�..��������.� */
	$"0000 FFF0 0F0C DCCC CCCC CCCC CCCC 0CDF"            /* ..��..��������.� */
	$"00FF FF00 0F0C DCCC CCCC CCCC CCCC 0CDF"            /* .��...��������.� */
	$"0FFF 0000 0F0C 0000 0000 0000 0000 0CDF"            /* .�.............� */
	$"0FFF FFFF FF0C CCCC CCCC CCCC CCCC CCDF"            /* .����.���������� */
	$"0000 0000 0F0C CCCC CCCC CCCC CCCC CCDF"            /* ......���������� */
	$"0000 0000 0F0C DDDD DDDD DDDD DDDD DCDF"            /* ......���������� */
	$"0000 0000 0F0C F00C F00C F00C F00C FCDF"            /* ......�.�.�.�.�� */
	$"0000 0000 0F0C F0CD F0CD F0CD F0CD FCDF"            /* ......���������� */
	$"0000 0000 0F0C FCDD FCDD FCDD FCDD FCDF"            /* ......���������� */
	$"0000 0000 0F0C FFFF FFFF FFFF FFFF FCDF"            /* ......���������� */
	$"0000 0000 0F0C CCCC CCCC CCCC CCCC CCDF"            /* ......���������� */
	$"0000 0000 0F0C DDDD DDDD DDDD DDDD DCDF"            /* ......���������� */
	$"0000 0000 0F0C F00C F00C F00C F00C FCDF"            /* ......�.�.�.�.�� */
	$"0000 0000 0F0C F0CD F0CD F0CD F0CD FCDF"            /* ......���������� */
	$"0000 0000 0F0C F3DD FCDD FCDD FCDD FCDF"            /* ......���������� */
	$"0000 0000 0F03 3FFF FFFF FFFF FFFF FCDF"            /* ......?��������� */
	$"0000 0000 033F 3CCC CCCC CCCC CCCC CCDF"            /* .....?<��������� */
	$"0000 0003 33F3 DDDD DDDD DDDD DDDD DCDF"            /* ....3����������� */
	$"0000 0000 3F33 F00C F00C F00C F00C FCDF"            /* ....?3�.�.�.�.�� */
	$"0000 0000 033C F0CD F0CD F0CD F0CD FCDF"            /* .....<���������� */
	$"0000 0003 0F3C FCDD FCDD FCDD FCDD FCDF"            /* .....<���������� */
	$"0000 0330 0F0C FFFF FFFF FFFF FFFF FCDF"            /* ...0..���������� */
	$"0003 3F30 0F0C CCCC CCCC CCCC CCCC CCDF"            /* ..?0..���������� */
	$"0333 F300 0F0D DDDD DDDD DDDD DDDD DDDF"            /* .3�..����������� */
	$"003F 3300 0FFF FFFF FFFF FFFF FFFF FFFF"            /* .?3..����������� */
	$"0003 3000 00FF FFFF FFFF FFFF FFFF FFFF"            /* ..0..����������� */
	$"0000 3000 0000 0000 0000 0000 0000 0000"            /* ..0............. */
};

data 'DCL2' (0, "Owner resource") {
	$"00"                                                 /* . */
};

