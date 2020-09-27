/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1995                **/
/**********************************************************************/

/*
    FILE HISTORY:

*/

#include "stdafx.h"
#include "comprop.h"

#include <stdlib.h>
#include <memory.h>
#include <ctype.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

CObjHelper :: CObjHelper ()
    : m_ctor_err( 0 ),
      m_api_err( 0 ),
      m_b_dirty( FALSE ),
      m_time_created( ::GetCurrentTime() )
{
}

void
CObjHelper :: ReportError (
    LONG errInConstruction
    )
{
    TRACEEOLID( _T("CObjectPlus construction failure, error = ")
        << errInConstruction );
    m_ctor_err = errInConstruction;
}

LONG
CObjHelper :: SetApiErr (
    LONG errApi
    )
{
    return m_api_err = errApi;
}

void
CObjHelper :: AssertValid () const
{
    ASSERT(QueryError() == 0);
}

BOOL
CObjHelper :: IsValid () const
{
    return QueryError() == 0;
}

DWORD
CObjHelper :: QueryAge () const
{
    DWORD dwTime = ::GetCurrentTime(),
          dwDiff ;

    if ( dwTime < m_time_created )
    {
        dwDiff = dwTime + (((DWORD) -1) - (m_time_created - 1));
    }
    else
    {
        dwDiff = dwTime - m_time_created;
    }

    return dwDiff;
}

//
//  Constructor of extended object
//
CObjectPlus :: CObjectPlus ()
{
}

//
//  Compare one object with another:  default implementation
//  orders objects by creation time.  Return -1, 0 or 1.
//
int
CObjectPlus :: Compare (
    const CObjectPlus * pob
    ) const
{
    return QueryCreationTime() < pob->QueryCreationTime()
        ? -1
        : QueryCreationTime() != pob->QueryCreationTime();
}

CObListIter :: CObListIter (
    const CObOwnedList & obList
    )
    : m_obList( obList )
{
    Reset();
}

void
CObListIter :: Reset ()
{
    m_pos = m_obList.GetCount() 
        ? m_obList.GetHeadPosition() 
        : NULL;
}

CObject * CObListIter :: Next ()
{
    return m_pos == NULL
        ? NULL
        : m_obList.GetNext(m_pos);
}

//
//  Subclass of CObList whose default behavior is to destroy
//    its contents during its own destruction
//
CObOwnedList :: CObOwnedList (
    int nBlockSize
    )
    : CObList( nBlockSize ),
      m_b_owned( TRUE )
{
}

CObOwnedList :: ~ CObOwnedList ()
{
    RemoveAll() ;
}

void
CObOwnedList :: RemoveAll ()
{
    if ( m_b_owned )
    {
        //
        //  Remove and discard all the objects
        //
        while ( ! IsEmpty() )
        {
            CObject * pob = RemoveHead() ;
            delete pob ;
        }
    }
    else
    {
        //
        //  Just remove the object pointers
        //
        CObList::RemoveAll() ;
    }
}

CObject *
CObOwnedList :: Index (
    int index
    )
{
   CObListIter oli( *this ) ;

   CObject * pob ;

   for (int i = 0 ; (pob = oli.Next()) && i++ < index ; );

   return pob;
}

CObject *
CObOwnedList :: RemoveIndex (
    int index
    )
{
   POSITION pos ;
   CObListIter oli( *this ) ;
   int i ;
   CObject * pob ;

   for ( i = 0, pos = oli.QueryPosition() ;
     (pob = oli.Next()) && i < index ;
     i++, pos = oli.QueryPosition() );

   if ( pob && i == index )
   {
        RemoveAt(pos);
   }
   else
   {
        pob = NULL;
   }

   return pob;
}

//
//  Remove the first (and hopefully only) occurrence of an object
//  pointer from this list.
//
BOOL
CObOwnedList :: Remove (
    CObject * pob
    )
{
    POSITION pos = Find( pob ) ;

    if (pos == NULL)
    {
        return FALSE;
    }

    RemoveAt(pos);

    return TRUE;
}

//
//  Set all elements to dirty or clean.  Return TRUE if
//  any element was dirty.
//
BOOL
CObOwnedList :: SetAll (
    BOOL bDirty
    )
{
    int cDirtyItems = 0 ;
    CObListIter oli( *this ) ;
    CObjectPlus * pob ;

    while ( pob = (CObjectPlus *) oli.Next() )
    {
        cDirtyItems += pob->IsDirty();
        pob->SetDirty( bDirty );
    }
    SetDirty(bDirty);

    return cDirtyItems > 0;
}

int
CObOwnedList :: FindElement (
    CObject * pobSought
    ) const
{
    CObListIter oli( *this ) ;
    CObject * pob ;

    for ( int i = 0 ;
        (pob = oli.Next()) && pob != pobSought;
        i++ );

    return pob 
        ? i 
        : -1;
}

//
//  Override of CObList::AddTail() to control exception handling.
//  Returns NULL if addition fails.
//
POSITION
CObOwnedList :: AddTail (
    CObjectPlus * pobj,
    BOOL bThrowException
    )
{
    POSITION pos = NULL ;

    //
    //  Catch only memory exceptions.
    //
    TRY
    {
        pos = CObList::AddTail( pobj ) ;
    }
    CATCH( CMemoryException, e )
    {
        pos = NULL ;
    }
    END_CATCH

    if ( pos == NULL && bThrowException )
    {
        //
        //  CObList::AddTail() threw an exception. Echo it.
        //
        ::AfxThrowMemoryException() ;
    }

    return pos ;
}

typedef struct
{
    CObjectPlus * pObj ;            // Pointer to object to be sorted
    CObjectPlus::PCOBJPLUS_ORDER_FUNC  pFunc ;  // Pointer to ordering function
} CBOWNEDLIST_SORT_HELPER ;

//
//  This static member function is used to quick sort an array of structures
//  as declared above.  Each element contains the object pointer and a
//  pointer to the object's member function to be invoked for comparison.
//
//
int _cdecl 
CObOwnedList :: SortHelper (
    const void * pa,
    const void * pb
    )
{
    CBOWNEDLIST_SORT_HELPER
    * pHelp1 = (CBOWNEDLIST_SORT_HELPER *) pa,
    * pHelp2 = (CBOWNEDLIST_SORT_HELPER *) pb ;

    return (pHelp1->pObj->*pHelp1->pFunc)( pHelp2->pObj );
}

//
//  Sort the list by recreating it entirely.
//
LONG
CObOwnedList :: Sort (
    CObjectPlus::PCOBJPLUS_ORDER_FUNC pOrderFunc
    )
{
    LONG err = 0 ;
    int cItems = GetCount() ;

    if ( cItems < 2 )
    {
        return NO_ERROR ;
    }

    CObListIter obli( *this ) ;
    CObjectPlus * pObNext ;
    BOOL bOwned = SetOwnership( FALSE ) ;
    int i ;

    CBOWNEDLIST_SORT_HELPER * paSortHelpers = NULL ;

    CATCH_MEM_EXCEPTION
    {
        //
        //  Allocate the helper array
        //
        paSortHelpers = new CBOWNEDLIST_SORT_HELPER[ cItems ] ;

        //
        // Fill the helper array.
        //
        for ( i = 0 ; pObNext = (CObjectPlus *) obli.Next() ; i++ )
        {
            paSortHelpers[i].pFunc = pOrderFunc ;
            paSortHelpers[i].pObj = pObNext ;
        }

        //
        //  Release all object pointer references.  Note that we
        //  forced "owned" to FALSE above.
        //
        RemoveAll() ;

        ASSERT( GetCount() == 0 ) ;

        //
        //  Sort the helper array
        //
        ::qsort( (void *) paSortHelpers,
            cItems,
            sizeof paSortHelpers[0],
            SortHelper
            );

        //
        //  Refill the list from the helper array.
        //
        for ( i = 0 ; i < cItems ; i++ )
        {
            AddTail( paSortHelpers[i].pObj ) ;
        }

        ASSERT( GetCount() == cItems ) ;
    }
    END_MEM_EXCEPTION(err)

    //
    //  Delete the working array
    //
    delete [] paSortHelpers;

    //
    //  Restore the object ownership state
    //
    SetOwnership(bOwned);

    return err;
}

