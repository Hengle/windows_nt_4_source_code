/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989 Microsoft Corporation

 Module Name:
	
	ilxlat.hxx

 Abstract:

	The definitions for ilxlat module.

 Notes:


 History:

 	VibhasC		Aug-13-1993		Created.

 ----------------------------------------------------------------------------*/
#ifndef __ILXLAT_HXX__
#define __ILXLAT_HXX__

#include "fldattr.hxx"
#include "walkctxt.hxx"
#include "optprop.hxx"

class node_skl;

class XLAT_CTXT;

enum _IL_ANCESTOR_FLAGS
	{
	IL_IN_MULTIDIM_CONF		=	0x00000001,	// parent is multidimension conf arr
	IL_IN_MULTIDIM_VAR		=	0x00000002,	// parent is multidimension var arr
    IL_IN_LIBRARY           =   0x00000004, // node is contained within a type library
    IL_IN_LOCAL             =   0x00000008, // node is contained within a local proc
	};


typedef unsigned long	IL_ANCESTOR_FLAGS;

class XLAT_CTXT:	public WALK_CTXT, public XLAT_SIZE_INFO
	{
private:
	
	unsigned long		ILAncestorBits;

public:
						// use this constructor to manufacture a context node without a parent context
						XLAT_CTXT( node_skl * pPar	= NULL):
								WALK_CTXT( pPar )
							{
							ILAncestorBits = 0;
							}
						
						// use this constructor to build information from a parent context
						XLAT_CTXT( node_skl * pPar, XLAT_CTXT * pCtxt) :
								WALK_CTXT( pPar, pCtxt ),
								XLAT_SIZE_INFO( pCtxt )
							{
							ILAncestorBits = pCtxt->ILAncestorBits;
							}

						// use this to make one with info from a parent context
						// that also corresponds to this node
						XLAT_CTXT( XLAT_CTXT * pCtxt) :
								WALK_CTXT( pCtxt ),
								XLAT_SIZE_INFO( pCtxt )
							{
							ILAncestorBits = pCtxt->ILAncestorBits;
							}

	// ancestor bit manipulation

	IL_ANCESTOR_FLAGS&		GetAncestorBits()
								{
								return ILAncestorBits;
								}

	IL_ANCESTOR_FLAGS&		SetAncestorBits( IL_ANCESTOR_FLAGS f )
								{
								ILAncestorBits |= f;
								return ILAncestorBits;
								}
	
	IL_ANCESTOR_FLAGS&		ClearAncestorBits( IL_ANCESTOR_FLAGS f )
								{
								ILAncestorBits &= ~f;
								return ILAncestorBits;
								}
	
	BOOL					AnyAncestorBits( IL_ANCESTOR_FLAGS f )
								{
								return (ILAncestorBits & f);
								}
	
	BOOL					AllAncestorBits( IL_ANCESTOR_FLAGS f )
								{
								return ((ILAncestorBits & f) == f);
								}



	};


#endif // __ILXLAT_HXX__
