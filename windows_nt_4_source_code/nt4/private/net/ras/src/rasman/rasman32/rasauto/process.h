/*++

Copyright(c) 1995 Microsoft Corporation

MODULE NAME
    process.h

ABSTRACT
    Header file for NT process routines.

AUTHOR
    Anthony Discolo (adiscolo) 12-Aug-1995

REVISION HISTORY

--*/

PSYSTEM_PROCESS_INFORMATION
GetSystemProcessInfo();

PSYSTEM_PROCESS_INFORMATION
FindProcessByName(
    IN PSYSTEM_PROCESS_INFORMATION pProcessInfo,
    IN LPTSTR lpExeName
    );

VOID
FreeSystemProcessInfo(
    IN PSYSTEM_PROCESS_INFORMATION pProcessInfo
    );
