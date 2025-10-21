#include "Utils.hpp"

using base = GenericVideoFilter;

class AudioFade : public GenericVideoFilter {
    std::vector<std::pair<double, double>> _ranges;
    float _amplify;
public:
    explicit AudioFade(const PClip& _child, const char* ranges_str, float amplify,
        IScriptEnvironment* env);

    intptr_t __stdcall SetCacheHints(int cachehints, int frame_range) override {
        return child->SetCacheHints(cachehints, frame_range);
    }

    void __stdcall GetAudio(void* buf, long long start, long long count, IScriptEnvironment* env) override;
};


AudioFade::AudioFade(const PClip &_child, const char *ranges_str, const float amplify, IScriptEnvironment *env) : base{
    env->Invoke("ConvertAudio", AVSValueArray { _child, SAMPLE_FLOAT, SAMPLE_FLOAT }).AsClip(),__FUNCTION__
}, _amplify{ amplify } {


    auto sample_rate = env->GetTimelineInfo()->SampleRate;;
    if (vi.SamplesPerSecond() != sample_rate) {
        child = env->Invoke("ResampleAudio", AVSValueArray { child, sample_rate }).AsClip();
        vi = child->GetVideoInfo();
    }

    try {
        auto fps = env->GetTimelineInfo()->FrameRate;
        vi.SetFPS(lround(fps * 1001), 1001);
    } catch (...) {}

    std::vector<std::string> ranges;
    utils::StringExt::split(ranges, std::string { ranges_str }, "|", utils::StringExt::split_no_empties);
    for (auto &&range : ranges) {
        std::vector<std::string> pair;
        utils::StringExt::split(pair, range, ",", utils::StringExt::split_no_empties);

        if (pair.size() == 2 && !pair[0].empty() && !pair[1].empty()) {
            char *end;
            _ranges.emplace_back(std::strtod(pair[0].c_str(), &end), std::strtod(pair[1].c_str(), &end));
        }
    }
}

void AudioFade::GetAudio(void *buf, long long start, long long count, IScriptEnvironment *env) {
    base::GetAudio(buf, start, count, env);

    const auto sps = vi.SamplesPerSecond();
    if (sps > 0) {
        const auto channels = vi.AudioChannels();
        const auto samples = static_cast<SFLOAT*>(buf);

        for (auto i = 0ll; i < count; i++) {
            for (auto j = 0; j < channels; j++) {
                const auto second = static_cast<double>(start + i) / static_cast<double>(sps);
                for (auto &&range : _ranges) {
                    if (range.first <= second && second <= range.second) {
                        samples[i * channels + j] *= _amplify;
                        break;
                    }
                }
            }
        }
    }
}

AVSValue __cdecl Create_AudioFade(AVSValue args, void* user_data, IScriptEnvironment* env) {
    return new AudioFade(args[0].AsClip(), args[1].AsString(""), args[2].AsFloat(1.0f),
        env);
}