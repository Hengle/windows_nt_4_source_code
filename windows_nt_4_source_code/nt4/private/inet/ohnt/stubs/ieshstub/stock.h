/*
 * stock.h - Stock header file.
 */


/* Constants
 ************/

#define FOREVER                  for (;;)

#define INVALID_SEEK_POSITION    (0xffffffff)

#define EMPTY_STRING             ""

#ifdef DEBUG
#define NULL_STRING              "(null)"
#endif

#define SLASH_SLASH              "\\\\"

#define ASTERISK                 '*'
#define BACKSLASH                '/'
#define COLON                    ':'
#define COMMA                    ','
#define EQUAL                    '='
#define PERIOD                   '.'
#define POUND                    '#'
#define QMARK                    '?'
#define QUOTE                    '\''
#define QUOTES                   '"'
#define SLASH                    '\\'
#define SPACE                    ' '
#define TAB                      '\t'

/* linkage */

/* #pragma data_seg() doesn't work for variables defined extern. */
#define PUBLIC_CODE
#define PUBLIC_DATA
/* Make private functions and data public for profiling and debugging. */
#define PRIVATE_CODE             PUBLIC_CODE
#define PRIVATE_DATA             PUBLIC_DATA
#ifdef __cplusplus
#define INLINE                   inline
#else
#define INLINE                   __inline
#endif

/* limits */

#define WORD_MAX                 USHRT_MAX
#define DWORD_MAX                ULONG_MAX

/* file system constants */

#define MAX_PATH_LEN             MAX_PATH
#define MAX_NAME_LEN             MAX_PATH
#define MAX_FOLDER_DEPTH         (MAX_PATH / 2)
#define DRIVE_ROOT_PATH_LEN      (4)

/* ui constants */

#define MAX_MSG_LEN              MAX_PATH_LEN

/* invalid thread ID */

#define INVALID_THREAD_ID        (0xffffffff)

/* no context-sensitive help available */
#ifdef NO_HELP
#undef NO_HELP
#endif
#define NO_HELP                  ((DWORD)-1)

/* Win32 HRESULTs */

#define E_FILE_NOT_FOUND         MAKE_SCODE(SEVERITY_ERROR, FACILITY_WIN32, ERROR_FILE_NOT_FOUND)
#define E_PATH_NOT_FOUND         MAKE_SCODE(SEVERITY_ERROR, FACILITY_WIN32, ERROR_PATH_NOT_FOUND)

/* file-related flag combinations */

#define ALL_FILE_ACCESS_FLAGS          (GENERIC_READ |\
                                        GENERIC_WRITE)

#define ALL_FILE_SHARING_FLAGS         (FILE_SHARE_READ |\
                                        FILE_SHARE_WRITE)

#define ALL_FILE_ATTRIBUTES            (FILE_ATTRIBUTE_READONLY |\
                                        FILE_ATTRIBUTE_HIDDEN |\
                                        FILE_ATTRIBUTE_SYSTEM |\
                                        FILE_ATTRIBUTE_DIRECTORY |\
                                        FILE_ATTRIBUTE_ARCHIVE |\
                                        FILE_ATTRIBUTE_NORMAL |\
                                        FILE_ATTRIBUTE_TEMPORARY |\
                                        FILE_ATTRIBUTE_ATOMIC_WRITE |\
                                        FILE_ATTRIBUTE_XACTION_WRITE)

#define ALL_FILE_FLAGS                 (FILE_FLAG_WRITE_THROUGH |\
                                        FILE_FLAG_OVERLAPPED |\
                                        FILE_FLAG_NO_BUFFERING |\
                                        FILE_FLAG_RANDOM_ACCESS |\
                                        FILE_FLAG_SEQUENTIAL_SCAN |\
                                        FILE_FLAG_DELETE_ON_CLOSE |\
                                        FILE_FLAG_BACKUP_SEMANTICS |\
                                        FILE_FLAG_POSIX_SEMANTICS)

#define ALL_FILE_ATTRIBUTES_AND_FLAGS  (ALL_FILE_ATTRIBUTES |\
                                        ALL_FILE_FLAGS)


/* Macros
 *********/

#ifndef DECLARE_STANDARD_TYPES

/*
 * For a type "FOO", define the standard derived types PFOO, CFOO, and PCFOO.
 */

#define DECLARE_STANDARD_TYPES(type)      typedef type *P##type; \
                                          typedef const type C##type; \
                                          typedef const type *PC##type;

#endif


/*
 * For a type "FOO", define the standard derived UNALIGNED types PFOO, CFOO, and PCFOO.
 */

#define DECLARE_STANDARD_TYPES_U(type)    typedef UNALIGNED type *P##type; \
                                          typedef UNALIGNED const type C##type; \
                                          typedef UNALIGNED const type *PC##type;


/* character manipulation */

#define IS_SLASH(ch)                      ((ch) == SLASH || (ch) == BACKSLASH)

/* bit flag manipulation */

#define SET_FLAG(dwAllFlags, dwFlag)      ((dwAllFlags) |= (dwFlag))
#define CLEAR_FLAG(dwAllFlags, dwFlag)    ((dwAllFlags) &= (~dwFlag))

#define IS_FLAG_SET(dwAllFlags, dwFlag)   ((BOOL)((dwAllFlags) & (dwFlag)))
#define IS_FLAG_CLEAR(dwAllFlags, dwFlag) (! (IS_FLAG_SET(dwAllFlags, dwFlag)))

/* array element count */

#define ARRAY_ELEMENTS(rg)                (sizeof(rg) / sizeof((rg)[0]))

/* string safety */

#define CHECK_STRING(psz)                 ((psz) ? (psz) : NULL_STRING)

/* file attribute manipulation */

#define IS_ATTR_DIR(attr)                 (IS_FLAG_SET((attr), FILE_ATTRIBUTE_DIRECTORY))
#define IS_ATTR_VOLUME(attr)              (IS_FLAG_SET((attr), FILE_ATTRIBUTE_VOLUME))

/* stuff a point value packed in an LPARAM into a POINT */

#define LPARAM_TO_POINT(lparam, pt)       ((pt).x = LOWORD(lparam), \
                                           (pt).y = HIWORD(lparam))


/* Types
 ********/

typedef const void *PCVOID;
typedef const INT CINT;
typedef const INT *PCINT;
typedef const UINT CUINT;
typedef const UINT *PCUINT;
typedef const LONG CULONG;
typedef const LONG *PCULONG;
typedef const BYTE CBYTE;
typedef const BYTE *PCBYTE;
typedef const WORD CWORD;
typedef const WORD *PCWORD;
typedef const DWORD CDWORD;
typedef const DWORD *PCDWORD;
typedef const CRITICAL_SECTION CCRITICAL_SECTION;
typedef const CRITICAL_SECTION *PCCRITICAL_SECTION;
typedef const FILETIME CFILETIME;
typedef const FILETIME *PCFILETIME;
typedef const BITMAPINFO CBITMAPINFO;
typedef const BITMAPINFO *PCBITMAPINFO;
typedef const POINT CPOINT;
typedef const POINT *PCPOINT;
typedef const POINTL CPOINTL;
typedef const POINTL *PCPOINTL;
typedef const SECURITY_ATTRIBUTES CSECURITY_ATTRIBUTES;
typedef const SECURITY_ATTRIBUTES *PCSECURITY_ATTRIBUTES;
typedef const WIN32_FIND_DATA CWIN32_FIND_DATA;
typedef const WIN32_FIND_DATA *PCWIN32_FIND_DATA;

DECLARE_STANDARD_TYPES(HGLOBAL);
DECLARE_STANDARD_TYPES(HICON);
DECLARE_STANDARD_TYPES(HMENU);
DECLARE_STANDARD_TYPES(HWND);
DECLARE_STANDARD_TYPES(NMHDR);

#ifndef _COMPARISONRESULT_DEFINED_

/* comparison result */

typedef enum _comparisonresult
{
   CR_FIRST_SMALLER = -1,
   CR_EQUAL = 0,
   CR_FIRST_LARGER = +1
}
COMPARISONRESULT;
DECLARE_STANDARD_TYPES(COMPARISONRESULT);

#define _COMPARISONRESULT_DEFINED_

#endif

