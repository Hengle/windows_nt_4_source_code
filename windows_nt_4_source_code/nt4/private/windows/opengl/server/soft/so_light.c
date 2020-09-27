/*
** Copyright 1991, Silicon Graphics, Inc.
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
** Lighting and coloring code.
**
** $Revision: 1.42 $
** $Date: 1993/12/08 02:20:39 $
*/

#include "precomp.h"
#pragma hdrstop

/*
** Scale an incoming color from the user.
*/
void FASTCALL __glScaleColorf(__GLcontext *gc, __GLcolor *dst, const GLfloat src[4])
{
    dst->r = src[0] * gc->redVertexScale;
    dst->g = src[1] * gc->greenVertexScale;
    dst->b = src[2] * gc->blueVertexScale;
    dst->a = src[3] * gc->alphaVertexScale;
}

/*
** Clamp and scale an incoming color from the user.
*/
void FASTCALL __glClampAndScaleColorf(__GLcontext *gc, __GLcolor *d, const GLfloat s[4])
{
    __GLfloat zero = __glZero;

    d->r = s[0] * gc->redVertexScale;
    if (d->r < zero) d->r = zero;
    if (d->r > gc->redVertexScale) d->r = gc->redVertexScale;

    d->g = s[1] * gc->greenVertexScale;
    if (d->g < zero) d->g = zero;
    if (d->g > gc->greenVertexScale) d->g = gc->greenVertexScale;

    d->b = s[2] * gc->blueVertexScale;
    if (d->b < zero) d->b = zero;
    if (d->b > gc->blueVertexScale) d->b = gc->blueVertexScale;

    d->a = s[3] * gc->alphaVertexScale;
    if (d->a < zero) d->a = zero;
    if (d->a > gc->alphaVertexScale) d->a = gc->alphaVertexScale;
}

/*
** Clamp an incoming color from the user.
*/
void FASTCALL __glClampColorf(__GLcontext *gc, __GLcolor *d, const GLfloat s[4])
{
    __GLfloat zero = __glZero;
    __GLfloat one = __glOne;
    __GLfloat r,g,b,a;

    r = s[0];
    g = s[1];
    b = s[2];
    a = s[3];

    if (r < zero) d->r = zero;
    else if (r > one) d->r = one;
    else d->r = r;

    if (g < zero) d->g = zero;
    else if (g > one) d->g = one;
    else d->g = g;

    if (b < zero) d->b = zero;
    else if (b > one) d->b = one;
    else d->b = b;

    if (a < zero) d->a = zero;
    else if (a > one) d->a = one;
    else d->a = a;
}

#ifdef NT_DEADCODE_GLSCALECOLORI
/*
** Scale an incoming color from the user.  This also converts the incoming
** color to floating point.
*/
void FASTCALL __glScaleColori(__GLcontext *gc, __GLcolor *dst, const GLint src[4])
{
    dst->r = __GL_I_TO_FLOAT(src[0]) * gc->redVertexScale;
    dst->g = __GL_I_TO_FLOAT(src[1]) * gc->greenVertexScale;
    dst->b = __GL_I_TO_FLOAT(src[2]) * gc->blueVertexScale;
    dst->a = __GL_I_TO_FLOAT(src[3]) * gc->alphaVertexScale;
}
#endif // NT_DEADCODE_GLSCALECOLORI

/*
** Clamp and scale an incoming color from the user.
*/
void FASTCALL __glClampAndScaleColori(__GLcontext *gc, __GLcolor *d, const GLint s[4])
{
    __GLfloat zero = __glZero;

    d->r = __GL_I_TO_FLOAT(s[0]) * gc->redVertexScale;
    if (d->r < zero) d->r = zero;
    if (d->r > gc->redVertexScale) d->r = gc->redVertexScale;

    d->g = __GL_I_TO_FLOAT(s[1]) * gc->greenVertexScale;
    if (d->g < zero) d->g = zero;
    if (d->g > gc->greenVertexScale) d->g = gc->greenVertexScale;

    d->b = __GL_I_TO_FLOAT(s[2]) * gc->blueVertexScale;
    if (d->b < zero) d->b = zero;
    if (d->b > gc->blueVertexScale) d->b = gc->blueVertexScale;

    d->a = __GL_I_TO_FLOAT(s[3]) * gc->alphaVertexScale;
    if (d->a < zero) d->a = zero;
    if (d->a > gc->alphaVertexScale) d->a = gc->alphaVertexScale;
}

/*
** Clamp an incoming color from the user.
*/
void FASTCALL __glClampColori(__GLcontext *gc, __GLcolor *d, const GLint s[4])
{
    __GLfloat zero = __glZero;
    __GLfloat one = __glOne;
    __GLfloat r,g,b,a;

    r = __GL_I_TO_FLOAT(s[0]);
    g = __GL_I_TO_FLOAT(s[1]);
    b = __GL_I_TO_FLOAT(s[2]);
    a = __GL_I_TO_FLOAT(s[3]);

    if (r < zero) d->r = zero;
    else if (r > one) d->r = one;
    else d->r = r;

    if (g < zero) d->g = zero;
    else if (g > one) d->g = one;
    else d->g = g;

    if (b < zero) d->b = zero;
    else if (b > one) d->b = one;
    else d->b = b;

    if (a < zero) d->a = zero;
    else if (a > one) d->a = one;
    else d->a = a;
}

/*
** Reverse the scaling back to the users original
*/
void FASTCALL __glUnScaleColorf(__GLcontext *gc, GLfloat dst[4], const __GLcolor* src)
{
    dst[0] = src->r * gc->oneOverRedVertexScale;
    dst[1] = src->g * gc->oneOverGreenVertexScale;
    dst[2] = src->b * gc->oneOverBlueVertexScale;
    dst[3] = src->a * gc->oneOverAlphaVertexScale;
}

/*
** Reverse the scaling back to the users original
*/
void FASTCALL __glUnScaleColori(__GLcontext *gc, GLint dst[4], const __GLcolor* src)
{
    dst[0] = __GL_FLOAT_TO_I(src->r * gc->oneOverRedVertexScale);
    dst[1] = __GL_FLOAT_TO_I(src->g * gc->oneOverGreenVertexScale);
    dst[2] = __GL_FLOAT_TO_I(src->b * gc->oneOverBlueVertexScale);
    dst[3] = __GL_FLOAT_TO_I(src->a * gc->oneOverAlphaVertexScale);
}

/*
** Clamp an already scaled RGB color.
*/
void FASTCALL __glClampRGBColor(__GLcontext *gc, __GLcolor *dst, const __GLcolor *src)
{
    __GLfloat zero = __glZero;
    __GLfloat r, g, b, a;
    __GLfloat rl, gl, bl, al;

    r = src->r; rl = gc->redVertexScale;
    if (r <= zero) {
	dst->r = zero;
    } else {
	if (r >= rl) {
	    dst->r = rl;
	} else {
	    dst->r = r;
	}
    }
    g = src->g; gl = gc->greenVertexScale;
    if (g <= zero) {
	dst->g = zero;
    } else {
	if (g >= gl) {
	    dst->g = gl;
	} else {
	    dst->g = g;
	}
    }
    b = src->b; bl = gc->blueVertexScale;
    if (b <= zero) {
	dst->b = zero;
    } else {
	if (b >= bl) {
	    dst->b = bl;
	} else {
	    dst->b = b;
	}
    }
    a = src->a; al = gc->alphaVertexScale;
    if (a <= zero) {
	dst->a = zero;
    } else {
	if (a >= al) {
	    dst->a = al;
	} else {
	    dst->a = a;
	}
    }
}

/************************************************************************/

/*
** r = vector from p1 to p2
*/
void FASTCALL vvec(__GLcoord *r, const __GLcoord *p1, const __GLcoord *p2)
{
    __GLfloat oneOverW;

    if (p2->w == __glZero) {
	if (p1->w == __glZero) {
	    r->x = p2->x - p1->x;
	    r->y = p2->y - p1->y;
	    r->z = p2->z - p1->z;
	} else {
	    r->x = p2->x;
	    r->y = p2->y;
	    r->z = p2->z;
	}
    } else
    if (p1->w == __glZero) {
	r->x = -p1->x;
	r->y = -p1->y;
	r->z = -p1->z;
    } else{
	oneOverW = ((__GLfloat) 1.0) / p2->w;
	r->x = p2->x * oneOverW;
	r->y = p2->y * oneOverW;
	r->z = p2->z * oneOverW;
	oneOverW = ((__GLfloat) 1.0) / p1->w;
	r->x -= p1->x * oneOverW;
	r->y -= p1->y * oneOverW;
	r->z -= p1->z * oneOverW;
    }
}

#ifdef NT_DEADCODE_POLYARRAY
void FASTCALL __glCalcRGBColor(__GLcontext *gc, GLint face, __GLvertex *vx)
{
    __GLlightSourceMachine *lsm;
    __GLlightSourcePerMaterialMachine *lspmm;
    __GLmaterialMachine *msm;
    __GLcolor *c;
    __GLfloat r, g, b, zero, nx, ny, nz;
    GLboolean eyeWIsZero, localViewer;
    static __GLcoord Pe = { 0, 0, 0, 1 };

    /* Pick material to use */
    if (face == __GL_FRONTFACE) {
	c = &vx->colors[__GL_FRONTFACE];
	msm = &gc->light.front;
	nx = vx->normal.x;
	ny = vx->normal.y;
	nz = vx->normal.z;
    } else {
	c = &vx->colors[__GL_BACKFACE];
	msm = &gc->light.back;
	nx = -vx->normal.x;
	ny = -vx->normal.y;
	nz = -vx->normal.z;
    }

    /* Compute the color */
    zero = __glZero;
    r = msm->sceneColor.r;
    g = msm->sceneColor.g;
    b = msm->sceneColor.b;
    eyeWIsZero = vx->eye.w == zero;
    localViewer = gc->state.light.model.localViewer;
    lsm = gc->light.sources;
    while (lsm) {
	lspmm = &lsm->front + face;
	if (lsm->slowPath || eyeWIsZero) {
	    __GLcoord hHat, vPli, vPliHat, vPeHat;
	    __GLfloat att, attSpot;
	    __GLfloat hv[3];

	    /* Compute unit h[i] */
	    vvec(&vPli, &vx->eye, &lsm->position);
	    __glNormalize(&vPliHat.x, &vPli.x);
	    if (localViewer) {
		vvec(&vPeHat, &vx->eye, &Pe);
		__glNormalize(&vPeHat.x, &vPeHat.x);
		hv[0] = vPliHat.x + vPeHat.x;
		hv[1] = vPliHat.y + vPeHat.y;
		hv[2] = vPliHat.z + vPeHat.z;
	    } else {
		hv[0] = vPliHat.x;
		hv[1] = vPliHat.y;
		hv[2] = vPliHat.z + __glOne;
	    }
	    __glNormalize(&hHat.x, hv);

	    /* Compute attenuation */
	    if (lsm->position.w != zero) {
		__GLfloat k0, k1, k2, dist;

		k0 = lsm->constantAttenuation;
		k1 = lsm->lizNearAttenuation;
		k2 = lsm->quadraticAttenuation;
		if ((k1 == zero) && (k2 == zero)) {
		    /* Use pre-computed 1/k0 */
		    att = lsm->attenuation;
		} else {
		    dist = __GL_SQRTF(vPli.x*vPli.x + vPli.y*vPli.y
				      + vPli.z*vPli.z);
		    att = __glOne / (k0 + k1 * dist + k2 * dist * dist);
		}
	    } else {
		att = __glOne;
	    }

	    /* Compute spot effect if light is a spot light */
	    attSpot = att;
	    if (lsm->isSpot) {
		__GLfloat dot, px, py, pz;

		px = -vPliHat.x;
		py = -vPliHat.y;
		pz = -vPliHat.z;
		dot = px * lsm->direction.x + py * lsm->direction.y
		    + pz * lsm->direction.z;
		if ((dot >= lsm->threshold) && (dot >= lsm->cosCutOffAngle)) {
		    GLint ix = (GLint)((dot - lsm->threshold) * lsm->scale 
				+ __glHalf);
		    if (ix < __GL_SPOT_LOOKUP_TABLE_SIZE) {
			attSpot = att * lsm->spotTable[ix];
		    }
		} else {
		    attSpot = zero;
		}
	    }

	    /* Add in remaining effect of light, if any */
	    if (attSpot) {
		__GLfloat n1, n2;
		__GLcolor sum;

		sum.r = lspmm->ambient.r;
		sum.g = lspmm->ambient.g;
		sum.b = lspmm->ambient.b;

		n1 = nx * vPliHat.x + ny * vPliHat.y + nz * vPliHat.z;
		if (n1 > zero) {
		    n2 = nx * hHat.x + ny * hHat.y + nz * hHat.z;
		    n2 -= msm->threshold;
		    if (n2 >= zero) {
#ifdef NT
		        __GLfloat fx = n2 * msm->scale + __glHalf;
                        if( fx < (__GLfloat)__GL_SPEC_LOOKUP_TABLE_SIZE )
		            n2 = msm->specTable[(GLint)fx];
                        else
                            n2 = __glOne;
#else
			GLint ix = (GLint)(n2 * msm->scale + __glHalf);
			if (ix < __GL_SPEC_LOOKUP_TABLE_SIZE) {
			    n2 = msm->specTable[ix];
			} else n2 = __glOne;
#endif
			sum.r += n2 * lspmm->specular.r;
			sum.g += n2 * lspmm->specular.g;
			sum.b += n2 * lspmm->specular.b;
		    }
		    sum.r += n1 * lspmm->diffuse.r;
		    sum.g += n1 * lspmm->diffuse.g;
		    sum.b += n1 * lspmm->diffuse.b;
		}

		r += attSpot * sum.r;
		g += attSpot * sum.g;
		b += attSpot * sum.b;
	    }
	} else {
	    __GLfloat n1, n2;

	    r += lspmm->ambient.r;
	    g += lspmm->ambient.g;
	    b += lspmm->ambient.b;

	    /* Add in specular and diffuse effect of light, if any */
	    n1 = nx * lsm->unitVPpli.x + ny * lsm->unitVPpli.y +
		nz * lsm->unitVPpli.z;
	    if (n1 > zero) {
		n2 = nx * lsm->hHat.x + ny * lsm->hHat.y + nz * lsm->hHat.z;
		n2 -= msm->threshold;
		if (n2 >= zero) {
#ifdef NT
		    __GLfloat fx = n2 * msm->scale + __glHalf;
                    if( fx < (__GLfloat)__GL_SPEC_LOOKUP_TABLE_SIZE )
		        n2 = msm->specTable[(GLint)fx];
                    else
                        n2 = __glOne;
#else
		    GLint ix = (GLint)(n2 * msm->scale + __glHalf);
		    if (ix < __GL_SPEC_LOOKUP_TABLE_SIZE) {
			n2 = msm->specTable[ix];
		    } else n2 = __glOne;
#endif
		    r += n2 * lspmm->specular.r;
		    g += n2 * lspmm->specular.g;
		    b += n2 * lspmm->specular.b;
		}
		r += n1 * lspmm->diffuse.r;
		g += n1 * lspmm->diffuse.g;
		b += n1 * lspmm->diffuse.b;
	    }
	}
	lsm = lsm->next;
    }

    /* Clamp the computed color */
    {
	__GLfloat rl, gl, bl;
	rl = gc->redVertexScale;
	gl = gc->greenVertexScale;
	bl = gc->blueVertexScale;
	if (r <= zero) {
	    r = zero;
	} else {
	    if (r >= rl) {
		r = rl;
	    }
	}
	if (g <= zero) {
	    g = zero;
	} else {
	    if (g >= gl) {
		g = gl;
	    }
	}
	if (b <= zero) {
	    b = zero;
	} else {
	    if (b >= bl) {
		b = bl;
	    }
	}
	c->r = r;
	c->g = g;
	c->b = b;
	c->a = msm->alpha;
    }
}
#endif // NT_DEADCODE_POLYARRAY

#ifdef NT_DEADCODE_POLYARRAY
#ifndef __GL_ASM_FASTCALCRGBCOLOR
void FASTCALL __glFastCalcRGBColor(__GLcontext *gc, GLint face, __GLvertex *vx)
{
    __GLlightSourceMachine *lsm;
    __GLlightSourcePerMaterialMachine *lspmm;
    __GLmaterialMachine *msm;
    __GLcolor *c;
    __GLfloat r, g, b;
    __GLfloat nx, ny, nz;
    __GLfloat zero;

    zero = __glZero;

    /* Pick material to use */
    if (face == __GL_FRONTFACE) {
	c = &vx->colors[__GL_FRONTFACE];
	msm = &gc->light.front;
	nx = vx->normal.x;
	ny = vx->normal.y;
	nz = vx->normal.z;
    } else {
	c = &vx->colors[__GL_BACKFACE];
	msm = &gc->light.back;
	nx = -vx->normal.x;
	ny = -vx->normal.y;
	nz = -vx->normal.z;
    }

    /* Compute the color */
    r = msm->sceneColor.r;
    g = msm->sceneColor.g;
    b = msm->sceneColor.b;
    lsm = gc->light.sources;
    while (lsm) {
	__GLfloat n1, n2;

	lspmm = &lsm->front + face;
	r += lspmm->ambient.r;
	g += lspmm->ambient.g;
	b += lspmm->ambient.b;

	/* Add in specular and diffuse effect of light, if any */
	n1 = nx * lsm->unitVPpli.x + ny * lsm->unitVPpli.y +
	    nz * lsm->unitVPpli.z;
	if (n1 > zero) {
	    n2 = nx * lsm->hHat.x + ny * lsm->hHat.y + nz * lsm->hHat.z;
	    n2 -= msm->threshold;
	    if (n2 >= zero) {
#ifdef NT
		__GLfloat fx = n2 * msm->scale + __glHalf;
                if( fx < (__GLfloat)__GL_SPEC_LOOKUP_TABLE_SIZE )
		    n2 = msm->specTable[(GLint)fx];
                else
                    n2 = __glOne;
#else
		GLint ix = (GLint)(n2 * msm->scale + __glHalf);
		if (ix < __GL_SPEC_LOOKUP_TABLE_SIZE) {
		    n2 = msm->specTable[ix];
		} else n2 = __glOne;
#endif
		r += n2 * lspmm->specular.r;
		g += n2 * lspmm->specular.g;
		b += n2 * lspmm->specular.b;
	    }
	    r += n1 * lspmm->diffuse.r;
	    g += n1 * lspmm->diffuse.g;
	    b += n1 * lspmm->diffuse.b;
	}
	lsm = lsm->next;
    }

    /* Clamp the computed color */
    {
	__GLfloat rl, gl, bl;
	rl = gc->redVertexScale;
	gl = gc->greenVertexScale;
	bl = gc->blueVertexScale;
	if (r <= zero) {
	    r = zero;
	} else {
	    if (r >= rl) {
		r = rl;
	    }
	}
	if (g <= zero) {
	    g = zero;
	} else {
	    if (g >= gl) {
		g = gl;
	    }
	}
	if (b <= zero) {
	    b = zero;
	} else {
	    if (b >= bl) {
		b = bl;
	    }
	}
	c->r = r;
	c->g = g;
	c->b = b;
	c->a = msm->alpha;
    }
}
#endif  /* !__GL_ASM_FASTCALCRGBCOLOR */
#endif // NT_DEADCODE_POLYARRAY

#ifdef NT_DEADCODE_POLYARRAY
void FASTCALL __glFogRGBColor(__GLcontext *gc, GLint face, __GLvertex *vx)
{
    __GLfloat fog, oneMinusFog;
    __GLcolor *cp;

    /* Get the vertex fog value */
    fog = vx->fog;
    oneMinusFog = __glOne - fog;

    /* Now whack the color */
    cp = &vx->colors[face];
    cp->r = fog * cp->r + oneMinusFog * gc->state.fog.color.r;
    cp->g = fog * cp->g + oneMinusFog * gc->state.fog.color.g;
    cp->b = fog * cp->b + oneMinusFog * gc->state.fog.color.b;
}
#endif // NT_DEADCODE_POLYARRAY

#ifdef NT_DEADCODE_POLYARRAY
void FASTCALL __glFogLitRGBColor(__GLcontext *gc, GLint face, __GLvertex *vx)
{
    __GLfloat fog, oneMinusFog;
    __GLcolor *cp;

    /* First compute vertex color */
    (*gc->procs.calcColor2)(gc, face, vx);

    /* Get the vertex fog value */
    fog = vx->fog;
    oneMinusFog = __glOne - fog;

    /* Now whack the color */
    cp = &vx->colors[face];
    cp->r = fog * cp->r + oneMinusFog * gc->state.fog.color.r;
    cp->g = fog * cp->g + oneMinusFog * gc->state.fog.color.g;
    cp->b = fog * cp->b + oneMinusFog * gc->state.fog.color.b;
}
#endif // NT_DEADCODE_POLYARRAY

#ifdef NT_DEADCODE_POLYARRAY
void FASTCALL __glCalcCIColor(__GLcontext *gc, GLint face, __GLvertex *vx)
{
    __GLmaterialMachine *msm;
    __GLmaterialState *ms;
    __GLlightSourceMachine *lsm;
    __GLcolor *c;
    __GLfloat ci, s, d, zero, nx, ny, nz;
    GLboolean eyeWIsZero, localViewer;
    static __GLcoord Pe = { 0, 0, 0, 1 };

    /* Pick material to use */
    if (face == __GL_FRONTFACE) {
	c = &vx->colors[__GL_FRONTFACE];
	ms = &gc->state.light.front;
	msm = &gc->light.front;
	nx = vx->normal.x;
	ny = vx->normal.y;
	nz = vx->normal.z;
    } else {
	c = &vx->colors[__GL_BACKFACE];
	ms = &gc->state.light.back;
	msm = &gc->light.back;
	nx = -vx->normal.x;
	ny = -vx->normal.y;
	nz = -vx->normal.z;
    }

    zero = __glZero;
    s = zero;
    d = zero;
    lsm = gc->light.sources;
    eyeWIsZero = vx->eye.w == zero;
    localViewer = gc->state.light.model.localViewer;
    while (lsm) {
	if (lsm->slowPath || eyeWIsZero) {
	    __GLfloat n1, n2, att, attSpot;
	    __GLcoord vPliHat, vPli, hHat, vPeHat;
	    __GLfloat hv[3];

	    /* Compute vPli, hi (normalized) */
	    vvec(&vPli, &vx->eye, &lsm->position);
	    __glNormalize(&vPliHat.x, &vPli.x);
	    if (localViewer) {
		vvec(&vPeHat, &vx->eye, &Pe);
		__glNormalize(&vPeHat.x, &vPeHat.x);
		hv[0] = vPliHat.x + vPeHat.x;
		hv[1] = vPliHat.y + vPeHat.y;
		hv[2] = vPliHat.z + vPeHat.z;
	    } else {
		hv[0] = vPliHat.x;
		hv[1] = vPliHat.y;
		hv[2] = vPliHat.z + __glOne;
	    }
	    __glNormalize(&hHat.x, hv);

	    /* Compute attenuation */
	    if (lsm->position.w != zero) {
		__GLfloat k0, k1, k2, dist;

		k0 = lsm->constantAttenuation;
		k1 = lsm->lizNearAttenuation;
		k2 = lsm->quadraticAttenuation;
		if ((k1 == zero) && (k2 == zero)) {
		    /* Use pre-computed 1/k0 */
		    att = lsm->attenuation;
		} else {
		    dist = __GL_SQRTF(vPli.x*vPli.x + vPli.y*vPli.y
				      + vPli.z*vPli.z);
		    att = __glOne / (k0 + k1 * dist + k2 * dist * dist);
		}
	    } else {
		att = __glOne;
	    }

	    /* Compute spot effect if light is a spot light */
	    attSpot = att;
	    if (lsm->isSpot) {
		__GLfloat dot, px, py, pz;

		px = -vPliHat.x;
		py = -vPliHat.y;
		pz = -vPliHat.z;
		dot = px * lsm->direction.x + py * lsm->direction.y
		    + pz * lsm->direction.z;
		if ((dot >= lsm->threshold) && (dot >= lsm->cosCutOffAngle)) {
		    GLint ix = (GLint)((dot - lsm->threshold) * lsm->scale 
				+ __glHalf);
		    if (ix < __GL_SPOT_LOOKUP_TABLE_SIZE) {
			attSpot = att * lsm->spotTable[ix];
		    }
		} else {
		    attSpot = zero;
		}
	    }

	    /* Add in remaining effect of light, if any */
	    if (attSpot) {
		n1 = nx * vPliHat.x + ny * vPliHat.y + nz * vPliHat.z;
		if (n1 > zero) {
		    n2 = nx * hHat.x + ny * hHat.y + nz * hHat.z;
		    n2 -= msm->threshold;
		    if (n2 >= zero) {
#ifdef NT
		        __GLfloat fx = n2 * msm->scale + __glHalf;
                        if( fx < (__GLfloat)__GL_SPEC_LOOKUP_TABLE_SIZE )
		            n2 = msm->specTable[(GLint)fx];
                        else
                            n2 = __glOne;
#else
			GLint ix = (GLint)(n2 * msm->scale + __glHalf);
			if (ix < __GL_SPEC_LOOKUP_TABLE_SIZE) {
			    n2 = msm->specTable[ix];
			} else n2 = __glOne;
#endif
			s += attSpot * n2 * lsm->sli;
		    }
		    d += attSpot * n1 * lsm->dli;
		}
	    }
	} else {
	    __GLfloat n1, n2;

	    /* Compute specular contribution */
	    n1 = nx * lsm->unitVPpli.x + ny * lsm->unitVPpli.y +
		nz * lsm->unitVPpli.z;
	    if (n1 > zero) {
		n2 = nx * lsm->hHat.x + ny * lsm->hHat.y + nz * lsm->hHat.z;
		n2 -= msm->threshold;
		if (n2 >= zero) {
#ifdef NT
		    __GLfloat fx = n2 * msm->scale + __glHalf;
                    if( fx < (__GLfloat)__GL_SPEC_LOOKUP_TABLE_SIZE )
		        n2 = msm->specTable[(GLint)fx];
                    else
                        n2 = __glOne;
#else
		    GLint ix = (GLint)(n2 * msm->scale + __glHalf);
		    if (ix < __GL_SPEC_LOOKUP_TABLE_SIZE) {
			n2 = msm->specTable[ix];
		    } else n2 = __glOne;
#endif
		    s += n2 * lsm->sli;
		}
		d += n1 * lsm->dli;
	    }
	}
	lsm = lsm->next;
    }

    /* Compute final color */
    if (s > __glOne) {
	s = __glOne;
    }
    ci = ms->cmapa + (__glOne - s) * d * (ms->cmapd - ms->cmapa)
	+ s * (ms->cmaps - ms->cmapa);
    if (ci > ms->cmaps) {
	ci = ms->cmaps;
    }
    if (ci > (GLfloat) gc->frontBuffer.redMax) {
        GLfloat fraction;
        GLint integer;

        integer = (GLint) ci;
        fraction = ci - (GLfloat) integer;
        integer = integer & (GLint) gc->frontBuffer.redMax;
        ci = (GLfloat) integer + fraction;
    } else if (ci < 0) {
        GLfloat fraction;
        GLint integer;

        integer = (GLint) __GL_FLOORF(ci);
        fraction = ci - (GLfloat) integer;
        integer = integer & (GLint) gc->frontBuffer.redMax;
        ci = (GLfloat) integer + fraction;
    }
    c->r = ci;
}
#endif // NT_DEADCODE_POLYARRAY

#ifdef NT_DEADCODE_POLYARRAY
/*
** No slow lights version.
*/
void FASTCALL __glFastCalcCIColor(__GLcontext *gc, GLint face, __GLvertex *vx)
{
    __GLmaterialMachine *msm;
    __GLmaterialState *ms;
    __GLlightSourceMachine *lsm;
    __GLcolor *c;
    __GLfloat ci, s, d, zero, nx, ny, nz;

    zero = __glZero;
    if (vx->eye.w == zero) {
	/* Have to use slow path to deal with zero eye w coordinate */
	__glCalcCIColor(gc, face, vx);
	return;
    }

    /* Pick material to use */
    if (face == __GL_FRONTFACE) {
	c = &vx->colors[__GL_FRONTFACE];
	ms = &gc->state.light.front;
	msm = &gc->light.front;
	nx = vx->normal.x;
	ny = vx->normal.y;
	nz = vx->normal.z;
    } else {
	c = &vx->colors[__GL_BACKFACE];
	ms = &gc->state.light.back;
	msm = &gc->light.back;
	nx = -vx->normal.x;
	ny = -vx->normal.y;
	nz = -vx->normal.z;
    }

    s = zero;
    d = zero;
    lsm = gc->light.sources;
    while (lsm) {
	__GLfloat n1, n2;

	/* Compute specular contribution */
	n1 = nx * lsm->unitVPpli.x + ny * lsm->unitVPpli.y +
	    nz * lsm->unitVPpli.z;
	if (n1 > zero) {
	    n2 = nx * lsm->hHat.x + ny * lsm->hHat.y + nz * lsm->hHat.z;
	    n2 -= msm->threshold;
	    if (n2 >= zero) {
#ifdef NT
		__GLfloat fx = n2 * msm->scale + __glHalf;
                if( fx < (__GLfloat)__GL_SPEC_LOOKUP_TABLE_SIZE )
		    n2 = msm->specTable[(GLint)fx];
                else
                    n2 = __glOne;
#else
		GLint ix = (GLint)(n2 * msm->scale + __glHalf);
		if (ix < __GL_SPEC_LOOKUP_TABLE_SIZE) {
		    n2 = msm->specTable[ix];
		} else n2 = __glOne;
#endif
		s += n2 * lsm->sli;
	    }
	    d += n1 * lsm->dli;
	}
	lsm = lsm->next;
    }

    /* Compute final color */
    if (s > __glOne) {
	s = __glOne;
    }
    ci = ms->cmapa + (__glOne - s) * d * (ms->cmapd - ms->cmapa)
	+ s * (ms->cmaps - ms->cmapa);
    if (ci > ms->cmaps) {
	ci = ms->cmaps;
    }
    c->r = ci;

//SGIBUG: need to mask color index before color clipping!
}
#endif // NT_DEADCODE_POLYARRAY

#ifdef NT_DEADCODE_POLYARRAY
void FASTCALL __glFogCIColor(__GLcontext *gc, GLint face, __GLvertex *vx)
{
    __GLfloat fog, oneMinusFog, maxR;
    __GLcolor *cp;

    /* Get the vertex fog value */
    fog = vx->fog;
    oneMinusFog = __glOne - fog;

    /* Now whack the color */
    cp = &vx->colors[face];
    cp->r = cp->r + oneMinusFog * gc->state.fog.index;
    maxR = (1 << gc->modes.indexBits) - 1;
    if (cp->r > maxR) {
	cp->r = maxR;
    }
}
#endif // NT_DEADCODE_POLYARRAY

#ifdef NT_DEADCODE_POLYARRAY
void FASTCALL __glFogLitCIColor(__GLcontext *gc, GLint face, __GLvertex *vx)
{
    __GLfloat fog, oneMinusFog, maxR;
    __GLcolor *cp;

    /* First compute vertex color */
    (*gc->procs.calcColor2)(gc, face, vx);

    /* Get the vertex fog value */
    fog = vx->fog;
    oneMinusFog = __glOne - fog;

    /* Now whack the color */
    cp = &vx->colors[face];
    cp->r = cp->r + oneMinusFog * gc->state.fog.index;
    maxR = (1 << gc->modes.indexBits) - 1;
    if (cp->r > maxR) {
	cp->r = maxR;
    }
}
#endif // NT_DEADCODE_POLYARRAY

/************************************************************************/

/*
** gc->procs.applyColor procs.  These are used to apply the current color
** change to either a material color, or to current.color (when not
** lighting), preparing the color for copying into the vertex.
*/

#ifndef __GL_ASM_CLAMPANDSCALECOLOR
/*
** Clamp and scale the current color (range 0.0 to 1.0) using the color
** buffer scales.  From here on out the colors in the vertex are in their
** final form.
*/
#ifdef NT_DEADCODE_CURRENT_COLOR
void FASTCALL __glClampAndScaleColor(__GLcontext *gc)
{
    __GLfloat one = __glOne;
    __GLfloat zero = __glZero;
    __GLfloat r, g, b, a;

    r = gc->state.current.userColor.r;
    g = gc->state.current.userColor.g;
    b = gc->state.current.userColor.b;
    a = gc->state.current.userColor.a;

    if (r <= zero) {
	gc->state.current.color.r = zero;
    } else
    if (r >= one) {
	gc->state.current.color.r = gc->redVertexScale;
    } else {
	gc->state.current.color.r = r * gc->redVertexScale;
    }

    if (g <= zero) {
	gc->state.current.color.g = zero;
    } else
    if (g >= one) {
	gc->state.current.color.g = gc->greenVertexScale;
    } else {
	gc->state.current.color.g = g * gc->greenVertexScale;
    }

    if (b <= zero) {
	gc->state.current.color.b = zero;
    } else
    if (b >= one) {
	gc->state.current.color.b = gc->blueVertexScale;
    } else {
	gc->state.current.color.b = b * gc->blueVertexScale;
    }

    if (a <= zero) {
	gc->state.current.color.a = zero;
    } else
    if (a >= one) {
	gc->state.current.color.a = gc->alphaVertexScale;
    } else {
	gc->state.current.color.a = a * gc->alphaVertexScale;
    }
}
#endif /* NT_DEADCODE_CURRENT_COLOR */
#endif /* !__GL_ASM_CLAMPANDSCALECOLOR */

void FASTCALL ChangeMaterialEmission(__GLcontext *gc, __GLmaterialState *ms,
				   __GLmaterialMachine *msm)
{
    __GLfloat r, g, b;

    r = gc->state.current.userColor.r * gc->redVertexScale;
    g = gc->state.current.userColor.g * gc->greenVertexScale;
    b = gc->state.current.userColor.b * gc->blueVertexScale;

    ms->emissive.r = r;
    ms->emissive.g = g;
    ms->emissive.b = b;
    ms->emissive.a = gc->state.current.userColor.a * gc->alphaVertexScale;

#ifdef NT
    // compute the invarient scene color
    msm->paSceneColor.r = ms->ambient.r * gc->state.light.model.ambient.r;
    msm->paSceneColor.g = ms->ambient.g * gc->state.light.model.ambient.g;
    msm->paSceneColor.b = ms->ambient.b * gc->state.light.model.ambient.b;
#else
    msm->sceneColor.r = r + ms->ambient.r * gc->state.light.model.ambient.r;
    msm->sceneColor.g = g + ms->ambient.g * gc->state.light.model.ambient.g;
    msm->sceneColor.b = b + ms->ambient.b * gc->state.light.model.ambient.b;
#endif
}

void FASTCALL ChangeMaterialSpecular(__GLcontext *gc, __GLmaterialState *ms,
				   __GLmaterialMachine *msm)
{
    __GLlightSourcePerMaterialMachine *lspmm;
    __GLlightSourceMachine *lsm;
    __GLlightSourceState *lss;
    GLboolean isBack;
    __GLfloat r, g, b;

    r = gc->state.current.userColor.r;
    g = gc->state.current.userColor.g;
    b = gc->state.current.userColor.b;

    ms->specular.r = r;
    ms->specular.g = g;
    ms->specular.b = b;
    ms->specular.a = gc->state.current.userColor.a;

    /*
    ** Update per-light-source state that depends on material specular
    ** state
    */
    isBack = msm == &gc->light.back;
    for (lsm = gc->light.sources; lsm; lsm = lsm->next) {
	lspmm = &lsm->front + isBack;
	lss = lsm->state;

	/* Recompute per-light per-material cached specular */
	lspmm->specular.r = r * lss->specular.r;
	lspmm->specular.g = g * lss->specular.g;
	lspmm->specular.b = b * lss->specular.b;
    }
}

void FASTCALL ChangeMaterialAmbient(__GLcontext *gc, __GLmaterialState *ms,
				  __GLmaterialMachine *msm)
{
    __GLlightSourcePerMaterialMachine *lspmm;
    __GLlightSourceMachine *lsm;
    __GLlightSourceState *lss;
    GLboolean isBack;
    __GLfloat r, g, b;

    r = gc->state.current.userColor.r;
    g = gc->state.current.userColor.g;
    b = gc->state.current.userColor.b;

    ms->ambient.r = r;
    ms->ambient.g = g;
    ms->ambient.b = b;
    ms->ambient.a = gc->state.current.userColor.a;

#ifdef NT
    // compute the invarient scene color
    msm->paSceneColor.r = ms->emissive.r;
    msm->paSceneColor.g = ms->emissive.g;
    msm->paSceneColor.b = ms->emissive.b;
#else
    msm->sceneColor.r = ms->emissive.r + r * gc->state.light.model.ambient.r;
    msm->sceneColor.g = ms->emissive.g + g * gc->state.light.model.ambient.g;
    msm->sceneColor.b = ms->emissive.b + b * gc->state.light.model.ambient.b;
#endif

    /*
    ** Update per-light-source state that depends on material ambient
    ** state.
    */
    isBack = msm == &gc->light.back;
    for (lsm = gc->light.sources; lsm; lsm = lsm->next) {
	lspmm = &lsm->front + isBack;
	lss = lsm->state;

	/* Recompute per-light per-material cached ambient */
	lspmm->ambient.r = r * lss->ambient.r;
	lspmm->ambient.g = g * lss->ambient.g;
	lspmm->ambient.b = b * lss->ambient.b;
    }
}

void FASTCALL ChangeMaterialDiffuse(__GLcontext *gc, __GLmaterialState *ms,
				  __GLmaterialMachine *msm)
{
    __GLlightSourcePerMaterialMachine *lspmm;
    __GLlightSourceMachine *lsm;
    __GLlightSourceState *lss;
    GLboolean isBack;
    __GLfloat r, g, b, a;

    r = gc->state.current.userColor.r;
    g = gc->state.current.userColor.g;
    b = gc->state.current.userColor.b;
    a = gc->state.current.userColor.a;

    ms->diffuse.r = r;
    ms->diffuse.g = g;
    ms->diffuse.b = b;
    ms->diffuse.a = a;

    if (a < __glZero) {
	a = __glZero;
    } else if (a > __glOne) {
	a = __glOne;
    }
    msm->alpha = a * gc->alphaVertexScale;

    /*
    ** Update per-light-source state that depends on material diffuse
    ** state.
    */
    isBack = msm == &gc->light.back;
    for (lsm = gc->light.sources; lsm; lsm = lsm->next) {
	lspmm = &lsm->front + isBack;
	lss = lsm->state;

	/* Recompute per-light per-material cached diffuse */
	lspmm->diffuse.r = r * lss->diffuse.r;
	lspmm->diffuse.g = g * lss->diffuse.g;
	lspmm->diffuse.b = b * lss->diffuse.b;
    }
}

void FASTCALL ChangeMaterialAmbientAndDiffuse(__GLcontext *gc,
					    __GLmaterialState *ms,
					    __GLmaterialMachine *msm)
{
    __GLlightSourcePerMaterialMachine *lspmm;
    __GLlightSourceMachine *lsm;
    __GLlightSourceState *lss;
    GLboolean isBack;
    __GLfloat r, g, b, a;

    r = gc->state.current.userColor.r;
    g = gc->state.current.userColor.g;
    b = gc->state.current.userColor.b;
    a = gc->state.current.userColor.a;

    ms->ambient.r = r;
    ms->ambient.g = g;
    ms->ambient.b = b;
    ms->ambient.a = a;

    ms->diffuse.r = r;
    ms->diffuse.g = g;
    ms->diffuse.b = b;
    ms->diffuse.a = a;

#ifdef NT
    // compute the invarient scene color
    msm->paSceneColor.r = ms->emissive.r;
    msm->paSceneColor.g = ms->emissive.g;
    msm->paSceneColor.b = ms->emissive.b;
#else
    msm->sceneColor.r = ms->emissive.r + r * gc->state.light.model.ambient.r;
    msm->sceneColor.g = ms->emissive.g + g * gc->state.light.model.ambient.g;
    msm->sceneColor.b = ms->emissive.b + b * gc->state.light.model.ambient.b;
#endif

    if (a < __glZero) {
	a = __glZero;
    } else if (a > __glOne) {
	a = __glOne;
    }
    msm->alpha = a * gc->alphaVertexScale;

    /*
    ** Update per-light-source state that depends on per-material state.
    */
    isBack = msm == &gc->light.back;
    for (lsm = gc->light.sources; lsm; lsm = lsm->next) {
	lspmm = &lsm->front + isBack;
	lss = lsm->state;

	/* Recompute per-light per-material cached ambient */
	lspmm->ambient.r = r * lss->ambient.r;
	lspmm->ambient.g = g * lss->ambient.g;
	lspmm->ambient.b = b * lss->ambient.b;

	/* Recompute per-light per-material cached diffuse */
	lspmm->diffuse.r = r * lss->diffuse.r;
	lspmm->diffuse.g = g * lss->diffuse.g;
	lspmm->diffuse.b = b * lss->diffuse.b;
    }
}

void FASTCALL __glChangeOneMaterialColor(__GLcontext *gc)
{
#ifdef NT_DEADCODE_POLYARRAY
    /*
    ** If we are in the middle of contructing a primitive, we possibly
    ** need to validate the front and back colors of the vertices which 
    ** we have queued.
    */
    if (__GL_IN_BEGIN() && gc->vertex.materialNeeds) {
	(*gc->procs.matValidate)(gc);
    }
#endif

    (*gc->procs.changeMaterial)(gc, gc->light.cm, gc->light.cmm);

#ifdef NT_DEADCODE_CURRENT_COLOR
    if (!(gc->state.enables.general & __GL_LIGHTING_ENABLE)) {
	/*
	** If colormaterial is enabled but lighting isn't we have to
	** clamp the current.color after the material has been updated.
	*/
	__glClampAndScaleColor(gc);
    }
#endif /* NT_DEADCODE_CURRENT_COLOR */
}

void FASTCALL __glChangeBothMaterialColors(__GLcontext *gc)
{
#ifdef NT_DEADCODE_POLYARRAY
    /*
    ** If we are in the middle of contructing a primitive, we possibly
    ** need to validate the front and back colors of the vertices which 
    ** we have queued.
    */
    if (__GL_IN_BEGIN() && gc->vertex.materialNeeds) {
	(*gc->procs.matValidate)(gc);
    }
#endif

    (*gc->procs.changeMaterial)(gc, &gc->state.light.front, &gc->light.front);
    (*gc->procs.changeMaterial)(gc, &gc->state.light.back, &gc->light.back);

#ifdef NT_DEADCODE_CURRENT_COLOR
    if (!(gc->state.enables.general & __GL_LIGHTING_ENABLE)) {
	/*
	** If colormaterial is enabled but lighting isn't we have to
	** clamp the current.color after the material has been updated.
	*/
	__glClampAndScaleColor(gc);
    }
#endif /* NT_DEADCODE_CURRENT_COLOR */
}

/************************************************************************/

/*
** DEPENDENCIES:
**
** Material 	EMISSIVE, AMBIENT, DIFFUSE, SHININESS
** Light Model 	AMBIENT
*/

/*
** Compute derived state for a material
*/
void ComputeMaterialState(__GLcontext *gc, __GLmaterialState *ms,
				 __GLmaterialMachine *msm, GLint changeBits)
{
    GLdouble  exponent;
    __GLspecLUTEntry *lut;

    if ((changeBits & (__GL_MATERIAL_EMISSIVE | __GL_MATERIAL_AMBIENT | 
	    __GL_MATERIAL_DIFFUSE | __GL_MATERIAL_SHININESS)) == 0) {
	return;
    }
    /* Only compute specular lookup table when it changes */
    if (!msm->cache || (ms->specularExponent != msm->specularExponent)) {
	/*
	** Specular lookup table generation.  Instead of performing a
	** "pow" computation each time a vertex is lit, we generate a
	** lookup table which approximates the pow function:
	**
	** 	n2 = n circle-dot hHat[i]
	** 	if (n2 >= threshold) {
	** 		n2spec = specTable[n2 * scale];
	** 		...
	** 	}
	**
	** Remember that n2 is a value constrained to be between 0.0 and
	** 1.0, inclusive (n is the normalized normal; hHat[i] is the
	** unit h vector).  "threshold" is the threshold where incoming
	** n2 values become meaningful for a given exponent.  The larger
	** the specular exponent, the closer "threshold" will approach
	** 1.0.
	**
	** A simple lizNear mapping of the n2 value to a table index will
	** not suffice because in most cases the majority of the table
	** entries would be zero, while the useful non-zero values would
	** be compressed into a few table entries.  By setting up a
	** threshold, we can use the entire table to represent the useful
	** values beyond the threshold.  "scale" is computed based on
	** this threshold.
	*/
	exponent = msm->specularExponent = ms->specularExponent;

	__glFreeSpecLUT(gc, msm->cache);
	lut = msm->cache = __glCreateSpecLUT(gc, exponent);
#ifdef NT
        if (lut)
        {
            msm->threshold = lut->threshold;
            msm->scale = lut->scale;
            msm->specTable = lut->table;
        }
        else
        {
            msm->threshold = (GLfloat) 0.0;
            msm->scale = (GLfloat) __GL_SPEC_LOOKUP_TABLE_SIZE;
            msm->specTable = NULL;
        }
#else
	msm->threshold = lut->threshold;
	msm->scale = lut->scale;
	msm->specTable = lut->table;
#endif // NT
    }

#ifdef NT
    /* Compute invarient scene color */
    if (changeBits & (__GL_MATERIAL_EMISSIVE | __GL_MATERIAL_AMBIENT))
    {
	if (msm->colorMaterialChange & __GL_MATERIAL_EMISSIVE)
	{
	    msm->paSceneColor.r = ms->ambient.r * gc->state.light.model.ambient.r;
	    msm->paSceneColor.g = ms->ambient.g * gc->state.light.model.ambient.g;
	    msm->paSceneColor.b = ms->ambient.b * gc->state.light.model.ambient.b;
	}
	else if (msm->colorMaterialChange & __GL_MATERIAL_AMBIENT)
	{
	    msm->paSceneColor.r = ms->emissive.r;
	    msm->paSceneColor.g = ms->emissive.g;
	    msm->paSceneColor.b = ms->emissive.b;
	}
	else
	{
	    // there is no color material but need to compute this anyway!
	    msm->paSceneColor.r = ms->emissive.r 
		+ ms->ambient.r * gc->state.light.model.ambient.r;
	    msm->paSceneColor.g = ms->emissive.g 
		+ ms->ambient.g * gc->state.light.model.ambient.g;
	    msm->paSceneColor.b = ms->emissive.b 
		+ ms->ambient.b * gc->state.light.model.ambient.b;
	}
    }
#else
    /* Compute scene color */
    if (changeBits & (__GL_MATERIAL_EMISSIVE | __GL_MATERIAL_AMBIENT)) {
	msm->sceneColor.r = ms->emissive.r
	    + ms->ambient.r * gc->state.light.model.ambient.r;
	msm->sceneColor.g = ms->emissive.g
	    + ms->ambient.g * gc->state.light.model.ambient.g;
	msm->sceneColor.b = ms->emissive.b
	    + ms->ambient.b * gc->state.light.model.ambient.b;
    }
#endif

    /* Clamp material alpha */
    if (changeBits & __GL_MATERIAL_DIFFUSE) {
	msm->alpha = ms->diffuse.a * gc->alphaVertexScale;
	if (msm->alpha < __glZero) {
	    msm->alpha = __glZero;
	} else if (msm->alpha > gc->alphaVertexScale) {
	    msm->alpha = gc->alphaVertexScale;
	}
    }
}

/*
** DEPENDENCIES:
**
** Derived state:
**
** Enables	LIGHTx
** Lightx	DIFFUSE, AMBIENT, SPECULAR, POSITION, SPOT_EXPONENT, 
**		SPOT_CUTOFF, CONSTANT_ATTENUATION, LINEAR_ATTENUATION,
**		QUADRATIC_ATTENUATION
** Light Model  LOCAL_VIEWER
*/

/*
** Compute any derived state for the enabled lights.
*/
void FASTCALL ComputeLightState(__GLcontext *gc)
{
    __GLlightSourceState *lss;
    __GLlightSourceMachine *lsm, **lsmp;
    __GLfloat zero;
    GLuint enables;
    GLint i;
    __GLspecLUTEntry *lut;

    zero = __glZero;

    lss = &gc->state.light.source[0];
    lsm = &gc->light.source[0];
    lsmp = &gc->light.sources;
    enables = gc->state.enables.lights;
    for (i = 0; i < gc->constants.numberOfLights;
	    i++, lss++, lsm++, enables >>= 1) {
	if (!(enables & 1)) continue;

	/* Link this enabled light on to the list */
	*lsmp = lsm;
	lsm->state = lss;	/* Could be done once, elsewhere... */
	lsmp = &lsm->next;

	/*
	** Compute per-light derived state that wasn't already done
	** in the api handlers.
	*/
	lsm->position = lss->positionEye;
	lsm->isSpot = lss->spotLightCutOffAngle != 180;
	if (lsm->isSpot) {
	    lsm->cosCutOffAngle =
		__GL_COSF(lss->spotLightCutOffAngle * __glDegreesToRadians);
	}

	if (lsm->isSpot && (!lsm->cache ||
	        (lsm->spotLightExponent != lss->spotLightExponent))) {
	    GLdouble exponent;

	    /*
	    ** Compute spot light exponent lookup table, but only when
	    ** the exponent changes value and the light is a spot light.
	    */
	    exponent = lsm->spotLightExponent = lss->spotLightExponent;

	    if (lsm->cache) {
		__glFreeSpecLUT(gc, lsm->cache);
	    }
	    lut = lsm->cache = __glCreateSpecLUT(gc, exponent);
#ifdef NT
            if (lut)
            {
                lsm->threshold = lut->threshold;
                lsm->scale = lut->scale;
                lsm->spotTable = lut->table;
            }
            else
            {
                lsm->threshold = (GLfloat) 0.0;
                lsm->scale = (GLfloat) __GL_SPEC_LOOKUP_TABLE_SIZE;
                lsm->spotTable = NULL;
            }
#else
	    lsm->threshold = lut->threshold;
	    lsm->scale = lut->scale;
	    lsm->spotTable = lut->table;
#endif // NT
	}

	lsm->constantAttenuation = lss->constantAttenuation;
	if (lsm->constantAttenuation) {
	    lsm->attenuation = __glOne / lss->constantAttenuation;
	}
	lsm->lizNearAttenuation = lss->lizNearAttenuation;
	lsm->quadraticAttenuation = lss->quadraticAttenuation;

	/*
	** Pick per-light calculation proc based on the state
	** of the light source
	*/
	if (gc->modes.colorIndexMode) {
	    lsm->sli = ((__GLfloat) 0.30) * lss->specular.r
		+ ((__GLfloat) 0.59) * lss->specular.g
		+ ((__GLfloat) 0.11) * lss->specular.b;
	    lsm->dli = ((__GLfloat) 0.30) * lss->diffuse.r
		+ ((__GLfloat) 0.59) * lss->diffuse.g
		+ ((__GLfloat) 0.11) * lss->diffuse.b;
	}
	if (!gc->state.light.model.localViewer && !lsm->isSpot
		&& (lsm->position.w == zero)) {
	    __GLfloat hv[3];

	    /* Compute unit h[i] (normalized) */
	    __glNormalize(hv, &lsm->position.x);
	    lsm->unitVPpli.x = hv[0];
	    lsm->unitVPpli.y = hv[1];
	    lsm->unitVPpli.z = hv[2];
	    hv[2] += __glOne;
	    __glNormalize(&lsm->hHat.x, hv);
	    lsm->slowPath = GL_FALSE;
	} else {
	    lsm->slowPath = GL_TRUE;
	}
    }
    *lsmp = 0;
}

/*
** DEPENDENCIES:
**
** Procs:
**
** Light Model	LOCAL_VIEWER
** Lightx	SPOT_CUTOFF, POSITION
** Enables	LIGHTING
** modeFlags	CHEAP_FOG
*/
void FASTCALL ComputeLightProcs(__GLcontext *gc)
{
    GLboolean anySlow = GL_FALSE;
    __GLlightSourceMachine *lsm;

    for (lsm = gc->light.sources; lsm; lsm = lsm->next) {
	if (lsm->slowPath) {
	    anySlow = GL_TRUE;
	    break;
	}
    }

#ifdef NT
    if ((gc->polygon.shader.modeFlags & __GL_SHADE_CHEAP_FOG) &&
	    (gc->polygon.shader.modeFlags & __GL_SHADE_SMOOTH_LIGHT) &&
	    gc->renderMode == GL_RENDER)
    {
	if (gc->modes.colorIndexMode)
	    gc->procs.paApplyCheapFog = PolyArrayCheapFogCIColor;
	else
	    gc->procs.paApplyCheapFog = PolyArrayCheapFogRGBColor;
    }
    else
	gc->procs.paApplyCheapFog = 0;	// for debugging

    if (gc->state.enables.general & __GL_LIGHTING_ENABLE)
    {
	if (gc->modes.colorIndexMode)
	{
	    if (!anySlow)
		gc->procs.paCalcColor = PolyArrayFastCalcCIColor;
	    else
		gc->procs.paCalcColor = PolyArrayCalcCIColor;
	}
	else
	{
	    if (!anySlow)
	    {
		// If there are no color material changes in front and back
		// faces, use the zippy function!
		if (!gc->light.front.colorMaterialChange
		 && !gc->light.back.colorMaterialChange)
		    gc->procs.paCalcColor = PolyArrayZippyCalcRGBColor;
		else
		    gc->procs.paCalcColor = PolyArrayFastCalcRGBColor;
	    }
	    else
	    {
		gc->procs.paCalcColor = PolyArrayCalcRGBColor;
	    }
	}
    }
    else
    {
	// set it to NULL for debugging
	gc->procs.paCalcColor = (PFN_POLYARRAYCALCCOLOR) NULL;
    }

    if (gc->modes.colorIndexMode)
	gc->procs.paCalcColorSkip = PolyArrayFillIndex0;
    else
	gc->procs.paCalcColorSkip = PolyArrayFillColor0;
#else
    if (gc->state.enables.general & __GL_LIGHTING_ENABLE) {
	if (gc->modes.colorIndexMode) {
	    if (!anySlow) {
		gc->procs.calcColor = __glFastCalcCIColor;
	    } else {
		gc->procs.calcColor = __glCalcCIColor;
	    }
	} else {
	    if (!anySlow) {
		gc->procs.calcColor = __glFastCalcRGBColor;
	    } else {
		gc->procs.calcColor = __glCalcRGBColor;
	    }
	}
	gc->procs.calcRasterColor = gc->procs.calcColor;
	if ((gc->polygon.shader.modeFlags & __GL_SHADE_CHEAP_FOG) &&
		(gc->polygon.shader.modeFlags & __GL_SHADE_SMOOTH_LIGHT) &&
		gc->renderMode == GL_RENDER) {
	    gc->procs.calcColor2 = gc->procs.calcColor;
	    if (gc->modes.colorIndexMode) {
		gc->procs.calcColor = __glFogLitCIColor;
	    } else {
		gc->procs.calcColor = __glFogLitRGBColor;
	    }
	}
    } else {
	gc->procs.calcRasterColor = __glNopLight;
	if ((gc->polygon.shader.modeFlags & __GL_SHADE_CHEAP_FOG) &&
		(gc->polygon.shader.modeFlags & __GL_SHADE_SMOOTH_LIGHT) &&
		gc->renderMode == GL_RENDER) {
	    if (gc->modes.colorIndexMode) {
		gc->procs.calcColor = __glFogCIColor;
	    } else {
		gc->procs.calcColor = __glFogRGBColor;
	    }
	} else {
	    gc->procs.calcColor = __glNopLight;
	}
    }
#endif
}

/*
** DEPENDENCIES:
**
** Material	AMBIENT, DIFFUSE, SPECULAR
** Lightx	AMBIENT, DIFFUSE, SPECULAR
*/
void FASTCALL ComputeLightMaterialState(__GLcontext *gc, GLint frontChange,
				      GLint backChange)
{
    __GLmaterialState *front, *back;
    __GLlightSourceMachine *lsm;
    __GLlightSourceState *lss;
    __GLfloat r, g, b;
    GLint allChange;

    allChange = frontChange | backChange;
    if ((allChange & (__GL_MATERIAL_AMBIENT | __GL_MATERIAL_DIFFUSE | 
	    __GL_MATERIAL_SPECULAR)) == 0) {
	return;
    }

    front = &gc->state.light.front;
    back = &gc->state.light.back;
    for (lsm = gc->light.sources; lsm; lsm = lsm->next) {
	lss = lsm->state;
	/*
	** Pre-multiply and the front & back ambient, diffuse and
	** specular colors
	*/
	if (allChange & __GL_MATERIAL_AMBIENT) {
	    r = lss->ambient.r;
	    g = lss->ambient.g;
	    b = lss->ambient.b;
	    if (frontChange & __GL_MATERIAL_AMBIENT) {
		lsm->front.ambient.r = front->ambient.r * r;
		lsm->front.ambient.g = front->ambient.g * g;
		lsm->front.ambient.b = front->ambient.b * b;
	    }
	    if (backChange & __GL_MATERIAL_AMBIENT) {
		lsm->back.ambient.r = back->ambient.r * r;
		lsm->back.ambient.g = back->ambient.g * g;
		lsm->back.ambient.b = back->ambient.b * b;
	    }
	}

	if (allChange & __GL_MATERIAL_DIFFUSE) {
	    r = lss->diffuse.r;
	    g = lss->diffuse.g;
	    b = lss->diffuse.b;
	    if (frontChange & __GL_MATERIAL_DIFFUSE) {
		lsm->front.diffuse.r = front->diffuse.r * r;
		lsm->front.diffuse.g = front->diffuse.g * g;
		lsm->front.diffuse.b = front->diffuse.b * b;
	    }
	    if (backChange & __GL_MATERIAL_DIFFUSE) {
		lsm->back.diffuse.r = back->diffuse.r * r;
		lsm->back.diffuse.g = back->diffuse.g * g;
		lsm->back.diffuse.b = back->diffuse.b * b;
	    }
	}

	if (allChange & __GL_MATERIAL_SPECULAR) {
	    r = lss->specular.r;
	    g = lss->specular.g;
	    b = lss->specular.b;
	    if (frontChange & __GL_MATERIAL_SPECULAR) {
		lsm->front.specular.r = front->specular.r * r;
		lsm->front.specular.g = front->specular.g * g;
		lsm->front.specular.b = front->specular.b * b;
	    }
	    if (backChange & __GL_MATERIAL_SPECULAR) {
		lsm->back.specular.r = back->specular.r * r;
		lsm->back.specular.g = back->specular.g * g;
		lsm->back.specular.b = back->specular.b * b;
	    }
	}
    }
}

/*
** DEPENDENCIES:
**
** Material 	EMISSIVE, AMBIENT, DIFFUSE, SHININESS, SPECULAR
** Light Model 	AMBIENT
** Lightx	AMBIENT, DIFFUSE, SPECULAR
*/

/*
** Recompute light state based upon the material change indicated by 
** frontChange and backChange.
*/
void FASTCALL __glValidateMaterial(__GLcontext *gc, GLint frontChange, GLint backChange)
{
    ComputeMaterialState(gc, &gc->state.light.front, &gc->light.front, 
	    frontChange);
    ComputeMaterialState(gc, &gc->state.light.back, &gc->light.back, 
	    backChange);
    ComputeLightMaterialState(gc, frontChange, backChange);
}

/*
** DEPENDENCIES:
**
** Enables	LIGHTx, LIGHTING
** ( Material 	EMISSIVE, AMBIENT, DIFFUSE, SHININESS, SPECULAR )
** Light Model 	AMBIENT, LOCAL_VIEWER
** Lightx	DIFFUSE, AMBIENT, SPECULAR, POSITION, SPOT_EXPONENT, 
**		SPOT_CUTOFF, CONSTANT_ATTENUATION, LINEAR_ATTENUATION,
**		QUADRATIC_ATTENUATION
** modeFlags	CHEAP_FOG
*/

/*
** Pre-compute lighting state.
*/
void FASTCALL __glValidateLighting(__GLcontext *gc)
{
    if (gc->dirtyMask & __GL_DIRTY_LIGHTING) {
	ComputeLightState(gc);
	ComputeLightProcs(gc);
	__glValidateMaterial(gc, __GL_MATERIAL_ALL, __GL_MATERIAL_ALL);
    } else {
	ComputeLightProcs(gc);
    }
}

void FASTCALL __glGenericPickColorMaterialProcs(__GLcontext *gc)
{
    if (gc->modes.rgbMode) {
	if (gc->state.enables.general & __GL_COLOR_MATERIAL_ENABLE) {
	    switch (gc->state.light.colorMaterialFace) {
	      case GL_FRONT_AND_BACK:
		gc->procs.applyColor = __glChangeBothMaterialColors;
		gc->light.cm = 0;
		gc->light.cmm = 0;
		break;
	      case GL_FRONT:
		gc->procs.applyColor = __glChangeOneMaterialColor;
		gc->light.cm = &gc->state.light.front;
		gc->light.cmm = &gc->light.front;
		break;
	      case GL_BACK:
		gc->procs.applyColor = __glChangeOneMaterialColor;
		gc->light.cm = &gc->state.light.back;
		gc->light.cmm = &gc->light.back;
		break;
	    }
	    switch (gc->state.light.colorMaterialParam) {
	      case GL_EMISSION:
		gc->procs.changeMaterial = ChangeMaterialEmission;
		break;
	      case GL_SPECULAR:
		gc->procs.changeMaterial = ChangeMaterialSpecular;
		break;
	      case GL_AMBIENT:
		gc->procs.changeMaterial = ChangeMaterialAmbient;
		break;
	      case GL_DIFFUSE:
		gc->procs.changeMaterial = ChangeMaterialDiffuse;
		break;
	      case GL_AMBIENT_AND_DIFFUSE:
		gc->procs.changeMaterial = ChangeMaterialAmbientAndDiffuse;
		break;
	    }
	} else {
#ifdef NT_DEADCODE_CURRENT_COLOR
	    if (gc->state.enables.general & __GL_LIGHTING_ENABLE) {
		/* When no color material, user color is not used */
		gc->procs.applyColor = __glNopGC;
	    } else {
		gc->procs.applyColor = __glClampAndScaleColor;
	    }
#else
	    gc->procs.applyColor = __glNopGC;
#endif
	}
    } else {
	/*
	** When in color index mode the value is copied from the
	** current.userColorIndex into the vertex
	*/
	gc->procs.applyColor = __glNopGC;
    }
}
