// Avisynth v2.5.  Copyright 2002 Ben Rudiak-Gould et al.
// http://www.avisynth.org

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA, or visit
// http://www.gnu.org/copyleft/gpl.html .
//
// Linking Avisynth statically or dynamically with other modules is making a
// combined work based on Avisynth.  Thus, the terms and conditions of the GNU
// General Public License cover the whole combination.
//
// As a special exception, the copyright holders of Avisynth give you
// permission to link Avisynth with independent modules that communicate with
// Avisynth solely through the interfaces defined in avisynth.h, regardless of the license
// terms of these independent modules, and to copy and distribute the
// resulting combined work under terms of your choice, provided that
// every copy of the combined work is accompanied by a complete copy of
// the source code of Avisynth (the version of Avisynth used to produce the
// combined work), being distributed under the terms of the GNU General
// Public License plus this exception.  An independent module is a module
// which is not derived from or based on Avisynth, such as 3rd-party filters,
// import and export plugins, or graphical user interfaces.

// ConvertPlanar (c) 2005 by Klaus Post

#ifndef _CRTIMP
#define _CRTIMP
#endif

#include "avisynth/avisynth_stdafx.h"
#include "convert.h"
#include "convert_planar.h"
#include "../filters/resample.h"
#include "../filters/planeswap.h"
#include "../filters/field.h"
#include <tmmintrin.h>

enum   {PLACEMENT_MPEG2, PLACEMENT_MPEG1, PLACEMENT_DV } ;

static int getPlacement( const AVSValue& _placement, IScriptEnvironment* env);


ConvertToY8::ConvertToY8(PClip src, int in_matrix, IScriptEnvironment* env) : GenericVideoFilter(src,__FUNCTION__ ), matrix(NULL) {
  yuy2_input = blit_luma_only = rgb_input = false;

  if (vi.IsPlanar()) {
    blit_luma_only = true;
    vi.pixel_type = CS_Y8;
    return;
  }

  if (vi.IsYUY2()) {
    yuy2_input = true;
    vi.pixel_type = CS_Y8;
    return;
  }

  if (vi.IsRGB()) {
    rgb_input = true;
    pixel_step = vi.BytesFromPixels(1);
    vi.pixel_type = CS_Y8;
    matrix = (signed short*)av_malloc(sizeof(short)*4);
    signed short* m = matrix;
    if (in_matrix == Rec601) {
      *m++ = (signed short)((219.0/255.0)*0.114*32768.0+0.5);  //B
      *m++ = (signed short)((219.0/255.0)*0.587*32768.0+0.5);  //G
      *m++ = (signed short)((219.0/255.0)*0.299*32768.0+0.5);  //R
      offset_y = 16;
    } else if (in_matrix == PC_601) {
      *m++ = (signed short)(0.114*32768.0+0.5);  //B
      *m++ = (signed short)(0.587*32768.0+0.5);  //G
      *m++ = (signed short)(0.299*32768.0+0.5);  //R
      offset_y = 0;
    } else if (in_matrix == Rec709) {
      *m++ = (signed short)((219.0/255.0)*0.0722*32768.0+0.5);  //B
      *m++ = (signed short)((219.0/255.0)*0.7152*32768.0+0.5);  //G
      *m++ = (signed short)((219.0/255.0)*0.2126*32768.0+0.5);  //R
      offset_y = 16;
    } else if (in_matrix == PC_709) {
      *m++ = (signed short)(0.0722*32768.0+0.5);  //B
      *m++ = (signed short)(0.7152*32768.0+0.5);  //G
      *m++ = (signed short)(0.2126*32768.0+0.5);  //R
      offset_y = 0;
    } else if (in_matrix == AVERAGE) {
      *m++ = (signed short)(32768.0/3 + 0.5);  //B
      *m++ = (signed short)(32768.0/3 + 0.5);  //G
      *m++ = (signed short)(32768.0/3 + 0.5);  //R
      offset_y = 0;
    } else {
        av_free(matrix);
      matrix = 0;
      env->ThrowError("ConvertToY8: Unknown matrix.");
    }
    *m = 0;  // Alpha

    return;
  }

  env->ThrowError("ConvertToY8: Unknown input format");
}

ConvertToY8::~ConvertToY8() {
    av_free(matrix);
  matrix = NULL;
}


//This is faster than mmx only sometimes. But will work on x64
static void convert_yuy2_to_y8_sse2(const BYTE *srcp, BYTE *dstp, size_t src_pitch, size_t dst_pitch, size_t width, size_t height)
{
  size_t mod16_width = (width / 16) * 16;
  __m128i luma_mask = _mm_set1_epi16(0xFF);

  for(size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < mod16_width; x+=16) {
      __m128i src1 = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp+x*2));
      __m128i src2 = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp+x*2+16));
      src1 = _mm_and_si128(src1, luma_mask);
      src2 = _mm_and_si128(src2, luma_mask);
      _mm_store_si128(reinterpret_cast<__m128i*>(dstp+x), _mm_packus_epi16(src1, src2));
    }

    if (width != mod16_width) {
      __m128i src1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcp+width*2-32));
      __m128i src2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcp+width*2-16));
      src1 = _mm_and_si128(src1, luma_mask);
      src2 = _mm_and_si128(src2, luma_mask);
      _mm_storeu_si128(reinterpret_cast<__m128i*>(dstp+width-16), _mm_packus_epi16(src1, src2));
    }

    dstp += dst_pitch;
    srcp += src_pitch;
  }
}

#ifdef _M_IX86
static void convert_yuy2_to_y8_mmx(const BYTE *srcp, BYTE *dstp, size_t src_pitch, size_t dst_pitch, size_t width, size_t height)
{
  size_t mod8_width = (width / 8) * 8;
  __m64 luma_mask = _mm_set1_pi16(0xFF);

  for(size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < mod8_width; x+=8) {
      __m64 src1 = *reinterpret_cast<const __m64*>(srcp+x*2);
      __m64 src2 = *reinterpret_cast<const __m64*>(srcp+x*2+8);
      src1 = _mm_and_si64(src1, luma_mask);
      src2 = _mm_and_si64(src2, luma_mask);
      *reinterpret_cast<__m64*>(dstp+x) = _mm_packs_pu16(src1, src2);
    }

    if (width != mod8_width) {
        __m64 src1 = *reinterpret_cast<const __m64*>(srcp+width*2-16);
      __m64 src2 = *reinterpret_cast<const __m64*>(srcp+width*2-8);
      src1 = _mm_and_si64(src1, luma_mask);
      src2 = _mm_and_si64(src2, luma_mask);
      *reinterpret_cast<__m64*>(dstp+width-8) = _mm_packs_pu16(src1, src2);
    }

    dstp += dst_pitch;
    srcp += src_pitch;
  }
  _mm_empty();
}
#endif


static __forceinline __m128i convert_rgb_to_y8_sse2_core(const __m128i &pixel01, const __m128i &pixel23, const __m128i &pixel45, const __m128i &pixel67, __m128i& zero, __m128i &matrix, __m128i &round_mask, __m128i &offset) {
  //int Y = offset_y + ((m0 * srcp[0] + m1 * srcp[1] + m2 * srcp[2] + 16384) >> 15);
  // in general the algorithm is identical to MMX version, the only different part is getting r and g+b in appropriate registers. We use shuffling instead of unpacking here.
  __m128i pixel01m = _mm_madd_epi16(pixel01, matrix); //a1*0 + r1*cyr | g1*cyg + b1*cyb | a0*0 + r0*cyr | g0*cyg + b0*cyb
  __m128i pixel23m = _mm_madd_epi16(pixel23, matrix); //a3*0 + r3*cyr | g3*cyg + b3*cyb | a2*0 + r2*cyr | g2*cyg + b2*cyb
  __m128i pixel45m = _mm_madd_epi16(pixel45, matrix); //a5*0 + r5*cyr | g5*cyg + b5*cyb | a4*0 + r4*cyr | g4*cyg + b4*cyb
  __m128i pixel67m = _mm_madd_epi16(pixel67, matrix); //a7*0 + r7*cyr | g7*cyg + b7*cyb | a6*0 + r6*cyr | g6*cyg + b6*cyb

  __m128i pixel_0123_r = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(pixel01m), _mm_castsi128_ps(pixel23m), _MM_SHUFFLE(3, 1, 3, 1))); // r3*cyr | r2*cyr | r1*cyr | r0*cyr
  __m128i pixel_4567_r = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(pixel45m), _mm_castsi128_ps(pixel67m), _MM_SHUFFLE(3, 1, 3, 1))); // r7*cyr | r6*cyr | r5*cyr | r4*cyr

  __m128i pixel_0123 = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(pixel01m), _mm_castsi128_ps(pixel23m), _MM_SHUFFLE(2, 0, 2, 0)));
  __m128i pixel_4567 = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(pixel45m), _mm_castsi128_ps(pixel67m), _MM_SHUFFLE(2, 0, 2, 0)));

  pixel_0123 = _mm_add_epi32(pixel_0123, pixel_0123_r); 
  pixel_4567 = _mm_add_epi32(pixel_4567, pixel_4567_r); 

  pixel_0123 = _mm_add_epi32(pixel_0123, round_mask);
  pixel_4567 = _mm_add_epi32(pixel_4567, round_mask);

  pixel_0123 = _mm_srai_epi32(pixel_0123, 15); 
  pixel_4567 = _mm_srai_epi32(pixel_4567, 15); 

  __m128i result = _mm_packs_epi32(pixel_0123, pixel_4567);
  
  result = _mm_adds_epi16(result, offset);
  result = _mm_packus_epi16(result, zero); 

  return result;
}

static void convert_rgb32_to_y8_sse2(const BYTE *srcp, BYTE *dstp, size_t src_pitch, size_t dst_pitch, size_t width, size_t height, BYTE offset_y, short cyr, short cyg, short cyb ) {
  __m128i matrix = _mm_set_epi16(0, cyr, cyg, cyb, 0, cyr, cyg, cyb);
  __m128i zero = _mm_setzero_si128();
  __m128i offset = _mm_set1_epi16(offset_y);
  __m128i round_mask = _mm_set1_epi32(16384);

  size_t loop_limit;
  bool not_mod8 = false;
  //todo: simplify
  if (width % 8 == 0) {
    loop_limit = width;
  } else if (dst_pitch % 8 == 0) {
    loop_limit = dst_pitch; //we'll just write some garbage
  } else {
    loop_limit = width / 8 * 8;
    not_mod8 = true;
  }

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < loop_limit; x+=8) {
      __m128i src0123 = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp+x*4)); //pixels 0, 1, 2 and 3
      __m128i src4567 = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp+x*4+16));//pixels 4, 5, 6 and 7

      __m128i pixel01 = _mm_unpacklo_epi8(src0123, zero); 
      __m128i pixel23 = _mm_unpackhi_epi8(src0123, zero); 
      __m128i pixel45 = _mm_unpacklo_epi8(src4567, zero); 
      __m128i pixel67 = _mm_unpackhi_epi8(src4567, zero); 

      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstp+x), convert_rgb_to_y8_sse2_core(pixel01, pixel23, pixel45, pixel67, zero, matrix, round_mask, offset));
    }

    if (not_mod8) {
      __m128i src0123 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcp+width*4-32)); //pixels 0, 1, 2 and 3
      __m128i src4567 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcp+width*4-16));//pixels 4, 5, 6 and 7

      __m128i pixel01 = _mm_unpacklo_epi8(src0123, zero); 
      __m128i pixel23 = _mm_unpackhi_epi8(src0123, zero); 
      __m128i pixel45 = _mm_unpacklo_epi8(src4567, zero); 
      __m128i pixel67 = _mm_unpackhi_epi8(src4567, zero); 

      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstp+width-8), convert_rgb_to_y8_sse2_core(pixel01, pixel23, pixel45, pixel67, zero, matrix, round_mask, offset));
    }

    srcp -= src_pitch;
    dstp += dst_pitch;
  }
}


static void convert_rgb24_to_y8_sse2(const BYTE *srcp, BYTE *dstp, size_t src_pitch, size_t dst_pitch, size_t width, size_t height, BYTE offset_y, short cyr, short cyg, short cyb ) {
  __m128i matrix = _mm_set_epi16(0, cyr, cyg, cyb, 0, cyr, cyg, cyb);
  __m128i zero = _mm_setzero_si128();
  __m128i offset = _mm_set1_epi16(offset_y);
  __m128i round_mask = _mm_set1_epi32(16384);

  size_t loop_limit;
  bool not_mod8 = false;
  //todo: simplify
  if (width % 8 == 0) {
    loop_limit = width;
  } else if (dst_pitch % 8 == 0) {
    loop_limit = dst_pitch; //we'll just write some garbage
  } else {
    loop_limit = width / 8 * 8;
    not_mod8 = true;
  }

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < loop_limit; x+=8) {
      __m128i pixel01 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+x*3)); //pixels 0 and 1
      __m128i pixel23 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+x*3+6)); //pixels 2 and 3
      __m128i pixel45 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+x*3+12)); //pixels 4 and 5
      __m128i pixel67 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+x*3+18)); //pixels 6 and 7

      
      //0 0 0 0 0 0 0 0 | x x r1 g1 b1 r0 g0 b0  -> 0 x 0 x 0 r1 0 g1 | 0 b1 0 r0 0 g0 0 b0 -> 0 r1 0 g1 0 b1 0 r0 | 0 b1 0 r0 0 g0 0 b0 -> 0 r1 0 r1 0 g1 0 b1 | 0 b1 0 r0 0 g0 0 b0
      pixel01 = _mm_shufflehi_epi16(_mm_shuffle_epi32(_mm_unpacklo_epi8(pixel01, zero), _MM_SHUFFLE(2, 1, 1, 0)), _MM_SHUFFLE(0, 3, 2, 1));
      pixel23 = _mm_shufflehi_epi16(_mm_shuffle_epi32(_mm_unpacklo_epi8(pixel23, zero), _MM_SHUFFLE(2, 1, 1, 0)), _MM_SHUFFLE(0, 3, 2, 1));
      pixel45 = _mm_shufflehi_epi16(_mm_shuffle_epi32(_mm_unpacklo_epi8(pixel45, zero), _MM_SHUFFLE(2, 1, 1, 0)), _MM_SHUFFLE(0, 3, 2, 1));
      pixel67 = _mm_shufflehi_epi16(_mm_shuffle_epi32(_mm_unpacklo_epi8(pixel67, zero), _MM_SHUFFLE(2, 1, 1, 0)), _MM_SHUFFLE(0, 3, 2, 1));

      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstp+x), convert_rgb_to_y8_sse2_core(pixel01, pixel23, pixel45, pixel67, zero, matrix, round_mask, offset));
    }

    if (not_mod8) {
      __m128i pixel01 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+width*3-24)); //pixels 0 and 1
      __m128i pixel23 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+width*3-18)); //pixels 2 and 3
      __m128i pixel45 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+width*3-12)); //pixels 4 and 5
      __m128i pixel67 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+width*3-6)); //pixels 6 and 7

      pixel01 = _mm_shufflehi_epi16(_mm_shuffle_epi32(_mm_unpacklo_epi8(pixel01, zero), _MM_SHUFFLE(2, 1, 1, 0)), _MM_SHUFFLE(0, 3, 2, 1));
      pixel23 = _mm_shufflehi_epi16(_mm_shuffle_epi32(_mm_unpacklo_epi8(pixel23, zero), _MM_SHUFFLE(2, 1, 1, 0)), _MM_SHUFFLE(0, 3, 2, 1));
      pixel45 = _mm_shufflehi_epi16(_mm_shuffle_epi32(_mm_unpacklo_epi8(pixel45, zero), _MM_SHUFFLE(2, 1, 1, 0)), _MM_SHUFFLE(0, 3, 2, 1));
      pixel67 = _mm_shufflehi_epi16(_mm_shuffle_epi32(_mm_unpacklo_epi8(pixel67, zero), _MM_SHUFFLE(2, 1, 1, 0)), _MM_SHUFFLE(0, 3, 2, 1));

      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstp+width-8), convert_rgb_to_y8_sse2_core(pixel01, pixel23, pixel45, pixel67, zero, matrix, round_mask, offset));
    }

    srcp -= src_pitch;
    dstp += dst_pitch;
  }
}


#ifdef _M_IX86

#pragma warning(push)
#pragma warning(disable: 4799)
static __forceinline int convert_rgb_to_y8_mmx_core(const __m64 &pixel0, const __m64 &pixel1, const __m64 &pixel2, const __m64 &pixel3, __m64& zero, __m64 &matrix, __m64 &round_mask, __m64 &offset) {
  //int Y = offset_y + ((m0 * srcp[0] + m1 * srcp[1] + m2 * srcp[2] + 16384) >> 15);
  
  __m64 pixel0m = _mm_madd_pi16(pixel0, matrix); //a0*0 + r0*cyr | g0*cyg + b0*cyb
  __m64 pixel1m = _mm_madd_pi16(pixel1, matrix); //a1*0 + r1*cyr | g1*cyg + b1*cyb
  __m64 pixel2m = _mm_madd_pi16(pixel2, matrix); //a2*0 + r2*cyr | g2*cyg + b2*cyb
  __m64 pixel3m = _mm_madd_pi16(pixel3, matrix); //a3*0 + r3*cyr | g3*cyg + b3*cyb

  __m64 pixel_01_r = _mm_unpackhi_pi32(pixel0m, pixel1m); // r1*cyr | r0*cyr
  __m64 pixel_23_r = _mm_unpackhi_pi32(pixel2m, pixel3m); // r3*cyr | r2*cyr

  __m64 pixel_01 = _mm_unpacklo_pi32(pixel0m, pixel1m); //g1*cyg + b1*cyb | g0*cyg + b0*cyb
  __m64 pixel_23 = _mm_unpacklo_pi32(pixel2m, pixel3m); //g3*cyg + b3*cyb | g2*cyg + b2*cyb

  pixel_01 = _mm_add_pi32(pixel_01, pixel_01_r); // r1*cyr + g1*cyg + b1*cyb | r0*cyr + g0*cyg + b0*cyb
  pixel_23 = _mm_add_pi32(pixel_23, pixel_23_r); // r3*cyr + g3*cyg + b3*cyb | r2*cyr + g2*cyg + b2*cyb

  pixel_01 = _mm_add_pi32(pixel_01, round_mask); //r1*cyr + g1*cyg + b1*cyb + 16384 | r0*cyr + g0*cyg + b0*cyb + 16384
  pixel_23 = _mm_add_pi32(pixel_23, round_mask); //r3*cyr + g3*cyg + b3*cyb + 16384 | r2*cyr + g2*cyg + b2*cyb + 16384

  pixel_01 = _mm_srai_pi32(pixel_01, 15); //0 | p1 | 0 | p0
  pixel_23 = _mm_srai_pi32(pixel_23, 15); //0 | p3 | 0 | p2

  __m64 result = _mm_packs_pi32(pixel_01, pixel_23); //p3 | p2 | p1 | p0

  result = _mm_adds_pi16(result, offset);
  result = _mm_packs_pu16(result, zero); //0 0 0 0 p3 p2 p1 p0

  return _mm_cvtsi64_si32(result);
}
#pragma warning(pop)

static void convert_rgb32_to_y8_mmx(const BYTE *srcp, BYTE *dstp, size_t src_pitch, size_t dst_pitch, size_t width, size_t height, BYTE offset_y, short cyr, short cyg, short cyb ) {
  __m64 matrix = _mm_set_pi16(0, cyr, cyg, cyb);
  __m64 zero = _mm_setzero_si64();
  __m64 offset = _mm_set1_pi16(offset_y);
  __m64 round_mask = _mm_set1_pi32(16384);

  size_t loop_limit;
  bool not_mod4 = false;
  //todo: simplify
  if (width % 4 == 0) {
    loop_limit = width;
  } else if (dst_pitch % 4 == 0) {
    loop_limit = dst_pitch; //we'll just write some garbage
  } else {
    loop_limit = width / 4 * 4;
    not_mod4 = true;
  }

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < loop_limit; x+=4) {
      __m64 src01 = *reinterpret_cast<const __m64*>(srcp+x*4); //pixels 0 and 1
      __m64 src23 = *reinterpret_cast<const __m64*>(srcp+x*4+8);//pixels 2 and 3

      __m64 pixel0 = _mm_unpacklo_pi8(src01, zero); //a0 r0 g0 b0
      __m64 pixel1 = _mm_unpackhi_pi8(src01, zero); //a1 r1 g1 b1
      __m64 pixel2 = _mm_unpacklo_pi8(src23, zero); //a2 r2 g2 b2
      __m64 pixel3 = _mm_unpackhi_pi8(src23, zero); //a3 r3 g3 b3

      *reinterpret_cast<int*>(dstp+x) = convert_rgb_to_y8_mmx_core(pixel0, pixel1, pixel2, pixel3, zero, matrix, round_mask, offset);
    }

    if (not_mod4) {
      __m64 src01 = *reinterpret_cast<const __m64*>(srcp+width*4-16);
      __m64 src23 = *reinterpret_cast<const __m64*>(srcp+width*4-8);

      __m64 pixel0 = _mm_unpacklo_pi8(src01, zero); //a0 r0 g0 b0
      __m64 pixel1 = _mm_unpackhi_pi8(src01, zero); //a1 r1 g1 b1
      __m64 pixel2 = _mm_unpacklo_pi8(src23, zero); //a2 r2 g2 b2
      __m64 pixel3 = _mm_unpackhi_pi8(src23, zero); //a3 r3 g3 b3

      *reinterpret_cast<int*>(dstp+width-4) = convert_rgb_to_y8_mmx_core(pixel0, pixel1, pixel2, pixel3, zero, matrix, round_mask, offset);
    }

    srcp -= src_pitch;
    dstp += dst_pitch;
  }
  _mm_empty();
}


static void convert_rgb24_to_y8_mmx(const BYTE *srcp, BYTE *dstp, size_t src_pitch, size_t dst_pitch, size_t width, size_t height, BYTE offset_y, short cyr, short cyg, short cyb ) {
  __m64 matrix = _mm_set_pi16(0, cyr, cyg, cyb);
  __m64 zero = _mm_setzero_si64();
  __m64 offset = _mm_set1_pi16(offset_y);
  __m64 round_mask = _mm_set1_pi32(16384);

  size_t loop_limit;
  bool not_mod4 = false;
  //todo: simplify
  if (width % 4 == 0) {
    loop_limit = width;
  } else if (dst_pitch % 4 == 0) {
    loop_limit = dst_pitch; //we'll just write some garbage
  } else {
    loop_limit = width / 4 * 4;
    not_mod4 = true;
  }

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < loop_limit; x+=4) {
      __m64 pixel0 = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcp+x*3)); //pixel 0
      __m64 pixel1 = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcp+x*3+3)); //pixel 1
      __m64 pixel2 = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcp+x*3+6)); //pixel 2
      __m64 pixel3 = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcp+x*3+9)); //pixel 3
      
      pixel0 = _mm_unpacklo_pi8(pixel0, zero);
      pixel1 = _mm_unpacklo_pi8(pixel1, zero);
      pixel2 = _mm_unpacklo_pi8(pixel2, zero);
      pixel3 = _mm_unpacklo_pi8(pixel3, zero);

      *reinterpret_cast<int*>(dstp+x) = convert_rgb_to_y8_mmx_core(pixel0, pixel1, pixel2, pixel3, zero, matrix, round_mask, offset);
    }

    if (not_mod4) {
      __m64 pixel0 = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcp+width*3-12)); //pixel 0
      __m64 pixel1 = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcp+width*3-9)); //pixel 1
      __m64 pixel2 = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcp+width*3-6)); //pixel 2
      __m64 pixel3 = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcp+width*3-3)); //pixel 3

      pixel0 = _mm_unpacklo_pi8(pixel0, zero);
      pixel1 = _mm_unpacklo_pi8(pixel1, zero);
      pixel2 = _mm_unpacklo_pi8(pixel2, zero);
      pixel3 = _mm_unpacklo_pi8(pixel3, zero);

      *reinterpret_cast<int*>(dstp+width-4) = convert_rgb_to_y8_mmx_core(pixel0, pixel1, pixel2, pixel3, zero, matrix, round_mask, offset);
    }

    srcp -= src_pitch;
    dstp += dst_pitch;
  }
  _mm_empty();
}

#endif // X86_32


PVideoFrame __stdcall ConvertToY8::GetFrame(int n, IScriptEnvironment* env) {
  PVideoFrame src = child->GetFrame(n, env);

  if (src == nullptr || src.m_ptr == nullptr) {
      return nullptr;
  }
  if (blit_luma_only) {
    // Abuse Subframe to snatch the Y plane
    return env->Subframe(src, 0, src->GetPitch(PLANAR_Y), src->GetRowSize(PLANAR_Y), src->GetHeight(PLANAR_Y));
  }

  PVideoFrame dst = env->NewVideoFrame(vi);
  if (dst == nullptr || dst.m_ptr == nullptr) {
      return nullptr;
  }

  if (yuy2_input) {

    const BYTE* srcP = src->GetReadPtr();
    const int srcPitch = src->GetPitch();
    int width = dst->GetRowSize(PLANAR_Y);
    int height = dst->GetHeight(PLANAR_Y);

    BYTE* dstY = dst->GetWritePtr(PLANAR_Y);
    const int dstPitch = dst->GetPitch(PLANAR_Y);
    

    if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(srcP, 16)) {
      convert_yuy2_to_y8_sse2(srcP, dstY, srcPitch, dstPitch, width, height);
    } else
#ifdef _M_IX86
    if (env->GetCPUFlags() & CPUF_MMX) {
      convert_yuy2_to_y8_mmx(srcP, dstY, srcPitch, dstPitch, width, height);
    } else
#endif 
    {
      for (int y=0; y<height; y++) {
        for (int x=0; x<width; x++) {
          dstY[x] = srcP[x*2];
        }
        srcP+=srcPitch;
        dstY+=dstPitch;
      }
    }
    return dst;
  }

  if (rgb_input) {
    const int src_pitch = src->GetPitch();
    const BYTE* srcp = src->GetReadPtr() + src_pitch * (vi.height-1);  // We start at last line

    BYTE* dstp = dst->GetWritePtr(PLANAR_Y);
    const int dst_pitch = dst->GetPitch(PLANAR_Y);

    const int m0 = matrix[0];
    const int m1 = matrix[1];
    const int m2 = matrix[2];

    if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(srcp, 16)) {
      if (pixel_step == 4) {
        convert_rgb32_to_y8_sse2(srcp, dstp, src_pitch, dst_pitch, vi.width, vi.height, offset_y, m2, m1, m0);
        return dst;
      } else if (pixel_step == 3) {
        convert_rgb24_to_y8_sse2(srcp, dstp, src_pitch, dst_pitch, vi.width, vi.height, offset_y, m2, m1, m0);
        return dst;
      }
    }

#ifdef _M_IX86
    if (env->GetCPUFlags() & CPUF_MMX) {
      if (pixel_step == 4) {
        convert_rgb32_to_y8_mmx(srcp, dstp, src_pitch, dst_pitch, vi.width, vi.height, offset_y, m2, m1, m0);
        return dst;
      } else if (pixel_step == 3) {
        convert_rgb24_to_y8_mmx(srcp, dstp, src_pitch, dst_pitch, vi.width, vi.height, offset_y, m2, m1, m0);
        return dst;
      } 
    }
#endif

    const int srcMod = src_pitch + (vi.width * pixel_step);
    for (int y=0; y<vi.height; y++) {
      for (int x=0; x<vi.width; x++) {
        const int Y = offset_y + ((m0 * srcp[0] + m1 * srcp[1] + m2 * srcp[2] + 16384) >> 15);
        dstp[x] = PixelClip(Y);  // All the safety we can wish for.
        srcp += pixel_step;
      }
      srcp -= srcMod;
      dstp += dst_pitch;
    }
  }
  return dst;
}

AVSValue __cdecl ConvertToY8::Create(AVSValue args, void*, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();
  if (clip->GetVideoInfo().IsY8())
    return clip;
  return new ConvertToY8(clip, getMatrix(args[1].AsString(0), env), env);
}


/*****************************************************
 * ConvertRGBToYV24
 ******************************************************/

ConvertRGBToYV24::ConvertRGBToYV24(PClip src, int in_matrix, IScriptEnvironment* env)
  : GenericVideoFilter(src,__FUNCTION__ )
{
  if (!vi.IsRGB())
    env->ThrowError("ConvertRGBToYV24: Only RGB data input accepted");

  pixel_step = vi.BytesFromPixels(1);
  vi.pixel_type = CS_YV24;

  const int shift = 15;

  if (in_matrix == Rec601) {
    /*
    Y'= 0.299*R' + 0.587*G' + 0.114*B'
    Cb=-0.169*R' - 0.331*G' + 0.500*B'
    Cr= 0.500*R' - 0.419*G' - 0.081*B'
    */
    BuildMatrix(0.299,  /* 0.587  */ 0.114,  219, 112, 16, shift);
  }
  else if (in_matrix == PC_601) {

    BuildMatrix(0.299,  /* 0.587  */ 0.114,  255, 127,  0, shift);
  }
  else if (in_matrix == Rec709) {
    /*
    Y'= 0.2126*R' + 0.7152*G' + 0.0722*B'
    Cb=-0.1145*R' - 0.3855*G' + 0.5000*B'
    Cr= 0.5000*R' - 0.4542*G' - 0.0458*B'
    */
    BuildMatrix(0.2126, /* 0.7152 */ 0.0722, 219, 112, 16, shift);
  }
  else if (in_matrix == PC_709) {

    BuildMatrix(0.2126, /* 0.7152 */ 0.0722, 255, 127,  0, shift);
  }
  else if (in_matrix == AVERAGE) {

    BuildMatrix(1.0/3, /* 1.0/3 */ 1.0/3, 255, 127,  0, shift);
  }
  else {
    env->ThrowError("ConvertRGBToYV24: Unknown matrix.");
  }
}

void ConvertRGBToYV24::BuildMatrix(double Kr, double Kb, int Sy, int Suv, int Oy, int shift)
{
/*
  Kr   = {0.299, 0.2126}
  Kb   = {0.114, 0.0722}
  Kg   = 1 - Kr - Kb // {0.587, 0.7152}
  Srgb = 255
  Sy   = {219, 255}
  Suv  = {112, 127}
  Oy   = {16, 0}
  Ouv  = 128

  R = r/Srgb                     // 0..1
  G = g/Srgb
  B = b*Srgb

  Y = Kr*R + Kg*G + Kb*B         // 0..1
  U = B - (Kr*R + Kg*G)/(1-Kb)   //-1..1
  V = R - (Kg*G + Kb*B)/(1-Kr)

  y = Y*Sy  + Oy                 // 16..235, 0..255
  u = U*Suv + Ouv                // 16..240, 1..255
  v = V*Suv + Ouv
*/
  const double mulfac = double(1<<shift);

  const double Kg = 1.- Kr - Kb;
  const int Srgb = 255;

  matrix.y_b  = (signed short)(Sy  * Kb        * mulfac / Srgb + 0.5); //B
  matrix.y_g  = (signed short)(Sy  * Kg        * mulfac / Srgb + 0.5); //G
  matrix.y_r  = (signed short)(Sy  * Kr        * mulfac / Srgb + 0.5); //R
  matrix.u_b  = (signed short)(Suv             * mulfac / Srgb + 0.5);
  matrix.u_g  = (signed short)(Suv * Kg/(Kb-1) * mulfac / Srgb + 0.5);
  matrix.u_r  = (signed short)(Suv * Kr/(Kb-1) * mulfac / Srgb + 0.5);
  matrix.v_b  = (signed short)(Suv * Kb/(Kr-1) * mulfac / Srgb + 0.5);
  matrix.v_g  = (signed short)(Suv * Kg/(Kr-1) * mulfac / Srgb + 0.5);
  matrix.v_r  = (signed short)(Suv             * mulfac / Srgb + 0.5);
  matrix.offset_y = Oy;
}

static void convert_rgb32_to_yv24_sse2(BYTE* dstY, BYTE* dstU, BYTE* dstV, const BYTE*srcp, size_t dst_pitch_y, size_t UVpitch, size_t src_pitch, size_t width, size_t height, const ConversionMatrix &matrix) {
  srcp += src_pitch * (height-1);

  size_t mod8_width = width / 8 * 8;

  __m128i matrix_y = _mm_set_epi16(0, matrix.y_r, matrix.y_g, matrix.y_b, 0, matrix.y_r, matrix.y_g, matrix.y_b);
  __m128i matrix_u = _mm_set_epi16(0, matrix.u_r, matrix.u_g, matrix.u_b, 0, matrix.u_r, matrix.u_g, matrix.u_b);
  __m128i matrix_v = _mm_set_epi16(0, matrix.v_r, matrix.v_g, matrix.v_b, 0, matrix.v_r, matrix.v_g, matrix.v_b);

  __m128i zero = _mm_setzero_si128();
  __m128i offset = _mm_set1_epi16(matrix.offset_y);
  __m128i round_mask = _mm_set1_epi32(16384);
  __m128i v128 = _mm_set1_epi16(128);

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < mod8_width; x+=8) {
      __m128i src0123 = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp+x*4)); //pixels 0, 1, 2 and 3
      __m128i src4567 = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp+x*4+16));//pixels 4, 5, 6 and 7

      __m128i pixel01 = _mm_unpacklo_epi8(src0123, zero); 
      __m128i pixel23 = _mm_unpackhi_epi8(src0123, zero); 
      __m128i pixel45 = _mm_unpacklo_epi8(src4567, zero); 
      __m128i pixel67 = _mm_unpackhi_epi8(src4567, zero); 

      __m128i result_y = convert_rgb_to_y8_sse2_core(pixel01, pixel23, pixel45, pixel67, zero, matrix_y, round_mask, offset);
      __m128i result_u = convert_rgb_to_y8_sse2_core(pixel01, pixel23, pixel45, pixel67, zero, matrix_u, round_mask, v128);
      __m128i result_v = convert_rgb_to_y8_sse2_core(pixel01, pixel23, pixel45, pixel67, zero, matrix_v, round_mask, v128);

      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstY+x), result_y);
      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstU+x), result_u);
      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstV+x), result_v);
    }

    if (mod8_width != width) {
      __m128i src0123 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcp+width*4-32)); //pixels 0, 1, 2 and 3
      __m128i src4567 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcp+width*4-16));//pixels 4, 5, 6 and 7

      __m128i pixel01 = _mm_unpacklo_epi8(src0123, zero); 
      __m128i pixel23 = _mm_unpackhi_epi8(src0123, zero); 
      __m128i pixel45 = _mm_unpacklo_epi8(src4567, zero); 
      __m128i pixel67 = _mm_unpackhi_epi8(src4567, zero); 

      __m128i result_y = convert_rgb_to_y8_sse2_core(pixel01, pixel23, pixel45, pixel67, zero, matrix_y, round_mask, offset);
      __m128i result_u = convert_rgb_to_y8_sse2_core(pixel01, pixel23, pixel45, pixel67, zero, matrix_u, round_mask, v128);
      __m128i result_v = convert_rgb_to_y8_sse2_core(pixel01, pixel23, pixel45, pixel67, zero, matrix_v, round_mask, v128);

      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstY+width-8), result_y);
      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstU+width-8), result_u);
      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstV+width-8), result_v);
    }

    srcp -= src_pitch;
    dstY += dst_pitch_y;
    dstU += UVpitch;
    dstV += UVpitch;
  }
}

static void convert_rgb24_to_yv24_sse2(BYTE* dstY, BYTE* dstU, BYTE* dstV, const BYTE*srcp, size_t dst_pitch_y, size_t UVpitch, size_t src_pitch, size_t width, size_t height, const ConversionMatrix &matrix) {
  srcp += src_pitch * (height-1);

  size_t mod8_width = width / 8 * 8;

  __m128i matrix_y = _mm_set_epi16(0, matrix.y_r, matrix.y_g, matrix.y_b, 0, matrix.y_r, matrix.y_g, matrix.y_b);
  __m128i matrix_u = _mm_set_epi16(0, matrix.u_r, matrix.u_g, matrix.u_b, 0, matrix.u_r, matrix.u_g, matrix.u_b);
  __m128i matrix_v = _mm_set_epi16(0, matrix.v_r, matrix.v_g, matrix.v_b, 0, matrix.v_r, matrix.v_g, matrix.v_b);

  __m128i zero = _mm_setzero_si128();
  __m128i offset = _mm_set1_epi16(matrix.offset_y);
  __m128i round_mask = _mm_set1_epi32(16384);
  __m128i v128 = _mm_set1_epi16(128);

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < mod8_width; x+=8) {
      __m128i pixel01 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+x*3)); //pixels 0 and 1
      __m128i pixel23 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+x*3+6)); //pixels 2 and 3
      __m128i pixel45 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+x*3+12)); //pixels 4 and 5
      __m128i pixel67 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+x*3+18)); //pixels 6 and 7

      //0 0 0 0 0 0 0 0 | x x r1 g1 b1 r0 g0 b0  -> 0 x 0 x 0 r1 0 g1 | 0 b1 0 r0 0 g0 0 b0 -> 0 r1 0 g1 0 b1 0 r0 | 0 b1 0 r0 0 g0 0 b0 -> 0 r1 0 r1 0 g1 0 b1 | 0 b1 0 r0 0 g0 0 b0
      pixel01 = _mm_shufflehi_epi16(_mm_shuffle_epi32(_mm_unpacklo_epi8(pixel01, zero), _MM_SHUFFLE(2, 1, 1, 0)), _MM_SHUFFLE(0, 3, 2, 1));
      pixel23 = _mm_shufflehi_epi16(_mm_shuffle_epi32(_mm_unpacklo_epi8(pixel23, zero), _MM_SHUFFLE(2, 1, 1, 0)), _MM_SHUFFLE(0, 3, 2, 1));
      pixel45 = _mm_shufflehi_epi16(_mm_shuffle_epi32(_mm_unpacklo_epi8(pixel45, zero), _MM_SHUFFLE(2, 1, 1, 0)), _MM_SHUFFLE(0, 3, 2, 1));
      pixel67 = _mm_shufflehi_epi16(_mm_shuffle_epi32(_mm_unpacklo_epi8(pixel67, zero), _MM_SHUFFLE(2, 1, 1, 0)), _MM_SHUFFLE(0, 3, 2, 1));

      __m128i result_y = convert_rgb_to_y8_sse2_core(pixel01, pixel23, pixel45, pixel67, zero, matrix_y, round_mask, offset);
      __m128i result_u = convert_rgb_to_y8_sse2_core(pixel01, pixel23, pixel45, pixel67, zero, matrix_u, round_mask, v128);
      __m128i result_v = convert_rgb_to_y8_sse2_core(pixel01, pixel23, pixel45, pixel67, zero, matrix_v, round_mask, v128);

      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstY+x), result_y);
      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstU+x), result_u);
      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstV+x), result_v);
    }

    if (mod8_width != width) {
      __m128i pixel01 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+width*3-24)); //pixels 0 and 1
      __m128i pixel23 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+width*3-18)); //pixels 2 and 3
      __m128i pixel45 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+width*3-12)); //pixels 4 and 5
      __m128i pixel67 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+width*3-6)); //pixels 6 and 7

      pixel01 = _mm_shufflehi_epi16(_mm_shuffle_epi32(_mm_unpacklo_epi8(pixel01, zero), _MM_SHUFFLE(2, 1, 1, 0)), _MM_SHUFFLE(0, 3, 2, 1));
      pixel23 = _mm_shufflehi_epi16(_mm_shuffle_epi32(_mm_unpacklo_epi8(pixel23, zero), _MM_SHUFFLE(2, 1, 1, 0)), _MM_SHUFFLE(0, 3, 2, 1));
      pixel45 = _mm_shufflehi_epi16(_mm_shuffle_epi32(_mm_unpacklo_epi8(pixel45, zero), _MM_SHUFFLE(2, 1, 1, 0)), _MM_SHUFFLE(0, 3, 2, 1));
      pixel67 = _mm_shufflehi_epi16(_mm_shuffle_epi32(_mm_unpacklo_epi8(pixel67, zero), _MM_SHUFFLE(2, 1, 1, 0)), _MM_SHUFFLE(0, 3, 2, 1)); 

      __m128i result_y = convert_rgb_to_y8_sse2_core(pixel01, pixel23, pixel45, pixel67, zero, matrix_y, round_mask, offset);
      __m128i result_u = convert_rgb_to_y8_sse2_core(pixel01, pixel23, pixel45, pixel67, zero, matrix_u, round_mask, v128);
      __m128i result_v = convert_rgb_to_y8_sse2_core(pixel01, pixel23, pixel45, pixel67, zero, matrix_v, round_mask, v128);

      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstY+width-8), result_y);
      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstU+width-8), result_u);
      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstV+width-8), result_v);
    }

    srcp -= src_pitch;
    dstY += dst_pitch_y;
    dstU += UVpitch;
    dstV += UVpitch;
  }
}

#ifdef _M_IX86

static void convert_rgb32_to_yv24_mmx(BYTE* dstY, BYTE* dstU, BYTE* dstV, const BYTE*srcp, size_t dst_pitch_y, size_t UVpitch, size_t src_pitch, size_t width, size_t height, const ConversionMatrix& matrix) {
  srcp += src_pitch * (height-1);

  size_t mod4_width = width / 4 * 4;

  __m64 matrix_y = _mm_set_pi16(0, matrix.y_r, matrix.y_g, matrix.y_b);
  __m64 matrix_u = _mm_set_pi16(0, matrix.u_r, matrix.u_g, matrix.u_b);
  __m64 matrix_v = _mm_set_pi16(0, matrix.v_r, matrix.v_g, matrix.v_b);

  __m64 zero = _mm_setzero_si64();
  __m64 offset = _mm_set1_pi16(matrix.offset_y);
  __m64 round_mask = _mm_set1_pi32(16384);
  __m64 v128 = _mm_set1_pi16(128);

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < mod4_width; x+=4) {
      __m64 src01 = *reinterpret_cast<const __m64*>(srcp+x*4); //pixels 0 and 1
      __m64 src23 = *reinterpret_cast<const __m64*>(srcp+x*4+8);//pixels 2 and 3

      __m64 pixel0 = _mm_unpacklo_pi8(src01, zero); //a0 r0 g0 b0
      __m64 pixel1 = _mm_unpackhi_pi8(src01, zero); //a1 r1 g1 b1
      __m64 pixel2 = _mm_unpacklo_pi8(src23, zero); //a2 r2 g2 b2
      __m64 pixel3 = _mm_unpackhi_pi8(src23, zero); //a3 r3 g3 b3

      *reinterpret_cast<int*>(dstY+x) = convert_rgb_to_y8_mmx_core(pixel0, pixel1, pixel2, pixel3, zero, matrix_y, round_mask, offset);
      *reinterpret_cast<int*>(dstU+x) = convert_rgb_to_y8_mmx_core(pixel0, pixel1, pixel2, pixel3, zero, matrix_u, round_mask, v128);
      *reinterpret_cast<int*>(dstV+x) = convert_rgb_to_y8_mmx_core(pixel0, pixel1, pixel2, pixel3, zero, matrix_v, round_mask, v128);
    }

    if (mod4_width != width) {
      __m64 src01 = *reinterpret_cast<const __m64*>(srcp+width*4-16);
      __m64 src23 = *reinterpret_cast<const __m64*>(srcp+width*4-8);

      __m64 pixel0 = _mm_unpacklo_pi8(src01, zero); //a0 r0 g0 b0
      __m64 pixel1 = _mm_unpackhi_pi8(src01, zero); //a1 r1 g1 b1
      __m64 pixel2 = _mm_unpacklo_pi8(src23, zero); //a2 r2 g2 b2
      __m64 pixel3 = _mm_unpackhi_pi8(src23, zero); //a3 r3 g3 b3

      *reinterpret_cast<int*>(dstY+width-4) = convert_rgb_to_y8_mmx_core(pixel0, pixel1, pixel2, pixel3, zero, matrix_y, round_mask, offset);
      *reinterpret_cast<int*>(dstU+width-4) = convert_rgb_to_y8_mmx_core(pixel0, pixel1, pixel2, pixel3, zero, matrix_u, round_mask, v128);
      *reinterpret_cast<int*>(dstV+width-4) = convert_rgb_to_y8_mmx_core(pixel0, pixel1, pixel2, pixel3, zero, matrix_v, round_mask, v128);
    }

    srcp -= src_pitch;
    dstY += dst_pitch_y;
    dstU += UVpitch;
    dstV += UVpitch;
  }
  _mm_empty();
}

static void convert_rgb24_to_yv24_mmx(BYTE* dstY, BYTE* dstU, BYTE* dstV, const BYTE*srcp, size_t dst_pitch_y, size_t UVpitch, size_t src_pitch, size_t width, size_t height, const ConversionMatrix &matrix) {
  srcp += src_pitch * (height-1);

  size_t mod4_width = width / 4 * 4;

  __m64 matrix_y = _mm_set_pi16(0, matrix.y_r, matrix.y_g, matrix.y_b);
  __m64 matrix_u = _mm_set_pi16(0, matrix.u_r, matrix.u_g, matrix.u_b);
  __m64 matrix_v = _mm_set_pi16(0, matrix.v_r, matrix.v_g, matrix.v_b);

  __m64 zero = _mm_setzero_si64();
  __m64 offset = _mm_set1_pi16(matrix.offset_y);
  __m64 round_mask = _mm_set1_pi32(16384);
  __m64 v128 = _mm_set1_pi16(128);

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < mod4_width; x+=4) {
      __m64 pixel0 = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcp+x*3)); //pixel 0
      __m64 pixel1 = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcp+x*3+3)); //pixel 1
      __m64 pixel2 = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcp+x*3+6)); //pixel 2
      __m64 pixel3 = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcp+x*3+9)); //pixel 3

      pixel0 = _mm_unpacklo_pi8(pixel0, zero);
      pixel1 = _mm_unpacklo_pi8(pixel1, zero);
      pixel2 = _mm_unpacklo_pi8(pixel2, zero);
      pixel3 = _mm_unpacklo_pi8(pixel3, zero);

      *reinterpret_cast<int*>(dstY+x) = convert_rgb_to_y8_mmx_core(pixel0, pixel1, pixel2, pixel3, zero, matrix_y, round_mask, offset);
      *reinterpret_cast<int*>(dstU+x) = convert_rgb_to_y8_mmx_core(pixel0, pixel1, pixel2, pixel3, zero, matrix_u, round_mask, v128);
      *reinterpret_cast<int*>(dstV+x) = convert_rgb_to_y8_mmx_core(pixel0, pixel1, pixel2, pixel3, zero, matrix_v, round_mask, v128);
    }

    if (mod4_width != width) {
      __m64 pixel0 = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcp+width*3-12)); //pixel 0
      __m64 pixel1 = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcp+width*3-9)); //pixel 1
      __m64 pixel2 = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcp+width*3-6)); //pixel 2
      __m64 pixel3 = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcp+width*3-3)); //pixel 3

      pixel0 = _mm_unpacklo_pi8(pixel0, zero);
      pixel1 = _mm_unpacklo_pi8(pixel1, zero);
      pixel2 = _mm_unpacklo_pi8(pixel2, zero);
      pixel3 = _mm_unpacklo_pi8(pixel3, zero);

      *reinterpret_cast<int*>(dstY+width-4) = convert_rgb_to_y8_mmx_core(pixel0, pixel1, pixel2, pixel3, zero, matrix_y, round_mask, offset);
      *reinterpret_cast<int*>(dstU+width-4) = convert_rgb_to_y8_mmx_core(pixel0, pixel1, pixel2, pixel3, zero, matrix_u, round_mask, v128);
      *reinterpret_cast<int*>(dstV+width-4) = convert_rgb_to_y8_mmx_core(pixel0, pixel1, pixel2, pixel3, zero, matrix_v, round_mask, v128);
    }

    srcp -= src_pitch;
    dstY += dst_pitch_y;
    dstU += UVpitch;
    dstV += UVpitch;
  }
  _mm_empty();
}

#endif


PVideoFrame __stdcall ConvertRGBToYV24::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame src = child->GetFrame(n, env);
  if (src == nullptr || src.m_ptr == nullptr) {
      return nullptr;
  }
  PVideoFrame dst = env->NewVideoFrame(vi);

  if (dst == nullptr || dst.m_ptr == nullptr) {
      return nullptr;
  }
  const BYTE* srcp = src->GetReadPtr();

  BYTE* dstY = dst->GetWritePtr(PLANAR_Y);
  BYTE* dstU = dst->GetWritePtr(PLANAR_U);
  BYTE* dstV = dst->GetWritePtr(PLANAR_V);

  const int Spitch = src->GetPitch();

  const int Ypitch = dst->GetPitch(PLANAR_Y);
  const int UVpitch = dst->GetPitch(PLANAR_U);

  const int awidth = dst->GetRowSize(PLANAR_Y_ALIGNED);

  if (pixel_step != 4 && pixel_step != 3) {
    env->ThrowError("Invalid pixel step. This is a bug.");
  }

  if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(srcp, 16)) {
    if (pixel_step == 4) {
      convert_rgb32_to_yv24_sse2(dstY, dstU, dstV, srcp, Ypitch, UVpitch, Spitch, vi.width, vi.height, matrix);
    } else {
      convert_rgb24_to_yv24_sse2(dstY, dstU, dstV, srcp, Ypitch, UVpitch, Spitch, vi.width, vi.height, matrix);
    }
    return dst;
  }

#ifdef _M_IX86
  if ((env->GetCPUFlags() & CPUF_MMX)) {
    if (pixel_step == 4) {
      convert_rgb32_to_yv24_mmx(dstY, dstU, dstV, srcp, Ypitch, UVpitch, Spitch, vi.width, vi.height, matrix);
    } else {
      convert_rgb24_to_yv24_mmx(dstY, dstU, dstV, srcp, Ypitch, UVpitch, Spitch, vi.width, vi.height, matrix);
    }
    return dst;
  }
#endif

  //Slow C-code.

  ConversionMatrix &m = matrix;
  srcp += Spitch * (vi.height-1);  // We start at last line
  const int Sstep = Spitch + (vi.width * pixel_step);
  for (int y = 0; y < vi.height; y++) {
    for (int x = 0; x < vi.width; x++) {
      int b = srcp[0];
      int g = srcp[1];
      int r = srcp[2];
      int Y = m.offset_y + (((int)m.y_b * b + (int)m.y_g * g + (int)m.y_r * r + 16384)>>15);
      int U = 128+(((int)m.u_b * b + (int)m.u_g * g + (int)m.u_r * r + 16384)>>15);
      int V = 128+(((int)m.v_b * b + (int)m.v_g * g + (int)m.v_r * r + 16384)>>15);
      *dstY++ = PixelClip(Y);  // All the safety we can wish for.
      *dstU++ = PixelClip(U);
      *dstV++ = PixelClip(V);
      srcp += pixel_step;
    }
    srcp -= Sstep;
    dstY += Ypitch - vi.width;
    dstU += UVpitch - vi.width;
    dstV += UVpitch - vi.width;
  }
  return dst;
}

AVSValue __cdecl ConvertRGBToYV24::Create(AVSValue args, void*, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();
  if (clip->GetVideoInfo().IsYV24())
    return clip;
  return new ConvertRGBToYV24(clip, getMatrix(args[1].AsString(0), env), env);
}




static __forceinline __m128i convert_yuv_to_rgb_sse2_core(const __m128i &px01, const __m128i &px23, const __m128i &px45, const __m128i &px67, const __m128i& zero, const __m128i &matrix, const __m128i &round_mask) {
  //int b = (((int)m[0] * Y + (int)m[1] * U + (int)m[ 2] * V + 4096)>>13);
  
  //px01 - xx xx 00 V1 00 U1 00 Y1 xx xx 00 V0 00 U0 00 Y0

  __m128i low_lo  = _mm_madd_epi16(px01, matrix); //xx*0 + v1*m2 | u1*m1 + y1*m0 | xx*0 + v0*m2 | u0*m1 + y0*m0
  __m128i low_hi  = _mm_madd_epi16(px23, matrix); //xx*0 + v3*m2 | u3*m1 + y3*m0 | xx*0 + v2*m2 | u2*m1 + y2*m0
  __m128i high_lo = _mm_madd_epi16(px45, matrix); 
  __m128i high_hi = _mm_madd_epi16(px67, matrix); 

  __m128i low_v  = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(low_lo), _mm_castsi128_ps(low_hi), _MM_SHUFFLE(3, 1, 3, 1))); // v3*m2 | v2*m2 | v1*m2 | v0*m2
  __m128i high_v = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(high_lo), _mm_castsi128_ps(high_hi), _MM_SHUFFLE(3, 1, 3, 1))); 

  __m128i low_yu  = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(low_lo), _mm_castsi128_ps(low_hi), _MM_SHUFFLE(2, 0, 2, 0))); // u3*m1 + y3*m0 | u2*m1 + y2*m0 | u1*m1 + y1*m0 | u0*m1 + y0*m0
  __m128i high_yu = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(high_lo), _mm_castsi128_ps(high_hi), _MM_SHUFFLE(2, 0, 2, 0)));

  __m128i t_lo = _mm_add_epi32(low_v, low_yu); // v3*m2 + u3*m1 + y3*m0...
  __m128i t_hi = _mm_add_epi32(high_v, high_yu); 

  t_lo = _mm_add_epi32(t_lo, round_mask); // v3*m2 + u3*m1 + y3*m0 + 4096...
  t_hi = _mm_add_epi32(t_hi, round_mask);

  t_lo = _mm_srai_epi32(t_lo, 13); // (v3*m2 + u3*m1 + y3*m0 + 4096) >> 13...
  t_hi = _mm_srai_epi32(t_hi, 13); 

  __m128i result = _mm_packs_epi32(t_lo, t_hi); 
  result = _mm_packus_epi16(result, zero); //00 00 00 00 00 00 00 00 b7 b6 b5 b4 b3 b2 b1 b0
  return result;
}

//todo: consider rewriting
template<int rgb_pixel_step, int instruction_set>
static void convert_yv24_to_rgb_ssex(BYTE* dstp, const BYTE* srcY, const BYTE* srcU, const BYTE*srcV, size_t dst_pitch, size_t src_pitch_y, size_t src_pitch_uv, size_t width, size_t height, const ConversionMatrix &matrix) {
  dstp += dst_pitch * (height-1);  // We start at last line

  size_t mod8_width = width / 8 * 8;

  __m128i matrix_b = _mm_set_epi16(0, matrix.v_b, matrix.u_b, matrix.y_b, 0, matrix.v_b, matrix.u_b, matrix.y_b);
  __m128i matrix_g = _mm_set_epi16(0, matrix.v_g, matrix.u_g, matrix.y_g, 0, matrix.v_g, matrix.u_g, matrix.y_g);
  __m128i matrix_r = _mm_set_epi16(0, matrix.v_r, matrix.u_r, matrix.y_r, 0, matrix.v_r, matrix.u_r, matrix.y_r);

  __m128i zero = _mm_setzero_si128();
  __m128i round_mask = _mm_set1_epi32(4096);
#pragma warning(push)
#pragma warning(disable: 4309)
  __m128i ff = _mm_set1_epi8(0xFF);
#pragma warning(pop)
  __m128i offset = _mm_set_epi16(0, -128, -128, matrix.offset_y, 0, -128, -128, matrix.offset_y);
  __m128i pixels0123_mask = _mm_set_epi8(0, 0, 0, 0, 14, 13, 12, 10, 9, 8, 6, 5, 4, 2, 1, 0);
  __m128i pixels4567_mask = _mm_set_epi8(4, 2, 1, 0, 0, 0, 0, 0, 14, 13, 12, 10, 9, 8, 6, 5);
  __m128i ssse3_merge_mask = _mm_set_epi32(0xFFFFFFFF, 0, 0, 0);
  BYTE temp[32];

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < mod8_width; x+=8) {
      __m128i src_y = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcY+x)); //0 0 0 0 0 0 0 0 Y7 Y6 Y5 Y4 Y3 Y2 Y1 Y0
      __m128i src_u = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcU+x)); //0 0 0 0 0 0 0 0 U7 U6 U5 U4 U3 U2 U1 U0
      __m128i src_v = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcV+x)); //0 0 0 0 0 0 0 0 V7 V6 V5 V4 V3 V2 V1 V0
      
      __m128i t1 = _mm_unpacklo_epi8(src_y, src_u); //U7 Y7 U6 Y6 U5 Y5 U4 Y4 U3 Y3 U2 Y2 U1 Y1 U0 Y0
      __m128i t2 = _mm_unpacklo_epi8(src_v, zero);  //00 V7 00 V6 00 V5 00 V4 00 V3 00 V2 00 V1 00 V0

      __m128i low  = _mm_unpacklo_epi16(t1, t2); //xx V3 U3 Y3 xx V2 U2 Y2 xx V1 U1 Y1 xx V0 U0 Y0
      __m128i high = _mm_unpackhi_epi16(t1, t2); //xx V7 U7 Y7 xx V6 U6 Y6 xx V5 U5 Y5 xx V4 U4 Y4

      __m128i px01 = _mm_unpacklo_epi8(low, zero);  //xx xx 00 V1 00 U1 00 Y1 xx xx 00 V0 00 U0 00 Y0
      __m128i px23 = _mm_unpackhi_epi8(low, zero);  //xx xx 00 V3 00 U3 00 Y3 xx xx 00 V2 00 U2 00 Y2
      __m128i px45 = _mm_unpacklo_epi8(high, zero); //xx xx 00 V5 00 U5 00 Y5 xx xx 00 V4 00 U4 00 Y4
      __m128i px67 = _mm_unpackhi_epi8(high, zero); //xx xx 00 V7 00 U7 00 Y7 xx xx 00 V6 00 U6 00 Y6
      
      px01 = _mm_add_epi16(px01, offset); 
      px23 = _mm_add_epi16(px23, offset); 
      px45 = _mm_add_epi16(px45, offset);
      px67 = _mm_add_epi16(px67, offset);
      
      __m128i result_b = convert_yuv_to_rgb_sse2_core(px01, px23, px45, px67, zero, matrix_b, round_mask); //00 00 00 00 00 00 00 00 b7 b6 b5 b4 b3 b2 b1 b0
      __m128i result_g = convert_yuv_to_rgb_sse2_core(px01, px23, px45, px67, zero, matrix_g, round_mask); //00 00 00 00 00 00 00 00 g7 g6 g5 g4 g3 g2 g1 g0
      __m128i result_r = convert_yuv_to_rgb_sse2_core(px01, px23, px45, px67, zero, matrix_r, round_mask); //00 00 00 00 00 00 00 00 r7 r6 r5 r4 r3 r2 r1 r0

      __m128i result_bg = _mm_unpacklo_epi8(result_b, result_g); //g7 b7 g6 b6 g5 b5 g4 b4 g3 b3 g2 b2 g1 b1 g0 b0
      __m128i result_ra = _mm_unpacklo_epi8(result_r, ff);       //a7 r7 a6 r6 a5 r5 a4 r4 a3 r3 a2 r2 a1 r1 a0 r0

      __m128i result_lo = _mm_unpacklo_epi16(result_bg, result_ra);
      __m128i result_hi = _mm_unpackhi_epi16(result_bg, result_ra);

      if (rgb_pixel_step == 4) {
        //rgb32
        _mm_store_si128(reinterpret_cast<__m128i*>(dstp+x*4),    result_lo);
        _mm_store_si128(reinterpret_cast<__m128i*>(dstp+x*4+16), result_hi);
      } else {
        //rgb24
        if (instruction_set == CPUF_SSSE3) {
          //"fast" SSSE3 version
          __m128i px0123 = _mm_shuffle_epi8(result_lo, pixels0123_mask); //xxxx xxxx b3g3 r3b2 g2r2 b1g1 r1b0 g0r0
          __m128i dst567 = _mm_shuffle_epi8(result_hi, pixels4567_mask); //r5b4 g4r4 xxxx xxxx b7g7 r7b6 g6r6 b5g5

          __m128i dst012345 = _mm_or_si128(
            _mm_andnot_si128(ssse3_merge_mask, px0123),
            _mm_and_si128(ssse3_merge_mask, dst567)
            ); //r5b4 g4r4 b3g3 r3b2 g2r2 b1g1 r1b0 g0r0

          _mm_storeu_si128(reinterpret_cast<__m128i*>(dstp+x*3), dst012345);
          _mm_storel_epi64(reinterpret_cast<__m128i*>(dstp+x*3+16), dst567);

        } else {
          //slow SSE2 version
          _mm_store_si128(reinterpret_cast<__m128i*>(temp),    result_lo);
          _mm_store_si128(reinterpret_cast<__m128i*>(temp+16), result_hi);

          for (int i = 0; i < 8; ++i) {
            *reinterpret_cast<int*>(dstp + (x+i)*3) = *reinterpret_cast<int*>(temp+i*4);
          }
          //last pixel
          dstp[(x+7)*3+0] = temp[7*4+0];
          dstp[(x+7)*3+1] = temp[7*4+1];
          dstp[(x+7)*3+2] = temp[7*4+2];
        }
      }
    }

    for (size_t x = mod8_width; x < width; ++x) {
      int Y = srcY[x] + matrix.offset_y;
      int U = srcU[x] - 128;
      int V = srcV[x] - 128;
      int b = (((int)matrix.y_b * Y + (int)matrix.u_b * U + (int)matrix.v_b * V + 4096)>>13);
      int g = (((int)matrix.y_g * Y + (int)matrix.u_g * U + (int)matrix.v_g * V + 4096)>>13);
      int r = (((int)matrix.y_r * Y + (int)matrix.u_r * U + (int)matrix.v_r * V + 4096)>>13);
      dstp[x*rgb_pixel_step+0] = PixelClip(b);  
      dstp[x*rgb_pixel_step+1] = PixelClip(g);  
      dstp[x*rgb_pixel_step+2] = PixelClip(r);
      if (rgb_pixel_step == 4) {
        dstp[x*4+3] = 255; 
      }
    }

    dstp -= dst_pitch;
    srcY += src_pitch_y;
    srcU += src_pitch_uv;
    srcV += src_pitch_uv;
  }
}


#ifdef _M_IX86

static __forceinline __m64 convert_yuv_to_rgb_mmx_core(const __m64 &px0, const __m64 &px1, const __m64 &px2, const __m64 &px3, const __m64& zero, const __m64 &matrix, const __m64 &round_mask) {
  //int b = (((int)m[0] * Y + (int)m[1] * U + (int)m[ 2] * V + 4096)>>13);

  //px01 - xx xx 00 V0 00 U0 00 Y0

  __m64 low_lo  = _mm_madd_pi16(px0, matrix); //xx*0 + v1*m2 | u1*m1 + y1*m0 | xx*0 + v0*m2 | u0*m1 + y0*m0
  __m64 low_hi  = _mm_madd_pi16(px1, matrix); //xx*0 + v3*m2 | u3*m1 + y3*m0 | xx*0 + v2*m2 | u2*m1 + y2*m0
  __m64 high_lo = _mm_madd_pi16(px2, matrix); 
  __m64 high_hi = _mm_madd_pi16(px3, matrix); 

  __m64 low_v = _mm_unpackhi_pi32(low_lo, low_hi); // v1*m2 | v0*m2
  __m64 high_v = _mm_unpackhi_pi32(high_lo, high_hi);

  __m64 low_yu = _mm_unpacklo_pi32(low_lo, low_hi); // u1*m1 + y1*m0 | u0*m1 + y0*m0
  __m64 high_yu = _mm_unpacklo_pi32(high_lo, high_hi); 

  __m64 t_lo = _mm_add_pi32(low_v, low_yu); // v3*m2 + u3*m1 + y3*m0...
  __m64 t_hi = _mm_add_pi32(high_v, high_yu); 

  t_lo = _mm_add_pi32(t_lo, round_mask); // v3*m2 + u3*m1 + y3*m0 + 4096...
  t_hi = _mm_add_pi32(t_hi, round_mask);

  t_lo = _mm_srai_pi32(t_lo, 13); // (v3*m2 + u3*m1 + y3*m0 + 4096) >> 13...
  t_hi = _mm_srai_pi32(t_hi, 13); 

  __m64 result = _mm_packs_pi32(t_lo, t_hi); 
  result = _mm_packs_pu16(result, zero); //00 00 00 00 b3 b2 b1 b0
  return result;
}

template<int rgb_pixel_step>
static void convert_yv24_to_rgb_mmx(BYTE* dstp, const BYTE* srcY, const BYTE* srcU, const BYTE*srcV, size_t dst_pitch, size_t src_pitch_y, size_t src_pitch_uv, size_t width, size_t height, const ConversionMatrix &matrix) {
  dstp += dst_pitch * (height-1);  // We start at last line

  size_t mod4_width = width / 4 * 4;

  __m64 matrix_b = _mm_set_pi16(0, matrix.v_b, matrix.u_b, matrix.y_b);
  __m64 matrix_g = _mm_set_pi16(0, matrix.v_g, matrix.u_g, matrix.y_g);
  __m64 matrix_r = _mm_set_pi16(0, matrix.v_r, matrix.u_r, matrix.y_r);

  __m64 zero = _mm_setzero_si64();
  __m64 round_mask = _mm_set1_pi32(4096);
#pragma warning(push)
#pragma warning(disable: 4309)
  __m64 ff = _mm_set1_pi8(0xFF);
#pragma warning(pop)
  __m64 offset = _mm_set_pi16(0, -128, -128, matrix.offset_y);
  __m64 low_pixel_mask = _mm_set_pi32(0, 0x00FFFFFF);
  __m64 high_pixel_mask = _mm_set_pi32(0x00FFFFFF, 0);

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < mod4_width; x+=4) {
      __m64 src_y = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcY+x)); //0 0 0 0 Y3 Y2 Y1 Y0
      __m64 src_u = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcU+x)); //0 0 0 0 U3 U2 U1 U0
      __m64 src_v = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcV+x)); //0 0 0 0 V3 V2 V1 V0

      __m64 t1 = _mm_unpacklo_pi8(src_y, src_u); //U3 Y3 U2 Y2 U1 Y1 U0 Y0
      __m64 t2 = _mm_unpacklo_pi8(src_v, zero);  //00 V3 00 V2 00 V1 00 V0

      __m64 low  = _mm_unpacklo_pi16(t1, t2); //xx V1 U1 Y1 xx V0 U0 Y0
      __m64 high = _mm_unpackhi_pi16(t1, t2); //xx V3 U3 Y3 xx V2 U2 Y2

      __m64 px0 = _mm_unpacklo_pi8(low, zero);  //xx xx 00 V0 00 U0 00 Y0
      __m64 px1 = _mm_unpackhi_pi8(low, zero);  //xx xx 00 V1 00 U1 00 Y1
      __m64 px2 = _mm_unpacklo_pi8(high, zero); //xx xx 00 V2 00 U2 00 Y2
      __m64 px3 = _mm_unpackhi_pi8(high, zero); //xx xx 00 V3 00 U3 00 Y3

      px0 = _mm_add_pi16(px0, offset); 
      px1 = _mm_add_pi16(px1, offset); 
      px2 = _mm_add_pi16(px2, offset);
      px3 = _mm_add_pi16(px3, offset);

      __m64 result_b = convert_yuv_to_rgb_mmx_core(px0, px1, px2, px3, zero, matrix_b, round_mask); //00 00 00 00 b3 b2 b1 b0
      __m64 result_g = convert_yuv_to_rgb_mmx_core(px0, px1, px2, px3, zero, matrix_g, round_mask); //00 00 00 00 g3 g2 g1 g0
      __m64 result_r = convert_yuv_to_rgb_mmx_core(px0, px1, px2, px3, zero, matrix_r, round_mask); //00 00 00 00 r3 r2 r1 r0

      __m64 result_bg = _mm_unpacklo_pi8(result_b, result_g); //g3 b3 g2 b2 g1 b1 g0 b0
      __m64 result_ra = _mm_unpacklo_pi8(result_r, ff);       //a3 r3 a2 r2 a1 r1 a0 r0

      __m64 result_lo = _mm_unpacklo_pi16(result_bg, result_ra);
      __m64 result_hi = _mm_unpackhi_pi16(result_bg, result_ra);

      if (rgb_pixel_step == 4) {
        //rgb32
        *reinterpret_cast<__m64*>(dstp+x*4) = result_lo;
        *reinterpret_cast<__m64*>(dstp+x*4+8) = result_hi;
      } else {
        __m64 p0 = _mm_and_si64(result_lo, low_pixel_mask); //0000 0000 00r0 g0b0
        __m64 p1 = _mm_and_si64(result_lo, high_pixel_mask); //00r1 g1b1 0000 0000
        __m64 p2 = _mm_and_si64(result_hi, low_pixel_mask); //0000 0000 00r2 g2b2
        __m64 p3 = _mm_and_si64(result_hi, high_pixel_mask); //00r3 g3b3 0000 0000

        __m64 dst01 = _mm_or_si64(p0, _mm_srli_si64(p1, 8)); //0000 r1g1 b1r0 g0b0
        p3 = _mm_srli_si64(p3, 24); //0000 0000 r3g3 b300

        __m64 dst012 = _mm_or_si64(dst01, _mm_slli_si64(p2, 48));  //g2b2 r1g1 b1r0 g0b0
        __m64 dst23 = _mm_or_si64(p3, _mm_srli_si64(p2, 16)); //0000 0000 r3g3 b3r2

        *reinterpret_cast<__m64*>(dstp+x*3) = dst012;
        *reinterpret_cast<int*>(dstp+x*3+8) = _mm_cvtsi64_si32(dst23);
      }
    }

    for (size_t x = mod4_width; x < width; ++x) {
      int Y = srcY[x] + matrix.offset_y;
      int U = srcU[x] - 128;
      int V = srcV[x] - 128;
      int b = (((int)matrix.y_b * Y + (int)matrix.u_b * U + (int)matrix.v_b * V + 4096)>>13);
      int g = (((int)matrix.y_g * Y + (int)matrix.u_g * U + (int)matrix.v_g * V + 4096)>>13);
      int r = (((int)matrix.y_r * Y + (int)matrix.u_r * U + (int)matrix.v_r * V + 4096)>>13);
      dstp[x*rgb_pixel_step+0] = PixelClip(b);  
      dstp[x*rgb_pixel_step+1] = PixelClip(g);  
      dstp[x*rgb_pixel_step+2] = PixelClip(r);
      if (rgb_pixel_step == 4) {
        dstp[x*4+3] = 255; 
      }
    }

    dstp -= dst_pitch;
    srcY += src_pitch_y;
    srcU += src_pitch_uv;
    srcV += src_pitch_uv;
  }
  _mm_empty();
}

#endif



/************************************
 * YUY2 to YV16
 ************************************/

ConvertYUY2ToYV16::ConvertYUY2ToYV16(PClip src, IScriptEnvironment* env) : GenericVideoFilter(src,__FUNCTION__ ) {

  if (!vi.IsYUY2())
    env->ThrowError("ConvertYUY2ToYV16: Only YUY2 is allowed as input");

  vi.pixel_type = CS_YV16;

}

template<int instruction_set>
void convert_yuy2_to_yv16_xsse(const BYTE *srcp, BYTE *dstp_y, BYTE *dstp_u, BYTE *dstp_v, size_t src_pitch, size_t dst_pitch_y, size_t dst_pitch_uv, size_t width, size_t height)
{
  __m128i low_byte_mask = _mm_set1_epi16(0x00FF);
  size_t half_width = width / 2;
  size_t mod8 = half_width / 8 * 8;
  __m128i u_mask = _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 14, 12, 10, 8, 6, 4, 2, 0);
  __m128i v_mask = _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 15, 13, 11, 9, 7, 5, 3, 1);

  for (size_t y=0; y<height; y++) { 
    for (size_t x=0; x<mod8; x+=8) {
      __m128i p0 = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp + x*4));
      __m128i p1 = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp + x*4 + 16));

      __m128i p0_luma = _mm_and_si128(p0, low_byte_mask);
      __m128i p1_luma = _mm_and_si128(p1, low_byte_mask); 
      __m128i luma = _mm_packus_epi16(p0_luma, p1_luma);
      _mm_store_si128(reinterpret_cast<__m128i*>(dstp_y + x*2), luma);

      __m128i p0_chroma = _mm_srli_epi16(p0, 8); //00 V3 00 U3 00 V2 00 U2 00 V1 00 U1 00 V0 00 U0
      __m128i p1_chroma = _mm_srli_epi16(p1, 8); //00 V7 00 U7 00 V6 00 U6 00 V5 00 U5 00 V4 00 U4

      __m128i tmp_chroma = _mm_packus_epi16(p0_chroma, p1_chroma); //V U V U V U V U V U

      __m128i chroma_u, chroma_v;

      if (instruction_set == CPUF_SSE2) {
        __m128i chroma_u16 = _mm_and_si128(tmp_chroma, low_byte_mask);
        __m128i chroma_v16 = _mm_srli_epi16(tmp_chroma, 8);

        chroma_u = _mm_packus_epi16(chroma_u16, chroma_u16);
        chroma_v = _mm_packus_epi16(chroma_v16, chroma_v16);
      } else {
        chroma_u = _mm_shuffle_epi8(tmp_chroma, u_mask);
        chroma_v = _mm_shuffle_epi8(tmp_chroma, v_mask);
      }

      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstp_u + x), chroma_u);
      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstp_v + x), chroma_v);
    }

    for (size_t x=mod8; x<half_width; x++) {
      dstp_y[x*2]   = srcp[x*4+0];
      dstp_y[x*2+1] = srcp[x*4+2];
      dstp_u[x]     = srcp[x*4+1];
      dstp_v[x]     = srcp[x*4+3];
    }

    srcp += src_pitch;
    dstp_y += dst_pitch_y;
    dstp_u += dst_pitch_uv;
    dstp_v += dst_pitch_uv;
  }
}


#ifdef _M_IX86

void convert_yuy2_to_yv16_mmx(const BYTE *srcp, BYTE *dstp_y, BYTE *dstp_u, BYTE *dstp_v, size_t src_pitch, size_t dst_pitch_y, size_t dst_pitch_uv, size_t width, size_t height)
{
  __m64 low_byte_mask = _mm_set1_pi16(0x00FF);
  size_t half_width = width / 2;
  size_t mod4 = half_width / 4 * 4;

  for (size_t y=0; y<height; y++) { 
    for (size_t x=0; x<mod4; x+=4) {
      __m64 p0 = *reinterpret_cast<const __m64*>(srcp + x*4);
      __m64 p1 = *reinterpret_cast<const __m64*>(srcp + x*4 + 8);

      __m64 p0_luma = _mm_and_si64(p0, low_byte_mask);
      __m64 p0_chroma = _mm_srli_pi16(p0, 8); //0 V 0 U 0 V 0 U
      __m64 p1_luma = _mm_and_si64(p1, low_byte_mask); 
      __m64 p1_chroma = _mm_srli_pi16(p1, 8);

      __m64 luma = _mm_packs_pu16(p0_luma, p1_luma);
      *reinterpret_cast<__m64*>(dstp_y + x*2) = luma;

      __m64 tmp_chroma = _mm_packs_pu16(p0_chroma, p1_chroma); //V U V U V U V U V U

      __m64 chroma_u16 = _mm_and_si64(tmp_chroma, low_byte_mask);
      __m64 chroma_v16 = _mm_srli_pi16(tmp_chroma, 8);

      __m64 chroma_u = _mm_packs_pu16(chroma_u16, chroma_u16);
      __m64 chroma_v = _mm_packs_pu16(chroma_v16, chroma_v16);

      
      *reinterpret_cast<int*>(dstp_u + x) = _mm_cvtsi64_si32(chroma_u);
      *reinterpret_cast<int*>(dstp_v + x) = _mm_cvtsi64_si32(chroma_v);
    }

    for (size_t x=mod4; x<half_width; x++) {
      dstp_y[x*2]   = srcp[x*4+0];
      dstp_y[x*2+1] = srcp[x*4+2];
      dstp_u[x]     = srcp[x*4+1];
      dstp_v[x]     = srcp[x*4+3];
    }

    srcp += src_pitch;
    dstp_y += dst_pitch_y;
    dstp_u += dst_pitch_uv;
    dstp_v += dst_pitch_uv;
  }
  _mm_empty();
}

#endif

void convert_yuy2_to_yv16_c(const BYTE *srcp, BYTE *dstp_y, BYTE *dstp_u, BYTE *dstp_v, size_t src_pitch, size_t dst_pitch_y, size_t dst_pitch_uv, size_t width, size_t height)
{
  for (size_t y=0; y<height; y++) { 
    for (size_t x=0; x<width/2; x++) {
      dstp_y[x*2]   = srcp[x*4+0];
      dstp_y[x*2+1] = srcp[x*4+2];
      dstp_u[x]     = srcp[x*4+1];
      dstp_v[x]     = srcp[x*4+3];
    }
    srcp += src_pitch;
    dstp_y += dst_pitch_y;
    dstp_u += dst_pitch_uv;
    dstp_v += dst_pitch_uv;
  }
}

PVideoFrame __stdcall ConvertYUY2ToYV16::GetFrame(int n, IScriptEnvironment* env) {
  PVideoFrame src = child->GetFrame(n, env);
  if (src == nullptr || src.m_ptr == nullptr) {
      return nullptr;
  }
  PVideoFrame dst = env->NewVideoFrame(vi);
  if (dst == nullptr || dst.m_ptr == nullptr) {
      return nullptr;
  }

  const BYTE* srcP = src->GetReadPtr();
  const int awidth = std::min(src->GetPitch()>>1, (vi.width+7) & -8);

  BYTE* dstY = dst->GetWritePtr(PLANAR_Y);
  BYTE* dstU = dst->GetWritePtr(PLANAR_U);
  BYTE* dstV = dst->GetWritePtr(PLANAR_V);

  if ((env->GetCPUFlags() & CPUF_SSSE3) && IsPtrAligned(srcP, 16)) {
    convert_yuy2_to_yv16_xsse<CPUF_SSSE3>(srcP, dstY, dstU, dstV, src->GetPitch(), dst->GetPitch(PLANAR_Y), dst->GetPitch(PLANAR_U), vi.width, vi.height);
  } else if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(srcP, 16)) {
    convert_yuy2_to_yv16_xsse<CPUF_SSE2>(srcP, dstY, dstU, dstV, src->GetPitch(), dst->GetPitch(PLANAR_Y), dst->GetPitch(PLANAR_U), vi.width, vi.height);
  } 
  else
#ifdef _M_IX86
  if (env->GetCPUFlags() & CPUF_MMX) { 
    convert_yuy2_to_yv16_mmx(srcP, dstY, dstU, dstV, src->GetPitch(), dst->GetPitch(PLANAR_Y), dst->GetPitch(PLANAR_U), vi.width, vi.height);
  } else
#endif
  {
    convert_yuy2_to_yv16_c(srcP, dstY, dstU, dstV, src->GetPitch(), dst->GetPitch(PLANAR_Y), dst->GetPitch(PLANAR_U), vi.width, vi.height);
  }
  
  return dst;
}


AVSValue __cdecl ConvertYUY2ToYV16::Create(AVSValue args, void*, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();
  if (clip->GetVideoInfo().IsYV16())
    return clip;
  return new ConvertYUY2ToYV16(clip, env);
}

/************************************
 * YV16 to YUY2
 ************************************/

ConvertYV16ToYUY2::ConvertYV16ToYUY2(PClip src, IScriptEnvironment* env) : GenericVideoFilter(src,__FUNCTION__ ) {

  if (!vi.IsYV16())
    env->ThrowError("ConvertYV16ToYUY2: Only YV16 is allowed as input");

  vi.pixel_type = CS_YUY2;
}

void convert_yv16_to_yuy2_sse2(const BYTE *srcp_y, const BYTE *srcp_u, const BYTE *srcp_v, BYTE *dstp, size_t src_pitch_y, size_t src_pitch_uv, size_t dst_pitch, size_t width, size_t height)
{
  size_t half_width = width / 2;
  size_t mod8 = half_width / 8 * 8;

  for (size_t y=0; y<height; y++) { 
    for (size_t x=0; x<mod8; x+=8) {
      
      __m128i y = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp_y + x*2));
      __m128i u = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp_u + x));
      __m128i v = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp_v + x));

      __m128i uv = _mm_unpacklo_epi8(u, v);
      __m128i yuv_lo = _mm_unpacklo_epi8(y, uv);
      __m128i yuv_hi = _mm_unpackhi_epi8(y, uv);

      _mm_store_si128(reinterpret_cast<__m128i*>(dstp + x*4), yuv_lo);
      _mm_store_si128(reinterpret_cast<__m128i*>(dstp + x*4 + 16), yuv_hi);
    }

    for (size_t x=mod8; x<half_width; x++) {
      dstp[x*4+0] = srcp_y[x*2];
      dstp[x*4+1] = srcp_u[x];
      dstp[x*4+2] = srcp_y[x*2+1];
      dstp[x*4+3] = srcp_v[x];
    }

    srcp_y += src_pitch_y;
    srcp_u += src_pitch_uv;
    srcp_v += src_pitch_uv;
    dstp += dst_pitch;
  }
}

#ifdef _M_IX86
void convert_yv16_to_yuy2_mmx(const BYTE *srcp_y, const BYTE *srcp_u, const BYTE *srcp_v, BYTE *dstp, size_t src_pitch_y, size_t src_pitch_uv, size_t dst_pitch, size_t width, size_t height)
{
  size_t half_width = width / 2;
  size_t mod4 = half_width / 4 * 4;

  for (size_t y=0; y<height; y++) { 
    for (size_t x=0; x<mod4; x+=4) {
      __m64 y = *reinterpret_cast<const __m64*>(srcp_y + x*2);
      __m64 u = *reinterpret_cast<const __m64*>(srcp_u + x);
      __m64 v = *reinterpret_cast<const __m64*>(srcp_v + x);

      __m64 uv = _mm_unpacklo_pi8(u, v);
      __m64 yuv_lo = _mm_unpacklo_pi8(y, uv);
      __m64 yuv_hi = _mm_unpackhi_pi8(y, uv);

      *reinterpret_cast<__m64*>(dstp + x*4) = yuv_lo;
      *reinterpret_cast<__m64*>(dstp + x*4+8) = yuv_hi;
    }

    for (size_t x=mod4; x<half_width; x++) {
      dstp[x*4+0] = srcp_y[x*2];
      dstp[x*4+1] = srcp_u[x];
      dstp[x*4+2] = srcp_y[x*2+1];
      dstp[x*4+3] = srcp_v[x];
    }

    srcp_y += src_pitch_y;
    srcp_u += src_pitch_uv;
    srcp_v += src_pitch_uv;
    dstp += dst_pitch;
  }
  _mm_empty();
}
#endif

void convert_yv16_to_yuy2_c(const BYTE *srcp_y, const BYTE *srcp_u, const BYTE *srcp_v, BYTE *dstp, size_t src_pitch_y, size_t src_pitch_uv, size_t dst_pitch, size_t width, size_t height) {
  for (size_t y=0; y < height; y++) {
    for (size_t x=0; x < width / 2; x++) {
      dstp[x*4+0] = srcp_y[x*2];
      dstp[x*4+1] = srcp_u[x];
      dstp[x*4+2] = srcp_y[x*2+1];
      dstp[x*4+3] = srcp_v[x];
    }
    srcp_y += src_pitch_y;
    srcp_u += src_pitch_uv;
    srcp_v += src_pitch_uv;
    dstp += dst_pitch;
  }
}

PVideoFrame __stdcall ConvertYV16ToYUY2::GetFrame(int n, IScriptEnvironment* env) {
  PVideoFrame src = child->GetFrame(n, env);
  if (src == nullptr || src.m_ptr == nullptr) {
      return nullptr;
  }
  PVideoFrame dst = env->NewVideoFrame(vi, 16);
  if (dst == nullptr || dst.m_ptr == nullptr) {
      return nullptr;
  }

  const BYTE* srcY = src->GetReadPtr(PLANAR_Y);
  const BYTE* srcU = src->GetReadPtr(PLANAR_U);
  const BYTE* srcV = src->GetReadPtr(PLANAR_V);
  const int awidth = std::min(src->GetPitch(PLANAR_Y), (vi.width+7) & -8);

  BYTE* dstp = dst->GetWritePtr();

  if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(srcY, 16)) {
    //U and V don't have to be aligned since we user movq to read from those
    convert_yv16_to_yuy2_sse2(srcY, srcU, srcV, dstp, src->GetPitch(PLANAR_Y), src->GetPitch(PLANAR_U), dst->GetPitch(), awidth, vi.height);
  } else
#ifdef _M_IX86
  if (env->GetCPUFlags() & CPUF_MMX) { 
    convert_yv16_to_yuy2_mmx(srcY, srcU, srcV, dstp, src->GetPitch(PLANAR_Y), src->GetPitch(PLANAR_U), dst->GetPitch(), awidth, vi.height);
  } else
#endif
  {
    convert_yv16_to_yuy2_c(srcY, srcU, srcV, dstp, src->GetPitch(PLANAR_Y), src->GetPitch(PLANAR_U), dst->GetPitch(), awidth, vi.height);
  }
  
  return dst;
}


AVSValue __cdecl ConvertYV16ToYUY2::Create(AVSValue args, void*, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();
  if (clip->GetVideoInfo().IsYUY2())
    return clip;
  return new ConvertYV16ToYUY2(clip, env);
}

/**********************************************
 * Converter between arbitrary planar formats
 *
 * This uses plane copy for luma, and the
 * bicubic resizer for chroma (could be
 * customizable later)
 *
 * (c) Klaus Post, 2005
 * (c) Ian Brabham, 2011
 **********************************************/

inline float ChrOffset(bool point, int sIn, float dIn, int sOut, float dOut) {
      //     (1 - sOut/sIn)/2 + (dOut-dIn)/sIn; // Gavino Jan 2011
      return point ? (dOut-dIn)/sIn : 0.5f + (dOut-dIn - 0.5f*sOut)/sIn;
}

ConvertToPlanarGeneric::ConvertToPlanarGeneric(PClip src, int dst_space, bool interlaced,
                                               const AVSValue& InPlacement, const AVSValue& chromaResampler,
                                               const AVSValue& OutPlacement, IScriptEnvironment* env) : GenericVideoFilter(src,__FUNCTION__ ) {
  Y8input = vi.IsY8();

  if (!Y8input) {

    if (! (vi.IsYV12() || dst_space == CS_YV12))
      interlaced = false;  // Ignore, if YV12 is not involved.

    // Describe input pixel positioning
    float xdInU = 0.0f, txdInU = 0.0f, bxdInU = 0.0f;
    float ydInU = 0.0f, tydInU = 0.0f, bydInU = 0.0f;
    float xdInV = 0.0f, txdInV = 0.0f, bxdInV = 0.0f;
    float ydInV = 0.0f, tydInV = 0.0f, bydInV = 0.0f;

    if (vi.IsYV12()) {
      switch (getPlacement(InPlacement, env)) {
        case PLACEMENT_DV:
          ydInU = 0.0f, tydInU = 0.0f, bydInU = 0.5f;
          ydInV = 1.0f, tydInV = 0.5f, bydInV = 1.0f;
          break;
        case PLACEMENT_MPEG1:
          xdInU = 0.5f, txdInU = 0.5f, bxdInU = 0.5f;
          xdInV = 0.5f, txdInV = 0.5f, bxdInV = 0.5f;
          // fall thru
        case PLACEMENT_MPEG2:
          ydInU = 0.5f, tydInU = 0.25f, bydInU = 0.75f;
          ydInV = 0.5f, tydInV = 0.25f, bydInV = 0.75f;
          break;
      }
    }
    else if (InPlacement.Defined())
      env->ThrowError("Convert: Input ChromaPlacement only available with YV12 source.");

    const int xsIn = 1 << vi.GetPlaneWidthSubsampling(PLANAR_U);
    const int ysIn = 1 << vi.GetPlaneHeightSubsampling(PLANAR_U);

    vi.pixel_type = dst_space;

    // Describe output pixel positioning
    float xdOutU = 0.0f, txdOutU = 0.0f, bxdOutU = 0.0f;
    float ydOutU = 0.0f, tydOutU = 0.0f, bydOutU = 0.0f;
    float xdOutV = 0.0f, txdOutV = 0.0f, bxdOutV = 0.0f;
    float ydOutV = 0.0f, tydOutV = 0.0f, bydOutV = 0.0f;

    if (vi.IsYV12()) {
      switch (getPlacement(OutPlacement, env)) {
        case PLACEMENT_DV:
          ydOutU = 0.0f, tydOutU = 0.0f, bydOutU = 0.5f;
          ydOutV = 1.0f, tydOutV = 0.5f, bydOutV = 1.0f;
          break;
        case PLACEMENT_MPEG1:
          xdOutU = 0.5f, txdOutU = 0.5f, bxdOutU = 0.5f;
          xdOutV = 0.5f, txdOutV = 0.5f, bxdOutV = 0.5f;
          // fall thru
        case PLACEMENT_MPEG2:
          ydOutU = 0.5f, tydOutU = 0.25f, bydOutU = 0.75f;
          ydOutV = 0.5f, tydOutV = 0.25f, bydOutV = 0.75f;
          break;
      }
    }
    else if (OutPlacement.Defined())
      env->ThrowError("Convert: Output ChromaPlacement only available with YV12 output.");

    const int xsOut = 1 << vi.GetPlaneWidthSubsampling(PLANAR_U);
    const int xmask = xsOut - 1;
    if (vi.width & xmask)
      env->ThrowError("Convert: Cannot convert if width isn't mod%d!", xsOut);

    const int ysOut = 1 << vi.GetPlaneHeightSubsampling(PLANAR_U);
    const int ymask = ysOut - 1;
    if (vi.height & ymask)
      env->ThrowError("Convert: Cannot convert if height isn't mod%d!", ysOut);

    int uv_width  = vi.width  >> vi.GetPlaneWidthSubsampling(PLANAR_U);
    int uv_height = vi.height >> vi.GetPlaneHeightSubsampling(PLANAR_U);


    bool P = !lstrcmpiA(chromaResampler.AsString(""), "point");

    if (interlaced) {
      uv_height /=  2;

      AVSValue tUsubSampling[4] = { ChrOffset(P, xsIn, txdInU, xsOut, txdOutU), ChrOffset(P, ysIn, tydInU, ysOut, tydOutU), AVSValue(), AVSValue() };
      AVSValue bUsubSampling[4] = { ChrOffset(P, xsIn, bxdInU, xsOut, bxdOutU), ChrOffset(P, ysIn, bydInU, ysOut, bydOutU), AVSValue(), AVSValue() };
      AVSValue tVsubSampling[4] = { ChrOffset(P, xsIn, txdInV, xsOut, txdOutV), ChrOffset(P, ysIn, tydInV, ysOut, tydOutV), AVSValue(), AVSValue() };
      AVSValue bVsubSampling[4] = { ChrOffset(P, xsIn, bxdInV, xsOut, bxdOutV), ChrOffset(P, ysIn, bydInV, ysOut, bydOutV), AVSValue(), AVSValue() };

      Usource = new SeparateFields(new AssumeParity(new SwapUVToY(child, SwapUVToY::UToY8, env), true), env);
      Vsource = new SeparateFields(new AssumeParity(new SwapUVToY(child, SwapUVToY::VToY8, env), true), env);

      PClip *tbUsource = new PClip[2]; // Interleave()::~Interleave() will delete these
      PClip *tbVsource = new PClip[2];

      tbUsource[0] = FilteredResize::CreateResize(new SelectEvery(Usource, 2, 0), uv_width, uv_height,  env);
      tbUsource[1] = FilteredResize::CreateResize(new SelectEvery(Usource, 2, 1), uv_width, uv_height, env);
      tbVsource[0] = FilteredResize::CreateResize(new SelectEvery(Vsource, 2, 0), uv_width, uv_height, env);
      tbVsource[1] = FilteredResize::CreateResize(new SelectEvery(Vsource, 2, 1), uv_width, uv_height, env);

      Usource = new SelectEvery(new DoubleWeaveFields(new Interleave(2, tbUsource, env)), 2, 0);
      Vsource = new SelectEvery(new DoubleWeaveFields(new Interleave(2, tbVsource, env)), 2, 0);
    }
    else {
      AVSValue UsubSampling[4] = { ChrOffset(P, xsIn, xdInU, xsOut, xdOutU), ChrOffset(P, ysIn, ydInU, ysOut, ydOutU), AVSValue(), AVSValue() };
      AVSValue VsubSampling[4] = { ChrOffset(P, xsIn, xdInV, xsOut, xdOutV), ChrOffset(P, ysIn, ydInV, ysOut, ydOutV), AVSValue(), AVSValue() };

      Usource = FilteredResize::CreateResize(new SwapUVToY(child, SwapUVToY::UToY8, env), uv_width, uv_height,env);
      Vsource = FilteredResize::CreateResize(new SwapUVToY(child, SwapUVToY::VToY8, env), uv_width, uv_height, env);
    }
  }
  else {
    vi.pixel_type = dst_space;
  }
}

PVideoFrame __stdcall ConvertToPlanarGeneric::GetFrame(int n, IScriptEnvironment* env) {
  PVideoFrame src = child->GetFrame(n, env);
  if (src == nullptr || src.m_ptr == nullptr) {
      return nullptr;
  }
  PVideoFrame dst = env->NewVideoFrame(vi);
  if (dst == nullptr || dst.m_ptr == nullptr) {
      return nullptr;
  }

  env->BitBlt(dst->GetWritePtr(PLANAR_Y), dst->GetPitch(PLANAR_Y), src->GetReadPtr(PLANAR_Y), src->GetPitch(PLANAR_Y),
              src->GetRowSize(PLANAR_Y_ALIGNED), src->GetHeight(PLANAR_Y));
  if (Y8input) {
    memset(dst->GetWritePtr(PLANAR_U), 0x80, dst->GetHeight(PLANAR_U)*dst->GetPitch(PLANAR_U));
    memset(dst->GetWritePtr(PLANAR_V), 0x80, dst->GetHeight(PLANAR_V)*dst->GetPitch(PLANAR_V));
  } else {
    src = Usource->GetFrame(n, env);
    env->BitBlt(dst->GetWritePtr(PLANAR_U), dst->GetPitch(PLANAR_U), src->GetReadPtr(PLANAR_Y), src->GetPitch(PLANAR_Y),
                src->GetRowSize(PLANAR_Y_ALIGNED), dst->GetHeight(PLANAR_U));
    src = Vsource->GetFrame(n, env);
    env->BitBlt(dst->GetWritePtr(PLANAR_V), dst->GetPitch(PLANAR_V), src->GetReadPtr(PLANAR_Y), src->GetPitch(PLANAR_Y),
                src->GetRowSize(PLANAR_Y_ALIGNED), dst->GetHeight(PLANAR_V));
  }
  return dst;
}

AVSValue __cdecl ConvertToPlanarGeneric::CreateYV12(AVSValue args, void*, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();

  if (clip->GetVideoInfo().IsYV12()) {
    if (getPlacement(args[3], env) == getPlacement(args[5], env))
      return clip;
  }
  else if (clip->GetVideoInfo().IsRGB())
    clip = new ConvertRGBToYV24(clip, getMatrix(args[2].AsString(0), env), env);
  else if (clip->GetVideoInfo().IsYUY2())
    clip = new ConvertYUY2ToYV16(clip,  env);

  if (!clip->GetVideoInfo().IsPlanar())
    env->ThrowError("ConvertToYV12: Can only convert from Planar YUV.");

  // ConvertToPlanarGeneric will invoke 3 chains upon clip, cache it!
  clip = env->Invoke("Cache", AVSValue(clip)).AsClip();
  return new ConvertToPlanarGeneric(clip, CS_YV12, args[1].AsBool(false), args[3], args[4], args[5], env);
}


AVSValue __cdecl ConvertToPlanarGeneric::CreateYV16(AVSValue args, void*, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();

  if (clip->GetVideoInfo().IsYV16())
    return clip;

  if (clip->GetVideoInfo().IsYUY2())
    return new ConvertYUY2ToYV16(clip,  env);

  if (clip->GetVideoInfo().IsRGB())
    clip = new ConvertRGBToYV24(clip, getMatrix(args[2].AsString(0), env), env);

  if (!clip->GetVideoInfo().IsPlanar())
    env->ThrowError("ConvertToYV16: Can only convert from Planar YUV.");

   //ConvertToPlanarGeneric will invoke 3 chains upon clip, cache it!
  clip = env->Invoke("Cache", AVSValue(clip)).AsClip();
  return new ConvertToPlanarGeneric(clip, CS_YV16, args[1].AsBool(false), args[3], args[4], AVSValue(), env);
}

AVSValue __cdecl ConvertToPlanarGeneric::CreateYV24(AVSValue args, void*, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();

  if (clip->GetVideoInfo().IsYV24() )
    return clip;

  if (clip->GetVideoInfo().IsRGB())
    return new ConvertRGBToYV24(clip, getMatrix(args[2].AsString(0), env), env);

  if (clip->GetVideoInfo().IsYUY2())
    clip = new ConvertYUY2ToYV16(clip,  env);

  if (!clip->GetVideoInfo().IsPlanar())
    env->ThrowError("ConvertToYV24: Can only convert from Planar YUV.");

  // ConvertToPlanarGeneric will invoke 3 chains upon clip, cache it!
  clip = env->Invoke("Cache", AVSValue(clip)).AsClip();
  return new ConvertToPlanarGeneric(clip, CS_YV24, args[1].AsBool(false), args[3], args[4], AVSValue(), env);
}

static int getPlacement(const AVSValue& _placement, IScriptEnvironment* env) {
  const char* placement = _placement.AsString(0);

  if (placement) {
    if (!lstrcmpiA(placement, "mpeg2"))
      return PLACEMENT_MPEG2;

    if (!lstrcmpiA(placement, "mpeg1"))
      return PLACEMENT_MPEG1;

    if (!lstrcmpiA(placement, "dv"))
      return PLACEMENT_DV;

    env->ThrowError("Convert: Unknown chromaplacement");
  }
  return PLACEMENT_MPEG2;
}
