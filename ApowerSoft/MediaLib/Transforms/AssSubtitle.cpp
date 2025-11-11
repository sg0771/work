
#include <Utils.hpp>

class  AssSubtitle : public GenericVideoFilter {
protected:

    std::string assfile;
    int offset;
    int substart;
    int subend;
    ASS_Track* m_track = nullptr;
    ASS_Renderer* m_ass_renderer = nullptr;

    PVideoFrame blendimage(PVideoFrame baseframe, ass_image* subimage, IScriptEnvironment* env);
    PVideoFrame blendimage24(PVideoFrame baseframe, ass_image* subimage, IScriptEnvironment* env);

public:
    //ass 文件内容最好是标准格式
    AssSubtitle(PClip clip, std::string assfile, std::string font, int offset, int substart, int subend, IScriptEnvironment* env);
    ~AssSubtitle();
    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) override;
    void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) override;
    const VideoInfo& __stdcall GetVideoInfo() override;
    bool __stdcall GetParity(int n) override;
    intptr_t __stdcall SetCacheHints(int cachehints, int frame_range) override;
};

#include "avisynth/avisynth_stdafx.h"

#define _r(c)  ((c)>>24)
#define _g(c)  (((c)>>16)&0xFF)
#define _b(c)  (((c)>>8)&0xFF)
#define _a(c)  ((c)&0xFF)

AssSubtitle::~AssSubtitle()
{
    if (m_track)
        ass_free_track(m_track);
}

AssSubtitle::AssSubtitle(PClip clip, std::string _assfile_utf8, std::string font, int _offset, int _substart, int _subend, IScriptEnvironment* env) :
    GenericVideoFilter(clip, __FUNCTION__), assfile(_assfile_utf8), offset(_offset), substart(_substart), subend(_subend) {
    m_ass_renderer = AssEngine::InstanceML().ass_renderer;
    m_track = AssEngine::InstanceML().Read(_assfile_utf8);
}

AVSValue Create_AssSubtitle(AVSValue args, void* user_data, IScriptEnvironment* env) {

    return new AssSubtitle(args[0].AsClip(), args[1].AsString(), args[2].AsString(), args[3].AsInt(), args[4].AsInt(), args[5].AsInt(), env);
}

#define div_255_fast1(x)    (((x)+(((x)+257) >> 8)) >> 8)

#define div_255_fast(x) (((x) + 1 + (((x) + 1) >> 8)) >> 8)
PVideoFrame AssSubtitle::blendimage(PVideoFrame baseframe, ass_image* subimage, IScriptEnvironment* env) {

    int cnt = 0;
    auto baseimage_read = baseframe->GetReadPtr();

    env->MakeWritable(&baseframe);

    auto baseimage_write = baseframe->GetWritePtr();

    while (subimage) {
        if (subimage->w >= 65535 || subimage->h >= 65535 || subimage->w <= 0 || subimage->h <= 0)
            break;

        //灰度图还原成rgba格式
        int x, y;
        unsigned char opacity = 255 - _a(subimage->color);
        unsigned char r = _r(subimage->color);
        unsigned char g = _g(subimage->color);
        unsigned char b = _b(subimage->color);

        unsigned char* src = subimage->bitmap;
        unsigned char* dst;

        auto subargbdata = gen_image(subimage->w, subimage->h);
        if (subargbdata == NULL || subargbdata->buffer == NULL)
        {
            break;
        }
        dst = subargbdata->buffer + subargbdata->stride * (subimage->h - 1);

        uint8_t ks[256] = {};
        for (size_t i = 0; i < 256; i++)
        {
            ks[i] = (uint8_t)(i * opacity / 255);
        }


        for (y = 0; y < subimage->h; ++y) {
            for (x = 0; x < subimage->w; ++x) {

                if (src[x] == 0)
                {
                    continue;
                }
                uint8_t k = ks[src[x]];// ((unsigned)src[x])* opacity / 255;
                //*((int*)dst) = k << 24 +  r<< 16 + g << 8 + b;
                // possible endianness problems
                dst[x * 4] = b;
                dst[x * 4 + 1] = g;
                dst[x * 4 + 2] = r;
                dst[x * 4 + 3] = k;

            }
            src += subimage->stride;
            dst -= subargbdata->stride;
        }

        //rgba数据叠加到底图上
        int xdest = subimage->dst_x;


        int ydest = vi.height - subimage->h - subimage->dst_y;
        ydest = (ydest < 0) ? 0 : ydest;
        auto baseimage_read_clip = baseimage_write + 4 * xdest + ydest * baseframe->GetPitch();

        auto baseimage_write_clip = baseimage_write + 4 * xdest + ydest * baseframe->GetPitch();
        auto over = subargbdata->buffer;

        libyuv::ARGBAttenuate(over, subargbdata->stride, over, subargbdata->stride, subargbdata->width, subargbdata->height);
        libyuv::ARGBBlend(over, subargbdata->stride, baseimage_read_clip, baseframe->GetPitch(), baseimage_write_clip, baseframe->GetPitch(), subargbdata->width, subargbdata->height);

        ++cnt;

        av_free(subargbdata->buffer);
        av_free(subargbdata);

        subimage = subimage->next;
        if (subimage == nullptr)
            break;
#ifdef _M_X64
        if ((uintptr_t)subimage == 0xdddddddddddddddd || subimage->w >= 65535 || subimage->h >= 65535 || subimage->w <= 0 || subimage->h <= 0)
            break;
#else
        if ((uintptr_t)subimage == 0xdddddddd || subimage->w >= 65535 || subimage->h >= 65535 || subimage->w <= 0 || subimage->h <= 0)
            break;
#endif
    }

    return baseframe;
}

PVideoFrame AssSubtitle::blendimage24(PVideoFrame baseframe, ass_image* subimage, IScriptEnvironment* env) {
    int cnt = 0;
    auto baseimage_read = baseframe->GetReadPtr();

    env->MakeWritable(&baseframe);

    auto baseimage_write = baseframe->GetWritePtr();

    while (subimage) {
        if (subimage->w >= 65535 || subimage->h >= 65535 || subimage->w <= 0 || subimage->h <= 0)
            break;

        int x, y;
        unsigned char opacity = 255 - _a(subimage->color);
        unsigned char r = _r(subimage->color);
        unsigned char g = _g(subimage->color);
        unsigned char b = _b(subimage->color);


        //字幕叠加到底图上
        int xdest = subimage->dst_x;


        int ydest = vi.height - subimage->h - subimage->dst_y;
        unsigned char* src = subimage->bitmap;
        unsigned char* dst = baseimage_write + ydest * baseframe->GetPitch() + xdest * 3;
        for (y = 0; y < subimage->h; ++y) {
            for (x = 0; x < subimage->w; ++x) {
                unsigned k = ((unsigned)src[x]) * opacity / 255;
                // possible endianness problems
                dst[x * 3] = (k * b + (255 - k) * dst[x * 3]) / 255;
                dst[x * 3 + 1] = (k * g + (255 - k) * dst[x * 3 + 1]) / 255;
                dst[x * 3 + 2] = (k * r + (255 - k) * dst[x * 3 + 2]) / 255;
            }
            src += subimage->stride;
            dst += baseframe->GetPitch();
        }
        ++cnt;
        subimage = subimage->next;
        if (subimage == nullptr)
            break;
#ifdef _M_X64
        if ((uintptr_t)subimage == 0xdddddddddddddddd || subimage->w >= 65535 || subimage->h >= 65535 || subimage->w <= 0 || subimage->h <= 0)
            break;
#else
        if ((uintptr_t)subimage == 0xdddddddd || subimage->w >= 65535 || subimage->h >= 65535 || subimage->w <= 0 || subimage->h <= 0)
            break;
#endif
    }
    return baseframe;
    printf("%d images blended\n", cnt);
}
PVideoFrame __stdcall AssSubtitle::GetFrame(int n, IScriptEnvironment* env) {
    PVideoFrame srcFrame = child->GetFrame(n, env);
    if (m_track == NULL)
    {
        return srcFrame;
    }

    //修复字幕时间戳问题
    if (n >= offset && n <= offset + subend - substart)
    {
        int subtime = (n - offset + this->substart) * vi.fps_denominator * 1000 / vi.fps_numerator;
        ass_set_frame_size(m_ass_renderer, vi.width, vi.height);
        ass_image* imgAss = ass_render_frame(m_ass_renderer, m_track, subtime, NULL);
        if (imgAss)
        {
            if (vi.IsRGB32())
            {
                return blendimage(srcFrame, imgAss, env);
            }
            else {
                return blendimage24(srcFrame, imgAss, env);
            }
        }
    }
    return srcFrame;
}

void __stdcall AssSubtitle::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {
    //child->GetAudio(buf, start, count, env);
}

const VideoInfo& __stdcall AssSubtitle::GetVideoInfo() {
    return vi;
}

bool __stdcall AssSubtitle::GetParity(int n) {
    //return child->GetParity(n);
    return false;
}
intptr_t __stdcall AssSubtitle::SetCacheHints(int cachehints, int frame_range) { return 0; }

