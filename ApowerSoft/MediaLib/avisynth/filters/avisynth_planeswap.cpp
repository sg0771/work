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


// Avisynth filter:  Swap planes
// by Klaus Post
// adapted by Richard Berg (avisynth-dev@richardberg.net)
// iSSE code by Ian Brabham


#include "avisynth/avisynth_stdafx.h"

#include "planeswap.h"


AVSValue __cdecl SwapUVToY::CreateUToY(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return new SwapUVToY(args[0].AsClip(), UToY, env);
}

AVSValue __cdecl SwapUVToY::CreateUToY8(AVSValue args, void* user_data, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();
  return new SwapUVToY(clip, (clip->GetVideoInfo().IsYUY2()) ? YUY2UToY8 : UToY8, env);
}

AVSValue __cdecl SwapUVToY::CreateVToY(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return new SwapUVToY(args[0].AsClip(), VToY, env);
}

AVSValue __cdecl SwapUVToY::CreateVToY8(AVSValue args, void* user_data, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();
  return new SwapUVToY(clip, (clip->GetVideoInfo().IsYUY2()) ? YUY2VToY8 : VToY8, env);
}

SwapUVToY::SwapUVToY(PClip _child, int _mode, IScriptEnvironment* env)
  : GenericVideoFilter(_child,__FUNCTION__ ), mode(_mode) {

  if (!vi.IsYUV())
    env->ThrowError("UVtoY: YUV data only!");

  if (vi.IsY8()) 
    env->ThrowError("UVtoY: There are no chroma channels in Y8!");

  vi.height >>= vi.GetPlaneHeightSubsampling(PLANAR_U);
  vi.width  >>= vi.GetPlaneWidthSubsampling(PLANAR_U);

  if (mode == UToY8 || mode == VToY8 || mode == YUY2UToY8 || mode == YUY2VToY8)
    vi.pixel_type = CS_Y8;

}


PVideoFrame __stdcall SwapUVToY::GetFrame(int n, IScriptEnvironment* env) {
  PVideoFrame src = child->GetFrame(n, env);

  if (src == nullptr || src.m_ptr == nullptr) {
      return nullptr;
  }
  if (vi.IsYUY2()) {  // YUY2 interleaved
    PVideoFrame dst = env->NewVideoFrame(vi);
    if (dst == nullptr || dst.m_ptr == nullptr) {
        return nullptr;
    }
    const BYTE* srcp = src->GetReadPtr();
    short* dstp = (short*)dst->GetWritePtr();
    const int srcpitch = src->GetPitch();
    const int dstpitch = dst->GetPitch()>>1;
    const int endx = dst->GetRowSize()>>1;
    if (mode==UToY) {
      for (int y=0; y<vi.height; y++) {
        for (int x = 0; x < endx; x+=2) {
          dstp[x  ] = 0x8000 | srcp[x*4+1];
          dstp[x+1] = 0x8000 | srcp[x*4+5];
        }
        srcp += srcpitch;
        dstp += dstpitch;
      }
    }
    else if (mode==VToY) {
      for (int y=0; y<vi.height; y++) {
        for (int x = 0; x < endx; x+=2) {
          dstp[x  ] = 0x8000 | srcp[x*4+3];
          dstp[x+1] = 0x8000 | srcp[x*4+7];
        }
        srcp += srcpitch;
        dstp += dstpitch;
      }
    }
    return dst;
  }

  // Planar

  if (mode==UToY8) {
    const int offset = src->GetOffset(PLANAR_U) - src->GetOffset(PLANAR_Y); // very naughty - don't do this at home!!
    // Abuse Subframe to snatch the U plane
    return env->Subframe(src, offset, src->GetPitch(PLANAR_U), src->GetRowSize(PLANAR_U), src->GetHeight(PLANAR_U));
  }
  else if (mode == VToY8) {
    const int offset = src->GetOffset(PLANAR_V) - src->GetOffset(PLANAR_Y); // very naughty - don't do this at home!!
    // Abuse Subframe to snatch the V plane
    return env->Subframe(src, offset, src->GetPitch(PLANAR_V), src->GetRowSize(PLANAR_V), src->GetHeight(PLANAR_V));
  }

  PVideoFrame dst = env->NewVideoFrame(vi);
  if (dst == nullptr || dst.m_ptr == nullptr) {
      return nullptr;
  }

  if (mode==YUY2UToY8 || mode==YUY2VToY8) {  // YUY2 U To Y
    const BYTE* srcp = src->GetReadPtr();
    BYTE* dstp = (BYTE*)dst->GetWritePtr(PLANAR_Y);
    srcp += (mode==YUY2UToY8) ? 1 : 3;
    for (int y=0; y<vi.height; y++) {
      for (int x = 0; x < vi.width; x++) {
        dstp[x] = srcp[(x<<2)];
      }
      srcp += src->GetPitch();
      dstp += dst->GetPitch(PLANAR_Y);
    }      
    return dst;
  }

  if (mode==UToY) {
    env->BitBlt(dst->GetWritePtr(PLANAR_Y), dst->GetPitch(PLANAR_Y),
                src->GetReadPtr(PLANAR_U), src->GetPitch(PLANAR_U), dst->GetRowSize(), dst->GetHeight());
  }
  else if (mode==VToY) {
    env->BitBlt(dst->GetWritePtr(PLANAR_Y), dst->GetPitch(PLANAR_Y),
                src->GetReadPtr(PLANAR_V), src->GetPitch(PLANAR_V), dst->GetRowSize(), dst->GetHeight());
  }

  // Clear chroma
  const int pitch = dst->GetPitch(PLANAR_U)/4;
  const int myx = (dst->GetRowSize(PLANAR_U)+3)/4;
  const int myy = dst->GetHeight(PLANAR_U);

  int *srcpUV = (int*)dst->GetWritePtr(PLANAR_U);
  {for (int y=0; y<myy; y++) {
    for (int x=0; x<myx; x++) {
      srcpUV[x] = 0x80808080;  // mod 8
    }
    srcpUV += pitch;
  }}

  srcpUV = (int*)dst->GetWritePtr(PLANAR_V);
  {for (int y=0; y<myy; ++y) {
    for (int x=0; x<myx; x++) {
      srcpUV[x] = 0x80808080;  // mod 8
    }
    srcpUV += pitch;
  }}
  return dst;
}



