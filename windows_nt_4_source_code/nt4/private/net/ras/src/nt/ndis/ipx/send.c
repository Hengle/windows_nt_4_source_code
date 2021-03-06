/*******************************************************************/
/*	      Copyright(c)  1993 Microsoft Corporation		   */
/*******************************************************************/

//***
//
// Filename:	send.c
//
// Description: send packet routines
//
// Author:	Stefan Solomon (stefans)    October 11, 1993.
//
// Revision History:
//
//***

#include    "rtdefs.h"

VOID
SendPacketComplete(PPACKET_TAG	    pktp);

UINT
SendPropagatedPacketComplete(PPACKET_TAG	pktp);

//***
//
// Function:	SendPacket
//
// Descr:	enqueues the packet in the requested Nic queue and initiates
//		the send
//
// Params:	Packet
//
// Returns:	none
//
//***

VOID
SendPacket(PPACKET_TAG		    pktp)
{
    PNDIS_PACKET	pktdescrp;
    NDIS_STATUS 	NdisStatus;
    UINT		BufferCount;
    PNDIS_BUFFER	FirstBufferp;
    USHORT		pktlen;
    PNICCB		niccbp;
    PUCHAR		hdrp;

    RtPrint(DBG_SEND, ("IpxRouter: SendPacket: pktp=0x%x\n", pktp));

    // get the packet length
    hdrp = pktp->DataBufferp;
    GETSHORT2USHORT(&pktlen, hdrp + IPXH_LENGTH);

    // get the pkt descr ptr
    pktdescrp = CONTAINING_RECORD(pktp, NDIS_PACKET, ProtocolReserved);

    // adjust the length of the packet to send
    NdisQueryPacket(pktdescrp,
		    NULL,
		    &BufferCount,
		    &FirstBufferp,
		    NULL);

    NdisAdjustBufferLength(FirstBufferp, pktlen);

    // chain the mac hdr buff descr at the front (the mac hdr buff descr
    // already points at the Mac header in the packet tag)
    NdisChainBufferAtFront(pktdescrp, pktp->HeaderBuffDescrp);

    // get the Nic Cb ptr where we should send this packet
    niccbp = NicCbPtrTab[pktp->RemoteAddress.NicId];

    // Do not send if it doesn't fit the max frame size
    // for this adapter
    if(pktlen > niccbp->MaximumPacketSize) {

	SendPacketComplete(pktp);
	return;
    }


    ACQUIRE_SPIN_LOCK(&niccbp->NicLock);

    // check Nic Status. Do not send on a closed Nic
    if(niccbp->NicState != NIC_ACTIVE) {

	RELEASE_SPIN_LOCK(&niccbp->NicLock);

	SendPacketComplete(pktp);
	return;
    }

    // Nic is active, we can send
    // set the QUEUE owner of the packet
    pktp->QueueOwnerNicCbp = niccbp;

    // enqueue the packet in the NIC's send list and wait for the
    // transfer to complete
    InsertTailList(&niccbp->SendQueue, &pktp->PacketLinkage);

    RELEASE_SPIN_LOCK(&niccbp->NicLock);

    // send the packet
    NdisStatus = IpxSendPacket(&pktp->RemoteAddress,
			       pktdescrp,
			       pktlen,
			       0);

    if(NdisStatus != NDIS_STATUS_PENDING) {

	RtSendComplete(pktdescrp, NdisStatus);
    }
}

//***
//
// Function:	RtSendComplete
//
// Descr:	called by the IPX driver when send completed
//
// Params:
//
// Returns:
//
//***

VOID
RtSendComplete(PNDIS_PACKET	pktdescrp,
	       NDIS_STATUS	NdisStatus)
{
    PNICCB	    niccbp;
    PPACKET_TAG     pktp;

    pktp = (PPACKET_TAG)pktdescrp->ProtocolReserved;

    RtPrint(DBG_SEND, ("IpxRouter: RtSendComplete: pktp=0x%x\n", pktp));

    niccbp = pktp->QueueOwnerNicCbp;

#if DBG
    // this is for debugging purposes
    if(NdisStatus != NDIS_STATUS_SUCCESS) {

	RtPrint(DBG_SEND, ("IpxRouter: SendPacket: FAILED with %xl\n", NdisStatus));
    }
#endif

    // dequeue the packet from the send queue and complete processing
    ACQUIRE_SPIN_LOCK(&niccbp->NicLock);

    RemoveEntryList(&pktp->PacketLinkage);

    RELEASE_SPIN_LOCK(&niccbp->NicLock);

    // complete the send packet processing
    SendPacketComplete(pktp);
}

//***
//
// Function:	SendPacketComplete
//
// Descr:	post send processing
//
// Params:	Packet
//
// Returns:	None
//
//***

VOID
SendPacketComplete(PPACKET_TAG	    pktp)
{
    PNDIS_PACKET    NdisPacket;
    PNDIS_BUFFER    NdisBuffer = NULL;
    UINT	    BufferCount;

    RtPrint(DBG_SEND, ("IpxRouter: SendPacketComplete: Entered\n"));

    // unchain the first buffer descriptor (MacHeader)
    NdisPacket = CONTAINING_RECORD(pktp, NDIS_PACKET, ProtocolReserved);
    NdisUnchainBufferAtFront (NdisPacket, &NdisBuffer);
    ASSERT(NdisBuffer == pktp->HeaderBuffDescrp);

    // readjust the original buffer descriptor length
    // get the pkt descr ptr
    NdisQueryPacket(NdisPacket,
		    NULL,
		    &BufferCount,
		    &NdisBuffer,
		    NULL);

    NdisAdjustBufferLength(NdisBuffer, pktp->DataBufferLength);

    // what to do next is controlled by the packet type
    switch(pktp->PacketType) {

	case RCV_PACKET:

	    // this has been a routed packet or a directed rip reply
	    // free it to the rcv pkt pool and discharge the nic

	    FreeRcvPkt(pktp);

	    break;

	case RIP_SEND_PACKET:

	    // call the completion routines for these guys
	    SendRipPktCompleted(pktp);

	    break;

	case PROPAGATED_BCAST_PACKET:

	    // send the packet on the next available net (if any left) or return it
	    // to the pool
	    if(SendPropagatedPacketComplete(pktp)) {

		RtPrint(DBG_NETBIOS, ("IpxRouter: SendPacketComplete: free propagated pkt 0x%x\n", pktp));

		// can't propagate further, return to rcv pkt pool
		FreeRcvPkt(pktp);
	    }

	    break;

	default:

	    // !!! break
	    ASSERT(FALSE);
	    break;
    }
}

//***
//
// Function:	SendPropagatedPacket
//
// Descr:	marks the packet as a propagated packet type and sends it
//		starting at the beginning of the LanNics list
//
// Params:	Packet
//
// Returns:	none
//
//***

VOID
SendPropagatedPacket(PPACKET_TAG	pktp)
{
    USHORT	    i;
    PNICCB	    niccbp;

    // check if there is an active Nic to send on next and if this Nic is
    // active and is not the packet owner nic
    for(i=0; i<MaximumNicCount; i++) {

	niccbp = NicCbPtrTab[i];

	if((niccbp->NicState == NIC_ACTIVE) &&
	   (niccbp->DeviceType != NdisMediumWan) &&
	   (!IsNetInNbPacket(pktp, niccbp))) {

	    // send the packet on this nic
	    pktp->PacketType = PROPAGATED_BCAST_PACKET;
	    pktp->RemoteAddress.NicId = niccbp->NicId;
	    memcpy(pktp->RemoteAddress.MacAddress, bcastaddress, 6);

	    RtPrint(DBG_NETBIOS, ("IpxRouter: SendPropagatedPacket: send pkt 0x%x on Nic %d\n", pktp, niccbp->NicId));

	    SendPacket(pktp);

	    return;
	}
    }

    // can't propagate this packet, free it
    FreeRcvPkt(pktp);
}

//***
//
// Function:	SendPropagatedPacketComplete
//
// Descr:	If there is a next active NicCb to send the packet on, queues
//		the packet in the propagation list and queues a Dpc to send the
//		packet on this Nic.
//
// Params:	Packet
//
// Returns:	0 - packet queued for propagation, 1 - can't propagate further
//
//***

UINT
SendPropagatedPacketComplete(PPACKET_TAG	pktp)
{
    PNICCB	    niccbp;  // nic that has sent the packet
    USHORT	    i;

    // check if there is another active Nic to send on next and if this Nic is
    // active and is not the packet owner nic
    for(i=pktp->RemoteAddress.NicId+1; // next Nic Id
	i<MaximumNicCount;
	i++) {

	niccbp = NicCbPtrTab[i];

	if((niccbp->NicState == NIC_ACTIVE) &&
	   (niccbp->DeviceType != NdisMediumWan) &&
	   (!IsNetInNbPacket(pktp, niccbp))) {

	    // We have found the next nic.
	    pktp->RemoteAddress.NicId = i;

	    // queue the packet for a propagated send.
	    ACQUIRE_SPIN_LOCK(&PropagatedPktsListLock);

	    InsertTailList(&PropagatedPktsList, &pktp->PacketLinkage);

	    RELEASE_SPIN_LOCK(&PropagatedPktsListLock);

	    // queue a Dpc to send the packet
	    KeInsertQueueDpc(&PropagatedPktsDpc, NULL, NULL);

	    return 0;
	}
    }

    // we are done with this packet
    return 1;
}

//***
//
// Function:	SendNextPropagatedPacket
//
// Descr:	DPC called routine which dequeues the next packet from the
//		propagated packets list and sends it
//
// Params:	none
//
// Returns:	none
//
//***


VOID
SendNextPropagatedPkt(PKDPC		Dpc,
			PVOID		DefferedContext,
			PVOID		SystemArgument1,
			PVOID		SystemArgument2)
{
    PLIST_ENTRY     lep;
    PPACKET_TAG     pktp;
    LIST_ENTRY	    sendlist;

    InitializeListHead(&sendlist);

    // get next item from the propagated bcast queue and send it

    ACQUIRE_SPIN_LOCK(&PropagatedPktsListLock);

    while(!IsListEmpty(&PropagatedPktsList)) {

	lep = RemoveHeadList(&PropagatedPktsList);
	InsertTailList(&sendlist, lep);
    }

    RELEASE_SPIN_LOCK(&PropagatedPktsListLock);

    while(!IsListEmpty(&sendlist)) {

	lep = RemoveHeadList(&sendlist);
	pktp = CONTAINING_RECORD(lep, PACKET_TAG, PacketLinkage);
	SendPacket(pktp);
    }
}
