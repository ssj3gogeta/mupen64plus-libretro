/*
* Glide64 - Glide video plugin for Nintendo 64 emulators.
* Copyright (c) 2002  Dave2001
* Copyright (c) 2003-2009  Sergey 'Gonetz' Lipski
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

//****************************************************************
//
// Glide64 - Glide Plugin for Nintendo 64 emulators
// Project started on December 29th, 2001
//
// Authors:
// Dave2001, original author, founded the project in 2001, left it in 2002
// Gugaman, joined the project in 2002, left it in 2002
// Sergey 'Gonetz' Lipski, joined the project in 2002, main author since fall of 2002
// Hiroshi 'KoolSmoky' Morii, joined the project in 2007
//
//****************************************************************
//
// To modify Glide64:
// * Write your name and (optional)email, commented by your work, so I know who did it, and so that you can find which parts you modified when it comes time to send it to me.
// * Do NOT send me the whole project or file that you modified.  Take out your modified code sections, and tell me where to put them.  If people sent the whole thing, I would have many different versions, but no idea how to combine them all.
//
//****************************************************************

#include "Gfx_1.3.h"
#include "../../libretro/SDL.h"

#include <math.h>
#include "3dmath.h"

#ifndef NOSSE
#include <xmmintrin.h>
#endif

void calc_light (VERTEX *v)
{
   uint32_t l;
   float light_intensity = 0.0f;
   float color[3];
   color[0] = rdp.light[rdp.num_lights].r;
   color[1] = rdp.light[rdp.num_lights].g;
   color[2] = rdp.light[rdp.num_lights].b;

   for (l = 0; l < rdp.num_lights; l++)
   {
      light_intensity = DotProduct (rdp.light_vector[l], v->vec);

      if (light_intensity > 0.0f) 
      {
         color[0] += rdp.light[l].r * light_intensity;
         color[1] += rdp.light[l].g * light_intensity;
         color[2] += rdp.light[l].b * light_intensity;
      }
   }

   if (color[0] > 1.0f)
      color[0] = 1.0f;
   if (color[1] > 1.0f)
      color[1] = 1.0f;
   if (color[2] > 1.0f)
      color[2] = 1.0f;

   v->r = (uint8_t)(color[0]*255.0f);
   v->g = (uint8_t)(color[1]*255.0f);
   v->b = (uint8_t)(color[2]*255.0f);
}

void calc_linear (VERTEX *v)
{
   if (settings.force_calc_sphere)
   {
      calc_sphere(v);
      return;
   }
   DECLAREALIGN16VAR(vec[3]);

   TransformVector (v->vec, vec, rdp.model);
   //    TransformVector (v->vec, vec, rdp.combined);
   NormalizeVector (vec);
   float x, y;
   if (!rdp.use_lookat)
   {
      x = vec[0];
      y = vec[1];
   }
   else
   {
      x = DotProduct (rdp.lookat[0], vec);
      y = DotProduct (rdp.lookat[1], vec);
   }

   if (x > 1.0f)
      x = 1.0f;
   else if (x < -1.0f)
      x = -1.0f;
   if (y > 1.0f)
      y = 1.0f;
   else if (y < -1.0f)
      y = -1.0f;

   if (rdp.cur_cache[0])
   {
      // scale >> 6 is size to map to
      v->ou = (acosf(x)/3.141592654f) * (rdp.tiles[rdp.cur_tile].org_s_scale >> 6);
      v->ov = (acosf(y)/3.141592654f) * (rdp.tiles[rdp.cur_tile].org_t_scale >> 6);
   }
   v->uv_scaled = 1;
#ifdef EXTREME_LOGGING
   FRDP ("calc linear u: %f, v: %f\n", v->ou, v->ov);
#endif
}

void calc_sphere (VERTEX *v)
{
   //  LRDP("calc_sphere\n");
   DECLAREALIGN16VAR(vec[3]);
   int s_scale, t_scale;
   if (settings.hacks&hack_Chopper)
   {
      s_scale = min(rdp.tiles[rdp.cur_tile].org_s_scale >> 6, rdp.tiles[rdp.cur_tile].lr_s);
      t_scale = min(rdp.tiles[rdp.cur_tile].org_t_scale >> 6, rdp.tiles[rdp.cur_tile].lr_t);
   }
   else
   {
      s_scale = rdp.tiles[rdp.cur_tile].org_s_scale >> 6;
      t_scale = rdp.tiles[rdp.cur_tile].org_t_scale >> 6;
   }
   TransformVector (v->vec, vec, rdp.model);
   NormalizeVector (vec);
   float x, y;
   if (!rdp.use_lookat)
   {
      x = vec[0];
      y = vec[1];
   }
   else
   {
      x = DotProduct (rdp.lookat[0], vec);
      y = DotProduct (rdp.lookat[1], vec);
   }
   v->ou = (x * 0.5f + 0.5f) * s_scale;
   v->ov = (y * 0.5f + 0.5f) * t_scale;
   v->uv_scaled = 1;
#ifdef EXTREME_LOGGING
   FRDP ("calc sphere u: %f, v: %f\n", v->ou, v->ov);
#endif
}

float DotProductC(float *v0, float *v1)
{
    float dot;
    dot = v0[0]*v1[0] + v0[1]*v1[1] + v0[2]*v1[2];
    return dot;
}

void NormalizeVectorC(float *v)
{
    float len;

    len = (float)(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
    if (len != 0.0)
    {
        len = (float)sqrt( len );
        v[0] /= (float)len;
        v[1] /= (float)len;
        v[2] /= (float)len;
    }
}

void TransformVectorC(float *src, float *dst, float mat[4][4])
{
   dst[0] = mat[0][0]*src[0] + mat[1][0]*src[1] + mat[2][0]*src[2];
   dst[1] = mat[0][1]*src[0] + mat[1][1]*src[1] + mat[2][1]*src[2];
   dst[2] = mat[0][2]*src[0] + mat[1][2]*src[1] + mat[2][2]*src[2];
}

void InverseTransformVectorC (float *src, float *dst, float mat[4][4])
{
   dst[0] = mat[0][0]*src[0] + mat[0][1]*src[1] + mat[0][2]*src[2];
   dst[1] = mat[1][0]*src[0] + mat[1][1]*src[1] + mat[1][2]*src[2];
   dst[2] = mat[2][0]*src[0] + mat[2][1]*src[1] + mat[2][2]*src[2];
}

void MulMatricesC(float m1[4][4],float m2[4][4],float r[4][4])
{
   int i;
   for (i=0; i < 4; i++)
   {
      r[i][0] = m1[i][0] * m2[0][0] + m1[i][1] * m2[1][0] + m1[i][2] * m2[2][0] + m1[i][3] * m2[3][0];
      r[i][1] = m1[i][0] * m2[0][1] + m1[i][1] * m2[1][1] + m1[i][2] * m2[2][1] + m1[i][3] * m2[3][1];
      r[i][2] = m1[i][0] * m2[0][2] + m1[i][1] * m2[1][2] + m1[i][2] * m2[2][2] + m1[i][3] * m2[3][2];
      r[i][3] = m1[i][0] * m2[0][3] + m1[i][1] * m2[1][3] + m1[i][2] * m2[2][3] + m1[i][3] * m2[3][3];
   }
}

// 2011-01-03 Balrog - removed because is in NASM format and not 64-bit compatible
// This will need fixing.
MULMATRIX MulMatrices = MulMatricesC;
TRANSFORMVECTOR TransformVector = TransformVectorC;
TRANSFORMVECTOR InverseTransformVector = InverseTransformVectorC;
DOTPRODUCT DotProduct = DotProductC;
NORMALIZEVECTOR NormalizeVector = NormalizeVectorC;

#if !defined(NOSSE)
// 2008.03.29 H.Morii - added SSE 3DNOW! 3x3 1x3 matrix multiplication
//                      and 3DNOW! 4x4 4x4 matrix multiplication

void MulMatricesSSE(float m1[4][4],float m2[4][4],float r[4][4])
{
   /* [row][col]*/
   int i;
   typedef float v4sf __attribute__ ((vector_size (16)));
   v4sf row0 = _mm_loadu_ps(m2[0]);
   v4sf row1 = _mm_loadu_ps(m2[1]);
   v4sf row2 = _mm_loadu_ps(m2[2]);
   v4sf row3 = _mm_loadu_ps(m2[3]);

   for (i = 0; i < 4; ++i)
   {
      v4sf leftrow = _mm_loadu_ps(m1[i]);

      // Fill tmp with four copies of leftrow[0]
      // Calculate the four first summands
      v4sf destrow = _mm_shuffle_ps (leftrow, leftrow, 0) * row0;

      destrow += (_mm_shuffle_ps (leftrow, leftrow, 1 + (1 << 2) + (1 << 4) + (1 << 6))) * row1;
      destrow += (_mm_shuffle_ps (leftrow, leftrow, 2 + (2 << 2) + (2 << 4) + (2 << 6))) * row2;
      destrow += (_mm_shuffle_ps (leftrow, leftrow, 3 + (3 << 2) + (3 << 4) + (3 << 6))) * row3;

      __builtin_ia32_storeups(r[i], destrow);
   }
}
#elif defined(HAVE_NEON)
static float DotProductNeon(float *v0, float *v1)
{
   float dot;
   __asm(
         "vld1.32 		{d8}, [%1]!			\n\t"	//d8={x0,y0}
         "vld1.32 		{d10}, [%2]!		\n\t"	//d10={x1,y1}
         "flds 			s18, [%1, #0]	    \n\t"	//d9[0]={z0}
         "flds 			s22, [%2, #0]	    \n\t"	//d11[0]={z1}
         "vmul.f32 		d12, d8, d10		\n\t"	//d0= d2*d4
         "vpadd.f32 		d12, d12, d12		\n\t"	//d0 = d[0] + d[1]
         "vmla.f32 		d12, d9, d11		\n\t"	//d0 = d0 + d3*d5
         "fmrs	        %0, s24	    		\n\t"	//r0 = s0
         : "=r"(dot), "+r"(v0), "+r"(v1):
         : "d8", "d9", "d10", "d11", "d12"

        );
   return dot;
}
#endif

void math_init(void)
{
   unsigned cpu = 0;

   if (perf_get_cpu_features_cb)
      cpu = perf_get_cpu_features_cb();

#if !defined(NOSSE)
   if (cpu & RETRO_SIMD_SSE2)
   {
      MulMatrices = MulMatricesSSE;
      if (log_cb)
         log_cb(RETRO_LOG_INFO, "SSE detected, using (some) optimized math functions.\n");
   }
#elif defined(HAVE_NEON)
   if (cpu & RETRO_SIMD_NEON)
   {
      DotProduct = DotProductNeon;
      if (log_cb)
         log_cb(RETRO_LOG_INFO, "NEON detected, using (some) optimized math functions.\n");
   }
#endif
}

