/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1993 Microsoft Corporation

 Module Name:

    typendr.cxx

 Abstract:

    Contains routines for the generation of the new NDR format strings for
    transmit_as and represent_as types.

 Notes:

 History:

    DKays     Jan-1994        Created.
    RyszardK  Jan-07-1994     Added transmit_as and represent as routines.
    RyszardK  Jan-17-1995     Added support for user_marshal.

 ----------------------------------------------------------------------------*/

#include "becls.hxx"

extern CMD_ARG * pCommand;

#pragma hdrstop


unsigned short
GenXmitOrRepAsQuintuple(
    CCB *       pCCB,
    BOOL        fXmit,
    CG_NDR *    pXmitNode,
    char *      pPresentedTypeName,
    node_skl *  pTypeForPrototype )
{
    unsigned short    Index;

    // Register the routine to be generated for future use.

    XMIT_AS_CONTEXT * pTransmitAsContext = new XMIT_AS_CONTEXT;

    pTransmitAsContext->fXmit     = fXmit;
    pTransmitAsContext->pXmitNode = pXmitNode;
    pTransmitAsContext->pTypeName = pPresentedTypeName;

    CG_NDR * pChild = (CG_NDR *)pXmitNode->GetChild();  // transmitted type.

    if ( pChild->GetCGID() == ID_CG_GENERIC_HDL )
        {
        pChild = (CG_NDR *)pChild->GetChild();   // skip it
        }

    BOOL  Added = pCCB->GetQuintupleDictionary()->Add( pTransmitAsContext );

    Index = pTransmitAsContext->Index;

    if ( Added )
        {
        // We haven't serviced this type yet.
          // Register with the ccb so that the prototypes can be emitted later

        if ( fXmit )
            pCCB->RegisterPresentedType( pTypeForPrototype );
        else
            pCCB->RegisterRepAsWireType( pTypeForPrototype );

        // Register the transmit_as contex to be able to generate
        // the helper routines.

        pCCB->RegisterQuintuple( pTransmitAsContext );
        }
    else
        delete pTransmitAsContext;

    return Index;
}


// ========================================================================
//       Transmit As
// ========================================================================

void
GenXmitOrRepAsNdrFormat(
    CCB *       pCCB,
    BOOL        fXmit,
    CG_NDR *    pXmitNode,
    char *      pPresentedTypeName,
    node_skl *  pPresentedType,
    node_skl *  pTransmittedType )
{
    FORMAT_STRING *    pFormatString;
    CG_NDR *        pChild;
    unsigned short  Index;
    long            ChildOffset;
    
    pFormatString = pCCB->GetFormatString();

    if (pCCB->GetFileCG()->GetLocalFormatString()
        == pFormatString)
    {
        // we're in a local proc
        pXmitNode->SetFormatStringOffset( pFormatString->GetCurrentOffset() );
        pFormatString->PushFormatChar(FC_BLKHOLE);
        pFormatString->PushByte( BLKHOLE_XURTYPE ); // flags
        pFormatString->PushShort( (short) -1 ); // Reserved
    	pXmitNode->SetFormatStringEndOffset( pFormatString->GetCurrentOffset() );
    	pXmitNode->SetFormatStringOffset( pFormatString->OptimizeFragment( pXmitNode ) );
        return;
    }

    pChild = (CG_NDR *) pXmitNode->GetChild();

    // Do this in case the child is a simple type.
    ChildOffset = pFormatString->GetCurrentOffset();

    pChild->GenNdrFormat( pCCB );

    // Again, do this in case the child is a simple type.
    pFormatString->Align();

    pXmitNode->SetFormatStringOffset( pFormatString->GetCurrentOffset() );

    Index = GenXmitOrRepAsQuintuple( pCCB,
                                     fXmit,
                                     pXmitNode,
                                     pPresentedTypeName,
                                     (fXmit ? pPresentedType
                                            : pTransmittedType) );

    if ( pChild->IsPointer() )
        pFormatString->PushFormatChar( fXmit ? FC_TRANSMIT_AS_PTR
                                             : FC_REPRESENT_AS_PTR );
    else
        pFormatString->PushFormatChar( fXmit ? FC_TRANSMIT_AS
                                             : FC_REPRESENT_AS );

    // Now the flag byte. Lower nibble keeps xmitted type alignment.
    // The upper one has the flag for -Oi when presented type is an array.

    unsigned char
    FlagByte = CvtAlignPropertyToAlign( pChild->GetWireAlignment()) - 1;

    if ( pPresentedType )
        {
        if ( pPresentedType->GetBasicType()->NodeKind() == NODE_ARRAY )
            FlagByte |= PRESENTED_TYPE_IS_ARRAY;
        else
            {
            if ( pXmitNode->GetMemoryAlignment() == 4 )
                FlagByte |= PRESENTED_TYPE_ALIGN_4;
            if ( pXmitNode->GetMemoryAlignment() == 8 )
                FlagByte |= PRESENTED_TYPE_ALIGN_8;
            }
        }

    pFormatString->PushByte( FlagByte );
    pFormatString->PushShort( (short) Index );

    // Now the presented type memory size and transmitted type bufsize.

    if ( pPresentedType )
        pFormatString->PushShort( (short)pPresentedType->GetSize( 0 ) );
    else
        {
        // unknown rep as type - will have to generate a sizing macro

        pCCB->GetRepAsSizeDict()->Register( pFormatString->GetCurrentOffset(),
                                            pPresentedTypeName );
        pFormatString->PushShortWithSizeMacro();
        }

    if ( pChild->HasAFixedBufferSize() )
        pFormatString->PushShort( (short) pChild->GetWireSize() );
    else
        pFormatString->PushShort( (short) 0 );

    if ( pChild->GetCGID() == ID_CG_GENERIC_HDL )
        pChild = (CG_NDR *)pChild->GetChild();

    if ( pChild->IsSimpleType() )
        {
        pFormatString->PushShortOffset( (short)
                    (ChildOffset - pFormatString->GetCurrentOffset()) );
        }
    else
        {
        pFormatString->PushShortOffset( (short)
                        ( pChild->GetFormatStringOffset() - 
                          pFormatString->GetCurrentOffset() ) );
        }
}

void
CG_TRANSMIT_AS::GenNdrFormat( CCB * pCCB )
/*++
    The format string is now:

        FC_TRANSMIT_AS
        Oi array flag/alignment<1>
        quintuple index<2>
        pres type mem size<2>
        tran type buf size<2>
        <<offset>>
--*/
{
    if ( GetFormatStringOffset() != -1 ) 
        return;

    GenXmitOrRepAsNdrFormat( pCCB,
                             TRUE,      // transmit as
                             (CG_NDR *)this,
                             GetPresentedType()->GetSymName(),
                             GetPresentedType(),
                             GetTransmittedType() );
}

long                    
CG_TRANSMIT_AS::GetStackSize()
{
    if ( pPresentedType->GetBasicType()->NodeKind() == NODE_ARRAY )
        return sizeof( void * );
    else
        return GetMemorySize();
}

//========================================================================
//      Represent_as
//========================================================================

void
CG_REPRESENT_AS::GenNdrFormat( CCB * pCCB )
/*++
    The format string is now:

        FC_REPRESENT_AS
        Oi array flag/alignment<1>
        quintuple index<2>
        pres type mem size<2>
        tran type buf size<2>
        <<offset>>
--*/
{
    if ( GetFormatStringOffset() != -1 ) 
        return;

    GenXmitOrRepAsNdrFormat( pCCB,
                             FALSE,      // represent as
                             (CG_NDR *)this,
                             GetRepAsTypeName(),
                             GetRepAsType(),
                             GetTransmittedType() );
}

long
CG_REPRESENT_AS::GetStackSize()
{
    //
    // A null representation type is ok here.  Unknown rep-as is not allowed
    // in the interpreter, so we just return 0 since the stack size will 
    // never actually be used for an -Os stub.
    //
    if ( ! pRepresentationType )
        return 0;

    if ( pRepresentationType->GetBasicType()->NodeKind() == NODE_ARRAY )
        return sizeof( void * );
    else
        return GetMemorySize();
}

void
CG_REPRESENT_AS::GenNdrParamDescription( CCB * pCCB )
{
    CG_NDR::GenNdrParamDescription( pCCB );

    // BUGBUG : Stack offsets of parameters after this guy are screwed.
}

void
CG_REPRESENT_AS::GenNdrParamDescriptionOld( CCB * pCCB )
{
    FORMAT_STRING * pProcFormatString;

    if ( GetRepAsType() )
        {
        CG_NDR::GenNdrParamDescriptionOld( pCCB );
        return;
        }

    pProcFormatString = pCCB->GetProcFormatString();

    pProcFormatString->PushUnknownStackSize( GetRepAsTypeName() );

    pProcFormatString->PushShortTypeOffset( (short) GetFormatStringOffset() );
}

void
CG_USER_MARSHAL::GenNdrFormat( CCB * pCCB )
/*++
    The format string is now:

        FC_USER_MARSHAL
        flags/alignment<1>
        quadruple index<2>
        pres type mem size<2>
        xmit type wire size<2>
        Offset to wire desc<2>
--*/
{
    if ( GetFormatStringOffset() != -1 ) 
        return;

    pCommand->GetNdrVersionControl().SetHasUserMarshal();

    FORMAT_STRING * pFormatString;
    CG_NDR *        pChild;
    long            ChildOffset;

    // Format offset

    pFormatString = pCCB->GetFormatString();

    if (pCCB->GetFileCG()->GetLocalFormatString()
        == pFormatString)
    {
        // we're in a local proc
        SetFormatStringOffset( pFormatString->GetCurrentOffset() );
        pFormatString->PushFormatChar(FC_BLKHOLE);
        pFormatString->PushByte( BLKHOLE_XURTYPE ); // flags
        pFormatString->PushShort( (short) -1 ); // Reserved
    	SetFormatStringEndOffset( pFormatString->GetCurrentOffset() );
    	SetFormatStringOffset( pFormatString->OptimizeFragment( this ) );
        return;        
    }

    pChild = (CG_NDR *) GetChild();

    assert( pChild );

    // Do this in case the child is a simple type.
    ChildOffset = pFormatString->GetCurrentOffset();

    pChild->GenNdrFormat( pCCB );

    // Again, do this in case the child is a simple type.
    pFormatString->Align();

    SetFormatStringOffset( pFormatString->GetCurrentOffset() );

    // Real stuff now

    pFormatString->PushFormatChar( FC_USER_MARSHAL );

    // Now the flag byte.
    // Top 2 bits convey the pointer info:
    //     0 - not a pointer
    //     1 - a ref
    //     2 - a unique

    unsigned char
    FlagByte = CvtAlignPropertyToAlign( pChild->GetWireAlignment()) - 1;

    if ( pChild->IsPointer() )
        {
        CG_POINTER * pPtr = (CG_POINTER *)pChild;
        assert( ! pPtr->IsFull() );

        if ( pPtr->IsUnique() )
            FlagByte |= USER_MARSHAL_UNIQUE;
        else if ( pPtr->IsRef() )
            FlagByte |= USER_MARSHAL_REF;
        }

    pFormatString->PushByte( FlagByte );

    // Register the routine to be generated for future use.

    USER_MARSHAL_CONTEXT * pUserMarshalContext = new USER_MARSHAL_CONTEXT;

    pUserMarshalContext->pTypeName = GetRepAsTypeName();
    pUserMarshalContext->pType     = GetRepAsType();

    BOOL  Added = pCCB->GetQuadrupleDictionary()->Add( pUserMarshalContext );

    unsigned short Index = pUserMarshalContext->Index;

    pFormatString->PushShort( (short) Index );

    if ( ! Added )
        delete pUserMarshalContext;

    // Now the presented type memory size and transmitted type bufsize.

    if ( GetRepAsType() )
        pFormatString->PushShort( (short) GetRepAsType()->GetSize( 0 ) );
    else
        {
        // Unknown user_marshall type - will have to generate a sizing macro
        // As represent_as and user_marshal are mutually exclusive,
        // we can use rep_as size dictionary.

        pCCB->GetRepAsSizeDict()->Register( pFormatString->GetCurrentOffset(),
                                            GetRepAsTypeName() );
        pFormatString->PushShortWithSizeMacro();
        }

    if ( pChild->HasAFixedBufferSize() )
        pFormatString->PushShort( (short) pChild->GetWireSize() );
    else
        pFormatString->PushShort( (short) 0 );

    if ( pChild->IsSimpleType() )
        {
        pFormatString->PushShortOffset( (short)
                    (ChildOffset - pFormatString->GetCurrentOffset()) );
        }
    else
        {
        pFormatString->PushShortOffset( (short)
                        ( pChild->GetFormatStringOffset() - 
                          pFormatString->GetCurrentOffset() ) );
        }

}


