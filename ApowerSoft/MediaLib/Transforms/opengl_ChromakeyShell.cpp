
#include "Utils.hpp"



using base = GenericVideoFilter;
class ChromakeyShell : public GenericVideoFilter {
public:
    explicit ChromakeyShell(const PClip& child,
        int start, int end, int width, int height, int color, int dcolor, float _threshhold,
        float _blurSize, IScriptEnvironment* env);

    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);



protected:
    std::string vertexshaderstr;
    std::string fragshaderstr;
    std::string _error;

    int _frame_count, _frame_start, _frame_end;

    PClip _child_0;
    VideoInfo _vi_0;
private:
    float _R;
    float _G;
    float _B;

    float _dR;
    float _dG;
    float _dB;

    float _threshhold;
    float _blurSize;
};



 AVSValue __cdecl Create_ChromakeyShell(AVSValue args, void* user_data, IScriptEnvironment* env) {
    ////WXLogA("------------- OPENGL File Create [%s]", __FUNCTION__);
    return new ChromakeyShell(args[0].AsClip(), args[1].AsInt(0), args[2].AsInt(200000000),
        args[3].AsInt(0), args[4].AsInt(0), args[5].AsInt(0), args[6].AsInt(0),
        args[7].AsFloat(0.0f), args[8].AsFloat(0.0f), env);
}

ChromakeyShell::ChromakeyShell(const PClip& child,
    int start, int end, int width, int height, int color, int dcolor, float _threshhold,
    float _blurSize, IScriptEnvironment* env)
    : base{ child ,__FUNCTION__ }, _frame_count{ 0 }, _frame_start{ start }, _frame_end{ end }, _child_0{ child }, _vi_0{ child->GetVideoInfo() }
    , _threshhold{ _threshhold }, _blurSize{ _blurSize }{

    if (!_vi_0.HasVideo() && !_vi_0.HasAudio()) {
        env->ThrowError("No Video && No Audio!");
    }

    switch (_vi_0.pixel_type) {
    case CS_BGR24:
    case CS_BGR32:
        break;
    default: {
        _child_0 = env->Invoke("ConvertToRGB32", _child_0).AsClip();
        _vi_0 = _child_0->GetVideoInfo();
    }
           break;
    }

    try {
        auto fps = env->GetTimelineInfo()->FrameRate;
        _vi_0.SetFPS(lround(fps * 1001), 1001);
    }
    catch (...) {}

    const auto orig_frames = _vi_0.num_frames;

    _frame_start = std::min(std::max(_frame_start, 0), orig_frames - 1);
    _frame_end = std::min(std::max(_frame_end, _frame_start), orig_frames - 1);
    _frame_count = _frame_end - _frame_start + 1;

    this->child = _child_0;
    this->vi = _vi_0;

    try {
        if (width > 0) {
            vi.width = width;
        }
        if (height > 0) {
            vi.height = height;
        }
    }
    catch (...) {}

    auto r = (color & 0xff0000) >> 16; 
    auto g = (color & 0xff00) >> 8;
    auto b = color & 0xff;

    _R = static_cast<float>(r)/255.0;
    _G = static_cast<float>(g) / 255.0;
    _B = static_cast<float>(b) / 255.0;

    auto dr = (dcolor & 0xff0000) >> 16;
    auto dg = (dcolor & 0xff00) >> 8;
    auto db = dcolor & 0xff;

    _dR = static_cast<float>(dr) / 255.0;
    _dG = static_cast<float>(dg) / 255.0;
    _dB = static_cast<float>(db) / 255.0;
    // ¿ÙÏñ
     _child_0 = env->Invoke("Chromakey", AVSValueArray{ _child_0, start, end, width, height, 
         _R, _G, _B, _dR, _dG, _dB,_threshhold, _blurSize }).AsClip();

    // ±ßÔµµ÷Õû
    // _child_0 = env->Invoke("EdgeIncrease", AVSValueArray{ _child_0, start, end, width, height, _edgeWidth }).AsClip();

    //// Óð»¯
    // _child_0 = env->Invoke("EdgeBlur", AVSValueArray{ _child_0, start, end, width, height, _blurRadius }).AsClip();
}

PVideoFrame ChromakeyShell::GetFrame(int n, IScriptEnvironment* env) {
    return _child_0->GetFrame(n, env);
}