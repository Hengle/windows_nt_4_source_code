/*++

Copyright (c) 1993  Weitek Corporation

Module Name:

    pci.c

Abstract:

    This module contains PCI code for the Weitek P9 miniport device driver.

Environment:

    Kernel mode

Revision History may be found at the end of this file.

--*/

#include "p9.h"
#include "p9gbl.h"
#include "p9000.h"
#include "pci.h"
#include "vga.h"
#include "p91regs.h"

//
// OEM specific static data.
//

extern VOID
VLSetModeP91(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );


extern VOID VLEnableP91(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VIDEO_ACCESS_RANGE Pci9001DefDACRegRange[] =
{
     {
        RS_0_PCI_9001_ADDR,                       // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_1_PCI_9001_ADDR,                           // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_2_PCI_9001_ADDR,                           // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_3_PCI_9001_ADDR,                           // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_4_PCI_9001_ADDR,                           // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_5_PCI_9001_ADDR,                           // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_6_PCI_9001_ADDR,                           // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_7_PCI_9001_ADDR,                           // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_8_PCI_9001_ADDR,                           // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_9_PCI_9001_ADDR,                           // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_A_PCI_9001_ADDR,                           // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_B_PCI_9001_ADDR,                           // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_C_PCI_9001_ADDR,                      // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_D_PCI_9001_ADDR,                      // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_E_PCI_9001_ADDR,                      // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_F_PCI_9001_ADDR,                      // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     }
};

VIDEO_ACCESS_RANGE Pci9002DefDACRegRange[] =
{
     {
        RS_0_PCI_9002_ADDR,                       // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_1_PCI_9002_ADDR,                           // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_2_PCI_9002_ADDR,                           // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_3_PCI_9002_ADDR,                           // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_4_PCI_9002_ADDR,                           // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_5_PCI_9002_ADDR,                           // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_6_PCI_9002_ADDR,                           // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_7_PCI_9002_ADDR,                           // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_8_PCI_9002_ADDR,                           // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_9_PCI_9002_ADDR,                           // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_A_PCI_9002_ADDR,                           // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_B_PCI_9002_ADDR,                           // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_C_PCI_9002_ADDR,                      // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_D_PCI_9002_ADDR,                      // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_E_PCI_9002_ADDR,                      // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_F_PCI_9002_ADDR,                      // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     }
};

 /******************************************************************************
 ** bIntergraphBoard
 *
 *  PARAMETERS: HwDeviceExtension
 *
 *  DESCRIPTION:    Determine if we're trying to init an Intergraph Board
 *
 *  RETURNS:    TRUE  - if this is an Intergraph Board
 *              FALSE - if this is not an Intergraph Board
 *
 *  CREATED:    02/20/95    13:33:23
 *
 *  BY: c-jeffn
 *
 *  copyright (c) 1995, Newman Consulting
 *
 ******************************************************************************/
BOOLEAN
bIntergraphBoard(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )
{
    ULONG   ulRet;
    UCHAR   jConfig66, jOEMId, *pjOEMId;
    VP_STATUS   vpStatus;
    VIDEO_ACCESS_RANGE  AccessRange;
    BOOLEAN bRet;

    VideoDebugPrint((2, "P9!bIntergraphBoard - Entry\n"));

    // Note that the P9100 must be in native mode before this function
    // is called.

    bRet = FALSE;

    // Test to see if the P9100 indicates external io device is there
 	// If not, can't be Intergraph board

    if ( (HwDeviceExtension->p91State.ulPuConfig & P91_PUC_EXT_IO) == 0 )
        goto exit;
        	
    // Set Bit 4 of Config 66.  This will allow access to the Intergraph
    // Specific registers.

    jConfig66 = 0x10;
    ulRet = VideoPortSetBusData(HwDeviceExtension,
                                PCIConfiguration,
                                HwDeviceExtension->PciSlotNum,
                                &jConfig66,
                                0x42,
                                sizeof (UCHAR));
    if (ulRet != 1)
    {
        VideoDebugPrint((2, "P9!bIntergraphBoard - failed VideoPortSetBusData\n"));
        VideoDebugPrint((2, "\tulRet: %x\n", ulRet));
        goto exit;
    }

    //       Check P9100 register 0x208 for ID in native mode

    ulRet = P9_RD_REG(P91_EXT_IO_ID);    // Get the external io id value
	jOEMId = (UCHAR)(ulRet >> 16);		 // per Weitek programmer's manual

    if (jOEMId == 0xFE)					 // This is the id assigned to Intergraph
        bRet = TRUE;

    // Need to reset Config register 66 bit 4.

    jConfig66 = 0x00;
    ulRet = VideoPortSetBusData(HwDeviceExtension,
                                PCIConfiguration,
                                HwDeviceExtension->PciSlotNum,
                                &jConfig66,
                                0x42,
                                sizeof (UCHAR));

exit:
    VideoDebugPrint((2, "P9!bIntergraphBoard - Exit: %x\n", bRet));

    return (bRet);

}


BOOLEAN
PciGetBaseAddr(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

    Perform board detection and if present return the P9 base address.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

Return Value:

TRUE    - Board found, P9 and Frame buffer address info was placed in
the device extension. PCI extended base address was placed in the
device extension.

FALSE   - Board not found.

--*/
{

    VIDEO_ACCESS_RANGE  PciAccessRange;
    PVIDEO_ACCESS_RANGE DefaultDACRegRange;
    ULONG               ulTempAddr;
    ULONG               ulBiosAddr;
    ULONG               ulBoardAddr;
    ULONG               ulTemp;
    LONG                i;
    VP_STATUS           status;

    ULONG               wcpID;

    //
    // Only the viper p9000 works on the Siemens boxes
    //

    if (HwDeviceExtension->MachineType == SIEMENS
    ||  HwDeviceExtension->MachineType == SIEMENS_P9100_VLB)
    {
        return FALSE;
    }

    //
    // See if the PCI HAL can locate a Weitek 9001 PCI Board.
    //

    //
    // First check for a P9100
    //

    if (PciFindDevice(HwDeviceExtension,
                      WTK_9100_ID,
                      WTK_VENDOR_ID,
                      &HwDeviceExtension->PciSlotNum))
    {
        wcpID = P9100_ID;
        HwDeviceExtension->usBusType = PCI;

        // Just a hack to get things working.
        // NOTE: !!! WE should really do the detection.

        HwDeviceExtension->p91State.bVideoPowerEnabled = FALSE;

        // Now make sure we are looking for a P9100, if were not
        // then fail.

        if (HwDeviceExtension->P9CoprocInfo.CoprocId != P9100_ID)
        {
            return(FALSE);
        }

#ifdef	_MIPS_
                //
                // SNI platform recognition and specific stuff
                //
        {

        extern VP_STATUS                GetCPUIdCallback(
               PVOID                    HwDeviceExtension,
               PVOID                    Context,
               VIDEO_DEVICE_DATA_TYPE   DeviceDataType,
               PVOID                    Identifier,
               ULONG                    IdentifierLength,
               PVOID                    ConfigurationData,
               ULONG                    ConfigurationDataLength,
               PVOID                    ComponentInformation,
               ULONG                    ComponentInformationLength
		);
        if(VideoPortIsCpu(L"RM200PCI")
        || VideoPortIsCpu(L"RM300PCI")
        || VideoPortIsCpu(L"RM300PCI MP")
        || VideoPortIsCpu(L"RM400PCI")
        || VideoPortIsCpu(L"RM4x0PCI"))
                {
                // adjust the VGA physical address with the E/ISA I/O space
                DriverAccessRanges[1].RangeStart.LowPart += 0x14000000 ;
                        HwDeviceExtension->MachineType = SIEMENS_P9100_PCi;
                }
        }
#endif
    }
    else if (PciFindDevice(HwDeviceExtension,
                      WTK_9001_ID,
                      WTK_VENDOR_ID,
                      &HwDeviceExtension->PciSlotNum))
    {
        wcpID = P9000_ID;

        // Now make sure we are looking for a P9000, if were not
        // then fail.

        if (HwDeviceExtension->P9CoprocInfo.CoprocId != P9000_ID)
        {
            return(FALSE);
        }

        //
        // Read the config space to determine if
        // this is Rev 1 or 2. This will determine at which addresses
        // the DAC registers are mapped.
        //

        if (!VideoPortGetBusData(HwDeviceExtension,
                                 PCIConfiguration,
                                 HwDeviceExtension->PciSlotNum,
                                 &ulTemp,
                                 P9001_REV_ID,
                                 sizeof(ulTemp)))
        {
            return(FALSE);
        }

        //
        // Got the 9001 rev id. Choose the appropriate table of DAC register
        // addresses.
        //

        switch((UCHAR) (ulTemp))
        {
            case 1 :
                //
                // This is a Rev 1 9001, which uses the standard VL DAC
                // Addresses. All known rev 1 implementations use the
                // Weitek 5286 VGA chip.
                //

                DefaultDACRegRange = VLDefDACRegRange;
                HwDeviceExtension->AdapterDesc.bWtk5x86 = TRUE;
                break;

            case 2 :
            default:
                //
                // This is a Rev 2 9001. Set up the table of DAC register
                // offsets accordingly.
                //

                DefaultDACRegRange = Pci9001DefDACRegRange;

                //
                // A Rev 2 9001 is present. Get the BIOS ROM address from the
                // PCI configuration space so we can do a ROM scan to
                // determine if this is a Viper PCI adapter.
                //

                PciAccessRange.RangeStart.LowPart = 0;
                PciAccessRange.RangeStart.HighPart = 0;

                if (VideoPortGetBusData(HwDeviceExtension,
                                        PCIConfiguration,
                                        HwDeviceExtension->PciSlotNum,
                                        &PciAccessRange.RangeStart.LowPart,
                                        P9001_BIOS_BASE_ADDR,
                                        sizeof(ULONG)) == 0)
                {
                    return FALSE;
                }

                if (PciAccessRange.RangeStart.LowPart)
                {
                    //
                    // We found an address for the BIOS.  Verify it.
                    //
                    // Set up the access range so we can map out the BIOS ROM
                    // space. This will allow us to scan the ROM and detect the
                    // presence of a Viper PCI adapter.
                    //

                    PciAccessRange.RangeInIoSpace = FALSE;
                    PciAccessRange.RangeVisible = TRUE;
                    PciAccessRange.RangeShareable = TRUE;
                    PciAccessRange.RangeLength = BIOS_RANGE_LEN;

                    //
                    // Check to see if another miniport driver has allocated the
                    // BIOS' memory space.
                    //

                    if (VideoPortVerifyAccessRanges(HwDeviceExtension,
                                                    1L,
                                                    &PciAccessRange) != NO_ERROR)
                    {
                        PciAccessRange.RangeStart.LowPart = 0;
                    }
                }


                if (PciAccessRange.RangeStart.LowPart == 0)
                {
                    //
                    // The Adapter has not been enabled, so we need to map the
                    // BIOS address.
                    //

                    IO_RESOURCE_DESCRIPTOR ioResource = {
                        IO_RESOURCE_PREFERRED,
                        CmResourceTypeMemory,
                        CmResourceShareDeviceExclusive,
                        0,
                        CM_RESOURCE_MEMORY_READ_WRITE,
                        0,
                        {
                          BIOS_RANGE_LEN,    // Length
                          BIOS_RANGE_LEN,    // Alignment
                          { 0x10000000, 0 }, // Minimum start address
                          { 0xefffffff, 0}   // Maximum end address
                        }
                    };

                    status = VideoPortGetAccessRanges(HwDeviceExtension,
                                                      1,
                                                      &ioResource,
                                                      1,
                                                      &PciAccessRange,
                                                      NULL,
                                                      NULL,
                                                      &HwDeviceExtension->PciSlotNum);

                    if (status != NO_ERROR)
                    {
                        return(FALSE);
                    }
                }

                //
                // Map in the BIOS' memory space. If it can't be mapped,
                // return an error.
                //

                if ((ulBiosAddr = (ULONG)
                        VideoPortGetDeviceBase(HwDeviceExtension,
                                               PciAccessRange.RangeStart,
                                               PciAccessRange.RangeLength,
                                               PciAccessRange.RangeInIoSpace)) == 0)
                {
                    return(FALSE);
                }

                //
                // Enable the Adapter BIOS.
                //

                ulTemp = PciAccessRange.RangeStart.LowPart | PCI_BIOS_ENB;

                VideoPortSetBusData(HwDeviceExtension,
                                    PCIConfiguration,
                                    HwDeviceExtension->PciSlotNum,
                                    &ulTemp,
                                    P9001_BIOS_BASE_ADDR,
                                    sizeof(ULONG));

                if (VideoPortScanRom(HwDeviceExtension,
                                     (PUCHAR) ulBiosAddr,
                                     BIOS_RANGE_LEN,
                                     VIPER_ID_STR))
                {
                    //
                    // A Viper PCI is present, use the Viper set mode,
                    // enable/disable video function pointers, and clk
                    // divisor values. Also, Viper PCI does not
                    // use a Weitek VGA.
                    //

                    HwDeviceExtension->AdapterDesc.OEMSetMode = ViperSetMode;
                    HwDeviceExtension->AdapterDesc.P9EnableVideo =
                        ViperPciP9Enable;
                    HwDeviceExtension->AdapterDesc.P9DisableVideo =
                        ViperPciP9Disable;
                    HwDeviceExtension->AdapterDesc.iClkDiv = 4;
                    HwDeviceExtension->AdapterDesc.bWtk5x86 = FALSE;
                }
                else
                {
                    //
                    // All non-Viper Rev 2 implementations use a Weitek
                    // 5286 VGA chip.
                    //

                    HwDeviceExtension->AdapterDesc.bWtk5x86 = TRUE;

                }

                //
                // Restore the BIOS register to it's original value.
                //

                VideoPortSetBusData(HwDeviceExtension,
                                    PCIConfiguration,
                                    HwDeviceExtension->PciSlotNum,
                                    &ulTempAddr,
                                    P9001_BIOS_BASE_ADDR,
                                    sizeof(ULONG));

                VideoPortFreeDeviceBase(HwDeviceExtension, (PVOID) ulBiosAddr);

                break;
        }

    }
    else if (PciFindDevice(HwDeviceExtension,   // Search for a Weitek 9002.
                           WTK_9002_ID,
                           WTK_VENDOR_ID,
                           &HwDeviceExtension->PciSlotNum))
    {
        wcpID = P9000_ID;

        // Now make sure we are looking for a P9000, if were not
        // then fail.

        if (HwDeviceExtension->P9CoprocInfo.CoprocId != P9000_ID)
        {
            return(FALSE);
        }

        //
        // Found a 9002 board. Set up the table of DAC addresses
        // accordingly.
        //

        DefaultDACRegRange = Pci9002DefDACRegRange;
    }
    else
    {
        //
        // No Weitek PCI devices were found, return an error.
        //

        return(FALSE);
    }

    //
    // Get the base address of the adapter.
    // Some machines rely on the address not changing - if the address changes
    // the machine randomly appears to corrupt memory.
    // So use the pre-configured address if it is available.
    //

    HwDeviceExtension->P9PhysAddr.LowPart = 0;

    VideoPortGetBusData(HwDeviceExtension,
                        PCIConfiguration,
                        HwDeviceExtension->PciSlotNum,
                        &HwDeviceExtension->P9PhysAddr.LowPart,
                        P9001_BASE_ADDR,
                        sizeof(ULONG));

    if (HwDeviceExtension->P9PhysAddr.LowPart == 0)
    {
        IO_RESOURCE_DESCRIPTOR ioResource = {
            IO_RESOURCE_PREFERRED,
            CmResourceTypeMemory,
            CmResourceShareDeviceExclusive,
            0,
            CM_RESOURCE_MEMORY_READ_WRITE,
            0,
            {
              RESERVE_PCI_ADDRESS_SPACE,  // Length
              RESERVE_PCI_ADDRESS_SPACE,  // Alignment
              { 0x10000000, 0 },          // Minimum start address
              { 0xefffffff, 0}            // Maximum end address
            }
        };

        status = VideoPortGetAccessRanges(HwDeviceExtension,
                                          1,
                                          &ioResource,
                                          1,
                                          &PciAccessRange,
                                          NULL,
                                          NULL,
                                          &HwDeviceExtension->PciSlotNum);

        if (status == NO_ERROR)
        {
            HwDeviceExtension->P9PhysAddr = PciAccessRange.RangeStart;

            //
            // Save the physical base address in the PCI config space.
            //

            VideoPortSetBusData(HwDeviceExtension,
                                PCIConfiguration,
                                HwDeviceExtension->PciSlotNum,
                                &HwDeviceExtension->P9PhysAddr.LowPart,
                                P9001_BASE_ADDR,
                                sizeof(ULONG));
        }
    }

    if (HwDeviceExtension->P9PhysAddr.LowPart == 0)
    {
        return(FALSE);
    }

    //
    // The P9100 can access the DAC directly, so no I/O space needs to be
    // allocated for DAC access.
    //

    if (wcpID == P9000_ID)
    {

        //
        // The Adapter has not been enabled, so we need to map the
        // base IO address.
        //

        IO_RESOURCE_DESCRIPTOR ioResource = {
            IO_RESOURCE_PREFERRED,
            CmResourceTypePort,
            CmResourceShareDeviceExclusive,
            0,
            CM_RESOURCE_PORT_IO,
            0,
            {
              P9001_IO_RANGE,    // Length
              P9001_IO_RANGE,    // Alignment
              { 0x00001000, 0 }, // Minimum start address
              { 0x0000ffff, 0}   // Maximum end address
            }
        };

        status = VideoPortGetAccessRanges(HwDeviceExtension,
                                          1,
                                          &ioResource,
                                          1,
                                          &PciAccessRange,
                                          NULL,
                                          NULL,
                                          &HwDeviceExtension->PciSlotNum);

        if (status == NO_ERROR)
        {
            HwDeviceExtension->P9001PhysicalAddress = PciAccessRange.RangeStart;
        }
        else
        {
            return(FALSE);
        }

        //
        // Save the physical base address in the PCI config space.
        //

        ulTemp = HwDeviceExtension->P9001PhysicalAddress.LowPart | 0x01;

        VideoPortSetBusData(HwDeviceExtension,
                            PCIConfiguration,
                            HwDeviceExtension->PciSlotNum,
                            &ulTemp,
                            P9001_REG_BASE,
                            sizeof(ULONG));

        //
        // Compute the actual base address.
        //

        HwDeviceExtension->P9001PhysicalAddress.LowPart &= 0xFFFFFFFE;

        //
        // If this is a 9002 board, map in and read the VGA id register.
        //

        if (DefaultDACRegRange == Pci9002DefDACRegRange)
        {
            //
            // Set up the access range so we can map out the VGA ID register.
            //

            PciAccessRange.RangeInIoSpace = TRUE;
            PciAccessRange.RangeVisible = TRUE;
            PciAccessRange.RangeShareable = TRUE;
            PciAccessRange.RangeLength = 1;

            PciAccessRange.RangeStart = HwDeviceExtension->P9001PhysicalAddress;
            PciAccessRange.RangeStart.LowPart += P9002_VGA_ID;

            //
            // Map in the VGA ID register. If it can't be mapped,
            // we can't determine the VGA type, so just use the default.
            //

            if ((ulBoardAddr = (ULONG)
                    VideoPortGetDeviceBase(HwDeviceExtension,
                                           PciAccessRange.RangeStart,
                                           PciAccessRange.RangeLength,
                                           PciAccessRange.RangeInIoSpace)) != 0)
            {
                HwDeviceExtension->AdapterDesc.bWtk5x86 =
                    (VideoPortReadPortUchar((PUCHAR) ulBoardAddr) & VGA_MSK == WTK_VGA);

                VideoPortFreeDeviceBase(HwDeviceExtension,
                                        (PVOID) ulBoardAddr);
            }
            else
            {
                //
                // Assume this is a 5x86 VGA.
                //

                HwDeviceExtension->AdapterDesc.bWtk5x86 = TRUE;
            }
        }

        //
        // Compute the actual DAC register addresses relative to the PCI
        // base address.
        //

        for (i = 0; i < HwDeviceExtension->Dac.cDacRegs; i++)
        {
            //
            // If this is not a palette addr or data register, and the board
            // is not using the standard VL addresses, compute the register
            // address relative to the register base address.
            //

            if ((i > 3) && (DefaultDACRegRange != VLDefDACRegRange))
            {
                DefaultDACRegRange[i].RangeStart.LowPart +=
                    HwDeviceExtension->P9001PhysicalAddress.LowPart;
            }
        }

        //
        // Copy the DAC register access range into the global list of access
        // ranges.
        //

        VideoPortMoveMemory(&DriverAccessRanges[NUM_DRIVER_ACCESS_RANGES],
                            DefaultDACRegRange,
                            sizeof(VIDEO_ACCESS_RANGE) *
                            HwDeviceExtension->Dac.cDacRegs);

        //
        // This is a hack. Initialize an additional access range to map out
        // the entire 4K range of contiguous IO space starting at PCI_REG_BASE.
        // apparently the 9001 decodes accesses over this entire range rather
        // than the individual register offsets within this range.
        //
        //
        // Set up the access range so we can map the entire 4k IO range.
        //

        PciAccessRange.RangeInIoSpace = TRUE;
        PciAccessRange.RangeVisible = TRUE;
        PciAccessRange.RangeShareable = TRUE;
        PciAccessRange.RangeLength = P9001_IO_RANGE;
        PciAccessRange.RangeStart = HwDeviceExtension->P9001PhysicalAddress;

        VideoPortMoveMemory(&DriverAccessRanges[NUM_DRIVER_ACCESS_RANGES +
                                                NUM_DAC_ACCESS_RANGES],
                            &PciAccessRange,
                            sizeof(VIDEO_ACCESS_RANGE));
    }

    return(TRUE);

}


BOOLEAN
PciFindDevice(
    IN      PHW_DEVICE_EXTENSION    HwDeviceExtension,
    IN      USHORT                  usDeviceId,
    IN      USHORT                  usVendorId,
    IN OUT  PULONG                  pulSlotNum
    )

/*++

Routine Description:

    Attempts to find a PCI device which matches the passed device id, vendor
    id and index.

Arguments:

    HwDeviceExtension - Pointer to the device extension.
    usDeviceId - PCI Device Id.
    usVendorId - PCI Vendor Id.
    pulSlotNum - Input -> Starting Slot Number
                 Output -> If found, the slot number of the matching device.

Return Value:

    TRUE if device found.

--*/
{
    ULONG   pciBuffer;
    USHORT  j;
    PCI_SLOT_NUMBER slotData;
    PPCI_COMMON_CONFIG  pciData;

    //
    //
    // typedef struct _PCI_SLOT_NUMBER {
    //     union {
    //         struct {
    //             ULONG   DeviceNumber:5;
    //             ULONG   FunctionNumber:3;
    //             ULONG   Reserved:24;
    //         } bits;
    //         ULONG   AsULONG;
    //     } u;
    // } PCI_SLOT_NUMBER, *PPCI_SLOT_NUMBER;
    //

    slotData.u.AsULONG = 0;

    pciData = (PPCI_COMMON_CONFIG) &pciBuffer;

    //
    // Look at each device.
    //

    *pulSlotNum = 0;

    while (*pulSlotNum < 32)
    {
        slotData.u.bits.DeviceNumber = *pulSlotNum;

        //
        // Look at each function.
        //

        for (j= 0; j < 8; j++)
        {

            slotData.u.bits.FunctionNumber = j;

            if (VideoPortGetBusData(HwDeviceExtension,
                                    PCIConfiguration,
                                    slotData.u.AsULONG,
                                    (PVOID) pciData,
                                    0,
                                    sizeof(ULONG)) == 0)
            {
                //
                // Out of functions. Go to next PCI bus.
                //

                break;
            }


            if (pciData->VendorID == PCI_INVALID_VENDORID)
            {
                //
                // No PCI device, or no more functions on device
                // move to next PCI device.
                //

                break;
            }

            if (pciData->VendorID == usVendorId &&
                pciData->DeviceID == usDeviceId)
            {
                *pulSlotNum = slotData.u.AsULONG;
                return(TRUE);
            }
        }

        (*pulSlotNum)++;
    }

    //
    // No matching PCI device was found.
    //

    return(FALSE);
}


BOOLEAN
PciP9MemEnable(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

Enable the physical memory and IO resources for PCI adapters.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

Return Value:

    None.

--*/

{
   ULONG    ulTemp;

    //
    // Read the PCI command register to determine if the memory/io
    // resources are enabled. If not, enable them.
    //

    if (!VideoPortGetBusData(HwDeviceExtension,
                             PCIConfiguration,
                             HwDeviceExtension->PciSlotNum,
                             &ulTemp,
                             P9001_CMD_REG,
                             sizeof(ulTemp)))
    {
        return(FALSE);
    }
    else if (!(ulTemp & (P9001_MEM_ENB | P9001_IO_ENB)))
    {
        ulTemp |= (P9001_MEM_ENB | P9001_IO_ENB);

        if (!VideoPortSetBusData(HwDeviceExtension,
                                 PCIConfiguration,
                                 HwDeviceExtension->PciSlotNum,
                                 &ulTemp,
                                 P9001_CMD_REG,
                                 sizeof(ulTemp)))
        {
            return(FALSE);
        }
    }
    return(TRUE);
}


VOID
ViperPciP9Enable(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

Perform the OEM specific tasks necessary to enable the P9. These
include memory mapping, setting the sync polarities, and enabling the
P9 video output.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

Return Value:

    None.

--*/

{

    USHORT  holdit;

    //
    // Select external frequency, and clear the polarity bits.
    //

    holdit = VGA_RD_REG(MISCIN) | (MISCD | MISCC);
    holdit &= ~(VIPER_HSYNC_POL_MASK | VIPER_VSYNC_POL_MASK);

    //
    // Viper controls h and v sync polarities independently.
    //

    if (HwDeviceExtension->VideoData.vp == POSITIVE)
    {
        holdit |= VIPER_VSYNC_POL_MASK;
    }

    if (HwDeviceExtension->VideoData.hp == POSITIVE)
    {
        holdit |= VIPER_HSYNC_POL_MASK;
    }

    VGA_WR_REG(MISCOUT, holdit);

    //
    // If this is a Weitek VGA, unlock the VGA.
    //

    if (HwDeviceExtension->AdapterDesc.bWtk5x86)
    {
        UnlockVGARegs(HwDeviceExtension);
    }

    //
    // Enable P9 Video.
    //

    VGA_WR_REG(SEQ_INDEX_PORT, SEQ_OUTCNTL_INDEX);
    VGA_WR_REG(SEQ_DATA_PORT, (VGA_RD_REG(SEQ_DATA_PORT)) | P9_VIDEO_ENB);

    //
    // If this is a Weitek VGA, lock the VGA sequencer registers.
    //

    if (HwDeviceExtension->AdapterDesc.bWtk5x86)
    {
        LockVGARegs(HwDeviceExtension);
    }

    return;
}


BOOLEAN
ViperPciP9Disable(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:


Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.
    pPal - Pointer to the array of pallete entries.
    StartIndex - Specifies the first pallete entry provided in pPal.
    Count - Number of palette entries in pPal

Return Value:

    TRUE, indicating no int10 is needed to complete the switch

--*/

{

    //
    //  Unlock the VGA extended regs to disable P9 video output.
    //

    //
    // If this is a Weitek VGA, unlock the VGA.
    //

    if (HwDeviceExtension->AdapterDesc.bWtk5x86)
    {
        UnlockVGARegs(HwDeviceExtension);
    }

    VGA_WR_REG(SEQ_INDEX_PORT, SEQ_OUTCNTL_INDEX);
    VGA_WR_REG(SEQ_DATA_PORT, (VGA_RD_REG(SEQ_DATA_PORT)) & P9_VIDEO_DIS);

    //
    // Restore clock select bits.
    //

    VGA_WR_REG(MISCOUT, HwDeviceExtension->MiscRegState);

    //
    // If this is a Weitek VGA, lock the VGA sequencer registers.
    //

    if (HwDeviceExtension->AdapterDesc.bWtk5x86)
    {
        LockVGARegs(HwDeviceExtension);
    }

    return TRUE;
}
