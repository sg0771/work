/*
将视频数据源统一为RGB32数据
*/
#include "avssources.h"
#include "./utils.h"

#include <algorithm>
#include <cassert>

#include <vector>
#include "./indexing.h"
#include "./track.h"
#include <Psapi.h>
#include "Utils.hpp"
#include "./videosource.h"
#include "./audiosource.h"


class AVSourceMap {
public:
    static cache::lru_cache <std::wstring, std::shared_ptr<FFMS_VideoSource>>& GetVMap(int index) {
        static cache::lru_cache <std::wstring, std::shared_ptr<FFMS_VideoSource>> s_mapVideoSource[MAX_CHANNEL];
        return s_mapVideoSource[index];
    }
    static cache::lru_cache <std::wstring, std::shared_ptr<FFMS_AudioSource>>& GetAMap() {
        static cache::lru_cache <std::wstring, std::shared_ptr<FFMS_AudioSource>> s_mapAudioSource(6);
        return s_mapAudioSource;
    }
};

EXTERN_C void FFMS_ClearSource() {
    for (size_t i = 0; i < MAX_CHANNEL; i++)
    {
        AVSourceMap::GetVMap(i).clear();
    }
    AVSourceMap::GetAMap().clear();
}

static AVPixelFormat CSNameToPIXFMT(const char* CSName, AVPixelFormat Default, bool HighBitDepth) {
    if (!CSName)
        return AV_PIX_FMT_NONE;
    std::string s = CSName;
    std::transform(s.begin(), s.end(), s.begin(), toupper);
    if (s == "")
        return Default;
    if (s == "YUV9" || s == "YUV410P8")
        return AV_PIX_FMT_YUV410P;
    if (s == "YV411" || s == "YUV411P8")
        return AV_PIX_FMT_YUV411P;
    if (s == "YV12" || s == "YUV420P8")
        return AV_PIX_FMT_YUV420P;
    if (s == "YV16" || s == "YUV422P8")
        return AV_PIX_FMT_YUV422P;
    if (s == "YV24" || s == "YUV444P8")
        return AV_PIX_FMT_YUV444P;
    if (s == "Y8" || s == "GRAY8")
        return AV_PIX_FMT_GRAY8;
    if (s == "YUY2")
        return AV_PIX_FMT_YUYV422;
    if (s == "RGB24")
        return AV_PIX_FMT_BGR24;
    if (s == "RGB32")
        return AV_PIX_FMT_RGB32;
    if (HighBitDepth) {
        if (s == "YUVA420P8")
            return AV_PIX_FMT_YUVA420P;
        if (s == "YUVA422P8")
            return AV_PIX_FMT_YUVA422P;
        if (s == "YUVA444P8")
            return AV_PIX_FMT_YUVA444P;
        if (s == "YUV420P16")
            return AV_PIX_FMT_YUV420P16;
        if (s == "YUVA420P16")
            return AV_PIX_FMT_YUVA420P16;
        if (s == "YUV422P16")
            return AV_PIX_FMT_YUV422P16;
        if (s == "YUVA422P16")
            return AV_PIX_FMT_YUVA422P16;
        if (s == "YUV444P16")
            return AV_PIX_FMT_YUV444P16;
        if (s == "YUVA444P16")
            return AV_PIX_FMT_YUVA444P16;
        if (s == "YUV420P10")
            return AV_PIX_FMT_YUV420P10;
        if (s == "YUVA420P10")
            return AV_PIX_FMT_YUVA420P10;
        if (s == "YUV422P10")
            return AV_PIX_FMT_YUV422P10;
        if (s == "YUVA422P10")
            return AV_PIX_FMT_YUVA422P10;
        if (s == "YUV444P10")
            return AV_PIX_FMT_YUV444P10;
        if (s == "YUVA444P10")
            return AV_PIX_FMT_YUVA444P10;
        if (s == "RGBP8")
            return AV_PIX_FMT_GBRP;
        if (s == "RGBP10")
            return AV_PIX_FMT_GBRP10;
        if (s == "RGBP12")
            return AV_PIX_FMT_GBRP12;
        if (s == "RGBP16")
            return AV_PIX_FMT_GBRP16;
        //if (s == "RGBPS")
        //    return AV_PIX_FMT_GBRPF32;
        if (s == "RGBAP8")
            return AV_PIX_FMT_GBRAP;
        if (s == "RGBAP10")
            return AV_PIX_FMT_GBRAP10;
        if (s == "RGBAP12")
            return AV_PIX_FMT_GBRAP12;
        if (s == "RGBAP16")
            return AV_PIX_FMT_GBRAP16;
        /*      if (s == "RGBAPS")
                  return AV_PIX_FMT_GBRAPF32;*/
        if (s == "Y10" || s == "GRAY10")
            return AV_PIX_FMT_GRAY10;
        if (s == "Y12" || s == "GRAY12")
            return AV_PIX_FMT_GRAY12;
        if (s == "Y16" || s == "GRAY16")
            return AV_PIX_FMT_GRAY16;
    }

    return AV_PIX_FMT_NONE;
}

static int GetSubSamplingH(const VideoInfo& vi) {
    if ((vi.IsYUV() || vi.IsYUVA()) && !vi.IsY() && vi.IsPlanar())
        return vi.GetPlaneHeightSubsampling(PLANAR_U);
    else
        return 0;
}

static int GetSubSamplingW(const VideoInfo& vi) {
    if ((vi.IsYUV() || vi.IsYUVA()) && !vi.IsY() && vi.IsPlanar())
        return vi.GetPlaneWidthSubsampling(PLANAR_U);
    else
        return 0;
}


AvisynthVideoSource::AvisynthVideoSource(int id, const char* SourceFile, int Track, FFMS_Index* Index,
    int FPSNum, int FPSDen, int Threads, int SeekMode, int RFFMode,
    int ResizeToWidth, int ResizeToHeight, const char* ResizerName,
    const char* ConvertToFormatName, const char* VarPrefix, IScriptEnvironment* Env)
    : IClip(__FUNCTION__), FPSNum(FPSNum)
    , FPSDen(FPSDen)
    , RFFMode(RFFMode)
    , VarPrefix(VarPrefix) {

    this->SetID(id);
    m_nID = id;

    Revert = false;
    this->FPSNum = FPSNum = (*Index)[Track].FPSNumerator;
    this->FPSDen = FPSDen = (*Index)[Track].FPSDenominator;
    //Log_info("this->FPSNum :%d, this->FPSDen:%d ", this->FPSNum, this->FPSDen);
    //Frames.TB.Den) / Frames.TB.Num
    VI = {};
    // check if the two functions we need for many bits are present
    VI.pixel_type = CS_BGR32;
    HighBitDepth = (VI.ComponentSize() == 2 && VI.IsY());

    ErrorInfo E;


    this->m_strUTF8 = utils::StringExt::base64_s2s(SourceFile);// std::string(SourceFile);
    this->m_strUTF16 = WXBase::UTF8ToUTF16(this->m_strUTF8.c_str());

    this->Track = Track;
    this->m_Index = Index;
    this->Threads = Threads;
    this->SeekMode = SeekMode;

    this->ResizeToWidth = ResizeToWidth;
    this->ResizeToHeight = ResizeToHeight;
    this->ResizerName = _strdup(ResizerName);
    this->ConvertToFormatName = _strdup(ConvertToFormatName);

    VI.fps_denominator = (*Index)[Track].FPSDenominator;// VP->FPSDenominator;
    VI.fps_numerator = (*Index)[Track].FPSNumerator;// VP->FPSNumerator;
    VI.num_frames = (*Index)[Track].size();// VP->NumFrames;

    double videoseconds = ((double)(VI.num_frames * VI.fps_denominator)) / (double)VI.fps_numerator;
    double tempseconds = 0;

    //Log_info("num_audio_samples:%d, audio_samples_per_second:%d", (*Index)[Track].num_audio_samples, (*Index)[Track].audio_samples_per_second );
    if ((*Index)[Track].num_audio_samples != 0) {
        tempseconds = ((double)(*Index)[Track].num_audio_samples) / ((double)(*Index)[Track].audio_samples_per_second);
    }


    if (tempseconds / videoseconds < 1.2 && videoseconds < tempseconds && tempseconds != std::numeric_limits<double>::infinity())
    {

        INT64 temp1 = ((INT64)(*Index)[Track].audio_samples_per_second) * ((INT64)VI.fps_denominator);

        VI.num_frames = av_rescale((*Index)[Track].num_audio_samples, VI.fps_numerator, temp1);
    }

    VI.pixel_type = CS_BGR32;


    VI.aspect_ratio = 1;
    if (((*Index)[Track].SARDen >= 1) && ((*Index)[Track].SARNum / (double)(*Index)[Track].SARDen != 0))
    {
        VI.aspect_ratio = (*Index)[Track].SARNum / (double)(*Index)[Track].SARDen;
    }

    //Log_info("videoseconds:%f, tempseconds:%f, VI.num_frames:%d,VI.fps_denominator:%d, VI.fps_numerator:%d ", videoseconds, tempseconds, VI.num_frames, VI.fps_denominator, VI.fps_numerator);

    original_frames = VI.num_frames;

    //if (VP->TopFieldFirst)
    if (FALSE)
        VI.image_type = IT_TFF;
    else
        VI.image_type = IT_BFF;
    VI.width = (*Index)[Track].Width;
    VI.height = (*Index)[Track].Height;

    // Crop to obey subsampling width/height requirements
    VI.width -= VI.width % (1 << GetSubSamplingW(VI));
    VI.height -= VI.height % (1 << (GetSubSamplingH(VI) + (RFFMode > 0 ? 1 : 0)));

}

AvisynthVideoSource::~AvisynthVideoSource() {
    WXLogW(L"[ID=%d] Video Source %ws Pop", this->GetID(), this->m_strUTF16.c_str());
}


void AvisynthVideoSource::InitOutputFormat(
    int ResizeToWidth, int ResizeToHeight, const char* ResizerName,
    const char* ConvertToFormatName, FFMS_VideoSource* V, IScriptEnvironment* Env) {

    ErrorInfo E;
    const FFMS_VideoProperties* VP = FFMS_GetVideoProperties(V);
    const FFMS_Frame* F = FFMS_GetFrame(V, 0, &E);
    if (!F)
        Env->ThrowError("FFVideoSource: %s", E.Buffer);

    std::vector<int> TargetFormats;
    if (HighBitDepth) {
        TargetFormats.push_back(FFMS_GetPixFmt("yuv420p16"));
        TargetFormats.push_back(FFMS_GetPixFmt("yuva420p16"));
        TargetFormats.push_back(FFMS_GetPixFmt("yuv422p16"));
        TargetFormats.push_back(FFMS_GetPixFmt("yuva422p16"));
        TargetFormats.push_back(FFMS_GetPixFmt("yuv444p16"));
        TargetFormats.push_back(FFMS_GetPixFmt("yuva444p16"));
        TargetFormats.push_back(FFMS_GetPixFmt("yuv420p10"));
        TargetFormats.push_back(FFMS_GetPixFmt("yuva420p10"));
        TargetFormats.push_back(FFMS_GetPixFmt("yuv422p10"));
        TargetFormats.push_back(FFMS_GetPixFmt("yuva422p10"));
        TargetFormats.push_back(FFMS_GetPixFmt("yuv444p10"));
        TargetFormats.push_back(FFMS_GetPixFmt("yuva444p10"));
        TargetFormats.push_back(FFMS_GetPixFmt("gbrpf32"));
        TargetFormats.push_back(FFMS_GetPixFmt("gbrapf32"));
        TargetFormats.push_back(FFMS_GetPixFmt("gbrp16"));
        TargetFormats.push_back(FFMS_GetPixFmt("gbrap16"));
        TargetFormats.push_back(FFMS_GetPixFmt("gbrp12"));
        TargetFormats.push_back(FFMS_GetPixFmt("gbrap12"));
        TargetFormats.push_back(FFMS_GetPixFmt("gbrp10"));
        TargetFormats.push_back(FFMS_GetPixFmt("gbrap10"));
        TargetFormats.push_back(FFMS_GetPixFmt("gray16"));
        TargetFormats.push_back(FFMS_GetPixFmt("gray12"));
        TargetFormats.push_back(FFMS_GetPixFmt("gray10"));
        TargetFormats.push_back(FFMS_GetPixFmt("yuva420p"));
        TargetFormats.push_back(FFMS_GetPixFmt("yuva422p"));
        TargetFormats.push_back(FFMS_GetPixFmt("yuva444p"));
    }
    TargetFormats.push_back(FFMS_GetPixFmt("yuv410p"));
    TargetFormats.push_back(FFMS_GetPixFmt("yuv411p"));
    TargetFormats.push_back(FFMS_GetPixFmt("yuv420p"));
    TargetFormats.push_back(FFMS_GetPixFmt("yuv422p"));
    TargetFormats.push_back(FFMS_GetPixFmt("yuv444p"));
    TargetFormats.push_back(FFMS_GetPixFmt("gray8"));
    TargetFormats.push_back(FFMS_GetPixFmt("yuyv422"));
    TargetFormats.push_back(FFMS_GetPixFmt("bgra"));

    // Remove unsupported formats from list so they don't appear as an early termination
    TargetFormats.erase(std::remove(TargetFormats.begin(), TargetFormats.end(), -1), TargetFormats.end());

    TargetFormats.push_back(-1);

    // PIX_FMT_NV21 is misused as a return value different to the defined ones in the function
    AVPixelFormat TargetPixelFormat = CSNameToPIXFMT(ConvertToFormatName, AV_PIX_FMT_NV21, HighBitDepth);
    if (TargetPixelFormat == AV_PIX_FMT_NONE)
        Env->ThrowError("FFVideoSource: Invalid colorspace name specified");

    if (TargetPixelFormat != AV_PIX_FMT_NV21) {
        TargetFormats.clear();
        TargetFormats.push_back(TargetPixelFormat);
        TargetFormats.push_back(-1);
    }

    if (ResizeToWidth <= 0)
        ResizeToWidth = F->EncodedWidth;

    if (ResizeToHeight <= 0)
        ResizeToHeight = F->EncodedHeight;

    if (FFMS_SetOutputFormatV2(V, ResizeToWidth, ResizeToHeight, &E))

        Env->ThrowError("FFVideoSource: No suitable output format found1");

    F = FFMS_GetFrame(V, 0, &E);
    TargetFormats.clear();
    TargetFormats.push_back(F->ConvertedPixelFormat);
    TargetFormats.push_back(-1);

    // This trick is required to first get the "best" default format and then set only that format as the output
    if (FFMS_SetOutputFormatV2(V, ResizeToWidth, ResizeToHeight, &E))
        Env->ThrowError("FFVideoSource: No suitable output format found2");

    F = FFMS_GetFrame(V, 0, &E);

    VI.pixel_type = CS_BGR32;

    if (RFFMode > 0 && ResizeToHeight != F->EncodedHeight)
        Env->ThrowError("FFVideoSource: Vertical scaling not allowed in RFF mode");

    if (RFFMode > 0 && TargetPixelFormat != AV_PIX_FMT_NV21)
        Env->ThrowError("FFVideoSource: Only the default output colorspace can be used in RFF mode");


    if (VP->TopFieldFirst)
        VI.image_type = IT_TFF;
    else
        VI.image_type = IT_BFF;

    VI.width = F->ScaledWidth;
    VI.height = F->ScaledHeight;

    // Crop to obey subsampling width/height requirements
    VI.width -= VI.width % (1 << GetSubSamplingW(VI));
    VI.height -= VI.height % (1 << (GetSubSamplingH(VI) + (RFFMode > 0 ? 1 : 0)));

}

static void BlitPlane(const FFMS_Frame* Frame, PVideoFrame& Dst, IScriptEnvironment* Env, int Plane, int PlaneId) {
    Env->BitBlt(Dst->GetWritePtr(PlaneId), Dst->GetPitch(PlaneId),
        Frame->Data[Plane], Frame->Linesize[Plane],
        Dst->GetRowSize(PlaneId), Dst->GetHeight(PlaneId));
}

void AvisynthVideoSource::OutputFrame(const FFMS_Frame* Frame, PVideoFrame& Dst, IScriptEnvironment* Env) {
    if (VI.IsPlanar()) {
        BlitPlane(Frame, Dst, Env, 0, VI.IsRGB() ? PLANAR_G : PLANAR_Y);
        if (HighBitDepth ? !VI.IsY() : !VI.IsY8()) {
            BlitPlane(Frame, Dst, Env, 1, VI.IsRGB() ? PLANAR_B : PLANAR_U);
            BlitPlane(Frame, Dst, Env, 2, VI.IsRGB() ? PLANAR_R : PLANAR_V);
        }
        if (VI.IsYUVA() || VI.IsPlanarRGBA())
            BlitPlane(Frame, Dst, Env, 3, PLANAR_A);
    }
    else if (VI.IsYUY2()) {
        BlitPlane(Frame, Dst, Env, 0, 0);
    }
    else if (VI.IsRGB24() || VI.IsRGB32()) {
        Env->BitBlt(
            Dst->GetWritePtr() + Dst->GetPitch() * (Dst->GetHeight() - 1), -Dst->GetPitch(),
            Frame->Data[0], Frame->Linesize[0],
            Dst->GetRowSize(), Dst->GetHeight());
    }
    else {
        assert(false);
    }
}

static void BlitField(const FFMS_Frame* Frame, PVideoFrame& Dst, IScriptEnvironment* Env, int Plane, int PlaneId, int Field) {
    Env->BitBlt(
        Dst->GetWritePtr(PlaneId) + Dst->GetPitch(PlaneId) * Field, Dst->GetPitch(PlaneId) * 2,
        Frame->Data[Plane] + Frame->Linesize[Plane] * Field, Frame->Linesize[Plane] * 2,
        Dst->GetRowSize(PlaneId), Dst->GetHeight(PlaneId) / 2);
}

void AvisynthVideoSource::OutputField(const FFMS_Frame* Frame, PVideoFrame& Dst, int Field, IScriptEnvironment* Env) {
    const FFMS_Frame* SrcPicture = Frame;
    if (VI.IsPlanar()) {
        BlitField(Frame, Dst, Env, 0, VI.IsRGB() ? PLANAR_G : PLANAR_Y, Field);
        if (HighBitDepth ? !VI.IsY() : !VI.IsY8()) {
            BlitField(Frame, Dst, Env, 1, VI.IsRGB() ? PLANAR_B : PLANAR_U, Field);
            BlitField(Frame, Dst, Env, 2, VI.IsRGB() ? PLANAR_R : PLANAR_V, Field);
        }
        if (VI.IsYUVA() || VI.IsPlanarRGBA())
            BlitField(Frame, Dst, Env, 3, PLANAR_A, Field);
    }
    else if (VI.IsYUY2()) {
        BlitField(Frame, Dst, Env, 0, 0, Field);
    }
    else if (VI.IsRGB24() || VI.IsRGB32()) {
        Env->BitBlt(
            Dst->GetWritePtr() + Dst->GetPitch() * (Dst->GetHeight() - 1 - Field), -Dst->GetPitch() * 2,
            SrcPicture->Data[0] + SrcPicture->Linesize[0] * Field, SrcPicture->Linesize[0] * 2,
            Dst->GetRowSize(), Dst->GetHeight() / 2);
    }
    else {
        assert(false);
    }
}
void AvisynthVideoSource::ClearFFMS_VideoSource()
{

}

void AvisynthVideoSource::SupportRevert(bool flag)
{
    Revert = flag;
    if (AVSourceMap::GetVMap(m_nID).exists(this->m_strUTF16))
    {
        FFMS_SupportRevert(AVSourceMap::GetVMap(m_nID).get(this->m_strUTF16).get(), flag);
    }
    return;
}



PVideoFrame AvisynthVideoSource::GetFrame(int n, IScriptEnvironment* Env) {

    if (!AVSourceMap::GetVMap(m_nID).exists(m_strUTF16))
    {
        ErrorInfo E;
        FFMS_VideoSource* videosourceptr = FFMS_CreateVideoSource(m_strUTF8.c_str(), Track, m_Index, Threads, SeekMode, &E);
        if (videosourceptr == NULL) {
            return NULL;
        }
        std::shared_ptr< FFMS_VideoSource> obj = std::shared_ptr< FFMS_VideoSource>(videosourceptr,
            [](FFMS_VideoSource* pp) {
                FFMS_DestroyVideoSource(pp);
            });
        AVSourceMap::GetVMap(m_nID).put(this->m_strUTF16, obj);

        WXLogW(L"[ID=%d] Video Source %ws Push", this->GetID(), this->m_strUTF16.c_str());

        FFMS_SupportRevert(videosourceptr, Revert);

        const FFMS_VideoProperties* VP = FFMS_GetVideoProperties(videosourceptr);
        if (original_frames > VP->NumFrames)
            original_frames = VP->NumFrames;
        InitOutputFormat(ResizeToWidth, ResizeToHeight, ResizerName, ConvertToFormatName, videosourceptr, Env);
    }
    FFMS_VideoSource* videosourceptr = AVSourceMap::GetVMap(m_nID).get(this->m_strUTF16).get();


    PVideoFrame Dst = Env->NewVideoFrame(VI);


    if (Dst == nullptr || Dst.m_ptr == nullptr) {
        return nullptr;
    }
    ErrorInfo E;
    if (RFFMode > 0) {

        const FFMS_Frame* Frame = FFMS_GetFrame(videosourceptr, std::min(FieldList[n].Top, FieldList[n].Bottom), &E);

        if (Frame == nullptr)
            Env->ThrowError("FFVideoSource: %s", E.Buffer);
        if (FieldList[n].Top == FieldList[n].Bottom) {
            OutputFrame(Frame, Dst, Env);
        }
        else {
            int FirstField = std::min(FieldList[n].Top, FieldList[n].Bottom) == FieldList[n].Bottom;
            OutputField(Frame, Dst, FirstField, Env);
            Frame = FFMS_GetFrame(videosourceptr, std::max(FieldList[n].Top, FieldList[n].Bottom), &E);
            if (Frame == nullptr)
                Env->ThrowError("FFVideoSource: %s", E.Buffer);
            OutputField(Frame, Dst, !FirstField, Env);
        }
        Env->SetVar(Env->Sprintf("%s%s", this->VarPrefix, "FFVFR_TIME"), -1);
        Env->SetVar(Env->Sprintf("%s%s", this->VarPrefix, "FFPICT_TYPE"), static_cast<int>('U'));
    }
    else {
        const FFMS_Frame* Frame;

        if (FPSNum > 0 && FPSDen > 0) {
            Frame = FFMS_GetFrameByTime(videosourceptr, FFMS_GetVideoProperties(videosourceptr)->FirstTime +
                (double)(n * (int64_t)FPSDen) / FPSNum, &E);
            Env->SetVar(Env->Sprintf("%s%s", this->VarPrefix, "FFVFR_TIME"), -1);
        }
        else {
            n = std::min(std::max(n, 0), original_frames - 1);
            Frame = FFMS_GetFrame(videosourceptr, n, &E);
            FFMS_Track* T = FFMS_GetTrackFromVideo(videosourceptr);

            const FFMS_TrackTimeBase* TB = FFMS_GetTimeBase(T);
            Env->SetVar(Env->Sprintf("%s%s", this->VarPrefix, "FFVFR_TIME"), static_cast<int>(FFMS_GetFrameInfo(T, n)->PTS * static_cast<double>(TB->Num) / TB->Den));
        }
        if (Frame == nullptr)
            Env->ThrowError("FFVideoSource: %s", E.Buffer);

        Env->SetVar(Env->Sprintf("%s%s", this->VarPrefix, "FFPICT_TYPE"), static_cast<int>(Frame->PictType));

        OutputFrame(Frame, Dst, Env);
    }
    return Dst;
}

bool AvisynthVideoSource::GetParity(int n) {
    return VI.image_type == IT_TFF;
}

static int64_t GetDelay(int DelayMode, const FFMS_Index* Index, int SampleRate, FFMS_Track& Frames)
{

    int64_t Delay = 0;
    if (DelayMode >= 0) {
        const FFMS_Track& VTrack = (*Index)[DelayMode];
        if (VTrack.size() == 0)
        {
            return 0;
        }
        Delay = -(VTrack[0].PTS * VTrack.TB.Num * SampleRate / (VTrack.TB.Den * 1000));
    }

    //Log_info("DelayMode:%d, Delay:%lld", DelayMode, Delay);

    if (Frames.HasTS) {
        int i = 0;
        if (Frames.size() == 0)
        {
            return 0;
        }
        while (Frames[i].PTS == AV_NOPTS_VALUE) ++i;
        Delay += Frames[i].PTS * Frames.TB.Num * SampleRate / (Frames.TB.Den * 1000);
        for (; i > 0; --i)
            Delay -= Frames[i].SampleCount;
    }
    return Delay;


}

AvisynthAudioSource::AvisynthAudioSource(const char* SourceFile, int Track, FFMS_Index* Index,
    int AdjustDelay, const char* VarPrefix, IScriptEnvironment* Env) : IClip(__FUNCTION__) {
    m_VI = {};
    this->m_strUTF8 = utils::StringExt::base64_s2s(SourceFile);// std::string(SourceFile);
    this->m_strUTF16 = WXBase::UTF8ToUTF16(this->m_strUTF8.c_str());
    m_VI.nchannels = 2;// AP->Channels;

    int64_t delay = GetDelay(0, Index, (*Index)[Track].audio_samples_per_second, (*Index)[Track]);

    m_VI.num_audio_samples = ((*Index)[Track].back()).SampleStart + ((*Index)[Track].back()).SampleCount + delay;// (*Index)[Track].num_audio_samples;

    m_VI.audio_samples_per_second = (*Index)[Track].audio_samples_per_second;
    m_VI.sample_type = (*Index)[Track].sample_type;

    this->m_Track = Track;
    this->m_Index = Index;
    this->m_AdjustDelay = AdjustDelay;
    this->m_VarPrefix = VarPrefix;
}

AvisynthAudioSource::~AvisynthAudioSource() {

}

void AvisynthAudioSource::GetAudio(void* Buf, __int64 Start, __int64 Count, IScriptEnvironment* Env) {
    if (!AVSourceMap::GetAMap().exists(this->m_strUTF16))
    {
        ErrorInfo E;
        FFMS_AudioSource* audioptr = FFMS_CreateAudioSource(this->m_strUTF8.c_str(), m_Track, m_Index, m_AdjustDelay, &E);
        if (NULL == audioptr) {
            return;
        }
        std::shared_ptr< FFMS_AudioSource> obj = std::shared_ptr< FFMS_AudioSource>(
            audioptr, [](FFMS_AudioSource* pp) {
                FFMS_DestroyAudioSource(pp);
            }
        );
        AVSourceMap::GetAMap().put(this->m_strUTF16, obj);
        //WXLogW(L"Put Audio Source %ws", WXBase::UTF8ToUTF16(this->m_Source.c_str()).c_str());
        const FFMS_AudioProperties* AP = FFMS_GetAudioProperties(audioptr);
        Env->SetVar(Env->Sprintf("%s%s", m_VarPrefix.c_str(), "FFCHANNEL_LAYOUT"), static_cast<int>(AP->ChannelLayout));
        Env->SetGlobalVar("FFVAR_PREFIX", m_VarPrefix.c_str());
    }
    ErrorInfo E;
    FFMS_GetAudio(AVSourceMap::GetAMap().get(this->m_strUTF16).get(), Buf, Start, Count, &E);
}



