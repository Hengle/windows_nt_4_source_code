/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    porti386.c

Abstract:

    This is the x86 specific part of the video port driver.

Author:

    Andre Vachon (andreva) 10-Jan-1991

Environment:

    kernel mode only

Notes:

    This module is a driver which implements OS dependant functions on the
    behalf of the video drivers

Revision History:

--*/

#include "dderror.h"
#include "ntos.h"
#include "vdm.h"
#include "ntddvdeo.h"
#include "video.h"

#include "zwapi.h"

#include "videoprt.h"
#include "..\..\..\nthals\x86new\xm86.h"
#include "..\..\..\nthals\x86new\x86new.h"

VP_STATUS
SymmetryDeviceDataCallback(
    PVOID HwDeviceExtension,
    PVOID Context,
    VIDEO_DEVICE_DATA_TYPE DeviceDataType,
    PVOID Identifier,
    ULONG IdentifierLength,
    PVOID ConfigurationData,
    ULONG ConfigurationDataLength,
    PVOID ComponentInformation,
    ULONG ComponentInformationLength
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,pVideoPortEnableVDM)
#pragma alloc_text(PAGE,pVideoPortInitializeInt10)
#pragma alloc_text(PAGE,VideoPortInt10)
#pragma alloc_text(PAGE,SymmetryDeviceDataCallback)
#pragma alloc_text(PAGE,pVideoPortRegisterVDM)
#pragma alloc_text(PAGE,pVideoPortSetIOPM)
#pragma alloc_text(PAGE,VideoPortSetTrappedEmulatorPorts)
#endif

//
// Control Whether or not the bottom MEG of the CSR address space has
// already been committed.
//

ULONG ServerBiosAddressSpaceInitialized = 0;
PVOID HalpEisaControlBase = NULL;
PVOID HalpEisaMemoryBase = NULL;


VP_STATUS
SymmetryDeviceDataCallback(
    PVOID HwDeviceExtension,
    PVOID Context,
    VIDEO_DEVICE_DATA_TYPE DeviceDataType,
    PVOID Identifier,
    ULONG IdentifierLength,
    PVOID ConfigurationData,
    ULONG ConfigurationDataLength,
    PVOID ComponentInformation,
    ULONG ComponentInformationLength
    )
{

    if (RtlCompareMemory(L"SEQUENT Symmetry",
                         Identifier,
                         sizeof(L"SEQUENT Symmetry")) ==
                         sizeof(L"SEQUENT Symmetry"))
    {
        return NO_ERROR;
    }

    return ERROR_INVALID_PARAMETER;
}


NTSTATUS
pVideoPortEnableVDM(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN BOOLEAN Enable,
    IN PVIDEO_VDM VdmInfo,
    IN ULONG VdmInfoSize
    )

/*++

Routine Description:

    This routine allows the kernel video driver to hook out I/O ports or
    specific interrupts from the V86 fault handler. Operations on the
    specified ports which are intercepted by the V86 fault handler will be
    forwarded to the kernel driver directly.

Arguments:

    DeviceExtension - Pointer to the port driver's device extension.

    Enable - Determines if the VDM should be enabled (TRUE) or disabled
        (FALSE).

    VdmInfo - Pointer to the VdmInfo passed by the caller.

    VdmInfoSize - Size of the VdmInfo struct passed by the caller.

Return Value:

    Return the value returned by ZwSetInformationProcess().

--*/

{

    PROCESS_IO_PORT_HANDLER_INFORMATION processHandlerInfo;
    NTSTATUS ntStatus;
    PEPROCESS process;
    PVOID virtualAddress;
    ULONG length;
    ULONG defaultMask = 0;
    ULONG inIoSpace = VIDEO_MEMORY_SPACE_MEMORY |
                      VIDEO_MEMORY_SPACE_USER_MODE;

    //
    // Must make sure the caller is a trusted subsystem with the
    // appropriate privilege level before executing this call.
    // If the calls returns FALSE we must return an error code.
    //

    if (!SeSinglePrivilegeCheck(RtlConvertLongToLuid(
                                    SE_TCB_PRIVILEGE),
                                DeviceExtension->CurrentIrpRequestorMode)) {

        return STATUS_PRIVILEGE_NOT_HELD;

    }

    //
    // Test to see if the parameter size is valid
    //

    if (VdmInfoSize < sizeof(VIDEO_VDM) ) {

        return STATUS_BUFFER_TOO_SMALL;

    }

    //
    // Set the enable flag in the process struct and put in the length and
    // pointer to the emulator info struct.
    //

    if (Enable) {

        processHandlerInfo.Install = TRUE;

    } else {

        processHandlerInfo.Install = FALSE;

    }

    processHandlerInfo.NumEntries =
        DeviceExtension->NumEmulatorAccessEntries;
    processHandlerInfo.EmulatorAccessEntries =
        DeviceExtension->EmulatorAccessEntries;
    processHandlerInfo.Context = DeviceExtension->EmulatorAccessEntriesContext;


    //
    // Call SetInformationProcess
    // BUGBUG Do we need to do handle checking?
    //

    ntStatus = ZwSetInformationProcess(VdmInfo->ProcessHandle,
                                       ProcessIoPortHandlers,
                                       &processHandlerInfo,
                                       sizeof(PROCESS_IO_PORT_HANDLER_INFORMATION));

    if (!NT_SUCCESS(ntStatus)) {

        return ntStatus;

    }

    //
    // If we are disabling the DOS application, give it the original IOPM
    // it had (which is mask zero.
    // If we are enabling it, then wait for the miniport to call to set it up
    // appropriately.
    //

    ntStatus = ObReferenceObjectByHandle(VdmInfo->ProcessHandle,
                                         0,
                                         *(PVOID *)PsProcessType,
                                         DeviceExtension->CurrentIrpRequestorMode,
                                         (PVOID *)&process,
                                         NULL);

    if (NT_SUCCESS(ntStatus)) {

        if (Enable) {

            defaultMask = 1;

        } // otherwise we are disabling and the mask number is 0;

        if (!Ke386IoSetAccessProcess(&process->Pcb,
                                     defaultMask)) {

            ntStatus = STATUS_IO_PRIVILEGE_FAILED;

        }

        ObDereferenceObject(process);
    }


    if (!NT_SUCCESS(ntStatus)) {

        return ntStatus;

    }

    //
    // We can now map (or unmap) the video frame buffer into the VDM's
    // address space.
    //

    virtualAddress = (PVOID) DeviceExtension->VdmPhysicalVideoMemoryAddress.LowPart;

    //
    // Override this with A0000 for the Sequent Symmetry machine.
    //

    if (VideoPortGetDeviceData(DeviceExtension->HwDeviceExtension,
                               VpMachineData,
                               &SymmetryDeviceDataCallback,
                               NULL) == NO_ERROR)
    {
        virtualAddress = (PVOID) 0xA0000;
    }

    length = DeviceExtension->VdmPhysicalVideoMemoryLength;

    if (Enable) {

        return pVideoPortMapUserPhysicalMem(DeviceExtension,
                                            VdmInfo->ProcessHandle,
                                            DeviceExtension->VdmPhysicalVideoMemoryAddress,
                                            &length,
                                            &inIoSpace,
                                            (PVOID *) &virtualAddress);

    } else {

        //
        // BUGBUG virtual address is not what we got mapped ...
        //

        return ZwUnmapViewOfSection(VdmInfo->ProcessHandle,
                    (PVOID)( ((ULONG)virtualAddress) & (~(PAGE_SIZE - 1))) );

    }

} // pVideoPortEnableVDM()


VP_STATUS
VideoPortInt10(
    PVOID HwDeviceExtension,
    PVIDEO_X86_BIOS_ARGUMENTS BiosArguments
    )

/*++

Routine Description:

    This function allows a miniport driver to call the kernel to perform
    an int10 operation.
    This will execute natively the BIOS ROM code on the device.

    THIS FUNCTION IS FOR X86 ONLY.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

    BiosArguments - Pointer to a structure containing the value of the
        basic x86 registers that should be set before calling the BIOS routine.
        0 should be used for unused registers.

Return Value:


Restrictions:

    Device uses IO ports ONLY.


--*/

{
    NTSTATUS ntStatus;
    CONTEXT context;
    ULONG dwCrc = 0xFFFFFFFF;

    PDEVICE_EXTENSION deviceExtension =
        deviceExtension = ((PDEVICE_EXTENSION) HwDeviceExtension) - 1;

    //
    // Must make sure the caller is a trusted subsystem with the
    // appropriate address space set up.
    //

    if (!SeSinglePrivilegeCheck(RtlConvertLongToLuid(
                                    SE_TCB_PRIVILEGE),
                                deviceExtension->CurrentIrpRequestorMode)) {

        return ERROR_INVALID_PARAMETER;
    }

    if (ServerBiosAddressSpaceInitialized == 0) {

        ASSERT(FALSE);

        return ERROR_INVALID_PARAMETER;
    }

    KeAttachProcess(&CsrProcess->Pcb);

    // Crc324 - Calculate the Crc32 value of a string in 4 bit nibbles.
    //
    // dwCrc  - initial value of CRC
    // cbBuffer - count in bytes of length of buffer
    // pbBuffer - pointer to buffer to checksum
    //
    // returns: value of crc32
    //
    // this table is derived from the 256 element CRCTable in
    // \\orville\razzle\src\net\svcdlls\ntlmssp\client\crc32.c
    // This table contains every 16th element since we do 4 bits
    // at a time, not 8
    //

    {
        ULONG CRCTable4[16] = {
            0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
            0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
            0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
            0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c,
            };

        UCHAR byte;
        UCHAR index;
        ULONG cBuffer = 0x1000;
        PUCHAR pBuffer = (PUCHAR) 0x000C0000;

        while (cBuffer-- != 0)
        {
            byte = *pBuffer++;

            index = (byte ^ (UCHAR)(dwCrc) ) & 0xf;
            dwCrc = (dwCrc >> 4) ^ CRCTable4[index];

            byte  = byte >> 4;
            index = (byte ^ (UCHAR)(dwCrc) ) & 0xf;
            dwCrc = (dwCrc >> 4) ^ CRCTable4[index];
        }

        dwCrc ^= 0xFFFFFFFF;
    }

    //
    // Zero out the context and initialize the required values with the
    // miniport's requested register values.
    //

    RtlZeroMemory(&context, sizeof(CONTEXT));

    context.Edi = BiosArguments->Edi;
    context.Esi = BiosArguments->Esi;
    context.Eax = BiosArguments->Eax;
    context.Ebx = BiosArguments->Ebx;
    context.Ecx = BiosArguments->Ecx;
    context.Edx = BiosArguments->Edx;
    context.Ebp = BiosArguments->Ebp;

    //
    // Now call the kernel to actually perform the int 10 operation.
    //

    ntStatus = Ke386CallBios(0x10, &context);

    KeDetachProcess();

    //
    // fill in struct with any return values from the context
    //

    BiosArguments->Edi = context.Edi;
    BiosArguments->Esi = context.Esi;
    BiosArguments->Eax = context.Eax;
    BiosArguments->Ebx = context.Ebx;
    BiosArguments->Ecx = context.Ecx;
    BiosArguments->Edx = context.Edx;
    BiosArguments->Ebp = context.Ebp;

    //
    // Delay the writting of the Crc until now since it can not be done from
    // within an attach.
    //

    VideoPortSetRegistryParameters(HwDeviceExtension,
                                   L"HardwareInformation.Crc32",
                                   &dwCrc,
                                   sizeof(ULONG));

    //
    // Return that status we got when calling the BIOS (writting to the
    // is secondary at best).
    //

    if (NT_SUCCESS(ntStatus)) {

        pVideoDebugPrint ((2, "VIDEOPRT: Int10: Int 10 succeded properly\n"));
        return NO_ERROR;

    } else {

        pVideoDebugPrint ((0, "VIDEOPRT: Int10 failed - status %08lx\n", ntStatus));
        return ERROR_INVALID_PARAMETER;

    }

#if 0

    XM86_CONTEXT context;
    BOOLEAN bRet;

    if (!ServerBiosAddressSpaceInitialized) {

        PVOID baseAddress;
        PHYSICAL_ADDRESS pa = {0x00000000, 0x00000000};

        HalpEisaMemoryBase = VideoPortGetDeviceBase(HwDeviceExtension,
                                                    pa,
                                                    0xFFFFF,
                                                    FALSE);

        x86BiosInitializeBios(HalpEisaControlBase, HalpEisaMemoryBase);

        //
        // Attempt to initialize the display adapter by executing its ROM bios
        // code. The standard ROM bios code address for PC video adapters is
        // 0xC000:0000 on the ISA bus.
        //

        if (x86BiosInitializeAdapter(0xc0000, NULL, NULL, NULL) != XM_SUCCESS)
        {
            pVideoDebugPrint((0, "VIDEOPRT: x86BiosInitializeAdapter failed !\n"));
            DbgBreakPoint();
        }
    }

    //
    // Zero out the context and initialize the required values with the
    // miniport's requested register values.
    //

    context.Edi = BiosArguments->Edi;
    context.Esi = BiosArguments->Esi;
    context.Eax = BiosArguments->Eax;
    context.Ebx = BiosArguments->Ebx;
    context.Ecx = BiosArguments->Ecx;
    context.Edx = BiosArguments->Edx;
    context.Ebp = BiosArguments->Ebp;

    //
    // Now call the kernel to actually perform the int 10 operation.
    //

    bRet = x86BiosExecuteInterrupt(0x10,
                                   &context,
                                   HalpEisaControlBase,
                                   HalpEisaMemoryBase);

    //
    // fill in struct with any return values from the context
    //

    BiosArguments->Edi = context.Edi;
    BiosArguments->Esi = context.Esi;
    BiosArguments->Eax = context.Eax;
    BiosArguments->Ebx = context.Ebx;
    BiosArguments->Ecx = context.Ecx;
    BiosArguments->Edx = context.Edx;
    BiosArguments->Ebp = context.Ebp;

    if (bRet == XM_SUCCESS) {

        pVideoDebugPrint ((2, "VIDEOPRT: Int10: Int 10 succeded properly\n"));
        return NO_ERROR;

    } else {

        ASSERT(FALSE);
        return ERROR_INVALID_PARAMETER;

    }

#endif

} // end VideoPortInt10()

//
// Internal definitions
//

#define KEY_VALUE_BUFFER_SIZE 1024
#define ONE_MEG             0x100000
#define ROM_BIOS_START      0xC0000
#define VIDEO_BUFFER_START  0xA0000
#define DOS_LOADED_ADDRESS  0x700
#define EBIOS_AREA_INFORMATION 0x40

typedef struct _EBIOS_INFORMATION {
    ULONG EBiosAddress;
    ULONG EBiosSize;
} EBIOS_INFORMATION, *PEBIOS_INFORMATION;



VOID
pVideoPortInitializeInt10(
    PDEVICE_EXTENSION deviceExtension
    )

/*++

Routine Description:

    Initializes the CSR address space so we can do an int 10.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

Return Value:


Restrictions:

    THIS FUNCTION IS FOR X86 ONLY.

    THIS FUNCTION MUST BE RUN IN THE CONTEXT OF CSR

    This function goes thru ROM BIOS area to map in all the ROM blocks and
    allocates memory for the holes inside the BIOS area.  The reason we
    allocate memory for the holes in BIOS area is because some int 10 BIOS
    code touches the nonexisting memory.  Under Nt, this triggers page fault
    and the int 10 is terminated.

    Note: the code is adapted from VdmpInitialize().



--*/

{
    NTSTATUS ntStatus;

    ULONG inIoSpace = VIDEO_MEMORY_SPACE_MEMORY |
                      VIDEO_MEMORY_SPACE_USER_MODE;
    PVOID virtualAddress;
    ULONG length;
    ULONG size;
    PVOID baseAddress;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING SectionName;
    UNICODE_STRING WorkString;
    ULONG ViewSize;
    LARGE_INTEGER ViewBase;
    PVOID BaseAddress;
    PVOID destination;
    HANDLE SectionHandle, RegistryHandle;
    ULONG ResultLength, EndingAddress;
    ULONG Index;
    PCM_FULL_RESOURCE_DESCRIPTOR ResourceDescriptor;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialResourceDescriptor;
    PKEY_VALUE_FULL_INFORMATION KeyValueBuffer;
    CM_ROM_BLOCK RomBlock;
    PCM_ROM_BLOCK BiosBlock;
    ULONG LastMappedAddress;

    //
    // NOTE   Due to the way compiler optimization code works, I have
    //        to declare EBiosInformation to be volatile.  Otherwise, no
    //        code will be generated for the EBiosInformation.
    //        It should be removed once the C compiler is fixed.
    //
    volatile PEBIOS_INFORMATION EBiosInformation = (PEBIOS_INFORMATION)
                           (DOS_LOADED_ADDRESS + EBIOS_AREA_INFORMATION);
    BOOLEAN EBiosInitialized = FALSE;

    //
    // If the address space has not been set up in the server yet, do it now.
    //        NOTE: no need to map in IO ports since the server has IOPL
    //

    if (CsrProcess == NULL)
    {
        CsrProcess = PsGetCurrentProcess();
    }

    //
    // If we have already initialized, or for some reason can not, return
    // right here.
    //

    if ((ServerBiosAddressSpaceInitialized == 1) ||
        (VpC0000Compatible == 0)                 ||
        (deviceExtension->VdmPhysicalVideoMemoryAddress.LowPart == 0))
    {
        return;
    }

    size = 0x00100000 - 1;        // 1 MEG

    //
    // We pass an address of 1, so Emory Management will round it down to 0.
    // if we passed in 0, memory management would think the argument was
    // not present.
    //

    baseAddress = (PVOID) 0x00000001;

    // N.B.        We expect that process creation has reserved the first 16 MB
    //        for us already. If not, then this won't work worth a darn

    ntStatus = ZwAllocateVirtualMemory( NtCurrentProcess(),
                                        &baseAddress,
                                        0L,
                                        &size,
                                        MEM_COMMIT,
                                        PAGE_READWRITE );

    if (!NT_SUCCESS(ntStatus)) {

        pVideoDebugPrint ((1, "VIDEOPRT: Int10: Failed to allocate 1MEG of memory for the VDM\n"));
        return;

    }
#if 0
    //
    // This part is done in the kernel now.
    //

    ntStatus = NtVdmControl(VdmInitialize,
                            NULL,
                            0,
                            NULL);

    if (!NT_SUCCESS(ntStatus)) {

        ZwFreeVirtualMemory(NtCurrentProcess(),
                            &baseAddress,
                            &size,
                            MEM_RELEASE);

        pVideoDebugPrint ((1, "VIDEOPRT: Int10: Failed to initialize the VDM address space\n"));
        return;

        }
#endif
    //
    // Map in the physical memory into the caller's address space so that
    // any memory references from the BIOS will work properly.
    //

    virtualAddress = (PVOID) deviceExtension->VdmPhysicalVideoMemoryAddress.LowPart;
    length = deviceExtension->VdmPhysicalVideoMemoryLength;

    ntStatus = ZwFreeVirtualMemory(NtCurrentProcess(),
                                   &virtualAddress,
                                   &length,
                                   MEM_RELEASE);

    if (!NT_SUCCESS(ntStatus)) {

        pVideoDebugPrint ((1, "VIDEOPRT: Int10: Failed to free memory space for video memory to be mapped\n"));
        return;

    }

    virtualAddress = (PVOID) deviceExtension->VdmPhysicalVideoMemoryAddress.LowPart;
    length = deviceExtension->VdmPhysicalVideoMemoryLength;

    ntStatus = pVideoPortMapUserPhysicalMem(deviceExtension,
                                            NtCurrentProcess(),
                                            deviceExtension->VdmPhysicalVideoMemoryAddress,
                                            &length,
                                            &inIoSpace,
                                            &virtualAddress);

    if (!NT_SUCCESS(ntStatus)) {

        pVideoDebugPrint ((1, "VIDEOPRT: Int10: Failed to Map video memory in address space\n"));
        return;

    }

    //
    // Initialize the default bios block which will be used if we can NOT
    // find any valid bios block.
    //

    RomBlock.Address = ROM_BIOS_START;
    RomBlock.Size = 0x40000;
    BiosBlock = &RomBlock;
    Index = 1;

    RtlInitUnicodeString(
        &SectionName,
        L"\\Device\\PhysicalMemory"
        );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &SectionName,
        OBJ_CASE_INSENSITIVE,
        (HANDLE) NULL,
        (PSECURITY_DESCRIPTOR) NULL
        );

    ntStatus = ZwOpenSection(
        &SectionHandle,
        SECTION_ALL_ACCESS,
        &ObjectAttributes
        );

    if (!NT_SUCCESS(ntStatus)) {

        return;

    }

    //
    // Copy the first page of physical memory into the CSR's address space
    //

    BaseAddress = 0;
    destination = 0;
    ViewSize = 0x1000;
    ViewBase.LowPart = 0;
    ViewBase.HighPart = 0;

    ntStatus =ZwMapViewOfSection(
        SectionHandle,
        NtCurrentProcess(),
        &BaseAddress,
        0,
        ViewSize,
        &ViewBase,
        &ViewSize,
        ViewUnmap,
        0,
        PAGE_READWRITE
        );

    if (!NT_SUCCESS(ntStatus)) {

        ZwClose(SectionHandle);
        return;

    }

    RtlMoveMemory(
        destination,
        BaseAddress,
        ViewSize
        );

    ntStatus = ZwUnmapViewOfSection(
        NtCurrentProcess(),
        BaseAddress
        );

    if (!NT_SUCCESS(ntStatus)) {

        ZwClose(SectionHandle);
        return;

    }

    //
    // Set up and open KeyPath
    //
    RtlInitUnicodeString(
        &WorkString,
        L"\\Registry\\Machine\\Hardware\\Description\\System"
        );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &WorkString,
        OBJ_CASE_INSENSITIVE,
        (HANDLE)NULL,
        NULL
        );

    ntStatus = ZwOpenKey(
        &RegistryHandle,
        KEY_READ,
        &ObjectAttributes
        );

    if (!NT_SUCCESS(ntStatus)) {
        ZwClose(SectionHandle);
        return;
    }

    //
    // Allocate space for the data
    //

    KeyValueBuffer = ExAllocatePool(PagedPool, KEY_VALUE_BUFFER_SIZE);
    if (KeyValueBuffer == NULL) {
        ZwClose(SectionHandle);
        ZwClose(RegistryHandle);
        return;
    }

    //
    // Get the data for the rom information
    //

    RtlInitUnicodeString(
        &WorkString,
        L"Configuration Data"
        );

    ntStatus = ZwQueryValueKey(
        RegistryHandle,
        &WorkString,
        KeyValueFullInformation,
        KeyValueBuffer,
        KEY_VALUE_BUFFER_SIZE,
        &ResultLength
        );

    if (!NT_SUCCESS(ntStatus)) {
        ZwClose(SectionHandle);
        ZwClose(RegistryHandle);
        ExFreePool(KeyValueBuffer);
        return;
    }

    ResourceDescriptor = (PCM_FULL_RESOURCE_DESCRIPTOR)
        ((PUCHAR) KeyValueBuffer + KeyValueBuffer->DataOffset);

    if ((KeyValueBuffer->DataLength >= sizeof(CM_FULL_RESOURCE_DESCRIPTOR)) &&
        (ResourceDescriptor->PartialResourceList.Count >= 2) ) {

        PartialResourceDescriptor = (PCM_PARTIAL_RESOURCE_DESCRIPTOR)
            ((PUCHAR)ResourceDescriptor +
            sizeof(CM_FULL_RESOURCE_DESCRIPTOR) +
            ResourceDescriptor->PartialResourceList.PartialDescriptors[0]
                .u.DeviceSpecificData.DataSize);

        if (KeyValueBuffer->DataLength >= ((PUCHAR)PartialResourceDescriptor -
            (PUCHAR)ResourceDescriptor + sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR)
            + sizeof(CM_ROM_BLOCK))) {
            BiosBlock = (PCM_ROM_BLOCK)((PUCHAR)PartialResourceDescriptor +
                sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

            Index = PartialResourceDescriptor->u.DeviceSpecificData.DataSize /
                sizeof(CM_ROM_BLOCK);
        }
    }

    //
    // First check if there is any Extended BIOS Data area.  If yes, we need
    // to map in the physical memory and copy the content to our virtual addr.
    //

    LastMappedAddress = 0;
    while (Index && BiosBlock->Address < ROM_BIOS_START) {
        EBiosInitialized = TRUE;
        destination = (PVOID)(BiosBlock->Address & ~(PAGE_SIZE - 1));
        BaseAddress = (PVOID)0;
        EndingAddress = (BiosBlock->Address + BiosBlock->Size + PAGE_SIZE - 1) &
                        ~(PAGE_SIZE - 1);
        ViewSize = EndingAddress - (ULONG)destination;

        if ((ULONG)destination < LastMappedAddress) {
            if (ViewSize > (LastMappedAddress - (ULONG)destination)) {
                ViewSize = ViewSize - (LastMappedAddress - (ULONG)destination);
                destination = (PVOID)LastMappedAddress;
            } else {
                ViewSize = 0;
            }
        }
        if (ViewSize > 0) {
            ViewBase.LowPart = (ULONG)destination;
            ViewBase.HighPart = 0;

            ntStatus =ZwMapViewOfSection(
                SectionHandle,
                NtCurrentProcess(),
                &BaseAddress,
                0,
                ViewSize,
                &ViewBase,
                &ViewSize,
                ViewUnmap,
                MEM_DOS_LIM,
                PAGE_READWRITE
                );

            if (NT_SUCCESS(ntStatus)) {
                ViewSize = EndingAddress - (ULONG)destination;  // only copy what we need
                LastMappedAddress = (ULONG)destination + ViewSize;
                RtlMoveMemory(destination, BaseAddress, ViewSize);
                ZwUnmapViewOfSection(NtCurrentProcess(), BaseAddress);
            }
        }
        BiosBlock++;
        Index--;
    }

    //
    // NOTE - The code should be removed after product 1.
    //    Due to some problem in VDM initialization, if we pass EBIOS data
    //    area information thru ROM block list, vdm init will fail and our
    //    subsequential int10 mode set will fail.  This prevents new ntdetect
    //    from working with beta versions of NT.  To solve this problem, the
    //    EBIOS information is passed to VDM with fonts information thru DOS
    //    loaded area.
    //
    //    We've shipped two products (about to be 3) and this works fine,
    //    don't screw with it.
    //

    if (EBiosInitialized == FALSE &&
        EBiosInformation->EBiosAddress != 0 &&
        EBiosInformation->EBiosAddress <= VIDEO_BUFFER_START &&
        EBiosInformation->EBiosSize != 0 &&
        (EBiosInformation->EBiosSize & 0x3ff) == 0 &&
        EBiosInformation->EBiosSize < 0x40000) {
        EndingAddress = EBiosInformation->EBiosAddress +
                                EBiosInformation->EBiosSize;
        if (EndingAddress <= VIDEO_BUFFER_START &&
            (EndingAddress & 0x3FF) == 0) {
            destination = (PVOID)(EBiosInformation->EBiosAddress & ~(PAGE_SIZE - 1));
            BaseAddress = (PVOID)0;
            EndingAddress = (EndingAddress + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
            ViewSize = EndingAddress - (ULONG)destination;

            ViewBase.LowPart = (ULONG)destination;
            ViewBase.HighPart = 0;

            ntStatus =ZwMapViewOfSection(
                SectionHandle,
                NtCurrentProcess(),
                &BaseAddress,
                0,
                ViewSize,
                &ViewBase,
                &ViewSize,
                ViewUnmap,
                MEM_DOS_LIM,
                PAGE_READWRITE
                );

            if (NT_SUCCESS(ntStatus)) {
                ViewSize = EndingAddress - (ULONG)destination;  // only copy what we need
                RtlMoveMemory(destination, BaseAddress, ViewSize);
                ZwUnmapViewOfSection(NtCurrentProcess(), BaseAddress);
            }
        }
    }

    //
    // N.B.  Rom blocks begin on 2K (not necessarily page) boundaries
    //       They end on 512 byte boundaries.  This means that we have
    //       to keep track of the last page mapped, and round the next
    //       Rom block up to the next page boundary if necessary.
    //

    LastMappedAddress = ROM_BIOS_START;

    while (Index) {
        if ((Index > 1) &&
            ((BiosBlock->Address + BiosBlock->Size) == BiosBlock[1].Address)) {

            //
            // Coalesce adjacent blocks
            //

            BiosBlock[1].Address = BiosBlock[0].Address;
            BiosBlock[1].Size += BiosBlock[0].Size;
            Index--;
            BiosBlock++;
            continue;
        }

        BaseAddress = (PVOID)(BiosBlock->Address & ~(PAGE_SIZE - 1));
        EndingAddress = (BiosBlock->Address + BiosBlock->Size + PAGE_SIZE - 1) &
                        ~(PAGE_SIZE - 1);
        ViewSize = EndingAddress - (ULONG)BaseAddress;

        if ((ULONG)BaseAddress < LastMappedAddress) {
            if (ViewSize > (LastMappedAddress - (ULONG)BaseAddress)) {
                ViewSize = ViewSize - (LastMappedAddress - (ULONG)BaseAddress);
                BaseAddress = (PVOID)LastMappedAddress;
            } else {
                ViewSize = 0;
            }
        }
        ViewBase.LowPart = (ULONG)BaseAddress;

        if (ViewSize > 0) {

            //
            // Move FF to the non-ROM area to make it like nonexisting memory
            //

            if ((ULONG)BaseAddress - LastMappedAddress > 0) {
                RtlFillMemory((PVOID)LastMappedAddress,
                              (ULONG)BaseAddress - LastMappedAddress,
                              0xFF
                              );
            }

            //
            // First unmap the reserved memory.  This must be done here to prevent
            // the virtual memory in question from being consumed by some other
            // alloc vm call.
            //

            ntStatus = ZwFreeVirtualMemory(
                NtCurrentProcess(),
                &BaseAddress,
                &ViewSize,
                MEM_RELEASE
                );

            // N.B.  This should probably take into account the fact that there are
            // a handfull of error conditions that are ok.  (such as no memory to
            // release.)

            if (!NT_SUCCESS(ntStatus)) {

                ZwClose(SectionHandle);
                ZwClose(RegistryHandle);
                ExFreePool(KeyValueBuffer);
                return;

            }

            ntStatus = ZwMapViewOfSection(
                SectionHandle,
                NtCurrentProcess(),
                &BaseAddress,
                0,
                ViewSize,
                &ViewBase,
                &ViewSize,
                ViewUnmap,
                MEM_DOS_LIM,
                PAGE_READWRITE
                );

            if (!NT_SUCCESS(ntStatus)) {
                break;
            }
            LastMappedAddress = (ULONG)BaseAddress + ViewSize;
        }

        Index--;
        BiosBlock++;
    }

    if (LastMappedAddress < ONE_MEG) {
        RtlFillMemory((PVOID)LastMappedAddress,
                      (ULONG)ONE_MEG - LastMappedAddress,
                      0xFF
                      );
    }

//#if DBG
//    BaseAddress = 0;
//    RegionSize = 0x1000;
//    ZwProtectVirtualMemory ( NtCurrentProcess(),
//                             &BaseAddress,
//                             &RegionSize,
//                             PAGE_NOACCESS,
//                             &OldProtect
//                             );
//#endif

    //
    // Free up the handles
    //

    ZwClose(SectionHandle);
    ZwClose(RegistryHandle);
    ExFreePool(KeyValueBuffer);

    //
    // Everything worked !
    //

    ServerBiosAddressSpaceInitialized = 1;

    return;
}


NTSTATUS
pVideoPortRegisterVDM(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PVIDEO_VDM VdmInfo,
    IN ULONG VdmInfoSize,
    OUT PVIDEO_REGISTER_VDM RegisterVdm,
    IN ULONG RegisterVdmSize,
    OUT PULONG OutputSize
    )

/*++

Routine Description:

    This routine is used to register a VDM when it is started up.

    What this routine does is map the VIDEO BIOS into the VDM address space
    so that DOS apps can use it directly. Since the BIOS is READ_ONLY, we
    have no problem in mapping it as many times as we want.

    It returns the size of the save state buffer that must be allocated by
    the caller.

Arguments:

    DeviceExtension - Pointer to the port driver's device extension.

    VdmInfo - Pointer to the VDM information necessary to perform the
        operation.

    VdmInfoSize - Length of the information buffer.

    RegisterVdm - Pointer to the output buffer into which the save state
        size is stored.

    RegisterVdmSize - Length of the passed in output buffer.

    OutputSize - Pointer to the size of the data stored in the output buffer.
        Can also be the minimum required size of the output buffer is the
        passed in buffer was too small.

Return Value:

    STATUS_SUCCESS if the call completed successfully.

--*/

{

    //
    // Must make sure the caller is a trusted subsystem with the
    // appropriate privilege level before executing this call.
    // If the calls returns FALSE we must return an error code.
    //

    if (!SeSinglePrivilegeCheck(RtlConvertLongToLuid(
                                    SE_TCB_PRIVILEGE),
                                DeviceExtension->CurrentIrpRequestorMode)) {

        return STATUS_PRIVILEGE_NOT_HELD;

    }

    //
    // Check the size of the output buffer.
    //

    if (RegisterVdmSize < sizeof(VIDEO_REGISTER_VDM)) {

        return STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // Return the size required for the save/restore state call.
    //

    *OutputSize = sizeof(VIDEO_REGISTER_VDM);
    RegisterVdm->MinimumStateSize = DeviceExtension->HardwareStateSize;

    return STATUS_SUCCESS;

} // end pVideoPortRegisterVDM()

NTSTATUS
pVideoPortSetIOPM(
    IN ULONG NumAccessRanges,
    IN PVIDEO_ACCESS_RANGE AccessRange,
    IN BOOLEAN Enable,
    IN ULONG IOPMNumber
    )

/*++

Routine Description:

    This routine is used to change the IOPM. It modifies the IOPM based on
    the valid IO ports for the particular device.
    It retrieves the video IOPM mask, changes the access to the I/O ports of
    the specified device and stores the updated mask.

    -- This call can only be performed if the requesting process has the
    appropriate privileges, as determined by the security subsystem. --

Arguments:

    NumAccessRanges - Number of entries in the array of access ranges.

    AccessRange - Pointer to the array of access ranges.

    Enable - Determine if the port listed must be enabled or disabled in the
        mask.

    IOPMNumber - Number of the mask being manipulated.

Return Value:

    STATUS_SUCCESS if the call completed successfully.
    The status from the VideoPortQueryIOPM call if it failed.
    ...

    The return value is also stored in the StatusBlock.

--*/

{

    NTSTATUS ntStatus;
    PKIO_ACCESS_MAP accessMap;
    ULONG port;
    ULONG entries;

    //
    // Retrieve the existing permission mask. If this fails, return
    // immediately.
    //

    if ((accessMap = (PKIO_ACCESS_MAP)ExAllocatePool(NonPagedPool,
                                                     IOPM_SIZE)) == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // Get the kernel map copied into our buffer.
    //

    if (!Ke386QueryIoAccessMap(IOPMNumber,
                               accessMap)) {

        //
        // An error occured while *accessing* the map in the
        // kernel. Return an error and exit normally.
        //

        ExFreePool(accessMap);

        return STATUS_IO_PRIVILEGE_FAILED;

    }

    //
    // Give the calling process access to all the IO ports enabled by the
    // miniport driver in the access range.
    //

    for (entries = 0; entries < NumAccessRanges; entries++) {

        for (port = AccessRange[entries].RangeStart.LowPart;
             (AccessRange[entries].RangeInIoSpace) &&
                 (port < AccessRange[entries].RangeStart.LowPart +
                 AccessRange[entries].RangeLength);
             port++) {

            //
            // Change the port access in the mask:
            // Shift the port address by three to get the index in bytes into
            // the mask. Then take the bottom three bits of the port address
            // and shift 0x01 by that amount to get the right bit in that
            // byte. the bit values are:
            //      0 - access to the port
            //      1 - no access to the port
            //

            if (Enable && AccessRange[entries].RangeVisible) {

                //
                // To give access to a port, NAND 1 with the original port.
                // ex:  11111111 ~& 00001000 = 11110111
                // which gives you  access to the port who's bit was 1.
                // If the port we are enabling is in the current IOPM mask,
                // return an error instead.
                //

                (*accessMap)[port >> 3] &= ~(0x01 << (port & 0x07));

            } else {  // disable mask

                //
                // To remove access to a port, OR 1 with the original port.
                // ex:  11110100 | 00001000 = 11111100
                // which removes access to the port who's bit was 1.
                // If the port we are disabling is not in the current IOPM mask,
                // return an error instead.
                //

                (*accessMap)[port >> 3] |= (0x01 << (port &0x07));

            } // if (Enable) ... else

        } // for (port == ...

    } // for (entries = 0; ...

    //
    // If the mask was updated properly, with no errors, set the new mask.
    // Otherwise, leave the existing one.
    //

    if (Ke386SetIoAccessMap(IOPMNumber,
                                accessMap)) {

        //
        // If the map was created correctly, associate the map to the
        // requesting process. We only need to do this once when the
        // IOPM is first assigned. But we don't know when the first time
        // is.
        //

        if (Ke386IoSetAccessProcess((PKPROCESS) CONTAINING_RECORD(
                                                   PsGetCurrentProcess(),
                                                   EPROCESS,
                                                   Pcb),
                                    IOPMNumber)) {

            ntStatus = STATUS_SUCCESS;

        } else {

            //
            // An error occured while *assigning* the map to
            // the process. Return an error and exit normally.
            //

            ntStatus = STATUS_IO_PRIVILEGE_FAILED;

        }

    } else {

        //
        // An error occured while *creating* the map in the
        // kernel. Return an error and exit normally.
        //

        ntStatus = STATUS_IO_PRIVILEGE_FAILED;

    } // if (Ke386Set ...) ... else

    //
    // Free the memory allocated for the map by the VideoPortQueryIOPM call
    // since the mask has been copied in the kernel TSS.
    //

    ExFreePool(accessMap);

    return ntStatus;

} // end pVideoPortSetIOPM();

VP_STATUS
VideoPortSetTrappedEmulatorPorts(
    PVOID HwDeviceExtension,
    ULONG NumAccessRanges,
    PVIDEO_ACCESS_RANGE AccessRange
    )

/*++

    VideoPortSetTrappedEmulatorPorts (x86 machines only) allows a miniport
    driver to dynamically change the list of I/O ports that are trapped when
    a VDM is running in full-screen mode. The default set of ports being
    trapped by the miniport driver is defined to be all ports in the
    EMULATOR_ACCESS_ENTRY structure of the miniport driver.
    I/O ports not listed in the EMULATOR_ACCESS_ENTRY structure are
    unavailable to the MS-DOS application.  Accessing those ports causes a
    trap to occur in the system, and the I/O operation to be reflected to a
    user-mode virtual device driver.

    The ports listed in the specified VIDEO_ACCESS_RANGE structure will be
    enabled in the I/O Permission Mask (IOPM) associated with the MS-DOS
    application.  This will enable the MS-DOS application to access those I/O
    ports directly, without having the IO instruction trap and be passed down
    to the miniport trap handling functions (for example EmulatorAccessEntry
    functions) for validation.  However, the subset of critical IO ports must
    always remain trapped for robustness.

    All MS-DOS applications use the same IOPM, and therefore the same set of
    enabled/disabled I/O ports.  Thus, on each switch of application, the
    set of trapped I/O ports is reinitialized to be the default set of ports
    (all ports in the EMULATOR_ACCESS_ENTRY structure).

Arguments:

    HwDeviceExtension - Points to the miniport driver's device extension.

    NumAccessRanges - Specifies the number of entries in the VIDEO_ACCESS_RANGE
        structure specified in AccessRange.

    AccessRange - Points to an array of access ranges (VIDEO_ACCESS_RANGE)
        defining the ports that can be untrapped and accessed directly by
        the MS-DOS application.

Return Value:

    This function returns the final status of the operation.

Environment:

    This routine cannot be called from a miniport routine synchronized with
    VideoPortSynchronizeRoutine or from an ISR.

--*/

{

    if (NT_SUCCESS(pVideoPortSetIOPM(NumAccessRanges,
                                     AccessRange,
                                     TRUE,
                                     1))) {

        return NO_ERROR;

    } else {

        return ERROR_INVALID_PARAMETER;

    }

} // end VideoPortSetTrappedEmulatorPorts()


VOID
VideoPortZeroDeviceMemory(
    IN PVOID Destination,
    IN ULONG Length
    )

/*++

Routine Description:

    VideoPortZeroDeviceMemory zeroes a block of device memory of a certain
    length (Length) located at the address specified in Destination.

Arguments:

    Destination - Specifies the starting address of the block of memory to be
        zeroed.

    Length - Specifies the length, in bytes, of the memory to be zeroed.

 Return Value:

    None.

--*/

{

    RtlZeroMemory(Destination,Length);

}
