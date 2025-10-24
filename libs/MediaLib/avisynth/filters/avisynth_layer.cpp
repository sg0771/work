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



// Avisynth filter: Layer
// by "poptones" (poptones@myrealbox.com)

#include "avisynth/avisynth_stdafx.h"

#include "layer.h"
#include "resample.h"

int yuvij[256] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,1,2,3,5,6,7,8,9,10,12,13,14,15,16,17,
	19,20,21,22,23,24,26,27,28,29,30,31,33,34,35,36,
	37,38,40,41,42,43,44,45,47,48,49,50,51,52,54,55,
	56,57,58,59,61,62,63,64,65,66,67,69,70,71,72,73,
	74,76,77,78,79,80,81,83,84,85,86,87,88,90,91,92,
	93,94,95,97,98,99,100,101,102,104,105,106,107,108,109,111,
	112,113,114,115,116,118,119,120,121,122,123,125,126,127,128,129,
	130,131,133,134,135,136,137,138,140,141,142,143,144,145,147,148,
	149,150,151,152,154,155,156,157,158,159,161,162,163,164,165,166,
	168,169,170,171,172,173,175,176,177,178,179,180,182,183,184,185,
	186,187,189,190,191,192,193,194,195,197,198,199,200,201,202,204,
	205,206,207,208,209,211,212,213,214,215,216,218,219,220,221,222,
	223,225,226,227,228,229,230,232,233,234,235,236,237,239,240,241,
	242,243,244,246,247,248,249,250,251,253,254,255,
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255 };

/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

extern const AVSFunction Layer_filters[] = {
  { "Mask", "cc", Mask::Create },     // clip, mask
  { "ColorKeyMask", "ci[]i[]i[]i", ColorKeyMask::Create },    // clip, color, tolerance[B, toleranceG, toleranceR]
  { "ResetMask", "c", ResetMask::Create },
  { "Invert", "c[channels]s", Invert::Create },
  { "ShowAlpha", "c[pixel_type]s", ShowChannel::Create, (void*)3 },
  { "ShowRed", "c[pixel_type]s", ShowChannel::Create, (void*)2 },
  { "ShowGreen", "c[pixel_type]s", ShowChannel::Create, (void*)1 },
  { "ShowBlue", "c[pixel_type]s", ShowChannel::Create, (void*)0 },
  { "MergeRGB",  "ccc[pixel_type]s", MergeRGB::Create, (void*)0 },
  { "MergeARGB", "cccc",             MergeRGB::Create, (void*)1 },
  { "Layer", "cc[op]s[level]i[x]i[y]i[threshold]i[use_chroma]b", Layer::Create },
  { "layer", "cc[op]s[level]i[x]i[y]i[threshold]i[use_chroma]b", Layer::Create },

{ "AutoLayer", "cciiii", AutoLayer::TinyCreate},

  { "Subtract", "cc", Subtract::Create },
  { 0,0,0 }
};





/******************************
 *******   Mask Filter   ******
 ******************************/

Mask::Mask(PClip _child1, PClip _child2, IScriptEnvironment* env)
  : IClip(__FUNCTION__), child1(_child1), child2(_child2)
{
  const VideoInfo& vi1 = child1->GetVideoInfo();
  const VideoInfo& vi2 = child2->GetVideoInfo();
  if (vi1.width != vi2.width || vi1.height != vi2.height)
    env->ThrowError("Mask error: image dimensions don't match");
 /* if (!vi1.IsRGB32() | !vi2.IsRGB32())
    env->ThrowError("Mask error: sources must be RGB32");*/

  vi = vi1;
  mask_frames = vi2.num_frames;
}

PVideoFrame __stdcall Mask::GetFrame(int n, IScriptEnvironment* env)
{
	
  PVideoFrame src1 = child1->GetFrame(n, env);

  if (src1 == nullptr || src1.m_ptr == nullptr) {
      return nullptr;
  }
  PVideoFrame src2 = child2->GetFrame(std::min(n,mask_frames-1), env);

  if (src2 == nullptr || src2.m_ptr == nullptr) {
      return nullptr;
  }
  
  env->MakeWritable(&src1);
  env->MakeWritable(&src2);

	BYTE* src1p = src1->GetWritePtr();
	const BYTE* src2pi = src2->GetReadPtr();
	 BYTE* src2p = src2->GetWritePtr();
	 const VideoInfo vi2 = child2->GetVideoInfo();
	const int src1_pitch = src1->GetPitch();
	const int src2_pitch = src2->GetPitch(PLANAR_Y);

	 for (size_t i = 0; i < vi2.height; i++)
	 {
		 for (size_t j = 0; j < vi2.width; j++)
		 {
			 src2p[j+ i* src2_pitch] = yuvij[src2pi[j+i* src2_pitch]];
		 }
	 }
	



	const int myx = vi.width;
	const int myy = vi.height;
	if (vi2.IsYV12() || vi2.IsYV16() || vi2.IsYV24())
	{

		libyuv::ARGBCopyYToAlpha(src2p, src2_pitch, src1p, src1_pitch, vi.width, -vi.height);
		return src1;
	}


	const int cyb = int(0.114*32768+0.5);
	const int cyg = int(0.587*32768+0.5);
	const int cyr = int(0.299*32768+0.5);

	__declspec(align(8)) static const __int64 rgb2lum = ((__int64)cyr << 32) | (cyg << 16) | cyb;

	static const int alpha_mask=0x00ffffff;
	static const int color_mask=0xff000000;
	static const int rounder   =16384;
#ifdef _M_X64
  for (int y=0; y<vi.height; ++y) {
	  for (int x=0; x<vi.width; ++x)
		  src1p[x*4+3] = (cyb*src2p[x*4+0] + cyg*src2p[x*4+1] +
                    cyr*src2p[x*4+2] + 16384) >> 15;

    src1p += src1_pitch;
    src2p += src2_pitch;
  }
#else
		__asm {
		mov			edi, src1p
		mov			esi, src2p
		mov			eax, myy
		movq		mm1, rgb2lum
		movd		mm2, alpha_mask
		movd		mm3, color_mask
		movd		mm7, rounder
		punpckldq	mm2, mm2
		punpckldq	mm3, mm3
		punpckldq	mm7, mm7
		xor			ecx, ecx
		pxor		mm0, mm0
		mov			edx, myx
		align		16
mask_mmxloop:
			movd		mm6, [esi + ecx*4]	; pipeline in next mask pixel RGB
			 movd		mm4, [edi + ecx*4]	;get color RGBA
			punpcklbw	mm6, mm0			;mm6= 00aa|00rr|00gg|00bb [src2]
			pmaddwd		mm6, mm1			;partial monochrome result
			punpckldq	mm5, mm6			;ready to add
			 paddd		mm6, mm7			;rounding
			paddd		mm6, mm5			;32 bit result in high top dword
			psrlq		mm6, 15+8			;8 bit result
			 pand		mm4, mm2			;strip out old alpha
			pand		mm6, mm3			;clear any possible junk
			 inc		ecx					;point to next - aka loop counter
			por			mm6, mm4			;merge new alpha and original color
			 cmp		ecx, edx
			movd		[edi+ecx*4-4],mm6	;store'em where they belong (at ecx-1)
			 jnz		mask_mmxloop

		add		edi, src1_pitch
		add		esi, src2_pitch
		xor		ecx, ecx
		dec		eax
		jnz		mask_mmxloop
		emms
		}
#endif
 return src1;
}

AVSValue __cdecl Mask::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  return new Mask(args[0].AsClip(), args[1].AsClip(), env);
}









/**************************************
 *******   ColorKeyMask Filter   ******
 **************************************/


ColorKeyMask::ColorKeyMask(PClip _child, int _color, int _tolB, int _tolG, int _tolR, IScriptEnvironment *env)
  : GenericVideoFilter(_child,__FUNCTION__ ), color(_color & 0xffffff), tolB(_tolB & 0xff), tolG(_tolG & 0xff), tolR(_tolR & 0xff)
{
  if (!vi.IsRGB32())
    env->ThrowError("ColorKeyMask: requires RGB32 input");
}

PVideoFrame __stdcall ColorKeyMask::GetFrame(int n, IScriptEnvironment *env)
{
  PVideoFrame frame = child->GetFrame(n, env);
  if (frame == nullptr || frame.m_ptr == nullptr) {
      return nullptr;
  }
  env->MakeWritable(&frame);

  BYTE* pf = frame->GetWritePtr();
  const int pitch = frame->GetPitch();
  const int rowsize = frame->GetRowSize();

#ifdef _M_IX86
  if (!(env->GetCPUFlags() & CPUF_MMX) || vi.width==1) {
#endif
	  const int R = (color >> 16) & 0xff;
    const int G = (color >> 8) & 0xff;
    const int B = color & 0xff;

    for (int y=0; y<vi.height; y++) {
      for (int x=0; x<rowsize; x+=4) {
        if (IsClose(pf[x],B,tolB) && IsClose(pf[x+1],G,tolG) && IsClose(pf[x+2],R,tolR))
          pf[x+3]=0;
      }
      pf += pitch;
    }
#ifdef _M_IX86
  } else { // MMX
    const int height = vi.height;
    const int col8 = color;
    const int tol8 = 0xff000000 | (tolR << 16) | (tolG << 8) | tolB;
    const int xloopcount = -(rowsize & -8);
    pf -= xloopcount;
    __asm {
      mov       esi, pf
      mov       edx, height
      pxor      mm0, mm0
      movd      mm1, col8
      movd      mm2, tol8
      punpckldq mm1, mm1
      punpckldq mm2, mm2

yloop:
      mov       ecx, xloopcount
xloop:
      movq      mm3, [esi+ecx]
      movq      mm4, mm1
      movq      mm5, mm3
      psubusb   mm4, mm3
      psubusb   mm5, mm1
      por       mm4, mm5
      psubusb   mm4, mm2
      add       ecx, 8
      pcmpeqd   mm4, mm0
      pslld     mm4, 24
      pandn     mm4, mm3
      movq      [esi+ecx-8], mm4
      jnz       xloop

      mov       ecx, rowsize
      and       ecx, 7
      jz        not_odd
      ; process last pixel
      movd      mm3, [esi]
      movq      mm4, mm1
      movq      mm5, mm3
      psubusb   mm4, mm3
      psubusb   mm5, mm1
      por       mm4, mm5
      psubusb   mm4, mm2
      pcmpeqd   mm4, mm0
      pslld     mm4, 24
      pandn     mm4, mm3
      movd      [esi], mm4

not_odd:
      add       esi, pitch
      dec       edx
      jnz       yloop
      emms
    }
  }
#endif
  return frame;
}

AVSValue __cdecl ColorKeyMask::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  return new ColorKeyMask(args[0].AsClip(), args[1].AsInt(0),
                          args[2].AsInt(10),
                          args[3].AsInt(args[2].AsInt(10)),
                          args[4].AsInt(args[2].AsInt(10)), env);
}








/********************************
 ******  ResetMask filter  ******
 ********************************/


ResetMask::ResetMask(PClip _child, IScriptEnvironment* env)
  : GenericVideoFilter(_child,__FUNCTION__ )
{
  if (!vi.IsRGB32())
    env->ThrowError("ResetMask: RGB32 data only");
}


PVideoFrame ResetMask::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame f = child->GetFrame(n, env);
  if (f == nullptr || f.m_ptr == nullptr) {
      return nullptr;
  }
  env->MakeWritable(&f);

  BYTE* pf = f->GetWritePtr();
  int pitch = f->GetPitch();
  int rowsize = f->GetRowSize();
  int height = f->GetHeight();

  for (int i=0; i<height; i++) {
    for (int j=3; j<rowsize; j+=4)
      pf[j] = 255;
    pf += pitch;
  }

  return f;
}


AVSValue ResetMask::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  return new ResetMask(args[0].AsClip(), env);
}




/********************************
 ******  Invert filter  ******
 ********************************/


Invert::Invert(PClip _child, const char * _channels, IScriptEnvironment* env)
  : GenericVideoFilter(_child,__FUNCTION__ ), channels(_channels)
{

}


PVideoFrame Invert::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame f = child->GetFrame(n, env);
  if (f == nullptr || f.m_ptr == nullptr) {
      return nullptr;
  }
  env->MakeWritable(&f);

  BYTE* pf = f->GetWritePtr();
  int pitch = f->GetPitch();
  int rowsize = f->GetRowSize();
  int height = f->GetHeight();


  bool doB = false;
  bool doG = false;
  bool doR = false;
  bool doA = false;

  bool doY = false;
  bool doU = false;
  bool doV = false;
  char ch = 1;

  for (int k=0; ch!='\0'; ++k) {
    ch = tolower(channels[k]);
    if (ch == 'b')
      doB = true;
    if (ch == 'g')
      doG = true;
    if (ch == 'r')
      doR = true;
    if (ch == 'a')
      doA = true;

    if (ch == 'y')
      doY = true;
    if (ch == 'u')
      doU = !vi.IsY8();
    if (ch == 'v')
      doV = !vi.IsY8();
  }

  if (vi.IsYUY2()) {
    int mask = doY ? 0x00ff00ff : 0;
    mask |= doU ? 0x0000ff00 : 0;
    mask |= doV ? 0xFF000000 : 0;

    ConvertFrame(pf, pitch, rowsize, height, mask);
  }

  if (vi.IsRGB32()) {
    int mask = doB ? 0xff : 0;
    mask |= doG ? 0xff00 : 0;
    mask |= doR ? 0xff0000 : 0;
    mask |= doA ? 0xff000000 : 0;
    ConvertFrame(pf, pitch, rowsize, height, mask);
  }

  if (vi.IsPlanar()) {
    if (doY)
      ConvertFrame(pf, pitch, f->GetRowSize(PLANAR_Y_ALIGNED), height, 0xffffffff);
    if (doU)
      ConvertFrame(f->GetWritePtr(PLANAR_U), f->GetPitch(PLANAR_U), f->GetRowSize(PLANAR_U_ALIGNED), f->GetHeight(PLANAR_U), 0xffffffff);
    if (doV)
      ConvertFrame(f->GetWritePtr(PLANAR_V), f->GetPitch(PLANAR_V), f->GetRowSize(PLANAR_V_ALIGNED), f->GetHeight(PLANAR_V), 0xffffffff);
  }

  if (vi.IsRGB24()) {
    int rMask= doR ? 0xff : 0;
    int gMask= doG ? 0xff : 0;
    int bMask= doB ? 0xff : 0;
    for (int i=0; i<height; i++) {

      for (int j=0; j<rowsize; j+=3) {
        pf[j] = pf[j] ^ bMask;
        pf[j+1] = pf[j+1] ^ gMask;
        pf[j+2] = pf[j+2] ^ rMask;
      }
      pf += pitch;
    }
  }

  return f;
}

/**********************
 * MMX invert function.
 *
 * Originally written by Klaus Post.
 *
 * Rewritten and optimized by ARDA.
 **********************/

void Invert::ConvertFrame
         (BYTE* frame,
          int pitch,
          int rowsize,    //must be mod 4
          int height,
          int mask) {
#ifdef _M_IX86
__asm {
    movd mm7,[mask]

    mov eax,[pitch]
    mov edi,[height]
    mov esi,[frame]
    sub eax,[rowsize]      //modulo

    punpckldq mm7,mm7
align 16
yloopback:
    mov ecx,[rowsize]
    mov edx,ecx
    sar ecx,5
    and edx,31
    test ecx,ecx
    jz  resttest
align 16
testloop:
//    prefetchnta [esi+256]

    movq mm0,[esi]
    movq mm1,[esi+8]
    movq mm2,[esi+16]
    movq mm3,[esi+24]

    pxor mm0, mm7
    pxor mm1, mm7
    pxor mm2, mm7
    pxor mm3, mm7

    movq [esi], mm0
    movq [esi+8], mm1
    movq [esi+16], mm2
    movq [esi+24], mm3

    add esi,32
    dec ecx
    jne testloop

resttest:
    test edx,edx
    jz outw

align 16
restloop:
    movd mm0,[esi]
    pxor mm0, mm7
    movd [esi],mm0
    add esi,4
    sub edx,4
    jg  restloop
align 16
outw:
    add esi,eax
    dec edi//sub height,1
    jne yloopback
    emms
  };
#else
	for (int y = 0; y < height; ++y) {
		uint8_t* rowPtr = frame + y * pitch;
		int remainingBytes = rowsize;

		// ��32λ��Ϊ��λ���д���
		while (remainingBytes >= 4) {
			// �����������ݶ�ȡ�ͷ�ת
			uint32_t* pixelPtr = reinterpret_cast<uint32_t*>(rowPtr);
			*pixelPtr ^= mask;

			// �ƶ�ָ��
			rowPtr += 4;
			remainingBytes -= 4;
		}

		// �����޷��� 4 ������ʣ���ֽ�
		while (remainingBytes > 0) {
			*rowPtr ^= static_cast<uint8_t>(mask & 0xFF);
			++rowPtr;
			--remainingBytes;
		}
	}
#endif
}


AVSValue Invert::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  return new Invert(args[0].AsClip(), args[0].AsClip()->GetVideoInfo().IsRGB() ? args[1].AsString("RGBA") : args[1].AsString("YUV"), env);
}




/**********************************
 ******  ShowChannel filter  ******
 **********************************/


ShowChannel::ShowChannel(PClip _child, const char * pixel_type, int _channel, IScriptEnvironment* env)
  : GenericVideoFilter(_child,__FUNCTION__ ), channel(_channel), input_type(_child->GetVideoInfo().pixel_type)
{
  static const char * const ShowText[4] = {"Blue", "Green", "Red", "Alpha"};

  if ((channel == 3) && !vi.IsRGB32())
    env->ThrowError("ShowAlpha: RGB32 data only");

  if (!vi.IsRGB())
    env->ThrowError("Show%s: RGB data only", ShowText[channel]);

  if (!lstrcmpiA(pixel_type, "rgb")) {
    vi.pixel_type = CS_BGR32;
  }
  else if (!lstrcmpiA(pixel_type, "rgb32")) {
    vi.pixel_type = CS_BGR32;
  }
  else if (!lstrcmpiA(pixel_type, "rgb24")) {
    vi.pixel_type = CS_BGR24;
  }
  else if (!lstrcmpiA(pixel_type, "yuy2")) {
    if (vi.width & 1) {
      env->ThrowError("Show%s: width must be mod 2 for yuy2", ShowText[channel]);
    }
    vi.pixel_type = CS_YUY2;
  }
  else if (!lstrcmpiA(pixel_type, "yv12")) {
    if (vi.width & 1) {
      env->ThrowError("Show%s: width must be mod 2 for yv12", ShowText[channel]);
    }
    if (vi.height & 1) {
      env->ThrowError("Show%s: height must be mod 2 for yv12", ShowText[channel]);
    }
    vi.pixel_type = CS_YV12;
  }
  else if (!lstrcmpiA(pixel_type, "y8")) {
    vi.pixel_type = CS_Y8;
  }
  else {
    env->ThrowError("Show%s supports the following output pixel types: RGB, Y8, YUY2, or YV12", ShowText[channel]);
  }
}


PVideoFrame ShowChannel::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame f = child->GetFrame(n, env);
  if (f == nullptr || f.m_ptr == nullptr) {
      return nullptr;
  }

  const BYTE* pf = f->GetReadPtr();
  const int height = f->GetHeight();
  const int pitch = f->GetPitch();
  const int rowsize = f->GetRowSize();

  if (input_type == CS_BGR32) {
    if (vi.pixel_type == CS_BGR32)
    {
      if (f->IsWritable()) {
        // we can do it in-place
        BYTE* dstp = f->GetWritePtr();

        for (int i=0; i<height; ++i) {
          for (int j=0; j<rowsize; j+=4) {
            dstp[j + 0] = dstp[j + 1] = dstp[j + 2] = dstp[j + channel];
          }
          dstp += pitch;
        }
        return f;
      }
      else {
        PVideoFrame dst = env->NewVideoFrame(vi);
        if (dst == nullptr || dst.m_ptr == nullptr) {
            return f;
        }
        BYTE * dstp = dst->GetWritePtr();
        const int dstpitch = dst->GetPitch();

        for (int i=0; i<height; ++i) {
          for (int j=0; j<rowsize; j+=4) {
            dstp[j + 0] = dstp[j + 1] = dstp[j + 2] = pf[j + channel];
            dstp[j + 3] = pf[j + 3];
          }
          pf   += pitch;
          dstp += dstpitch;
        }
        return dst;
      }
    }
    else if (vi.pixel_type == CS_BGR24)
    {
      PVideoFrame dst = env->NewVideoFrame(vi);
      if (dst == nullptr || dst.m_ptr == nullptr) {
          return f;
      }
      BYTE * dstp = dst->GetWritePtr();
      const int dstpitch = dst->GetPitch();

      for (int i=0; i<height; ++i) {
        for (int j=0; j<rowsize/4; j++) {
          dstp[j*3 + 0] = dstp[j*3 + 1] = dstp[j*3 + 2] = pf[j*4 + channel];
        }
        pf   += pitch;
        dstp += dstpitch;
      }
      return dst;
    }
    else if (vi.pixel_type == CS_YUY2)
    {
      PVideoFrame dst = env->NewVideoFrame(vi);
      if (dst == nullptr || dst.m_ptr == nullptr) {
          return f;
      }
      BYTE * dstp = dst->GetWritePtr();
      const int dstpitch = dst->GetPitch();
      const int dstrowsize = dst->GetRowSize();

      // RGB is upside-down
      pf += (height-1) * pitch;

      for (int i=0; i<height; ++i) {
        for (int j=0; j<dstrowsize; j+=2) {
          dstp[j + 0] = pf[j*2 + channel];
          dstp[j + 1] = 128;
        }
        pf -= pitch;
        dstp += dstpitch;
      }
      return dst;
    }
    else
    {
      if ((vi.pixel_type == CS_YV12) || (vi.pixel_type == CS_Y8))
      {
        PVideoFrame dst = env->NewVideoFrame(vi);
        if (dst == nullptr || dst.m_ptr == nullptr) {
            return f;
        }
        BYTE * dstp = dst->GetWritePtr();
        int dstpitch = dst->GetPitch();
        int dstrowsize = dst->GetRowSize();

        // RGB is upside-down
        pf += (height-1) * pitch;

        for (int i=0; i<height; ++i) {
          for (int j=0; j<dstrowsize; ++j) {
            dstp[j] = pf[j*4 + channel];
          }
          pf -= pitch;
          dstp += dstpitch;
        }
        if (vi.pixel_type == CS_YV12)
        {
          dstpitch = dst->GetPitch(PLANAR_U);
          dstrowsize = dst->GetRowSize(PLANAR_U_ALIGNED)/4;
          const int dstheight = dst->GetHeight(PLANAR_U);
          BYTE * dstpu = dst->GetWritePtr(PLANAR_U);
          BYTE * dstpv = dst->GetWritePtr(PLANAR_V);
          for (int i=0; i<dstheight; ++i) {
            for (int j=0; j<dstrowsize; ++j) {
              ((unsigned int*) dstpu)[j] = ((unsigned int*) dstpv)[j] = 0x80808080;
            }
            dstpu += dstpitch;
            dstpv += dstpitch;
          }
        }
        return dst;
      }
    }
  }
  else if (input_type == CS_BGR24) {
    if (vi.pixel_type == CS_BGR24)
    {
      if (f->IsWritable()) {
        // we can do it in-place
        BYTE* dstp = f->GetWritePtr();

        for (int i=0; i<height; ++i) {
          for (int j=0; j<rowsize; j+=3) {
            dstp[j + 0] = dstp[j + 1] = dstp[j + 2] = dstp[j + channel];
          }
          dstp += pitch;
        }
        return f;
      }
      else {
        PVideoFrame dst = env->NewVideoFrame(vi);
        if (dst == nullptr || dst.m_ptr == nullptr) {
            return f;
        }
        BYTE * dstp = dst->GetWritePtr();
        const int dstpitch = dst->GetPitch();

        for (int i=0; i<height; ++i) {
          for (int j=0; j<rowsize; j+=3) {
            dstp[j + 0] = dstp[j + 1] = dstp[j + 2] = pf[j + channel];
          }
          pf   += pitch;
          dstp += dstpitch;
        }
        return dst;
      }
    }
    else if (vi.pixel_type == CS_BGR32)
    {
      PVideoFrame dst = env->NewVideoFrame(vi);
      if (dst == nullptr || dst.m_ptr == nullptr) {
          return f;
      }
      BYTE * dstp = dst->GetWritePtr();
      const int dstpitch = dst->GetPitch();

      for (int i=0; i<height; ++i) {
        for (int j=0; j<rowsize/3; j++) {
          dstp[j*4 + 0] = dstp[j*4 + 1] = dstp[j*4 + 2] = dstp[j*4 + 3] = pf[j*3 + channel];
        }
        pf   += pitch;
        dstp += dstpitch;
      }
      return dst;
    }
    else if (vi.pixel_type == CS_YUY2)
    {
      PVideoFrame dst = env->NewVideoFrame(vi);
      if (dst == nullptr || dst.m_ptr == nullptr) {
          return f;
      }
      BYTE * dstp = dst->GetWritePtr();
      const int dstpitch = dst->GetPitch();
      const int dstrowsize = dst->GetRowSize()/2;

      // RGB is upside-down
      pf += (height-1) * pitch;

      for (int i=0; i<height; ++i) {
        for (int j=0; j<dstrowsize; j++) {
          dstp[j*2 + 0] = pf[j*3 + channel];
          dstp[j*2 + 1] = 128;
        }
        pf -= pitch;
        dstp += dstpitch;
      }
      return dst;
    }
    else
    {
      if ((vi.pixel_type == CS_YV12) || (vi.pixel_type == CS_Y8))
      {
        int i, j;  // stupid VC6

        PVideoFrame dst = env->NewVideoFrame(vi);
        if (dst == nullptr || dst.m_ptr == nullptr) {
            return f;
        }
        BYTE * dstp = dst->GetWritePtr();
        int dstpitch = dst->GetPitch();
        int dstrowsize = dst->GetRowSize();

        // RGB is upside-down
        pf += (height-1) * pitch;

        for (i=0; i<height; ++i) {
          for (j=0; j<dstrowsize; ++j) {
            dstp[j] = pf[j*3 + channel];
          }
          pf -= pitch;
          dstp += dstpitch;
        }
        if (vi.pixel_type == CS_YV12)
        {
          dstpitch = dst->GetPitch(PLANAR_U);
          dstrowsize = dst->GetRowSize(PLANAR_U_ALIGNED)/4;
          const int dstheight = dst->GetHeight(PLANAR_U);
          BYTE * dstpu = dst->GetWritePtr(PLANAR_U);
          BYTE * dstpv = dst->GetWritePtr(PLANAR_V);
          for (i=0; i<dstheight; ++i) {
            for (j=0; j<dstrowsize; ++j) {
              ((unsigned int*) dstpu)[j] = ((unsigned int*) dstpv)[j] = 0x80808080;
            }
            dstpu += dstpitch;
            dstpv += dstpitch;
          }
        }
        return dst;
      }
    }
  }
  env->ThrowError("ShowChannel: unexpected end of function");
  return f;
}


AVSValue ShowChannel::Create(AVSValue args, void* channel, IScriptEnvironment* env)
{
  return new ShowChannel(args[0].AsClip(), args[1].AsString("RGB"), (intptr_t)channel, env);
}





/**********************************
 ******  MergeRGB filter  ******
 **********************************/


MergeRGB::MergeRGB(PClip _child, PClip _blue, PClip _green, PClip _red, PClip _alpha,
                   const char * pixel_type, IScriptEnvironment* env)
  : GenericVideoFilter(_child,__FUNCTION__ ), blue(_blue), green(_green), red(_red), alpha(_alpha),
    viB(blue->GetVideoInfo()), viG(green->GetVideoInfo()), viR(red->GetVideoInfo()),
    viA(((alpha) ? alpha : child)->GetVideoInfo()), myname((alpha) ? "MergeARGB" : "MergeRGB")
{

  if (!lstrcmpiA(pixel_type, "rgb32")) {
    vi.pixel_type = CS_BGR32;
    if (alpha && (viA.pixel_type == CS_BGR24))
      env->ThrowError("MergeARGB: Alpha source channel may not be RGB24");
  }
  else if (!lstrcmpiA(pixel_type, "rgb24")) {
    vi.pixel_type = CS_BGR24;
  }
  else {
    env->ThrowError("MergeRGB: supports the following output pixel types: RGB24, or RGB32");
  }

  if ((vi.width  != viB.width)  || (vi.width  != viG.width)  || (vi.width  != viR.width)  || (vi.width != viA.width))
    env->ThrowError("%s: All clips must have the same width.", myname);

  if ((vi.height != viB.height) || (vi.height != viG.height) || (vi.height != viR.height) || (vi.height != viA.height))
    env->ThrowError("%s: All clips must have the same height.", myname);
}


PVideoFrame MergeRGB::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame B = blue->GetFrame(n, env);
  if (B == nullptr || B.m_ptr == nullptr) {
      return nullptr;
  }
  PVideoFrame G = green->GetFrame(n, env);
  if (G == nullptr || G.m_ptr == nullptr) {
      return nullptr;
  }
  PVideoFrame R = red->GetFrame(n, env);
  if (R == nullptr || R.m_ptr == nullptr) {
      return nullptr;
  }
  PVideoFrame A = (alpha) ? alpha->GetFrame(n, env) : 0;

  PVideoFrame dst = env->NewVideoFrame(vi);
  if (dst == nullptr || dst.m_ptr == nullptr) {
      return nullptr;
  }

  const int height = dst->GetHeight();
  const int pitch = dst->GetPitch();
  const int rowsize = dst->GetRowSize();
  const int modulo = pitch - rowsize;

  BYTE* dstp = dst->GetWritePtr();

  // RGB is upside-down, backscan any YUV to match
  const int Bpitch = (viB.IsYUV()) ? -(B->GetPitch()) : B->GetPitch();
  const int Gpitch = (viG.IsYUV()) ? -(G->GetPitch()) : G->GetPitch();
  const int Rpitch = (viR.IsYUV()) ? -(R->GetPitch()) : R->GetPitch();

  // Bump any RGB channels, move any YUV channels to last line
  const BYTE* Bp = B->GetReadPtr() + ((Bpitch < 0) ? Bpitch * (1-height) : 0);
  const BYTE* Gp = G->GetReadPtr() + ((Gpitch < 0) ? Gpitch * (1-height) : 1);
  const BYTE* Rp = R->GetReadPtr() + ((Rpitch < 0) ? Rpitch * (1-height) : 2);

  // Adjustment from the end of 1 line to the start of the next
  const int Bmodulo = Bpitch - B->GetRowSize();
  const int Gmodulo = Gpitch - G->GetRowSize();
  const int Rmodulo = Rpitch - R->GetRowSize();

  // Number of bytes per pixel (1, 2, 3 or 4)
  const int Bstride = viB.IsPlanar() ? 1 : (viB.BitsPerPixel()>>3);
  const int Gstride = viG.IsPlanar() ? 1 : (viG.BitsPerPixel()>>3);
  const int Rstride = viR.IsPlanar() ? 1 : (viR.BitsPerPixel()>>3);

  // End of VFB
  BYTE const * yend = dstp + pitch*height;

  if (alpha) { // ARGB mode
    const int Apitch = (viA.IsYUV()) ? -(A->GetPitch()) : A->GetPitch();
    const BYTE* Ap = A->GetReadPtr() + ((Apitch < 0) ? Apitch * (1-height) : 3);
    const int Amodulo = Apitch - A->GetRowSize();
    const int Astride = viA.IsPlanar() ? 1 : (viA.BitsPerPixel()>>3);

    while (dstp < yend) {
      BYTE const * xend = dstp + rowsize;
      while (dstp < xend) {
        *dstp++ = *Bp; Bp += Bstride;
        *dstp++ = *Gp; Gp += Gstride;
        *dstp++ = *Rp; Rp += Rstride;
        *dstp++ = *Ap; Ap += Astride;
      }
      dstp += modulo;
      Bp += Bmodulo;
      Gp += Gmodulo;
      Rp += Rmodulo;
      Ap += Amodulo;
    }
  }
  else if (vi.pixel_type == CS_BGR32) { // RGB32 mode
    while (dstp < yend) {
      BYTE const * xend = dstp + rowsize;
      while (dstp < xend) {
        *dstp++ = *Bp; Bp += Bstride;
        *dstp++ = *Gp; Gp += Gstride;
        *dstp++ = *Rp; Rp += Rstride;
        *dstp++ = 0;
      }
      dstp += modulo;
      Bp += Bmodulo;
      Gp += Gmodulo;
      Rp += Rmodulo;
    }
  }
  else if (vi.pixel_type == CS_BGR24) { // RGB24 mode
    while (dstp < yend) {
      BYTE const * xend = dstp + rowsize;
      while (dstp < xend) {
        *dstp++ = *Bp; Bp += Bstride;
        *dstp++ = *Gp; Gp += Gstride;
        *dstp++ = *Rp; Rp += Rstride;
      }
      dstp += modulo;
      Bp += Bmodulo;
      Gp += Gmodulo;
      Rp += Rmodulo;
    }
  }
  else
    env->ThrowError("%s: unexpected end of function", myname);

  return dst;
}


AVSValue MergeRGB::Create(AVSValue args, void* mode, IScriptEnvironment* env)
{
  if (mode) // ARGB
    return new MergeRGB(args[0].AsClip(), args[3].AsClip(), args[2].AsClip(), args[1].AsClip(), args[0].AsClip(), "RGB32", env);
  else      // RGB[type]
    return new MergeRGB(args[0].AsClip(), args[2].AsClip(), args[1].AsClip(), args[0].AsClip(), 0, args[3].AsString("RGB32"), env);
}





/*******************************
 *******   Layer Filter   ******
 *******************************/

#define RGBATIMELINE 1

Layer::Layer( PClip _child1, PClip _child2, const char _op[], int _lev, int _x, int _y,
              int _t, bool _chroma, IScriptEnvironment* env, int _offset)
  : IClip(__FUNCTION__), child1(_child1), child2(_child2), levelB(_lev), ofsX(_x), ofsY(_y), Op(_op),
    T(_t), chroma(_chroma),offset(_offset)
{
	
  const VideoInfo& vi1 = child1->GetVideoInfo();
  const VideoInfo& vi2 = child2->GetVideoInfo();


  vi = vi1;
#if RGBATIMELINE
#else
	if (vi.IsYV12())
	  vi.pixel_type = CS_I420;
#endif

  if (vi.IsRGB32()) ofsY = vi.height-vi2.height-ofsY; //RGB is upside down
  else ofsX = ofsX & 0xFFFFFFFE; //YUV must be aligned on even pixels

  xdest=(ofsX < 0)? 0: ofsX;
  ydest=(ofsY < 0)? 0: ofsY;

  xsrc=(ofsX < 0)? (0-ofsX): 0;
  ysrc=(ofsY < 0)? (0-ofsY): 0;

  xcount = (vi.width < (ofsX + vi2.width))? (vi.width-xdest) : (vi2.width - xsrc);
  ycount = (vi.height <  (ofsY + vi2.height))? (vi.height-ydest) : (vi2.height - ysrc);

  if (!( !lstrcmpiA(Op, "Mul") || !lstrcmpiA(Op, "Add") || !lstrcmpiA(Op, "Fast") ||
         !lstrcmpiA(Op, "Subtract") || !lstrcmpiA(Op, "Lighten") || !lstrcmpiA(Op, "Darken") ))
    env->ThrowError("Layer supports the following ops: Fast, Lighten, Darken, Add, Subtract, Mul");

  if (!chroma)
  {
    if (!lstrcmpiA(Op, "Darken") ) env->ThrowError("Layer: monochrome darken illegal op");
    if (!lstrcmpiA(Op, "Lighten")) env->ThrowError("Layer: monochrome lighten illegal op");
    if (!lstrcmpiA(Op, "Fast")   ) env->ThrowError("Layer: this mode not allowed in FAST; use ADD instead");
  }

  overlay_frames = vi2.num_frames;
}

PVideoFrame __stdcall Layer::GetFrame(int n, IScriptEnvironment* env)
{
    PVideoFrame src1 = child1->GetFrame(n, env);
    if (src1 == NULL || src1.m_ptr == NULL) {
        return src1;
    }

    if (xcount <= 0 || ycount <= 0 || offset > n) return src1;

    PVideoFrame src2 = child2->GetFrame(std::min(n, overlay_frames - 1), env);
    if (src2 == NULL || src2.m_ptr == NULL) {
        return src1;
    }


    if (vi.IsRGB32())
    {
        auto baseimage = src1->GetReadPtr() + 4 * xdest + ydest * src1->GetPitch();
        env->MakeWritable(&src1);
        auto dst = src1->GetWritePtr();
        dst = dst + 4 * xdest + ydest * src1->GetPitch();
        auto over = src2->GetReadPtr() + 4 * xsrc + ysrc * src2->GetPitch();

        env->MakeWritable(&src2);
        auto overtemp = src2->GetWritePtr() + 4 * xsrc + ysrc * src2->GetPitch();

        libyuv::ARGBAttenuate(over, src2->GetPitch(), overtemp, src2->GetPitch(), xcount, ycount);

        if (levelB == 0)
        {
            libyuv::ARGBAdd(overtemp, src2->GetPitch(), baseimage, src1->GetPitch(), dst, src1->GetPitch(), xcount, ycount);
        }
        else
        {
            libyuv::ARGBBlend(overtemp, src2->GetPitch(), baseimage, src1->GetPitch(), dst, src1->GetPitch(), xcount, ycount);
        }

        return src1;
    }

    return src1;
}



AVSValue __cdecl Layer::Create(AVSValue args, void*, IScriptEnvironment* env)
{
	
  return new Layer( args[0].AsClip(), args[1].AsClip(), args[2].AsString("Add"), args[3].AsInt(257),
                    args[4].AsInt(0), args[5].AsInt(0), args[6].AsInt(0), args[7].AsBool(true), env,0 );
}

AVSValue __cdecl Layer::TinyCreate(AVSValue args, void*, IScriptEnvironment* env)
{
	return new Layer(args[0].AsClip(), args[1].AsClip(), "Add", 257,
		args[2].AsInt(0), args[3].AsInt(0), 0, true, env,args[4].AsInt(0));
}


/*******************************
 *******   AutoLayer Filter   ******
 *******************************/

AutoLayer::AutoLayer(PClip _child1, PClip _child2, const char _op[], int _lev, int _x, int _y,
	int _t, bool _chroma, IScriptEnvironment* env, int _offset, int color)
	: IClip(__FUNCTION__), child1(_child1), child2(_child2), levelB(_lev), ofsX(_x), ofsY(_y), Op(_op),
	T(_t), chroma(_chroma), offset(_offset), blankcolor(color)
{

	const VideoInfo& vi1 = child1->GetVideoInfo();
	const VideoInfo& vi2 = child2->GetVideoInfo();

	/*if (vi1.pixel_type != vi2.pixel_type)
		env->ThrowError("Layer: image formats don't match");

	if (!(vi1.IsRGB32() | vi1.IsYUY2()))
		env->ThrowError("Layer only support RGB32 and YUY2 formats");*/

	vi = vi1;

	if (vi.IsRGB32()) ofsY = vi.height - vi2.height - ofsY; //RGB is upside down
	else ofsX = ofsX & 0xFFFFFFFE; //YUV must be aligned on even pixels

	xdest = (ofsX < 0) ? 0 : ofsX;
	ydest = (ofsY < 0) ? 0 : ofsY;

	xsrc = (ofsX < 0) ? (0 - ofsX) : 0;
	ysrc = (ofsY < 0) ? (0 - ofsY) : 0;

	xcount = (vi.width < (ofsX + vi2.width)) ? (vi.width - xdest) : (vi2.width - xsrc);
	ycount = (vi.height < (ofsY + vi2.height)) ? (vi.height - ydest) : (vi2.height - ysrc);

	overlay_frames = vi2.num_frames;
	//�Զ�����
	vi.num_frames = std::max(vi.num_frames, _offset + overlay_frames);

	//blankclip = env->Invoke("Blank")
	AVSValue values[] = {child1, blankcolor};
	char* name[] = { NULL, "color" };
	//blankclip = env->Invoke("BlankClip", AVSValue(values, 2),name ).AsClip();
	
}

PVideoFrame __stdcall AutoLayer::GetFrame(int n, IScriptEnvironment* env)
{
	PVideoFrame src1;
	if (n<child1->GetVideoInfo().num_frames)
	{
		src1 = child1->GetFrame(n, env);
	}
	else
	{
		src1 = child1->GetFrame(child1->GetVideoInfo().num_frames-1, env);
		//src1 = blankclip->GetFrame(0, env);
	}

    if (src1 == nullptr || src1.m_ptr == nullptr) {
        return nullptr;
    }
	//n = max( n - offset,0);

	if (xcount <= 0 || ycount <= 0 || offset > n) return src1;

	if (n>= offset+ child2->GetVideoInfo().num_frames)
	{
		return src1;
	}

	PVideoFrame src2 = child2->GetFrame(std::min(n-offset, overlay_frames - 1), env);
    if (src2 == nullptr || src2.m_ptr == nullptr) {
        return src1;
    }

	if (vi.IsYV12())
	{
		auto srcpitchy = src2->GetPitch(PLANAR_Y);

		auto srcpitchuv = src2->GetPitch(PLANAR_U);
		auto srcy = src2->GetReadPtr(PLANAR_Y) + xsrc + ysrc * srcpitchy;
		auto srcu = src2->GetReadPtr(PLANAR_U) + xsrc / 2 + ysrc / 2 * srcpitchy / 2;
		auto srcv = src2->GetReadPtr(PLANAR_V) + xsrc / 2 + ysrc / 2 * srcpitchy / 2;

		env->MakeWritable(&src1);
		auto dstpitchy = src1->GetPitch(PLANAR_Y);
		auto dsty = src1->GetWritePtr(PLANAR_Y) + xdest + ydest * dstpitchy;
		auto dstu = src1->GetWritePtr(PLANAR_U) + xdest / 2 + ydest / 2 * dstpitchy / 2;
		auto dstv = src1->GetWritePtr(PLANAR_V) + xdest / 2 + ydest / 2 * dstpitchy / 2;

		libyuv::I420ToI420(srcy, srcpitchy, srcu, srcpitchy / 2, srcv, srcpitchy / 2,
			dsty, dstpitchy, dstu, dstpitchy / 2, dstv, dstpitchy / 2,
			xcount, ycount
		);
	}

	if ( vi.IsRGB32())
	{
		auto baseimage = src1->GetReadPtr() + 4 * xdest + ydest * src1->GetPitch();
		env->MakeWritable(&src1);
		auto dst = src1->GetWritePtr();
		dst = dst + 4 * xdest + ydest * src1->GetPitch();
		auto over = src2->GetReadPtr() + 4 * xsrc + ysrc * src2->GetPitch();
		
		env->MakeWritable(&src2);
		auto overtemp= src2->GetWritePtr() + 4 * xsrc + ysrc * src2->GetPitch();

		libyuv::ARGBAttenuate(over, src2->GetPitch(), overtemp, src2->GetPitch(), xcount, ycount);

		if (levelB == 0)
		{
			libyuv::ARGBAdd(overtemp, src2->GetPitch(), baseimage, src1->GetPitch(), dst, src1->GetPitch(), xcount, ycount);
		}
		else
		{
			libyuv::ARGBBlend(overtemp, src2->GetPitch(), baseimage, src1->GetPitch(), dst, src1->GetPitch(), xcount, ycount);
		}

		return src1;
	}
	return src1;
}



AVSValue __cdecl AutoLayer::Create(AVSValue args, void*, IScriptEnvironment* env)
{

	return new Layer(args[0].AsClip(), args[1].AsClip(), args[2].AsString("Add"), args[3].AsInt(257),
		args[4].AsInt(0), args[5].AsInt(0), args[6].AsInt(0), args[7].AsBool(true), env, 0);
}

AVSValue __cdecl AutoLayer::TinyCreate(AVSValue args, void*, IScriptEnvironment* env)
{
	return new AutoLayer(args[0].AsClip(), args[1].AsClip(), "Add", 257,
		args[2].AsInt(0), args[3].AsInt(0), 0, true, env, args[4].AsInt(0), args[5].AsInt(0));
}





/**********************************
 *******   Subtract Filter   ******
 *********************************/
bool Subtract::DiffFlag = false;
BYTE Subtract::Diff[513];

Subtract::Subtract(PClip _child1, PClip _child2, IScriptEnvironment* env)
  : IClip(__FUNCTION__), child1(_child1), child2(_child2)
{
  VideoInfo vi1 = child1->GetVideoInfo();
  VideoInfo vi2 = child2->GetVideoInfo();

  if (vi1.width != vi2.width || vi1.height != vi2.height)
    env->ThrowError("Subtract: image dimensions don't match");

  if (!(vi1.IsSameColorspace(vi2)))
    env->ThrowError("Subtract: image formats don't match");

  vi = vi1;
  vi.num_frames = std::max(vi1.num_frames, vi2.num_frames);
  vi.num_audio_samples = std::max(vi1.num_audio_samples, vi2.num_audio_samples);

  if (!DiffFlag) { // Init the global Diff table
    DiffFlag = true;
    for (int i=0; i<=512; i++) Diff[i] = std::max(0, std::min(255,i-129));
  }
}

/*	// abs(a - b)
	movq	mm2, mm0
	psubusb	mm0, mm1
	psubusb	mm1, mm2
	por		mm0, mm1
*/

PVideoFrame __stdcall Subtract::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame src1 = child1->GetFrame(n, env);
  if (src1 == nullptr || src1.m_ptr == nullptr) {
      return nullptr;
  }
  PVideoFrame src2 = child2->GetFrame(n, env);

  if (src2 == nullptr || src2.m_ptr == nullptr) {
      return nullptr;
  }
  env->MakeWritable(&src1);

  BYTE* src1p = src1->GetWritePtr();
  const BYTE* src2p = src2->GetReadPtr();
  int row_size = src1->GetRowSize();

  if (vi.IsPlanar()) {
    for (int y=0; y<vi.height; y++) {
      for (int x=0; x<row_size; x++) {
        src1p[x] = Diff[src1p[x] - src2p[x] + 126 + 129];
      }
      src1p += src1->GetPitch();
      src2p += src2->GetPitch();
    }

    row_size=src1->GetRowSize(PLANAR_U);
    if (row_size) {
      BYTE* src1p = src1->GetWritePtr(PLANAR_U);
      const BYTE* src2p = src2->GetReadPtr(PLANAR_U);
      BYTE* src1pV = src1->GetWritePtr(PLANAR_V);
      const BYTE* src2pV = src2->GetReadPtr(PLANAR_V);

      for (int y=0; y<src1->GetHeight(PLANAR_U); y++) {
        for (int x=0; x<row_size; x++) {
          src1p[x] = Diff[src1p[x] - src2p[x] + 128 + 129];
          src1pV[x] = Diff[src1pV[x] - src2pV[x] + 128 + 129];
        }
        src1p += src1->GetPitch(PLANAR_U);
        src2p += src2->GetPitch(PLANAR_U);
        src1pV += src1->GetPitch(PLANAR_V);
        src2pV += src2->GetPitch(PLANAR_V);
      }
    }
    return src1;
  } // End planar

  // For YUY2, 50% gray is about (126,128,128) instead of (128,128,128).  Grr...
  if (vi.IsYUY2()) {
    for (int y=0; y<vi.height; ++y) {
      for (int x=0; x<row_size; x+=2) {
        src1p[x] = Diff[src1p[x] - src2p[x] + 126 + 129];
        src1p[x+1] = Diff[src1p[x+1] - src2p[x+1] + 128 + 129];
      }
      src1p += src1->GetPitch();
      src2p += src2->GetPitch();
    }
  }
  else { // RGB
    for (int y=0; y<vi.height; ++y) {
      for (int x=0; x<row_size; ++x)
        src1p[x] = Diff[src1p[x] - src2p[x] + 128 + 129];

      src1p += src1->GetPitch();
      src2p += src2->GetPitch();
    }
  }
  return src1;
}



AVSValue __cdecl Subtract::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  return new Subtract(args[0].AsClip(), args[1].AsClip(), env);
}
