/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   wstree.c

Abstract:

    This module contains the routines which manipulate the working
    set list tree.

Author:

    Lou Perazzoli (loup) 15-May-1989

Revision History:

--*/

#include "mi.h"

#if (_MSC_VER >= 800)
#pragma warning(disable:4010)           /* Allow pretty pictures without the noise */
#endif

extern ULONG MmSystemCodePage;
extern ULONG MmSystemCachePage;
extern ULONG MmPagedPoolPage;
extern ULONG MmSystemDriverPage;

ULONG MmNumberOfInserts;

ULONG
MiLookupWsleHashIndex (
    IN ULONG WsleEntry,
    IN PMMWSL WorkingSetList
    );

VOID
MiCheckWsleHash (
    IN PMMWSL WorkingSetList
    );


VOID
FASTCALL
MiInsertWsle (
    IN ULONG Entry,
    IN PMMWSL WorkingSetList
    )

/*++

Routine Description:

    This routine inserts a Working Set List Entry (WSLE) into the
    working set tree.

Arguments:

    Entry - The index number of the WSLE to insert.


Return Value:

    None.

Environment:

    Kernel mode, APC's disabled, Working Set Mutex held.

--*/

{
    ULONG i;
    ULONG Parent;
    PVOID VirtualAddress;
    PMMWSLE Wsle;
    PMMSUPPORT WsInfo;
    ULONG Hash;
    PMMWSLE_HASH Table;
    ULONG j;
    PMMPTE PointerPte;
    ULONG Index;
    LARGE_INTEGER TickCount;
    ULONG Size;
    KIRQL OldIrql;

    Wsle = WorkingSetList->Wsle;

    VirtualAddress = PAGE_ALIGN(Wsle[Entry].u1.VirtualAddress);

#if DBG
    if (MmDebug & MM_DBG_PTE_UPDATE) {
        DbgPrint("inserting element %lx %lx\n", Entry, Wsle[Entry].u1.Long);
    }

    ASSERT (Wsle[Entry].u1.e1.Valid == 1);
    ASSERT (Wsle[Entry].u1.e1.Direct != 1);
#endif //DBG

    WorkingSetList->NonDirectCount += 1;

    if ((Table = WorkingSetList->HashTable) == NULL) {
        return;
    }

#if DBG
    MmNumberOfInserts += 1;
#endif //DBG

    Hash = (Wsle[Entry].u1.Long >> (PAGE_SHIFT - 2)) % (WorkingSetList->HashTableSize - 1);

    //
    // Check hash table size and see if there is enough room to
    // hash or if the table should be grown.
    //

    if ((WorkingSetList->NonDirectCount + 10 +
            (WorkingSetList->HashTableSize >> 4)) >
                                 WorkingSetList->HashTableSize) {

        if (MmWorkingSetList == WorkingSetList) {
            WsInfo = &PsGetCurrentProcess()->Vm;
        } else {
            WsInfo = &MmSystemCacheWs;
        }

        if ((WorkingSetList->HashTableSize <
                 (MM_WSLE_MAX_HASH_SIZE - ((2*PAGE_SIZE) / sizeof (MMWSLE_HASH))))
                     &&
            (WsInfo->AllowWorkingSetAdjustment)) {
            LOCK_EXPANSION_IF_ALPHA (OldIrql);
            WsInfo->AllowWorkingSetAdjustment = MM_GROW_WSLE_HASH;
            UNLOCK_EXPANSION_IF_ALPHA (OldIrql);
        }

        if ((WorkingSetList->NonDirectCount +
                (WorkingSetList->HashTableSize >> 4)) >
                                     WorkingSetList->HashTableSize) {

            //
            // No more room in the hash table, remove one and add there.
            // Pick a victum within 16 of where this would hash to.
            //

            KeQueryTickCount(&TickCount);
            j = Hash + (TickCount.LowPart & 0xF);

            Size = WorkingSetList->HashTableSize;

            if (j >= Size) {
                j = TickCount.LowPart & 0xF;
            }

            do {
                if (Table[j].Key != 0) {
                    PointerPte = MiGetPteAddress (Table[j].Key);
                    Index = WorkingSetList->HashTable[j].Index;
                    ASSERT (Wsle[Index].u1.e1.Valid == 1);
                    PointerPte = MiGetPteAddress (Wsle[Index].u1.VirtualAddress);
                    if (MiFreeWsle (Index, WsInfo, PointerPte)) {
                        break;
                    }
                }
                j += 1;
                if (j >= Size) {
                    j = 0;
                }
            } while (TRUE);
        }
    }

    //
    // Add to the hash table.
    //

    while (Table[Hash].Key != 0) {
        Hash += 1;
        if (Hash >= WorkingSetList->HashTableSize) {
            Hash = 0;
        }
    }

    Table[Hash].Key = Wsle[Entry].u1.Long & ~(PAGE_SIZE - 1);
    Table[Hash].Index = Entry;

#if DBG
    if ((MmNumberOfInserts % 1000) == 0) {
        MiCheckWsleHash (WorkingSetList);
    }
#endif //DBG
    return;
}

#if DBG
VOID
MiCheckWsleHash (
    IN PMMWSL WorkingSetList
    )

{
    ULONG j;
    ULONG found = 0;
    PMMWSLE Wsle;

    Wsle = WorkingSetList->Wsle;

    for (j =0; j < WorkingSetList->HashTableSize ; j++ ) {
        if (WorkingSetList->HashTable[j].Key != 0) {
            found += 1;
            ASSERT (WorkingSetList->HashTable[j].Key ==
                     (Wsle[WorkingSetList->HashTable[j].Index].u1.Long &
                                                            ~(PAGE_SIZE -1)));
        }
    }
    if (found != WorkingSetList->NonDirectCount) {
        DbgPrint("MMWSLE: Found %lx, nondirect %lx\n",
                    found, WorkingSetList->NonDirectCount);
        DbgBreakPoint();
    }
}
#endif //dbg


ULONG
FASTCALL
MiLocateWsle (
    IN PVOID VirtualAddress,
    IN PMMWSL WorkingSetList,
    IN ULONG WsPfnIndex
    )

/*++

Routine Description:

    This function locates the specified virtual address within the
    working set list.

Arguments:

    VirtualAddress - Supplies the virtual to locate within the working
                     set list.

Return Value:

    Returns the index into the working set list which contains the entry.

Environment:

    Kernel mode, APC's disabled, Working Set Mutex held.

--*/

{
    ULONG i;
    PMMWSLE Wsle;
    ULONG LastWsle;
    ULONG Hash;
    PMMWSLE_HASH Table;
    ULONG Tries;

    Wsle = WorkingSetList->Wsle;

    VirtualAddress = PAGE_ALIGN(VirtualAddress);

    if (WsPfnIndex <= WorkingSetList->LastInitializedWsle) {
        if ((VirtualAddress == PAGE_ALIGN(Wsle[WsPfnIndex].u1.VirtualAddress)) &&
            (Wsle[WsPfnIndex].u1.e1.Valid == 1)) {
            return WsPfnIndex;
        }
    }

    if (WorkingSetList->HashTable) {
        Tries = 0;
        Table = WorkingSetList->HashTable;

        Hash = ((ULONG)VirtualAddress >> (PAGE_SHIFT - 2)) % (WorkingSetList->HashTableSize - 1);

        while (Table[Hash].Key != (ULONG)VirtualAddress) {
            Hash += 1;
            if (Hash >= WorkingSetList->HashTableSize) {
                Hash = 0;
                if (Tries != 0) {
                    KeBugCheckEx (MEMORY_MANAGEMENT,
                                  0x41284,
                                  (ULONG)VirtualAddress,
                                  WsPfnIndex,
                                  (ULONG)WorkingSetList);
                }
                Tries = 1;
            }
        }
        ASSERT (WorkingSetList->Wsle[Table[Hash].Index].u1.e1.Direct == 0);
        return Table[Hash].Index;
    }

    i = 0;

    for (; ; ) {
        if ((VirtualAddress == PAGE_ALIGN(Wsle[i].u1.VirtualAddress)) &&
            (Wsle[i].u1.e1.Valid == 1)) {
            ASSERT (WorkingSetList->Wsle[i].u1.e1.Direct == 0);
            return i;
        }
        i += 1;
    }
}


#if 0

ULONG
MiLocateWsleAndParent (
    IN PVOID VirtualAddress,
    OUT PULONG Parent,
    IN PMMWSL WorkingSetList,
    IN ULONG WsPfnIndex
    )

/*++

Routine Description:

    This routine locates both the working set list entry (via index) and
    it's parent.

Arguments:

    VirtualAddress - Supplies the virtual address of the WSLE to locate.

    Parent - Returns the index into the working set list for the parent.

    WorkingSetList - Supplies a pointer to the working set list.

    WsPfnIndex - Supplies the index field from the PFN database for
                 the physical page that maps the specified virtual address.

Return Value:

    Retuns the index of the virtual address in the working set list.

Environment:

    Kernel mode, APC's disabled, Working Set Mutex held.

--*/

{
    ULONG Previous;
    ULONG Entry;
    PMMWSLE Wsle;

    Wsle = WorkingSetList->Wsle;

    //
    // Check to see if the PfnIndex field refers to the WSLE in question.
    // Make sure the index is within the specified working set list.
    //

    if (WsPfnIndex <= WorkingSetList->LastInitializedWsle) {
        if (VirtualAddress == PAGE_ALIGN(Wsle[WsPfnIndex].u1.VirtualAddress)) {

            //
            // The index field points to the WSLE, however, this could
            // have been just a coincidence, so check to ensure it
            // really doesn't have a parent.
            //

            if (Wsle[WsPfnIndex].u2.BothPointers == 0) {

                //
                // Not in tree, therefore has no parent.
                //

                *Parent = WSLE_NULL_INDEX;
                return WsPfnIndex;
            }
        }
    }

    //
    // Search the tree for the entry remembering the parents.
    //

    Entry = WorkingSetList->Root;
    Previous = Entry;

    for (;;) {

        ASSERT (Entry != WSLE_NULL_INDEX);

        if (VirtualAddress == PAGE_ALIGN(Wsle[Entry].u1.VirtualAddress)) {
            break;
        }

        if (VirtualAddress < PAGE_ALIGN(Wsle[Entry].u1.VirtualAddress)) {
            Previous = Entry;
            Entry = Wsle[Entry].u2.s.LeftChild;
        } else {
            Previous = Entry;
            Entry = Wsle[Entry].u2.s.RightChild;
        }
    }

    *Parent = Previous;
    return Entry;
}
#endif //0


VOID
FASTCALL
MiRemoveWsle (
    ULONG Entry,
    IN PMMWSL WorkingSetList
    )

/*++

Routine Description:

    This routine removes a Working Set List Entry (WSLE) from the
    working set tree.

Arguments:

    Entry - The index number of the WSLE to remove.


Return Value:

    None.

Environment:

    Kernel mode, APC's disabled, Working Set Mutex held.

--*/
{
    ULONG i;
    ULONG Parent;
    ULONG Pred;
    ULONG PredParent;
    PMMWSLE Wsle;
    PVOID VirtualAddress;
    PMMWSLE_HASH Table;
    ULONG Hash;
    ULONG Tries;

    Wsle = WorkingSetList->Wsle;

    //
    // Locate the entry in the tree.
    //

#if DBG
    if (MmDebug & MM_DBG_PTE_UPDATE) {
        DbgPrint("removing wsle %lx   %lx\n",
            Entry, Wsle[Entry].u1.Long);
    }
    if (MmDebug & MM_DBG_DUMP_WSL) {
        MiDumpWsl();
        DbgPrint(" \n");
    }

#endif //DBG

    ASSERT (Wsle[Entry].u1.e1.Valid == 1);

    VirtualAddress = PAGE_ALIGN (Wsle[Entry].u1.VirtualAddress);

    if (WorkingSetList == MmSystemCacheWorkingSetList) {

        //
        // count system space inserts and removals.
        //

        if (VirtualAddress < (PVOID)MM_SYSTEM_CACHE_START) {
            MmSystemCodePage -= 1;
        } else if (VirtualAddress < MM_PAGED_POOL_START) {
            MmSystemCachePage -= 1;
        } else if (VirtualAddress < MmNonPagedSystemStart) {
            MmPagedPoolPage -= 1;
        } else {
            MmSystemDriverPage -= 1;
        }
    }

    Wsle[Entry].u1.e1.Valid = 0;

    if (Wsle[Entry].u1.e1.Direct == 0) {

        WorkingSetList->NonDirectCount -= 1;

        if (WorkingSetList->HashTable) {
            Hash = (Wsle[Entry].u1.Long >> (PAGE_SHIFT - 2)) % (WorkingSetList->HashTableSize - 1);
            Table = WorkingSetList->HashTable;
            Tries = 0;

            while (Table[Hash].Key != (ULONG)VirtualAddress) {
                Hash += 1;
                if (Hash >= WorkingSetList->HashTableSize) {
                    Hash = 0;
                    if (Tries != 0) {
                        KeBugCheckEx (MEMORY_MANAGEMENT,
                                      0x41784,
                                      (ULONG)VirtualAddress,
                                      Entry,
                                      (ULONG)WorkingSetList);
                    }
                    Tries = 1;
                }
            }
            Table[Hash].Key = 0;
        }
    }

    return;
}


VOID
MiSwapWslEntries (
    IN ULONG SwapEntry,
    IN ULONG Entry,
    IN PMMSUPPORT WsInfo
    )

/*++

Routine Description:

    This routine swaps the working set list entries Entry and SwapEntry
    in the specified working set list (process or system cache).

Arguments:

    SwapEntry - Supplies the first entry to swap.  This entry must be
                valid, i.e. in the working set at the current time.

    Entry - Supplies the other entry to swap.  This entry may be valid
            or invalid.

    WsInfo - Supplies the working set list.

Return Value:

    None.

Environment:

    Kernel mode, Working set lock and PFN lock held (if system cache),
                 APC's disabled.

--*/

{
    MMWSLE WsleEntry;
    MMWSLE WsleSwap;
    PMMPTE PointerPte;
    PMMPFN Pfn1;
    PMMWSLE Wsle;
    PMMWSL WorkingSetList;
    PMMWSLE_HASH Table;
#if DBG
    ULONG CurrentSize = WsInfo->WorkingSetSize;
#endif //DBG

    WorkingSetList = WsInfo->VmWorkingSetList;
    Wsle = WorkingSetList->Wsle;

    WsleSwap = Wsle[SwapEntry];

    ASSERT (WsleSwap.u1.e1.Valid != 0);

    WsleEntry = Wsle[Entry];

    Table = WorkingSetList->HashTable;

    if (WsleEntry.u1.e1.Valid == 0) {

        //
        // Entry is not on any list. Remove it from the free list.
        //

        MiRemoveWsleFromFreeList (Entry, Wsle, WorkingSetList);

        //
        // Copy the Entry to this free one.
        //

        Wsle[Entry] = WsleSwap;

        if (WsleSwap.u1.e1.Direct) {
            PointerPte = MiGetPteAddress (WsleSwap.u1.VirtualAddress);
            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
            Pfn1->u1.WsIndex = Entry;
        } else {

            //
            // Update hash table.
            //

            if (Table) {
                Table [ MiLookupWsleHashIndex (WsleSwap.u1.Long,
                                           WorkingSetList)].Index = Entry;
            }
        }

        //
        // Put entry on free list.
        //

        ASSERT (WorkingSetList->FirstFree <= WorkingSetList->LastInitializedWsle);
        Wsle[SwapEntry].u1.Long = WorkingSetList->FirstFree << MM_FREE_WSLE_SHIFT;
        WorkingSetList->FirstFree = SwapEntry;
        ASSERT ((WorkingSetList->FirstFree <= WorkingSetList->LastInitializedWsle) ||
            (WorkingSetList->FirstFree == WSLE_NULL_INDEX));

    } else {

        //
        // Both entries are valid.
        //

        Wsle[SwapEntry] = WsleEntry;

        if (WsleEntry.u1.e1.Direct) {

            //
            // Swap the PFN WsIndex element to point to the new slot.
            //

            PointerPte = MiGetPteAddress (WsleEntry.u1.VirtualAddress);
            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
            Pfn1->u1.WsIndex = SwapEntry;
        } else {

            //
            // Update hash table.
            //

            if (Table) {
                Table[ MiLookupWsleHashIndex (WsleEntry.u1.Long,
                                           WorkingSetList)].Index = SwapEntry;
            }
        }

        Wsle[Entry] = WsleSwap;

        if (WsleSwap.u1.e1.Direct) {

            PointerPte = MiGetPteAddress (WsleSwap.u1.VirtualAddress);
            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
            Pfn1->u1.WsIndex = Entry;
        } else {
            if (Table) {
                Table[ MiLookupWsleHashIndex (WsleSwap.u1.Long,
                                           WorkingSetList)].Index = Entry;
            }
        }
    }
    ASSERT (CurrentSize == WsInfo->WorkingSetSize);
    return;
}

ULONG
MiLookupWsleHashIndex (
    IN ULONG WsleEntry,
    IN PMMWSL WorkingSetList
    )

{
    ULONG Hash;
    ULONG VirtualAddress;
    PMMWSLE_HASH Table;
    ULONG Tries = 0;

    Table = WorkingSetList->HashTable;
    VirtualAddress = WsleEntry & ~(PAGE_SIZE - 1);

    Hash = ((ULONG)VirtualAddress >> (PAGE_SHIFT - 2)) % (WorkingSetList->HashTableSize - 1);

    while (Table[Hash].Key != (ULONG)VirtualAddress) {
        Hash += 1;
        if (Hash >= WorkingSetList->HashTableSize) {
            Hash = 0;
            if (Tries != 0) {
                KeBugCheckEx (MEMORY_MANAGEMENT,
                              0x41884,
                              (ULONG)VirtualAddress,
                              WsleEntry,
                              (ULONG)WorkingSetList);
            }
            Tries = 1;
        }
    }
    return Hash;
}

VOID
MiRemoveWsleFromFreeList (
    IN ULONG Entry,
    IN PMMWSLE Wsle,
    IN PMMWSL WorkingSetList
    )

/*++

Routine Description:

    This routine removes a working set list entry from the free list.
    It is used when the entry to required is not the first element
    in the free list.

Arguments:

    Entry - Supplies the index of the entry to remove.

    Wsle - Supplies a pointer to the array of WSLEs.

    WorkingSetList - Supplies a pointer to the working set list.

Return Value:

    None.

Environment:

    Kernel mode, Working set lock and PFN lock held, APC's disabled.

--*/

{
    ULONG Free;
    ULONG ParentFree;

    Free = WorkingSetList->FirstFree;

    if (Entry == Free) {
        ASSERT ((Wsle[Entry].u1.Long >> MM_FREE_WSLE_SHIFT) <= WorkingSetList->LastInitializedWsle);
        WorkingSetList->FirstFree = Wsle[Entry].u1.Long >> MM_FREE_WSLE_SHIFT;

    } else {
        do {
            ParentFree = Free;
            ASSERT (Wsle[Free].u1.e1.Valid == 0);
            Free = Wsle[Free].u1.Long >> MM_FREE_WSLE_SHIFT;
        } while (Free != Entry);

        Wsle[ParentFree].u1.Long = Wsle[Entry].u1.Long;
    }
    ASSERT ((WorkingSetList->FirstFree <= WorkingSetList->LastInitializedWsle) ||
            (WorkingSetList->FirstFree == WSLE_NULL_INDEX));
    return;
}


#if 0

VOID
MiSwapWslEntries (
    IN ULONG Entry,
    IN ULONG Parent,
    IN ULONG SwapEntry,
    IN PMMWSL WorkingSetList
    )

/*++

Routine Description:

    This function swaps the specified entry and updates its parent with
    the specified swap entry.

    The entry must be valid, i.e., the page is resident.  The swap entry
    can be valid or on the free list.

Arguments:

    Entry - The index of the WSLE to swap.

    Parent - The index of the parent of the WSLE to swap.

    SwapEntry - The index to swap the entry with.

Return Value:

    None.

Environment:

    Kernel mode, working set mutex held, APC's disabled.

--*/

{

    ULONG SwapParent;
    ULONG SavedRight;
    ULONG SavedLeft;
    ULONG Free;
    ULONG ParentFree;
    ULONG SavedLong;
    PVOID VirtualAddress;
    PMMWSLE Wsle;
    PMMPFN Pfn1;
    PMMPTE PointerPte;

    Wsle = WorkingSetList->Wsle;

    if (Wsle[SwapEntry].u1.e1.Valid == 0) {

        //
        // This entry is not in use and must be removed from
        // the free list.
        //

        Free = WorkingSetList->FirstFree;

        if (SwapEntry == Free) {
            WorkingSetList->FirstFree = Entry;
            ASSERT ((WorkingSetList->FirstFree <= WorkingSetList->LastInitializedWsle) ||
                (WorkingSetList->FirstFree == WSLE_NULL_INDEX));

        } else {

            while (Free != SwapEntry) {
                ParentFree = Free;
                Free = Wsle[Free].u2.s.LeftChild;
            }

            Wsle[ParentFree].u2.s.LeftChild = Entry;
        }

        //
        // Swap the previous entry and the new unused entry.
        //

        SavedLeft = Wsle[Entry].u2.s.LeftChild;
        Wsle[Entry].u2.s.LeftChild = Wsle[SwapEntry].u2.s.LeftChild;
        Wsle[SwapEntry].u2.s.LeftChild = SavedLeft;
        Wsle[SwapEntry].u2.s.RightChild = Wsle[Entry].u2.s.RightChild;
        Wsle[SwapEntry].u1.Long = Wsle[Entry].u1.Long;
        Wsle[Entry].u1.Long = 0;

        //
        // Make the parent point to the new entry.
        //

        if (Parent == WSLE_NULL_INDEX) {

            //
            // This entry is not in the tree.
            //

            PointerPte = MiGetPteAddress (Wsle[SwapEntry].u1.VirtualAddress);
            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
            Pfn1->u1.WsIndex = SwapEntry;
            return;
        }

        if (Parent == Entry) {

            //
            // This element is the root, update the root pointer.
            //

            WorkingSetList->Root = SwapEntry;

        } else {

            if (Wsle[Parent].u2.s.LeftChild == Entry) {
                Wsle[Parent].u2.s.LeftChild = SwapEntry;
            } else {
                ASSERT (Wsle[Parent].u2.s.RightChild == Entry);

                Wsle[Parent].u2.s.RightChild = SwapEntry;
            }
        }

    } else {

        if ((Parent == WSLE_NULL_INDEX) &&
            (Wsle[SwapEntry].u2.BothPointers == 0)) {

            //
            // Neither entry is in the tree, just swap their pointers.
            //

            SavedLong = Wsle[SwapEntry].u1.Long;
            Wsle[SwapEntry].u1.Long = Wsle[Entry].u1.Long;
            Wsle[Entry].u1.Long = SavedLong;

            PointerPte = MiGetPteAddress (Wsle[Entry].u1.VirtualAddress);
            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
            Pfn1->u1.WsIndex = Entry;

            PointerPte = MiGetPteAddress (Wsle[SwapEntry].u1.VirtualAddress);
            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
            Pfn1->u1.WsIndex = SwapEntry;

            return;
        }

        //
        // The entry at FirstDynamic is valid; swap it with this one and
        // update both parents.
        //

        SwapParent = WorkingSetList->Root;

        if (SwapParent == SwapEntry) {

            //
            // The entry we are swapping with is at the root.
            //

            if (Wsle[SwapEntry].u2.s.LeftChild == Entry) {

                //
                // The entry we are going to swap is the left child of this
                // entry.
                //
                //              R(SwapEntry)
                //             / \
                //      (entry)
                //

                WorkingSetList->Root = Entry;

                Wsle[SwapEntry].u2.s.LeftChild = Wsle[Entry].u2.s.LeftChild;
                Wsle[Entry].u2.s.LeftChild = SwapEntry;
                SavedRight = Wsle[SwapEntry].u2.s.RightChild;
                Wsle[SwapEntry].u2.s.RightChild = Wsle[Entry].u2.s.RightChild;
                Wsle[Entry].u2.s.RightChild = SavedRight;

                SavedLong = Wsle[Entry].u1.Long;
                Wsle[Entry].u1.Long = Wsle[SwapEntry].u1.Long;
                Wsle[SwapEntry].u1.Long = SavedLong;

                return;

            } else {

                if (Wsle[SwapEntry].u2.s.RightChild == Entry) {

                    //
                    // The entry we are going to swap is the right child of this
                    // entry.
                    //
                    //              R(SwapEntry)
                    //             / \
                    //                (entry)
                    //

                    WorkingSetList->Root = Entry;

                    Wsle[SwapEntry].u2.s.RightChild = Wsle[Entry].u2.s.RightChild;
                    Wsle[Entry].u2.s.RightChild = SwapEntry;
                    SavedLeft = Wsle[SwapEntry].u2.s.LeftChild;
                    Wsle[SwapEntry].u2.s.LeftChild = Wsle[Entry].u2.s.LeftChild;
                    Wsle[Entry].u2.s.LeftChild = SavedLeft;


                    SavedLong = Wsle[Entry].u1.Long;
                    Wsle[Entry].u1.Long = Wsle[SwapEntry].u1.Long;
                    Wsle[SwapEntry].u1.Long = SavedLong;

                    return;
                }
            }

            //
            // The swap entry is the root, but the other entry is not
            // its child.
            //
            //
            //              R(SwapEntry)
            //             / \
            //            .....
            //                 Parent(Entry)
            //                  \
            //                   Entry (left or right)
            //
            //

            WorkingSetList->Root = Entry;

            SavedRight = Wsle[SwapEntry].u2.s.RightChild;
            Wsle[SwapEntry].u2.s.RightChild = Wsle[Entry].u2.s.RightChild;
            Wsle[Entry].u2.s.RightChild = SavedRight;
            SavedLeft = Wsle[SwapEntry].u2.s.LeftChild;
            Wsle[SwapEntry].u2.s.LeftChild = Wsle[Entry].u2.s.LeftChild;
            Wsle[Entry].u2.s.LeftChild = SavedLeft;

            SavedLong = Wsle[Entry].u1.Long;
            Wsle[Entry].u1.Long = Wsle[SwapEntry].u1.Long;
            Wsle[SwapEntry].u1.Long = SavedLong;

            if (Parent == WSLE_NULL_INDEX) {

                //
                // This entry is not in the tree.
                //

                PointerPte = MiGetPteAddress (Wsle[SwapEntry].u1.VirtualAddress);
                Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
                Pfn1->u1.WsIndex = SwapEntry;
                return;
            }

            //
            // Change the parent of the entry to point to the swap entry.
            //

            if (Wsle[Parent].u2.s.RightChild == Entry) {
                Wsle[Parent].u2.s.RightChild = SwapEntry;
            } else {
                Wsle[Parent].u2.s.LeftChild = SwapEntry;
            }

            return;

        }

        //
        // The SwapEntry is not the root, find its parent.
        //

        if (Wsle[SwapEntry].u2.BothPointers == 0) {

            //
            // Entry is not in tree, therefore no parent.

            SwapParent = WSLE_NULL_INDEX;

        } else {

            VirtualAddress = PAGE_ALIGN(Wsle[SwapEntry].u1.VirtualAddress);

            for (;;) {

                ASSERT (SwapParent != WSLE_NULL_INDEX);

                if (Wsle[SwapParent].u2.s.LeftChild == SwapEntry) {
                    break;
                }
                if (Wsle[SwapParent].u2.s.RightChild == SwapEntry) {
                    break;
                }


                if (VirtualAddress < PAGE_ALIGN(Wsle[SwapParent].u1.VirtualAddress)) {
                    SwapParent = Wsle[SwapParent].u2.s.LeftChild;
                } else {
                    SwapParent = Wsle[SwapParent].u2.s.RightChild;
                }
            }
        }

        if (Parent == WorkingSetList->Root) {

            //
            // The entry is at the root.
            //

            if (Wsle[Entry].u2.s.LeftChild == SwapEntry) {

                //
                // The entry we are going to swap is the left child of this
                // entry.
                //
                //              R(Entry)
                //             / \
                //  (SwapEntry)
                //

                WorkingSetList->Root = SwapEntry;

                Wsle[Entry].u2.s.LeftChild = Wsle[SwapEntry].u2.s.LeftChild;
                Wsle[SwapEntry].u2.s.LeftChild = Entry;
                SavedRight = Wsle[Entry].u2.s.RightChild;
                Wsle[Entry].u2.s.RightChild = Wsle[SwapEntry].u2.s.RightChild;
                Wsle[SwapEntry].u2.s.RightChild = SavedRight;

                SavedLong = Wsle[Entry].u1.Long;
                Wsle[Entry].u1.Long = Wsle[SwapEntry].u1.Long;
                Wsle[SwapEntry].u1.Long = SavedLong;

                return;

            } else if (Wsle[SwapEntry].u2.s.RightChild == Entry) {

                //
                // The entry we are going to swap is the right child of this
                // entry.
                //
                //              R(SwapEntry)
                //             / \
                //                (entry)
                //

                WorkingSetList->Root = Entry;

                Wsle[SwapEntry].u2.s.RightChild = Wsle[Entry].u2.s.RightChild;
                Wsle[Entry].u2.s.RightChild = SwapEntry;
                SavedLeft = Wsle[SwapEntry].u2.s.LeftChild;
                Wsle[SwapEntry].u2.s.LeftChild = Wsle[Entry].u2.s.LeftChild;
                Wsle[Entry].u2.s.LeftChild = SavedLeft;


                SavedLong = Wsle[Entry].u1.Long;
                Wsle[Entry].u1.Long = Wsle[SwapEntry].u1.Long;
                Wsle[SwapEntry].u1.Long = SavedLong;

                return;
            }

            //
            // The swap entry is the root, but the other entry is not
            // its child.
            //
            //
            //              R(SwapEntry)
            //             / \
            //            .....
            //                 Parent(Entry)
            //                  \
            //                   Entry (left or right)
            //
            //

            WorkingSetList->Root = Entry;

            SavedRight = Wsle[SwapEntry].u2.s.RightChild;
            Wsle[SwapEntry].u2.s.RightChild = Wsle[Entry].u2.s.RightChild;
            Wsle[Entry].u2.s.RightChild = SavedRight;
            SavedLeft = Wsle[SwapEntry].u2.s.LeftChild;
            Wsle[SwapEntry].u2.s.LeftChild = Wsle[Entry].u2.s.LeftChild;
            Wsle[Entry].u2.s.LeftChild = SavedLeft;

            SavedLong = Wsle[Entry].u1.Long;
            Wsle[Entry].u1.Long = Wsle[SwapEntry].u1.Long;
            Wsle[SwapEntry].u1.Long = SavedLong;

            if (SwapParent == WSLE_NULL_INDEX) {

                //
                // This entry is not in the tree.
                //

                PointerPte = MiGetPteAddress (Wsle[Entry].u1.VirtualAddress);
                Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
                ASSERT (Pfn1->u1.WsIndex == SwapEntry);
                Pfn1->u1.WsIndex = Entry;
                return;
            }

            //
            // Change the parent of the entry to point to the swap entry.
            //

            if (Wsle[SwapParent].u2.s.RightChild == SwapEntry) {
                Wsle[SwapParent].u2.s.RightChild = Entry;
            } else {
                Wsle[SwapParent].u2.s.LeftChild = Entry;
            }

            return;

        }

        //
        // Neither entry is the root.
        //

        if (Parent == SwapEntry) {

            //
            // The parent of the entry is the swap entry.
            //
            //
            //              R
            //            .....
            //
            //              (SwapParent)
            //              |
            //              (SwapEntry)
            //              |
            //              (Entry)
            //

            //
            // Update the parent pointer for the swapentry.
            //

            if (Wsle[SwapParent].u2.s.LeftChild == SwapEntry) {
                Wsle[SwapParent].u2.s.LeftChild = Entry;
            } else {
                Wsle[SwapParent].u2.s.RightChild = Entry;
            }

            //
            // Determine if this goes left or right.
            //

            if (Wsle[SwapEntry].u2.s.LeftChild == Entry) {

                //
                // The entry we are going to swap is the left child of this
                // entry.
                //
                //              R
                //            .....
                //
                //             (SwapParent)
                //
                //             (SwapEntry)  [Parent(entry)]
                //            / \
                //     (entry)
                //

                Wsle[SwapEntry].u2.s.LeftChild = Wsle[Entry].u2.s.LeftChild;
                Wsle[Entry].u2.s.LeftChild = SwapEntry;
                SavedRight = Wsle[SwapEntry].u2.s.RightChild;
                Wsle[SwapEntry].u2.s.RightChild = Wsle[Entry].u2.s.RightChild;
                Wsle[Entry].u2.s.RightChild = SavedRight;

                SavedLong = Wsle[Entry].u1.Long;
                Wsle[Entry].u1.Long = Wsle[SwapEntry].u1.Long;
                Wsle[SwapEntry].u1.Long = SavedLong;

                return;

            } else {

                ASSERT (Wsle[SwapEntry].u2.s.RightChild == Entry);

                //
                // The entry we are going to swap is the right child of this
                // entry.
                //
                //              R
                //            .....
                //
                //              (SwapParent)
                //               \
                //                (SwapEntry)
                //               / \
                //                  (entry)
                //

                Wsle[SwapEntry].u2.s.RightChild = Wsle[Entry].u2.s.RightChild;
                Wsle[Entry].u2.s.RightChild = SwapEntry;
                SavedLeft = Wsle[SwapEntry].u2.s.LeftChild;
                Wsle[SwapEntry].u2.s.LeftChild = Wsle[Entry].u2.s.LeftChild;
                Wsle[Entry].u2.s.LeftChild = SavedLeft;


                SavedLong = Wsle[Entry].u1.Long;
                Wsle[Entry].u1.Long = Wsle[SwapEntry].u1.Long;
                Wsle[SwapEntry].u1.Long = SavedLong;

                return;
            }


        }
        if (SwapParent == Entry) {


            //
            // The parent of the swap entry is the entry.
            //
            //              R
            //            .....
            //
            //              (Parent)
            //              |
            //              (Entry)
            //              |
            //              (SwapEntry)
            //

            //
            // Update the parent pointer for the entry.
            //

            if (Wsle[Parent].u2.s.LeftChild == Entry) {
                Wsle[Parent].u2.s.LeftChild = SwapEntry;
            } else {
                Wsle[Parent].u2.s.RightChild = SwapEntry;
            }

            //
            // Determine if this goes left or right.
            //

            if (Wsle[Entry].u2.s.LeftChild == SwapEntry) {

                //
                // The entry we are going to swap is the left child of this
                // entry.
                //
                //              R
                //            .....
                //
                //              (Parent)
                //              |
                //              (Entry)
                //              /
                //   (SwapEntry)
                //

                Wsle[Entry].u2.s.LeftChild = Wsle[SwapEntry].u2.s.LeftChild;
                Wsle[SwapEntry].u2.s.LeftChild = Entry;
                SavedRight = Wsle[Entry].u2.s.RightChild;
                Wsle[Entry].u2.s.RightChild = Wsle[SwapEntry].u2.s.RightChild;
                Wsle[SwapEntry].u2.s.RightChild = SavedRight;

                SavedLong = Wsle[Entry].u1.Long;
                Wsle[Entry].u1.Long = Wsle[SwapEntry].u1.Long;
                Wsle[SwapEntry].u1.Long = SavedLong;

                return;

            } else {

                ASSERT (Wsle[Entry].u2.s.RightChild == SwapEntry);

                //
                // The entry we are going to swap is the right child of this
                // entry.
                //
                //              R(Entry)
                //             / \
                //                (SwapEntry)
                //

                Wsle[Entry].u2.s.RightChild = Wsle[SwapEntry].u2.s.RightChild;
                Wsle[SwapEntry].u2.s.RightChild = Entry;
                SavedLeft = Wsle[SwapEntry].u2.s.LeftChild;
                Wsle[SwapEntry].u2.s.LeftChild = Wsle[Entry].u2.s.LeftChild;
                Wsle[Entry].u2.s.LeftChild = SavedLeft;

                SavedLong = Wsle[Entry].u1.Long;
                Wsle[Entry].u1.Long = Wsle[SwapEntry].u1.Long;
                Wsle[SwapEntry].u1.Long = SavedLong;

                return;
            }

        }

        //
        // Neither entry is the parent of the other.  Just swap them
        // and update the parent entries.
        //

        if (Parent == WSLE_NULL_INDEX) {

            //
            // This entry is not in the tree.
            //

            PointerPte = MiGetPteAddress (Wsle[Entry].u1.VirtualAddress);
            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
            ASSERT (Pfn1->u1.WsIndex == Entry);
            Pfn1->u1.WsIndex = SwapEntry;

        } else {

            if (Wsle[Parent].u2.s.LeftChild == Entry) {
                Wsle[Parent].u2.s.LeftChild = SwapEntry;
            } else {
                Wsle[Parent].u2.s.RightChild = SwapEntry;
            }
        }

        if (SwapParent == WSLE_NULL_INDEX) {

            //
            // This entry is not in the tree.
            //

            PointerPte = MiGetPteAddress (Wsle[SwapEntry].u1.VirtualAddress);
            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
            ASSERT (Pfn1->u1.WsIndex == SwapEntry);
            Pfn1->u1.WsIndex = Entry;
        } else {

            if (Wsle[SwapParent].u2.s.LeftChild == SwapEntry) {
                Wsle[SwapParent].u2.s.LeftChild = Entry;
            } else {
                Wsle[SwapParent].u2.s.RightChild = Entry;
            }
        }

        SavedRight = Wsle[SwapEntry].u2.s.RightChild;
        Wsle[SwapEntry].u2.s.RightChild = Wsle[Entry].u2.s.RightChild;
        Wsle[Entry].u2.s.RightChild = SavedRight;
        SavedLeft = Wsle[SwapEntry].u2.s.LeftChild;
        Wsle[SwapEntry].u2.s.LeftChild = Wsle[Entry].u2.s.LeftChild;
        Wsle[Entry].u2.s.LeftChild = SavedLeft;

        SavedLong = Wsle[Entry].u1.Long;
        Wsle[Entry].u1.Long = Wsle[SwapEntry].u1.Long;
        Wsle[SwapEntry].u1.Long = SavedLong;

        return;
    }
}
#endif //0
