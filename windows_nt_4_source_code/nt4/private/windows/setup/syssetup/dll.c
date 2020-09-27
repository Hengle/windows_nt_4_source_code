#include "setupp.h"
#pragma hdrstop

HANDLE MyModuleHandle;

BOOL
CommonProcessAttach(
    IN BOOL Attach
    );

//
// Called by CRT when _DllMainCRTStartup is the DLL entry point
//
BOOL
WINAPI
DllMain(
    IN HANDLE DllHandle,
    IN DWORD  Reason,
    IN LPVOID Reserved
    )
{
    BOOL b;

    UNREFERENCED_PARAMETER(Reserved);

    b = TRUE;

    switch(Reason) {

    case DLL_PROCESS_ATTACH:

        InitCommonControls();
        MyModuleHandle = DllHandle;
        b = CommonProcessAttach(TRUE);
        //
        // Fall through to process first thread
        //

    case DLL_THREAD_ATTACH:

        break;

    case DLL_PROCESS_DETACH:

        CommonProcessAttach(FALSE);
        break;

    case DLL_THREAD_DETACH:

        break;
    }

    return(b);
}



BOOL
CommonProcessAttach(
    IN BOOL Attach
    )
{
    BOOL b;

    //
    // Assume success for detach, failure for attach
    //
    b = !Attach;

    if(Attach) {
        b =  InitializeBmpClass()
          && RegisterActionItemListControl(TRUE)
          && (PlatformSpecificInit() == NO_ERROR);
    } else {
        DestroyBmpClass();
        RegisterActionItemListControl(FALSE);
    }

    return(b);
}
