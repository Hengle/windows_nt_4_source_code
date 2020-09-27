#include <wanhelp.h>

PSTR MediumType[] =
{
	"NdisMedium802_3",
	"NdisMedium802_5",
	"NdisMediumFddi",
	"NdisMediumWan",
	"NdisMediumLocalTalk",
	"NdisMediumDix",
	"NdisMediumArcnetRaw",
	"NdisMediumArcnet878_2",
	"NdisMediumAtm",
	"NdisMediumWirelessWan",
	"NdisMediumIrda"
};

PSTR WanMediumSubtype[] =
{
    "NdisWanMediumHub",
    "NdisWanMediumX_25",
    "NdisWanMediumIsdn",
    "NdisWanMediumSerial",
    "NdisWanMediumFrameRelay",
    "NdisWanMediumAtm",
    "NdisWanMediumSonet",
    "NdisWanMediumSW56K"
};

PSTR WanHeaderFormat[] =
{
	"NdisWanHeaderNative",       // src/dest based on subtype, followed by NLPID
	"NdisWanHeaderEthernet"      // emulation of ethernet header
};

PSTR HardwareStatus[] =
{
    "NdisHardwareStatusReady",
    "NdisHardwareStatusInitializing",
    "NdisHardwareStatusReset",
    "NdisHardwareStatusClosing",
    "NdisHardwareStatusNotReady"
};

PSTR LinkCBStates[] =
{
	"LinkDown",
	"LinkGoingDown",
	"LinkUp"
};

PSTR BundleCBStates[] =
{
	"BundleDown",
	"BundleGowingDown",
	"BundleUp",
	"BundleRouted",
	"BundleUnrouting"
};

PSTR WanQuality[] =
{
	"NdisWanRaw",
	"NdisWanErrorControl",
	"NdisWanReliable"
};

PSTR DeferredQueueDesc[] =
{
	"ReceiveIndication",
	"SendComplete",
	"StatusIndication",
	"Loopback"
};

VOID
DisplayNdisWanCB(
	DWORD	Address,
	PNDISWANCB	NdisWanCB
	)
{
	dprintf("NdisWanCB: 0x%8.8x\n\n", Address);
	dprintf("   Lock: 0x%8.8x Irql: 0x%8.8x\n", NdisWanCB->Lock.SpinLock, NdisWanCB->Lock.OldIrql);
	dprintf("   NdisWrapperHandle: 0x%8.8x\n", NdisWanCB->hNdisWrapperHandle);
	dprintf("   ProtocolHandle: 0x%8.8x\n", NdisWanCB->hProtocolHandle);
	dprintf("   NumberOfProtocols: %ld\n", NdisWanCB->ulNumberOfProtocols);
	dprintf("   NumberOfLinks: %ld\n", NdisWanCB->ulNumberOfLinks);
	dprintf("   MiniumFragmentSize: 0x%8.8x\n", NdisWanCB->ulMinFragmentSize);
	dprintf("   TraceLevel: 0x%8.8x\n", NdisWanCB->ulTraceLevel);
	dprintf("   TraceMask: 0x%8.8x\n", NdisWanCB->ulTraceMask);
	dprintf("   DriverObject: 0x%8.8x\n", NdisWanCB->pDriverObject);
	dprintf("   DeviceObject: 0x%8.8x\n", NdisWanCB->pDeviceObject);
	dprintf("   PacketsSent: %ld\n", NdisWanCB->SendCount);
	dprintf("   PacketsCompleted: %ld\n", NdisWanCB->SendCompleteCount);
	dprintf("   IORecvQueueEmpty: %ld\n", NdisWanCB->IORecvError1);
	dprintf("   IORecvBeforMap: %ld\n", NdisWanCB->IORecvError2);
	dprintf("   PromiscuousAdapter: 0x%8.8x\n",NdisWanCB->PromiscuousAdapter);
}

VOID
DisplayWanAdapterCB(
	DWORD	Address,
	PWAN_ADAPTERCB	WanAdapterCB
	)
{
	dprintf("\n\nWanAdapterCB: 0x%8.8x\n", Address);

	dprintf("   Linkage:\n");
	dprintf("      Flink: 0x%8.8x Blink: 0x%8.8x\n",
			WanAdapterCB->Linkage.Flink, WanAdapterCB->Linkage.Blink);

	dprintf("   Lock: 0x%8.8x Irql: 0x%8.8x\n",
			WanAdapterCB->Lock.SpinLock, WanAdapterCB->Lock.OldIrql);

	dprintf("   FreeLinkCBList:\n");
	dprintf("      Flink: 0x%8.8x Blink: 0x%8.8x\n",
			WanAdapterCB->FreeLinkCBList.Flink, WanAdapterCB->FreeLinkCBList.Blink);

	dprintf("   NdisBindingHandle: 0x%8.8x\n", WanAdapterCB->hNdisBindingHandle);

	dprintf("   WanRequest: 0x%8.8x\n", WanAdapterCB->pWanRequest);
	dprintf("   LastRequest: 0x%8.8x\n", WanAdapterCB->pLastWanRequest);

	dprintf("   MiniportName: Buffer: 0x%8.8x Length: %ld\n",
			WanAdapterCB->MiniportName.Buffer, WanAdapterCB->MiniportName.Length);

	dprintf("   MediumType: %s\n", MediumType[WanAdapterCB->MediumType]);

	dprintf("   MediumSubType: %s\n", WanMediumSubtype[WanAdapterCB->MediumSubType]);

	dprintf("   WanHeaderFormat: %s\n", WanHeaderFormat[WanAdapterCB->WanHeaderFormat]);

	dprintf("   MaxFrameSize: %ld\n", WanAdapterCB->WanInfo.MaxFrameSize);

	dprintf("   MaxTransmit: %ld\n", WanAdapterCB->WanInfo.MaxTransmit);

	dprintf("   HeaderPadding: %ld\n",WanAdapterCB->WanInfo.HeaderPadding);

	dprintf("   TailPadding: %ld\n",WanAdapterCB->WanInfo.TailPadding);

	dprintf("   Endpoints: %ld\n",WanAdapterCB->WanInfo.Endpoints);

	dprintf("   MemoryFlags: 0x%8.8x\n",WanAdapterCB->WanInfo.MemoryFlags);

	dprintf("   HighestAddress: 0x%8.8x 0x%8.8x\n",
				WanAdapterCB->WanInfo.HighestAcceptableAddress.HighPart,
				WanAdapterCB->WanInfo.HighestAcceptableAddress.LowPart);

	dprintf("   FramingBits: 0x%8.8x\n",WanAdapterCB->WanInfo.FramingBits);

	dprintf("   DesiredACCM: 0x%8.8x\n",WanAdapterCB->WanInfo.DesiredACCM);
}

VOID
DisplayAdapterCB(
	DWORD	Address,
	PADAPTERCB	AdapterCB
	)
{
	DWORD	i;

	dprintf("\n\nAdapterCB: 0x%8.8x\n", Address);

	dprintf("   Linkage:\n");
	dprintf("      Flink: 0x%8.8x Blink: 0x%8.8x\n",
			AdapterCB->Linkage.Flink, AdapterCB->Linkage.Blink);

	dprintf("   Lock: 0x%8.8x Irql: 0x%8.8x\n",
			AdapterCB->Lock.SpinLock, AdapterCB->Lock.OldIrql);

	dprintf("   ReferenceCount: %ld\n", AdapterCB->ulReferenceCount);

	dprintf("   MiniportAdapterHandle: 0x%8.8x\n", AdapterCB->hMiniportHandle);

	dprintf("   Flags: 0x%8.8x\n", AdapterCB->Flags);

	dprintf("   FreeDeferredQueue:\n");
	dprintf("      Head: 0x%8.8x\n", AdapterCB->FreeDeferredQueue.Head);
	dprintf("      Tail: 0x%8.8x\n", AdapterCB->FreeDeferredQueue.Tail);
	dprintf("      Count: %ld\n", AdapterCB->FreeDeferredQueue.Count);
	dprintf("      MaxCount: %ld\n", AdapterCB->FreeDeferredQueue.MaxCount);
	for (i = 0; i < MAX_DEFERRED_QUEUE_TYPES; i++) {
		dprintf("   DeferredQueue: %s\n", DeferredQueueDesc[i]);
		dprintf("      Head: 0x%8.8x\n", AdapterCB->DeferredQueue[i].Head);
		dprintf("      Tail: 0x%8.8x\n", AdapterCB->DeferredQueue[i].Tail);
		dprintf("      Count: %ld\n", AdapterCB->DeferredQueue[i].Count);
		dprintf("      MaxCount: %ld\n", AdapterCB->DeferredQueue[i].MaxCount);
	}

	dprintf("   MediumType: %s\n", MediumType[AdapterCB->MediumType]);

	dprintf("   HardwareStatus: %s\n", HardwareStatus[AdapterCB->HardwareStatus]);

	dprintf("   AdapterName: Buffer: 0x%8.8x Length: %d\n",
			AdapterCB->AdapterName.Buffer, AdapterCB->AdapterName.Length);

	dprintf("   NetworkAddress: 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x\n",
			AdapterCB->NetworkAddress[0], AdapterCB->NetworkAddress[1], AdapterCB->NetworkAddress[2],
			AdapterCB->NetworkAddress[3], AdapterCB->NetworkAddress[4], AdapterCB->NetworkAddress[5]);

	dprintf("   NumberOfProtocols: %ld\n", AdapterCB->ulNumberofProtocols);

	dprintf("   ProtocolType: 0x%4.4x\n", AdapterCB->ProtocolType);

	dprintf("   NbfBundleCB: 0x%8.8x\n", AdapterCB->NbfBundleCB);
	dprintf("   NbfProtocolHandle: 0x%8.8x\n", AdapterCB->NbfProtocolHandle);

}

VOID
DisplayConnectionTable(
	DWORD	Address,
	PCONNECTION_TABLE	ConnectionTable
	)
{
	dprintf("\n\nConnectionTable: 0x%8.8x\n", Address);
	dprintf("   Lock: 0x%8.8x Irql: 0x%8.8x\n",
			ConnectionTable->Lock.SpinLock, ConnectionTable->Lock.OldIrql);
	dprintf("   AllocationSize: %ld\n", ConnectionTable->ulAllocationSize);
	dprintf("   ArraySize: %ld\n", ConnectionTable->ulArraySize);
	dprintf("   Number Of Active Links: %ld\n", ConnectionTable->ulNumActiveLinks);
	dprintf("   Number Of Active Bundles: %ld\n", ConnectionTable->ulNumActiveBundles);
	dprintf("   LinkArray: 0x%8.8x\n", ConnectionTable->LinkArray);
	dprintf("   BundleArray: 0x%8.8x\n", ConnectionTable->BundleArray);
}

VOID
DisplayLinkCB(
	DWORD	Address,
	PLINKCB	LinkCB
	)
{
	dprintf("\n\nLinkCB: 0x%8.8x\n", Address);
	dprintf("   Linkage:\n");
	dprintf("      Flink: 0x%8.8x Blink: 0x%8.8x\n",
	LinkCB->Linkage.Flink, LinkCB->Linkage.Blink);
	dprintf("   Handle: 0x%8.8x\n", LinkCB->hLinkHandle);
	dprintf("   User Context: 0x%8.8x\n", LinkCB->hLinkContext);
	dprintf("   ReferenceCount: %ld\n", LinkCB->ulReferenceCount);
	dprintf("   State: %s\n", LinkCBStates[LinkCB->State]);
	dprintf("   WanAdapterCB: 0x%8.8x\n", LinkCB->WanAdapterCB);
	dprintf("   BundleCB: 0x%8.8x\n", LinkCB->BundleCB);
	dprintf("   WanMiniport Lineup Context: 0x%8.8x\n", LinkCB->NdisLinkHandle);
	dprintf("   WanPacketPool:\n");
	dprintf("      Flink: 0x%8.8x Blink: 0x%8.8x\n",
	LinkCB->WanPacketPool.Flink, LinkCB->WanPacketPool.Blink);
	dprintf("   WanPacketCount: %ld\n", LinkCB->ulWanPacketCount);
	dprintf("   OutstandingFrames: %ld\n", LinkCB->OutstandingFrames);
	dprintf("   LinkBandwidth: %ld\n", LinkCB->ulBandwidth);
	dprintf("   LinkInfo:\n");
	dprintf("      MaxSendFrameSize: %ld\n", LinkCB->LinkInfo.MaxSendFrameSize);
	dprintf("      MaxRecvFrameSize: %ld\n", LinkCB->LinkInfo.MaxRecvFrameSize);
	dprintf("      HeaderPadding: %ld\n", LinkCB->LinkInfo.HeaderPadding);
	dprintf("      TailPadding: %ld\n", LinkCB->LinkInfo.TailPadding);
	dprintf("      SendFramingBits: 0x%8.8x\n", LinkCB->LinkInfo.SendFramingBits);
	dprintf("      RecvFramingBits: 0x%8.8x\n", LinkCB->LinkInfo.RecvFramingBits);
	dprintf("      SendCompressionBits: 0x%8.8x\n", LinkCB->LinkInfo.SendCompressionBits);
	dprintf("      RecvCompressionBits: 0x%8.8x\n", LinkCB->LinkInfo.RecvCompressionBits);
	dprintf("      SendACCM: 0x%8.8x\n", LinkCB->LinkInfo.SendACCM);
	dprintf("      RecvACCM: 0x%8.8x\n", LinkCB->LinkInfo.RecvACCM);
	dprintf("      MaxRSendFrameSize: %ld\n", LinkCB->LinkInfo.MaxRSendFrameSize);
	dprintf("      MaxRRecvFrameSize: %ld\n", LinkCB->LinkInfo.MaxRRecvFrameSize);
	dprintf("   LineUpInfo:\n");
	dprintf("      LinkSpeed: %ld\n", LinkCB->LineUpInfo.LinkSpeed);
	dprintf("      Quality: 0x%8.8x\n", LinkCB->LineUpInfo.Quality);
	dprintf("      SendWindow: %d\n", LinkCB->LineUpInfo.SendWindow);
	dprintf("      ConnectionWrapperID: 0x%8.8x\n", LinkCB->LineUpInfo.ConnectionWrapperID);
	dprintf("      NdisLinkHandle: 0x%8.8x\n", LinkCB->LineUpInfo.NdisLinkHandle);
	dprintf("      NdisLinkContext: 0x%8.8x\n", LinkCB->LineUpInfo.NdisLinkContext);
	dprintf("   FriendlyName: %s\n", LinkCB->Name);
	dprintf("   LinkStats:\n");
	dprintf("      BytesTransmitted: %ld\n", LinkCB->LinkStats.BytesTransmitted);
	dprintf("      BytesReceived: %ld\n", LinkCB->LinkStats.BytesReceived);
	dprintf("      FramesTransmitted: %ld\n", LinkCB->LinkStats.FramesTransmitted);
	dprintf("      FramesReceived: %ld\n", LinkCB->LinkStats.FramesReceived);

	dprintf("      CRCErrors: %ld\n", LinkCB->LinkStats.CRCErrors);
	dprintf("      TimeoutErrors: %ld\n", LinkCB->LinkStats.TimeoutErrors);
	dprintf("      AlignmentErrors: %ld\n", LinkCB->LinkStats.AlignmentErrors);
	dprintf("      SerialOverrunErrors: %ld\n", LinkCB->LinkStats.SerialOverrunErrors);
	dprintf("      FramingErrors: %ld\n", LinkCB->LinkStats.FramingErrors);
	dprintf("      BufferOverrunErrors: %ld\n", LinkCB->LinkStats.BufferOverrunErrors);
	dprintf("      ByteTransmittedUncompressed: %ld\n", LinkCB->LinkStats.BytesTransmittedUncompressed);
	dprintf("      BytesReceivedUncompressed: %ld\n", LinkCB->LinkStats.BytesReceivedUncompressed);
	dprintf("      BytesTransmittedCompressed: %ld\n", LinkCB->LinkStats.BytesTransmittedCompressed);
	dprintf("      BytesReceivedCompressed: %ld\n", LinkCB->LinkStats.BytesReceivedCompressed);
}

VOID
DisplayBundleCB(
	DWORD	Address,
	PBUNDLECB	BundleCB
	)
{
	DWORD	i;

	dprintf("\n\nBundleCB: 0x%8.8x\n", Address);
	dprintf("   Handle: 0x%8.8x\n",BundleCB->hBundleHandle);
	dprintf("   ReferenceCount: %ld\n",BundleCB->ulReferenceCount);
	dprintf("   State: %s\n",BundleCBStates[BundleCB->State]);
	dprintf("   LinkCBList:\n");
	dprintf("      Flink: 0x%8.8x Blink: 0x%8.8x\n",
	BundleCB->LinkCBList.Flink, BundleCB->LinkCBList.Blink);
	dprintf("   LinkCBCount: %ld\n",BundleCB->ulLinkCBCount);
	dprintf("   FramingInfo:\n");
	dprintf("      SendFramingBits: 0x%8.8x\n", BundleCB->FramingInfo.SendFramingBits);
	dprintf("      RecvFramingBits: 0x%8.8x\n", BundleCB->FramingInfo.RecvFramingBits);
	dprintf("      MaxRSendFrameSize: %ld\n", BundleCB->FramingInfo.MaxRSendFrameSize);
	dprintf("      MaxRRecvFrameSize: %ld\n", BundleCB->FramingInfo.MaxRRecvFrameSize);
	dprintf("   NextLinkToXmit: 0x%8.8x\n",BundleCB->NextLinkToXmit);
	dprintf("   SendingLinks: %d\n", BundleCB->SendingLinks);
	dprintf("   SendPacketQueue:\n");
	dprintf("      Flink: 0x%8.8x Blink: 0x%8.8x\n",
	BundleCB->SendPacketQueue.Flink, BundleCB->SendPacketQueue.Blink);
	dprintf("   SendSequenceNumber: 0x%8.8x\n", BundleCB->SendSeqNumber);
	dprintf("   MaxSendSequenceNumber: 0x%8.8x\n", BundleCB->MaxSendSeqNumber);
	dprintf("   SendSequenceMask: 0x%8.8x\n", BundleCB->SendSeqMask);
	dprintf("   Flags: 0x%8.8x\n",BundleCB->Flags);
	dprintf("   OutstandingFrames: %ld\n",BundleCB->OutstandingFrames);
	dprintf("   RecvDescPool:\n");
	dprintf("      Flink: 0x%8.8x Blink: 0x%8.8x\n",
	BundleCB->RecvDescPool.Flink, BundleCB->RecvDescPool.Blink);
	dprintf("   RecvDesc Count: %ld\n", BundleCB->RecvDescCount);
	dprintf("   RecvDesc Max Count: %ld\n", BundleCB->RecvDescMax);
	dprintf("   RecvDescAssemblyList:\n");
	dprintf("      Flink: 0x%8.8x Blink: 0x%8.8x\n",
	BundleCB->RecvDescAssemblyList.Flink, BundleCB->RecvDescAssemblyList.Blink);
	dprintf("   RecvDescHole: 0x%8.8x\n", BundleCB->RecvDescHole);
	dprintf("   MaxRecvSeqNumber: 0x%8.8x\n", BundleCB->MaxRecvSeqNumber);
	dprintf("   RecvSeqMask: 0x%8.8x\n", BundleCB->RecvSeqMask);
	dprintf("   TimeToLive: %ld\n", BundleCB->TimeToLive);
	dprintf("   RecvFragmentsLost: %ld\n", BundleCB->RecvFragmentsLost);
	dprintf("   ProtocolCBTable: 0x%8.8x\n",BundleCB->ProtocolCBTable);
	dprintf("   Number Of Routes: %ld\n",BundleCB->ulNumberOfRoutes);
	dprintf("   ProtocolCBList:\n");
	dprintf("      Flink: 0x%8.8x Blink: 0x%8.8x\n",
	BundleCB->ProtocolCBList.Flink, BundleCB->ProtocolCBList.Blink);
	dprintf("   SendMask: 0x%8.8x\n",BundleCB->SendMask);
	dprintf("   LineUpInfo:\n");
	dprintf("      BundleSpeed: %ld\n", BundleCB->LineUpInfo.BundleSpeed);
	dprintf("      MaxSendSize: %ld\n", BundleCB->LineUpInfo.ulMaximumTotalSize);
	dprintf("      LinkQuality: %s\n", WanQuality[BundleCB->LineUpInfo.Quality]);
	dprintf("      SendWindow: %d\n", BundleCB->LineUpInfo.usSendWindow);
	dprintf("   SendVJInfo:\n");
	dprintf("      IPCompressionProtocol: 0x%4.4x\n", BundleCB->SendVJInfo.IPCompressionProtocol);
	dprintf("      MaxSlotID: %d\n", BundleCB->SendVJInfo.MaxSlotID);
	dprintf("      CompSlotID: %d\n", BundleCB->SendVJInfo.CompSlotID);
	dprintf("      VJCompress: 0x%8.8x\n", BundleCB->SendVJCompress);
	dprintf("   RecvVJInfo:\n");
	dprintf("      IPCompressionProtocol: 0x%4.4x\n", BundleCB->RecvVJInfo.IPCompressionProtocol);
	dprintf("      MaxSlotID: %d\n", BundleCB->RecvVJInfo.MaxSlotID);
	dprintf("      CompSlotID: %d\n", BundleCB->RecvVJInfo.CompSlotID);
	dprintf("      VJCompress: 0x%8.8x\n", BundleCB->RecvVJCompress);
	dprintf("   SendCompInfo:\n");
	dprintf("      SessionKey: 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x\n",
	BundleCB->SendCompInfo.SessionKey[0],BundleCB->SendCompInfo.SessionKey[1],
	BundleCB->SendCompInfo.SessionKey[2],BundleCB->SendCompInfo.SessionKey[3],
	BundleCB->SendCompInfo.SessionKey[4],BundleCB->SendCompInfo.SessionKey[5],
	BundleCB->SendCompInfo.SessionKey[6],BundleCB->SendCompInfo.SessionKey[7]);
	dprintf("      MSCompType: 0x%8.8x\n", BundleCB->SendCompInfo.MSCompType);
	dprintf("   SendCompressContext: 0x%8.8x\n", BundleCB->SendCompressContext);
	dprintf("   RecvCompInfo:\n");
	dprintf("      SessionKey: 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x\n",
	BundleCB->RecvCompInfo.SessionKey[0],BundleCB->RecvCompInfo.SessionKey[1],
	BundleCB->RecvCompInfo.SessionKey[2],BundleCB->RecvCompInfo.SessionKey[3],
	BundleCB->RecvCompInfo.SessionKey[4],BundleCB->RecvCompInfo.SessionKey[5],
	BundleCB->RecvCompInfo.SessionKey[6],BundleCB->RecvCompInfo.SessionKey[7]);
	dprintf("      MSCompType: 0x%8.8x\n", BundleCB->RecvCompInfo.MSCompType);
	dprintf("   RecvCompressContext: 0x%8.8x\n", BundleCB->RecvCompressContext);
	dprintf("   SendRC4Key: 0x%8.8x\n", BundleCB->SendRC4Key);
	dprintf("   RecvRC4Key: 0x%8.8x\n", BundleCB->RecvRC4Key);
	dprintf("   SCoherencyCounter: 0x%4.4x\n", BundleCB->SCoherencyCounter);
	dprintf("   RCoherencyCounter: 0x%4.4x\n", BundleCB->RCoherencyCounter);
	dprintf("   LastRC4Reset: 0x%4.4x\n", BundleCB->LastRC4Reset);
	dprintf("   CCPIdentifier: 0x%4.4x\n", BundleCB->CCPIdentifier);
	dprintf("   UpperBonDInfo:\n");
	dprintf("      BytesThreshold: %ld\n", BundleCB->UpperBonDInfo.ulBytesThreshold);
	dprintf("      PercentBandwidth: %d\n", BundleCB->UpperBonDInfo.usPercentBandwidth);
	dprintf("      SecondsInSamplePeriod: %ld\n", BundleCB->UpperBonDInfo.ulSecondsInSamplePeriod);
	dprintf("      State: 0x%8.8x\n", BundleCB->UpperBonDInfo.State);
	dprintf("      StartTime: 0x%8.8x%8.8x\n",
	BundleCB->UpperBonDInfo.StartTime.HighPart, BundleCB->UpperBonDInfo.StartTime.LowPart);
	dprintf("      SampleTable:\n");
	dprintf("         FirstIndex: %ld\n", BundleCB->UpperBonDInfo.SampleTable.ulFirstIndex);
	dprintf("         CurrentIndex: %ld\n", BundleCB->UpperBonDInfo.SampleTable.ulCurrentIndex);
	dprintf("         CurrentSampleByteCount: %ld\n", BundleCB->UpperBonDInfo.SampleTable.ulCurrentSampleByteCount);
	dprintf("         SampleArraySize: %ld\n", BundleCB->UpperBonDInfo.SampleTable.ulSampleArraySize);
	dprintf("         SampleRate: 0x%8.8x%8.8x\n",
	BundleCB->UpperBonDInfo.SampleTable.SampleRate.HighPart, BundleCB->UpperBonDInfo.SampleTable.SampleRate.LowPart);
	dprintf("         SamplePeriod: 0x%8.8x%8.8x\n",
	BundleCB->UpperBonDInfo.SampleTable.SamplePeriod.HighPart, BundleCB->UpperBonDInfo.SampleTable.SamplePeriod.LowPart);
	dprintf("         SampleTable:\n");
	for (i = 0; i < SAMPLE_ARRAY_SIZE; i++) {
		dprintf("         Sample %d:\n", i);
		dprintf("            BytesThisSend: %ld\n",BundleCB->UpperBonDInfo.SampleTable.SampleArray[i].ulBytesThisSend);
		dprintf("            ReferenceCount: %ld\n",BundleCB->UpperBonDInfo.SampleTable.SampleArray[i].ulReferenceCount);
		dprintf("            TimeStample: 0x%8.8x%8.8x\n",
		BundleCB->UpperBonDInfo.SampleTable.SampleArray[i].TimeStamp.HighPart, BundleCB->UpperBonDInfo.SampleTable.SampleArray[i].TimeStamp.LowPart);
	}
	dprintf("   LowerBonDInfo:\n");
	dprintf("      BytesThreshold: %ld\n", BundleCB->LowerBonDInfo.ulBytesThreshold);
	dprintf("      PercentBandwidth: %d\n", BundleCB->LowerBonDInfo.usPercentBandwidth);
	dprintf("      SecondsInSamplePeriod: %ld\n", BundleCB->LowerBonDInfo.ulSecondsInSamplePeriod);
	dprintf("      State: 0x%8.8x\n", BundleCB->LowerBonDInfo.State);
	dprintf("      StartTime: 0x%8.8x%8.8x\n",
	BundleCB->LowerBonDInfo.StartTime.HighPart, BundleCB->LowerBonDInfo.StartTime.LowPart);
	dprintf("      SampleTable:\n");
	dprintf("         FirstIndex: %ld\n", BundleCB->LowerBonDInfo.SampleTable.ulFirstIndex);
	dprintf("         CurrentIndex: %ld\n", BundleCB->LowerBonDInfo.SampleTable.ulCurrentIndex);
	dprintf("         CurrentSampleByteCount: %ld\n", BundleCB->LowerBonDInfo.SampleTable.ulCurrentSampleByteCount);
	dprintf("         SampleArraySize: %ld\n", BundleCB->LowerBonDInfo.SampleTable.ulSampleArraySize);
	dprintf("         SampleRate: 0x%8.8x%8.8x\n",
	BundleCB->LowerBonDInfo.SampleTable.SampleRate.HighPart, BundleCB->LowerBonDInfo.SampleTable.SampleRate.LowPart);
	dprintf("         SamplePeriod: 0x%8.8x%8.8x\n",
	BundleCB->LowerBonDInfo.SampleTable.SamplePeriod.HighPart, BundleCB->LowerBonDInfo.SampleTable.SamplePeriod.LowPart);
	dprintf("         SampleArray:\n");
	for (i = 0; i < SAMPLE_ARRAY_SIZE; i++) {
		dprintf("         Sample %d:\n", i);
		dprintf("            BytesThisSend: %ld\n",BundleCB->LowerBonDInfo.SampleTable.SampleArray[i].ulBytesThisSend);
		dprintf("            ReferenceCount: %ld\n",BundleCB->LowerBonDInfo.SampleTable.SampleArray[i].ulReferenceCount);
		dprintf("            TimeStample: 0x%8.8x%8.8x\n",
		BundleCB->LowerBonDInfo.SampleTable.SampleArray[i].TimeStamp.HighPart, BundleCB->LowerBonDInfo.SampleTable.SampleArray[i].TimeStamp.LowPart);
	}
	dprintf("   FriendlyName: %s\n", BundleCB->Name);
	dprintf("   BundleStats:\n");
	dprintf("      BytesTransmitted: %ld\n", BundleCB->BundleStats.BytesTransmitted);
	dprintf("      BytesReceived: %ld\n", BundleCB->BundleStats.BytesReceived);
	dprintf("      FramesTransmitted: %ld\n", BundleCB->BundleStats.FramesTransmitted);
	dprintf("      FramesReceived: %ld\n", BundleCB->BundleStats.FramesReceived);

	dprintf("      CRCErrors: %ld\n", BundleCB->BundleStats.CRCErrors);
	dprintf("      TimeoutErrors: %ld\n", BundleCB->BundleStats.TimeoutErrors);
	dprintf("      AlignmentErrors: %ld\n", BundleCB->BundleStats.AlignmentErrors);
	dprintf("      SerialOverrunErrors: %ld\n", BundleCB->BundleStats.SerialOverrunErrors);
	dprintf("      FramingErrors: %ld\n", BundleCB->BundleStats.FramingErrors);
	dprintf("      BufferOverrunErrors: %ld\n", BundleCB->BundleStats.BufferOverrunErrors);
	dprintf("      ByteTransmittedUncompressed: %ld\n", BundleCB->BundleStats.BytesTransmittedUncompressed);
	dprintf("      BytesReceivedUncompressed: %ld\n", BundleCB->BundleStats.BytesReceivedUncompressed);
	dprintf("      BytesTransmittedCompressed: %ld\n", BundleCB->BundleStats.BytesTransmittedCompressed);
	dprintf("      BytesReceivedCompressed: %ld\n", BundleCB->BundleStats.BytesReceivedCompressed);
}

VOID
DisplayProtocolCB(
	DWORD	Address,
	PPROTOCOLCB	ProtocolCB
	)
{
	DWORD	i;

	dprintf("\n\nProtocolCB: 0x%8.8x\n", Address);
	dprintf("   Linkage:\n");
	dprintf("      Flink: 0x%8.8x Blink: 0x%8.8x\n",
	ProtocolCB->Linkage.Flink, ProtocolCB->Linkage.Blink);
	dprintf("   ProtocolHandle: 0x%8.8x\n", ProtocolCB->hProtocolHandle);
	dprintf("   ReferenceCount: %ld\n", ProtocolCB->ulReferenceCount);
	dprintf("   HeadNdisPacketQueue: 0x%8.8x\n", ProtocolCB->HeadNdisPacketQueue);
	dprintf("   TailNdisPacketQueue: 0x%8.8x\n", ProtocolCB->TailNdisPacketQueue);
	dprintf("   SendMaskBit: 0x%8.8x\n", ProtocolCB->SendMaskBit);
	dprintf("   AdapterCB: 0x%8.8x\n", ProtocolCB->AdapterCB);
	dprintf("   BundleCB: 0x%8.8x\n", ProtocolCB->BundleCB);
	dprintf("   Flags: 0x%8.8x\n", ProtocolCB->Flags);
	dprintf("   ProtocolType: 0x%4.4x\n", ProtocolCB->usProtocolType);
	dprintf("   PPP ProtocolID: 0x%4.4x\n", ProtocolCB->usPPPProtocolID);
	dprintf("   Priority: %ld\n", ProtocolCB->usPriority);
	dprintf("   Bytes Quota: %ld\n", ProtocolCB->ulByteQuota);
	dprintf("      SampleTable:\n");
	dprintf("         FirstIndex: %ld\n", ProtocolCB->SampleTable.ulFirstIndex);
	dprintf("         CurrentIndex: %ld\n", ProtocolCB->SampleTable.ulCurrentIndex);
	dprintf("         CurrentSampleByteCount: %ld\n", ProtocolCB->SampleTable.ulCurrentSampleByteCount);
	dprintf("         SampleArraySize: %ld\n", ProtocolCB->SampleTable.ulSampleArraySize);
	dprintf("         SampleRate: 0x%8.8x%8.8x\n",
	ProtocolCB->SampleTable.SampleRate.HighPart, ProtocolCB->SampleTable.SampleRate.LowPart);
	dprintf("         SamplePeriod: 0x%8.8x%8.8x\n",
	ProtocolCB->SampleTable.SamplePeriod.HighPart, ProtocolCB->SampleTable.SamplePeriod.LowPart);
	dprintf("         SampleArray:\n");
	for (i = 0; i < SAMPLE_ARRAY_SIZE; i++) {
		dprintf("         Sample %d:\n", i);
		dprintf("            BytesThisSend: %ld\n",ProtocolCB->SampleTable.SampleArray[i].ulBytesThisSend);
		dprintf("            ReferenceCount: %ld\n",ProtocolCB->SampleTable.SampleArray[i].ulReferenceCount);
		dprintf("            TimeStample: 0x%8.8x%8.8x\n",
		ProtocolCB->SampleTable.SampleArray[i].TimeStamp.HighPart, ProtocolCB->SampleTable.SampleArray[i].TimeStamp.LowPart);
	}
	dprintf("   TransportHandle: 0x%8.8x\n", ProtocolCB->hTransportHandle);
	dprintf("   NdisWanAddress: 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x\n",
			ProtocolCB->NdisWanAddress[0], ProtocolCB->NdisWanAddress[1], ProtocolCB->NdisWanAddress[2],
			ProtocolCB->NdisWanAddress[3], ProtocolCB->NdisWanAddress[4], ProtocolCB->NdisWanAddress[5]);
	dprintf("   TransportAddress: 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x\n",
			ProtocolCB->TransportAddress[0], ProtocolCB->TransportAddress[1], ProtocolCB->TransportAddress[2],
			ProtocolCB->TransportAddress[3], ProtocolCB->TransportAddress[4], ProtocolCB->TransportAddress[5]);
	dprintf("   DeviceName: Buffer: 0x%8.8x Length: %ld\n",
			ProtocolCB->DeviceName.Buffer, ProtocolCB->DeviceName.Length);
}

VOID
DisplayWanPacket(
	DWORD	Address,
	PNDIS_WAN_PACKET	Packet
	)
{

	dprintf("\n\nWanPacket: 0x%8.8x\n", Address);
	dprintf("   WanPacketQueue:\n");
	dprintf("      Flink: 0x%8.8x Blink: 0x%8.8x\n",
	Packet->WanPacketQueue.Flink, Packet->WanPacketQueue.Blink);
	dprintf("   CurrentBuffer: 0x%8.8x\n", Packet->CurrentBuffer);
	dprintf("   CurrentLength: %ld\n", Packet->CurrentLength);
	dprintf("   StartBuffer: 0x%8.8x\n", Packet->StartBuffer);
	dprintf("   EndBuffer: 0x%8.8x\n", Packet->EndBuffer);
	dprintf("   PR1 (LinkCB): 0x%8.8x\n", Packet->ProtocolReserved1);
	dprintf("   PR2 (NdisPacket): 0x%8.8x\n", Packet->ProtocolReserved2);
	dprintf("   PR3 (ProtocolCB): 0x%8.8x\n", Packet->ProtocolReserved3);
	dprintf("   PR4 (BytesSent): %ld\n", Packet->ProtocolReserved4);
	dprintf("   MR1: 0x%8.8x\n", Packet->MacReserved1);
	dprintf("   MR2: 0x%8.8x\n", Packet->MacReserved2);
	dprintf("   MR3: 0x%8.8x\n", Packet->MacReserved3);
	dprintf("   MR4: 0x%8.8x\n", Packet->MacReserved4);
}

VOID
DisplayNdisPacket(
	DWORD	Address,
	PNDIS_PACKET	Packet
	)
{

	dprintf("\n\nNdisPacket: 0x%8.8x\n", Address);
	dprintf("   Private:\n");
	dprintf("      PhysicalCount: 0x%8.8x\n", Packet->Private.PhysicalCount);
	dprintf("      TotalLength: %ld\n", Packet->Private.TotalLength);
	dprintf("      BufferHead: 0x%8.8x\n", Packet->Private.Head);
	dprintf("      BufferTail: 0x%8.8x\n", Packet->Private.Tail);
	dprintf("      Pool: 0x%8.8x\n", Packet->Private.Pool);
	dprintf("      Count: 0x%8.8x\n", Packet->Private.Count);
	dprintf("      Flags: 0x%8.8x\n", Packet->Private.Flags);
	dprintf("      ValidCounts: %d\n", Packet->Private.ValidCounts);
	dprintf("   MR1 (Next/MagicNumber): 0x%8.8x\n", *((PDWORD)&Packet->MiniportReserved[0]));
	dprintf("   MR2 (ReferenceCount): 0x%4.4x (Flags): 0x%4.4x\n",
	*((PWORD)&Packet->MiniportReserved[4]),*((PWORD)&Packet->MiniportReserved[6]));
	dprintf("   WR1: 0x%8.8x\n", *((PDWORD)&Packet->WrapperReserved[0]));
	dprintf("   WR2: 0x%8.8x\n", *((PDWORD)&Packet->WrapperReserved[4]));
}

