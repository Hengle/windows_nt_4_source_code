/*
** Copyright 1994, Silicon Graphics, Inc.
** All Rights Reserved.
** 
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of Silicon Graphics, Inc.
** 
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
** rights reserved under the Copyright Laws of the United States.
**
** Author: Eric Veach, July 1994.
*/

#include <assert.h>
#include "memalloc.h"
#include "tess.h"
#include "mesh.h"
#include "normal.h"
#include "sweep.h"
#include "tessmono.h"
#include "render.h"

#define GLU_TESS_DEFAULT_TOLERANCE 0.0
#ifndef NT
#define GLU_TESS_MESH		100112	/* void (*)(GLUmesh *mesh)	    */
#endif

#define TRUE 1
#define FALSE 0

/*ARGSUSED*/ static void noBegin( GLenum type ) {}
/*ARGSUSED*/ static void noEdgeFlag( GLboolean boundaryEdge ) {}
/*ARGSUSED*/ static void noVertex( void *data ) {}
/*ARGSUSED*/ static void noEnd( void ) {}
/*ARGSUSED*/ static void noError( GLenum errno ) {}
/*ARGSUSED*/ static void noCombine( GLdouble coords[3], void *data[4],
                                    GLfloat weight[4], void **dataOut ) {}
/*ARGSUSED*/ static void noMesh( GLUmesh *mesh ) {}


/*ARGSUSED*/ void __gl_noBeginData( GLenum type, void *polygonData ) {}
/*ARGSUSED*/ void __gl_noEdgeFlagData( GLboolean boundaryEdge, 
				       void *polygonData ) {}
/*ARGSUSED*/ void __gl_noVertexData( void *data, void *polygonData ) {}
/*ARGSUSED*/ void __gl_noEndData( void *polygonData ) {}
/*ARGSUSED*/ void __gl_noErrorData( GLenum errno, void *polygonData ) {}
/*ARGSUSED*/ void __gl_noCombineData( GLdouble coords[3], void *data[4],
			 GLfloat weight[4], void **outData,
			 void *polygonData ) {}

/* Half-edges are allocated in pairs (see mesh.c) */
typedef struct { GLUhalfEdge e, eSym; } EdgePair;

#define MAX(a,b)	((a) > (b) ? (a) : (b))
#define MAX_FAST_ALLOC	(MAX(sizeof(EdgePair), \
			 MAX(sizeof(GLUvertex),sizeof(GLUface))))


#ifdef NT
GLUtesselator* APIENTRY gluNewTess( void )
#else
GLUtesselator *gluNewTess( void )
#endif
{
  GLUtesselator *tess;

  /* Only initialize fields which can be changed by the api.  Other fields
   * are initialized where they are used.
   */

  if (memInit( MAX_FAST_ALLOC ) == 0) {
     return 0;			/* out of memory */
  }
  tess = (GLUtesselator *)memAlloc( sizeof( GLUtesselator ));
  if (tess == NULL) {
     return 0;			/* out of memory */
  }

  tess->state = T_DORMANT;

  tess->normal[0] = 0;
  tess->normal[1] = 0;
  tess->normal[2] = 0;

  tess->relTolerance = GLU_TESS_DEFAULT_TOLERANCE;
  tess->windingRule = GLU_TESS_WINDING_ODD;
  tess->flagBoundary = FALSE;
  tess->boundaryOnly = FALSE;

  tess->callBegin = &noBegin;
  tess->callEdgeFlag = &noEdgeFlag;
  tess->callVertex = &noVertex;
  tess->callEnd = &noEnd;

  tess->callError = &noError;
  tess->callCombine = &noCombine;
  tess->callMesh = &noMesh;

  tess->callBeginData= &__gl_noBeginData;
  tess->callEdgeFlagData= &__gl_noEdgeFlagData;
  tess->callVertexData= &__gl_noVertexData;
  tess->callEndData= &__gl_noEndData;
  tess->callErrorData= &__gl_noErrorData;
  tess->callCombineData= &__gl_noCombineData;

  tess->polygonData= NULL;

  return tess;
}

static void MakeDormant( GLUtesselator *tess )
{
  /* Return the tesselator to its original dormant state. */

  if( tess->mesh != NULL ) {
    __gl_meshDeleteMesh( tess->mesh );
  }
  tess->state = T_DORMANT;
  tess->lastEdge = NULL;
  tess->mesh = NULL;
}

#define RequireState( tess, s )   if( tess->state != s ) GotoState(tess,s)

static void GotoState( GLUtesselator *tess, enum TessState newState )
{
#ifdef NT
  while( tess->state != (GLenum) newState ) {
#else
  while( tess->state != newState ) {
#endif
    /* We change the current state one level at a time, to get to
     * the desired state.
     */
#ifdef NT
    if( tess->state < (GLenum) newState ) {
#else
    if( tess->state < newState ) {
#endif
      switch( tess->state ) {
      case T_DORMANT:
	CALL_ERROR_OR_ERROR_DATA( GLU_TESS_MISSING_BEGIN_POLYGON );
	gluTessBeginPolygon( tess, NULL );
	break;
      case T_IN_POLYGON:
	CALL_ERROR_OR_ERROR_DATA( GLU_TESS_MISSING_BEGIN_CONTOUR );
	gluTessBeginContour( tess );
	break;
      }
    } else {
      switch( tess->state ) {
      case T_IN_CONTOUR:
	CALL_ERROR_OR_ERROR_DATA( GLU_TESS_MISSING_END_CONTOUR );
	gluTessEndContour( tess );
	break;
      case T_IN_POLYGON:
	CALL_ERROR_OR_ERROR_DATA( GLU_TESS_MISSING_END_POLYGON );
	/* gluTessEndPolygon( tess ) is too much work! */
	MakeDormant( tess );
	break;
      }
    }
  }
}


#ifdef NT
void APIENTRY gluDeleteTess( GLUtesselator *tess )
#else
void gluDeleteTess( GLUtesselator *tess )
#endif
{
  RequireState( tess, T_DORMANT );
  memFree( tess );
}


#ifdef NT
void APIENTRY gluTessProperty( GLUtesselator *tess, GLenum which, GLdouble value )
#else
void gluTessProperty( GLUtesselator *tess, GLenum which, GLdouble value )
#endif
{
  GLenum windingRule;

  switch( which ) {
  case GLU_TESS_TOLERANCE:
    if( value < 0.0 || value > 1.0 ) break;
    tess->relTolerance = value;
    return;

  case GLU_TESS_WINDING_RULE:
    windingRule = (GLenum) value;
    if( windingRule != value ) break;	/* not an integer */

    switch( windingRule ) {
    case GLU_TESS_WINDING_ODD:
    case GLU_TESS_WINDING_NONZERO:
    case GLU_TESS_WINDING_POSITIVE:
    case GLU_TESS_WINDING_NEGATIVE:
    case GLU_TESS_WINDING_ABS_GEQ_TWO:
      tess->windingRule = windingRule;
      return;
    default:
      break;
    }

  case GLU_TESS_BOUNDARY_ONLY:
    tess->boundaryOnly = (value != 0);
    return;

  default:
    CALL_ERROR_OR_ERROR_DATA( GLU_INVALID_ENUM );
    return;
  }
  CALL_ERROR_OR_ERROR_DATA( GLU_INVALID_VALUE );
}

/* Returns tesselator property */
#ifdef NT
void APIENTRY gluGetTessProperty( GLUtesselator *tess, GLenum which, GLdouble *value )
#else
void gluGetTessProperty( GLUtesselator *tess, GLenum which, GLdouble *value )
#endif
{
   switch (which) {
   case GLU_TESS_TOLERANCE:
      /* tolerance should be in range [0..1] */
      assert(0.0 <= tess->relTolerance && tess->relTolerance <= 1.0);
      *value= tess->relTolerance;
      break;    
   case GLU_TESS_WINDING_RULE:
      assert(tess->windingRule == GLU_TESS_WINDING_ODD ||
	     tess->windingRule == GLU_TESS_WINDING_NONZERO ||
	     tess->windingRule == GLU_TESS_WINDING_POSITIVE ||
	     tess->windingRule == GLU_TESS_WINDING_NEGATIVE ||
	     tess->windingRule == GLU_TESS_WINDING_ABS_GEQ_TWO);
      *value= tess->windingRule;
      break;
   case GLU_TESS_BOUNDARY_ONLY:
      assert(tess->boundaryOnly == TRUE || tess->boundaryOnly == FALSE);
      *value= tess->boundaryOnly;
      break;
   default:
      *value= 0.0;
      CALL_ERROR_OR_ERROR_DATA( GLU_INVALID_ENUM );
      break;
   }
} /* gluGetTessProperty() */

#ifdef NT
void APIENTRY gluTessNormal( GLUtesselator *tess, GLdouble x, GLdouble y, GLdouble z )
#else
void gluTessNormal( GLUtesselator *tess, GLdouble x, GLdouble y, GLdouble z )
#endif
{
  tess->normal[0] = x;
  tess->normal[1] = y;
  tess->normal[2] = z;
}

#ifdef NT
void APIENTRY gluTessCallback( GLUtesselator *tess, GLenum which, void (*fn)())
#else
void gluTessCallback( GLUtesselator *tess, GLenum which, void (*fn)())
#endif
{
  switch( which ) {
  case GLU_TESS_BEGIN:
    tess->callBegin = (fn == NULL) ? &noBegin : (void (*)(GLenum)) fn;
    return;
  case GLU_TESS_BEGIN_DATA:
    tess->callBeginData = (fn == NULL) ? &__gl_noBeginData : 
                                         (void (*)(GLenum, void *)) fn;
    return;
  case GLU_TESS_EDGE_FLAG:
    tess->callEdgeFlag = (fn == NULL) ? &noEdgeFlag : (void (*)(GLboolean)) fn;
    /* If the client wants boundary edges to be flagged,
     * we render everything as separate triangles (no strips or fans).
     */
    tess->flagBoundary = (fn != NULL);
    return;
  case GLU_TESS_EDGE_FLAG_DATA:
    tess->callEdgeFlagData= (fn == NULL) ? &__gl_noEdgeFlagData :
                                           (void (*)(GLboolean, void *)) fn; 
    /* If the client wants boundary edges to be flagged,
     * we render everything as separate triangles (no strips or fans).
     */
    tess->flagBoundary = (fn != NULL);
    return;
  case GLU_TESS_VERTEX:
    tess->callVertex = (fn == NULL) ? &noVertex : (void (*)(void *)) fn;
    return;
  case GLU_TESS_VERTEX_DATA:
    tess->callVertexData = (fn == NULL) ? &__gl_noVertexData : 
                                          (void (*)(void *, void *)) fn;
    return;
  case GLU_TESS_END:
    tess->callEnd = (fn == NULL) ? &noEnd : (void (*)(void)) fn;
    return;
  case GLU_TESS_END_DATA:
    tess->callEndData = (fn == NULL) ? &__gl_noEndData : 
                                       (void (*)(void *)) fn;
    return;
  case GLU_TESS_ERROR:
    tess->callError = (fn == NULL) ? &noError : (void (*)(GLenum)) fn;
    return;
  case GLU_TESS_ERROR_DATA:
    tess->callErrorData = (fn == NULL) ? &__gl_noErrorData : 
                                         (void (*)(GLenum, void *)) fn;
    return;
  case GLU_TESS_COMBINE:
    tess->callCombine = (fn == NULL) ? &noCombine :
	(void (*)(GLdouble [3],void *[4], GLfloat [4], void ** )) fn;
    return;
  case GLU_TESS_COMBINE_DATA:
    tess->callCombineData = (fn == NULL) ? &__gl_noCombineData :
                                           (void (*)(GLdouble [3],
						     void *[4], 
						     GLfloat [4], 
						     void **,
						     void *)) fn;
    return;
#ifndef NT
  case GLU_TESS_MESH:
    tess->callMesh = (fn == NULL) ? &noMesh : (void (*)(GLUmesh *)) fn;
    return;
#endif
  default:
    CALL_ERROR_OR_ERROR_DATA( GLU_INVALID_ENUM );
    return;
  }
}

static void AddVertex( GLUtesselator *tess, GLdouble coords[3], void *data )
{
  GLUhalfEdge *e;

  e = tess->lastEdge;
  if( e == NULL ) {
    /* Make a self-loop (one vertex, one edge). */

    e = __gl_meshMakeEdge( tess->mesh );
    __gl_meshSplice( e, e->Sym );
  } else {
    /* Create a new vertex and edge which immediately follow e
     * in the ordering around the left face.
     */
    (void) __gl_meshSplitEdge( e );
    e = e->Lnext;
  }

  /* The new vertex is now e->Org. */
  e->Org->data = data;
  e->Org->coords[0] = coords[0];
  e->Org->coords[1] = coords[1];
  e->Org->coords[2] = coords[2];
  
  /* The winding of an edge says how the winding number changes as we
   * cross from the edge''s right face to its left face.  We add the
   * vertices in such an order that a CCW contour will add +1 to
   * the winding number of the region inside the contour.
   */
  e->winding = 1;
  e->Sym->winding = -1;

  tess->lastEdge = e;
}


static void CacheVertex( GLUtesselator *tess, GLdouble coords[3], void *data )
{
  CachedVertex *v = &tess->cache[tess->cacheCount];

  v->data = data;
  v->coords[0] = coords[0];
  v->coords[1] = coords[1];
  v->coords[2] = coords[2];
  ++tess->cacheCount;
}


static void EmptyCache( GLUtesselator *tess )
{
  CachedVertex *v = tess->cache;
  CachedVertex *vLast;

  tess->mesh = __gl_meshNewMesh();

  for( vLast = v + tess->cacheCount; v < vLast; ++v ) {
    AddVertex( tess, v->coords, v->data );
  }
  tess->cacheCount = 0;
  tess->emptyCache = FALSE;
}


#ifdef NT
void APIENTRY gluTessVertex( GLUtesselator *tess, GLdouble coords[3], void *data )
#else
void gluTessVertex( GLUtesselator *tess, GLdouble coords[3], void *data )
#endif
{
  int i, tooLarge = FALSE;
  GLdouble x, clamped[3];

  RequireState( tess, T_IN_CONTOUR );

  if( tess->emptyCache ) {
    EmptyCache( tess );
    tess->lastEdge = NULL;
  }
  for( i = 0; i < 3; ++i ) {
    x = coords[i];
    if( x < - GLU_TESS_MAX_COORD ) {
      x = - GLU_TESS_MAX_COORD;
      tooLarge = TRUE;
    }
    if( x > GLU_TESS_MAX_COORD ) {
      x = GLU_TESS_MAX_COORD;
      tooLarge = TRUE;
    }
    clamped[i] = x;
  }
  if( tooLarge ) {
    CALL_ERROR_OR_ERROR_DATA( GLU_TESS_COORD_TOO_LARGE );
  }

  if( tess->mesh == NULL ) {
    if( tess->cacheCount < TESS_MAX_CACHE ) {
      CacheVertex( tess, clamped, data );
      return;
    }
    EmptyCache( tess );
  }
  AddVertex( tess, clamped, data );
}


#ifdef NT
void APIENTRY gluTessBeginPolygon( GLUtesselator *tess, void *data )
#else
void gluTessBeginPolygon( GLUtesselator *tess, void *data )
#endif
{
  RequireState( tess, T_DORMANT );

  tess->state = T_IN_POLYGON;
  tess->cacheCount = 0;
  tess->emptyCache = FALSE;
  tess->mesh = NULL;

  tess->polygonData= data;
}


#ifdef NT
void APIENTRY gluTessBeginContour( GLUtesselator *tess )
#else
void gluTessBeginContour( GLUtesselator *tess )
#endif
{
  RequireState( tess, T_IN_POLYGON );

  tess->state = T_IN_CONTOUR;
  tess->lastEdge = NULL;
  if( tess->cacheCount > 0 ) {
    /* Just set a flag so we don't get confused by empty contours
     * -- these can be generated accidentally with the obsolete
     * NextContour() interface.
     */
    tess->emptyCache = TRUE;
  }
}


#ifdef NT
void APIENTRY gluTessEndContour( GLUtesselator *tess )
#else
void gluTessEndContour( GLUtesselator *tess )
#endif
{
  RequireState( tess, T_IN_CONTOUR );
  tess->state = T_IN_POLYGON;
}


#ifdef NT
void APIENTRY gluTessEndPolygon( GLUtesselator *tess )
#else
void gluTessEndPolygon( GLUtesselator *tess )
#endif
{
  GLUmesh *mesh;

  RequireState( tess, T_IN_POLYGON );
  tess->state = T_DORMANT;

  if( tess->mesh == NULL ) {
    if( ! tess->flagBoundary && tess->callMesh == &noMesh ) {

      /* Try some special code to make the easy cases go quickly
       * (eg. convex polygons).  This code does NOT handle multiple contours,
       * intersections, edge flags, and of course it does not generate
       * an explicit mesh either.
       */
      if( __gl_renderCache( tess )) {
	tess->polygonData= NULL; 
	return;
      }
    }
    EmptyCache( tess );
  }

  /* Determine the polygon normal and project vertices onto the plane
   * of the polygon.
   */
  __gl_projectPolygon( tess );

  /* __gl_computeInterior( tess ) computes the planar arrangement specified
   * by the given contours, and further subdivides this arrangement
   * into regions.  Each region is marked "inside" if it belongs
   * to the polygon, according to the rule given by tess->windingRule.
   * Each interior region is guaranteed be monotone.
   */
  __gl_computeInterior( tess );

  mesh = tess->mesh;
  if( ! tess->fatalError ) {
    /* If the user wants only the boundary contours, we throw away all edges
     * except those which separate the interior from the exterior.
     * Otherwise we tesselate all the regions marked "inside".
     */
    if( tess->boundaryOnly ) {
      __gl_meshSetWindingNumber( mesh, 1, TRUE );
    } else {
      __gl_meshTesselateInterior( mesh );
    }
    __gl_meshCheckMesh( mesh );

    if( tess->callBegin != &noBegin || tess->callEnd != &noEnd
       || tess->callVertex != &noVertex || tess->callEdgeFlag != &noEdgeFlag 
       || tess->callBeginData != &__gl_noBeginData 
       || tess->callEndData != &__gl_noEndData
       || tess->callVertexData != &__gl_noVertexData
       || tess->callEdgeFlagData != &__gl_noEdgeFlagData )
    {
      if( tess->boundaryOnly ) {
	__gl_renderBoundary( tess, mesh );  /* output boundary contours */
      } else {
	__gl_renderMesh( tess, mesh );	   /* output strips and fans */
      }
    }
    if( tess->callMesh != &noMesh ) {

      /* Throw away the exterior faces, so that all faces are interior.
       * This way the user doesn't have to check the "inside" flag,
       * and we don't need to even reveal its existence.  It also leaves
       * the freedom for an implementation to not generate the exterior
       * faces in the first place.
       */
      __gl_meshDiscardExterior( mesh );
      (*tess->callMesh)( mesh );		/* user wants the mesh itself */
      tess->mesh = NULL;
      tess->polygonData= NULL;
      return;
    }
  }
  __gl_meshDeleteMesh( mesh );
  tess->polygonData= NULL;
  tess->mesh = NULL;
}


#ifndef NT
void gluDeleteMesh( GLUmesh *mesh )
{
  __gl_meshDeleteMesh( mesh );
}
#endif


/*******************************************************/

/* Obsolete calls -- for backward compatibility */

#ifdef NT
void APIENTRY gluBeginPolygon( GLUtesselator *tess )
#else
void gluBeginPolygon( GLUtesselator *tess )
#endif
{
  gluTessBeginPolygon( tess, NULL );
  gluTessBeginContour( tess );
}


/*ARGSUSED*/
#ifdef NT
void APIENTRY gluNextContour( GLUtesselator *tess, GLenum type )
#else
void gluNextContour( GLUtesselator *tess, GLenum type )
#endif
{
  gluTessEndContour( tess );
  gluTessBeginContour( tess );
}


#ifdef NT
void APIENTRY gluEndPolygon( GLUtesselator *tess )
#else
void gluEndPolygon( GLUtesselator *tess )
#endif
{
  gluTessEndContour( tess );
  gluTessEndPolygon( tess );
}




