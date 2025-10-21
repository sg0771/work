/*

功能 : 将采集或者解码的音视频数据编码生成MP4等格式的文件
说明 : GIF 格式不支持恒定帧率
       支持H264 H265硬编码(需要显卡支持)
	   编码H264+AAC时会缓存一个FLV临时文件，如果录制中断退出，可以用恢复工具将FLV转码为MP4文件
	   FLV 是经过修改的格式，可以用WXMedia内部工具播放、转码，其它第三方工具如ffmpeg、vlc不支持对应的处理
	   通过CPU核数来划分电脑性能层次，针对不同性能的电脑使用不同的编码参数来优化画质
	   如果是需要避免涂鸦的锯齿问题，需要传入抗涂鸦参数

*/

#ifndef _FFMPEG_MUXER_H_
#define _FFMPEG_MUXER_H_

#include <WXMediaCpp.h>
#include "libimagequant.h" //pngquant

class FfmpegMuxer {

	int m_crf_h264 = 24;//H264默认 CRF
	int m_crf_h265 = 29;//H265默认 CRF
	int m_crf_vp8 = 15; //VP8默认 CRF
	int m_crf_vp9 = 29; //VP9默认 CRF
	int m_crf_av1 = 29; //AV1默认 CRF

	int m_nVideoScale = 100;//录制前缩放

	int m_iMachLevel = LEVEL_BETTER;//默认机器性能

	WXVideoConvert   m_gifConvert;
	WXVideoFrame m_tempFrame;
	liq_attr*  m_liq = nullptr;
	uint8_t**  m_row_pointers = nullptr;
	BOOL       m_bGif = FALSE;
	void       GifImpl(AVFrame* srcFrame, AVFrame* dstFrame);

	WXString   m_strFileName; //文件名字

	BOOL     m_bPause = FALSE;

	int        m_bCFR = 0;//是否CFR模式
	BOOL       m_bHasVideo = FALSE;
	BOOL       m_bHasAudio = FALSE;
	int64_t    m_ptsVideo = 0;
	int64_t    m_ptsAudio = 0;
	int64_t    m_ptsDelay = 0;
	BOOL       m_bHardwareEncoder = FALSE;//H264硬编码

	int64_t    m_iVideoFrame1 = 0;//真实编码帧
	int64_t    m_iVideoFrame2 = 0;//实际输出帧，可能包括空包和重复帧
	int64_t    m_iVideoDataSize = 0;//视频实际数据量
	int64_t    m_iAudioFrame = 0;//音频包数，用于计算时间戳
	int64_t    m_iAudioDataSize = 0;//音频实际数据量

	int64_t    m_nTime = 3750;
	int        m_videoMode = MODE_NORMAL;
	int        m_iWidth = 0;
	int        m_iHeight = 0;
	int        m_iFps = 24;
	int64_t    m_iVideoBitrate = 0;
	int64_t    m_ptsLastVideo = 0;
	BOOL       m_bX264AntiAliasing = FALSE;//抗锯齿，输入格式YUV444，不能是Baseline
	int        m_nX264AntiAliasing = 0;
	int        m_iAudioFrameSize = 0;//音频编码每一帧有多少字节，AAC 一般是 1024 * 2 * 2;
	int        m_iSampleRate = 48000;
	int        m_iChannel = 2;
	int        m_iAudioBitrate = 128000;
	WXString   m_strVideoCodec = L"";
	WXString   m_strAudioCodec = L"";

	int m_bNotUseTemp = 0;// 不录屏不需要临时文件

	BOOL m_bUseTemp = FALSE; //是否使用临时文件
	WXString   m_strType = L"";//文件类型,默认是MP4
	WXString   m_strTemp = L"";//视频临时文件

	BOOL m_bXWS_Temp = FALSE;
	BOOL m_bMKV_Temp = FALSE;

	WXString   m_strExt;//输出文件扩展名	 A.MP4.XWS--->XWS
	WXString   m_strExt2;//输出文件扩展名    a.MP4.XWS-->MP4

	int        m_nFlags = 0;
	AVFormatContext *m_pFmtCtx = nullptr;
	AVStream*  m_pVideoStream = nullptr;
	AVRational m_tbVideo{ 1,90000 };//MS

	AVRational m_tbMS{ 1,1000 };//MS

	AVStream*  m_pAudioStream = nullptr;
	AVRational m_tbAudio{ 1,90000 };//MS
	AVCodecContext*  m_pVideoCtx = nullptr;
	AVCodecContext*  m_pAudioCtx = nullptr;
	WXAudioFrame     m_pAudioS16Frame;//目标格式
	WXAudioFrame     m_pAudioFrame;//目标格式
	WXAudioConvert*  m_AudioConvert = nullptr;
	WXFifo           m_audioFifo;//输入声音 每个包  10ms  SampleRate+Channel+PCM16

	//指定编码器
	AVCodecID        m_forceVideoID = AV_CODEC_ID_NONE;
	AVCodecID        m_forceAudioID = AV_CODEC_ID_NONE;

	WXLocker         m_mutex;//ffmpeg 写入锁

	WXVideoFrame     m_videoFrame;
	WXVideoFrame     m_I420Frame;
	WXVideoFrame     m_RGBAFrame;

	WXVideoConvert   m_VideoConvert;
	AVPixelFormat    m_dstFmt = AV_PIX_FMT_NONE;

	BOOL   OpenAudioEncoder(enum AVCodecID codec_id);
	BOOL   OpenVideoEncoder(enum AVCodecID codec_id);

	AVCodec* m_pVideoCodec = nullptr;
	void   OpenH264Encoder();
	void   OpenH265Encoder();
	void   OpenVP8Encoder();
	void   OpenVP9Encoder();
	void   OpenAV1Encoder();

	int    EncodeFrameImpl(AVFrame*  pFrame, int bVideo);
	void   WritePacketImpl(AVPacket* pPkt, int bVideo);//数据包
public:
	FfmpegMuxer();
	virtual ~FfmpegMuxer();
public:
	WXCTSTR GetName() {
		return m_strFileName.str();
	}
	void Pause(int bPause) {
		m_bPause = !!bPause;
	}
	int64_t GetVideoSize();
	int64_t GetVideoTime();
	int64_t GetAudioSize();
	int64_t GetAudioTime();
	int     GetAudioFrameSize();
	int64_t GetFileSize();
	int64_t GetVideoFrameSize() {
		return m_iVideoFrame1;
	}

	void    SetVideoConfig(int nWidth, int nHeight, int nFps,
		        int nBitrate, int nMode, int bHardwareCodec,
		        int bForceFps, int bAntiAliasing,int nVideoSacle = 100, WXCTSTR strVideoCodec = nullptr);
	void    SetAudioConfig(int nSampleRate, int nChannel, int nBitrate, WXCTSTR strAudioCodec = nullptr);
public:
	int     Open(WXCTSTR wszFileName, int notUseTemp = 0);//启动，成功返回0，失败返回错误值
	int     Close();//关闭,成功返回0，失败返回错误值
	void    WriteAudioFrame(uint8_t *pBuf, int nSize);//48000 2ch 10ms S16音频数据输入
	void    WriteVideoFrame(AVFrame * pFrame);//AVFrame 数据输入， 可能来自于视频解码
};

#endif
