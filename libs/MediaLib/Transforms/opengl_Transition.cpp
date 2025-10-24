/*
转场对象
*/
#include "Utils.hpp"
#include "opengl_GLTransition.h"
#include "opengl_SimpleGLFilter.h"

#include <avisynth/avisynth.h>
#include <string>
#include "md5.h"
#include <gdiplus.h>

void MLSetWaveLength(int n);
//获取波形数据
int MLGetWaveData(int* pData);
//波形图
class WaveForm : public IClip {

protected:
    VideoInfo m_vi;

    int m_data[256] = { 0 };//数据
    COLORREF m_dwColor = RGB(0, 255, 0);//颜色
    int m_R = 0;
    int m_G = 0;
    int m_B = 0;
    int m_width;
    int m_height;
public:
    WaveForm(const int width,//count
        const int height,
        const int color,
        const int fps,
        IScriptEnvironment* env) :
        IClip(__FUNCTION__), m_width{ width },
        m_height{ height },
        m_dwColor{ (COLORREF)color }
    {
        memset(&m_vi, 0, sizeof m_vi);
        m_vi.width = m_width;
        m_vi.height = m_height;
        m_vi.pixel_type = CS_BGR32;
        m_vi.num_frames = 24000;
        m_vi.SetFPS(fps, 1);
        MLSetWaveLength(width);//设置波形长度
        m_R = GetRValue(m_dwColor);
        m_G = GetGValue(m_dwColor);
        m_B = GetBValue(m_dwColor);
    }

    ~WaveForm() override {
        //release();
    }

    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) override {
        PVideoFrame dstFrame = env->NewVideoFrame(m_vi); //DstFrame
        if (dstFrame == nullptr || dstFrame.m_ptr == nullptr) {
            return nullptr;
        }
        env->MakeWritable(&dstFrame);
        BYTE* pDst = dstFrame->GetWritePtr();
        const int DstPitch = dstFrame->GetPitch();

        MLGetWaveData(m_data);//获取波形数据

        for (size_t i = 0; i < m_width; i++) {
            m_data[i] = m_data[i] * m_height / 32767;;
        }

        memset(pDst, 0x00, DstPitch * m_height);//清空数据

        for (size_t h = 0; h < m_height; h++)
        {
            for (size_t w = 0; w < m_width; w++)
            {
                int vaule = m_data[w];//波形绘制高度
                int pos = h * DstPitch + w * 4;
                if (h > vaule) {
                    pDst[pos + 0] = 0;
                    pDst[pos + 1] = 0;
                    pDst[pos + 2] = 0;
                    pDst[pos + 3] = 0;
                }
                else {
                    pDst[pos + 0] = m_B;
                    pDst[pos + 1] = m_G;
                    pDst[pos + 2] = m_R;
                    pDst[pos + 3] = 0xFF;
                }
            }
        }
        return dstFrame;
    }

    void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) override {
        //child->GetAudio(buf, start, count, env);
    }

    const VideoInfo& __stdcall GetVideoInfo() override {
        return m_vi;
    }

    bool __stdcall GetParity(int n) override {
        //return child->GetParity(n);
        return false;
    }

    intptr_t __stdcall SetCacheHints(int cachehints, int frame_range) override {
        switch (cachehints) {
        case CACHE_GETCHILD_CACHE_MODE:
            return CACHE_GENERIC;
        case CACHE_GETCHILD_AUDIO_MODE:
            return CACHE_AUDIO_NOTHING;
        case CACHE_GETCHILD_COST:
            return CACHE_COST_UNIT;
        case CACHE_GETCHILD_THREAD_MODE:
            return CACHE_THREAD_CLASS;
        case CACHE_GETCHILD_ACCESS_COST:
            return CACHE_ACCESS_RAND;
        default:;
        }
        return 0;
    }
};

AVSValue __cdecl Create_WaveForm(AVSValue args, void* user_data, IScriptEnvironment* env) {
    auto timelineinfo = env->GetTimelineInfo();
    return new WaveForm(args[0].AsInt(128),
        args[1].AsInt(timelineinfo->FrameHeight),
        args[2].AsInt(RGB(0, 255, 0)),
        args[3].AsInt(timelineinfo->FrameRate),
        env);
}


class WICImage : public IClip {

protected:
    VideoInfo m_vi;

    std::wstring m_file_path;

    std::wstring m_cache_path;

    const int m_width, m_height, m_rotate_flip;
public:
    WICImage(const char* path,
        const int width, const int height, const int rotate_flip,
        const int frames, const double fps,
        IScriptEnvironment* env) : IClip(__FUNCTION__), m_file_path{ WXBase::UTF8ToUTF16(path) }, m_width{ width }, m_height{ height }, m_rotate_flip{ rotate_flip }
    {
        memset(&m_vi, 0, sizeof m_vi);
        m_vi.width = m_width;
        m_vi.height = m_height;
        m_vi.pixel_type = CS_BGR32;
        m_vi.num_frames = std::max(frames, 1);
        m_vi.SetFPS(lround(fps * 1001), 1001);
        m_cache_path = m_file_path + std::to_wstring(width) + L"x" + std::to_wstring(height);
    }

    ~WICImage() override {
        //release();
    }

    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) override;


    PVideoFrame read_image_file(std::wstring strImageName, IScriptEnvironment* env) {
        if (m_width <= 0 || m_height <= 0)
            return NULL;
        Gdiplus::Bitmap bitmap(strImageName.c_str());
        int width = bitmap.GetWidth();
        int height = bitmap.GetHeight();
        if (width && &height) {
            PVideoFrame dstFrame = env->NewVideoFrame(m_vi); //DstFrame
            if (dstFrame == nullptr || dstFrame.m_ptr == nullptr) {
                return nullptr;
            }
            env->MakeWritable(&dstFrame);
            BYTE* rgbdstp = dstFrame->GetWritePtr();
            const int Dpitch = dstFrame->GetPitch();
            if (rgbdstp == nullptr) {
                //WXLogA("Decode Image Create New Frame Error [%dx%d]", m_width, m_height);
                return NULL;
            }

            int ImageRoate = 0;
            Gdiplus::PropertyItem* pPropItem = nullptr;
            UINT size = bitmap.GetPropertyItemSize(PropertyTagOrientation);
            if (size) {
                pPropItem = (Gdiplus::PropertyItem*)malloc(size);
                bitmap.GetPropertyItem(PropertyTagOrientation, size, pPropItem);
                ImageRoate = *(WORD*)pPropItem->value;
                free(pPropItem);
            }

            switch (ImageRoate)
            {
            case 2:
                bitmap.RotateFlip(Gdiplus::RotateNoneFlipX);
                break;
            case 3:
                bitmap.RotateFlip(Gdiplus::Rotate180FlipNone);
                break;
            case 4:
                bitmap.RotateFlip(Gdiplus::RotateNoneFlipY);
                break;
            case 5:
                bitmap.RotateFlip(Gdiplus::Rotate90FlipX);
                break;
            case 6:
                bitmap.RotateFlip(Gdiplus::Rotate90FlipNone);
                break;
            case 7:
                bitmap.RotateFlip(Gdiplus::Rotate270FlipX);
                break;
            case 8:
                bitmap.RotateFlip(Gdiplus::Rotate270FlipNone);
                break;
            default:
                // 正常，不需要旋转
                break;
            }
            width = bitmap.GetWidth();
            height = bitmap.GetHeight();

            Gdiplus::BitmapData bmpData;
            Gdiplus::Rect       rect(0, 0, width, height);
            Gdiplus::Status st = bitmap.LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bmpData);
            if (st == Gdiplus::Ok) { //ARGBCopy
                uint8_t* pSrc = (uint8_t*)bmpData.Scan0;
                int stride = bmpData.Stride;
                if (m_width == width && m_height == width) {
                    libyuv::ARGBCopy(pSrc, stride,
                        rgbdstp + (height - 1) * Dpitch, -Dpitch,
                        width, height);
                }
                else {  //ARGBScale
                    libyuv::ARGBScale(pSrc, stride, width, height,
                        rgbdstp + (m_height - 1) * Dpitch, -Dpitch, m_width, m_height,
                        libyuv::FilterMode::kFilterBilinear
                    );
                }
                bitmap.UnlockBits(&bmpData);
                return dstFrame;
            }
        }
        return NULL;
    }
    void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) override {
        //child->GetAudio(buf, start, count, env);
    }

    const VideoInfo& __stdcall GetVideoInfo() override {
        return m_vi;
    }

    bool __stdcall GetParity(int n) override {
        //return child->GetParity(n);
        return false;
    }

    intptr_t __stdcall SetCacheHints(int cachehints, int frame_range) override {
        switch (cachehints) {
        case CACHE_GETCHILD_CACHE_MODE:
            return CACHE_GENERIC;
        case CACHE_GETCHILD_AUDIO_MODE:
            return CACHE_AUDIO_NOTHING;
        case CACHE_GETCHILD_COST:
            return CACHE_COST_UNIT;
        case CACHE_GETCHILD_THREAD_MODE:
            return CACHE_THREAD_CLASS;
        case CACHE_GETCHILD_ACCESS_COST:
            return CACHE_ACCESS_RAND;
        default:;
        }
        return 0;
    }
};


//最多缓存 10张1080p尺寸的图片 相当于 80M内存
static cache::lru_cache <std::wstring, PVideoFrame> g_cacheWICImage(10);

EXTERN_C void FFMS_ClearImage() {
    g_cacheWICImage.clear();
}

PVideoFrame WICImage::GetFrame(int n, IScriptEnvironment* env) {

    if (g_cacheWICImage.exists(m_cache_path))
    {
        PVideoFrame f = g_cacheWICImage.get(m_cache_path);
        if (f == nullptr || f.m_ptr == nullptr)
        {
            g_cacheWICImage.clear();
        }
        else {
            return f;
        }
    }
    auto FilterFrame = read_image_file(this->m_file_path, env);
    if (FilterFrame) {
        g_cacheWICImage.put(m_cache_path, FilterFrame);
        WXLogW(L"Put Image Source %ws", m_cache_path.c_str());
    }
    return FilterFrame;
}

//音频转成波形图
AVSValue __cdecl Create_WICImage(AVSValue args, void* user_data, IScriptEnvironment* env) {
    auto timelineinfo = env->GetTimelineInfo();
    std::string filename = args[0].AsString("");
    return new WICImage(args[0].AsString(""),
        args[1].AsInt(timelineinfo->FrameWidth), args[2].AsInt(timelineinfo->FrameHeight), args[3].AsInt(0),
        args[4].AsInt(1), args[5].AsFloat(20),
        env);
}

//转场
class Transition : public GenericVideoFilter {
public:
    int m_left_overlap = 0;
    int m_right_overlap = 0;
    int m_mask_range = 0;
    std::string m_type = "";
    std::string m_param_path = "";
private:
    PClip _left_child;
    PClip _right_child;
    PClip _overlap_clip;
    VideoInfo _left_vi, _right_vi;
    void process_overlap(IScriptEnvironment* env);

    PClip create_transition(const PClip& at, const PClip& bt, int ft, IScriptEnvironment* env) const;
    PClip create_transition_avs(const PClip& at, const PClip& bt, int ft, IScriptEnvironment* env) const;
    PClip create_transition_image(const PClip& at, const PClip& bt, int ft, IScriptEnvironment* env) const;
    PClip create_transition_glsl(const PClip& at, const PClip& bt, int ft, IScriptEnvironment* env) const;
    PClip create_transition_newglsl(const PClip& at, const PClip& bt, int ft, IScriptEnvironment* env) const;

public:
    virtual ~Transition();
    explicit Transition(const PClip& child, const PClip& right_child,
        int left_overlap, int right_overlap,
        const char* type, const char* param, int mask_range,
        IScriptEnvironment* env);

    intptr_t __stdcall SetCacheHints(int cachehints, int frame_range) override {
        _right_child->SetCacheHints(cachehints, frame_range);
        return _left_child->SetCacheHints(cachehints, frame_range);
    }

    bool __stdcall GetParity(int n) override;
    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) override;
    void __stdcall GetAudio(void* buf, long long start, long long count, IScriptEnvironment* env) override;
};

AVSValue __cdecl Create_Transition(AVSValue args, void* user_data, IScriptEnvironment* env) {
    PClip left_child = args[0].AsClip();
    PClip right_child = args[1].AsClip();
    int left_overlap = std::max(0, args[2].AsInt(0));
    int right_overlap = std::max(0, args[3].AsInt(0));
    const char* _type = args[4].AsString("");
    const char* _param_path = args[5].AsString("");
    int mask_range = args[6].AsInt(50);
    if (!WXBase::Exists(_param_path)) {
        env->ThrowError("Transition: [%s] not exists: %s", _type, _param_path);
    }
    Transition* new_obj = new Transition(left_child, right_child, left_overlap, right_overlap,
        _type, _param_path, mask_range, env);
    return new_obj;
}

Transition::~Transition() {

}

Transition::Transition(const PClip& child, const PClip& right_child,
    const int left_overlap, const int right_overlap,
    const char* type, const char* param, const int mask_range,
    IScriptEnvironment* env) : base{ child,__FUNCTION__ }, _left_child{ child }, _right_child{ right_child },
    _left_vi{ child->GetVideoInfo() }, _right_vi{ right_child->GetVideoInfo() },
    m_left_overlap{ left_overlap }, m_right_overlap{ right_overlap },
    m_mask_range{ mask_range }, m_type{ type }, m_param_path{ param } {

    auto fps = env->GetTimelineInfo()->FrameRate;

    if (!_left_child->GetVideoInfo().IsRGB32()) {
        _left_child = env->Invoke("ConvertToRGB32", _left_child).AsClip();
    }
    const auto left_overlap_frames = std::to_string(_left_child->GetVideoInfo().num_frames - 1);
    _left_child = env->Invoke("FrameCache", AVSValueArray{ _left_child, left_overlap_frames.c_str() }).AsClip();
    _left_vi = _left_child->GetVideoInfo();
    _left_vi.SetFPS(lround(fps * 1001), 1001);

    if (!_right_child->GetVideoInfo().IsRGB32()) {
        _right_child = env->Invoke("ConvertToRGB32", _right_child).AsClip();
    }
    _right_child = env->Invoke("FrameCache", AVSValueArray{ _right_child, "0" }).AsClip();
    _right_vi = _right_child->GetVideoInfo();
    _right_vi.SetFPS(lround(fps * 1001), 1001);

    if (!(_left_vi.width == _right_vi.width && _left_vi.height == _right_vi.height)) {
        env->ThrowError("Transition: width & height must be same.");
    }

    vi = _left_child->GetVideoInfo();
    vi.SetFPS(lround(fps * 1001), 1001);
    vi.num_frames += _right_vi.num_frames;
    vi.num_audio_samples += _right_vi.num_audio_samples;

    process_overlap(env);
}

PVideoFrame Transition::GetFrame(int n, IScriptEnvironment* env) {
    if (!_overlap_clip) {
        return n < _left_vi.num_frames ? _left_child->GetFrame(n, env) : _right_child->GetFrame(n - _left_vi.num_frames, env);
    }

    if (n < _left_vi.num_frames - m_left_overlap) {
        return _left_child->GetFrame(n, env);
    }

    if (n >= _left_vi.num_frames + m_right_overlap) {
        return _right_child->GetFrame(n - _left_vi.num_frames, env);
    }

    //const auto now1 = std::chrono::system_clock().now();
    //std::cerr << utils::StringExt::toString(now1) << ", overlap_clip->GetFrame" << std::endl;

    return _overlap_clip->GetFrame(n - _left_vi.num_frames + m_left_overlap, env);


}

void Transition::GetAudio(void* buf, long long start, long long count, IScriptEnvironment* env) {
    if (start + count <= _left_vi.num_audio_samples) {
        _left_child->GetAudio(buf, start, count, env);
    }
    else if (start >= _left_vi.num_audio_samples) {
        _right_child->GetAudio(buf, start, count, env);
    }
    else {
        const auto bpas = vi.BytesPerAudioSample();
        const auto samples = static_cast<char*>(buf);

        const auto left_count = _left_vi.num_audio_samples - start;
        const auto right_count = start + count - _left_vi.num_audio_samples;

        _left_child->GetAudio(samples, start, left_count, env);
        _right_child->GetAudio(samples + left_count * bpas, 0, right_count, env);
    }
}

bool Transition::GetParity(int n) {
    return n < _left_vi.num_frames ? _left_child->GetParity(n) : _right_child->GetParity(n - _left_vi.num_frames);
}

void Transition::process_overlap(IScriptEnvironment* env) {

    const auto ft = m_left_overlap + m_right_overlap;

    if (m_left_overlap > 0 && m_right_overlap > 0) {
        auto atl = env->Invoke("Trim1", AVSValueArray{ _left_child, _left_vi.num_frames - m_left_overlap, _left_vi.num_frames - 1 }).AsClip();
        atl = env->Invoke("Cache", AVSValue(atl)).AsClip();


        /* const auto btl = env->Invoke("LoopFrames", AVSValueArray{ _right_child, _left_overlap, 0, 0 }).AsClip();

         const auto atr = env->Invoke("LoopFrames", AVSValueArray{ _left_child, _right_overlap, _left_vi.num_frames - 1, _left_vi.num_frames - 1 }).AsClip();*/
        auto btr = env->Invoke("Trim1", AVSValueArray{ _right_child, 0, m_right_overlap - 1 }).AsClip();
        btr = env->Invoke("Cache", AVSValue(btr)).AsClip();

        GLTransition* lefttrans = new GLTransition(atl, m_right_overlap, env);
        GLTransition* righttrans = new GLTransition(btr, -m_left_overlap, env);
        _overlap_clip = create_transition(lefttrans, righttrans, ft, env);
        return;
    }
    else if (m_left_overlap > 0) {
        auto at = env->Invoke("Trim1", AVSValueArray{ _left_child, _left_vi.num_frames - m_left_overlap, _left_vi.num_frames - 1 }).AsClip();

        _right_child = env->Invoke("Cache", AVSValue(_right_child)).AsClip();
        auto bt = env->Invoke("LoopFrames", AVSValueArray{ _right_child, m_left_overlap, 0, 0 }).AsClip();

        at = env->Invoke("Cache", AVSValue(at)).AsClip();


        _overlap_clip = create_transition(at, bt, ft, env);
    }
    else if (m_right_overlap > 0) {
        _left_child = env->Invoke("Cache", AVSValue(_left_child)).AsClip();
        auto at = env->Invoke("LoopFrames", AVSValueArray{ _left_child, m_right_overlap, _left_vi.num_frames - 1, _left_vi.num_frames - 1 }).AsClip();
        auto bt = env->Invoke("Trim1", AVSValueArray{ _right_child, 0, m_right_overlap - 1 }).AsClip();

        bt = env->Invoke("Cache", AVSValue(bt)).AsClip();

        _overlap_clip = create_transition(at, bt, ft, env);
    }
}

PClip Transition::create_transition(const PClip& at, const PClip& bt, const int ft, IScriptEnvironment* env) const {
    if (ft <= 0) {
        return bt;
    }

    enum type_value {
        ev_not_defined,
        ev_type_avs,
        ev_type_image,
        ev_type_glsl,
        ev_type_newglsl,
        ev_end
    };
    static std::map<std::string, type_value> map_type_values{
        { "avs", ev_type_avs },
        { "image", ev_type_image },
        { "glsl", ev_type_glsl },
        { "newglsl", ev_type_newglsl }
    };

    auto aFrameNum = at->GetVideoInfo().num_frames;
    auto bFrameNum = bt->GetVideoInfo().num_frames;
    if (aFrameNum < ft || bFrameNum < ft) {
        //Log_info(" aFrameNum %d < ft || bFrameNum %d < ft %d ", aFrameNum, bFrameNum, ft);
    }

    switch (map_type_values[m_type]) {
    case ev_type_avs: {
        const auto rt = create_transition_avs(at, bt, ft, env);
        return env->Invoke("InternalCache", rt).AsClip();
    }
                    break;
    case ev_type_image: {
        const auto rt = create_transition_image(at, bt, ft, env);
        return env->Invoke("InternalCache", rt).AsClip();
    }
                      break;
    case ev_type_glsl: {
        const auto rt = create_transition_glsl(at, bt, ft, env);
        return env->Invoke("InternalCache", rt).AsClip();
    }
                     break;
    case ev_type_newglsl: {
        const auto rt = create_transition_newglsl(at, bt, ft, env);
        return env->Invoke("InternalCache", rt).AsClip();
    }
                        break;
    default:;
    }

    env->ThrowError("Transition: Not support type: %s", m_type.c_str());
    return PClip{ };
}

PClip Transition::create_transition_avs(const PClip& at, const PClip& bt, const int ft, IScriptEnvironment* env) const {
    auto cat = env->Invoke("ConvertToYV12", at).AsClip();
    auto cbt = env->Invoke("ConvertToYV12", bt).AsClip();
    const auto w = vi.width;
    const auto h = vi.height;
    const auto resize = w % 4 != 0 || h % 4 != 0;
    if (resize) {
        cat = env->Invoke("BilinearResize", AVSValueArray{ cat, w - w % 4, h - h % 4 }).AsClip();
        cbt = env->Invoke("BilinearResize", AVSValueArray{ cbt, w - w % 4, h - h % 4 }).AsClip();
    }

    std::string filename = std::filesystem::path{ m_param_path }.stem().string();
    if (filename.find('#') == 0) {
        filename = filename.substr(8);
    }
    filename.insert(0, "TA_");
    auto rt = env->Invoke(filename.c_str(), AVSValueArray{ cat, cbt, ft }).AsClip();

    if (resize) {
        rt = env->Invoke("BilinearResize", AVSValueArray{ rt, w, h }).AsClip();
    }

    rt = env->Invoke("ConvertToRGB32", rt).AsClip();

    return rt;
}

PClip Transition::create_transition_image(const PClip& at, const PClip& bt, const int ft, IScriptEnvironment* env) const {
    const auto w = vi.width;
    const auto h = vi.height;
    const auto fps = static_cast<double>(vi.fps_numerator) / vi.fps_denominator;

    const auto mask_source = new WICImage(m_param_path.c_str(),
        w,
        h, 0, ft, fps, env);

    const std::initializer_list<AVSValue> mask_args = {
        mask_source,
        0,
        ft,
        "Levels",
        0,
        1.0,
        m_mask_range,
        0,
        255,
        255 - m_mask_range,
        1.0,
        255,
        0,
        255
    };
    const auto mask = env->Invoke("Animate1", AVSValueArray{ mask_args }).AsClip();

    const auto at_mask = env->Invoke("Mask", AVSValueArray{ at, mask }).AsClip();
    const auto rt = env->Invoke("Layer", AVSValueArray{ bt, at_mask }).AsClip();

    return rt;
}

PClip Transition::create_transition_glsl(const PClip& at, const PClip& bt, const int ft, IScriptEnvironment* env) const {

    const AVSValue clips = AVSValueArray{ at, bt };
    //return new SimpleGLFilter(clips, { _param_path }, 0, INT32_MAX, env);
    const auto rt = env->Invoke("GlLayer", AVSValueArray{ at, bt, 0, 2000000000, 0, 0, m_param_path.c_str() }).AsClip();
    return rt;
}

PClip Transition::create_transition_newglsl(const PClip& at, const PClip& bt, const int ft, IScriptEnvironment* env) const {

    const AVSValue clips = AVSValueArray{ at, bt };
    return new SimpleGLFilter(clips, { m_param_path }, 0, INT32_MAX, env);

}
