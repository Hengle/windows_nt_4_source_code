/************************************************************************

Copyright (c) 1993 Microsoft Corporation

Module Name :

    srvout.c

Abstract :

    Contains routines for support of [out] parameters on server side during 
    unmarshalling phase. This includes deferral, allocation and handle 
    initialization.

Author :     

    Bruce McQuistan (brucemc)   12/93.

Revision History :

    DKays   10/94   Major comment and code clean up.

 ***********************************************************************/

#include "ndrp.h"
#include "hndl.h"
#include "interp.h"

void
NdrOutInit(
    PMIDL_STUB_MESSAGE      pStubMsg,
    PFORMAT_STRING          pFormat,
    uchar **                ppArg
    )
/*++

Routine Description :        

    This routine is called to manage server side issues for [out] params 
    such as allocation and context handle initialization. Due to the fact 
    that for [out] conformant objects on stack, their size descriptors may 
    not have been unmarshalled when we need to know their size, this routine 
    must be called after all other unmarshalling has occurred. Really, we 
    could defer only [out], conformant data, but the logic in walking the 
    format string to determine if an object is conformant does not warrant 
    that principle, so all [out] data is deferred.

Arguments :      

    pStubMsg    - Pointer to stub message.
    pFormat     - Format string description for the type.
    ppArg       - Location of argument on stack.

Return :

    None.

 --*/
{
    // Don't screw with this, it must be a signed long!
    long    Size;  

    //
    // Check for a non-Interface pointer (they have a much different format 
    // than regular pointers).
    //
    if ( IS_BASIC_POINTER(*pFormat) )
        {
        //
        // Check for a pointer to a basetype (we don't have to worry about
        // a non-sized string pointer because these are not allowed as [out]
        // only.
        //
        if ( SIMPLE_POINTER(pFormat[1]) )
            {
            Size = SIMPLE_TYPE_MEMSIZE(pFormat[2]);
            goto DoAlloc;
            }

        //
        // Check for a pointer to a pointer.
        //
        if ( POINTER_DEREF(pFormat[1]) )
            {
            Size = sizeof(void *);
            goto DoAlloc;
            }

        // We have a pointer to complex type.

        pFormat += 2;
        pFormat += *(signed short *)pFormat;
        }

    if ( *pFormat == FC_BIND_CONTEXT )
        {
        NdrSaveContextHandle( 
            pStubMsg,
            NDRSContextUnmarshall( 0, pStubMsg->RpcMsg->DataRepresentation ),
            ppArg,
            pFormat );

        return;
        }

    //
    // If we get here we have to make a call to size a complex type.
    //
    Size = (long) NdrpMemoryIncrement( pStubMsg,
                                       0, 
                                       pFormat );

DoAlloc:

    //
    // Check for a negative size.  This an application error condition for
    // signed size specifiers.
    //
    if ( Size < 0 )
        RpcRaiseException( RPC_X_INVALID_BOUND );

    *ppArg = NdrAllocate( pStubMsg, (size_t) Size);

    MIDL_memset( *ppArg, 0, (size_t) Size );

    // We are almost done, except for an out ref to ref to ... etc.
    // If this is the case keep allocating pointees of ref pointers.

    if ( *pFormat == FC_RP  &&  POINTER_DEREF(pFormat[1]) )
        {
        pFormat += 2;
        pFormat += *(signed short *)pFormat;

        if ( *pFormat == FC_RP )
            NdrOutInit( pStubMsg, pFormat, (uchar **) *ppArg );
        }
}

