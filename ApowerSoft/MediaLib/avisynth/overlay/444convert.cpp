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

// Overlay (c) 2003, 2004 by Klaus Post

#include "avisynth/avisynth_stdafx.h"

#include "444convert.h"
#include <libyuv.h>
void Convert444FromYV12::ConvertImage(PVideoFrame src, Image444* dst, IScriptEnvironment* env) {
  env->BitBlt(dst->GetPtr(PLANAR_Y), dst->pitch,
    src->GetReadPtr(PLANAR_Y),src->GetPitch(PLANAR_Y), src->GetRowSize(PLANAR_Y), src->GetHeight());
  const BYTE* srcY = src->GetReadPtr(PLANAR_U);
  const BYTE* srcU = src->GetReadPtr(PLANAR_U);
  const BYTE* srcV = src->GetReadPtr(PLANAR_V);
  int srcYpitch = src->GetPitch(PLANAR_Y);
  int srcUpitch = src->GetPitch(PLANAR_U);
  int srcVpitch = src->GetPitch(PLANAR_V);

  BYTE* dstY = dst->GetPtr(PLANAR_Y);
  BYTE* dstU = dst->GetPtr(PLANAR_U);
  BYTE* dstV = dst->GetPtr(PLANAR_V);

  int dstPitch = dst->pitch;

  int w = ((src->GetRowSize(PLANAR_U)+7)/8)*8;
  int h = src->GetHeight(PLANAR_U);

  //ConvertYV12ChromaTo444(dstU, srcU, dstUVpitch, srcUVpitch, w, h);
  //ConvertYV12ChromaTo444(dstV, srcV, dstUVpitch, srcUVpitch, w, h);
  libyuv::I420ToI444(
      srcY, srcYpitch,
      srcU, srcUpitch,
      srcV, srcVpitch,
      dstY, dstPitch,
      dstU, dstPitch,
      dstV, dstPitch,
      w,h
  );
}


void Convert444FromYV12::ConvertImageLumaOnly(PVideoFrame src, Image444* dst, IScriptEnvironment* env) {
  env->BitBlt(dst->GetPtr(PLANAR_Y), dst->pitch,
    src->GetReadPtr(PLANAR_Y),src->GetPitch(PLANAR_Y), src->GetRowSize(PLANAR_Y), src->GetHeight());
}

void Convert444FromYV24::ConvertImage(PVideoFrame src, Image444* dst, IScriptEnvironment* env) {
  env->BitBlt(dst->GetPtr(PLANAR_Y), dst->pitch,
    src->GetReadPtr(PLANAR_Y),src->GetPitch(PLANAR_Y), src->GetRowSize(PLANAR_Y), src->GetHeight(PLANAR_Y));
  env->BitBlt(dst->GetPtr(PLANAR_U), dst->pitch,
    src->GetReadPtr(PLANAR_U),src->GetPitch(PLANAR_U), src->GetRowSize(PLANAR_U), src->GetHeight(PLANAR_U));
  env->BitBlt(dst->GetPtr(PLANAR_V), dst->pitch,
    src->GetReadPtr(PLANAR_V),src->GetPitch(PLANAR_V), src->GetRowSize(PLANAR_V), src->GetHeight(PLANAR_V));
}

void Convert444FromYV24::ConvertImageLumaOnly(PVideoFrame src, Image444* dst, IScriptEnvironment* env) {
  env->BitBlt(dst->GetPtr(PLANAR_Y), dst->pitch,
    src->GetReadPtr(PLANAR_Y),src->GetPitch(PLANAR_Y), src->GetRowSize(PLANAR_Y), src->GetHeight(PLANAR_Y));
}

void Convert444FromY8::ConvertImage(PVideoFrame src, Image444* dst, IScriptEnvironment* env) {
  env->BitBlt(dst->GetPtr(PLANAR_Y), dst->pitch,
    src->GetReadPtr(PLANAR_Y),src->GetPitch(PLANAR_Y), src->GetRowSize(PLANAR_Y), src->GetHeight(PLANAR_Y));
  memset((void *)dst->GetPtr(PLANAR_U), (char)0x80, dst->pitch * dst->h());
  memset((void *)dst->GetPtr(PLANAR_V), (char)0x80, dst->pitch * dst->h());
}

void Convert444FromY8::ConvertImageLumaOnly(PVideoFrame src, Image444* dst, IScriptEnvironment* env) {
  env->BitBlt(dst->GetPtr(PLANAR_Y), dst->pitch,
    src->GetReadPtr(PLANAR_Y),src->GetPitch(PLANAR_Y), src->GetRowSize(PLANAR_Y), src->GetHeight(PLANAR_Y));
}

/***** YUY2 -> YUV 4:4:4   ******/


void Convert444FromYUY2::ConvertImage(PVideoFrame src, Image444* dst, IScriptEnvironment* env) {

  const BYTE* srcP = src->GetReadPtr();
  int srcPitch = src->GetPitch();

  BYTE* dstY = dst->GetPtr(PLANAR_Y);
  BYTE* dstU = dst->GetPtr(PLANAR_U);
  BYTE* dstV = dst->GetPtr(PLANAR_V);

  int dstPitch = dst->pitch;

  int w = dst->w();
  int h = dst->h();

  for (int y=0; y<h; y++) {
    for (int x=0; x<w; x+=2) {
      int x2 = x<<1;
      dstY[x]   = srcP[x2];
      dstU[x]   = dstU[x+1] = srcP[x2+1];
      dstV[x]   = dstV[x+1] = srcP[x2+3];
      dstY[x+1] = srcP[x2+2];
    }
    srcP+=srcPitch;

    dstY+=dstPitch;
    dstU+=dstPitch;
    dstV+=dstPitch;
  }
}


void Convert444FromYUY2::ConvertImageLumaOnly(PVideoFrame src, Image444* dst, IScriptEnvironment* env) {

  const BYTE* srcP = src->GetReadPtr();
  int srcPitch = src->GetPitch();

  BYTE* dstY = dst->GetPtr(PLANAR_Y);

  int dstPitch = dst->pitch;

  int w = dst->w();
  int h = dst->h();

  for (int y=0; y<h; y++) {
    for (int x=0; x<w; x+=2) {
      int x2 = x<<1;
      dstY[x]   = srcP[x2];
      dstY[x+1] = srcP[x2+2];
    }
    srcP+=srcPitch;
    dstY+=dstPitch;
  }
}

/****** YUV 4:4:4 -> YUV 4:4:4   - Perhaps the easiest job in the world ;)  *****/
PVideoFrame Convert444ToYV24::ConvertImage(Image444* src, PVideoFrame dst, IScriptEnvironment* env) {
  env->MakeWritable(&dst);

  env->BitBlt(dst->GetWritePtr(PLANAR_Y), dst->GetPitch(PLANAR_Y),
    src->GetPtr(PLANAR_Y), src->pitch, dst->GetRowSize(PLANAR_Y), dst->GetHeight(PLANAR_Y));
  env->BitBlt(dst->GetWritePtr(PLANAR_U), dst->GetPitch(PLANAR_U),
    src->GetPtr(PLANAR_U), src->pitch, dst->GetRowSize(PLANAR_U), dst->GetHeight(PLANAR_U));
  env->BitBlt(dst->GetWritePtr(PLANAR_V), dst->GetPitch(PLANAR_V),
    src->GetPtr(PLANAR_V), src->pitch, dst->GetRowSize(PLANAR_V), dst->GetHeight(PLANAR_V));
  return dst;
}

PVideoFrame Convert444ToY8::ConvertImage(Image444* src, PVideoFrame dst, IScriptEnvironment* env) {
  env->MakeWritable(&dst);

  env->BitBlt(dst->GetWritePtr(PLANAR_Y), dst->GetPitch(PLANAR_Y),
    src->GetPtr(PLANAR_Y), src->pitch, dst->GetRowSize(PLANAR_Y), dst->GetHeight());
  return dst;
}


//I422ToI420
PVideoFrame Convert444ToYV12::ConvertImage(Image444* src, PVideoFrame dst, IScriptEnvironment* env) {
  env->MakeWritable(&dst);

  const BYTE* srcY = src->GetPtr(PLANAR_Y);
  const BYTE* srcU = src->GetPtr(PLANAR_U);
  const BYTE* srcV = src->GetPtr(PLANAR_V);
  int srcPitch = src->pitch;

  BYTE* dstY = dst->GetWritePtr(PLANAR_Y);
  BYTE* dstU = dst->GetWritePtr(PLANAR_U);
  BYTE* dstV = dst->GetWritePtr(PLANAR_V);

  int dstYPitch = dst->GetPitch(PLANAR_Y);
  int dstUPitch = dst->GetPitch(PLANAR_U);
  int dstVPitch = dst->GetPitch(PLANAR_V);

  int w = ((dst->GetRowSize(PLANAR_U)+7)/8)*8;
  int h = dst->GetHeight(PLANAR_U);

  libyuv::I444ToI420(
      srcY, srcPitch,
      srcU, srcPitch,
      srcV, srcPitch,
      dstY, dstYPitch,
      dstU, dstUPitch,
      dstV, dstVPitch,
      w, h
  );

  return dst;
}

/*****   YUV 4:4:4 -> YUY2   *******/

PVideoFrame Convert444ToYUY2::ConvertImage(Image444* src, PVideoFrame dst, IScriptEnvironment* env) {
  env->MakeWritable(&dst);

  const BYTE* srcY = src->GetPtr(PLANAR_Y);
  const BYTE* srcU = src->GetPtr(PLANAR_U);
  const BYTE* srcV = src->GetPtr(PLANAR_V);

  int srcPitch = src->pitch;

  BYTE* dstP = dst->GetWritePtr();

  int dstPitch = dst->GetPitch();

  int w = src->w();
  int h = src->h();

  for (int y=0; y<h; y++) {
    for (int x=0; x<w; x+=2) {
      int x2 = x<<1;
      dstP[x2]   = srcY[x];
      dstP[x2+1] = (srcU[x] + srcU[x+1] + 1)>>1;
      dstP[x2+2] = srcY[x+1];
      dstP[x2+3] = (srcV[x] + srcV[x+1] + 1)>>1;
    }
    srcY+=srcPitch;
    srcU+=srcPitch;
    srcV+=srcPitch;
    dstP+=dstPitch;
  }
  return dst;
}

/*****   YUV 4:4:4 -> RGB24/32   *******/

#define Kr 0.299
#define Kg 0.587
#define Kb 0.114

PVideoFrame Convert444ToRGB::ConvertImage(Image444* src, PVideoFrame dst, IScriptEnvironment* env) {
  const int crv = int(2*(1-Kr)       * 255.0/224.0 * 65536+0.5);
  const int cgv = int(2*(1-Kr)*Kr/Kg * 255.0/224.0 * 65536+0.5);
  const int cgu = int(2*(1-Kb)*Kb/Kg * 255.0/224.0 * 65536+0.5);
  const int cbu = int(2*(1-Kb)       * 255.0/224.0 * 65536+0.5);

  env->MakeWritable(&dst);

  const BYTE* srcY = src->GetPtr(PLANAR_Y);
  const BYTE* srcU = src->GetPtr(PLANAR_U);
  const BYTE* srcV = src->GetPtr(PLANAR_V);

  int srcPitch = src->pitch;

  BYTE* dstP = dst->GetWritePtr();
  int dstPitch = dst->GetPitch();

  int w = src->w();
  int h = src->h();

  dstP += h*dstPitch-dstPitch;
  int bpp = dst->GetRowSize()/w;

  for (int y=0; y<h; y++) {
    int xRGB = 0;
    for (int x=0; x<w; x++) {
      const int Y = (srcY[x] -  16) * int(255.0/219.0*65536+0.5);
      const int U =  srcU[x] - 128;
      const int V =  srcV[x] - 128;

      dstP[xRGB+0] = ScaledPixelClip(Y + U * cbu);
      dstP[xRGB+1] = ScaledPixelClip(Y - U * cgu - V * cgv);
      dstP[xRGB+2] = ScaledPixelClip(Y           + V * crv);
      xRGB += bpp;
    }
    srcY+=srcPitch;
    srcU+=srcPitch;
    srcV+=srcPitch;
    dstP-=dstPitch;
  }
  return dst;
}

PVideoFrame Convert444NonCCIRToRGB::ConvertImage(Image444* src, PVideoFrame dst, IScriptEnvironment* env) {
  const int crv = int(2*(1-Kr)       * 65536+0.5);
  const int cgv = int(2*(1-Kr)*Kr/Kg * 65536+0.5);
  const int cgu = int(2*(1-Kb)*Kb/Kg * 65536+0.5);
  const int cbu = int(2*(1-Kb)       * 65536+0.5);

  env->MakeWritable(&dst);

  const BYTE* srcY = src->GetPtr(PLANAR_Y);
  const BYTE* srcU = src->GetPtr(PLANAR_U);
  const BYTE* srcV = src->GetPtr(PLANAR_V);

  int srcPitch = src->pitch;

  BYTE* dstP = dst->GetWritePtr();
  int dstPitch = dst->GetPitch();

  int w = src->w();
  int h = src->h();

  dstP += h*dstPitch-dstPitch;
  int bpp = dst->GetRowSize()/w;

  for (int y=0; y<h; y++) {
    int xRGB = 0;
    for (int x=0; x<w; x++) {
      const int Y = srcY[x]*65536;
      const int U = srcU[x] - 128;
      const int V = srcV[x] - 128;

      dstP[xRGB+0] = ScaledPixelClip(Y + U * cbu);
      dstP[xRGB+1] = ScaledPixelClip(Y - U * cgu - V * cgv);
      dstP[xRGB+2] = ScaledPixelClip(Y           + V * crv);
      xRGB += bpp;
    }
    srcY+=srcPitch;
    srcU+=srcPitch;
    srcV+=srcPitch;
    dstP-=dstPitch;
  }
  return dst;
}


/******* RGB 24/32 -> YUV444   *******/

void Convert444FromRGB::ConvertImageLumaOnly(PVideoFrame src, Image444* dst, IScriptEnvironment* env) {

  const BYTE* srcP = src->GetReadPtr();
  int srcPitch = src->GetPitch();

  BYTE* dstY = dst->GetPtr(PLANAR_Y);

  int dstPitch = dst->pitch;

  int w = dst->w();
  int h = dst->h();

  int bpp = src->GetRowSize()/w;
  srcP += h*srcPitch-srcPitch;

  for (int y=0; y<h; y++) {
    int RGBx = 0;
    for (int x=0; x<w; x++) {
      dstY[x] = srcP[RGBx]; // Blue channel only ???
      RGBx+=bpp;
    }
    srcP-=srcPitch;
    dstY+=dstPitch;
  }
}

void Convert444FromRGB::ConvertImage(PVideoFrame src, Image444* dst, IScriptEnvironment* env) {
  const int cyr = int(Kr * 219/255 * 65536 + 0.5);
  const int cyg = int(Kg * 219/255 * 65536 + 0.5);
  const int cyb = int(Kb * 219/255 * 65536 + 0.5);

  const int kv = int(32768 / (2*(1-Kr) * 255.0/224.0) + 0.5); // 20531
  const int ku = int(32768 / (2*(1-Kb) * 255.0/224.0) + 0.5); // 16244

  const BYTE* srcP = src->GetReadPtr();
  int srcPitch = src->GetPitch();

  BYTE* dstY = dst->GetPtr(PLANAR_Y);
  BYTE* dstU = dst->GetPtr(PLANAR_U);
  BYTE* dstV = dst->GetPtr(PLANAR_V);

  const int dstPitch = dst->pitch;

  const int w = dst->w();
  const int h = dst->h();

  const int bpp = src->GetRowSize()/w;
  srcP += h*srcPitch-srcPitch;

  for (int y=0; y<h; y++) {
    int RGBx = 0;
    for (int x=0; x<w; x++) {
      const int b = srcP[RGBx+0];
      const int g = srcP[RGBx+1];
      const int r = srcP[RGBx+2];

      const int y = (cyb*b + cyg*g + cyr*r + 0x108000) >> 16; // 0x108000 = 16.5 * 65536
      const int scaled_y = (y - 16) * int(255.0/219.0*65536+0.5);
      const int b_y = (b << 16) - scaled_y;
      const int r_y = (r << 16) - scaled_y;

      dstY[x] = y;
      dstU[x] = ((b_y>>11) * ku + 0x8080000)>>20; // 0x8080000 = 128.5 << 20
      dstV[x] = ((r_y>>11) * kv + 0x8080000)>>20;

      RGBx += bpp;
    }

    srcP-=srcPitch;

    dstY+=dstPitch;
    dstU+=dstPitch;
    dstV+=dstPitch;
  }
}

void Convert444NonCCIRFromRGB::ConvertImage(PVideoFrame src, Image444* dst, IScriptEnvironment* env) {
  const int cyb = int(Kb * 65536 + 0.5);
  const int cyg = int(Kg * 65536 + 0.5);
  const int cyr = int(Kr * 65536 + 0.5);

  const int kv = int(65536 / (2*(1-Kr)) + 0.5);
  const int ku = int(65536 / (2*(1-Kb)) + 0.5);

  const BYTE* srcP = src->GetReadPtr();
  const int srcPitch = src->GetPitch();

  BYTE* dstY = dst->GetPtr(PLANAR_Y);
  BYTE* dstU = dst->GetPtr(PLANAR_U);
  BYTE* dstV = dst->GetPtr(PLANAR_V);

  const int dstPitch = dst->pitch;

  const int w = dst->w();
  const int h = dst->h();

  const int bpp = src->GetRowSize()/w;
  srcP += h*srcPitch-srcPitch;

  for (int y=0; y<h; y++) {
    int RGBx = 0;
    for (int x=0; x<w; x++) {
      const int b = srcP[RGBx+0];
      const int g = srcP[RGBx+1];
      const int r = srcP[RGBx+2];

      const int y = (cyb*b + cyg*g + cyr*r + 0x8000) >> 16; // 0x8000 = 0.5 * 65536

      dstY[x] = y;
      dstU[x] = ((b - y) * ku + 0x808000)>>16; // 0x808000 = 128.5 * 65536
      dstV[x] = ((r - y) * kv + 0x808000)>>16;

      RGBx+=bpp;
    }
    srcP-=srcPitch;

    dstY+=dstPitch;
    dstU+=dstPitch;
    dstV+=dstPitch;
  }
}

