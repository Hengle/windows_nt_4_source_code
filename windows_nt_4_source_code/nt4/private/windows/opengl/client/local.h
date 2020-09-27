/******************************Module*Header*******************************\
* Module Name: local.h                                                     *
*                                                                          *
* Definitions needed for client side objects and attribute caching.        *
*                                                                          *
* Modified: 3-Aug-1992 22:35:30 by Gerrit van Wingerden [gerritv]          *
*   Added client side transform support.                                   *
*                                                                          *
* Created: 30-May-1991 21:55:01                                            *
* Author: Charles Whitmer [chuckwh]                                        *
*                                                                          *
* Copyright (c) 1993 Microsoft Corporation                                 *
\**************************************************************************/

/**************************************************************************\
 *
 * Local handle macros
 *
\**************************************************************************/

// Handle uniqueness is nice to check but an unnecesary performance cost in
// a free build.

// To match the uniqness field:  If the handle uniqness == 0, let it through
// anyway.  This is a method for WOW to only keep track of the low 16 bits but
// still get reasonable performance.  Even if a 32 bit app does this, all it
// can do is hose it self, not the system or another app.

#define INDEX_MASK  0xFFFF
#define UNIQ_SHIFT  16
#define UNIQ_MASK   0xFFFF
#define HIPART(x) *(((USHORT *) &(x))+1)
#define MATCHUNIQ(plhe,h) ((USHORT) plhe->iUniq == HIPART(h))
#define MASKINDEX(h) ((UINT)h & INDEX_MASK)
#define LHANDLE(i)  (i+((ULONG)pLocalTable[i].iUniq<<UNIQ_SHIFT))

//!!!XXX -- Do we really need typing?  Not really, but we may add more
//!!!XXX    later.  So eventually we might take it out, but its nice for now.

// Define the types of local objects.

enum LO_TYPE
{
    LO_NULL,
    LO_RC,
    LO_LAST
};

#define INVALID_INDEX 0xFFFFFFFFL
#define COMMIT_COUNT  (4096/sizeof(LHE))
#define MAX_HANDLES (16384/COMMIT_COUNT)*COMMIT_COUNT

// Define a Local Handle Entry.  Our Local Handle Table, pLocalTable, is an
// array of these.

typedef struct _LHE
{
    ULONG  hgre;        // GRE Handle.
    USHORT cRef;        // Reference count of the object.
    BYTE   iType;       // Object type.
    BYTE   iUniq;       // Handle uniqueness field.  Always non-zero.
    PVOID  pv;          // Pointer to local object.
    ULONG  metalink;    // Non-zero if object is a "metafile friend".
                        // Points to a metafile DC object if it's a metafile.
                        // Also links the free list.
    DWORD  tidOwner;    // Per-thread lock owner.
    LONG   cLock;       // Lock count.
} LHE,*PLHE;

extern LHE                  *pLocalTable;   // Points to handle table.
extern ULONG                 iFreeLhe;      // Identifies a free handle index.
extern ULONG                 cLheCommitted; // Count of LHEs with committed RAM.
extern CRITICAL_SECTION      semLocal;      // Semaphore for handle allocation.
extern CRITICAL_SECTION      wfo_cs;        // Semaphore for wglUseFontOutlines


// Semaphore utilities

#define INITIALIZECRITICALSECTION(psem) InitializeCriticalSection((psem))
#define ENTERCRITICALSECTION(hsem)      EnterCriticalSection((hsem))
#define LEAVECRITICALSECTION(hsem)      LeaveCriticalSection((hsem))
#define DELETECRITICALSECTION(psem)     DeleteCriticalSection((psem))

// Local data structures

// Maximum OpenGL driver name

#define MAX_GLDRIVER_NAME   MAX_PATH

// GetCurrentThreadID will never return this value

#define INVALID_THREAD_ID   0

// Driver context function prototypes

typedef BOOL            (APIENTRY *PFN_DRVVALIDATEVERSION) (ULONG);
typedef VOID            (APIENTRY *PFN_DRVSETCALLBACKPROCS)(INT, PROC *);

// Driver data

typedef struct _GLDRIVER {
    HINSTANCE             hModule;             // Module handle

    // Driver function pointers

    // Required
    DHGLRC          (APIENTRY *pfnDrvCreateContext)(HDC);
    BOOL            (APIENTRY *pfnDrvDeleteContext)(DHGLRC);
    PGLCLTPROCTABLE (APIENTRY *pfnDrvSetContext)(HDC, DHGLRC,
                                                 PFN_SETPROCTABLE);
    BOOL            (APIENTRY *pfnDrvReleaseContext)(DHGLRC);

    // Optional
    BOOL            (APIENTRY *pfnDrvCopyContext)(DHGLRC, DHGLRC, UINT);
    DHGLRC          (APIENTRY *pfnDrvCreateLayerContext)(HDC, int);
    BOOL            (APIENTRY *pfnDrvShareLists)(DHGLRC, DHGLRC);
    PROC            (APIENTRY *pfnDrvGetProcAddress)(LPCSTR);
    BOOL            (APIENTRY *pfnDrvDescribeLayerPlane)(HDC, INT, INT, UINT,
                                                      LPLAYERPLANEDESCRIPTOR);
    INT             (APIENTRY *pfnDrvSetLayerPaletteEntries)(HDC, INT, INT,
                                                             INT,
                                                             CONST COLORREF *);
    INT             (APIENTRY *pfnDrvGetLayerPaletteEntries)(HDC, INT, INT,
                                                             INT, COLORREF *);
    BOOL            (APIENTRY *pfnDrvRealizeLayerPalette)(HDC, INT, BOOL);
    BOOL            (APIENTRY *pfnDrvSwapLayerBuffers)(HDC, UINT);
    
#if defined(_WIN95_)
    LONG            (APIENTRY *pfnDrvDescribePixelFormat)(HDC, LONG, ULONG,
                                                      PIXELFORMATDESCRIPTOR *);
    BOOL            (APIENTRY *pfnDrvSetPixelFormat)(HDC, LONG);
    BOOL            (APIENTRY *pfnDrvSwapBuffers)(HDC);
#endif

    struct _GLDRIVER    *pGLDriver;            // Next loaded GL driver
    WCHAR wszDrvName[MAX_GLDRIVER_NAME+1];     // Null terminated unicode
                                               //   driver name
} GLDRIVER, *PGLDRIVER;

extern PGLDRIVER APIENTRY pgldrvLoadInstalledDriver(HDC hdc);


/****************************************************************************/

#ifdef NT_DEADCODE_VERTEXARRAY
// vertex array data

typedef (*PFNCLTVECTOR)(GLbyte* p);

typedef struct _CLTARRAYPOINTER *PCLTARRAYPOINTER;
typedef struct _CLTARRAYPOINTER {
    GLint size;
    GLenum type;
    GLsizei stride;
    GLsizei count;
    GLbyte* pointer;

    BOOL fEnabled;
    PFNCLTVECTOR pfn;
    GLsizei ibytes;
} CLTARRAYPOINTER;

void            APIENTRY VArrayEnable     (IN GLenum cap);
void            APIENTRY VArrayDisable    (IN GLenum cap);
GLboolean       APIENTRY VArrayIsEnabled  (IN GLenum cap);
void            APIENTRY VArrayGetBooleanv(IN GLenum pname, OUT GLboolean params[]);
void            APIENTRY VArrayGetDoublev (IN GLenum pname, OUT GLdouble params[]);
void            APIENTRY VArrayGetFloatv  (IN GLenum pname, OUT GLfloat params[]);
void            APIENTRY VArrayGetIntegerv(IN GLenum pname, OUT GLint params[]);
const GLubyte * APIENTRY VArrayGetString  ( IN GLenum name );

typedef void           (APIENTRY* PFNENABLE     )(IN GLenum cap);
typedef void           (APIENTRY* PFNDISABLE    )(IN GLenum cap);
typedef GLboolean      (APIENTRY* PFNISENABLED  )(IN GLenum cap);
typedef void           (APIENTRY* PFNGETBOOLEANV)(IN GLenum pname, OUT GLboolean params[]);
typedef void           (APIENTRY* PFNGETDOUBLEV )(IN GLenum pname, OUT GLdouble params[]);
typedef void           (APIENTRY* PFNGETFLOATV  )(IN GLenum pname, OUT GLfloat params[]);
typedef void           (APIENTRY* PFNGETINTEGERV)(IN GLenum pname, OUT GLint params[]);
typedef const GLubyte* (APIENTRY* PFNGETSTRING  )(GLenum name);
#endif // NT_DEADCODE_VERTEXARRAY

void APIENTRY glAddSwapHintRectWIN(IN GLint, IN GLint, IN GLint, IN GLint);

void glColorTableEXT( GLenum target, GLenum internalFormat, GLsizei width, GLenum format, GLenum type, const GLvoid *data);
void glColorSubTableEXT( GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid *data);
void glGetColorTableEXT( GLenum target, GLenum format, GLenum type, GLvoid *data);
void glGetColorTableParameterivEXT( GLenum target, GLenum pname, GLint *params);
void glGetColorTableParameterfvEXT( GLenum target, GLenum pname, GLfloat *params);

/****************************************************************************/

// Local RC object

#define LRC_IDENTIFIER    0x2043524C    /* 'LRC ' */

typedef struct _LRC {
    DHGLRC    dhrc;             // Driver handle
    HGLRC     hrc;              // Client handle
    int       iPixelFormat;     // Pixel format index
    DWORD     ident;            // LRC_IDENTIFIER
    DWORD     tidCurrent;       // Thread id if the DC is current,
                                //   INVALID_THREAD_ID otherwise
    PGLDRIVER pGLDriver;        // Driver data
    HDC       hdcCurrent;       // hdc associated with the current context

#ifdef GL_METAFILE
    GLuint    uiGlsCaptureContext;  // GLS capturing context for metafile RC's
    GLuint    uiGlsPlaybackContext; // GLS context for playback
    BOOL      fCapturing;       // GLS is in BeginCapture
    HDC       hdcMeta;          // GLS metafile context, needed even
                                // when the RC is not current
    
    // GLS playback scaling factors
    int iGlsSubtractX;
    int iGlsSubtractY;
    int iGlsNumeratorX;
    int iGlsNumeratorY;
    int iGlsDenominatorX;
    int iGlsDenominatorY;
    int iGlsAddX;
    int iGlsAddY;
    GLfloat fGlsScaleX;
    GLfloat fGlsScaleY;
#endif

#ifdef NT_DEADCODE_VERTEXARRAY
    // vertex array data

    CLTARRAYPOINTER apVertex;
    CLTARRAYPOINTER apNormal;
    CLTARRAYPOINTER apColor;
    CLTARRAYPOINTER apIndex;
    CLTARRAYPOINTER apTexCoord;
    CLTARRAYPOINTER apEdgeFlag;

    PFNENABLE      pfnEnable     ;
    PFNDISABLE     pfnDisable    ;
    PFNISENABLED   pfnIsEnabled  ;
    PFNGETBOOLEANV pfnGetBooleanv;
    PFNGETDOUBLEV  pfnGetDoublev ;
    PFNGETFLOATV   pfnGetFloatv  ;
    PFNGETINTEGERV pfnGetIntegerv;
    PFNGETSTRING   pfnGetString  ;
#endif // NT_DEADCODE_VERTEXARRAY

    GLubyte *pszExtensions;

#ifdef GL_METAFILE
    XFORM xformMeta;            // World transform storage during GLS blocks
    LPRECTL prclGlsBounds;      // Bounds during GLS recording
#endif
} LRC, *PLRC;

// Various dispatch tables available
extern GLCLTPROCTABLE glNullCltProcTable;
extern GLCLTPROCTABLE glCltRGBAProcTable;
extern GLCLTPROCTABLE glCltCIProcTable;
extern GLEXTPROCTABLE glNullExtProcTable;
extern GLEXTPROCTABLE glExtProcTable;
#ifdef GL_METAFILE
extern GLCLTPROCTABLE gcptGlsProcTable;
extern GLEXTPROCTABLE geptGlsExtProcTable;
#endif

// Declare support functions.

ULONG   iAllocHandle(ULONG iType,ULONG hgre,PVOID pv);
VOID    vFreeHandle(ULONG h);
LONG    cLockHandle(ULONG h);
VOID    vUnlockHandle(ULONG h);
VOID    vCleanupAllLRC(VOID);

#define GdiSetLastError(x)  SetLastError(x)

VOID GLInitializeThread(ULONG ulReason);
