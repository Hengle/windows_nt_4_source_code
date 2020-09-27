/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    Align.h

Abstract:

    Defines a macro for aligning an integer value or pointer
    to 0 mod 2^n for any n.

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     12-22-95    Bits 'n pieces
    MarioGo     02-19-96    Made type safe for C++.

--*/

#ifndef _ALIGN_H
#define _ALIGN_H

#ifdef __cplusplus

//
// The C++ interface looks like
//
//  val = Align(val, 8)  // returns val aligned to 0 mod 8
//  val = Align16(val);  // returns val aligned to 0 mod 16
//
//  Boths forms on the interface are equally efficient.
//
//  Returns the argument aligned up to the nearest "0 mod factor" boundary.  Has
//  no affect on values which are already aligned to 0 mod factor.  The argument
//  maybe any integer or void pointer type.
//
//

#define DECL_ALIGN_N(type) inline type Align( type value, int poft)       \
    {                                                                     \
    return (type)( ((unsigned long)(value) + ((poft)-1)) & ~(poft - 1) ); \
    }

#define DECL_ALIGN(poft, type) inline type Align##poft ( type value )     \
    {                                                                     \
    return Align(value, poft);                                            \
    }

#define DECL_PAD_N(type) inline unsigned int Pad( type value, int poft )  \
    {                                                                     \
    return (-(long)value) & (poft - 1);                                   \
    }

#define DECL_PAD(poft, type) inline unsigned int Pad##poft (type value)   \
    {                                                                     \
    return Pad(value, poft);                                              \
    }

#define DECL_ALL_ALIGN(type)     \
    DECL_ALIGN_N(type)           \
    DECL_ALIGN(2, type)          \
    DECL_ALIGN(4, type)          \
    DECL_ALIGN(8, type)          \
    DECL_ALIGN(16, type)         \
    DECL_ALIGN(32, type)

#define DECL_ALL_PAD(type)       \
    DECL_PAD_N(type)             \
    DECL_PAD(2, type)            \
    DECL_PAD(4, type)            \
    DECL_PAD(8, type)            \
    DECL_PAD(16, type)           \
    DECL_PAD(32, type)

#define DECL_ALL_ALIGN_AND_PAD(type) \
    DECL_ALL_PAD(type)               \
    DECL_ALL_ALIGN(type)

DECL_ALL_ALIGN_AND_PAD(short)
DECL_ALL_ALIGN_AND_PAD(unsigned short)
DECL_ALL_ALIGN_AND_PAD(long)
DECL_ALL_ALIGN_AND_PAD(unsigned long)
DECL_ALL_ALIGN_AND_PAD(int)
DECL_ALL_ALIGN_AND_PAD(unsigned int)
DECL_ALL_ALIGN_AND_PAD(void __RPC_FAR *)

#ifdef DOS
DECL_ALL_ALIGN_AND_PAD(void __near *)
#endif

#else

// C interface.

#define AlignN(p, poft) ( ((unsigned long)(p) + ((poft)-1)) & ~(poft - 1) )
#define PadN(p, poft) ( (-(long)p) & (poft - 1) )

#ifdef DOS
#define AlignPtrN(value, poft) (void __far *)AlignN(value, poft)
#define AlignNearPtrN(value, poft) (void __near *)AlignN(value, poft)
#else
#define AlignPtrN(value, poft) (void *)AlignN(value, poft)
#define AlignNearPtrN(value, poft) (void *)AlignN(value, poft)
#endif

// For aligning integer values

#define Align2(p) AlignN((p), 2)
#define Align4(p) AlignN((p), 4)
#define Align8(p) AlignN((p), 8)
#define Align16(p) AlignN((p), 16)
#define Align32(p) AlignN((p), 32)

// For aligning pointers

#define AlignPtr2(p) AlignPtrN((p), 2)
#define AlignPtr4(p) AlignPtrN((p), 4)
#define AlignPtr8(p) AlignPtrN((p), 8)
#define AlignPtr16(p) AlignPtrN((p), 16)
#define AlignPtr32(p) AlignPtrN((p), 32)

// For near pointers
#define AlignNearPtr2(p) AlignNearPtrN((p), 2)
#define AlignNearPtr4(p) AlignNearPtrN((p), 4)
#define AlignNearPtr8(p) AlignNearPtrN((p), 8)
#define AlignNearPtr16(p) AlignNearPtrN((p), 16)
#define AlignNearPtr32(p) AlignNearPtrN((p), 32)

// For everything
#define Pad2(p) PadN((p), 2)
#define Pad4(p) PadN((p), 4)
#define Pad8(p) PadN((p), 8)
#define Pad16(p) PadN((p), 16)
#define Pad32(p) PadN((p), 32)

#endif // __cplusplus

#endif // _ALIGN_H

