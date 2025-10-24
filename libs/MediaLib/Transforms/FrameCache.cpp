
#include "Utils.hpp"
#include <avisynth/avisynth.h>


using base = GenericVideoFilter;

class FrameCache : public GenericVideoFilter {
    std::vector<int> _indexes;
    //std::map<int, PVideoFrame> _frames;
public:
    explicit FrameCache(const PClip& child, const std::vector<int>& array, IScriptEnvironment* env) : base{ child,__FUNCTION__ }, _indexes{ array } {};



public:
    explicit FrameCache(const PClip& child, const std::initializer_list<int>& array, IScriptEnvironment* env) : FrameCache{ child, std::vector<int> { array }, env } {};

    explicit FrameCache(const PClip& child, const char* value, IScriptEnvironment* env);

    ~FrameCache() override {
        _indexes.clear();
        //_frames.clear();
    }

    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) override;
};


 AVSValue __cdecl Create_FrameCache(AVSValue args, void* user_data, IScriptEnvironment* env) {
    auto params = args[1];
    if (params.IsArray()) {
        std::vector<int> indexes;
        if (params.ArraySize() > 0) {
            for (auto i = 0; i < params.ArraySize(); ++i) {
                if (params[i].IsInt()) {
                    indexes.emplace_back(params[i].AsInt(-1));
                }
            }
        }
        return new FrameCache(args[0].AsClip(), indexes, env);
    }
    return new FrameCache(args[0].AsClip(), params.AsString(""), env);
}

FrameCache::FrameCache(const PClip &child, const char *value, IScriptEnvironment *env): base{ child ,__FUNCTION__ } {
    std::vector<std::string> indexes;
    utils::StringExt::split(indexes, value, ",", utils::StringExt::split_no_empties);
    for (auto &&index : indexes) {
        _indexes.emplace_back(stoi(index));
    }
}

PVideoFrame FrameCache::GetFrame(int n, IScriptEnvironment *env) {
    /*const auto cache = _frames.find(n);
    if (cache != _frames.end()) {
        return cache->second;
    }

    auto FilterFrame = */
		return base::GetFrame(n, env);

    /*if (find(_indexes.begin(), _indexes.end(), n) != _indexes.end()) {
        _frames[n] = FilterFrame;
    }

    return FilterFrame;*/
}
