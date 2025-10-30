

#ifndef FFVIDEOSOURCE_H
#define FFVIDEOSOURCE_H

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/display.h>
#include <libavutil/mastering_display_metadata.h>
}

#include <vector>

#include "track.h"
#include "utils.h"

#include <avisynth/avisynth.h>

struct FFMS_VideoSource {
public:
    static std::vector<std::wstring>& GetNotSupportDXVA() {
		static std::vector<std::wstring> g_notSupportDXVA;
		return g_notSupportDXVA;
    }

private:
    BOOL m_bGetHWFmt = FALSE;
    enum AVPixelFormat m_hw_pix_fmt = AV_PIX_FMT_NONE;
    AVBufferRef* m_hw_device_ctx = NULL;

    static enum AVPixelFormat get_hw_format(AVCodecContext* ctx, const enum AVPixelFormat* pix_fmts) {
        FFMS_VideoSource* pThis = (FFMS_VideoSource*)ctx->opaque;
        for (const enum AVPixelFormat* p = pix_fmts; *p != AV_PIX_FMT_NONE; p++) {
            if (*p == pThis->m_hw_pix_fmt)
                return *p;
        }
        return AV_PIX_FMT_NONE;
    }

    //-------------------------------------------------------
    SwsContext *m_pSWS = nullptr;
    int Delay = 0;
    int DelayCounter = 0;
    int InitialDecode = 1;
    bool PAFFAdjusted = false;

    int LastFrameHeight = -1;
    int LastFrameWidth = -1;
    AVPixelFormat LastFramePixelFormat = AV_PIX_FMT_NONE;

    int TargetHeight = -1;
    int TargetWidth = -1;
    std::vector<AVPixelFormat> TargetPixelFormats;

    AVPixelFormat m_OutputFormat = AV_PIX_FMT_RGB32;
    AVColorRange OutputColorRange = AVCOL_RANGE_UNSPECIFIED;
    AVColorSpace OutputColorSpace = AVCOL_SPC_UNSPECIFIED;
    bool OutputColorRangeSet = false;
    bool OutputColorSpaceSet = false;

    int OutputColorPrimaries = -1;
    int OutputTransferCharateristics = -1;
    int OutputChromaLocation = -1;

    bool InputFormatOverridden = false;
    AVPixelFormat m_InputFormat = AV_PIX_FMT_NONE;
    AVColorRange InputColorRange = AVCOL_RANGE_UNSPECIFIED;
    AVColorSpace InputColorSpace = AVCOL_SPC_UNSPECIFIED;

    uint8_t *SWSFrameData[4] = {};
    int SWSFrameLinesize[4] = {};

    void DetectInputFormat(AVFrame* frame);
    bool HasPendingDelayedFrames();

    FFMS_VideoProperties VP = {};
    FFMS_Frame LocalFrame = {};
    AVFrame* m_pHwFrame = nullptr;
    AVFrame * m_pDecodeFrame = nullptr;
    AVFrame *m_pLastDecodedFrame = nullptr;
    int LastFrameNum = 0;
    FFMS_Index &Index;
    FFMS_Track Frames;
    int VideoTrack;
    int CurrentFrame = 1;
    int DecodingThreads;
    AVCodecContext *CodecContext = nullptr;
    AVFormatContext *FormatContext = nullptr;
    int SeekMode;
    bool SeekByPos = false;
    int64_t PosOffset = 0;
	
	int N;
	void ReAdjustOutputFormat(AVFrame *Frame);
    FFMS_Frame *OutputFrame(AVFrame *Frame);
    void SetVideoProperties();
    bool DecodePacket(AVPacket *Packet);
    void DecodeNextFrame(int64_t &PTS, int64_t &Pos);
    bool SeekTo(int n, int SeekOffset);
    int Seek(int n);
    int ReadFrame(AVPacket *pkt);
    void Free();
    static void SanityCheckFrameForData(AVFrame *Frame);
	bool needCache(AVFrame* frame);
	bool hasCache(AVFrame* frame);
public:
	#ifdef CACHEVIDEO1
	std::list<VideoBlock > CacheVideoBlocks;
	bool IsReverse;
	AVFrame* GetFromCache(int n);
	void ClearCache();
    #endif

    std::string  m_strUTF = "";
    std::wstring m_strUTF16 = L"";

    FFMS_VideoSource(const char *SourceFile, FFMS_Index &Index, int Track, int Threads, int SeekMode);
    ~FFMS_VideoSource();
    const FFMS_VideoProperties& GetVideoProperties() { return VP; }
    FFMS_Track *GetTrack() { return &Frames; }
    FFMS_Frame *GetFrame(int n);
    void GetFrameCheck(int n);
    FFMS_Frame *GetFrameByTime(double Time);
    void SetOutputFormat(int Width, int Height);
    void ResetOutputFormat();
    void SetInputFormat(int ColorSpace, int ColorRange, AVPixelFormat Format);
    void ResetInputFormat();
};

#endif
