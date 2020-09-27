/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    adapter.c

Abstract:

    This module contains code which implements the ADAPTER object.
    Routines are provided to reference, and dereference transport
    adapter objects.

Environment:

    Kernel mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


//
// These are init only until binding is really dynamic.
//
#ifndef	_PNP_POWER
#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,IpxCreateAdapter)
#endif
#endif	_PNP_POWER

//
// [FW] So, later we can change this to pnp-compatible value
//

//
// ULONG
// ADAPTER_INDEX_TO_FWCONTEXT(
//    IN ULONG _adapterindex;
// );
//

#define ADAPTER_INDEX_TO_FWCONTEXT(_adapterindex) _adapterindex


VOID
IpxRefBinding(
    IN PBINDING Binding
    )

/*++

Routine Description:

    This routine increments the reference count on a device context.

Arguments:

    Binding - Pointer to a transport device context object.

Return Value:

    none.

--*/

{
    CTEAssert (Binding->ReferenceCount > 0);    // not perfect, but...

    (VOID)InterlockedIncrement (&Binding->ReferenceCount);

}   /* IpxRefBinding */


VOID
IpxDerefBinding(
    IN PBINDING Binding
    )

/*++

Routine Description:

    This routine dereferences a device context by decrementing the
    reference count contained in the structure.  Currently, we don't
    do anything special when the reference count drops to zero, but
    we could dynamically unload stuff then.

Arguments:

    Binding - Pointer to a transport device context object.

Return Value:

    none.

--*/

{
    LONG result;

    result = InterlockedDecrement (&Binding->ReferenceCount);

    CTEAssert (result >= 0);

    if (result == 0) {
        IpxDestroyBinding (Binding);
    }

}   /* IpxDerefBinding */


NTSTATUS
IpxCreateAdapter(
    IN PDEVICE Device,
    IN PUNICODE_STRING AdapterName,
    IN OUT PADAPTER *AdapterPtr
    )

/*++

Routine Description:

    This routine creates and initializes a device context structure.

Arguments:


    DriverObject - pointer to the IO subsystem supplied driver object.

    Adapter - Pointer to a pointer to a transport device context object.

    AdapterName - pointer to the name of the device this device object points to.

Return Value:

    STATUS_SUCCESS if all is well; STATUS_INSUFFICIENT_RESOURCES otherwise.

--*/

{
    PADAPTER Adapter;
#if 0
    UINT i, j;
#endif

    Adapter = (PADAPTER)IpxAllocateMemory (sizeof(ADAPTER) + AdapterName->Length + sizeof(WCHAR), MEMORY_ADAPTER, "Adapter");

#ifdef	_PNP_POWER
    if (Adapter == NULL) {
		if (KeGetCurrentIrql() == 0) {
			IPX_DEBUG (ADAPTER, ("Create adapter %ws failed\n", AdapterName));
		} else {
			IPX_DEBUG (ADAPTER, ("Create adapter %lx failed\n", AdapterName));
		}
		return STATUS_INSUFFICIENT_RESOURCES;
	}

    IPX_DEBUG (ADAPTER, ("Create adapter %lx %lx succeeded\n", Adapter, AdapterName));
#else
    if (Adapter == NULL) {
        IPX_DEBUG (ADAPTER, ("Create adapter %ws failed\n", AdapterName->Buffer));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    IPX_DEBUG (ADAPTER, ("Create adapter %ws succeeded\n", AdapterName->Buffer));
#endif

    RtlZeroMemory(Adapter, sizeof(ADAPTER));

    //
    // Copy over the adapter name.
    //

    Adapter->AdapterNameLength = AdapterName->Length + sizeof(WCHAR);
    Adapter->AdapterName = (PWCHAR)(Adapter+1);
    RtlCopyMemory(
        Adapter->AdapterName,
        AdapterName->Buffer,
        AdapterName->Length);
    Adapter->AdapterName[AdapterName->Length/sizeof(WCHAR)] = UNICODE_NULL;


#if DBG
    RtlCopyMemory(Adapter->Signature1, "IAD1", 4);
#endif

    Adapter->Type = IPX_ADAPTER_SIGNATURE;
    Adapter->Size = sizeof(ADAPTER);

    CTEInitLock (&Adapter->Lock);

    InitializeListHead (&Adapter->RequestCompletionQueue);

    InitializeListHead (&Adapter->ReceiveBufferPoolList);

    ExInitializeSListHead (&Adapter->ReceiveBufferList);

    Adapter->Device = Device;
    Adapter->DeviceLock = &Device->Lock;
    IpxReferenceDevice (Device, DREF_ADAPTER);

#if 0
    Adapter->ReceiveBufferPool.Next = NULL;
    for (i = 0; i < ISN_FRAME_TYPE_MAX; i++) {
        Adapter->Bindings[i] = NULL;
    }
    Adapter->BindingCount = 0;

    for (i = 0; i < IDENTIFIER_TOTAL; i++) {
        for (j = 0; j < SOURCE_ROUTE_HASH_SIZE; j++) {
            Adapter->SourceRoutingHeads[i][j] = (PSOURCE_ROUTE)NULL;
        }
    }
#endif

    //
    // BUGBUG: For the moment, we have to do the source
    // routing operation on any type where broadcast
    // may not be used for discovery -- improve this
    // hopefully.
    //

    Adapter->SourceRoutingEmpty[IDENTIFIER_RIP] = FALSE;
    Adapter->SourceRoutingEmpty[IDENTIFIER_IPX] = FALSE;
    Adapter->SourceRoutingEmpty[IDENTIFIER_SPX] = FALSE;
    Adapter->SourceRoutingEmpty[IDENTIFIER_NB] = TRUE;

#ifdef	_PNP_POWER
	//
	// Lock here? [BUGBUGZZ]
	//
	Adapter->ReferenceCount = 1;
#endif

    *AdapterPtr = Adapter;

    return STATUS_SUCCESS;

}   /* IpxCreateAdapter */


VOID
IpxDestroyAdapter(
    IN PADAPTER Adapter
    )

/*++

Routine Description:

    This routine destroys a device context structure.

Arguments:

    Adapter - Pointer to a pointer to a transport device context object.

Return Value:

    None.

--*/

{
    ULONG Database, Hash;
    PSOURCE_ROUTE Current;
    ULONG ReceiveBufferPoolSize;
    PIPX_RECEIVE_BUFFER ReceiveBuffer;
    PIPX_RECEIVE_BUFFER_POOL ReceiveBufferPool;
    PDEVICE Device = Adapter->Device;
    PLIST_ENTRY p;
    UINT i;

    IPX_DEBUG (ADAPTER, ("Destroy adapter %lx\n", Adapter));

    //
    // Free any receive buffer pools this adapter has.
    //

    ReceiveBufferPoolSize = FIELD_OFFSET (IPX_RECEIVE_BUFFER_POOL, Buffers[0]) +
                       (sizeof(IPX_RECEIVE_BUFFER) * Device->InitReceiveBuffers) +
                       (Adapter->MaxReceivePacketSize * Device->InitReceiveBuffers);

    while (!IsListEmpty (&Adapter->ReceiveBufferPoolList)) {

        p = RemoveHeadList (&Adapter->ReceiveBufferPoolList);
        ReceiveBufferPool = CONTAINING_RECORD (p, IPX_RECEIVE_BUFFER_POOL, Linkage);

        for (i = 0; i < ReceiveBufferPool->BufferCount; i++) {

            ReceiveBuffer = &ReceiveBufferPool->Buffers[i];
            IpxDeinitializeReceiveBuffer (Adapter, ReceiveBuffer, Adapter->MaxReceivePacketSize);

        }

        IPX_DEBUG (PACKET, ("Free buffer pool %lx\n", ReceiveBufferPool));
        IpxFreeMemory (ReceiveBufferPool, ReceiveBufferPoolSize, MEMORY_PACKET, "ReceiveBufferPool");
    }

    //
    // Free all the source routing information for this adapter.
    //

    for (Database = 0; Database < IDENTIFIER_TOTAL; Database++) {

        for (Hash = 0; Hash < SOURCE_ROUTE_HASH_SIZE; Hash++) {

            while (Adapter->SourceRoutingHeads[Database][Hash]) {

                Current = Adapter->SourceRoutingHeads[Database][Hash];
                Adapter->SourceRoutingHeads[Database][Hash] = Current->Next;

                IpxFreeMemory (Current, SOURCE_ROUTE_SIZE (Current->SourceRoutingLength), MEMORY_SOURCE_ROUTE, "SourceRouting");
            }
        }
    }

    IpxDereferenceDevice (Adapter->Device, DREF_ADAPTER);
    IpxFreeMemory (Adapter, sizeof(ADAPTER) + Adapter->AdapterNameLength, MEMORY_ADAPTER, "Adapter");

}   /* IpxDestroyAdapter */


NTSTATUS
IpxCreateBinding(
    IN PDEVICE Device,
    IN PBINDING_CONFIG ConfigBinding OPTIONAL,
    IN ULONG NetworkNumberIndex,
    IN PWCHAR AdapterName,
    IN OUT PBINDING *BindingPtr
    )

/*++

Routine Description:

    This routine creates and initializes a binding structure.

Arguments:

    Device - The device.

    ConfigBinding - Information about this binding. If this is
        NULL then this is a WAN binding and all the relevant
        information will be filled in by the caller.

    NetworkNumberIndex - The index in the frame type array for
        ConfigBinding indicating which frame type this binding is for.
        Not used if ConfigBinding is not provided.

    AdapterName - Used for error logging.

    BindingPtr - Returns the allocated binding structure.

Return Value:

    STATUS_SUCCESS if all is well; STATUS_INSUFFICIENT_RESOURCES otherwise.

--*/

{
    PBINDING Binding;
#ifdef  _PNP_POWER
    PSINGLE_LIST_ENTRY s;

    s = IPX_POP_ENTRY_LIST(
            &Device->BindingList,
            &Device->SListsLock);

    if (s != NULL) {
         goto GotBinding;
    }

    //
    // This function tries to allocate another packet pool.
    //

    s = IpxPopBinding(Device);

    //
    // Possibly we should queue the packet up to wait
    // for one to become free.
    //

    if (s == NULL) {

#if DBG
        if (KeGetCurrentIrql() == 0) {
            IPX_DEBUG (ADAPTER, ("Create binding %ws failed\n", AdapterName));
        } else {
            IPX_DEBUG (ADAPTER, ("Create binding WAN failed\n"));
        }
#endif
        return STATUS_INSUFFICIENT_RESOURCES;
    }

GotBinding:

    Binding = CONTAINING_RECORD (s, BINDING, PoolLinkage);

#else
    Binding = (PBINDING)IpxAllocateMemory (sizeof(BINDING), MEMORY_ADAPTER, "Binding");

    //
    // We can't vsprintf a %ws at DPC level, so we check for
    // that. Only WAN bindings will be created then.
    //

    if (Binding == NULL) {
#if DBG
        if (KeGetCurrentIrql() == 0) {
            IPX_DEBUG (ADAPTER, ("Create binding %ws failed\n", AdapterName));
        } else {
            IPX_DEBUG (ADAPTER, ("Create binding WAN failed\n"));
        }
#endif
        return STATUS_INSUFFICIENT_RESOURCES;
    }
#endif

#if DBG
    if (KeGetCurrentIrql() == 0) {
        IPX_DEBUG (ADAPTER, ("Create binding %ws succeeded, %lx\n", AdapterName, Binding));
    } else {
        IPX_DEBUG (ADAPTER, ("Create binding WAN succeeded\n"));
    }
#endif

    RtlZeroMemory(Binding, sizeof(BINDING));

    //
    // Initialize the reference count.
    //

    Binding->ReferenceCount = 1;
#if DBG
    Binding->RefTypes[BREF_BOUND] = 1;
#endif

#if DBG
    RtlCopyMemory(Binding->Signature1, "IBI1", 4);
#endif

    Binding->Type = IPX_BINDING_SIGNATURE;
    Binding->Size = sizeof(BINDING);

    Binding->Device = Device;
    Binding->DeviceLock = &Device->Lock;

    if (ConfigBinding != NULL) {

        ULONG Temp = ConfigBinding->NetworkNumber[NetworkNumberIndex];
        Binding->ConfiguredNetworkNumber = REORDER_ULONG (Temp);

        Binding->AutoDetect = ConfigBinding->AutoDetect[NetworkNumberIndex];
        Binding->DefaultAutoDetect = ConfigBinding->DefaultAutoDetect[NetworkNumberIndex];

        Binding->AllRouteDirected = (BOOLEAN)ConfigBinding->Parameters[BINDING_ALL_ROUTE_DEF];
        Binding->AllRouteBroadcast = (BOOLEAN)ConfigBinding->Parameters[BINDING_ALL_ROUTE_BC];
        Binding->AllRouteMulticast = (BOOLEAN)ConfigBinding->Parameters[BINDING_ALL_ROUTE_MC];

    }

    Binding->ReceiveBroadcast = TRUE;
#if 0
    Binding->BindingSetMember = FALSE;
    Binding->NextBinding = (PBINDING)NULL;
    Binding->DialOutAsync = FALSE;
#endif

    //
    // We set Binding->FrameType later, after we can map it based on the
    // media type of the adapter we bind to.
    //

    *BindingPtr = Binding;

    return STATUS_SUCCESS;

}   /* IpxCreateBinding */


VOID
IpxDestroyBinding(
    IN PBINDING Binding
    )

/*++

Routine Description:

    This routine destroys a binding structure.

Arguments:

    Binding - Pointer to a transport binding structure.

Return Value:

    None.

--*/

{
    IPX_DEBUG (ADAPTER, ("Destroy binding %lx\n", Binding));

#ifdef  _PNP_POWER

    IPX_PUSH_ENTRY_LIST(
        &IpxDevice->BindingList,
        &Binding->PoolLinkage,
        &IpxDevice->SListsLock);
#else
    IpxFreeMemory (Binding, sizeof(BINDING), MEMORY_ADAPTER, "Binding");
#endif

}   /* IpxDestroyBinding */


#ifdef  _PNP_POWER
VOID
IpxAllocateBindingPool(
    IN PDEVICE Device
    )

/*++

Routine Description:

    This routine adds 10 bindings to the pool for this device.

Arguments:

    Device - The device.

Return Value:

    None.

--*/

{
    PIPX_BINDING_POOL BindingPool;
    UINT BindingPoolSize;
    UINT BindingNum;
    PBINDING Binding;
    CTELockHandle LockHandle;

    BindingPoolSize = FIELD_OFFSET (IPX_BINDING_POOL, Bindings[0]) +
                       (sizeof(BINDING) * Device->InitBindings);

    BindingPool = (PIPX_BINDING_POOL)IpxAllocateMemory (BindingPoolSize, MEMORY_PACKET, "BindingPool");

    if (BindingPool == NULL) {
        IPX_DEBUG (PNP, ("Could not allocate binding pool memory\n"));
        return;
    }


    IPX_DEBUG (PNP, ("Initializing Binding pool %lx, %d bindings\n",
                             BindingPool, Device->InitBindings));

    BindingPool->BindingCount = Device->InitBindings;

    CTEGetLock (&Device->Lock, &LockHandle);

    for (BindingNum = 0; BindingNum < BindingPool->BindingCount; BindingNum++) {

        Binding = &BindingPool->Bindings[BindingNum];
        IPX_PUSH_ENTRY_LIST (&Device->BindingList, &Binding->PoolLinkage, &Device->SListsLock);

#ifdef IPX_TRACK_POOL
        Binding->Pool = BindingPool;
#endif
    }

    InsertTailList (&Device->BindingPoolList, &BindingPool->Linkage);

    Device->AllocatedBindings += BindingPool->BindingCount;

    CTEFreeLock (&Device->Lock, LockHandle);

}   /* IpxAllocateBindingPool */


PSINGLE_LIST_ENTRY
IpxPopBinding(
    PDEVICE Device
    )

/*++

Routine Description:

    This routine allocates a binding from the device context's pool.
    If there are no bindings in the pool, it allocates one up to
    the configured limit.

Arguments:

    Device - Pointer to our device to charge the packet to.

Return Value:

    The pointer to the Linkage field in the allocated binding.

--*/

{
    PSINGLE_LIST_ENTRY s;

    s = IPX_POP_ENTRY_LIST(
            &Device->BindingList,
            &Device->SListsLock);

    if (s != NULL) {
        return s;
    }

    //
    // No packets in the pool, see if we can allocate more.
    //

    if (Device->AllocatedBindings < Device->MaxPoolBindings) {

        //
        // Allocate a pool and try again.
        //

        IpxAllocateBindingPool (Device);
        s = IPX_POP_ENTRY_LIST(
                &Device->BindingList,
                &Device->SListsLock);

        return s;

    } else {

        return NULL;

    }

}   /* IpxPopBinding */
#endif

//
// [FW]
//

NTSTATUS
IpxOpenAdapter(
   IN    NIC_HANDLE  AdapterIndex1,
   IN    ULONG       FwdAdapterContext,
   OUT   PNIC_HANDLE IpxAdapterContext
   )

/*++

Routine Description:

   This routine is called by the Kernel Forwarder to open an adapter

Arguments:

   AdapterIndex - index of the adapter to open (NICid for now - will change to a struct
                  with a version number, signature and the NicId
   FwdAdapterContext - Forwarder's context
   IpxAdapterContext - our context (for now we use the NICid - for pnp will change
                       this to contain a signature and version #)

Return Value:

   STATUS_INVALID_HANDLE   if the AdapterIndex handle was invalid
   STATUS_ADAPTER_ALREADY_OPENED    if the Adapter is being opened a second time
   STATUS_SUCCESS

--*/

{
   PBINDING Binding;
   PDEVICE  Device = IpxDevice;
   USHORT   AdapterIndex = AdapterIndex1.NicId;

#if DBG
   DbgPrint ("IPX: Entered IpxOpenAdapter\n");
#endif

   //
   // Return error if the AdapterIndex is out of range.
   // We do indicate the slave bindings to NB/SPX (but not to RIP)
   // Hence, the index should be less than  HighestExternalNicId (not ValidBindings)
   //

   if (AdapterIndex > Device->HighestExternalNicId) {
      return STATUS_INVALID_HANDLE;
   }


   //
   // Fill up our context to be returned to the Forwarder
   //
   NIC_HANDLE_FROM_NIC((*IpxAdapterContext), AdapterIndex);

   //
   // If AdapterIndex is 0, it is for the virtual net
   // BUGBUG: Will the forwarder open this at all?
   //

   if (AdapterIndex == 0) {
      return STATUS_SUCCESS;
   }

   //
   // Get the binding pointer
   //

   Binding = NIC_ID_TO_BINDING(IpxDevice, AdapterIndex);

   //
   // Return error if adapter is being opened a second time (or more times)
   //

   if (GET_VALUE(Binding->ReferenceCount) >= 2) {
      return STATUS_ADAPTER_ALREADY_OPENED;
   }

   //
   // Store the Forwarder's Adapter Context in the binding
   //

   Binding->FwdAdapterContext = FwdAdapterContext;

   //
   // Reference the Binding
   //

   IpxReferenceBinding(Binding, BREF_FWDOPEN);

   return STATUS_SUCCESS;

}

NTSTATUS
IpxCloseAdapter(
   IN NIC_HANDLE  IpxAdapterContext
   )

/*++

Routine Description:

   This routine is called by the Kernel Forwarder to close an adapter

Arguments:

   IpxAdapterContext - our context (for now we use the NICid - for pnp will change
                       this to contain a signature and version#)

Return Value:

   STATUS_ADAPTER_ALREADY_CLOSED - if the adapter is being closed a second time
   STATUS_SUCCESS

--*/

{

   PBINDING Binding;

#if DBG
   DbgPrint ("IPX: Entered IpxCloseAdapter\n");
#endif

   Binding = NIC_ID_TO_BINDING(IpxDevice, IpxAdapterContext.NicId);

   //
   // Either the adapter is around (count = 2)
   // or it went away (count = 1). The latter cannot happen now.
   //

   if (GET_VALUE(Binding->ReferenceCount) <= 1) {
      return STATUS_ADAPTER_ALREADY_CLOSED;
   }

   //
   // Dereference the Binding so it can be deleted
   //

   IpxDereferenceBinding(Binding, BREF_FWDOPEN);


   //
   // Clear the Forwarder's Adapter Context in the binding
   //

   Binding->FwdAdapterContext = 0;

   return STATUS_SUCCESS;
}

