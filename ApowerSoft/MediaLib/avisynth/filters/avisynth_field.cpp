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


#include "avisynth/avisynth_stdafx.h"

#include "field.h"
#include "resample.h"




/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

extern const AVSFunction Field_filters[] = {
  { "ComplementParity", "c", ComplementParity::Create },
  { "AssumeTFF", "c", AssumeParity::Create, (void*)true },
  { "AssumeBFF", "c", AssumeParity::Create, (void*)false },
  { "AssumeFieldBased", "c", AssumeFieldBased::Create },
  { "AssumeFrameBased", "c", AssumeFrameBased::Create },
  { "SeparateColumns", "ci", SeparateColumns::Create },
  { "WeaveColumns", "ci", WeaveColumns::Create },
  { "SeparateRows", "ci", SeparateRows::Create },
  { "WeaveRows", "ci", WeaveRows::Create },
  { "SeparateFields", "c", SeparateFields::Create },
  { "Weave", "c", Create_Weave },
  { "DoubleWeave", "c", Create_DoubleWeave },
  { "Pulldown", "cii", Create_Pulldown },
  { "SelectEvery", "cii*", SelectEvery::Create },
  { "SelectEven", "c", SelectEvery::Create_SelectEven },
  { "SelectOdd", "c", SelectEvery::Create_SelectOdd },
  { "Interleave", "c+", Interleave::Create },
  { "SwapFields", "c", Create_SwapFields },
  { "SelectRangeEvery", "c[every]i[length]i[offset]i[audio]b", SelectRangeEvery::Create},
  { 0 }
};





/*********************************
 *******   SeparateColumns  ******
 *********************************/

SeparateColumns::SeparateColumns(PClip _child, int _interval, IScriptEnvironment* env)
 : GenericVideoFilter(_child,__FUNCTION__ ), interval(_interval)
{
  if (_interval <= 0)
    env->ThrowError("SeparateColumns: interval must be greater than zero.");

  if (_interval > vi.width)
    env->ThrowError("SeparateColumns: interval must be less than or equal width.");

  if (vi.width % _interval)
    env->ThrowError("SeparateColumns: width must be mod %d.", _interval);

  vi.width /= _interval;
  vi.MulDivFPS(_interval, 1);
  vi.num_frames *= _interval;

  if (vi.num_frames < 0)
    env->ThrowError("SeparateColumns: Maximum number of frames exceeded.");


  if (vi.IsYUY2() && vi.width & 1)
    env->ThrowError("SeparateColumns: YUY2 output width must be even.");
  if (vi.IsYV12() && vi.width & 1)
    env->ThrowError("SeparateColumns: YV12 output width must be even.");
  if (vi.IsYV16() && vi.width & 1)
    env->ThrowError("SeparateColumns: YV16 output width must be even.");
  if (vi.IsYV411() && vi.width & 3)
    env->ThrowError("SeparateColumns: YV411 output width must be mod 4.");
}


PVideoFrame SeparateColumns::GetFrame(int n, IScriptEnvironment* env) 
{
  const int m = n%interval;
  const int f = n/interval;

  PVideoFrame src = child->GetFrame(f, env);
  if (src == nullptr || src.m_ptr == nullptr) {
      return nullptr;
  }
  PVideoFrame dst = env->NewVideoFrame(vi);
  if (dst == nullptr || dst.m_ptr == nullptr) {
      return nullptr;
  }

  if (vi.IsPlanar()) {
    const int srcpitchY = src->GetPitch(PLANAR_Y);
    const int dstpitchY = dst->GetPitch(PLANAR_Y);
    const int heightY = dst->GetHeight(PLANAR_Y);
    const int rowsizeY = dst->GetRowSize(PLANAR_Y);

    const BYTE* srcpY = src->GetReadPtr(PLANAR_Y);
    BYTE* dstpY = dst->GetWritePtr(PLANAR_Y);

    for (int yY=0; yY<heightY; yY+=1) {
      for (int i=m, j=0; j<rowsizeY; i+=interval, j+=1) {
        dstpY[j] = srcpY[i];
      }
      srcpY += srcpitchY;
      dstpY += dstpitchY;
    }

    const int srcpitchUV = src->GetPitch(PLANAR_U);
    const int dstpitchUV = dst->GetPitch(PLANAR_U);
    const int heightUV = dst->GetHeight(PLANAR_U);
    const int rowsizeUV = dst->GetRowSize(PLANAR_U);

    if (dstpitchUV) {
      const BYTE* srcpV = src->GetReadPtr(PLANAR_V);
      BYTE* dstpV = dst->GetWritePtr(PLANAR_V);

      for (int yV=0; yV<heightUV; yV+=1) {
        for (int i=m, j=0; j<rowsizeUV; i+=interval, j+=1) {
          dstpV[j] = srcpV[i];
        }
        srcpV += srcpitchUV;
        dstpV += dstpitchUV;
      }

      const BYTE* srcpU = src->GetReadPtr(PLANAR_U);
      BYTE* dstpU = dst->GetWritePtr(PLANAR_U);

      for (int yU=0; yU<heightUV; yU+=1) {
        for (int i=m, j=0; j<rowsizeUV; i+=interval, j+=1) {
          dstpU[j] = srcpU[i];
        }
        srcpU += srcpitchUV;
        dstpU += dstpitchUV;
      }
    }
  }
  else if (vi.IsYUY2()) {
    const int srcpitch = src->GetPitch();
    const int dstpitch = dst->GetPitch();
    const int height = dst->GetHeight();
    const int rowsize = dst->GetRowSize();

    const BYTE* srcp = src->GetReadPtr();
    BYTE* dstp = dst->GetWritePtr();

    const int m2 = m*2;
    const int interval2 = interval*2;
    const int interval4 = interval*4;

    for (int y=0; y<height; y+=1) {
      for (int i=m2, j=0; j<rowsize; i+=interval4, j+=4) {
        // Luma
        dstp[j+0] = srcp[i+0];
        dstp[j+2] = srcp[i+interval2];
        // Chroma
        dstp[j+1] = srcp[i+m2+1];
        dstp[j+3] = srcp[i+m2+3];
      }
      srcp += srcpitch;
      dstp += dstpitch;
    }
  }
  else if (vi.IsRGB24()) {
    const int srcpitch = src->GetPitch();
    const int dstpitch = dst->GetPitch();
    const int height = dst->GetHeight();
    const int rowsize = dst->GetRowSize();

    const BYTE* srcp = src->GetReadPtr();
    BYTE* dstp = dst->GetWritePtr();

    const int m3 = m*3;
    const int interval3 = interval*3;

    for (int y=0; y<height; y+=1) {
      for (int i=m3, j=0; j<rowsize; i+=interval3, j+=3) {
        dstp[j+0] = srcp[i+0];
        dstp[j+1] = srcp[i+1];
        dstp[j+2] = srcp[i+2];
      }
      srcp += srcpitch;
      dstp += dstpitch;
    }
  }
  else if (vi.IsRGB32()) {
    const int srcpitch4 = src->GetPitch()>>2;
    const int dstpitch4 = dst->GetPitch()>>2;
    const int height = dst->GetHeight();
    const int rowsize4 = dst->GetRowSize()>>2;

    const int* srcp4 = (const int*)src->GetReadPtr();
    int* dstp4 = (int*)dst->GetWritePtr();

    for (int y=0; y<height; y+=1) {
      for (int i=m, j=0; j<rowsize4; i+=interval, j+=1) {
        dstp4[j] = srcp4[i];
      }
      srcp4 += srcpitch4;
      dstp4 += dstpitch4;
    }
  }
  return dst;
}


AVSValue __cdecl SeparateColumns::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  if (args[1].AsInt() == 1)
    return args[0];

  return new SeparateColumns(args[0].AsClip(), args[1].AsInt(), env);
}







/*******************************
 *******   WeaveColumns   ******
 *******************************/

WeaveColumns::WeaveColumns(PClip _child, int _period, IScriptEnvironment* env)
 : GenericVideoFilter(_child,__FUNCTION__ ), period(_period), inframes(vi.num_frames)
{
  if (_period <= 0)
    env->ThrowError("WeaveColumns: period must be greater than zero.");

  vi.width *= _period;
  vi.MulDivFPS(1, _period);
  vi.num_frames += _period-1; // Ceil!
  vi.num_frames /= _period;
}


PVideoFrame WeaveColumns::GetFrame(int n, IScriptEnvironment* env) 
{
  const int b = n * period;

  PVideoFrame dst = env->NewVideoFrame(vi);
  if (dst == nullptr || dst.m_ptr == nullptr) {
      return nullptr;
  }
  BYTE *_dstp = dst->GetWritePtr();
  const int dstpitch = dst->GetPitch();
  BYTE *_dstpU = dst->GetWritePtr(PLANAR_U);
  BYTE *_dstpV = dst->GetWritePtr(PLANAR_V);
  const int dstpitchUV = dst->GetPitch(PLANAR_U);

  for (int m=0; m<period; m++) {
    const int f = b+m < inframes ? b+m : inframes-1;
    PVideoFrame src = child->GetFrame(f, env);

    if (src == nullptr || src.m_ptr == nullptr) {
        return dst;
    }
    if (vi.IsPlanar()) {
      const int srcpitchY = src->GetPitch(PLANAR_Y);
      const int heightY = src->GetHeight(PLANAR_Y);
      const int rowsizeY = src->GetRowSize(PLANAR_Y);
      const BYTE* srcpY = src->GetReadPtr(PLANAR_Y);
      BYTE* dstpY = _dstp;

      for (int yY=0; yY<heightY; yY+=1) {
        for (int i=m, j=0; j<rowsizeY; i+=period, j+=1) {
          dstpY[i] = srcpY[j];
        }
        srcpY += srcpitchY;
        dstpY += dstpitch;
      }

      if (dstpitchUV) {
        const int srcpitchUV = src->GetPitch(PLANAR_U);
        const int heightUV = src->GetHeight(PLANAR_U);
        const int rowsizeUV = src->GetRowSize(PLANAR_U);

        const BYTE* srcpU = src->GetReadPtr(PLANAR_U);
        BYTE* dstpU = _dstpU;

        for (int yU=0; yU<heightUV; yU+=1) {
          for (int i=m, j=0; j<rowsizeUV; i+=period, j+=1) {
            dstpU[i] = srcpU[j];
          }
          srcpU += srcpitchUV;
          dstpU += dstpitchUV;
        }

        const BYTE* srcpV = src->GetReadPtr(PLANAR_V);
        BYTE* dstpV = _dstpV;

        for (int yV=0; yV<heightUV; yV+=1) {
          for (int i=m, j=0; j<rowsizeUV; i+=period, j+=1) {
            dstpV[i] = srcpV[j];
          }
          srcpV += srcpitchUV;
          dstpV += dstpitchUV;
        }
      }
    }
    else if (vi.IsYUY2()) {
      const int srcpitch = src->GetPitch();
      const int height = src->GetHeight();
      const int rowsize = src->GetRowSize();
      const BYTE* srcp = src->GetReadPtr();
      BYTE* dstp = _dstp;
      const int m2 = m*2;
      const int period2 = period*2;
      const int period4 = period*4;

      for (int y=0; y<height; y+=1) {
        for (int i=m2, j=0; j<rowsize; i+=period4, j+=4) {
          // Luma
          dstp[i+0]       = srcp[j+0];
          dstp[i+period2] = srcp[j+2];
          // Chroma
          dstp[i+m2+1]    = srcp[j+1];
          dstp[i+m2+3]    = srcp[j+3];
        }
        srcp += srcpitch;
        dstp += dstpitch;
      }
    }
    else if (vi.IsRGB24()) {
      const int srcpitch = src->GetPitch();
      const int height = src->GetHeight();
      const int rowsize = src->GetRowSize();
      const BYTE* srcp = src->GetReadPtr();
      BYTE* dstp = _dstp;
      const int m3 = m*3;
      const int period3 = period*3;

      for (int y=0; y<height; y+=1) {
        for (int i=m3, j=0; j<rowsize; i+=period3, j+=3) {
          dstp[i+0] = srcp[j+0];
          dstp[i+1] = srcp[j+1];
          dstp[i+2] = srcp[j+2];
        }
        srcp += srcpitch;
        dstp += dstpitch;
      }
    }
    else if (vi.IsRGB32()) {
      const int srcpitch4 = src->GetPitch()>>2;
      const int dstpitch4 = dstpitch>>2;
      const int height = src->GetHeight();
      const int rowsize4 = src->GetRowSize()>>2;
      const int* srcp4 = (const int*)src->GetReadPtr();
      int* dstp4 = (int*)_dstp;

      for (int y=0; y<height; y+=1) {
        for (int i=m, j=0; j<rowsize4; i+=period, j+=1) {
          dstp4[i] = srcp4[j];
        }
        srcp4 += srcpitch4;
        dstp4 += dstpitch4;
      }
    }
  }
  return dst;
}


AVSValue __cdecl WeaveColumns::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  if (args[1].AsInt() == 1)
    return args[0];

  return new WeaveColumns(args[0].AsClip(), args[1].AsInt(), env);
}







/*********************************
 *******   SeparateRows   ******
 *********************************/

SeparateRows::SeparateRows(PClip _child, int _interval, IScriptEnvironment* env)
 : GenericVideoFilter(_child,__FUNCTION__ ), interval(_interval)
{
  if (_interval <= 0)
    env->ThrowError("SeparateRows: interval must be greater than zero.");

  if (_interval > vi.height)
    env->ThrowError("SeparateRows: interval must be less than or equal height.");

  if (vi.height % _interval)
    env->ThrowError("SeparateRows: height must be mod %d.", _interval);

  vi.height /= _interval;
  vi.MulDivFPS(_interval, 1);
  vi.num_frames *= _interval;

  if (vi.num_frames < 0)
    env->ThrowError("SeparateRows: Maximum number of frames exceeded.");

  if (vi.IsYV12() && vi.height & 1)
    env->ThrowError("SeparateRows: YV12 output height must be even.");
}


PVideoFrame SeparateRows::GetFrame(int n, IScriptEnvironment* env) 
{
  const int m = vi.IsRGB() ? interval-1 - n%interval : n%interval; // RGB upsidedown
  const int f = n/interval;

  PVideoFrame frame = child->GetFrame(f, env);

  if (vi.IsPlanar() && !vi.IsY8()) {
    const int Ypitch   = frame->GetPitch(PLANAR_Y);
    const int UVpitch  = frame->GetPitch(PLANAR_U);
    const int Yoffset  = Ypitch  * m;
    const int UVoffset = UVpitch * m;

    return env->SubframePlanar(frame, Yoffset, Ypitch * interval,
                               frame->GetRowSize(PLANAR_Y), vi.height,
                               UVoffset, UVoffset, UVpitch * interval);
  }
  const int pitch = frame->GetPitch();
  return env->Subframe(frame, pitch * m, pitch * interval, frame->GetRowSize(), vi.height);  
}


AVSValue __cdecl SeparateRows::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  if (args[1].AsInt() == 1)
    return args[0];

  return new SeparateRows(args[0].AsClip(), args[1].AsInt(), env);
}







/****************************
 *******   WeaveRows   ******
 ****************************/

WeaveRows::WeaveRows(PClip _child, int _period, IScriptEnvironment* env)
 : GenericVideoFilter(_child,__FUNCTION__ ), period(_period), inframes(vi.num_frames)
{
  if (_period <= 0)
    env->ThrowError("WeaveRows: period must be greater than zero.");

  vi.height *= _period;
  vi.MulDivFPS(1, _period);
  vi.num_frames += _period-1; // Ceil!
  vi.num_frames /= _period;
}


PVideoFrame WeaveRows::GetFrame(int n, IScriptEnvironment* env) 
{
  const int b = n * period;
  const int e = b + period;

  PVideoFrame dst = env->NewVideoFrame(vi);
  BYTE *dstp = dst->GetWritePtr();
  const int dstpitch = dst->GetPitch();

  if (vi.IsRGB()) { // RGB upsidedown
    dstp += dstpitch * period;
    for (int i=b; i<e; i++) {
      dstp -= dstpitch;
      const int j = i < inframes ? i : inframes-1;
      PVideoFrame src = child->GetFrame(j, env);
      BitBlt( dstp,              dstpitch * period,
              src->GetReadPtr(), src->GetPitch(),
              src->GetRowSize(), src->GetHeight() );
    }
  }
  else {
    BYTE *dstpU = dst->GetWritePtr(PLANAR_U);
    BYTE *dstpV = dst->GetWritePtr(PLANAR_V);
    const int dstpitchUV = dst->GetPitch(PLANAR_U);
    for (int i=b; i<e; i++) {
      const int j = i < inframes ? i : inframes-1;
      PVideoFrame src = child->GetFrame(j, env);
      BitBlt( dstp,              dstpitch * period,
              src->GetReadPtr(), src->GetPitch(),
              src->GetRowSize(), src->GetHeight() );
      dstp += dstpitch;
      if (dstpitchUV) {
        BitBlt( dstpU,                     dstpitchUV * period,
                src->GetReadPtr(PLANAR_U), src->GetPitch(PLANAR_U),
                src->GetRowSize(PLANAR_U), src->GetHeight(PLANAR_U) );
        BitBlt( dstpV,                     dstpitchUV * period,
                src->GetReadPtr(PLANAR_V), src->GetPitch(PLANAR_V),
                src->GetRowSize(PLANAR_V), src->GetHeight(PLANAR_V) );
        dstpU += dstpitchUV;
        dstpV += dstpitchUV;
      }
    }
  }
  return dst;
}


AVSValue __cdecl WeaveRows::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  if (args[1].AsInt() == 1)
    return args[0];

  return new WeaveRows(args[0].AsClip(), args[1].AsInt(), env);
}







/*********************************
 *******   SeparateFields   ******
 *********************************/

SeparateFields::SeparateFields(PClip _child, IScriptEnvironment* env)
 : GenericVideoFilter(_child,__FUNCTION__ )
{
  if (vi.height & 1)
    env->ThrowError("SeparateFields: height must be even");
  if (vi.IsYV12() && vi.height & 3)
    env->ThrowError("SeparateFields: YV12 height must be multiple of 4");
  vi.height >>= 1;
  vi.MulDivFPS(2, 1);
  vi.num_frames *= 2;

  if (vi.num_frames < 0)
    env->ThrowError("SeparateFields: Maximum number of frames exceeded.");

  vi.SetFieldBased(true);
}


PVideoFrame SeparateFields::GetFrame(int n, IScriptEnvironment* env) 
{
  PVideoFrame frame = child->GetFrame(n>>1, env);
  if (vi.IsPlanar()) {
    const bool topfield = GetParity(n);
    const int UVoffset = !topfield ? frame->GetPitch(PLANAR_U) : 0;
    const int Yoffset = !topfield ? frame->GetPitch(PLANAR_Y) : 0;
    return env->SubframePlanar(frame,Yoffset, frame->GetPitch()*2, frame->GetRowSize(), frame->GetHeight()>>1,
                               UVoffset, UVoffset, frame->GetPitch(PLANAR_U)*2);
  }
  return env->Subframe(frame,(GetParity(n) ^ vi.IsYUY2()) ? frame->GetPitch() : 0,
                         frame->GetPitch()*2, frame->GetRowSize(), frame->GetHeight()>>1);  
}


AVSValue __cdecl SeparateFields::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  PClip clip = args[0].AsClip();
  if (clip->GetVideoInfo().IsFieldBased())
    env->ThrowError("SeparateFields: SeparateFields should be applied on frame-based material: use AssumeFrameBased() beforehand");
//    return clip;
//  else
    return new SeparateFields(clip, env);
}







/******************************
 *******   Interleave   *******
 ******************************/

Interleave::Interleave(int _num_children, const PClip* _child_array, IScriptEnvironment* env)
  : IClip(__FUNCTION__),num_children(_num_children), child_array(_child_array)
{
  vi = child_array[0]->GetVideoInfo();
  vi.MulDivFPS(num_children, 1);
  vi.num_frames = (vi.num_frames - 1) * num_children + 1;
  for (int i=1; i<num_children; ++i) 
  {
    const VideoInfo& vi2 = child_array[i]->GetVideoInfo();
    if (vi.width != vi2.width || vi.height != vi2.height)
      env->ThrowError("Interleave: videos must be of the same size.");
    if (!vi.IsSameColorspace(vi2))
      env->ThrowError("Interleave: video formats don't match");
    
    vi.num_frames = std::max(vi.num_frames, (vi2.num_frames - 1) * num_children + i + 1);
  }
  if (vi.num_frames < 0)
    env->ThrowError("Interleave: Maximum number of frames exceeded.");

}

AVSValue __cdecl Interleave::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  args = args[0];
  const int num_args = args.ArraySize();
  if (num_args == 1)
    return args[0];
  PClip* child_array = new PClip[num_args];
  for (int i=0; i<num_args; ++i)
    child_array[i] = args[i].AsClip();
  return new Interleave(num_args, child_array, env);
}






/*********************************
 ********   SelectEvery    *******
 *********************************/


SelectEvery::SelectEvery(PClip _child, int _every, int _from)
 : GenericVideoFilter(_child,__FUNCTION__ ), every(_every), from(_from)
{
  vi.MulDivFPS(1, every);
  vi.num_frames = (vi.num_frames-1-from) / every + 1;
}


AVSValue __cdecl SelectEvery::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  const int num_vals = args[2].ArraySize();
  if (num_vals <= 1)
    return new SelectEvery(args[0].AsClip(), args[1].AsInt(), num_vals>0 ? args[2][0].AsInt() : 0);
  else {
    PClip* child_array = new PClip[num_vals];
    for (int i=0; i<num_vals; ++i)
      child_array[i] = new SelectEvery(args[0].AsClip(), args[1].AsInt(), args[2][i].AsInt());
    return new Interleave(num_vals, child_array, env);
  }
}








/**************************************
 ********   DoubleWeaveFields   *******
 *************************************/

DoubleWeaveFields::DoubleWeaveFields(PClip _child)
  : GenericVideoFilter(_child,__FUNCTION__ ) 
{
  vi.height *= 2;
  vi.SetFieldBased(false);
}


PVideoFrame DoubleWeaveFields::GetFrame(int n, IScriptEnvironment* env) 
{
  PVideoFrame a = child->GetFrame(n, env);  
  if (a == nullptr || a.m_ptr == nullptr) {
      return nullptr;
  }
  PVideoFrame b = child->GetFrame(n+1, env);
  if (b == nullptr || b.m_ptr == nullptr) {
      return nullptr;
  }
  PVideoFrame result = env->NewVideoFrame(vi);
  if (result == nullptr || result.m_ptr == nullptr) {
      return nullptr;
  }
  const bool parity = child->GetParity(n);

  CopyField(result, a, parity);
  CopyField(result, b, !parity);

  return result;
}


void DoubleWeaveFields::CopyField(const PVideoFrame& dst, const PVideoFrame& src, bool parity) 
{
  const int add_pitch = dst->GetPitch() * (parity ^ vi.IsYUV());
  const int add_pitchUV = dst->GetPitch(PLANAR_U) * (parity ^ vi.IsYUV());

  BitBlt( dst->GetWritePtr()         + add_pitch,   dst->GetPitch()*2,
          src->GetReadPtr(),                        src->GetPitch(),
          src->GetRowSize(),                        src->GetHeight() );

  BitBlt( dst->GetWritePtr(PLANAR_U) + add_pitchUV, dst->GetPitch(PLANAR_U)*2,
          src->GetReadPtr(PLANAR_U),                src->GetPitch(PLANAR_U),
          src->GetRowSize(PLANAR_U),                src->GetHeight(PLANAR_U) );

  BitBlt( dst->GetWritePtr(PLANAR_V) + add_pitchUV, dst->GetPitch(PLANAR_V)*2,
          src->GetReadPtr(PLANAR_V),                src->GetPitch(PLANAR_V),
          src->GetRowSize(PLANAR_V),                src->GetHeight(PLANAR_V) );
}








/**************************************
 ********   DoubleWeaveFrames   *******
 *************************************/

DoubleWeaveFrames::DoubleWeaveFrames(PClip _child) 
  : GenericVideoFilter(_child,__FUNCTION__ ) 
{
  vi.num_frames *= 2;
  if (vi.num_frames < 0)
    vi.num_frames = 0x7FFFFFFF; // MAXINT

  vi.MulDivFPS(2, 1);
}


PVideoFrame DoubleWeaveFrames::GetFrame(int n, IScriptEnvironment* env) 
{
  if (!(n&1)) 
  {
    return child->GetFrame(n>>1, env);
  } 
  else {
    PVideoFrame a = child->GetFrame(n>>1, env); 
    if (a == nullptr || a.m_ptr == nullptr) {
        return nullptr;
    }
    PVideoFrame b = child->GetFrame((n+1)>>1, env); 
    if (b == nullptr || b.m_ptr == nullptr) {
        return nullptr;
    }
    bool parity = this->GetParity(n);

    if (a->IsWritable()) {
      CopyAlternateLines(a, b, !parity);
      return a;
    } 
    else if (b->IsWritable()) {
      CopyAlternateLines(b, a, parity);
      return b;
    } 
    else {
      PVideoFrame result = env->NewVideoFrame(vi);  
      if (result == nullptr || result.m_ptr == nullptr) {
          return nullptr;
      }
      CopyAlternateLines(result, a, parity);
      CopyAlternateLines(result, b, !parity);
      return result;
    }
  }
}


void DoubleWeaveFrames::CopyAlternateLines(const PVideoFrame& dst, const PVideoFrame& src, bool parity) 
{
  const int src_add_pitch   = src->GetPitch()         * (parity ^ vi.IsYUV());
  const int src_add_pitchUV = src->GetPitch(PLANAR_U) * (parity ^ vi.IsYUV());

  const int dst_add_pitch   = dst->GetPitch()         * (parity ^ vi.IsYUV());
  const int dst_add_pitchUV = dst->GetPitch(PLANAR_U) * (parity ^ vi.IsYUV());
 
  BitBlt( dst->GetWritePtr()         + dst_add_pitch,   dst->GetPitch()*2,
          src->GetReadPtr()          + src_add_pitch,   src->GetPitch()*2,
          src->GetRowSize(),                            src->GetHeight()>>1 );

  BitBlt( dst->GetWritePtr(PLANAR_U) + dst_add_pitchUV, dst->GetPitch(PLANAR_U)*2,
          src->GetReadPtr(PLANAR_U)  + src_add_pitchUV, src->GetPitch(PLANAR_U)*2,
          src->GetRowSize(PLANAR_U),                    src->GetHeight(PLANAR_U)>>1 );

  BitBlt( dst->GetWritePtr(PLANAR_V) + dst_add_pitchUV, dst->GetPitch(PLANAR_V)*2,
          src->GetReadPtr(PLANAR_V)  + src_add_pitchUV, src->GetPitch(PLANAR_V)*2,
          src->GetRowSize(PLANAR_V),                    src->GetHeight(PLANAR_V)>>1 );
}







/*******************************
 ********   Bob Filter   *******
 *******************************/

Fieldwise::Fieldwise(PClip _child1, PClip _child2) 
 : GenericVideoFilter(_child1,__FUNCTION__), child2(_child2)
  { vi.SetFieldBased(false); } // Make FrameBased, leave IT_BFF and IT_TFF alone


PVideoFrame __stdcall Fieldwise::GetFrame(int n, IScriptEnvironment* env) 
{
  return (child->GetParity(n) ? child2 : child)->GetFrame(n, env);
}


bool __stdcall Fieldwise::GetParity(int n)
{
  return child->GetParity(n) ^ (n&1); // ^ = XOR
}







/************************************
 ********   Factory Methods   *******
 ***********************************/

static AVSValue __cdecl Create_DoubleWeave(AVSValue args, void*, IScriptEnvironment* env) 
{
  PClip clip = args[0].AsClip();
  if (clip->GetVideoInfo().IsFieldBased())
    return new DoubleWeaveFields(clip);
  else
    return new DoubleWeaveFrames(clip);
}


static AVSValue __cdecl Create_Weave(AVSValue args, void*, IScriptEnvironment* env) 
{
  PClip clip = args[0].AsClip();
  if (!clip->GetVideoInfo().IsFieldBased())
    env->ThrowError("Weave: Weave should be applied on field-based material: use AssumeFieldBased() beforehand");
  return new SelectEvery(Create_DoubleWeave(args, 0, env).AsClip(), 2, 0);
}


static AVSValue __cdecl Create_Pulldown(AVSValue args, void*, IScriptEnvironment* env) 
{
  PClip clip = args[0].AsClip();
  PClip* child_array = new PClip[2];
  child_array[0] = new SelectEvery(clip, 5, args[1].AsInt() % 5);
  child_array[1] = new SelectEvery(clip, 5, args[2].AsInt() % 5);
  return new AssumeFrameBased(new Interleave(2, child_array, env));
}


static AVSValue __cdecl Create_SwapFields(AVSValue args, void*, IScriptEnvironment* env) 
{
  return new SelectEvery(new DoubleWeaveFields(new ComplementParity(
    new SeparateFields(args[0].AsClip(), env))), 2, 0);
}



SelectRangeEvery::SelectRangeEvery(PClip _child, int _every, int _length, int _offset, bool _audio, IScriptEnvironment* env)
    : GenericVideoFilter(_child,__FUNCTION__ ), audio(_audio), achild(_child)
{
  const __int64 num_audio_samples = vi.num_audio_samples;

  AVSValue trimargs[3] = { _child, _offset, 0};
  PClip c = env->Invoke("Trim1",AVSValue(trimargs,3)).AsClip();
  child = c;
  vi = c->GetVideoInfo();

  every = std::min(std::max(_every,1),vi.num_frames);
  length = std::min(std::max(_length,1),every);

  const int n = vi.num_frames;
  vi.num_frames = (n/every)*length+(n%every<length?n%every:length);

  if (audio && vi.HasAudio()) {
    vi.num_audio_samples = vi.AudioSamplesFromFrames(vi.num_frames);
  } else {
    vi.num_audio_samples = num_audio_samples; // Undo Trim's work!
  }
}


PVideoFrame __stdcall SelectRangeEvery::GetFrame(int n, IScriptEnvironment* env)
{
  return child->GetFrame((n/length)*every+(n%length), env);
}


bool __stdcall SelectRangeEvery::GetParity(int n)
{
  return child->GetParity((n/length)*every+(n%length));
}


void __stdcall SelectRangeEvery::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env)
{
  if (!audio) {
	// Use original unTrim'd child
    achild->GetAudio(buf, start, count, env);
    return;
  }

  __int64 samples_filled = 0;
  BYTE* samples = (BYTE*)buf;
  const int bps = vi.BytesPerAudioSample();
  int startframe = vi.FramesFromAudioSamples(start);
  __int64 general_offset = start - vi.AudioSamplesFromFrames(startframe);  // General compensation for startframe rounding.

  while (samples_filled < count) {
    const int iteration = startframe / length;                    // Which iteration is this.
    const int iteration_into = startframe % length;               // How far, in frames are we into this iteration.
    const int iteration_left = length - iteration_into;           // How many frames is left of this iteration.

    const __int64 iteration_left_samples = vi.AudioSamplesFromFrames(iteration_left);
    // This is the number of samples we can get without either having to skip, or being finished.
    const __int64 getsamples = std::min(iteration_left_samples, count-samples_filled);
    const __int64 start_offset = vi.AudioSamplesFromFrames(iteration * every + iteration_into) + general_offset;

    child->GetAudio(&samples[samples_filled*bps], start_offset, getsamples, env);
    samples_filled += getsamples;
    startframe = (iteration+1) * every;
    general_offset = 0; // On the following loops, general offset should be 0, as we are either skipping.
  }
}

AVSValue __cdecl SelectRangeEvery::Create(AVSValue args, void* user_data, IScriptEnvironment* env) {
    return new SelectRangeEvery(args[0].AsClip(), args[1].AsInt(1500), args[2].AsInt(50), args[3].AsInt(0), args[4].AsBool(true), env);
}
