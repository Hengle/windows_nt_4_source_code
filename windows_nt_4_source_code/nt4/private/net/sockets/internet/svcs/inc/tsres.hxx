/*++

   Copyright    (c)    1994    Microsoft Corporation

   Module  Name :

        tsres.hxx

   Abstract:

        Defines a wrapper class for locks ( for TCP Services Resources)
         used for light-weight syncronization and portability to Win95.

   Author:

           John Ludeman     ( JohnL)    21-Nov-1994

   Project:

           TCP Services Common DLL

   Revision History:

--*/

# ifndef _TSRES_HXX_
# define _TSRES_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/

# ifdef __cplusplus
extern "C" {
# endif // __cplusplus

# include <nt.h>
# include <ntrtl.h>

# ifdef __cplusplus
}; // extern "C"
# endif // __cplusplus


# ifndef TCP_REQUIRE
#define TCP_REQUIRE(x)            DBG_REQUIRE(x)
# endif // TCP_REQUIRE

# ifndef TCP_ASSERT
#define TCP_ASSERT(x)           DBG_ASSERT(x)
# endif // TCP_REQUIRE

/************************************************************
 *   Type Definitions
 ************************************************************/

# ifdef __cplusplus
extern "C" {
# endif // __cplusplus

dllexp
BOOL
InetInitializeResource(
    IN PRTL_RESOURCE Resource
    );

dllexp
BOOL
InetAcquireResourceShared(
    IN PRTL_RESOURCE Resource,
    IN BOOL          Wait
    );

dllexp
BOOL
InetAcquireResourceExclusive(
    IN PRTL_RESOURCE Resource,
    IN BOOL Wait
    );

dllexp
BOOL
InetReleaseResource(
    IN PRTL_RESOURCE Resource
    );

dllexp
BOOL
InetConvertSharedToExclusive(
    IN PRTL_RESOURCE Resource
    );

dllexp
BOOL
InetConvertExclusiveToShared(
    IN PRTL_RESOURCE Resource
    );

dllexp
VOID
InetDeleteResource (
    IN PRTL_RESOURCE Resource
    );

# ifdef __cplusplus
}; // extern "C"
# endif // __cplusplus


///////////////////////////////////////////////////////////////////////
//
//  Simple RTL_RESOURCE Wrapper class
//
//////////////////////////////////////////////////////////////////////

enum TSRES_LOCK_TYPE
{
	TSRES_LOCK_READ = 0,        // Take the lock for read only
	TSRES_LOCK_WRITE            // Take the lock for write
};

enum TSRES_CONV_TYPE
{
	TSRES_CONV_READ = 0,        // Convert the lock from write to read
	TSRES_CONV_WRITE            // Convert the lock from read to write
};

class TS_RESOURCE
{
public:
	
	TS_RESOURCE()
		{ DBG_REQUIRE( InetInitializeResource( &_rtlres )); }

	~TS_RESOURCE()
		{ InetDeleteResource( &_rtlres ); }

	void Lock( enum TSRES_LOCK_TYPE type )
		{ if ( type == TSRES_LOCK_READ ) {
		      DBG_REQUIRE( InetAcquireResourceShared( &_rtlres, TRUE ) );
           } else {
			  DBG_REQUIRE( InetAcquireResourceExclusive( &_rtlres, TRUE ));
           }
		}

	void Convert( enum TSRES_CONV_TYPE type )
		{ if ( type == TSRES_CONV_READ ) {
		      DBG_REQUIRE( InetConvertExclusiveToShared( &_rtlres ));
          } else {
			  DBG_REQUIRE( InetConvertSharedToExclusive( &_rtlres ));
          }
		}

	void Unlock( VOID )
		{ DBG_REQUIRE( InetReleaseResource( &_rtlres )); }

private:
	RTL_RESOURCE _rtlres;
};



# endif // _TSRES_HXX_

/************************ End of File ***********************/
