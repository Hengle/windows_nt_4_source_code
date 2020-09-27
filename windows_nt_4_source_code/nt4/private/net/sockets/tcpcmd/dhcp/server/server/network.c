/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    network.c

Abstract:

    This module contains the network interface for the DHCP server.

Author:

    Madan Appiah (madana)  10-Sep-1993
    Manny Weiser (mannyw)  24-Aug-1992

Environment:

    User Mode - Win32

Revision History:

--*/

#include "dhcpsrv.h"
#include "ipinfo.h"
#include "llinfo.h"
#include "ntddtcp.h"
#include "tdiinfo.h"

DWORD UpdateArpCache(DWORD dwIfAddress, DWORD dwIPAddress,
                     LPBYTE lpbLLAddress, DWORD dwLLAddrlen, BOOL bAdd);
DWORD OpenTcp();
DWORD TCPSetInformationEx(LPVOID lpvInBuffer, LPDWORD lpdwInSize,
                          LPVOID lpvOutBuffer, LPDWORD lpdwOutSize);
DWORD TCPQueryInformationEx(LPVOID lpvInBuffer, LPDWORD lpdwInSize,
                            LPVOID lpvOutBuffer, LPDWORD lpdwOutSize);

DWORD MapAddressToInstance(DWORD dwAddress, LPDWORD lpdwInstance,
                           LPDWORD lpdwIndex);
//
// TCPIP values
//
DWORD                 DhcpGlobalAddrToInstCount;
AddressToInstanceMap *DhcpGlobalAddrToInstTable = NULL;
HANDLE                DhcpGlobalTCPHandle;



DWORD
DhcpInitializeEndpoint(
    SOCKET *Socket,
    DHCP_IP_ADDRESS IpAddress,
    DWORD Port
    )
/*++

Routine Description:

    This function initializes an endpoint by creating and binding a
    socket to the local address.

Arguments:

    Socket - Receives a pointer to the newly created socket

    IpAddress - The IP address to initialize to.

    Port - The port to bind to.

Return Value:

    The status of the operation.

--*/
{
    DWORD Error;
    SOCKET Sock;
    DWORD OptValue;

#define SOCKET_RECEIVE_BUFFER_SIZE      1024 * 64   // 64K max.

    struct sockaddr_in SocketName;

    //
    // Create a socket
    //

    Sock = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP );

    if ( Sock == INVALID_SOCKET ) {
        Error = WSAGetLastError();
        goto Cleanup;
    }

    //
    // Make the socket share-able
    //

    OptValue = TRUE;
    Error = setsockopt(
                Sock,
                SOL_SOCKET,
                SO_REUSEADDR,
                (LPBYTE)&OptValue,
                sizeof(OptValue) );

    if ( Error != ERROR_SUCCESS ) {

        Error = WSAGetLastError();
        goto Cleanup;
    }

    OptValue = TRUE;
    Error = setsockopt(
                Sock,
                SOL_SOCKET,
                SO_BROADCAST,
                (LPBYTE)&OptValue,
                sizeof(OptValue) );

    if ( Error != ERROR_SUCCESS ) {

        Error = WSAGetLastError();
        goto Cleanup;
    }

    OptValue = SOCKET_RECEIVE_BUFFER_SIZE;
    Error = setsockopt(
                Sock,
                SOL_SOCKET,
                SO_RCVBUF,
                (LPBYTE)&OptValue,
                sizeof(OptValue) );

    if ( Error != ERROR_SUCCESS ) {

        Error = WSAGetLastError();
        goto Cleanup;
    }

    SocketName.sin_family = PF_INET;
    SocketName.sin_port = htons( (unsigned short)Port );
    SocketName.sin_addr.s_addr = IpAddress;
    RtlZeroMemory( SocketName.sin_zero, 8);

    //
    // Bind this socket to the DHCP server port
    //

    Error = bind(
               Sock,
               (struct sockaddr FAR *)&SocketName,
               sizeof( SocketName )
               );

    if ( Error != ERROR_SUCCESS ) {

        Error = WSAGetLastError();
        goto Cleanup;
    }

    *Socket = Sock;
    Error = ERROR_SUCCESS;

Cleanup:

    if( Error != ERROR_SUCCESS ) {

        //
        // if we aren't successful, close the socket if it is opened.
        //

        if( Sock != INVALID_SOCKET ) {
            closesocket( Sock );
        }

        DhcpPrint(( DEBUG_ERRORS,
            "DhcpInitializeEndpoint failed, %ld.\n", Error ));
    }

    return( Error );
}



DWORD
DhcpWaitForMessage(
    DHCP_REQUEST_CONTEXT *pRequestContext
    )
/*++

Routine Description:

    This function waits for a request on the DHCP port on any of the
    configured interfaces.

Arguments:

    RequestContext - A pointer to a request context block for
        this request.

Return Value:

    The status of the operation.

--*/
{
    DWORD length;
    DWORD error;
    fd_set readSocketSet;
    DWORD i;
    int readySockets;
    struct timeval timeout = { 0x7FFFFFFF, 0 }; // forever.

    //
    // Setup the file descriptor set for select.
    //

    FD_ZERO( &readSocketSet );
    for ( i = 0; i < DhcpGlobalNumberOfNets ; i++ ) {
        FD_SET(
            DhcpGlobalEndpointList[i].Socket,
            &readSocketSet
            );
    }

    readySockets = select( 0, &readSocketSet, NULL, NULL, &timeout );

    //
    // return to caller when the service is shutting down or select()
    // times out.
    //

    if( (readySockets == 0)  ||
        (WaitForSingleObject( DhcpGlobalProcessTerminationEvent, 0 ) == 0) ) {

        return( ERROR_SEM_TIMEOUT );
    }

    //
    // Time to play 20 question with winsock.  Which socket is ready?
    //

#if DBG
    pRequestContext->ActiveEndpoint = NULL;
#endif

    for ( i = 0; i < DhcpGlobalNumberOfNets ; i++ ) {
        if ( FD_ISSET( DhcpGlobalEndpointList[i].Socket, &readSocketSet ) ) {
            pRequestContext->ActiveEndpoint = &DhcpGlobalEndpointList[i];
            break;
        }
    }

    DhcpAssert( pRequestContext->ActiveEndpoint != NULL );


    //
    // Read data from the net.  If multiple sockets have data, just
    // process the first available socket.
    //

    pRequestContext->SourceNameLength = sizeof( struct sockaddr );

    //
    // clean the receive buffer before receiving data in it.
    //

    RtlZeroMemory( pRequestContext->ReceiveBuffer, DHCP_MESSAGE_SIZE );
    pRequestContext->ReceiveMessageSize = DHCP_MESSAGE_SIZE;

    length = recvfrom(
                 pRequestContext->ActiveEndpoint->Socket,
                 (char *)pRequestContext->ReceiveBuffer,
                 pRequestContext->ReceiveMessageSize,
                 0,
                 &pRequestContext->SourceName,
                 (int *)&pRequestContext->SourceNameLength
                 );

    if ( length == SOCKET_ERROR ) {
        error = WSAGetLastError();
        DhcpPrint(( DEBUG_ERRORS, "Recv failed, error = %ld\n", error ));
    } else {
        DhcpPrint(( DEBUG_MESSAGE, "Received message\n", 0 ));
        error = ERROR_SUCCESS;
    }

    pRequestContext->ReceiveMessageSize = length;
    return( error );
}



DWORD
DhcpSendMessage(
    LPDHCP_REQUEST_CONTEXT DhcpRequestContext
    )
/*++

Routine Description:

    This function send a response to a DHCP client.

Arguments:

    RequestContext - A pointer to the DhcpRequestContext block for
        this request.

Return Value:

    The status of the operation.

--*/
{
    DWORD error;
    struct sockaddr_in *source;
    LPDHCP_MESSAGE dhcpMessage;
    LPDHCP_MESSAGE dhcpReceivedMessage;
    WORD SendPort;
    DWORD MessageLength;
    BOOL  ArpCacheUpdated = FALSE;

    dhcpMessage = (LPDHCP_MESSAGE) DhcpRequestContext->SendBuffer;
    dhcpReceivedMessage = (LPDHCP_MESSAGE) DhcpRequestContext->ReceiveBuffer;

    //
    // if the request arrived from a relay agent, then send the reply
    // on server port otherwise on client port.
    //

    if ( dhcpReceivedMessage->RelayAgentIpAddress != 0 ) {
         SendPort = DHCP_SERVR_PORT;
    }
    else {
         SendPort = DHCP_CLIENT_PORT;
    }

    source = (struct sockaddr_in *)&DhcpRequestContext->SourceName;
    source->sin_port = htons( SendPort );

    //
    // if this request arrived from relay agent then send the
    // response to the address the relay agent says.
    //

    if ( dhcpReceivedMessage->RelayAgentIpAddress != 0 ) {
        source->sin_addr.s_addr = dhcpReceivedMessage->RelayAgentIpAddress;
    }
    else {

        //
        // if the client didnt specify broadcast bit and if
        // we know the ipaddress of the client then send unicast.
        //

        //
        // But if IgnoreBroadcastFlag is set in the registry and
        // if the client requested to broadcast or the server is
        // nacking or If the client doesn't have an address yet,
        // respond via broadcast.
        // Note that IgnoreBroadcastFlag is off by default. But it
        // can be set as a workaround for the clients that are not
        // capable of receiving unicast
        // and they also dont set the broadcast bit.
        //

        if ( DhcpGlobalIgnoreBroadcastFlag ) {
            if( (ntohs(dhcpReceivedMessage->Reserved) & DHCP_BROADCAST) ||
                    (dhcpReceivedMessage->ClientIpAddress == 0) ||
                    (source->sin_addr.s_addr == 0) ) {

                source->sin_addr.s_addr = (DWORD)-1;

                dhcpMessage->Reserved = 0;
                    // this flag should be zero in the local response.
            }

        } else {

            if( (ntohs(dhcpReceivedMessage->Reserved) & DHCP_BROADCAST) ||
                (!dhcpMessage->YourIpAddress ) ){

                source->sin_addr.s_addr = (DWORD)-1;

                dhcpMessage->Reserved = 0;
                    // this flag should be zero in the local response.
            } else {
                DWORD   LocalError;
                if ( ( LocalError = UpdateArpCache(
                                        DhcpRequestContext->ActiveEndpoint->IpAddress,
                                        dhcpMessage->YourIpAddress,
                                        dhcpReceivedMessage->HardwareAddress,
                                        dhcpReceivedMessage->HardwareAddressLength,
                                        TRUE)) == STATUS_SUCCESS ) {

                    source->sin_addr.s_addr = dhcpMessage->YourIpAddress;
                    ArpCacheUpdated = TRUE;
                    DhcpPrint((DEBUG_STOC, "Arp entry added on %s interface for %s client \n",
                        inet_ntoa( *(struct in_addr *)&DhcpRequestContext->ActiveEndpoint->IpAddress),
                        inet_ntoa( *(struct in_addr *)&dhcpMessage->YourIpAddress)) );

                } else {
                    DhcpPrint(( DEBUG_ERRORS, "Failed to add Arp entry on %s, for client %s, Error %lx\n",
                        inet_ntoa( *(struct in_addr *)&DhcpRequestContext->ActiveEndpoint->IpAddress),
                        inet_ntoa( *(struct in_addr *)&dhcpMessage->YourIpAddress), LocalError) );

                    source->sin_addr.s_addr = (DWORD)-1;
                }

            }

        }
    }

    DhcpPrint(( DEBUG_STOC, "Sending response to = %s, XID = %lx.\n",
        inet_ntoa(source->sin_addr), dhcpMessage->TransactionID));


    //
    // send minimum DHCP_MIN_SEND_RECV_PK_SIZE (300) bytes, otherwise
    // bootp relay agents don't like the packet.
    //

    MessageLength = (DhcpRequestContext->SendMessageSize >
                    DHCP_MIN_SEND_RECV_PK_SIZE) ?
                        DhcpRequestContext->SendMessageSize :
                            DHCP_MIN_SEND_RECV_PK_SIZE;
    error = sendto(
                 DhcpRequestContext->ActiveEndpoint->Socket,
                (char *)DhcpRequestContext->SendBuffer,
                MessageLength,
                0,
                &DhcpRequestContext->SourceName,
                DhcpRequestContext->SourceNameLength
                );

    if ( error == SOCKET_ERROR ) {
        error = WSAGetLastError();
        DhcpPrint(( DEBUG_ERRORS, "Send failed, error = %ld\n", error ));
    } else {
        error = ERROR_SUCCESS;
    }

    if ( ArpCacheUpdated ) {
        DWORD   LocalError;
        if ( ( LocalError = UpdateArpCache(
                                DhcpRequestContext->ActiveEndpoint->IpAddress,
                                dhcpMessage->YourIpAddress,
                                dhcpReceivedMessage->HardwareAddress,
                                dhcpReceivedMessage->HardwareAddressLength,
                                FALSE)) != STATUS_SUCCESS ) {

            DhcpPrint(( DEBUG_ERRORS, "Failed to delete Arp entry on %s, for client %s, Error %lx\n",
                inet_ntoa( *(struct in_addr *)&DhcpRequestContext->ActiveEndpoint->IpAddress),
                inet_ntoa( *(struct in_addr *)&dhcpMessage->YourIpAddress), LocalError) );

        }

    }

    return( error );
}

//---------------------------------------------------------------------------
// Function:    UpdateArpCache
//
// Parameters:
//      DWORD   dwIfAddress     the interface to modify
//      DWORD   dwIPAddress     the IP address to add
//      LPBYTE  lpbLLAddress    the hardware address to add
//      DWORD   dwLLAddrlen     the length of the hardware address
//      BOOL    bAdd            if true, an entry is added. otherwise,
//                              an entry is deleted.
//
// This function adds or deletes an entry to the local ARP cache for
// the network interface whose IP address is dwIfAddress
//---------------------------------------------------------------------------
DWORD APIENTRY UpdateArpCache(DWORD dwIfAddress, DWORD dwIPAddress,
                     LPBYTE lpbLLAddress, DWORD dwLLAddrlen, BOOL bAdd) {
    UCHAR *lpContext;
    TDIObjectID *lpObject;
    IPNetToMediaEntry *lpinmeEntry;
    TCP_REQUEST_SET_INFORMATION_EX *lptrsiBuffer;

    DWORD dwErr, dwInstance, dwIndex, dwInSize, dwOutSize;


    dwErr = MapAddressToInstance(dwIfAddress, &dwInstance, &dwIndex);
    if (dwErr == (DWORD)-1) { return 1; }

    dwInSize = sizeof(TCP_REQUEST_SET_INFORMATION_EX) +
               sizeof(IPNetToMediaEntry) - 1;
    lptrsiBuffer = DhcpAllocateMemory(dwInSize);
    if (lptrsiBuffer == NULL) {
        return GetLastError();
    }

    lptrsiBuffer->BufferSize = sizeof(IPNetToMediaEntry);

    lpObject = &lptrsiBuffer->ID;
    lpObject->toi_id = AT_MIB_ADDRXLAT_ENTRY_ID;
    lpObject->toi_class = INFO_CLASS_PROTOCOL;
    lpObject->toi_type = INFO_TYPE_PROVIDER;
    lpObject->toi_entity.tei_entity = AT_ENTITY;
    lpObject->toi_entity.tei_instance = dwInstance;

    lpinmeEntry = (IPNetToMediaEntry *)lptrsiBuffer->Buffer;
    lpinmeEntry->inme_index = dwIndex;
    lpinmeEntry->inme_addr = dwIPAddress;
    lpinmeEntry->inme_type = (bAdd ? INME_TYPE_DYNAMIC : INME_TYPE_INVALID);
    lpinmeEntry->inme_physaddrlen = dwLLAddrlen;
    CopyMemory(lpinmeEntry->inme_physaddr, lpbLLAddress, dwLLAddrlen);

    dwOutSize = 0;
    dwErr = TCPSetInformationEx((LPVOID)lptrsiBuffer, &dwInSize,
                                NULL, &dwOutSize);

    return dwErr;
}



//-----------------------------------------------------------------------
// Function:    GetAddressToInstanceTable
//
// Parameters:
//      none
//
// Builds the mappings from IP address to ARP Entity Instance; these
// mappings are needed for adding entries to ARP caches when messages
// are being relayed to clients.
// The algorithm is as follows:

//  get table of all TDI entities.
//  query each address translation entity in the table, and save
//      the instance numbers of all entities which support ARP.
//  query each ARP entity for its address translation info; this
//      info includes a field axi_index.
//  query the IP layer for the table of all IP address entries; these
//      entries include a field iae_index which corresponds to axi_index.
//  for each IP address, make an entry in our global AddressToInstanceMap
//      array, setting the index (by matching iae_index to axi_index),
//      the address (using iae_address), and the instance number
//      by using iae_index (==axi_index) to find the AddrXlatInfo for this
//      IP address, and then using the AddrXlatInfo to find the
//      ARP instance number.
//-----------------------------------------------------------------------
DWORD APIENTRY GetAddressToInstanceTable() {
    UCHAR *lpContext;
    IPSNMPInfo ipsiInfo;
    TDIObjectID *lpObject;

    DWORD dwErr, dwInSize, dwOutSize;
    TCP_REQUEST_QUERY_INFORMATION_EX trqiBuffer;

    DWORD dwAxiCount;
    AddrXlatInfo *lpAXITable, *lpaxi, *lpaxiend;

    DWORD dwAddrCount;
    IPAddrEntry *lpAddrTable, *lpaddr, *lpaddrend;

    DWORD dwEntityCount;
    TDIEntityID *lpEntityTable, *lpent, *lpentend;

    DWORD dwArpInstCount;
    DWORD adwArpInstTable[MAX_TDI_ENTITIES], *lpdwinst, *lpdwinstend;

    AddressToInstanceMap *lpAddrToInstTable, *lpatoi;


    //-----------------
    // get entity table
    //-----------------
    dwInSize = sizeof(TCP_REQUEST_QUERY_INFORMATION_EX);
    dwOutSize = MAX_TDI_ENTITIES * sizeof(TDIEntityID);

    lpEntityTable = (TDIEntityID *)DhcpAllocateMemory(dwOutSize);
    if (lpEntityTable == NULL) {
        return GetLastError();
    }

    lpContext = trqiBuffer.Context;
    ZeroMemory(lpContext, CONTEXT_SIZE);

    lpObject = &trqiBuffer.ID;
    lpObject->toi_id = ENTITY_LIST_ID;
    lpObject->toi_type = INFO_TYPE_PROVIDER;
    lpObject->toi_class = INFO_CLASS_GENERIC;
    lpObject->toi_entity.tei_instance = 0;
    lpObject->toi_entity.tei_entity = GENERIC_ENTITY;

    dwErr = TCPQueryInformationEx(&trqiBuffer, &dwInSize,
                                  lpEntityTable, &dwOutSize);
    if (dwErr != NO_ERROR || dwOutSize == 0) {
        DhcpFreeMemory(lpEntityTable);
        return dwErr;
    }

    dwEntityCount = dwOutSize / sizeof(TDIEntityID);


    //-------------------------------------------
    // copy instance numbers for all ARP entities
    //-------------------------------------------
    dwInSize = sizeof(TCP_REQUEST_QUERY_INFORMATION_EX);
    lpContext = trqiBuffer.Context;
    lpObject = &trqiBuffer.ID;
    lpObject->toi_id = ENTITY_TYPE_ID;
    lpObject->toi_type = INFO_TYPE_PROVIDER;
    lpObject->toi_class = INFO_CLASS_GENERIC;
    lpObject->toi_entity.tei_entity = AT_ENTITY;

    lpdwinst = adwArpInstTable;
    lpentend = lpEntityTable + dwEntityCount;
    for (lpent = lpEntityTable; lpent < lpentend; lpent++) {
        if (lpent->tei_entity == AT_ENTITY) {
            DWORD dwEntityType;

            lpObject->toi_type = INFO_TYPE_PROVIDER;
            lpObject->toi_class = INFO_CLASS_GENERIC;
            lpObject->toi_entity.tei_entity = AT_ENTITY;
            lpObject->toi_entity.tei_instance = lpent->tei_instance;

            dwInSize = sizeof(TCP_REQUEST_QUERY_INFORMATION_EX);
            dwOutSize = sizeof(dwEntityType);
            ZeroMemory(lpContext, CONTEXT_SIZE);

            dwErr = TCPQueryInformationEx(&trqiBuffer, &dwInSize,
                                          &dwEntityType, &dwOutSize);
            if (dwErr != NO_ERROR) {
                continue;
            }

            if (dwEntityType == AT_ARP) {
                *lpdwinst++ = lpent->tei_instance;
            }

        }
    }

    // done with the entity table now
    DhcpFreeMemory(lpEntityTable);

    dwArpInstCount = lpdwinst - adwArpInstTable;
    if (dwArpInstCount == 0) {
        return 0;
    }


    //------------------------------------------------
    // make memory for the AT entities and then
    // query ARP entities for address translation info
    //------------------------------------------------
    lpAXITable = DhcpAllocateMemory(dwArpInstCount * sizeof(AddrXlatInfo));
    if (lpAXITable == NULL) {
        return GetLastError();
    }

    lpContext = trqiBuffer.Context;

    lpObject->toi_id = AT_MIB_ADDRXLAT_INFO_ID;
    lpObject->toi_class = INFO_CLASS_PROTOCOL;
    lpObject->toi_entity.tei_entity = AT_ENTITY;

    lpaxi = lpAXITable;
    lpaxiend = lpAXITable + dwArpInstCount;
    lpdwinstend = adwArpInstTable + dwArpInstCount;
    for (lpdwinst = adwArpInstTable; lpdwinst < lpdwinstend; lpdwinst++) {
        lpObject->toi_entity.tei_instance = *lpdwinst;

        dwInSize = sizeof(TCP_REQUEST_QUERY_INFORMATION_EX);
        dwOutSize = sizeof(AddrXlatInfo);

        ZeroMemory(lpContext, CONTEXT_SIZE);

        dwErr = TCPQueryInformationEx(&trqiBuffer, &dwInSize,
                                      lpaxi, &dwOutSize);

        // ignore errors, since each AddrXlatInfo must be in
        // a position in the AXI table that is parallel with
        // the postion of its instance number in the instance table
        if (dwErr != 0) {
            lpaxi->axi_index = (DWORD)-1;
        }
        ++lpaxi;
    }

    dwAxiCount = lpaxi - lpAXITable;
    if (dwAxiCount == 0) {
        DhcpFreeMemory(lpAXITable);
        return 0;
    }


    //------------------------------
    // query IP for IP address table
    //------------------------------

    // first get address count
    dwInSize = sizeof(TCP_REQUEST_QUERY_INFORMATION_EX);
    dwOutSize = sizeof(IPSNMPInfo);

    lpContext = trqiBuffer.Context;
    ZeroMemory(lpContext, CONTEXT_SIZE);

    lpObject->toi_id = IP_MIB_STATS_ID;
    lpObject->toi_type = INFO_TYPE_PROVIDER;
    lpObject->toi_class = INFO_CLASS_PROTOCOL;
    lpObject->toi_entity.tei_entity = CL_NL_ENTITY;
    lpObject->toi_entity.tei_instance = 0;

    dwErr = TCPQueryInformationEx(&trqiBuffer, &dwInSize,
                                  &ipsiInfo, &dwOutSize);
    if (dwErr != NO_ERROR || ipsiInfo.ipsi_numaddr == 0) {
        DhcpFreeMemory(lpAXITable);
        return dwErr;
    }

    dwAddrCount = ipsiInfo.ipsi_numaddr;

    // got address count, now get address table

    dwInSize = sizeof(TCP_REQUEST_QUERY_INFORMATION_EX);
    dwOutSize = (dwAddrCount + 5) * sizeof(IPAddrEntry);

    lpAddrTable = DhcpAllocateMemory(dwOutSize);
    if (lpAddrTable == NULL) {
        DhcpFreeMemory(lpAXITable);
        return GetLastError();
    }

    lpContext = trqiBuffer.Context;
    ZeroMemory(lpContext, CONTEXT_SIZE);

    lpObject->toi_id = IP_MIB_ADDRTABLE_ENTRY_ID;
    lpObject->toi_type = INFO_TYPE_PROVIDER;
    lpObject->toi_class = INFO_CLASS_PROTOCOL;
    lpObject->toi_entity.tei_entity = CL_NL_ENTITY;
    lpObject->toi_entity.tei_instance = 0;

    dwErr = TCPQueryInformationEx(&trqiBuffer, &dwInSize,
                                  lpAddrTable, &dwOutSize);
    if (dwErr != NO_ERROR) {
        DhcpFreeMemory(lpAXITable);
        DhcpFreeMemory(lpAddrTable);
        return GetLastError();
    }


    // build table of AddressToInstanceMap structures
    // for each IP address, as follows:
    //      use IPAddrEntry.iae_index to find corresponding
    //          AddrXlatInfo.axi_index
    //      use the AddrXlatInfo found to find corresponding TDIEntityID
    //      use the TDIEntity found to set AddressToInstanceMap.dwInstance
    //--------------------------------------------------------------------
    lpAddrToInstTable = DhcpAllocateMemory(dwAddrCount * sizeof(AddressToInstanceMap));
    if (lpAddrToInstTable == NULL) {
        DhcpFreeMemory(lpAXITable);
        DhcpFreeMemory(lpAddrTable);
        return GetLastError();
    }

    lpatoi = lpAddrToInstTable;
    lpaxiend = lpAXITable + dwAxiCount;
    lpaddrend = lpAddrTable + dwAddrCount;
    for (lpaddr = lpAddrTable; lpaddr < lpaddrend; lpaddr++) {
        lpatoi->dwIndex = lpaddr->iae_index;
        lpatoi->dwIPAddress = lpaddr->iae_addr;

        // find the instance number
        lpdwinst = adwArpInstTable;
        for (lpaxi = lpAXITable; lpaxi < lpaxiend; lpaxi++) {
            if (lpaxi->axi_index != (DWORD)-1 &&
                lpaxi->axi_index == lpaddr->iae_index) {
                lpatoi->dwInstance = *lpdwinst;
                ++lpatoi;
                break;
            }
            ++lpdwinst;
        }
    }

    // done with AXI table and IP address table
    DhcpFreeMemory(lpAXITable);
    DhcpFreeMemory(lpAddrTable);

    if (DhcpGlobalAddrToInstTable != NULL) {
        DhcpFreeMemory(DhcpGlobalAddrToInstTable);
    }

    DhcpGlobalAddrToInstCount = lpatoi - lpAddrToInstTable;
    DhcpGlobalAddrToInstTable = lpAddrToInstTable;

    return 0;
}



//------------------------------------------------------------------
// Function:    MapAddressToInstance
//
// Parameters:
//      DWORD   dwAddress       the address to map
//
//------------------------------------------------------------------
DWORD MapAddressToInstance(DWORD dwAddress, LPDWORD lpdwInstance,
                           LPDWORD lpdwIndex) {
    DWORD dwErr;
    AddressToInstanceMap *lpptr, *lpend;

    dwErr = (DWORD)-1;

    if ( DhcpGlobalAddrToInstTable == NULL ) {
        return dwErr;
    }
    lpend = DhcpGlobalAddrToInstTable + DhcpGlobalAddrToInstCount;
    for (lpptr = DhcpGlobalAddrToInstTable; lpptr < lpend; lpptr++) {
        if (dwAddress == lpptr->dwIPAddress) {
            *lpdwIndex = lpptr->dwIndex;
            *lpdwInstance = lpptr->dwInstance;
            dwErr = 0;
            break;
        }
    }

    return dwErr;
}



//------------------------------------------------------------------
// Function:    OpenTcp
//
// Parameters:
//      none.
//
// Opens the handle to the Tcpip driver.
//------------------------------------------------------------------
DWORD OpenTcp() {
    NTSTATUS status;
    UNICODE_STRING nameString;
    IO_STATUS_BLOCK ioStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;

    // Open the ip stack for setting routes and parps later.
    //
    // Open a Handle to the TCP driver.
    //
    RtlInitUnicodeString(&nameString, DD_TCP_DEVICE_NAME);

    InitializeObjectAttributes(&objectAttributes, &nameString,
			                   OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = NtCreateFile(&DhcpGlobalTCPHandle,
                          SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
		                  &objectAttributes, &ioStatusBlock, NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN_IF, 0, NULL, 0);

    return (status == STATUS_SUCCESS ? 0 : ERROR_OPEN_FAILED);

}



//---------------------------------------------------------------------
// Function:        TCPQueryInformationEx
//
// Parameters:
//      TDIObjectID *ID            The TDI Object ID to query
//      void        *Buffer        buffer to contain the query results
//      LPDWORD     *BufferSize    pointer to the size of the buffer
//                                 filled in with the amount of data.
//      UCHAR       *Context       context value for the query. should
//                                 be zeroed for a new query. It will be
//                                 filled with context information for
//                                 linked enumeration queries.
//
// Returns:
//      An NTSTATUS value.
//
//  This routine provides the interface to the TDI QueryInformationEx
//      facility of the TCP/IP stack on NT.
//---------------------------------------------------------------------
DWORD TCPQueryInformationEx(LPVOID lpvInBuffer, LPDWORD lpdwInSize,
                            LPVOID lpvOutBuffer, LPDWORD lpdwOutSize) {
    NTSTATUS status;
    IO_STATUS_BLOCK isbStatusBlock;

    if (DhcpGlobalTCPHandle == NULL) {
        OpenTcp();
    }

    status = NtDeviceIoControlFile(DhcpGlobalTCPHandle, // Driver handle
                                   NULL,                // Event
                                   NULL,                // APC Routine
                                   NULL,                // APC context
                                   &isbStatusBlock,     // Status block
                                   IOCTL_TCP_QUERY_INFORMATION_EX,  // Control
                                   lpvInBuffer,         // Input buffer
                                   *lpdwInSize,         // Input buffer size
                                   lpvOutBuffer,        // Output buffer
                                   *lpdwOutSize);       // Output buffer size

    if (status == STATUS_PENDING) {
	    status = NtWaitForSingleObject(DhcpGlobalTCPHandle, TRUE, NULL);
        status = isbStatusBlock.Status;
    }

    if (status != STATUS_SUCCESS) {
        *lpdwOutSize = 0;
    }
    else {
        *lpdwOutSize = isbStatusBlock.Information;
    }

    return status;
}




//---------------------------------------------------------------------------
// Function:        TCPSetInformationEx
//
// Parameters:
//
//      TDIObjectID *ID         the TDI Object ID to set
//      void      *lpvBuffer    data buffer containing the information
//                              to be set
//      DWORD     dwBufferSize  the size of the data buffer.
//
//  This routine provides the interface to the TDI SetInformationEx
//  facility of the TCP/IP stack on NT.
//---------------------------------------------------------------------------
DWORD TCPSetInformationEx(LPVOID lpvInBuffer, LPDWORD lpdwInSize,
                          LPVOID lpvOutBuffer, LPDWORD lpdwOutSize) {
    NTSTATUS status;
    IO_STATUS_BLOCK isbStatusBlock;

    if (DhcpGlobalTCPHandle == NULL) {
        OpenTcp();
    }

    status = NtDeviceIoControlFile(DhcpGlobalTCPHandle, // Driver handle
                                   NULL,                // Event
                                   NULL,                // APC Routine
                                   NULL,                // APC context
                                   &isbStatusBlock,     // Status block
                                   IOCTL_TCP_SET_INFORMATION_EX,    // Control
                                   lpvInBuffer,         // Input buffer
                                   *lpdwInSize,         // Input buffer size
                                   lpvOutBuffer,        // Output buffer
                                   *lpdwOutSize);       // Output buffer size

    if (status == STATUS_PENDING) {
        status = NtWaitForSingleObject(DhcpGlobalTCPHandle, TRUE, NULL);
        status = isbStatusBlock.Status;
    }

    if (status != STATUS_SUCCESS) {
        *lpdwOutSize = 0;
    }
    else {
        *lpdwOutSize = isbStatusBlock.Information;
    }

    return status;
}


DHCP_IP_ADDRESS
DhcpResolveName(
    CHAR *szHostName
    )
/*++

Routine Description:
    Resolves the specified host name to an IP address
    .
Arguments:

    szHostName  - host name
    .

Return Value:

    Success - the IP address for szHostName.
    Failure - 0.

    .

--*/

{
    DHCP_IP_ADDRESS IpAddress = 0;

    IpAddress = inet_addr( szHostName );

    if ( INADDR_NONE == IpAddress )
    {
        HOSTENT *ph = gethostbyname( szHostName );
        if ( ph )
        {
            IpAddress = *((DHCP_IP_ADDRESS*) ph->h_addr_list[0]);
        }
    }

    return IpAddress;
}
