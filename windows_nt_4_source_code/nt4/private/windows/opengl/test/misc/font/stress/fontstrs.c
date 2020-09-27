#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <commdlg.h>
#include <ptypes32.h>
#include <pwin32.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>

#include <GL/glaux.h>
#include "tk.h"

#include "sscommon.h"
#include "trackbal.h"

static void Draw(void);
static void DrawAuto(void);
static void Init(void);
static void InitLighting(void);
static void Reshape(int, int);
static void SetModelView( void );
static GLenum Key(int key, GLenum mask);

static int GetStringExtent( char *string, POINTFLOAT *extent );
static void CalcStringMetrics( char *string );

static void Init(void);
static void InitFont();
static void KillFont();
static void InitLighting(void);
static void Reshape(int, int);
static GLenum Key(int key, GLenum mask);
VOID Arg( LPSTR );

// Global variables
HDC   hdc;
GLuint  dlFont, dpFont;
GLint firstGlyph = 32;
char singleChar[] = { 65, '\0' };

int numGlyphs=224;
GLint Width = 300, Height = 300;
POINTFLOAT strSize;
int strLen;
GLfloat xRot = 0.0f, yRot = 0.0f, zRot = 0.0f;
GLdouble zTrans = -1.5;
GLfloat extrusion = 0.5f;
GLenum doubleBuffer = GL_TRUE;
GLenum bLighting = GL_TRUE;
GLenum fontStyle = WGL_FONT_POLYGONS;
LPGLYPHMETRICSFLOAT lpgmf;

// output from tesselator always ccw
GLenum orientation = GL_CCW;

char *texFileName = 0;
GLint matIndex = 1;
BOOL bAutoRotate = TRUE;
BOOL bCullBack   = FALSE;
BOOL bTwoSidedLighting = FALSE;

BOOL    bInitTexture = TRUE;
BOOL    bTexture = FALSE;

//char singleChar[] = "A";
char string[] = "12:27";
//char string[] = "OpenGL!";
//char string[] = { 74, 177, 93, 0 };
//char string[] = { 74, 177, 0 };  // intersecting polys
//char string[] = { 93, 0 }; // tess crasher ??
char *drawString = singleChar;

int WINAPI
WinMain(    HINSTANCE   hInstance,
            HINSTANCE   hPrevInstance,
            LPSTR       lpCmdLine,
            int         nCmdShow
        )
{
    GLenum type;

    Arg( lpCmdLine );

    tkInitPosition( 0, 0, Width, Height );

    //type = TK_DEPTH16; // looks bad
    type = TK_DEPTH24;
    type |= TK_RGB;
    type |= (doubleBuffer) ? TK_DOUBLE : TK_SINGLE;
    tkInitDisplayMode(type);

    if (tkInitWindow("Fontal assault") == GL_FALSE) {
	    tkQuit();
    }

    Init();

    tkExposeFunc( Reshape );
    tkReshapeFunc( Reshape );
    tkKeyDownFunc( Key );
    tkMouseDownFunc( trackball_MouseDown );
    tkMouseUpFunc( trackball_MouseUp );

    trackball_Init( Width, Height );

    if( bAutoRotate ) {
        tkIdleFunc( DrawAuto );
        tkDisplayFunc(0);
    } else {
        tkDisplayFunc(Draw);
    }

    tkExec();
    return 1;
}

static GLenum Key(int key, GLenum mask)
{

    switch (key) {
      case TK_ESCAPE:
	    tkQuit();

      case TK_LEFT:
	    yRot -= 5;
	    break;
      case TK_RIGHT:
	    yRot += 5;
	    break;
      case TK_UP:
	    xRot -= 5;
	    break;
      case TK_DOWN:
	    xRot += 5;
	    break;
      case TK_X:
	    zRot += 5;
	    break;
      case TK_x:
	    zRot -= 5;
	    break;
      case TK_Z:
	    zTrans -= strSize.y/10.0;
        SetModelView();
	    break;
      case TK_z:
	    zTrans += strSize.y/10.0;
        SetModelView();
	    break;
      case TK_l:
	    if( bLighting = !bLighting ) {
            glEnable(GL_LIGHTING);
            glEnable(GL_LIGHT0);
        }
        else
        {
            glDisable(GL_LIGHTING);
            glDisable(GL_LIGHT0);
        }
	    break;
      case TK_m:
        if( !bTexture ) {
            if( ++matIndex > NUM_TEA_MATERIALS )
                matIndex =0;
            ss_SetMaterialIndex(matIndex);
        }
	    break;
      case TK_n:
            singleChar[0] += 1;
	    break;
      case TK_N:
            singleChar[0] -= 1;
	    break;
      case TK_t:
        bTexture = !bTexture;
        if( bTexture ) {
            ss_SetMaterialIndex( WHITE );
            glEnable( GL_TEXTURE_2D );
            glEnable(GL_TEXTURE_GEN_S);
            glEnable(GL_TEXTURE_GEN_T);
        } else {
            ss_SetMaterialIndex( matIndex );
            glDisable( GL_TEXTURE_2D );
            glDisable(GL_TEXTURE_GEN_S);
            glDisable(GL_TEXTURE_GEN_T);
        }
        break;
      case TK_o:
	    orientation = (orientation == GL_CCW) ? GL_CW : GL_CCW;
        glFrontFace( orientation );
	    break;
      case TK_s:
        // switch between wireframe and solid polygons
	    fontStyle = (fontStyle == WGL_FONT_LINES) ? WGL_FONT_POLYGONS :
                    WGL_FONT_LINES;
	    break;

      case TK_c:
        // switch between wireframe and solid polygons
        drawString = (drawString == singleChar) ? string : singleChar;
        CalcStringMetrics( drawString );
        // change viewing matrix based on string extent
        SetModelView();
        break;

      case TK_u:
        // cUllface stuff
        bCullBack = !bCullBack;
        if( bCullBack ) {
            glCullFace( GL_BACK );
            glEnable( GL_CULL_FACE );
        } else {
            glDisable( GL_CULL_FACE );
        }
        break;

      case TK_2:
        bTwoSidedLighting = !bTwoSidedLighting;
        if( bTwoSidedLighting ) {
            glLightModeli( GL_LIGHT_MODEL_TWO_SIDE, 1 );
        } else {
            glLightModeli( GL_LIGHT_MODEL_TWO_SIDE, 0 );
        }
        break;

      case TK_a:
        // animate or not
        bAutoRotate= !bAutoRotate;
        if( bAutoRotate ) {
            tkIdleFunc(DrawAuto);
            tkDisplayFunc(0);
        } else {
            tkIdleFunc(0);
            tkDisplayFunc(Draw);
        }
        break;

      default:
	    return GL_FALSE;
    }
    return GL_TRUE;
}

static void InitLighting(void)
{
    static float ambient[] = {0.1f, 0.1f, 0.1f, 1.0f};
    static float diffuse[] = {1.0f, 1.0f, 1.0f, 1.0f};
    static float position[] = {0.0f, 0.0f, 150.0f, 0.0f};

    static float back_mat_shininess[] = {50.0f};
    static float back_mat_specular[] = {0.5f, 0.5f, 0.2f, 1.0f};
    static float back_mat_diffuse[] = {1.0f, 0.0f, 0.0f, 1.0f};
    static float lmodel_ambient[] = {1.0f, 1.0f, 1.0f, 1.0f};
    static float decal[] = {(GLfloat) GL_DECAL}; 
    static float modulate[] = {(GLfloat) GL_MODULATE};
    static float repeat[] = {(GLfloat) GL_REPEAT}; 
    static float nearest[] = {(GLfloat) GL_NEAREST};
    TK_RGBImageRec *image;

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);
    glFrontFace( orientation );

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, position);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
    if( bLighting ) {
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
    }

    // override back material
    glMaterialfv( GL_BACK, GL_AMBIENT, ambient );
    glMaterialfv( GL_BACK, GL_DIFFUSE, back_mat_diffuse );
    glMaterialfv( GL_BACK, GL_SPECULAR, back_mat_specular );
    glMaterialfv( GL_BACK, GL_SHININESS, back_mat_shininess );

    if( bTwoSidedLighting ) {
        glLightModeli( GL_LIGHT_MODEL_TWO_SIDE, 1 );
    } else {
        glLightModeli( GL_LIGHT_MODEL_TWO_SIDE, 0 );
    }

    // set back face culling mode

    if( bCullBack ) {
        glCullFace( GL_BACK );
        glEnable( GL_CULL_FACE );
    } else {
        glDisable( GL_CULL_FACE );
    }

    // Initialize materials from screen saver common lib
    ss_InitMaterials();
    matIndex = JADE;
    ss_SetMaterialIndex( matIndex );

    // do texturing preparations

    if( bInitTexture ) {
        //glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, decal);
        glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, modulate);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeat);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeat);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, nearest);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, nearest);
        if (texFileName) {
	        image = tkRGBImageLoad(texFileName);
	        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	        gluBuild2DMipmaps(GL_TEXTURE_2D, 3, image->sizeX, image->sizeY,
			              GL_RGB, GL_UNSIGNED_BYTE, image->data);
        }
        glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
        glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
        if( bTexture ) {
            glEnable( GL_TEXTURE_2D );
            glEnable(GL_TEXTURE_GEN_S);
            glEnable(GL_TEXTURE_GEN_T);
            ss_SetMaterialIndex( WHITE );
        }
    }
}

void Init(void)
{
    CHOOSEFONT cf;
    LOGFONT    lf;
    HFONT      hfont, hfontOld;
    HWND  hwnd;

    hdc = tkGetHDC();
    hwnd = tkGetHWND();

    // Create and select a font.

    memset(&lf, 0, sizeof(LOGFONT));
    lstrcpy(lf.lfFaceName, "Arial");
    lf.lfHeight = 50;

    // lf reflects user's final choices

    hfont = CreateFontIndirect(&lf);

    hfontOld = SelectObject(hdc, hfont);

    /* Set the clear color */

    glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );

    // Initialize lighting
    InitLighting();
}

static void Reshape( int width, int height )
{
    static GLdouble zNear = 0.1, zFar = 1000.0, fovy = 90.0;
    float aspect;

    trackball_Resize( width, height );

    /* Set up the projection matrix */

    Width = width;
    Height = height;
    aspect = Height == 0 ? 1.0f : Width/(float)Height;

    glViewport(0, 0, Width, Height );
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    zFar = 10.0f;
    gluPerspective(fovy, aspect, zNear, zFar );
    glMatrixMode(GL_MODELVIEW);

    SetModelView();
}

static void SetModelView( void )
{
    glLoadIdentity();
#if 1
    glTranslated( 0, 0, zTrans );
#else
    /* this has side effect of flipping around viewpoint when pass through
       origin
    */
    gluLookAt( 0, 0, -zTrans,
               0, 0, 0,
               0, 1, 0 );
#endif
}

static void InitFont()
{
    static GLfloat     chordal_deviation = 0.0f;

    // Generate a display list for font

    dlFont = glGenLists(numGlyphs);
    dpFont = glGenLists(numGlyphs);

    lpgmf = (LPGLYPHMETRICSFLOAT) LocalAlloc( LMEM_FIXED, numGlyphs *
                                    sizeof(GLYPHMETRICSFLOAT) );
    if( (chordal_deviation += 0.001f) > .01f )
        chordal_deviation = 0.0f;

    if( (extrusion += 0.1f) > 1.0f )
        extrusion = 0.0f;

    wglUseFontOutlinesA(hdc, firstGlyph, numGlyphs, dpFont, 
                   chordal_deviation, extrusion, 
                   WGL_FONT_POLYGONS, lpgmf );

    CalcStringMetrics( drawString );
    LocalFree( lpgmf );
}

static void DrawAuto(void)
{
    POINT pt;
    float  matRot[4][4];

    InitFont();

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    if( fontStyle == WGL_FONT_LINES )
        glListBase(dlFont-firstGlyph);
    else
        glListBase(dpFont-firstGlyph);

    glPushMatrix();

    glRotatef( yRot+= 5.0f, 0.0f, 1.0f, 0.0f );

    // now draw text, centered in window

    glTranslated( -strSize.x/2.0, -strSize.y/2.0, extrusion/2.0 );

    glCallLists(strLen, GL_UNSIGNED_BYTE, (GLubyte *) drawString);

    glPopMatrix();

    glFlush();
    if( doubleBuffer )
        tkSwapBuffers();

    KillFont();

    // increment character drawn
    if( singleChar[0] < (firstGlyph+numGlyphs-1) )
        singleChar[0] += 1;
    else
        singleChar[0] = (char) firstGlyph;
}

static void KillFont()
{

    // delete the display lists
    glDeleteLists( dlFont, numGlyphs );
    glDeleteLists( dpFont, numGlyphs );
}

static void Draw(void)
{

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    if( fontStyle == WGL_FONT_LINES )
        glListBase(dlFont-firstGlyph);
    else
        glListBase(dpFont-firstGlyph);

    glPushMatrix();

    glRotatef( xRot, 1.0f, 0.0f, 0.0f );
    glRotatef( yRot, 0.0f, 1.0f, 0.0f );
    glRotatef( zRot, 0.0f, 0.0f, 1.0f );

    // center the string in window
    glTranslated( -strSize.x/2.0, -strSize.y/2.0, extrusion/2.0 );
    glCallLists(strLen, GL_UNSIGNED_BYTE, (GLubyte *) drawString);

    glPopMatrix();

    glFlush();
    if( doubleBuffer )
        tkSwapBuffers();
}

static int GetStringExtent( char *string, POINTFLOAT *extent )
{
    int len, strLen;
    unsigned char *c;
    GLYPHMETRICSFLOAT *pgmf;

    extent->x = extent->y = 0.0f;
    len = strLen = lstrlenA( string );
    c = string;

    for( ; strLen; strLen--, c++ ) {
        if( *c < firstGlyph )
            continue;
        pgmf = &lpgmf[ *c - firstGlyph ];
        extent->x += pgmf->gmfCellIncX;
        if( pgmf->gmfBlackBoxY > extent->y )
            extent->y = pgmf->gmfBlackBoxY;
    }
    return len;
}

static void CalcStringMetrics( char *string )
{
    // get string size for window reshape
    strLen = GetStringExtent( string, &strSize );

    // set zTrans based on glyph extent
    zTrans = -5.0 * (strSize.x / 2.0f);
}

VOID Arg(LPSTR lpstr)
{
    while (*lpstr && *lpstr != '-') lpstr++;
    // only one arg for now (e.g.: -f 3.rgb)
    lpstr++;
    if( *lpstr++ == 'f' ) {
        texFileName = ++lpstr; 
    }
}
