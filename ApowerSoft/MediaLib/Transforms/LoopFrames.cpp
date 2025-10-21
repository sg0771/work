#include "Utils.hpp"
#include <avisynth/avisynth.h>

using base = GenericVideoFilter;

class LoopFrames : public GenericVideoFilter {
private:
    int need_frames, orig_frames;
    int frame_count, frame_start, frame_end;
    long long aud_count, aud_start, aud_end;
    int in_start;
    int convert(int n) const {
        if (in_start != -2)
            return need_frames < 0 ? n : n % frame_count + frame_start;
        else if (n > frame_count)
            return frame_count + frame_start;
        else
            return n;
    }

    long long AudioSamplesFromFrames(const long long frames) const {
        return vi.fps_numerator ? vi.SamplesPerSecond() * frames * vi.fps_denominator / vi.fps_numerator : 0;
    }


public:
    explicit LoopFrames(const PClip& _child,
        int _frames, int _start, int _end,
        IScriptEnvironment* env);

    bool __stdcall GetParity(int n) override {
        return base::GetParity(convert(n));
    }

    intptr_t __stdcall SetCacheHints(int cachehints, int frame_range) override {
        return child->SetCacheHints(cachehints, frame_range);
    }

    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) override {
        return base::GetFrame(convert(n), env);
    }

    void __stdcall GetAudio(void* buf, long long start, long long count, IScriptEnvironment* env) override;
};


AVSValue __cdecl Create_LoopFrames(AVSValue args, void* user_data, IScriptEnvironment* env) {
    return new LoopFrames(args[0].AsClip(),
        args[1].AsInt(-1), args[2].AsInt(0), args[3].AsInt(2000000000),
        env);
}


LoopFrames::LoopFrames(const PClip &_child,
                       const int _frames, const int _start, const int _end,
                       IScriptEnvironment *env): base{ _child,__FUNCTION__ }, need_frames { _frames }, orig_frames{ 0 },
                                                 frame_count{ 0 }, frame_start { _start }, frame_end{ _end },
                                                 aud_count{ 0 }, aud_start{ 0 }, aud_end{ 0 } {
    in_start = _start;
    if (need_frames >= 0) {
        if (!vi.HasVideo() && !vi.HasAudio()) {
            env->ThrowError("No Video && No Audio!");
        }

        try {
            auto fps = env->GetTimelineInfo()->FrameRate;
            vi.SetFPS(lround(fps * 1001), 1001);
        } catch (...) {}

        if (!vi.HasVideo()) {
            vi.num_frames = vi.FramesFromAudioSamples(vi.num_audio_samples);
        }

        orig_frames = vi.num_frames;

        frame_start = std::min(std::max(frame_start, 0), orig_frames - 1);
        frame_end = std::min(std::max(frame_end, frame_start), orig_frames - 1);
        frame_count = frame_end - frame_start + 1;

        vi.num_frames = need_frames;

        if (vi.HasAudio()) {
            aud_start = AudioSamplesFromFrames(frame_start);
            aud_end = AudioSamplesFromFrames(frame_end + 1) - 1;
            aud_count = aud_end - aud_start + 1;

            vi.num_audio_samples = AudioSamplesFromFrames(need_frames);
        }
    }
}

void LoopFrames::GetAudio(void *buf, long long start, long long count, IScriptEnvironment *env) {
    if (need_frames < 0) {
        base::GetAudio(buf, start, count, env);
        return;
    }

    const auto bpas = vi.BytesPerAudioSample();
    auto samples = static_cast<char*>(buf);

    while (count > 0) {
        const auto get_start = start % aud_count + aud_start;
        const auto get_count = std::min(count, aud_end + 1 - get_start);

        base::GetAudio(samples, get_start, get_count, env);

        samples += get_count * bpas;
        start += get_count;
        count -= get_count;
    }
}
