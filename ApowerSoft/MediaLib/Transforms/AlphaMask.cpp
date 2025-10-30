
#include "Utils.hpp"

using base = GenericVideoFilter;

class AlphaMask : public GenericVideoFilter {
    PClip _mask_clip;
public:
    explicit AlphaMask(const PClip& _child, const PClip& mask_clip,
        IScriptEnvironment* env);

    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) override;

    intptr_t __stdcall SetCacheHints(int cachehints, int frame_range) override {
        return child->SetCacheHints(cachehints, frame_range);
    }
};


AVSValue __cdecl Create_AlphaMask(AVSValue args, void* user_data, IScriptEnvironment* env) {
    return new AlphaMask(args[0].AsClip(), args[1].AsClip(), env);
}

AlphaMask::AlphaMask(const PClip &_child, const PClip &mask_clip, IScriptEnvironment *env): base{ _child,__FUNCTION__ }, _mask_clip{ mask_clip } {
    auto vi2 = _mask_clip->GetVideoInfo();

    if (!vi.IsRGB32()) {
        child = env->Invoke("ConvertToRGB32", child).AsClip();
        vi = child->GetVideoInfo();
    }
    if (!vi2.IsRGB32()) {
        _mask_clip = env->Invoke("ConvertToRGB32", _mask_clip).AsClip();
        vi2 = _mask_clip->GetVideoInfo();
    }
    if (vi.width != vi2.width || vi.height != vi2.height) {
        _mask_clip = env->Invoke("BilinearResize", AVSValueArray { _mask_clip, vi.width, vi.height }).AsClip();
        vi2 = _mask_clip->GetVideoInfo();
    }
    if (vi2.num_frames < vi.num_frames) {
        _mask_clip = env->Invoke("LoopFrames", AVSValueArray { _mask_clip, vi.num_frames }).AsClip();
        vi2 = _mask_clip->GetVideoInfo();
    }
}

PVideoFrame AlphaMask::GetFrame(int n, IScriptEnvironment *env) {
    auto frame1 = base::GetFrame(n, env);
    env->MakeWritable(&frame1);
    const auto pitch = frame1->GetPitch();

    const auto pframe1 = frame1->GetWritePtr();
    const auto pframe2 = _mask_clip->GetFrame(n, env)->GetReadPtr();

    for (auto y = 0; y < vi.height; ++y) {
        for (auto x = 3; x < pitch; x += 4) {
            const auto i = y * pitch + x;
            const int val1 = pframe1[i];
            const int val2 = pframe2[i];
            pframe1[i] = static_cast<BYTE>(val1 * val2 / 255);
        }
    }

    return frame1;
}
