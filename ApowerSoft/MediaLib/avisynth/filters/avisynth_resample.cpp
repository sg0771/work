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

#ifndef _CRTIMP
#define _CRTIMP
#endif
#include "avisynth/avisynth_stdafx.h"

#include "resample.h"

#define RGBATIMELINE 1

extern const AVSFunction Resample_filters[] = {
 { "TinyResize", "cii[src_width]f[src_height]f", FilteredResize::Create_TinyResize },
  { "PointResize", "cii[src_left]f[src_top]f[src_width]f[src_height]f", FilteredResize::Create_PointResize },
  { "BilinearResize", "cii[src_left]f[src_top]f[src_width]f[src_height]f", FilteredResize::Create_BilinearResize },

  { 0 }
};


struct VideoFrameData
{
    int width;
    int height;
	int pixel_type;
    PVideoFrame frame;
};
std::vector<VideoFrameData>s_arrResizerVideoFrame[MAX_CHANNEL];
EXTERN_C void yuvresize_clear() {
    for (size_t i = 0; i < MAX_CHANNEL; i++)
    {
        s_arrResizerVideoFrame[i].clear();
    }
}

class Resizer : public GenericVideoFilter {
public:
    int m_nID = 0;
    Resizer(PClip _child, int _width, int _height, IScriptEnvironment* env) :
        GenericVideoFilter(_child, __FUNCTION__), width(_width), height(_height) {
        vi.width = _width;
        vi.height = _height;
        std::string strFmt = "";
        if (vi.IsYV12()) {
            strFmt = "yv12";
        }
        else  if (vi.IsYV24()) {
            strFmt = "yv24";
        }
        else   if (vi.IsRGB32()) {
            strFmt = "rgb32";
        }
        else   if (vi.IsRGB()) {
            strFmt = "rgb24";
        }
        m_nID = this->GetID();
        if(m_nID<0)
            WXLogA("---- yuvresize ID=%d [%dx%d] [%s]", m_nID, width, height, strFmt.c_str());
    }

    virtual ~Resizer(void) {

    }

    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env)
    {
        PVideoFrame src = child->GetFrame(n, env);
        if (src == nullptr || src.m_ptr == nullptr) {
            return nullptr;
        }

        PVideoFrame dst = 0;
        if (m_nID >= 0) {
            for (size_t i = 0; i < s_arrResizerVideoFrame[m_nID].size(); i++)
            {
                if (s_arrResizerVideoFrame[m_nID][i].width == vi.width &&
                    s_arrResizerVideoFrame[m_nID][i].height == vi.height &&
                    s_arrResizerVideoFrame[m_nID][i].pixel_type == vi.pixel_type)
                {
                    dst = s_arrResizerVideoFrame[m_nID][i].frame;
                    //WXLogA("[%s] Find Exist VideoFrame! ", __FUNCTION__);
                    break;
                }
            }
        }
        else {
            dst = env->NewVideoFrame(vi);
        }



        if (dst == nullptr || dst.m_ptr == nullptr) {
            dst = env->NewVideoFrame(vi);
            
            if (dst == nullptr || dst.m_ptr == nullptr) {
                return nullptr;
            }
            if (m_nID >= 0) {
                VideoFrameData ss{ vi.width,vi.height,vi.pixel_type,dst };
                s_arrResizerVideoFrame[m_nID].push_back(ss);
                //WXLogA("[%s] Create VideoFrame! ", __FUNCTION__);
            }

        }

        if (dst == nullptr || dst.m_ptr == nullptr) {
            return nullptr;
        }

        const BYTE* srcY = src->GetReadPtr(PLANAR_Y);
        const BYTE* srcU = src->GetReadPtr(PLANAR_U);
        const BYTE* srcV = src->GetReadPtr(PLANAR_V);
        const int srcYpitch = src->GetPitch(PLANAR_Y);
        const int srcUVpitch = src->GetPitch(PLANAR_U);

        BYTE* dstY = dst->GetWritePtr(PLANAR_Y);
        BYTE* dstU = dst->GetWritePtr(PLANAR_U);
        BYTE* dstV = dst->GetWritePtr(PLANAR_V);
        if (dstY == nullptr) {
            int yy = 0;
        }
        const int dstYpitch = dst->GetPitch(PLANAR_Y);
        const int dstUVpitch = dst->GetPitch(PLANAR_U);
        if (vi.IsYV12())
        {
            libyuv::I420Scale(srcY, srcYpitch, srcU, srcUVpitch, srcV, srcUVpitch, this->child->GetVideoInfo().width, this->child->GetVideoInfo().height, dstY, dstYpitch, dstU, dstUVpitch, dstV, dstUVpitch, vi.width, vi.height, libyuv::FilterMode::kFilterBilinear);
        }
        if (vi.IsYV24())
        {
            libyuv::I444Scale(srcY, srcYpitch, srcU, srcUVpitch, srcV, srcUVpitch, this->child->GetVideoInfo().width, this->child->GetVideoInfo().height, dstY, dstYpitch, dstU, dstUVpitch, dstV, dstUVpitch, vi.width, vi.height, libyuv::FilterMode::kFilterBilinear);

        }
        else if (vi.IsRGB32())
        {
            srcY = src->GetReadPtr();
            dstY = dst->GetWritePtr();
            libyuv::ARGBScale(srcY, srcYpitch, this->child->GetVideoInfo().width, this->child->GetVideoInfo().height, dstY, dstYpitch, vi.width, vi.height, libyuv::FilterMode::kFilterBilinear);
        }
        else if (vi.IsRGB24())
        {
            srcY = src->GetReadPtr();
            dstY = dst->GetWritePtr();
            libyuv::RGBScale(srcY, srcYpitch, this->child->GetVideoInfo().width, this->child->GetVideoInfo().height, dstY, dstYpitch, vi.width, vi.height, libyuv::FilterMode::kFilterBilinear);
        }
        return dst;
    }
private:
    int width, height;
};


/**************************************
 ***** Filtered Resize - Vertical ******
 ***************************************/

FilteredResizeV::FilteredResizeV( PClip _child, double subrange_top, double subrange_height,
                                  int target_height, ResamplingFunction* func, IScriptEnvironment* env )
  : GenericVideoFilter(_child,__FUNCTION__ ), resampling_pattern(0), resampling_patternUV(0),
    yOfs(0), yOfsUV(0), pitch_gY(-1), pitch_gUV(-1)
{
	if (target_height <= 0)
	{
		target_height = vi.height;
		//Log_info("Resize: Height must be greater than 0");
	}
    //env->ThrowError("Resize: Height must be greater than 0.");

  target_height = target_height / 2 * 2;
  if (vi.IsPlanar() && !vi.IsY8()) {
    const int mask = (1 << vi.GetPlaneHeightSubsampling(PLANAR_U)) - 1;

	if (target_height & mask)
      env->ThrowError("Resize: Planar destination height must be a multiple of %d.", mask+1);
  }

  if (vi.IsRGB())
    subrange_top = vi.height - subrange_top - subrange_height;

  resampling_pattern = func->GetResamplingPatternRGB(vi.height, subrange_top, subrange_height, target_height, env);

  if (vi.IsPlanar() && !vi.IsY8()) {
    const int shift = vi.GetPlaneHeightSubsampling(PLANAR_U);
    const int div   = 1 << shift;

    resampling_patternUV = func->GetResamplingPatternRGB(
                                          vi.height      >> shift,
                                          subrange_top    / div,
                                          subrange_height / div,
                                          target_height  >> shift,
                                          env);
  }

  vi.height = target_height;

  try {
    assemblerY         = GenerateResizer(PLANAR_Y, false, env);
    assemblerY_aligned = GenerateResizer(PLANAR_Y, true, env);
    if (vi.IsPlanar() && !vi.IsY8()) {
      assemblerUV         = GenerateResizer(PLANAR_U, false, env);
      assemblerUV_aligned = GenerateResizer(PLANAR_U, true, env);
    }
  }
  catch (SoftWire::Error err) {
     env->ThrowError("Resize: SoftWire exception : %s", err.getString());
  }
}


/*******************************
 * Note on multithreading (Klaus Post, 2007):
 * GetFrame is currently not re-entrant due to dynamic code variables.
 * I have not been able to find a good solution for this
 * (pushing a struct pointer to dynamic data on to the stack is not a solution IMO).
 * We could guard the function, to avoid re-entrance.
 ******************************/


PVideoFrame __stdcall FilteredResizeV::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame src = child->GetFrame(n, env);
  if (src == nullptr || src.m_ptr == nullptr) {
      return nullptr;
  }
  PVideoFrame dst = env->NewVideoFrame(vi);
  if (dst == nullptr || dst.m_ptr == nullptr) {
      return nullptr;
  }
  src_pitch = src->GetPitch();
  dst_pitch = dst->GetPitch();
  srcp = src->GetReadPtr();
  dstp = dst->GetWritePtr();
  y = vi.height;
  int plane = vi.IsPlanar() && (!vi.IsY8()) ? 4:1;

  if (pitch_gUV != src->GetPitch(PLANAR_U)) {  // Pitch is not the same as last frame
    int shUV = src->GetHeight(PLANAR_U);
    pitch_gUV = src->GetPitch(PLANAR_U);

    if (!yOfsUV)
      yOfsUV = new int[shUV];

    for (int i = 0; i < shUV; i++)
      yOfsUV[i] = pitch_gUV * i;
  }

  if (pitch_gY != src->GetPitch(PLANAR_Y))  { // Pitch is not the same as last frame
    int sh = src->GetHeight();
    pitch_gY = src->GetPitch(PLANAR_Y);

    if (!yOfs)
      yOfs = new int[sh];

    for (int i = 0; i < sh; i++)
      yOfs[i] = pitch_gY * i;
  }

  yOfs2 = this->yOfs;

  while (plane-->0){
    switch (plane) {
      case 2:  // take V plane
        src_pitch = src->GetPitch(PLANAR_V);
        dst_pitch = dst->GetPitch(PLANAR_V);
        dstp = dst->GetWritePtr(PLANAR_V);
        srcp = src->GetReadPtr(PLANAR_V);
        y = dst->GetHeight(PLANAR_V);
        yOfs2 = this->yOfsUV;
        if (((intptr_t)srcp & 15) || (src_pitch & 15) || !assemblerUV_aligned)
          assemblerUV.Call();
        else
          assemblerUV_aligned.Call();
        break;
      case 1: // U Plane
        dstp = dst->GetWritePtr(PLANAR_U);
        srcp = src->GetReadPtr(PLANAR_U);
        y = dst->GetHeight(PLANAR_U);
        src_pitch = src->GetPitch(PLANAR_U);
        dst_pitch = dst->GetPitch(PLANAR_U);
        yOfs2 = this->yOfsUV;
        plane--; // skip case 0
        if (((intptr_t)srcp & 15) || (src_pitch & 15) || !assemblerUV_aligned)
          assemblerUV.Call();
        else
          assemblerUV_aligned.Call();
        break;
      case 3: // Y plane for planar
      case 0: // Default for interleaved
        if (((intptr_t)srcp & 15) || (src_pitch & 15) || !assemblerY_aligned)
          assemblerY.Call();
        else
          assemblerY_aligned.Call();
        break;
    }
  } // end while
  return dst;
}


FilteredResizeV::~FilteredResizeV(void)
{
  if (resampling_pattern) { av_free(resampling_pattern); resampling_pattern = 0; }
  if (resampling_patternUV) { av_free(resampling_patternUV); resampling_patternUV = 0; }
  if (yOfs) { delete[] yOfs; yOfs = 0; }
  if (yOfsUV) { delete[] yOfsUV; yOfsUV = 0; }
  assemblerY.Free();
  assemblerUV.Free();
  assemblerY_aligned.Free();
  assemblerUV_aligned.Free();
}



/***********************************
 * Dynamically Assembled Resampler
 *
 * (c) 2007, Klaus Post
 * (c) 2009, Ian Brabham
 *
 * Dynamic version of the Vertical resizer
 *
 * The Algorithm is the same, except this
 *  one is able to process 16 pixels in parallel in SSE2+, 8 pixels in MMX.
 * The inner loop filter is unrolled based on the
 *  exact filter size.
 * SSSE3 version is approximately twice as fast as original MMX, 
 * SSE2 version is approximately 60% faster than new MMX, 
 * New mmx version is approximately 55% faster than original MMX, 
 * SSSE3 PSNR is more than 67dB to MMX version using 4 taps. i.e <1 bit
 * SSE2 is bit identical with MMX.
 * align parameter indicates if source plane and pitch is 16 byte aligned for sse2+.
 * dest should always be 16 byte aligned.
 **********************************/



DynamicAssembledCode FilteredResizeV::GenerateResizer(int gen_plane, bool aligned, IScriptEnvironment* env) {
  __declspec(align(8)) static const __int64 FPround           = 0x0000200000002000; // 16384/2
  __declspec(align(8)) static const __int64 FProundSSSE3      = 0x0020002000200020; // 128/2
  __declspec(align(8)) static const __int64 UnpackByteShuffle = 0x0100010001000100; 

  Assembler x86;   // This is the class that assembles the code.
  bool ssse3 = !!(env->GetCPUFlags() & CPUF_SSSE3);  // We have one version for SSSE3 and one for plain MMX.
  bool sse3  = !!(env->GetCPUFlags() & CPUF_SSE3);   // We have specialized load routine for SSE3.
  bool sse2  = !!(env->GetCPUFlags() & CPUF_SSE2);
  bool isse  = !!(env->GetCPUFlags() & CPUF_INTEGER_SSE);

  if (aligned && !sse2) // No fast aligned version without SSE2+
    return DynamicAssembledCode();

  int xloops = 0;
  int y = vi.height;

  int* array = (gen_plane == PLANAR_U || gen_plane == PLANAR_V) ? resampling_patternUV : resampling_pattern ;
  int fir_filter_size = array[0];
  int* cur = &array[1];

  if (vi.IsPlanar()) {
    xloops = vi.width >> vi.GetPlaneWidthSubsampling(gen_plane);
    y = y >> (vi.GetPlaneHeightSubsampling(gen_plane));
  } else {
    xloops = vi.BytesFromPixels(vi.width);
  }

  if (sse2)
    xloops = ((xloops+15) / 16) * 16;
  else
    xloops = (xloops+7) / 8;


  // Store registers
  x86.push(           eax);
  x86.push(           ebx);
  x86.push(           ecx);
  x86.push(           edx);
  x86.push(           esi);
  x86.push(           edi);
  x86.push(           ebp);

  if (fir_filter_size == 1) {                 // Fast PointResize
// eax ebx ecx edx esi edi ebp
// mm0 mm1 mm2 mm3 mm4 mm5 mm6 mm7
// xmm0 XMM1 xmm2 XMM3 xmm4 XMM5 xmm6 XMM7
    x86.mov(          edx, (intptr_t)cur);              // edx = &array[1] -> start_pos
    x86.mov(          ebx, dword_ptr[(intptr_t)&dst_pitch]);
    x86.mov(          ebp, dword_ptr[(intptr_t)&dstp]);
    x86.mov(          edi, y);                     // edi = vi.height

    x86.align(16);
x86.label("yloop");
    x86.mov(          esi, dword_ptr[(intptr_t)&yOfs2]);// int pitch_table[height] = {0, src_pitch, src_pitch*2, ...}
    x86.mov(          eax, dword_ptr[edx]);        // eax = *cur = start_pos
    x86.lea(          edx, dword_ptr[edx+(fir_filter_size*4)+4]); // cur += fir_filter_size+1
    x86.mov(          esi, dword_ptr[esi+eax*4]);  // esi = yOfs[*cur] = start_pos * src_pitch
    x86.mov(          eax, ebp);                   // eax = dstp
    x86.add(          esi, dword_ptr[(intptr_t)&srcp]); // esi = srcp + yOfs[*cur] = srcp + start_pos * src_pitch
    x86.add(          ebp, ebx);                   // dstp += dst_pitch
                 
    if (sse2) {
      int ploops = xloops / 64;
      int plrem  = xloops % 64;
      if (!plrem && ploops) {
        ploops -=  1;
        plrem  += 64;
      }
      if (ploops>1) {
        x86.mov(      ecx, ploops);
        x86.align(16);
x86.label("xloop");
      }
      if (ploops) {
        if (aligned) {
          x86.movdqa( xmm0, xmmword_ptr[esi+ (intptr_t)0]);  // xmm0 = *(srcp2= srcp + x) = p|o|n|m|l|k|j|i|h|g|f|e|d|c|b|a
          x86.movdqa( xmm2, xmmword_ptr[esi+ (intptr_t)16]);
          x86.movdqa( xmm4, xmmword_ptr[esi+ (intptr_t)32]);
          x86.movdqa( xmm6, xmmword_ptr[esi+ (intptr_t)48]);
        } else if (sse3) {
          x86.lddqu(  xmm0, xmmword_ptr[esi+ (intptr_t)0]); // SSE3
          x86.lddqu(  xmm2, xmmword_ptr[esi+ (intptr_t)16]);
          x86.lddqu(  xmm4, xmmword_ptr[esi+ (intptr_t)32]);
          x86.lddqu(  xmm6, xmmword_ptr[esi+ (intptr_t)48]);
        } else {
          x86.movdqu( xmm0, xmmword_ptr[esi+ (intptr_t)0]);
          x86.movdqu( xmm2, xmmword_ptr[esi+ (intptr_t)16]);
          x86.movdqu( xmm4, xmmword_ptr[esi+ (intptr_t)32]);
          x86.movdqu( xmm6, xmmword_ptr[esi+ (intptr_t)48]);
        }
        x86.add(      esi, 64);

        x86.movdqa(   xmmword_ptr[eax+ (intptr_t)0], xmm0);   // dstp[x] = p|o|n|m|l|k|j|i|h|g|f|e|d|c|b|a
        x86.movdqa(   xmmword_ptr[eax+ (intptr_t)16], xmm2);
        x86.movdqa(   xmmword_ptr[eax+ (intptr_t)32], xmm4);
        x86.movdqa(   xmmword_ptr[eax+ (intptr_t)48], xmm6);
        x86.add(      eax, 64);
      }
      if (ploops>1)
        x86.loop(     "xloop");
      if (aligned) {
        if (plrem> 0) x86.movdqa( xmm0, xmmword_ptr[esi+ (intptr_t)0]);
        if (plrem>16) x86.movdqa( xmm2, xmmword_ptr[esi+ (intptr_t)16]);
        if (plrem>32) x86.movdqa( xmm4, xmmword_ptr[esi+ (intptr_t)32]);
        if (plrem>48) x86.movdqa( xmm6, xmmword_ptr[esi+ (intptr_t)48]);
      } else if (sse3) {
        if (plrem> 0) x86.lddqu(  xmm0, xmmword_ptr[esi+ (intptr_t)0]);
        if (plrem>16) x86.lddqu(  xmm2, xmmword_ptr[esi+ (intptr_t)16]);
        if (plrem>32) x86.lddqu(  xmm4, xmmword_ptr[esi+ (intptr_t)32]);
        if (plrem>48) x86.lddqu(  xmm6, xmmword_ptr[esi+ (intptr_t)48]);
      } else {
        if (plrem> 0) x86.movdqu( xmm0, xmmword_ptr[esi+ (intptr_t)0]);
        if (plrem>16) x86.movdqu( xmm2, xmmword_ptr[esi+ (intptr_t)16]);
        if (plrem>32) x86.movdqu( xmm4, xmmword_ptr[esi+ (intptr_t)32]);
        if (plrem>48) x86.movdqu( xmm6, xmmword_ptr[esi+ (intptr_t)48]);
      }
      if (plrem> 0) x86.movdqa(   xmmword_ptr[eax+ (intptr_t)0], xmm0);
      if (plrem>16) x86.movdqa(   xmmword_ptr[eax+ (intptr_t)16], xmm2);
      if (plrem>32) x86.movdqa(   xmmword_ptr[eax+ (intptr_t)32], xmm4);
      if (plrem>48) x86.movdqa(   xmmword_ptr[eax+ (intptr_t)48], xmm6);
    }
    else { // MMX
      int ploops = xloops / 8;
      int plrem  = xloops % 8;
      if (!plrem && ploops) {
        ploops -= 1;
        plrem  += 8;
      }
      if (ploops>1) {
        x86.mov(      ecx, ploops);
        x86.align(16);
x86.label("xloop");
      }
      if (ploops) {
        x86.movq(     mm0, qword_ptr[esi+ (intptr_t)0]);  // mm0 = *(srcp2= srcp + x) = h|g|f|e|d|c|b|a
        x86.movq(     mm1, qword_ptr[esi+ (intptr_t)8]);
        x86.movq(     mm2, qword_ptr[esi+ (intptr_t)16]);
        x86.movq(     mm3, qword_ptr[esi+ (intptr_t)24]);
        x86.movq(     mm4, qword_ptr[esi+ (intptr_t)32]);
        x86.movq(     mm5, qword_ptr[esi+ (intptr_t)40]);
        x86.movq(     mm6, qword_ptr[esi+ (intptr_t)48]);
        x86.movq(     mm7, qword_ptr[esi+ (intptr_t)56]);
        x86.add(      esi, 64);

        x86.movq(     qword_ptr[eax+ (intptr_t)0], mm0); // dstp[x] = h|g|f|e|d|c|b|a
        x86.movq(     qword_ptr[eax+ (intptr_t)8], mm1);
        x86.movq(     qword_ptr[eax+ (intptr_t)16], mm2);
        x86.movq(     qword_ptr[eax+ (intptr_t)24], mm3);
        x86.movq(     qword_ptr[eax+ (intptr_t)32], mm4);
        x86.movq(     qword_ptr[eax+ (intptr_t)40], mm5);
        x86.movq(     qword_ptr[eax+ (intptr_t)48], mm6);
        x86.movq(     qword_ptr[eax+ (intptr_t)56], mm7);
        x86.add(      eax, 64);
      }
      if (ploops>1)
        x86.loop(     "xloop");
      if (plrem>0) x86.movq( mm0, qword_ptr[esi+ (intptr_t)0]);
      if (plrem>1) x86.movq( mm1, qword_ptr[esi+ (intptr_t)8]);
      if (plrem>2) x86.movq( mm2, qword_ptr[esi+ (intptr_t)16]);
      if (plrem>3) x86.movq( mm3, qword_ptr[esi+ (intptr_t)24]);
      if (plrem>4) x86.movq( mm4, qword_ptr[esi+ (intptr_t)32]);
      if (plrem>5) x86.movq( mm5, qword_ptr[esi+ (intptr_t)40]);
      if (plrem>6) x86.movq( mm6, qword_ptr[esi+ (intptr_t)48]);
      if (plrem>7) x86.movq( mm7, qword_ptr[esi+ (intptr_t)56]);

      if (plrem>0) x86.movq( qword_ptr[eax+ (intptr_t)0], mm0);
      if (plrem>1) x86.movq( qword_ptr[eax+ (intptr_t)8], mm1);
      if (plrem>2) x86.movq( qword_ptr[eax+ (intptr_t)16], mm2);
      if (plrem>3) x86.movq( qword_ptr[eax+ (intptr_t)24], mm3);
      if (plrem>4) x86.movq( qword_ptr[eax+ (intptr_t)32], mm4);
      if (plrem>5) x86.movq( qword_ptr[eax+ (intptr_t)40], mm5);
      if (plrem>6) x86.movq( qword_ptr[eax+ (intptr_t)48], mm6);
      if (plrem>7) x86.movq( qword_ptr[eax+ (intptr_t)56], mm7);
    }
    x86.dec(        edi);                       // y -= 1
    x86.jnz(        "yloop");
  }
  else if (ssse3 && fir_filter_size <= 8) { // We will get too many rounding errors. Probably only lanczos etc
                                            // if taps parameter is large and very high downscale ratios. 
    x86.mov(          edx, (intptr_t)cur);
    x86.mov(          ebp, dword_ptr[(intptr_t)&src_pitch]);
    x86.mov(          ebx, dword_ptr[(intptr_t)&dst_pitch]);
    x86.mov(          edi, y);
    x86.movq(         xmm6, qword_ptr[(intptr_t)&FProundSSSE3]);        // Rounder for final division. Not touched!
    x86.movq(         xmm0, qword_ptr[(intptr_t)&UnpackByteShuffle]);   // Rounder for final division. Not touched!
    x86.pxor(         xmm5, xmm5);                                 // zeroes
    x86.punpcklqdq(   xmm6, xmm6);
    x86.punpcklqdq(   xmm0, xmm0);
                      
    x86.align(16);    
x86.label("yloop");  
    x86.mov(          esi, dword_ptr[(intptr_t)&yOfs2]);
    x86.mov(          eax, dword_ptr[edx]);               // eax = *cur
    x86.add(          edx, 4);                            // cur++
    x86.mov(          esi, dword_ptr[esi+eax*4]);
    x86.xor(          ecx, ecx);                          // ecx = x = 0
    x86.add(          esi, dword_ptr[(intptr_t)&srcp]);        // esi = srcp + yOfs[*cur]
    x86.movdqa(       xmm1, xmm6);                        // Init with rounder
    x86.movdqa(       xmm7, xmm6);
                      
    x86.align(16);    
x86.label("xloop");  
    x86.lea(          eax, dword_ptr[esi+ecx]);           // eax = srcp2 = srcp + x
    if (aligned) {    
      x86.movdqa(     xmm4, xmmword_ptr[eax]);            // xmm4 = p|o|n|m|l|k|j|i|h|g|f|e|d|c|b|a
    } else if (sse3) {
      x86.lddqu(      xmm4, xmmword_ptr[eax]); // SSE3
    } else {          
      x86.movdqu(     xmm4, xmmword_ptr[eax]);
    }

    for (int i = 0; i< fir_filter_size; i++) {
      x86.movd(       xmm3, dword_ptr[edx+i*4]);          // mm3 = cur[b] = 0|co
      x86.movdqa(     xmm2, xmm4);
      x86.punpckhbw(  xmm4, xmm5);                        // mm4 = *srcp2 = 0p|0o|0n|0m|0l|0k|0j|0i
      x86.punpcklbw(  xmm2, xmm5);                        // mm2 = *srcp2 = 0h|0g|0f|0e|0d|0c|0b|0a
      x86.pshufb(     xmm3, xmm0);                        // Unpack coefficient to all words
      x86.psllw(      xmm2, 7);                           // Extend to signed word
      x86.psllw(      xmm4, 7);                           // Extend to signed word
      x86.add(        eax, ebp);                          // srcp2 += src_pitch
      x86.pmulhrsw(   xmm2, xmm3); // SSSE3               // Multiply [h|g|f|e|d|c|b|a] 14bit(coeff) x 16bit(signed) ->
      x86.pmulhrsw(   xmm3, xmm4); // SSSE3               // Multiply [p|o|n|m|l|k|j|i] 16bit signed and rounded result.
      if (i<fir_filter_size-1) {                          // Load early for next loop
        if (aligned)
          x86.movdqa( xmm4, xmmword_ptr[eax]);            // xmm4 = p|o|n|m|l|k|j|i|h|g|f|e|d|c|b|a
        else if (sse3)
          x86.lddqu(  xmm4, xmmword_ptr[eax]); // SSE3
        else
          x86.movdqu( xmm4, xmmword_ptr[eax]);
      }
      x86.paddsw(     xmm1, xmm2);                        // Accumulate: h|g|f|e|d|c|b|a (signed words)
      x86.paddsw(     xmm7, xmm3);                        // Accumulate: p|o|n|m|l|k|j|i
    }
    x86.psraw(        xmm1, 6);                           // Compensate fraction
    x86.psraw(        xmm7, 6);                           // Compensate fraction
    x86.mov(          eax, dword_ptr[(intptr_t)&dstp]);
    x86.packuswb(     xmm1, xmm7);                        // mm1 = p|o|n|m|l|k|j|i|h|g|f|e|d|c|b|a
    x86.add(          ecx, 16);
    x86.movdqa(       xmm7, xmm6);
    x86.movdqa(       xmmword_ptr[eax+ecx-16], xmm1);     // Dest should always be aligned.
    x86.cmp(          ecx, xloops);
    x86.movdqa(       xmm1, xmm6);                        // Init with rounder
    x86.jl(           "xloop");

    x86.add(          eax, ebx);
    x86.lea(          edx, dword_ptr[edx+(fir_filter_size*4)]); //cur += fir_filter_size
    x86.dec(          edi);
    x86.mov(          dword_ptr[(intptr_t)&dstp], eax);
    x86.jnz(          "yloop");
  }
  else if (sse2) {
  // eax ebx ecx edx esi edi ebp
  // xmm0 xmm1 xmm2 xmm3 xmm4 xmm5 xmm6 xmm7
    x86.mov(     edx, (intptr_t)cur);                  // edx = &array[1] -> start_pos
    x86.mov(     ebp, dword_ptr[(intptr_t)&src_pitch]);
    x86.mov(     ebx, dword_ptr[(intptr_t)&dst_pitch]);
    x86.mov(     edi, y);                         // edi = vi.height

    x86.align(16);
x86.label("yloop");
    x86.mov(     esi, dword_ptr[(intptr_t)&yOfs2]);    // int pitch_table[height] = {0, src_pitch, src_pitch*2, ...}
    x86.mov(     eax, dword_ptr[edx]);            // eax = *cur = start_pos
    x86.mov(     esi, dword_ptr[esi+eax*4]);      // esi = yOfs[*cur] = start_pos * src_pitch
    x86.add(     edx, 4);                         // cur++  (*cur = coeff[0])
    x86.add(     esi, dword_ptr[(intptr_t)&srcp]);     // esi = srcp + yOfs[*cur] = srcp + start_pos * src_pitch
    x86.xor(     ecx, ecx);                       // ecx = x = 0

    x86.align(16);
x86.label("xloop");
    x86.lea(          eax,  dword_ptr[esi+ecx]);  // eax = srcp2 = srcp + x
    x86.movd(         xmm7, dword_ptr[(intptr_t)&FPround]);// Rounder for final division.
    if (aligned)
      x86.movdqa(     xmm0, xmmword_ptr[eax]);    // xmm0 = *srcp2 = p|o|n|m|l|k|j|i|h|g|f|e|d|c|b|a
    else if (sse3)
      x86.lddqu(      xmm0, xmmword_ptr[eax]); // SSE3
    else
      x86.movdqu(     xmm0, xmmword_ptr[eax]);
    x86.pshufd(       xmm7, xmm7, 0x00);          // totalPONM
    x86.movdqa(       xmm6, xmm7);                // totalLKJI
    x86.movdqa(       xmm5, xmm7);                // totalHGFE
    x86.movdqa(       xmm4, xmm7);                // totalDCBA
    int i=0;
    for ( ; i < fir_filter_size/2; i++) { // Doing row pairs
      if (aligned)
        x86.movdqa(     xmm2, xmmword_ptr[eax+ebp]);// xmm2 = *(srcp2+src_pitch) = P|O|N|M|L|K|J|I|H|G|F|E|D|C|B|A
      else if (sse3)
        x86.lddqu(      xmm2, xmmword_ptr[eax+ebp]); // SSE3
      else
        x86.movdqu(     xmm2, xmmword_ptr[eax+ebp]);

      x86.movq(       xmm3, qword_ptr[edx+i*8]);  // xmm3 = cur[b] = 00|00|--coeff[i+1]|--coeff[i]
      x86.lea(        eax,  dword_ptr[eax+ebp*2]);// srcp2 += src_pitch*2
      x86.movdqa(     xmm1, xmm0);                // xmm1                      = p|o|n|m|l|k|j|i|h|g|f|e|d|c|b|a
      x86.packssdw(   xmm3, xmm3);                // xmm3 = 00|00|00|00|CO|co|CO|co

      x86.punpcklbw(  xmm0, xmm2);                // xmm0 = Hh|Gg|Ff|Ee|Dd|Cc|Bb|Aa
      x86.punpckhbw(  xmm1, xmm2);                // xmm1 = Pp|Oo|Nn|Mm|Ll|Kk|Jj|Ii
      x86.pshufd(     xmm3, xmm3, 0x00);          // xmm3 = CO|co|CO|co|CO|co|CO|co

      x86.punpckhbw(  xmm2, xmm0);                // xmm2 = HP|hO|GN|gM|FL|fK|EJ|eI
      x86.punpcklbw(  xmm0, xmm0);                // xmm0 = DD|dd|CC|cc|BB|bb|AA|aa
      x86.psrlw(      xmm2, 8);                   // xmm2 = 0H|0h|0G|0g|0F|0f|0E|0e
      x86.psrlw(      xmm0, 8);                   // xmm0 = 0D|0d|0C|0c|0B|0b|0A|0a

      x86.pmaddwd(    xmm2, xmm3);                // xmm2 =  H*CO+h*co|G*CO+g*co|F*CO+f*co|E*CO+e*co
      x86.pmaddwd(    xmm0, xmm3);                // xmm0 =  D*CO+d*co|C*CO+c*co|B*CO+b*co|A*CO+a*co
      x86.paddd(      xmm5, xmm2);                // accumulateHGFE
      x86.paddd(      xmm4, xmm0);                // accumulateDCBA

      x86.pxor(       xmm0, xmm0);
      x86.movdqa(     xmm2, xmm1);                // xmm2 = Pp|Oo|Nn|Mm|Ll|Kk|Jj|Ii
      x86.punpcklbw(  xmm1, xmm0);                // xmm1 = 0L|0l|0K|0k|0J|0j|0I|0i
      x86.punpckhbw(  xmm2, xmm0);                // xmm2 = 0P|0p|0O|0o|0N|0n|0M|0m

      if (i*2 < fir_filter_size-2) {              // Load early for next loop
        if (aligned)
          x86.movdqa( xmm0, xmmword_ptr[eax]);    // xmm0 = *srcp2 = p|o|n|m|l|k|j|i|h|g|f|e|d|c|b|a
        else if (sse3)
          x86.lddqu(  xmm0, xmmword_ptr[eax]); // SSE3
        else
          x86.movdqu( xmm0, xmmword_ptr[eax]);
      }
      x86.pmaddwd(    xmm1, xmm3);                // xmm1 =  L*CO+l*co|K*CO+k*co|J*CO+j*co|I*CO+i*co
      x86.pmaddwd(    xmm2, xmm3);                // xmm4 =  P*CO+p*co|O*CO+o*co|N*CO+n*co|M*CO+m*co
      x86.paddd(      xmm6, xmm1);                // accumulateLKJI
      x86.paddd(      xmm7, xmm2);                // accumulatePONM
    }
    if (i*2 < fir_filter_size) { // Do last odd row
      x86.movd(       xmm3, dword_ptr[edx+i*8]);  // xmm3 = cur[b] = 00|00|00|--coeff[i]
      x86.pxor(       xmm2, xmm2);
      x86.punpckhbw(  xmm1, xmm0);                // xmm1 = p.|o.|n.|m.|l.|k.|j.|i.
      x86.pshufd(     xmm3, xmm3, 0x00);          // xmm3 = --co|--co|--co|--co
      x86.punpcklbw(  xmm0, xmm2);                // xmm0 = 0h|0g|0f|0e|0d|0c|0b|0a 
      x86.pslld(      xmm3, 16);                  // xmm3 = co|00|co|00|co|00|co|00
      x86.psrlw(      xmm1, 8);                   // xmm1 = 0p|0o|0n|0m|0l|0k|0j|0i
      x86.punpckhwd(  xmm2, xmm0);                // xmm2 = 0h|..|0g|..|0f|..|0e|..
      x86.punpcklwd(  xmm0, xmm0);                // xmm0 = 0d|0d|0c|0c|0b|0b|0a|0a
      x86.pmaddwd(    xmm2, xmm3);                // xmm2 =  h*co|g*co|f*co|e*co
      x86.pmaddwd(    xmm0, xmm3);                // xmm0 =  d*co|c*co|b*co|a*co
      x86.paddd(      xmm5, xmm2);                // accumulateHGFE
      x86.paddd(      xmm4, xmm0);                // accumulateDCBA
      x86.punpckhwd(  xmm2, xmm1);                // xmm2 = 0p|..|0o|..|0n|..|0m|..
      x86.punpcklwd(  xmm1, xmm1);                // xmm1 = 0l|0l|0k|0k|0j|0j|0i|0i
      x86.pmaddwd(    xmm2, xmm3);                // xmm4 =  p*co|o*co|n*co|m*co
      x86.pmaddwd(    xmm1, xmm3);                // xmm1 =  l*co|k*co|j*co|i*co
      x86.paddd(      xmm7, xmm2);                // accumulatePONM
      x86.paddd(      xmm6, xmm1);                // accumulateLKJI
    }
    x86.psrad(        xmm4, 14);                  // 14 bits -> 16bit fraction [--FF....|--FF....]
    x86.psrad(        xmm5, 14);                  // compensate the fact that FPScale = 16384
    x86.psrad(        xmm6, 14);
    x86.psrad(        xmm7, 14);
    x86.packssdw(     xmm4, xmm5);                // xmm4 = 0h|0g|0f|0e|0d|0c|0b|0a
    x86.mov(          eax, dword_ptr[(intptr_t)&dstp]);
    x86.packssdw(     xmm6, xmm7);                // xmm6 = 0p|0o|0n|0m|0l|0k|0j|0i
    x86.add(          ecx, 16);                   // x += 16
    x86.packuswb(     xmm4, xmm6);                // xmm4 = p|o|n|m|l|k|j|i|h|g|f|e|d|c|b|a
    x86.cmp(          ecx, xloops);
    x86.movdqa(       xmmword_ptr[eax+ecx-16], xmm4); // dstp[x] = p|o|n|m|l|k|j|i|h|g|f|e|d|c|b|a
    x86.jnz(          "xloop");
                     
    x86.lea(          edx, dword_ptr[edx+(fir_filter_size*4)]); // cur += fir_filter_size
    x86.add(          eax, ebx);                  // dstp += dst_pitch
    x86.dec(          edi);                       // y -= 1
    x86.mov(          dword_ptr[(intptr_t)&dstp], eax);
    x86.jnz(          "yloop");
  }
  else {
  // eax ebx ecx edx esi edi ebp
  // mm0 mm1 mm2 mm3 mm4 mm5 mm6 mm7
    x86.mov(        edx, (intptr_t)cur);              // edx = &array[1] -> start_pos
    x86.mov(        ebp, dword_ptr[(intptr_t)&src_pitch]);
    x86.mov(        ebx, dword_ptr[(intptr_t)&dst_pitch]);
    x86.mov(        edi, y);                     // edi = vi.height

    x86.align(16);
x86.label("yloop");
    x86.mov(        esi, dword_ptr[(intptr_t)&yOfs2]);// int pitch_table[height] = {0, src_pitch, src_pitch*2, ...}
    x86.mov(        eax, dword_ptr[edx]);        // eax = *cur = start_pos
    x86.mov(        esi, dword_ptr[esi+eax*4]);  // esi = yOfs[*cur] = start_pos * src_pitch
    x86.add(        edx, 4);                     // cur++  (*cur = coeff[0])
    x86.add(        esi, dword_ptr[(intptr_t)&srcp]); // esi = srcp + yOfs[*cur] = srcp + start_pos * src_pitch
    x86.xor(        ecx, ecx);                   // ecx = x = 0

    x86.align(16);
x86.label("xloop");
    x86.lea(        eax, qword_ptr[esi+ecx*8]);  // eax = srcp2 = srcp + x
    x86.movq(       mm7, qword_ptr[(intptr_t)&FPround]);// totalHG = Rounder for final division.
    x86.movq(       mm0, qword_ptr[eax]);        // mm0 = *srcp2             = h|g|f|e|d|c|b|a
    x86.movq(       mm6, mm7);                   // totalFE
    x86.movq(       mm5, mm7);                   // totalDC
    x86.movq(       mm4, mm7);                   // totalBA
    int i=0;
    for ( ; i < fir_filter_size/2; i++) { // Doing row pairs
      x86.movq(       mm2, qword_ptr[eax+ebp]);  // mm2 = *(srcp2+src_pitch) = H|G|F|E|D|C|B|A
      x86.movq(       mm3, qword_ptr[edx+i*8]);  // mm3 = cur[b] = --coeff[i+1]|--coeff[i]
      x86.lea(        eax, qword_ptr[eax+ebp*2]);// srcp2 += src_pitch*2
      x86.movq(       mm1, mm0);                 // mm1                      = h|g|f|e|d|c|b|a
      x86.packssdw(   mm3, mm3);                 // mm3 = CO|co|CO|co

      x86.punpcklbw(  mm0, mm2);                 // mm0 = Dd|Cc|Bb|Aa
      x86.punpckhbw(  mm1, mm2);                 // mm1 = Hh|Gg|Ff|Ee

      x86.punpckhbw(  mm2, mm0);                 // mm2 = DH|dG|CF|cE
      x86.punpcklbw(  mm0, mm0);                 // mm0 = BB|bb|AA|aa
      x86.psrlw(      mm2, 8);                   // mm2 = 0D|0d|0C|0c
      x86.psrlw(      mm0, 8);                   // mm0 = 0B|0b|0A|0a

      x86.pmaddwd(    mm2, mm3);                 // mm2 = D*CO+d*co|C*CO+c*co
      x86.pmaddwd(    mm0, mm3);                 // mm0 = B*CO+b*co|A*CO+a*co
      x86.paddd(      mm5, mm2);                 // accumulateDC
      x86.paddd(      mm4, mm0);                 // accumulateBA

      x86.pxor(       mm0, mm0);
      x86.movq(       mm2, mm1);                 // mm2 = Hh|Gg|Ff|Ee
      x86.punpcklbw(  mm1, mm0);                 // mm1 = 0F|0f|0E|0e
      x86.punpckhbw(  mm2, mm0);                 // mm2 = 0H|0h|0G|0g

      if (i*2 < fir_filter_size-2)               // Load early for next loop
        x86.movq(     mm0, qword_ptr[eax]);      // mm0 = *srcp2             = h|g|f|e|d|c|b|a

      x86.pmaddwd(    mm1, mm3);                 // mm1 = F*CO+f*co|E*CO+e*co
      x86.pmaddwd(    mm2, mm3);                 // mm2 = H*CO+h*co|G*CO+g*co
      x86.paddd(      mm6, mm1);                 // accumulateFE
      x86.paddd(      mm7, mm2);                 // accumulateHG
    }
    if (i*2 < fir_filter_size) { // Do last odd row
      x86.movd(       mm3, dword_ptr[edx+i*8]);  // mm3 = cur[b] = 0|--coeff[i]
      x86.pxor(       mm2, mm2);
      x86.punpckhbw(  mm1, mm0);                 // mm1 = h.|g.|f.|e.
      if (isse) {
        x86.punpcklbw(mm0, mm2);                 // mm0 = 0d|0c|0b|0a
        x86.pshufw(   mm3, mm3, 0x33);           // mm3 = co|00|co|00
      } else {
        x86.punpckldq(mm3, mm3);                 // mm3 = --co|--co
        x86.punpcklbw(mm0, mm2);                 // mm0 = 0d|0c|0b|0a
        x86.pslld(    mm3, 16);                  // mm3 = co|00|co|00
      }
      x86.psrlw(      mm1, 8);                   // mm1 = 0h|0g|0f|0e
      x86.punpckhwd(  mm2, mm0);                 // mm2 = 0d|..|0c|..
      x86.punpcklwd(  mm0, mm0);                 // mm0 = 0b|0b|0a|0a
      x86.pmaddwd(    mm2, mm3);                 // mm2 =  d*co|c*co
      x86.pmaddwd(    mm0, mm3);                 // mm0 =  b*co|a*co
      x86.paddd(      mm5, mm2);                 // accumulateDC
      x86.paddd(      mm4, mm0);                 // accumulateBA
      x86.punpckhwd(  mm2, mm1);                 // mm2 = 0h|..|0g|..
      x86.punpcklwd(  mm1, mm1);                 // mm1 = 0f|0f|0e|0e
      x86.pmaddwd(    mm2, mm3);                 // mm2 =  h*co|g*co
      x86.pmaddwd(    mm1, mm3);                 // mm1 =  f*co|e*co
      x86.paddd(      mm7, mm2);                 // accumulateHG
      x86.paddd(      mm6, mm1);                 // accumulateFE
    }
    x86.psrad(      mm4, 14);                    // 14 bits -> 16bit fraction [--FF....|--FF....]
    x86.psrad(      mm5, 14);                    // compensate the fact that FPScale = 16384
    x86.psrad(      mm6, 14);
    x86.psrad(      mm7, 14);
    x86.packssdw(   mm4, mm5);                   // mm4 = 0d|0c|0b|0a
    x86.mov(        eax, dword_ptr[(intptr_t)&dstp]);
    x86.packssdw(   mm6, mm7);                   // mm6 = 0h|0g|0f|0e
    x86.inc(        ecx);                        // x += 1
    x86.packuswb(   mm4, mm6);                   // mm4 = h|g|f|e|d|c|b|a
    x86.cmp(        ecx, xloops);
    x86.movq(       qword_ptr[eax+ecx*8-8], mm4); // dstp[x] = h|g|f|e|d|c|b|a
    x86.jnz(        "xloop");

    x86.lea(        edx, dword_ptr[edx+(fir_filter_size*4)]);        // cur += fir_filter_size
    x86.add(        eax, ebx);                   // dstp += dst_pitch
    x86.dec(        edi);                        // y -= 1
    x86.mov(        dword_ptr[(intptr_t)&dstp], eax);
    x86.jnz(        "yloop");
  }
  // No more mmx for now
  x86.emms();
  // Restore registers
  x86.pop(          ebp);
  x86.pop(          edi);
  x86.pop(          esi);
  x86.pop(          edx);
  x86.pop(          ecx);
  x86.pop(          ebx);
  x86.pop(          eax);
  x86.ret();

  return DynamicAssembledCode(x86, env, "ResizeV: Dynamic code could not be compiled.");
}


PClip FilteredResize::CreateResize(PClip clip, int target_width, int target_height,IScriptEnvironment* env)
{

  const VideoInfo& vi = clip->GetVideoInfo();
  if (vi.IsYV12()|| vi.IsYV24() || vi.IsRGB32() || vi.IsRGB24())
  {
      return new Resizer(clip, target_width, target_height, env);
  }
  return 0;
}

AVSValue __cdecl FilteredResize::Create_PointResize(AVSValue args, void*, IScriptEnvironment* env)
{
    return CreateResize(args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), env);
}



AVSValue __cdecl FilteredResize::Create_TinyResize(AVSValue args, void*, IScriptEnvironment* env)
{

    return CreateResize(args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), env);
}

AVSValue __cdecl FilteredResize::Create_BilinearResize(AVSValue args, void*, IScriptEnvironment* env)
{
    return CreateResize(args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), env);
}








