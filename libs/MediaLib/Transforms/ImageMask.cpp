#include "Utils.hpp"
#include <avisynth/avisynth.h>

using base = GenericVideoFilter;

class ImageMask : public GenericVideoFilter {
    std::wstring _mask_path;
    const float _x, _y, _w, _h;
    const bool _alpha_or_grayscale;

public:


public:
    explicit ImageMask(const PClip& child, const char* mask,
        float x, float y, float w, float h,
        bool alpha_or_grayscale,
        IScriptEnvironment* env);

    intptr_t __stdcall SetCacheHints(int cachehints, int frame_range) override {
        return child->SetCacheHints(cachehints, frame_range);
    }
};


AVSValue __cdecl Create_ImageMask(AVSValue args, void* user_data, IScriptEnvironment* env) {
    return new ImageMask(args[0].AsClip(), args[1].AsString(""),
        args[2].AsFloat(), args[3].AsFloat(), args[4].AsFloat(), args[5].AsFloat(),
        args[6].AsBool(),
        env);
}
ImageMask::ImageMask(const PClip &_child, const char *mask,
                     const float x, const float y, const float w, const float h,
                     const bool alpha_or_grayscale,
                     IScriptEnvironment *env) : base{ _child,__FUNCTION__ }, _mask_path{ utils::StringExt::base64_s2ws(mask) },
                                                _x{ x }, _y{ y }, _w{ w }, _h{ h }, _alpha_or_grayscale{ alpha_or_grayscale } {
    if (_w * _h > 0 && WXBase::Exists(_mask_path)) {
        

        if (!vi.IsRGB32()) {
            child = env->Invoke("ConvertToRGB32", child).AsClip();
            vi = child->GetVideoInfo();
        }

        const auto frames = vi.num_frames;
        const auto fps = static_cast<double>(vi.fps_numerator) / vi.fps_denominator;

        static const char *blankclip_arg_names[] = { "width", "height", "length", "fps", "audio_rate" };
        const auto mask_base = env->Invoke("BlankClip", AVSValueArray { vi.width, vi.height, frames, fps, 0 }, blankclip_arg_names).AsClip();

        const auto mask_path = WXBase::UTF16ToUTF8(_mask_path);
        const std::initializer_list<AVSValue> mask_source_args = {
            mask_path.c_str(),
            (int)lround(_w * vi.width),
            (int)lround(_h * vi.height),
            0,
            frames,
            fps,
            1
        };
        auto mask_source = env->Invoke("WICImage", AVSValueArray { mask_source_args }).AsClip();

        if (_alpha_or_grayscale) {
            mask_source = env->Invoke("ShowAlpha", mask_source).AsClip();
        }

        static const char *layer_arg_names[] = { nullptr, nullptr, "x", "y" };
        const std::initializer_list<AVSValue> mask_clip_args = {
            mask_base,
            mask_source,
            (int)lround(_x * vi.width),
            (int)lround(_y * vi.height)
        };
        const auto mask_clip = env->Invoke("Layer", AVSValueArray { mask_clip_args }, layer_arg_names).AsClip();

        child = env->Invoke("AlphaMask", AVSValueArray { child, mask_clip }).AsClip();
        child = env->Invoke("InternalCache", child).AsClip();
        vi = child->GetVideoInfo();
    }
}
