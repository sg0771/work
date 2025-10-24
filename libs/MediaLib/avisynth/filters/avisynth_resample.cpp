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








