/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :

      perfutil.h

   Abstract:

      This file supports routines used to parse and create Performance Monitor
       Data structures, used by all the Internet Services product.

   Author:

       Murali R. Krishnan    ( MuraliK )    16-Nov-1995  
         From the common code for perfmon interface (Russ Blake's).

   Environment:

      User Mode

   Project:
   
       Internet Services Common Runtime code

   Revision History:

--*/

# ifndef _PERFUTIL_H_
# define _PERFUTIL_H_

/************************************************************
 *     Symbolic Constants
 ************************************************************/


#define QUERY_GLOBAL    1
#define QUERY_ITEMS     2
#define QUERY_FOREIGN   3
#define QUERY_COSTLY    4


/************************************************************
 *     Function Declarations
 ************************************************************/

DWORD
GetQueryType (IN LPWSTR lpwszValue);

BOOL
IsNumberInUnicodeList (IN DWORD dwNumber, IN LPWSTR lpwszUnicodeList);


# endif // _PERFUTIL_H_

/************************ End of File ***********************/
