/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    uibuffer.hxx
    BUFFER class declaration

    FILE HISTORY:
        rustanl     03-Aug-1990     Created
        beng        30-Apr-1991     Uses lmui.hxx

*/

#ifndef _BUFFER_HXX_
#define _BUFFER_HXX_

extern "C"
{

# include <nt.h>
# include <ntrtl.h>
# include <nturtl.h>
# include <windows.h>

};


# if !defined( dllexp)
# define dllexp               __declspec( dllexport)
# endif // !defined( dllexp)




/*************************************************************************

    NAME:       BUFFER (buf)

    SYNOPSIS:   A resizable object which lives in the application heap.

                Upon construction, the buffer takes a requested size in
                bytes; it allocates storage sufficient to hold that size.
                The client can later change this size with Resize, Trim,
                and FillOut.  QuerySize returns the current size of
                the buffer; QueryPtr returns a pointer to its storage.

                Note that a buffer may have size 0, in which case it
                keeps no allocated storage.

    INTERFACE:  BUFFER()        - Constructor, naming initial size in bytes

                QuerySize()     - return size in bytes
                QueryPtr()      - return pointer to data buffer

                Resize()        - resize the object to the given number
                                  of bytes.  Returns TRUE if the resize was
                                  successful; otherwise returns FALSE (use
                                  GetLastError for error code)

                Trim()          - force block to occupy no more storage
                                  than the client has requested.

    PARENT:

    HISTORY:
        RustanL     3 Aug 90        Created
        DavidHov    5 Mar 91        Merged with BUFFER.HXX to supplant same
        beng        19-Jun-1991     Inherit from BASE; UINT sizes
        beng        15-Jul-1991     Resize returns APIERR instead of BOOL
        beng        19-Mar-1992     Remove OS/2 support

**************************************************************************/

class BUFFER
{
private:
    BYTE *  _pb;        // pointer to storage
    UINT    _cb;        // size of storage, as requested by client

    inline VOID VerifyState() const;

    UINT QueryActualSize();
    dllexp BOOL GetNewStorage( UINT cbRequested );
    BOOL ReallocStorage( UINT cbNewlyRequested );

public:
    dllexp BUFFER( UINT cbRequested = 0 )
    {
        _pb = NULL;
        _cb = 0;

        if ( cbRequested != 0 )
        {
            GetNewStorage(cbRequested);
        }
    }

    dllexp ~BUFFER()
    {
        if ( _pb )
        {
            ::LocalFree( (HANDLE) _pb );
        }
    }

    dllexp VOID * QueryPtr() const
        { return _pb; }

    dllexp UINT QuerySize() const
        { return _cb; }

    //
    //  If a resize is needed, added cbSlop to it
    //

    dllexp BOOL Resize( UINT cbNewReqestedSize,
                        UINT cbSlop = 0);

    // The following method deals with the difference between the
    // actual memory size and the requested size.  These methods are
    // intended to be used when optimization is key.
    // Trim reallocates the buffer so that the actual space allocated is
    // minimally more than the size requested
    //
    dllexp VOID Trim();
};

//
//  This class is a single item in a chain of buffers
//

class BUFFER_CHAIN_ITEM : public BUFFER
{

friend class BUFFER_CHAIN;

public:
    dllexp BUFFER_CHAIN_ITEM( UINT cbReq = 0 )
      : BUFFER( cbReq ),
        _cbUsed( 0 )
        { _ListEntry.Flink = NULL; }

    dllexp ~BUFFER_CHAIN_ITEM()
        { if ( _ListEntry.Flink )
              RemoveEntryList( &_ListEntry );
        }

    dllexp DWORD QueryUsed( VOID ) const
        { return _cbUsed; }

    dllexp VOID SetUsed( DWORD cbUsed )
        { _cbUsed = cbUsed; }

private:
    LIST_ENTRY _ListEntry;
    DWORD      _cbUsed;     // Bytes of valid data in this buffer
};

class BUFFER_CHAIN
{
public:
    dllexp BUFFER_CHAIN()
        { InitializeListHead( &_ListHead ); }

    dllexp ~BUFFER_CHAIN()
        { DeleteChain(); }

    dllexp BOOL AppendBuffer( BUFFER_CHAIN_ITEM * pBCI );

    //
    //  Returns total number of bytes freed by deleting all of the buffer
    //  chain items
    //

    dllexp DWORD DeleteChain();

    //
    //  Enums buffer chain.  Pass pBCI as NULL on first call, pass return
    //  value till NULL on subsequent calls
    //

    dllexp BUFFER_CHAIN_ITEM * NextBuffer( BUFFER_CHAIN_ITEM * pBCI );

    //
    //  Gives back total number of bytes allocated by chain (includes unused
    //  bytes)
    //

    dllexp DWORD CalcTotalSize( BOOL fUsed = FALSE ) const;

private:

    LIST_ENTRY _ListHead;

};

#endif // _BUFFER_HXX_

