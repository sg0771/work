/*

功能 : 将采集或者解码的音视频数据编码生成MP4等格式的文件


*/

#include "FfmpegMuxer.h"


//不同模式下非H264、H265 视频编码器对应的压缩比
const int64_t s_nVideoCompress[3][3] = { {130,110,90} ,{120,100,80},{80,65,50} };

extern void  GetWaveDataImpl(uint8_t* data, int size);

//PAL8 调色板
void FfmpegMuxer::GifImpl(AVFrame* srcFrame, AVFrame* dstFrame) {
	for (int row = 0; row < m_iHeight; row++) {
		m_row_pointers[row] = dstFrame->data[0] + row * dstFrame->linesize[0];
	}
	if (srcFrame->format == AV_PIX_FMT_RGB32) {
		liq_attr_process(m_liq, srcFrame->data[0], m_row_pointers, dstFrame->data[1]);
	}else{//先转换为RGB32 才能再转PLA8
		if (srcFrame->format == AV_PIX_FMT_YUV420P) {
			libyuv::I420ToARGB(
				srcFrame->data[0], srcFrame->linesize[0],
				srcFrame->data[1], srcFrame->linesize[1],
				srcFrame->data[2], srcFrame->linesize[2],
				m_tempFrame.GetFrame()->data[0], m_tempFrame.GetFrame()->linesize[0],
				m_iWidth, m_iHeight
			);
		}else if (srcFrame->format == AV_PIX_FMT_YUVJ420P) {
			libyuv::I420ToARGB(
				srcFrame->data[0], srcFrame->linesize[0],
				srcFrame->data[1], srcFrame->linesize[1],
				srcFrame->data[2], srcFrame->linesize[2],

				m_tempFrame.GetFrame()->data[0], m_tempFrame.GetFrame()->linesize[0],
				m_iWidth, m_iHeight);
		}else{
			m_gifConvert.Convert(srcFrame, m_tempFrame.GetFrame());
		}
		liq_attr_process(m_liq, m_tempFrame.GetFrame()->data[0], m_row_pointers, dstFrame->data[1]);
	}
}


void  FfmpegMuxer::WritePacketImpl(AVPacket *pkt, int bVideo) {

	if (bVideo) {
		m_iVideoDataSize += pkt->size;//视频数据
		m_iVideoFrame2++;
		m_ptsVideo = pkt->pts * 1000 / m_tbVideo.den;//视频时间戳，单位MS
		//从输入时间戳(毫秒或者 1/90000s)转换到输出时间戳
		av_packet_rescale_ts(pkt, m_tbVideo, m_pVideoStream->time_base);
		pkt->stream_index = m_pVideoStream->index;
	}
	else {
		m_iAudioDataSize += pkt->size;
		m_ptsAudio = pkt->pts * 1000 / m_iSampleRate; //音频时间戳，单位MS
		av_packet_rescale_ts(pkt, m_tbAudio, m_pAudioStream->time_base);//转换音频时间戳
		pkt->stream_index = m_pAudioStream->index;
	}

	WXAutoLock al(m_mutex);
	av_interleaved_write_frame(m_pFmtCtx, pkt);
}

//音频编码输出到文件
int    FfmpegMuxer::EncodeFrameImpl(AVFrame* frame, int bVideo) {
	AVCodecContext* pCtx = bVideo ? m_pVideoCtx : m_pAudioCtx;
	if (pCtx == nullptr)return -1;
	int ret = avcodec_send_frame(pCtx, frame);
	if (ret >= 0) {
		int got = -1;
		while (TRUE) {
			AVPacket pkt;
			av_init_packet(&pkt);
			ret = avcodec_receive_packet(pCtx, &pkt);
			if (ret >= 0) {
				got = 0;
				WritePacketImpl(&pkt, bVideo);
			}else {
				break;
			}
		}
		return got;
	}
	return -1;//avcodec_send_frame error
}

void    FfmpegMuxer::WriteVideoFrame(AVFrame *srcFrame) {
	if (m_bPause)
		return;
	if (srcFrame == nullptr || m_pVideoCtx == nullptr)return;
	m_iVideoFrame1++;
	m_ptsVideo = srcFrame->pts;//毫秒时间戳
	srcFrame->pts *= 90;///毫秒转 1/90000s
	AVFrame *dstFrame = srcFrame;
	//视频拼接的时候，后面的文件分辨率可能和第一个文件不一样
	//限制输入格式为AV_PIX_FMT_YUV420P I420格式
	if (srcFrame->width != m_iWidth || srcFrame->height != m_iHeight) {

		if (m_nVideoScale == 100) {
			int dx = 0;
			int dy = 0;
			GetXY(srcFrame->width, srcFrame->height, m_iWidth, m_iHeight, dx, dy);
			dstFrame = m_I420Frame.GetFrame();//输出目标
			memset(dstFrame->data[0], 0, dstFrame->linesize[0] * m_iHeight);
			memset(dstFrame->data[1], 128, dstFrame->linesize[1] * m_iHeight / 2);
			memset(dstFrame->data[2], 128, dstFrame->linesize[2] * m_iHeight / 2);
			libyuv::I420Scale(srcFrame->data[0], srcFrame->linesize[0],
				srcFrame->data[1], srcFrame->linesize[1],
				srcFrame->data[2], srcFrame->linesize[2],
				srcFrame->width, srcFrame->height,
				dstFrame->data[0] + dx + dy * dstFrame->linesize[0],
				dstFrame->linesize[0],
				dstFrame->data[1] + dx / 2 + (dy / 2) * dstFrame->linesize[1],
				dstFrame->linesize[1],
				dstFrame->data[2] + dx / 2 + (dy / 2) * dstFrame->linesize[2],
				dstFrame->linesize[2],
				m_iWidth - 2 * dx, m_iHeight - 2 * dy,
				libyuv::FilterMode::kFilterBilinear);

			dstFrame = m_videoFrame.GetFrame();//输出目标
			av_frame_make_writable(dstFrame);//GIF  RGB8
			dstFrame->pts = srcFrame->pts;
			dstFrame->pict_type = srcFrame->pict_type;
			dstFrame->key_frame = srcFrame->key_frame;
			m_VideoConvert.Convert(m_I420Frame.GetFrame(), dstFrame);
		}
		else {
			if (srcFrame->format == AV_PIX_FMT_YUV420P || srcFrame->format == AV_PIX_FMT_YUV420P){

				dstFrame = m_I420Frame.GetFrame();//输出目标
				dstFrame->format = srcFrame->format;
				libyuv::I420Scale(srcFrame->data[0], srcFrame->linesize[0],
					srcFrame->data[1], srcFrame->linesize[1],
					srcFrame->data[2], srcFrame->linesize[2],
					srcFrame->width, srcFrame->height,
					dstFrame->data[0], dstFrame->linesize[0],
					dstFrame->data[1], dstFrame->linesize[1],
					dstFrame->data[2], dstFrame->linesize[2],
					dstFrame->width, dstFrame->height,
					libyuv::FilterMode::kFilterBilinear);
				if (m_videoFrame.GetFrame()->format != dstFrame->format) {
					dstFrame = m_videoFrame.GetFrame();//输出目标
					m_VideoConvert.Convert(m_I420Frame.GetFrame(), dstFrame);
				}
				av_frame_make_writable(dstFrame);//GIF  RGB8
				dstFrame->pts = srcFrame->pts;
				dstFrame->pict_type = srcFrame->pict_type;
				dstFrame->key_frame = srcFrame->key_frame;
			}
			else if (srcFrame->format == AV_PIX_FMT_RGB32) {
				dstFrame = m_RGBAFrame.GetFrame();//输出目标
				dstFrame->format = srcFrame->format;
				libyuv::ARGBScale(srcFrame->data[0], srcFrame->linesize[0],
					srcFrame->width, srcFrame->height,
					dstFrame->data[0], dstFrame->linesize[0],
					dstFrame->width, dstFrame->height,
					libyuv::FilterMode::kFilterBilinear);
				if (m_videoFrame.GetFrame()->format != dstFrame->format) {
					dstFrame = m_videoFrame.GetFrame();//输出目标
					m_VideoConvert.Convert(m_RGBAFrame.GetFrame(), dstFrame);
				}
				av_frame_make_writable(dstFrame);//GIF  RGB8
				dstFrame->pts = srcFrame->pts;
				dstFrame->pict_type = srcFrame->pict_type;
				dstFrame->key_frame = srcFrame->key_frame;
			}
		}
	}

	//输入视频和编码视频格式不一样
	else if (srcFrame->format != m_dstFmt) {
		dstFrame = m_videoFrame.GetFrame();//输出目标
		dstFrame->pts = srcFrame->pts;
		dstFrame->pict_type = srcFrame->pict_type;
		dstFrame->key_frame = srcFrame->key_frame;

		if (dstFrame->format == AV_PIX_FMT_NV12) { //ffmpeg-qsv-h264-encoder
			if (srcFrame->format == AV_PIX_FMT_RGB32) { //RGB32 To NV12
				libyuv::ARGBToNV12(srcFrame->data[0], srcFrame->linesize[0],
					dstFrame->data[0], dstFrame->linesize[0],
					dstFrame->data[1], dstFrame->linesize[1],
					m_iWidth, m_iHeight);
			}else if (srcFrame->format == AV_PIX_FMT_YUV420P) {  //I420 To NV12
				libyuv::I420ToNV12(srcFrame->data[0], srcFrame->linesize[0],
					srcFrame->data[1], srcFrame->linesize[1],
					srcFrame->data[2], srcFrame->linesize[2],
					dstFrame->data[0], dstFrame->linesize[0],
					dstFrame->data[1], dstFrame->linesize[1],
					m_iWidth, m_iHeight);
			}

		}else if (srcFrame->format == AV_PIX_FMT_RGB32){ //录屏和游戏采集
			if (m_dstFmt == AV_PIX_FMT_YUV420P) {
				libyuv::ARGBToI420(srcFrame->data[0], srcFrame->linesize[0],
					dstFrame->data[0], dstFrame->linesize[0],
					dstFrame->data[1], dstFrame->linesize[1],
					dstFrame->data[2], dstFrame->linesize[2],
					m_iWidth, m_iHeight);
			}
			else if (m_dstFmt == AV_PIX_FMT_YUVJ420P) {
				libyuv::ARGBToJ420(srcFrame->data[0], srcFrame->linesize[0],
					dstFrame->data[0], dstFrame->linesize[0],
					dstFrame->data[1], dstFrame->linesize[1],
					dstFrame->data[2], dstFrame->linesize[2],
					m_iWidth, m_iHeight);
			}
			else if (m_dstFmt == AV_PIX_FMT_YUV444P) {
				libyuv::ARGBToI444(srcFrame->data[0], srcFrame->linesize[0],
					dstFrame->data[0], dstFrame->linesize[0],
					dstFrame->data[1], dstFrame->linesize[1],
					dstFrame->data[2], dstFrame->linesize[2],
					m_iWidth, m_iHeight);
			}
			else { //其它格式用swscale
				av_frame_make_writable(dstFrame);//GIF  RGB8
				if (m_bGif) {
					GifImpl(srcFrame, dstFrame);
				}else {
					m_VideoConvert.Convert(srcFrame, dstFrame);
				}
			}
		}else { //其它格式用swscale
			av_frame_make_writable(dstFrame);//GIF  RGB8
			if (m_bGif) {
				GifImpl(srcFrame, dstFrame);
			}else {
				m_VideoConvert.Convert(srcFrame, dstFrame);
			}
		}
	}

	//---------------  ForceFPS ------------

	//编码和输出过程

	if (m_bCFR == 0) { //动态帧率
		EncodeFrameImpl(dstFrame, TRUE);
	}
	else if (m_bCFR == 1) { //补帧方式CFR
		int64_t ptsTarget = srcFrame->pts / m_nTime * m_nTime;//输出的pts
		while (true) {
			int64_t pts1 = m_ptsLastVideo;
			if (pts1 > ptsTarget)
				break;
			m_ptsLastVideo = pts1;
			dstFrame->pts = m_ptsLastVideo;
			m_ptsLastVideo += m_nTime;
			EncodeFrameImpl(dstFrame, TRUE);
		}
	}
}

int64_t FfmpegMuxer::GetVideoSize() { return m_iVideoDataSize; }

int64_t FfmpegMuxer::GetVideoTime() { return m_ptsVideo; }

int64_t FfmpegMuxer::GetAudioSize() { return m_iAudioDataSize; }

int64_t FfmpegMuxer::GetAudioTime() { return m_ptsAudio; }

int     FfmpegMuxer::GetAudioFrameSize() { return m_iAudioFrameSize; }

int64_t FfmpegMuxer::GetFileSize() { return m_iVideoDataSize + m_iAudioDataSize; }

void    FfmpegMuxer::SetVideoConfig(int width, int height, int fps,
	int bitrate, int mode, int HardwareCodec,
	int forceFps, int AntiAliasing, int nVideoSacle /*= 100*/, WXCTSTR video_codec) {
	m_bHasVideo = TRUE;
	m_nVideoScale = nVideoSacle;
	m_iWidth = (width * m_nVideoScale / 100) /2 * 2;
	m_iHeight = (height * m_nVideoScale / 100) /2 * 2;
	m_videoMode = mode;

	m_bHardwareEncoder = HardwareCodec;
	m_bX264AntiAliasing = AntiAliasing;//YUV444
	m_iFps = fps;
	m_bCFR = forceFps;
	if (m_iFps > 30) {
		m_bCFR = 0;
		WXLogA("CFR Max Rate is 30，now is %d", m_iFps);
	}
	m_iVideoBitrate = bitrate;
	m_nTime = 90000 / m_iFps;
	if (WXSupportHarewareCodec() &&
		m_bHardwareEncoder &&
		m_iWidth * m_iHeight <= 2560 * 1600) {
		m_bHardwareEncoder = true;
	}
	m_bX264AntiAliasing = m_bX264AntiAliasing | m_nX264AntiAliasing;
	if (video_codec) {
		m_strVideoCodec = video_codec;
		if (m_strVideoCodec.length()) { //设置视频编码器
			if (stricmp(m_strVideoCodec.c_str(), "H264") == 0|| stricmp(m_strVideoCodec.c_str(), "AVC") == 0) {
				m_forceVideoID = AV_CODEC_ID_H264;//H264
			}
			else if (stricmp(m_strVideoCodec.c_str(), "H265") == 0 || stricmp(m_strVideoCodec.c_str(), "HEVC") == 0) {
				m_forceVideoID = AV_CODEC_ID_H265;//h265
			}
			else if (stricmp(m_strVideoCodec.c_str(), "FLV") == 0) {
				m_forceVideoID = AV_CODEC_ID_H264;// AV_CODEC_ID_FLV1;//flv  h263p
			}			
			else if (stricmp(m_strVideoCodec.c_str(), "MPEG1") == 0) {
				m_forceVideoID = AV_CODEC_ID_MPEG1VIDEO;//MPEG1
			}
			else if (stricmp(m_strVideoCodec.c_str(), "MPEG2") == 0) {
				m_forceVideoID = AV_CODEC_ID_MPEG2VIDEO;//MPEG2
			}			
			else if (stricmp(m_strVideoCodec.c_str(), "MPEG4") == 0) {
				m_forceVideoID = AV_CODEC_ID_MPEG4;//MPEG4
			}		
			else if (stricmp(m_strVideoCodec.c_str(), "MSMPEG4v2") == 0) {
				m_forceVideoID = AV_CODEC_ID_MPEG4;//MSMPEG4v2
			}
			else if (stricmp(m_strVideoCodec.c_str(), "MSMPEG4v3") == 0) {
				m_forceVideoID = AV_CODEC_ID_MPEG4;//MSMPEG4v3
			}			
			else if (stricmp(m_strVideoCodec.c_str(), "WMV1") == 0) {
				m_forceVideoID = AV_CODEC_ID_MPEG4;//wmv/asf
			}			
			else if (stricmp(m_strVideoCodec.c_str(), "WMV2") == 0) {
				m_forceVideoID = AV_CODEC_ID_MPEG4;//wmv/asf
			}				
			else if (stricmp(m_strVideoCodec.c_str(), "VP8") == 0) {
				m_forceVideoID = AV_CODEC_ID_VP8;//vp8 libvpx
			}
			else if (stricmp(m_strVideoCodec.c_str(), "VP9") == 0) {
				m_forceVideoID = AV_CODEC_ID_VP9;//vp9 libvpx
			}
			else if (stricmp(m_strVideoCodec.c_str(), "AV1") == 0) {
				m_forceVideoID = AV_CODEC_ID_AV1;//AV1 libaom
			}
			else if (stricmp(m_strVideoCodec.c_str(), "VC1") == 0) {
				m_forceVideoID = AV_CODEC_ID_VC1;//VC1
			}
		}
	}
}

void  FfmpegMuxer::SetAudioConfig(int SampleRate, int channel, int bitrate, WXCTSTR audio_codec) {
	m_bHasAudio = TRUE;
	m_iSampleRate = SampleRate;
	m_iChannel = channel;
	m_iAudioBitrate = bitrate;
	if (audio_codec) {
		m_strAudioCodec = audio_codec;
		if (m_strAudioCodec.length() > 0) { //设置音频编码器
			if (stricmp(m_strAudioCodec.c_str(), "AAC") == 0)
				m_forceAudioID = AV_CODEC_ID_AAC;
			else if (stricmp(m_strAudioCodec.c_str(), "MP3") == 0)
				m_forceAudioID = AV_CODEC_ID_MP3;			
			else if (stricmp(m_strAudioCodec.c_str(), "AC3") == 0)
				m_forceAudioID = AV_CODEC_ID_AC3;			
			else if (stricmp(m_strAudioCodec.c_str(), "OPUS") == 0)
				m_forceAudioID = AV_CODEC_ID_OPUS;	
			else if (stricmp(m_strAudioCodec.c_str(), "MP1") == 0)
				m_forceAudioID = AV_CODEC_ID_MP1;	
			else if (stricmp(m_strAudioCodec.c_str(), "MP2") == 0)
				m_forceAudioID = AV_CODEC_ID_MP2;
			else if (stricmp(m_strAudioCodec.c_str(), "WMA") == 0)
				m_forceAudioID = AV_CODEC_ID_AAC;
		}
	}
}

//启动
int  FfmpegMuxer::Open(WXCTSTR wszFileName, int notUseTemp/* = 0*/) {
	WXAutoLock al(m_mutex);

	m_bNotUseTemp = notUseTemp;
	if (!m_bHasVideo && !m_bHasAudio) {
		WXLogW(L"%ws No Media Data", __FUNCTIONW__);
		return WX_EVENT_NO_DATA; //不存在音视频参数
	}

	m_strFileName.Format(L"%ws", wszFileName);

	std::wstring wstrName = wszFileName;
	std::wstring strExt = L"";
	std::wstring strExt2 = L"";

	size_t lastDot = wstrName.find_last_of(L'.');
	if (lastDot != std::wstring::npos) {
		strExt = wstrName.substr(lastDot + 1);
		size_t secondLastDot = wstrName.find_last_of(L'.', lastDot - 1);
		if (secondLastDot != std::wstring::npos) {
			strExt2 = wstrName.substr(secondLastDot + 1, lastDot - secondLastDot - 1);
		}
	}
	m_strExt = strExt.c_str();
	m_strExt2 = strExt2.c_str();

	WXLogW(L"---------  File Ext is %ws", m_strExt.str());
	WXLogW(L"---------  File Ext2 is %ws", m_strExt2.str());
	//根据加密需求， 文件名可能会直接传入  a.mp4.xws

	if (WXStrcmp(m_strExt.str(), _T("gif")) == 0|| 
		WXStrcmp(m_strExt2.str(), _T("gif")) == 0) {
		m_forceAudioID = AV_CODEC_ID_NONE;
		m_forceVideoID = AV_CODEC_ID_GIF;
		m_bHasAudio = FALSE;
		m_bHasVideo = TRUE;
		m_bMKV_Temp = TRUE;
		m_strType = L"matroska";
	}

	//OGG 音频,不支持XWS
	if (WXStrcmp(m_strExt.str(), _T("ogg")) == 0 || 
		WXStrcmp(m_strExt2.str(), _T("ogg")) == 0) {
		m_forceAudioID = AV_CODEC_ID_VORBIS;
		m_forceVideoID = AV_CODEC_ID_NONE;
		m_bHasAudio = TRUE;
		m_bHasVideo = FALSE;
		m_bMKV_Temp = TRUE;
		m_strType = L"matroska";
	}

	//FLAC 音频
	if (WXStrcmp(m_strExt.str(), _T("flac")) == 0 ||
		WXStrcmp(m_strExt2.str(), _T("flac")) == 0) {
		m_forceAudioID = AV_CODEC_ID_FLAC;
		m_forceVideoID = AV_CODEC_ID_NONE;
		m_bHasAudio = TRUE;
		m_bHasVideo = FALSE;
		m_bMKV_Temp = TRUE;
		m_strType = L"matroska";
	}

	//MP3 音频
	if (WXStrcmp(m_strExt.str(), _T("mp3")) == 0 ||
		WXStrcmp(m_strExt2.str(), _T("mp3")) == 0) {
		m_forceAudioID = AV_CODEC_ID_MP3;
		m_forceVideoID = AV_CODEC_ID_NONE;
		m_bHasAudio = TRUE;
		m_bHasVideo = FALSE;
	}
	//AAC 音频
	if (WXStrcmp(m_strExt.str(), _T("aac")) == 0 ||
		WXStrcmp(m_strExt2.str(), _T("aac")) == 0) {
		m_forceAudioID = AV_CODEC_ID_AAC;
		m_forceVideoID = AV_CODEC_ID_NONE;
		m_bHasAudio = TRUE;
		m_bHasVideo = FALSE;
	}

	//WAV 音频,不支持XWS
	if (WXStrcmp(m_strExt.str(), _T("wav")) == 0 ||
		WXStrcmp(m_strExt2.str(), _T("wav")) == 0) {
		m_forceAudioID = AV_CODEC_ID_PCM_S16LE;
		m_forceVideoID = AV_CODEC_ID_NONE;
		m_bHasAudio = TRUE;
		m_bHasVideo = FALSE;
		m_bMKV_Temp = TRUE;
		m_strType = L"matroska";
	}

	if (m_forceVideoID == AV_CODEC_ID_VP8) {
		//VP8不能在MP4中使用
		if (WXStrcmp(m_strExt.str(), _T("mp4")) == 0 ||
			WXStrcmp(m_strExt.str(), _T("mov")) == 0 ||
			WXStrcmp(m_strExt.str(), _T("avi")) == 0 ||
			WXStrcmp(m_strExt2.str(), _T("mp4")) == 0 ||
			WXStrcmp(m_strExt2.str(), _T("mov")) == 0 ||
			WXStrcmp(m_strExt2.str(), _T("avi")) == 0) {
			m_forceVideoID = AV_CODEC_ID_VP9;
		}
		m_bMKV_Temp = TRUE;
		m_strType = L"matroska";
	}
	if (m_forceVideoID == AV_CODEC_ID_VP9) {
		m_bMKV_Temp = TRUE;
		m_strType = L"matroska";
	}

	if (WXStrcmp(m_strExt.str(), _T("ts")) == 0 ||
		WXStrcmp(m_strExt.str(), _T("xws")) == 0 ||
		WXStrcmp(m_strExt2.str(), _T("ts")) == 0 ||
		WXStrcmp(m_strExt2.str(), _T("xws")) == 0) {
		if (m_forceVideoID == AV_CODEC_ID_VP8 || m_forceVideoID == AV_CODEC_ID_VP9) {
			m_forceVideoID = AV_CODEC_ID_H264;//VP8/9不能在TS中使用
		}
	}
	//VCD DVD 格式
	if (WXStrcmp(m_strExt.str(), _T("mpeg")) == 0 || 
		WXStrcmp(m_strExt.str(), _T("mpg")) == 0 ||
		WXStrcmp(m_strExt.str(), _T("vob")) == 0 ||
		WXStrcmp(m_strExt2.str(), _T("mpeg")) == 0 ||
		WXStrcmp(m_strExt2.str(), _T("mpg")) == 0 ||
		WXStrcmp(m_strExt2.str(), _T("vob")) == 0) {
		if (m_bHasVideo && m_forceVideoID == AV_CODEC_ID_NONE)
			m_forceVideoID = AV_CODEC_ID_MPEG1VIDEO;
		if (m_bHasAudio && m_forceAudioID == AV_CODEC_ID_NONE)
			m_forceAudioID = AV_CODEC_ID_MP2;
	}

	//原生的ASF格式支持流播放，但是默认编码器兼容性有问题，对H264编码支持完整
	if (WXStrcmp(m_strExt.str(), _T("asf")) == 0 ||
		WXStrcmp(m_strExt.str(), _T("wmv")) == 0 ||
		WXStrcmp(m_strExt2.str(), _T("asf")) == 0 ||
		WXStrcmp(m_strExt2.str(), _T("wmv")) == 0) {
		if (m_bHasAudio)
			m_forceAudioID = AV_CODEC_ID_MP3;
		if (m_bHasVideo)
			m_forceVideoID = AV_CODEC_ID_MPEG4;
	}

	//配置默认编码器
	if ( WXStrcmp(m_strExt.str(), _T("ts")) == 0
		|| WXStrcmp(m_strExt.str(), _T("flv")) == 0
		|| WXStrcmp(m_strExt.str(), _T("mkv")) == 0 
		|| WXStrcmp(m_strExt.str(), _T("mp4")) == 0
		|| WXStrcmp(m_strExt.str(), _T("mov")) == 0
		|| WXStrcmp(m_strExt.str(), _T("avi")) == 0 
		|| WXStrcmp(m_strExt.str(), _T("xws")) == 0
		|| WXStrcmp(m_strExt2.str(), _T("ts")) == 0
		|| WXStrcmp(m_strExt2.str(), _T("flv")) == 0
		|| WXStrcmp(m_strExt2.str(), _T("mkv")) == 0
		|| WXStrcmp(m_strExt2.str(), _T("mp4")) == 0
		|| WXStrcmp(m_strExt2.str(), _T("mov")) == 0
		|| WXStrcmp(m_strExt2.str(), _T("avi")) == 0
		|| WXStrcmp(m_strExt2.str(), _T("xws")) == 0) {
		if (m_bHasVideo && m_forceVideoID == AV_CODEC_ID_NONE)
			m_forceVideoID = AV_CODEC_ID_H264;
		if (m_bHasAudio && m_forceAudioID == AV_CODEC_ID_NONE)
			m_forceAudioID = AV_CODEC_ID_AAC;
	}

	//如果是视频格式而且支持XWS格式，就先录制XWS，然后在录制结束之后快速转回来
	if (m_forceVideoID != AV_CODEC_ID_GIF && !m_bNotUseTemp){ //高性能录屏处理
		BOOL bSupportXWSAudio = FALSE;
		if (m_forceAudioID == AV_CODEC_ID_NONE ||
			m_forceAudioID == AV_CODEC_ID_MP1 ||
			m_forceAudioID == AV_CODEC_ID_MP2 ||
			m_forceAudioID == AV_CODEC_ID_MP3 ||
			m_forceAudioID == AV_CODEC_ID_AAC ||
			m_forceAudioID == AV_CODEC_ID_AC3 ||
			m_forceAudioID == AV_CODEC_ID_OPUS
			) {
			bSupportXWSAudio = TRUE;
		}
		BOOL bSupportXWSVideo = FALSE;
		if (m_forceVideoID == AV_CODEC_ID_NONE ||
			m_forceVideoID == AV_CODEC_ID_MPEG4 ||
			m_forceVideoID == AV_CODEC_ID_H264 ||
			m_forceVideoID == AV_CODEC_ID_HEVC ||
			m_forceVideoID == AV_CODEC_ID_VC1 ||
			m_forceVideoID == AV_CODEC_ID_MPEG1VIDEO ||
			m_forceVideoID == AV_CODEC_ID_MPEG2VIDEO
			) {
			bSupportXWSVideo = TRUE;
		}
		if(bSupportXWSVideo && bSupportXWSAudio) {
			m_bUseTemp = TRUE;
			m_strType = L"xws";
			m_bXWS_Temp = TRUE;
			m_bMKV_Temp = FALSE;
			WXLogW(L"Using XWS  Type!");
		}
		if (!m_bXWS_Temp && m_bMKV_Temp) {
			m_bUseTemp = TRUE;
			m_bXWS_Temp = FALSE;
			WXLogW(L"Using MKV  Type!");
		}

		if (WXStrcmp(m_strExt.str(), _T("xws")) == 0
			|| WXStrcmp(m_strExt2.str(), _T("xws")) == 0) {
			m_bUseTemp = FALSE;
			WXLogW(L"Not Using temp  file  m_strType=[%ws]!", m_strType.str());
		}

		if (m_bUseTemp) {
			m_strTemp = wszFileName;
			m_strTemp += L".xws";
			WXLogW(L"Not Using temp  [%ws] file  m_strType=[%ws]!", m_strTemp.str(), m_strType.str());
		}
	}

	if (m_bNotUseTemp) {
		//不使用临时文件的情况
		m_strType = L"";
		m_bUseTemp = FALSE;
	}

	if (strnicmp(m_strFileName.c_str(), "rtmp://", 7) == 0) {
		avformat_alloc_output_context2(&m_pFmtCtx, nullptr, "flv", m_strFileName.c_str());
		if (m_bHasVideo)
			m_forceVideoID = AV_CODEC_ID_H264;
		if (m_bHasAudio)
			m_forceAudioID = AV_CODEC_ID_AAC;
	}
	else if (strnicmp(m_strFileName.c_str(), "rtsp://", 7) == 0) {

		avformat_alloc_output_context2(&m_pFmtCtx, nullptr, "mp4", m_strFileName.c_str());
		if (m_bHasVideo)
			m_forceVideoID = AV_CODEC_ID_H264;
		if (m_bHasAudio)
			m_forceAudioID = AV_CODEC_ID_AAC;
	}else {
		if (m_bUseTemp) {
			avformat_alloc_output_context2(&m_pFmtCtx, nullptr, m_strType.c_str() , m_strTemp.c_str());
		}else {
			avformat_alloc_output_context2(&m_pFmtCtx, nullptr, m_strType.length() ? m_strType.c_str() : nullptr, m_strFileName.c_str());
		}
	}

	if (nullptr == m_pFmtCtx) {
		WXLogW(L"FfmpegMuxer avformat_alloc_output_context2 Error Filename=%ws", m_strFileName.str());
		return WX_EVENT_INTI_FFMPEG_MUXER; //avformat_alloc_output_context2 初始化失败，一般是格式不支持
	}

	m_nFlags = m_pFmtCtx->oformat->flags;

	if (m_forceVideoID != AV_CODEC_ID_NONE && 
		m_pFmtCtx->oformat->video_codec != AV_CODEC_ID_NONE) {
		m_pFmtCtx->oformat->video_codec = (AVCodecID)m_forceVideoID;
		//WXLogW(L"%ws Using m_forceVideoID", m_strExt.str());
	}


	if (m_forceAudioID != AV_CODEC_ID_NONE && 
		m_pFmtCtx->oformat->audio_codec != AV_CODEC_ID_NONE) {
		m_pFmtCtx->oformat->audio_codec = (AVCodecID)m_forceAudioID;
		//WXLogW(L"%ws Using m_forceAudioID", m_strExt.str());
	}


	if (m_bHasVideo && m_pFmtCtx->oformat->video_codec != AV_CODEC_ID_NONE && m_iWidth && m_iHeight) {  //视频编码器初始化
		m_forceVideoID = m_pFmtCtx->oformat->video_codec;

		if (m_pVideoCtx == nullptr) {  //ffmpeg-codec
			WXLogA("FfmpegMuxer Create Video Codec Begin");
			BOOL openVideo = OpenVideoEncoder(m_forceVideoID);
			if (!openVideo) {
				WXLogA("FfmpegMuxer OpenVideoEncoder Error");
			}
		}
		else {
			WXLogA("FfmpegMuxer Not Using Software Encoder, Size=%dx%d", m_iWidth, m_iHeight);
		}
	}

	if (m_bHasAudio && m_pFmtCtx->oformat->audio_codec != AV_CODEC_ID_NONE) {//音频编码器初始化
		m_forceAudioID = m_pFmtCtx->oformat->audio_codec;
		BOOL openAuido = OpenAudioEncoder(m_forceAudioID);
		if (!openAuido) {
			WXLogA("FfmpegMuxer openAuido Error");
		}
	}

	if (m_pAudioCtx == nullptr && m_pVideoCtx == nullptr) {
		if (m_pFmtCtx) {
			avformat_free_context(m_pFmtCtx);
			m_pFmtCtx = nullptr;
		}
		WXLogA("FfmpegMuxer not have audio and video encoder");
		return WX_EVENT_INTI_FFMPEG_ENCODER; //音视频编码器创建失败！！
	}

	if (!(m_nFlags & AVFMT_NOFILE)) {
		int ret = avio_open(&m_pFmtCtx->pb, m_pFmtCtx->filename, AVIO_FLAG_WRITE);
		if (ret < 0) {
			avformat_free_context(m_pFmtCtx);
			m_pFmtCtx = nullptr;
			WXLogA("%s avio_open Error",__FUNCTION__);
			return WX_EVENT_FFMPEG_AVIO; //文件IO失败
		}
	}
	int ret = avformat_write_header(m_pFmtCtx, nullptr);
	if (ret < 0) {
		WXLogA("%s avformat_write_header Error", __FUNCTION__);
		return WX_ERROR_FFMPEG_WRITE_HEADER; //写入文件头失败
	}

	if (m_pVideoCtx) {
		m_dstFmt = m_pVideoCtx->pix_fmt;
		m_videoFrame.Init(m_dstFmt, m_iWidth, m_iHeight);//GIF 格式会特殊处理
		m_I420Frame.Init(AV_PIX_FMT_YUV420P, m_iWidth, m_iHeight);
		m_RGBAFrame.Init(AV_PIX_FMT_RGB32, m_iWidth, m_iHeight);
	}


	WXLogA("---FfmpegMuxer Start %s OK", m_strFileName.c_str());
	return WX_ERROR_SUCCESS;
}

FfmpegMuxer::~FfmpegMuxer() {

}

extern bool s_bFfmpegExe;// = false;
DWORD ExecuteFfmpegExe(const wchar_t* wszArg);
int FfmpegMuxer::Close() {
	//WXLogA("%s Beign---FfmpegMuxer Begin %s", __FUNCTION__, m_strFileName.c_str());
	WXAutoLock al(m_mutex);
	//BEGIN_LOG_FUNC

	int ff_error = WX_ERROR_SUCCESS;

	if (m_pFmtCtx) {
		//flush video
		if (m_pVideoCtx) {
			int flush_video = 0;
			while (true)
			{
				int flush= EncodeFrameImpl(nullptr, TRUE);
				flush_video++;
				if (flush < 0) {
					break;
				}
			}
			WXLogA("flush_video = %d-----", flush_video);
		}

		//flush audio
		if (m_pAudioCtx) {
			int flush_audio = 0;
			while (true)
			{
				int flush = EncodeFrameImpl(nullptr, FALSE);
				flush_audio++;
				if (flush < 0) {
					break;
				}
			}
			WXLogA("flush_audio = %d-----", flush_audio);
		}

		int ret = av_write_trailer(m_pFmtCtx);//写入文件尾
		if (ret < 0) {
			ff_error = WX_ERROR_FFMPEG_WRITE_TRAILER;//写入文件尾,失败
		}
		if (!(m_nFlags & AVFMT_NOFILE)) {
			avio_closep(&m_pFmtCtx->pb);//关闭文件锁定
		}
		avformat_free_context(m_pFmtCtx);
		m_pFmtCtx = nullptr;
	}

	if (m_ptsAudio > 0) { //计算音频码率
		int AudioBitrate = (int)(m_iAudioDataSize * 8 / m_ptsAudio);
		WXLogA("\tAudioTime=[%lld]ms\r\nAudio Bitrate=[%d]kbps ",m_ptsAudio, AudioBitrate);
	}

	if (m_ptsVideo > 0) { //计算视频码率
		int nVB = (int)(m_iVideoDataSize * 8 / m_ptsVideo);
		double fFps = (double)(m_iVideoFrame2 * 1000.0 / m_ptsVideo);//录制帧率
		double fRealFps = (double)(m_iVideoFrame1 * 1000.0 / m_ptsVideo);//实际帧率

		WXLogA("---Video Input=%lld Output=%lld --",
			m_iVideoFrame1, m_iVideoFrame2);


		WXLogA("\tVideoTime=[%lld]ms\r\nVidoe Bitrate=[%d]kbps\r\nFps=[%0.2f]Fps\r\nRealFps=[%0.2f]Fps",
			m_ptsVideo, nVB, fFps, fRealFps);
	}

	if (m_ptsVideo && m_ptsAudio) {
		WXLogW(L"\tFfmpegMuxer [PtsVideo=%lld] [PtsAudio=%lld] [Delay= %lld]", 
			m_ptsVideo, m_ptsAudio, m_ptsVideo - m_ptsAudio);
	}

	if (m_pVideoCtx) {
		avcodec_close(m_pVideoCtx);
		avcodec_free_context(&m_pVideoCtx);
		m_pVideoCtx = nullptr;
	}

	if (m_pAudioCtx) {
		avcodec_close(m_pAudioCtx);
		avcodec_free_context(&m_pAudioCtx);
		m_pAudioCtx = nullptr;
	}

	if (m_AudioConvert) {
		WXAudioConvertDeinit(m_AudioConvert);
		m_AudioConvert = nullptr;
	}

	//WXLogA("%s Beign---FfmpegMuxer End AAA", __FUNCTION__);

	if (m_bGif) {
		if (m_row_pointers) {
			free(m_row_pointers);
			m_row_pointers = NULL;
		}
		if (m_liq) {
			liq_attr_destroy(m_liq);
			m_liq = nullptr;
		}
		
	}

	if (m_bUseTemp) {

		WXConvertFast(m_strTemp.str(), m_strFileName.str());

		int error = 0;
		void* pInfo = WXMediaInfoCreateFast(m_strFileName.str(), &error);
		if (pInfo) {
			WXMediaInfoDestroy(pInfo);
			//转换成功，确保文件正常

#ifndef _DEBUG  //Release 就删除临时文件啊
			WXLogW(L"%ws DeleteFile [%ws]", __FUNCTIONW__, m_strTemp.str());
			::DeleteFile(m_strTemp.str());
#endif
		}
		else {
			::MessageBoxW(NULL, m_strFileName.str(), L"Rec Error", MB_OK);
			WXLogW(L"%ws Rec Error", m_strFileName.str());
		}

	}

	//WXLogA("%s Beign---FfmpegMuxer End BBB", __FUNCTION__);
	return ff_error;
}

void  FfmpegMuxer::WriteAudioFrame(uint8_t *buf, int buf_size) {
	if (m_bPause)
		return;
	if (m_pAudioCtx == nullptr)return;
	if (m_iAudioFrameSize != buf_size) {
		m_audioFifo.Write(buf, buf_size);
		while (m_audioFifo.Size() >= m_iAudioFrameSize) {
			m_audioFifo.Read(m_pAudioS16Frame.GetFrame()->data[0], m_iAudioFrameSize);//读取一帧音频数据
			GetWaveDataImpl(m_pAudioS16Frame.GetFrame()->data[0], m_iAudioFrameSize);//计算进入编码器的音频数据的音波条
			AVFrame *dstFtame = m_pAudioS16Frame.GetFrame();
			if (m_AudioConvert) {
				WXAudioConvertData(m_AudioConvert, m_pAudioFrame.GetFrame()->data, m_pAudioFrame.GetFrame()->nb_samples,
					(const uint8_t **)m_pAudioS16Frame.GetFrame()->data, m_pAudioS16Frame.GetFrame()->nb_samples);
				dstFtame = m_pAudioFrame.GetFrame();
			}
			if (dstFtame) {
				dstFtame->pts = m_iAudioFrame * dstFtame->nb_samples;
				m_ptsAudio = m_iAudioFrame * dstFtame->nb_samples * 1000 / m_iSampleRate; //总的音频时间 ms
				m_iAudioFrame++;
			}
			EncodeFrameImpl(dstFtame, FALSE);
		}
	}
	else {
		memcpy(m_pAudioS16Frame.GetFrame()->data[0], buf, buf_size);//读取一帧音频数据
		GetWaveDataImpl(m_pAudioS16Frame.GetFrame()->data[0], m_iAudioFrameSize);//计算进入编码器的音频数据的音波条
		AVFrame *dstFtame = m_pAudioS16Frame.GetFrame();
		if (m_AudioConvert) {
			WXAudioConvertData(m_AudioConvert, m_pAudioFrame.GetFrame()->data, m_pAudioFrame.GetFrame()->nb_samples,
				(const uint8_t **)m_pAudioS16Frame.GetFrame()->data, m_pAudioS16Frame.GetFrame()->nb_samples);
			dstFtame = m_pAudioFrame.GetFrame();
		}
		if (dstFtame) {
			dstFtame->pts = m_iAudioFrame * dstFtame->nb_samples;
			m_ptsAudio = m_iAudioFrame * dstFtame->nb_samples * 1000 / m_iSampleRate; //总的音频时间 ms
			m_iAudioFrame++;
		}
		EncodeFrameImpl(dstFtame, FALSE);
	}
}

FfmpegMuxer::FfmpegMuxer() {
	int nMemory = WXGetGlobalValue(L"Memory", -1);
	m_iMachLevel = WXGetGlobalValue(L"MachLevel", LEVEL_BETTER);//是否使用高性能采集
	if (nMemory < 5 && m_iMachLevel == LEVEL_BEST) {
		m_iMachLevel = LEVEL_BETTER; 
	}
	m_audioFifo.Init(192000 * 5);
}

//音频编码器，AAC默认使用libfdk-aac编码器
BOOL  FfmpegMuxer::OpenAudioEncoder(enum AVCodecID codec_id) {
	if (!m_bHasAudio)return FALSE;
	//BEGIN_LOG_FUNC

	AVCodec *audioCodec = nullptr;
	if (codec_id == AV_CODEC_ID_AAC) {
		audioCodec = avcodec_find_encoder_by_name("libfdk_aac");//自带AAC编码器码率控制不稳定
	}else {
		audioCodec = avcodec_find_encoder(codec_id);
	}

	if (nullptr == audioCodec) {
		WXLogA("OpenAudioEncoder avcodec_find_encoder error");
		return FALSE;
	}

	m_pAudioCtx = avcodec_alloc_context3(audioCodec);
	m_pAudioCtx->sample_fmt = audioCodec->sample_fmts ? audioCodec->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;

	if (codec_id == AV_CODEC_ID_MP3)
		m_pAudioCtx->sample_fmt = AV_SAMPLE_FMT_FLTP;

	//if (codec_id == AV_CODEC_ID_FLAC)
	//	m_pAudioCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	m_pAudioCtx->bit_rate = m_iAudioBitrate;
	m_pAudioCtx->sample_rate = m_iSampleRate;
	m_pAudioCtx->channel_layout = av_get_default_channel_layout(m_iChannel);
	m_pAudioCtx->channels = m_iChannel;
	m_pAudioCtx->time_base.num = 1;
	m_pAudioCtx->time_base.den = m_iSampleRate;
	if (m_nFlags & AVFMT_GLOBALHEADER)
		m_pAudioCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;


	if (audioCodec->id == AV_CODEC_ID_VORBIS) {//ffmpeg原来的值64，会导致只有1/4 声音录进去
		WXLogA("Using OGG Encoder");
		m_pAudioCtx->frame_size *= 8; //OGG录制异常
	}else if (audioCodec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE) {
		WXLogA("Using WAV Format");
		//m_pAudioCtx->frame_size = 1024; //WAV
		m_pAudioCtx->frame_size = m_iSampleRate / 100; //10ms
	}

	WXLogA("%s Codec=[%s,%s] Samplerate=%d Channel=%d Bitrate=%dkbps",
		__FUNCTION__,
		audioCodec->name,
		audioCodec->long_name,
		m_iSampleRate,
		m_iChannel,
		m_iAudioBitrate / 1000);

	int ret = avcodec_open2(m_pAudioCtx, audioCodec, nullptr);
	if (ret >= 0) {
		//WAV 格式再次进入编码器时，frame_size会被重置为0，需要手动设置
		if (audioCodec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE) {
			WXLogA("Using WAV Format");
			//m_pAudioCtx->frame_size = 1024; //WAV
			m_pAudioCtx->frame_size = m_iSampleRate / 100; //10ms
		}
		m_iAudioFrameSize = m_pAudioCtx->frame_size * sizeof(int16_t) * m_iChannel;//每次进入编码器的数据量，字节数
		m_pAudioS16Frame.Init(AV_SAMPLE_FMT_S16, m_iSampleRate, m_iChannel, m_pAudioCtx->frame_size);
		if (m_pAudioCtx->sample_fmt != AV_SAMPLE_FMT_S16) {
			m_pAudioFrame.Init(m_pAudioCtx->sample_fmt, m_iSampleRate, m_iChannel, m_pAudioCtx->frame_size);
			m_AudioConvert = WXAudioConvertInit(m_pAudioCtx->sample_rate, m_pAudioCtx->channels, AV_SAMPLE_FMT_S16,
				m_pAudioCtx->sample_rate, m_pAudioCtx->channels, m_pAudioCtx->sample_fmt);
		}
		m_pAudioStream = avformat_new_stream(m_pFmtCtx, nullptr);
		m_pAudioStream->id = m_pFmtCtx->nb_streams - 1;
		m_pAudioStream->time_base = m_pAudioCtx->time_base;
		m_pAudioStream->start_time = 0;
		m_tbAudio.den = m_pAudioCtx->time_base.den;
		m_tbAudio.num = m_pAudioCtx->time_base.num;
		avcodec_copy_context(m_pAudioStream->codec, m_pAudioCtx);
		return TRUE;
	}
	else {
		WXLogA("%s Error", __FUNCTION__);
		avcodec_free_context(&m_pAudioCtx);
		m_pAudioCtx = nullptr;
	}
	return FALSE;
}

//其它视频编码器
BOOL  FfmpegMuxer::OpenVideoEncoder(enum AVCodecID codec_id) {
	if (!m_bHasVideo)return FALSE;

	if (m_forceVideoID == AV_CODEC_ID_H264 || codec_id == AV_CODEC_ID_H264) {
		OpenH264Encoder();
	}
	else if (m_forceVideoID == AV_CODEC_ID_HEVC || codec_id == AV_CODEC_ID_HEVC) {
		OpenH265Encoder();
	}
	else if (m_forceVideoID == AV_CODEC_ID_VP8 || codec_id == AV_CODEC_ID_VP8) {
		OpenVP8Encoder();
	}
	else if (m_forceVideoID == AV_CODEC_ID_VP9 || codec_id == AV_CODEC_ID_VP9) {
		OpenVP9Encoder();
	}
	else
	{
		if (codec_id == AV_CODEC_ID_GIF) {
			WXLogW(L"Using GIF Encoder , must be VFR");
			m_bCFR = 0;  //GIF时间戳单位是10ms，不支持CFR模式！

			if (WXGetGlobalValue(L"HighQualityGif",0)) { //高画质GIF，普通画质GIF栅格化严重
				WXLogW(L"Using High Quality Gif Encoder");
				m_bGif = TRUE;
				if (m_row_pointers == nullptr) {
					m_row_pointers = (uint8_t**)malloc(m_iHeight * sizeof(uint8_t*));
				}
				m_liq = liq_attr_create(m_iWidth, m_iHeight);
				m_tempFrame.Init(AV_PIX_FMT_RGB32, m_iWidth, m_iHeight);
			}
		}
		m_pVideoCodec = avcodec_find_encoder(codec_id);
		if (m_pVideoCodec == nullptr) {
			WXLogW(L"OpenVideoEncoder avcodec_find_encoder Error");
			return FALSE;
		}
		m_pVideoCtx = avcodec_alloc_context3(m_pVideoCodec);
		m_pVideoCtx->width = m_iWidth;
		m_pVideoCtx->height = m_iHeight;

		m_pVideoCtx->time_base = m_tbMS;
		if (codec_id == AV_CODEC_ID_MPEG1VIDEO || codec_id == AV_CODEC_ID_MPEG2VIDEO || codec_id == AV_CODEC_ID_MPEG4 ) {
			m_pVideoCtx->time_base = { 1, m_iFps};//fixed for vob
		}
		m_pVideoCtx->framerate = { m_iFps, 1 };

		m_pVideoCtx->pix_fmt = m_pVideoCodec->pix_fmts ?
			m_pVideoCodec->pix_fmts[0] :
			AV_PIX_FMT_YUV420P;
		if (m_bGif)
			m_pVideoCtx->pix_fmt = AV_PIX_FMT_PAL8;
		m_pVideoCtx->gop_size = 12;//H263 MPEG4 之类的关键帧间隔不能设置过大，
		if (m_bCFR) {
			m_pVideoCtx->gop_size = FFMAX(12, m_iFps);
		}

		m_pVideoCtx->max_b_frames = 0;

		if (m_iVideoBitrate == 0) { //压缩比 80 100 120
			m_iVideoBitrate = (int64_t)((double)m_iWidth * (double)m_iHeight * (double)m_iFps * 12.0); //默认压缩比 100 
			m_iVideoBitrate /= s_nVideoCompress[m_iMachLevel][m_videoMode];//根据level和mode 选择合适的压缩比
		}
		m_pVideoCtx->bit_rate = m_iVideoBitrate;
		m_pVideoCtx->rc_max_rate = m_iVideoBitrate;
		m_pVideoCtx->rc_buffer_size = m_iVideoBitrate / 2;

		m_pVideoCtx->thread_count = std::thread::hardware_concurrency();//多线程编码
		m_pVideoCtx->gop_size = FFMIN(150, m_iFps * 5);

		if (m_nFlags & AVFMT_GLOBALHEADER)
			m_pVideoCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}

	if (m_pVideoCtx) {
		int ret = avcodec_open2(m_pVideoCtx, m_pVideoCodec, nullptr);
		if (ret >= 0) {
			WXLogA("%s Init Video Encoder [%s][%s] OK", __FUNCTION__, 
				m_pVideoCodec->name, m_pVideoCodec->long_name);
			m_pVideoStream = avformat_new_stream(m_pFmtCtx, nullptr);
			avcodec_copy_context(m_pVideoStream->codec, m_pVideoCtx);
			m_pVideoStream->id = m_pFmtCtx->nb_streams - 1;
			m_pVideoStream->time_base = m_pVideoCtx->time_base;
			return TRUE;
		}

		WXLogA("%s Init Video Encoder [%s][%s] Error", __FUNCTION__,
			m_pVideoCodec->name, m_pVideoCodec->long_name);
		avcodec_free_context(&m_pVideoCtx);
		m_pVideoCtx = nullptr;
		m_pVideoCodec = nullptr;
	}
	else {
		WXLogA("Can not find vidoe encoder !");
	}
	return FALSE;
}


//H265 编码器
void  FfmpegMuxer::OpenH265Encoder() {
	if (m_bHardwareEncoder && WXSupportH265Codec()) { //HEVC_QSV
		WXString strCodec = L"hevc_qsv";
		m_pVideoCodec = avcodec_find_encoder_by_name(strCodec.c_str());
	}
	if (m_pVideoCodec == nullptr) { //libx265
		m_bHardwareEncoder = FALSE;
		m_pVideoCodec = avcodec_find_encoder(AV_CODEC_ID_HEVC);
	}
	if (m_pVideoCodec == nullptr) { //libx265
		return;
	}

	m_pVideoCtx = avcodec_alloc_context3(m_pVideoCodec);
	m_pVideoCtx->width = m_iWidth;
	m_pVideoCtx->height = m_iHeight;
	m_pVideoCtx->max_b_frames = 0; //不用B帧
	m_pVideoCtx->refs = 1;
	m_pVideoCtx->time_base = m_tbVideo;
	m_pVideoCtx->framerate = { m_iFps, 1 };
	if (m_nFlags & AVFMT_GLOBALHEADER)
		m_pVideoCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER; //生成extradata数据

	//根据压缩模式设置压缩比来设置码率
	if (m_iVideoBitrate == 0) {
		int crf = m_crf_h265;
		if (m_bHardwareEncoder)

		if (m_iMachLevel == LEVEL_BEST) {
			crf -= 2;
		}else if(m_iMachLevel == LEVEL_GOOD) {
			crf += 2;
		}
		if (m_videoMode == MODE_BEST) {
			crf -= 2;
		}else if (m_videoMode == MODE_FAST) {
			crf += 2;
		}
		if (m_bHardwareEncoder) {
			crf -= 3;//硬编码器压缩比更高，同样的crf画质更差
			m_pVideoCtx->flags |= AV_CODEC_FLAG_QSCALE;
			m_pVideoCtx->global_quality = crf * FF_QP2LAMBDA;
		}
		else {
			av_opt_set_int(m_pVideoCtx->priv_data, "crf", crf, AV_OPT_SEARCH_CHILDREN);
		}
		WXLogA("%s Codec=[%s],  crf=%d",__FUNCTION__, m_pVideoCodec->long_name, crf);
	}
	else {
		m_pVideoCtx->bit_rate = m_iVideoBitrate;
		m_pVideoCtx->rc_max_rate = m_iVideoBitrate;
		m_pVideoCtx->rc_buffer_size = m_iVideoBitrate / 2;
		WXLogA("%s Codec=[%s],  Bitrate=%lldkbps", __FUNCTION__, m_pVideoCodec->name, m_iVideoBitrate / 1000);
	}

	if (stricmp(m_pVideoCodec->name, "hevc_qsv") == 0) {
		m_pVideoCtx->pix_fmt = AV_PIX_FMT_NV12;
		av_opt_set(m_pVideoCtx->priv_data, "profile", "main", 0);
		av_opt_set(m_pVideoCtx->priv_data, "preset", "faster", 0);
		av_opt_set(m_pVideoCtx->priv_data, "load_plugin", "hevc_hw", 0);
	}
	else {//libx265 软编码很慢！！！
		m_pVideoCtx->refs = 1;
		m_pVideoCtx->pix_fmt = AV_PIX_FMT_YUV420P;
		av_opt_set(m_pVideoCtx->priv_data, "profile", "main", 0);
		av_opt_set(m_pVideoCtx->priv_data, "preset", "ultrafast", 0);
		av_opt_set(m_pVideoCtx->priv_data, "tune", "zerolatency", 0);
	}
}

//H264编码器
void  FfmpegMuxer::OpenH264Encoder() {
	if (m_bHardwareEncoder && WXSupportH264Codec()) { //硬编码配置
		WXString codec_str = L"h264_qsv";
		m_pVideoCodec = avcodec_find_encoder_by_name(codec_str.c_str());
	}
	if (m_pVideoCodec == nullptr) {
		m_bHardwareEncoder = FALSE;
		m_pVideoCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
	}	
	if (m_pVideoCodec == nullptr) { //libx265
		return;
	}
	m_pVideoCtx = avcodec_alloc_context3(m_pVideoCodec);
	m_pVideoCtx->width = m_iWidth;
	m_pVideoCtx->height = m_iHeight;
	m_pVideoCtx->max_b_frames = 0; //不用B帧

	m_pVideoCtx->time_base = m_tbVideo;
	m_pVideoCtx->framerate = { m_iFps, 1 };
	if (m_nFlags & AVFMT_GLOBALHEADER)
		m_pVideoCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	//根据压缩模式设置压缩比来设置码率
	if (m_iVideoBitrate == 0) {
		int crf = m_crf_h264;
		if (m_iMachLevel == LEVEL_BEST) {
			crf -= 3;
		}
		else if (m_iMachLevel == LEVEL_GOOD) {
			crf += 3;
		}
		if (m_videoMode == MODE_BEST) {
			crf -= 3;
		}
		else if (m_videoMode == MODE_FAST) {
			crf += 3;
		}
		if (m_bHardwareEncoder) {
			crf -= 3;//硬编码器压缩比更高，同样的crf画质更差	
			m_pVideoCtx->flags |= AV_CODEC_FLAG_QSCALE;
			m_pVideoCtx->global_quality = crf * FF_QP2LAMBDA;
		}
		else {
			av_opt_set_int(m_pVideoCtx->priv_data, "qp", crf, AV_OPT_SEARCH_CHILDREN);
		}
		WXLogA("%s Codec=[%s],  crf=%d", __FUNCTION__, m_pVideoCodec->long_name, crf);
	}
	else {
		m_pVideoCtx->bit_rate = m_iVideoBitrate;
		m_pVideoCtx->rc_max_rate = m_iVideoBitrate;
		m_pVideoCtx->rc_buffer_size = m_iVideoBitrate / 2;
		WXLogA("%s Codec=[%s],  Bitrate=%lldkbps", __FUNCTION__, m_pVideoCodec->name, m_iVideoBitrate / 1000);
	}

	if (strcmp(m_pVideoCodec->name, "h264_qsv") == 0) { //Intel QSV
		m_pVideoCtx->pix_fmt = AV_PIX_FMT_NV12;
		m_pVideoCtx->gop_size = FFMIN(150, m_iFps * 5);
		//分辨率太高gop_size过大可能会导致QSV编码失败
		if (m_iWidth * m_iHeight > 1920 * 1080 && m_pVideoCtx->gop_size > 60)
			m_pVideoCtx->gop_size = 60;
	}
	else if (strcmp(m_pVideoCodec->name, "libx264") == 0) { //libx264 快速编码参数设置
		m_pVideoCtx->refs = 1;
		if (WXGetSystemVersion() > 7) {  //Win7 系统播放器对 1080P Level5.2 解码失败，走默认
			int imgsize = m_iWidth * m_iHeight;
			if (imgsize < 2560 * 1440 && imgsize >= 1920 * 1080) {
				m_pVideoCtx->level = 52;
			}
			else if (imgsize >= 1280 * 720 && imgsize < 1920 * 1080) {
				m_pVideoCtx->level = 41;
			}
		}
		m_pVideoCtx->thread_count = 1; //默认单线程编码
		if (m_iFps > 5) { //低帧率就用默认参数编码，保证画质
			m_pVideoCtx->gop_size = FFMIN(175, m_iFps * 8);//默认的250 可能在剧烈运动下产生马赛克
			if (m_iMachLevel == LEVEL_GOOD) { //远古机器，降低画质
				av_opt_set(m_pVideoCtx->priv_data, "preset", "ultrafast", 0);
			}
			else if (m_iMachLevel == LEVEL_BETTER) { //默认配置
				m_pVideoCtx->thread_count = std::thread::hardware_concurrency() / 2;//多线程优化提高编码速度
				av_opt_set(m_pVideoCtx->priv_data, "preset", "superfast", 0);
			}
			else {  //高性能编码
				m_pVideoCtx->thread_count = std::thread::hardware_concurrency();//多线程优化提高编码速度
				av_opt_set(m_pVideoCtx->priv_data, "preset", "faster", 0);
			}
		}
		if (m_bX264AntiAliasing) { //抗锯齿处理
			WXLogW(L"Libx264 AntiAliasing Config");
			m_pVideoCtx->pix_fmt = AV_PIX_FMT_YUV444P; //默认是 HighProfile, 但是Baseline不支持YUV444
		}
		else {
			m_pVideoCtx->pix_fmt = AV_PIX_FMT_YUV420P;
			if ((m_iFps >= 20 && m_iMachLevel == LEVEL_GOOD) || (WXGetSystemVersion() == 7)) {
				//性能差一些用Baseline模式
				av_opt_set(m_pVideoCtx->priv_data, "profile", "Baseline", 0);
			}
		}
		av_opt_set(m_pVideoCtx->priv_data, "tune", "zerolatency", 0);//零延时编码
	}
}

//VP8编码器
void  FfmpegMuxer::OpenVP8Encoder() {
	
	m_pVideoCodec = avcodec_find_encoder(AV_CODEC_ID_VP8);
	if (m_pVideoCodec == nullptr) { //libvpx vp8
		return;
	}
	m_pVideoCtx = avcodec_alloc_context3(m_pVideoCodec);
	m_pVideoCtx->width = m_iWidth;
	m_pVideoCtx->height = m_iHeight;
//	m_pVideoCtx->max_b_frames = 0; //不用B帧
//	m_pVideoCtx->refs = 1;
	m_pVideoCtx->time_base = m_tbVideo;
	m_pVideoCtx->framerate = { m_iFps, 1 };
	if (m_nFlags & AVFMT_GLOBALHEADER)
		m_pVideoCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	m_pVideoCtx->pix_fmt = AV_PIX_FMT_YUV420P;
	if (m_iVideoBitrate == 0) {
		int crf = m_crf_vp8;
		//if (m_bHardwareEncoder)
		//	crf -= 3;//硬编码器压缩比更高，同样的crf画质更差
		if (m_iMachLevel == LEVEL_BEST) {
			crf -= 2;
		}
		else if (m_iMachLevel == LEVEL_GOOD) {
			crf += 2;
		}
		if (m_videoMode == MODE_BEST) {
			crf -= 2;
		}
		else if (m_videoMode == MODE_FAST) {
			crf += 2;
		}
		av_opt_set_int(m_pVideoCtx->priv_data, "crf", crf, AV_OPT_SEARCH_CHILDREN);
		WXLogA("%s Codec=[%s],  crf=%d", __FUNCTION__, m_pVideoCodec->long_name, crf);

		m_iVideoBitrate = (int64_t)((double)m_iWidth * (double)m_iHeight * (double)m_iFps * 12.0) / 2; //默认压缩比 100 
		m_iVideoBitrate /= s_nVideoCompress[m_iMachLevel][m_videoMode];//根据level和mode 选择合适的压缩比

	}
	
	{
		m_pVideoCtx->bit_rate = m_iVideoBitrate;
		m_pVideoCtx->rc_max_rate = m_iVideoBitrate;
		m_pVideoCtx->rc_buffer_size = m_iVideoBitrate / 2;
		WXLogA("%s Codec=[%s],  Bitrate=%lldkbps", __FUNCTION__, m_pVideoCodec->long_name, m_iVideoBitrate / 1000);
	}
	m_pVideoCtx->thread_count = std::thread::hardware_concurrency();//多线程编码
	m_pVideoCtx->gop_size = FFMIN(150, m_iFps * 5);

	av_opt_set_int(m_pVideoCtx->priv_data, "cpu-used", 3, 0);//libvpx vp9  [-16,16]
}

//VP9编码器
void  FfmpegMuxer::OpenVP9Encoder() {

	m_pVideoCodec = avcodec_find_encoder(AV_CODEC_ID_VP9);
	if (m_pVideoCodec == nullptr) { //libvpx vp9
		return;
	}
	m_pVideoCtx = avcodec_alloc_context3(m_pVideoCodec);
	m_pVideoCtx->width = m_iWidth;
	m_pVideoCtx->height = m_iHeight;
	//m_pVideoCtx->max_b_frames = 0; //不用B帧
	//m_pVideoCtx->refs = 1;
	m_pVideoCtx->time_base = m_tbVideo;
	m_pVideoCtx->framerate = { m_iFps, 1 };
	if (m_nFlags & AVFMT_GLOBALHEADER)
		m_pVideoCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	m_pVideoCtx->pix_fmt = AV_PIX_FMT_YUV420P;
	//根据压缩模式设置压缩比来设置码率
	if (m_iVideoBitrate == 0) {
		int crf = m_crf_vp9;
		//if (m_bHardwareEncoder)
		//	crf -= 3;//硬编码器压缩比更高，同样的crf画质更差
		if (m_iMachLevel == LEVEL_BEST) {
			crf -= 2;
		}
		else if (m_iMachLevel == LEVEL_GOOD) {
			crf += 2;
		}
		if (m_videoMode == MODE_BEST) {
			crf -= 2;
		}
		else if (m_videoMode == MODE_FAST) {
			crf += 2;
		}
		av_opt_set_int(m_pVideoCtx->priv_data, "crf", crf, AV_OPT_SEARCH_CHILDREN);
		WXLogA("%s Codec=[%s],  crf=%d", __FUNCTION__, m_pVideoCodec->long_name, crf);
	}
	else {
		m_pVideoCtx->bit_rate = m_iVideoBitrate;
		m_pVideoCtx->rc_max_rate = m_iVideoBitrate;
		m_pVideoCtx->rc_buffer_size = m_iVideoBitrate / 2;
		WXLogA("%s Codec=[%s],  Bitrate=%lldkbps", __FUNCTION__, m_pVideoCodec->name, m_iVideoBitrate / 1000);
	}
	m_pVideoCtx->thread_count = std::thread::hardware_concurrency();//多线程编码
	m_pVideoCtx->gop_size = FFMIN(150, m_iFps * 5);
	av_opt_set_int(m_pVideoCtx->priv_data, "cpu-used", 8, 0);//libvpx vp9  [-16,16]
}

//AV1编码器
void  FfmpegMuxer::OpenAV1Encoder() {

	m_pVideoCodec = avcodec_find_encoder(AV_CODEC_ID_AV1);
	if (m_pVideoCodec == nullptr) { //libaom 
		return;
	}
	m_pVideoCtx = avcodec_alloc_context3(m_pVideoCodec);
	m_pVideoCtx->width = m_iWidth;
	m_pVideoCtx->height = m_iHeight;
	m_pVideoCtx->max_b_frames = 0; //不用B帧
	m_pVideoCtx->refs = 1;
	m_pVideoCtx->time_base = m_tbVideo;
	m_pVideoCtx->framerate = { m_iFps, 1 };
	if (m_nFlags & AVFMT_GLOBALHEADER)
		m_pVideoCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	m_pVideoCtx->pix_fmt = AV_PIX_FMT_YUV420P;

	//根据压缩模式设置压缩比来设置码率
	if (m_iVideoBitrate == 0) {

		int crf = m_crf_h265;
		//if (m_bHardwareEncoder)
		//	crf -= 3;//硬编码器压缩比更高，同样的crf画质更差
		if (m_iMachLevel == LEVEL_BEST) {
			crf -= 2;
		}
		else if (m_iMachLevel == LEVEL_GOOD) {
			crf += 2;
		}
		if (m_videoMode == MODE_BEST) {
			crf -= 2;
		}
		else if (m_videoMode == MODE_FAST) {
			crf += 2;
		}
		av_opt_set_int(m_pVideoCtx->priv_data, "crf", crf, AV_OPT_SEARCH_CHILDREN);
		WXLogA("%s Codec=[%s],  crf=%d", __FUNCTION__, m_pVideoCodec->long_name, crf);
	}
	else {
		m_pVideoCtx->bit_rate = m_iVideoBitrate;
		m_pVideoCtx->rc_max_rate = m_iVideoBitrate;
		m_pVideoCtx->rc_buffer_size = m_iVideoBitrate / 2;
		WXLogA("%s Codec=[%s],  Bitrate=%lldkbps", __FUNCTION__, m_pVideoCodec->name, m_iVideoBitrate / 1000);
	}
	m_pVideoCtx->thread_count = std::thread::hardware_concurrency();//多线程编码
	av_opt_set(m_pVideoCtx->priv_data, "preset", "7", 0);//libvpx AV1 0-8
}

