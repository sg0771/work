/*
录屏文件编辑功能
Tam
*/
#ifdef _WIN32
#include <WXMediaCpp.h>
#include "FfmpegMuxer.h"
#include "WXImageWaterMark.h"

//设置去水印的区域

//录屏时的处理
int g_bCaptureDelogo = 0;
int g_nCaptureDelogoX = 0;
int g_nCaptureDelogoY = 0;
int g_nCaptureDelogoW = 0;
int g_nCaptureDelogoH = 0;

//编辑时的处理
int g_bEditDelogo = 0;
int g_nEditDelogoX = 0;
int g_nEditDelogoY = 0;
int g_nEditDelogoW = 0;
int g_nEditDelogoH = 0;

WXMEDIA_API void    WXSetDelogo(int type, int nPosX, int nPosY, int nWidth, int nHeight) {
	if (type == 0) {
		if (nWidth <= 0 || nHeight <= 0) {
			g_bCaptureDelogo = 0;
			g_nCaptureDelogoX = 0;
			g_nCaptureDelogoY = 0;
			g_nCaptureDelogoW = 0;
			g_nCaptureDelogoH = 0;
		}
		else {
			g_bCaptureDelogo = 1;
			g_nCaptureDelogoX = nPosX;
			g_nCaptureDelogoY = nPosY;
			g_nCaptureDelogoW = nWidth;
			g_nCaptureDelogoH = nHeight;
		}
	}
	else if (type == 1) {
		if (nWidth <= 0 || nHeight <= 0) {
			g_bEditDelogo = 0;
			g_nEditDelogoX = 0;
			g_nEditDelogoY = 0;
			g_nEditDelogoW = 0;
			g_nEditDelogoH = 0;
		}
		else {
			g_bEditDelogo = 1;
			g_nEditDelogoX = nPosX;
			g_nEditDelogoY = nPosY;
			g_nEditDelogoW = nWidth;
			g_nEditDelogoH = nHeight;
		}
	}
}

//附加水印.
//录制
static WXString s_strLogo = L"";
static int s_nPosX = 0;
static int s_nPosY = 0;

WXMEDIA_API void WXSaveBmp32(WXCTSTR wszName) {
	WXString strBMP = wszName;
	strBMP += L"WXMedia.bmp";
	Gdiplus::Bitmap bmp(wszName);
	int dstwidth = bmp.GetWidth() / 2 * 2;
	int dstheight = bmp.GetHeight() / 2 * 2;
	if (dstwidth > 0 && dstheight > 0) {
		WXDataBuffer tmp;
		tmp.Init(NULL, dstwidth * dstheight * 4);

		Gdiplus::BitmapData bmpData;
		Gdiplus::Rect       rect(0, 0, dstwidth, dstheight);
		bmp.LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bmpData);
		uint8_t* pSrc = (uint8_t*)bmpData.Scan0;
		for (int i = 0; i < dstheight; i++) {
			memcpy(tmp.GetBuffer() + i * dstwidth * 4, pSrc + i * bmpData.Stride, dstwidth * 4);
		}
		bmp.UnlockBits(&bmpData);

		uint8_t* buf = tmp.GetBuffer();
		for (int j = 0; j < dstheight; j++) {
			for (int i = 0; i < dstwidth; i++) {
				int pos = j * dstwidth * 4 + i * 4;
				BYTE b = buf[pos];
				BYTE g = buf[pos + 1];
				BYTE r = buf[pos + 2];
				BYTE a = buf[pos + 3];
				if (b == 0 && g == 0 && r == 0 && a == 0) {
					buf[pos] = 255;
					buf[pos + 1] = 0;
					buf[pos + 2] = 0;
					buf[pos + 3] = 0;
				}
			}
		}

		std::ofstream of(strBMP.str(), std::ios::binary);
		if (of.is_open()) {
			BITMAPFILEHEADER bfh = { 0 };
			bfh.bfOffBits = 54;
			bfh.bfSize = 54 + 1920 * 1080 * 4;
			bfh.bfType = 'MB';
			BITMAPINFOHEADER bih = { 0 };
			bih.biBitCount = 32;
			bih.biPlanes = 1;
			bih.biSize = 40;
			bih.biWidth = 1920;
			bih.biHeight = -1080;
			bih.biSizeImage = 1920 * 1080 * 4;

			of.write((const char*)&bfh, 14);
			of.write((const char*)&bih, 40);
			of.write((const char*)tmp.GetBuffer(), 1920 * 1080 * 4);
			of.close();
		}

	}
}

WXMEDIA_API void WXCaptureSetWM(WXCTSTR wszName, int nPosX, int nPosY) {
	WXLogW(L"%ws --- Name[%ws]", __FUNCTIONW__, wszName);
	s_strLogo = wszName;
	s_nPosX = nPosX;
	s_nPosY = nPosY;
}


WXCTSTR WXCaptureGetWM() {
	return s_strLogo.length() ? s_strLogo.str() : nullptr;
}

int     WXCaptureGetWMPosX() {
	return s_nPosX;
}

int     WXCaptureGetWMPosY() {
	return s_nPosY;
}


//在指定窗口上显示某张图片
//wszName 为NULL时使用自定义数据
//static WXLocker s_lockGlobal;
WXMEDIA_API void WXVideoRenderCheck(HWND hwnd, int mode, WXCTSTR wszName) {
	std::thread new_thread([hwnd, mode, wszName] {
		//WXAutoLock al(s_lockGlobal);
		WXVideoFrame videoFrame;
		void * pVideoRender = nullptr;
		int width = GetSystemMetrics(SM_CXSCREEN); //屏幕宽度
		int height = GetSystemMetrics(SM_CYSCREEN); //屏幕高度
		if (wszName) {
			Gdiplus::Bitmap bitmap(wszName);
			width = bitmap.GetWidth();
			height = bitmap.GetHeight();
			if (width &&& height) {
				width = width / 4 * 4;
				height = height / 4 * 4;
				videoFrame.Init(AV_PIX_FMT_RGB32, width, height);
				Gdiplus::BitmapData bmpData;
				Gdiplus::Rect       rect(0, 0, width, height);
				bitmap.LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bmpData);
				uint8_t *pSrc = (uint8_t *)bmpData.Scan0;
				libyuv::ARGBCopy((uint8_t *)bmpData.Scan0, bmpData.Stride,
					videoFrame.GetFrame()->data[0], videoFrame.GetFrame()->linesize[0],
					width, height);
				bitmap.UnlockBits(&bmpData);
				pVideoRender = WXVideoRenderCreate(hwnd, width, height);
			}
		}
		else {
			videoFrame.Init(AV_PIX_FMT_YUVJ420P, width, height);
			for (int h = 0; h < height; h++) {
				for (int w = 0; w < width; w++) {
					int posY = w + h * videoFrame.GetFrame()->linesize[0];
					videoFrame.GetFrame()->data[0][posY] = uint8_t(w + h);
				}
			}

			for (int h = 0; h < height / 2; h++) {
				for (int w = 0; w < width / 2; w++) {
					int posU = w + h * videoFrame.GetFrame()->linesize[1];
					videoFrame.GetFrame()->data[1][posU] = uint8_t(w * 2 + h);

					int posV = w + h * videoFrame.GetFrame()->linesize[2];
					videoFrame.GetFrame()->data[2][posV] = uint8_t(w + h * 2);
				}
			}
			pVideoRender = WXVideoRenderCreate(hwnd, width, height);
		}

		if (pVideoRender) {
			WXVideoRenderChangeModeForce(pVideoRender, mode);
			WXVideoRenderDisplay(pVideoRender, videoFrame.GetFrame(), TRUE, RENDER_ROTATE_NONE);
			SLEEPMS(2000);
			WXVideoRenderDestroy(pVideoRender);
		}
	});
	new_thread.detach();
}

static void  DrawBmp(Gdiplus::Image*bmp, HWND hwnd) {
	int width = bmp->GetWidth();
	int height = bmp->GetHeight();
	if (width &&& height) {
		HDC hdc = ::GetDC(hwnd);
		Gdiplus::Graphics display(hdc);
		display.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);//抗锯齿
		RECT rect;
		::GetClientRect(hwnd, &rect);
		Gdiplus::Rect rc{0,0,rect.right,rect.bottom};
		display.DrawImage(bmp, rc);
		::ReleaseDC(hwnd, hdc);
	}
}

WXMEDIA_API void WXDrawA(HWND hwnd, WXCTSTR wszName) {
	Gdiplus::Image image(wszName);
	DrawBmp(&image, hwnd);
}

WXMEDIA_API void WXDrawB(HWND hwnd, uint8_t* buf, int buf_size) {

	HGLOBAL hMemBmp = GlobalAlloc(GMEM_FIXED, buf_size);
	if (hMemBmp == NULL) return;

	IStream* pStmBmp = NULL;
	CreateStreamOnHGlobal(hMemBmp, TRUE, &pStmBmp);
	if (pStmBmp == NULL){
		GlobalFree(hMemBmp);
		return;
	}

	BYTE* pbyBmp = (BYTE *)GlobalLock(hMemBmp);
	memcpy(pbyBmp,buf,buf_size);

	Gdiplus::Image * pImage = new Gdiplus::Image(pStmBmp, FALSE);

	DrawBmp(pImage, hwnd);

	//pStmBmp->Release();
	delete pImage;
	GlobalFree(hMemBmp);

}


//增加音视频延迟处理
//wszInput : 输入文件
//wszOutput : 输出文件
//audio_delay :  音频延迟参数，单位毫秒，表示相比原文件，音频延迟播放的时间
WXMEDIA_API int WXConvertDelay(WXCTSTR wszInput, WXCTSTR wszOutput, int audio_delay) {
	//输入文件解析
	AVFormatContextPtr FCtxRead(wszInput);
	if (FCtxRead.get() == nullptr) {
		WXLogW(L"[%ws][%ws] avformat_open_input error", __FUNCTIONW__, wszInput);
		return -1;
	}
	int err = avformat_find_stream_info(FCtxRead.get(), NULL);
	if (err < 0) {
		WXLogW(L"[%ws][%ws] avformat_find_stream_info error", __FUNCTIONW__, wszInput);
		return 0;
	}

	AVCodecContext* pInputVideoCtx = nullptr;
	AVStream* pInputVideoStream = nullptr;
	int nInputVideoIndex = -1;

	AVCodecContext* pInputAudioCtx = nullptr;
	AVStream* pInputAudioStream = nullptr;
	int nInputAudioIndex = -1;

	for (int i = 0; i < FCtxRead.get()->nb_streams; i++) {
		if (FCtxRead.get()->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO && pInputVideoCtx == nullptr) {
			pInputVideoStream = FCtxRead.get()->streams[i];
			pInputVideoCtx = pInputVideoStream->codec;
			nInputVideoIndex = i;

			//2022.11.04
			//Gavin反馈的某wxm视频无法转换的问题
			if (pInputVideoCtx->codec_id == AV_CODEC_ID_H264 && pInputVideoCtx->width == 0 && pInputVideoCtx->extradata) {
				int w = 0;
				int h = 0;
				int profile = 0;
				VideoDecoderGetSize(TRUE, pInputVideoCtx->extradata, pInputVideoCtx->extradata_size, &w, &h, &profile);

				pInputVideoCtx->width = w;
				pInputVideoCtx->height = h;
				pInputVideoCtx->coded_width = FFALIGN( w,32);
				pInputVideoCtx->coded_height = FFALIGN(h, 32);
				pInputVideoCtx->profile = profile;
			}

		}
		if (FCtxRead.get()->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO && pInputAudioCtx == nullptr) {
			pInputAudioStream = FCtxRead.get()->streams[i];
			pInputAudioCtx = pInputAudioStream->codec;
			nInputAudioIndex = i;
		}
	}
	if (nInputVideoIndex == -1 || nInputAudioIndex == -1) {
		WXLogW(L"[%ws][%ws] Not had audio adn video data!", __FUNCTIONW__, wszInput);
		return 0;
	}

	//输出配置
	WXString strOutput;
	strOutput.Format(wszOutput);
	AVFormatContext* _icWrite = NULL;
	WXString strExt = WXBase::GetFileNameSuffix(strOutput.str());
	int bXWS = 0;
	if (wcsicmp(strExt.str(), L"xws") == 0) {
		bXWS = 1;
	}

	//输出文件
	err = avformat_alloc_output_context2(&_icWrite, nullptr, bXWS ? "xws" : nullptr, strOutput.c_str());
	if (_icWrite == NULL) {
		WXLogW(L"[%ws][%ws] avformat_alloc_output_context2 error!", __FUNCTIONW__, wszOutput);
		return -1;
	}
	AVFormatContextPtr pFCtxWrite(_icWrite);

	AVStream* pOutputVideoStream = nullptr;
	AVCodecContext* pOutputVideoCtx = nullptr;
	int nOutputVideoIndex = -1;

	AVStream* pOutputAudioStream = nullptr;
	AVCodecContext* pOutputAudioCtx = nullptr;
	int nOutputAudioIndex = -1;
	//视频输出
	if (pInputVideoCtx) {
		pOutputVideoStream = avformat_new_stream(pFCtxWrite.get(), nullptr);
		if (pOutputVideoStream) {
			avcodec_copy_context(pOutputVideoStream->codec, pInputVideoCtx);
			pOutputVideoStream->codec->codec_tag = 0;
			nOutputVideoIndex = pOutputVideoStream->index;
			pOutputVideoCtx = pOutputVideoStream->codec;
			if (pFCtxWrite.get()->oformat->flags & AVFMT_GLOBALHEADER)
				pOutputVideoStream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		}
	}


	//音频输出
	if (pInputAudioCtx) {
		pOutputAudioStream = avformat_new_stream(pFCtxWrite.get(), nullptr);
		if (pOutputAudioStream) {
			avcodec_copy_context(pOutputAudioStream->codec, pInputAudioCtx);
			pOutputAudioCtx = pOutputAudioStream->codec;
			pOutputAudioStream->codec->codec_tag = 0;
			if (pFCtxWrite.get()->oformat->flags & AVFMT_GLOBALHEADER)
				pOutputAudioStream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
			nOutputAudioIndex = pOutputAudioStream->index;
		}
	}

	err = avio_open(&pFCtxWrite.get()->pb, strOutput.c_str(), AVIO_FLAG_WRITE);
	if (err < 0) { //创建输出流
		WXLogW(L"[%ws][%ws] avio_open error!", __FUNCTIONW__, wszOutput);
		return 0;
	}

	//Write file header
	err = avformat_write_header(pFCtxWrite.get(), NULL);
	if (err < 0) { //写文件头
		WXLogW(L"[%ws][%ws] avformat_write_header error!", __FUNCTIONW__, wszOutput);
		return 0;
	}

	//读文件-写文件 处理过程
	int nAudioDelay = audio_delay * pOutputAudioStream->time_base.den / pOutputAudioStream->time_base.num / 1000;
	AVPacket pkt;
	while (av_read_frame(FCtxRead.get(), &pkt) != AVERROR_EOF) { //还有时间戳转换！
		if (pkt.stream_index == nInputVideoIndex) {
			pkt.stream_index = nOutputVideoIndex;
			av_packet_rescale_ts(&pkt, pInputVideoStream->time_base,
				pOutputVideoStream->time_base);//转换视频时间戳
			av_interleaved_write_frame(pFCtxWrite.get(), &pkt);
		}
		else if (pkt.stream_index == nInputAudioIndex) { //修改音频时间戳，加上延迟
			pkt.stream_index = nOutputAudioIndex;
			av_packet_rescale_ts(&pkt, pInputAudioStream->time_base,
				pOutputAudioStream->time_base);//转换音频时间戳

			pkt.pts += nAudioDelay;
			pkt.dts += nAudioDelay;
			av_interleaved_write_frame(pFCtxWrite.get(), &pkt);
		}
		av_packet_unref(&pkt);
	}

	return 1;
}


//快速转换ts/flv/xws/xwm/mp4 到 mp4/xws
WXMEDIA_API int WXConvertFast(WXCTSTR wszInput, WXCTSTR wszOutput) {
	return WXConvertFast2(wszInput, wszOutput,0);
}

//快速转换ts/flv/xws/xwm/mp4 到 mp4/xws
// nTime为最大输出时间,0为不限制
WXMEDIA_API int WXConvertFast2(WXCTSTR wszInput, WXCTSTR wszOutput, int nTime) {
	//输入文件解析
	AVFormatContextPtr FCtxRead(wszInput);
	if (FCtxRead.get() == nullptr) {
		WXLogW(L"[%ws][%ws] avformat_open_input error", __FUNCTIONW__, wszInput);
		return -1;
	}

	if (avformat_find_stream_info(FCtxRead.get(), NULL) < 0) {
		WXLogA("[%s] avformat_find_stream_info error", __FUNCTION__);
		return 0;
	}

	//指向了_icRead 内部数据，不需要额外销毁
	AVCodecContext* pInputVideoCtx = nullptr;
	AVStream* pInputVideoStream = nullptr;
	int nInputVideoIndex = -1;

	AVCodecContext* pInputAudioCtx = nullptr;
	AVStream* pInputAudioStream = nullptr;
	int nInputAudioIndex = -1;

	for (int i = 0; i < FCtxRead.get()->nb_streams; i++) {
		if (FCtxRead.get()->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO && pInputVideoCtx == nullptr) {
			pInputVideoStream = FCtxRead.get()->streams[i];
			pInputVideoCtx = pInputVideoStream->codec;
			nInputVideoIndex = i;


			//2022.11.04
			//Gavin反馈的某wxm视频无法转换的问题
			//可以正常获取数据头，但是InputVideoCtx->width InputVideoCtx->height 是0，导致无法输出MP4文件
			if (pInputVideoCtx->codec_id == AV_CODEC_ID_H264 && pInputVideoCtx->width == 0 && pInputVideoCtx->extradata) {
				int w = 0;
				int h = 0;
				int profile = 0;
				VideoDecoderGetSize(TRUE, pInputVideoCtx->extradata, pInputVideoCtx->extradata_size, &w, &h, &profile);

				pInputVideoCtx->width = w;
				pInputVideoCtx->height = h;
				pInputVideoCtx->coded_width = FFALIGN(w, 32);
				pInputVideoCtx->coded_height = FFALIGN(h, 32);
				pInputVideoCtx->profile = profile;
			}
		}
		if (FCtxRead.get()->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO && pInputAudioCtx == nullptr) {
			pInputAudioStream = FCtxRead.get()->streams[i];
			pInputAudioCtx = pInputAudioStream->codec;
			nInputAudioIndex = i;
		}
	}
	if (nInputVideoIndex == -1 && nInputAudioIndex == -1) {
		return 0;
	}

	//输出配置
	WXString strOutput;
	strOutput.Format(wszOutput);
	AVFormatContext* _icWrite = NULL;

	//指向了_icRead 内部数据，不需要额外销毁
	AVStream* pOutputVideoStream = nullptr;
	AVCodecContext* pOutputVideoCtx = nullptr;

	AVStream* pOutputAudioStream = nullptr;
	AVCodecContext* pOutputAudioCtx = nullptr;

	int nOutputVideoIndex = -1;
	int nOutputAudioIndex = -1;

	WXString strExt = WXBase::GetFileNameSuffix(strOutput.str());
	int bXWS = 0;
	if (wcsicmp(strExt.str(), L"xws") == 0) {
		bXWS = 1;
	}
	//输出文件
	avformat_alloc_output_context2(&_icWrite, NULL, bXWS ? "xws" : nullptr, strOutput.c_str());
	if (_icWrite == NULL) {
		return -1;
	}

	AVFormatContextPtr pFCtxWrite(_icWrite);//自动释放

	//视频输出
	if (pInputVideoCtx) {
		//通过avformat_free_context销毁pOutputVideoStream
		pOutputVideoStream = avformat_new_stream(pFCtxWrite.get(), nullptr);
		if (pOutputVideoStream) {
			avcodec_copy_context(pOutputVideoStream->codec, pInputVideoCtx);
			pOutputVideoStream->codec->codec_tag = 0;
			nOutputVideoIndex = pOutputVideoStream->index;
			pOutputVideoCtx = pOutputVideoStream->codec;
			if (pFCtxWrite.get()->oformat->flags & AVFMT_GLOBALHEADER)
				pOutputVideoStream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		}
	}


	if (pInputAudioCtx) {
		//音频输出
		pOutputAudioStream = avformat_new_stream(pFCtxWrite.get(), nullptr);
		if (pOutputAudioStream) {
			avcodec_copy_context(pOutputAudioStream->codec, pInputAudioCtx);
			pOutputAudioCtx = pOutputAudioStream->codec;
			pOutputAudioStream->codec->codec_tag = 0;
			if (pFCtxWrite.get()->oformat->flags & AVFMT_GLOBALHEADER)
				pOutputAudioStream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
			nOutputAudioIndex = pOutputAudioStream->index;


			//FLAC  copy 有问题
			//需要专门处理
			if (pInputAudioCtx->codec_id == AV_CODEC_ID_FLAC) {
				int flac_extrasize = pInputAudioCtx->extradata_size;
				uint8_t* flac_extradata = pInputAudioCtx->extradata;
				if (flac_extrasize > 8) {
					if (memcmp(flac_extradata, "fLaC", 4) == 0) {
						av_free(pOutputAudioCtx->extradata);//释放

						pOutputAudioCtx->extradata_size = flac_extrasize - 8;
						pOutputAudioCtx->extradata = (uint8_t*)av_malloc(pOutputAudioCtx->extradata_size);
						memcpy(pOutputAudioCtx->extradata, flac_extradata+8, flac_extrasize-8);
					}
				}
			}
		}
	}


	if (avio_open(&pFCtxWrite.get()->pb, strOutput.c_str(), AVIO_FLAG_WRITE) < 0) { //创建输出流
		WXLogW(L"[%ws] avio_open[%ws] error", __FUNCTIONW__, strOutput.str());
		return 0;
	}

	//Write file header
	if (avformat_write_header(pFCtxWrite.get(), NULL) < 0) { //写文件头
		WXLogW(L"[%ws] avformat_write_header[%ws] error", __FUNCTIONW__, strOutput.str());
		return 0;
	}

	//读文件-写文件 处理过程
	int bRead = TRUE;
	AVPacket pkt;
	AVRational tbMS{ 1,1000 };


		 
	while (bRead && av_read_frame(FCtxRead.get(), &pkt) != AVERROR_EOF) { //还有时间戳转换！
		if (pkt.stream_index == nInputVideoIndex) {
			pkt.stream_index = nOutputVideoIndex;
			AVRational tbVideo = pInputVideoStream->time_base;
			int64_t ms = pkt.pts * 1000 * tbVideo.num / tbVideo.den;
			if (nTime && ms >= nTime * 1000) {
				bRead = FALSE;
			}
			av_packet_rescale_ts(&pkt, pInputVideoStream->time_base, pOutputVideoStream->time_base);//转换音频时间戳
			av_interleaved_write_frame(pFCtxWrite.get(), &pkt);
		}
		else if (pkt.stream_index == nInputAudioIndex) {
			pkt.stream_index = nOutputAudioIndex;
			AVRational tbAudio = pInputAudioStream->time_base;

			int64_t ms = pkt.pts * 1000 * tbAudio.num / tbAudio.den;
			if (nTime && ms >= nTime * 1000) {
				bRead = FALSE;
			}
			av_packet_rescale_ts(&pkt, pInputAudioStream->time_base, pOutputAudioStream->time_base);//转换音频时间戳
			av_interleaved_write_frame(pFCtxWrite.get(), &pkt);
		}
		av_packet_unref(&pkt);
	}


	return 1;
}


// 添加片头片尾功能
//转换进度

WXLocker g_lockMedia;
int g_nRate = 0; //转换进度
volatile bool g_bBreak = false;

WXMEDIA_API void WXMediaFileEditSetSpeed(int nSpeed) {
	//s_nSpeed = nSpeed;
}

WXMEDIA_API int WXMediaFileEditGetRate() {
	return std::max(0, std::min(100, g_nRate));
}

WXMEDIA_API void WXMediaFileEditBreak() {
	g_bBreak = true;
}


//编辑时的处理
extern int g_bEditDelogo;// = 0;
extern int g_nEditDelogoX;//  = 0;
extern int g_nEditDelogoY;//  = 0;
extern int g_nEditDelogoW;//  = 0;
extern int g_nEditDelogoH;//  = 0;


//改变AVFrame的亮度，限制最暗为50%
static void AVFrameLuma(AVFrame *dstFrame, const int dst, const AVFrame *srcFrame,const int src) {
	if (dstFrame->format == srcFrame->format) {
		if (dstFrame->format == AV_PIX_FMT_RGB32) {
			av_frame_copy(dstFrame, srcFrame);
			if (dst == 0) {
				memset(dstFrame->data[0], 0, dstFrame->linesize[0] * dstFrame->height);
			}else if (src && (dst != src)) {
				for (int64_t i = 0; i < dstFrame->linesize[0] * dstFrame->height; i++)
					dstFrame->data[0][i] = srcFrame->data[0][i] * (dst * 40 + src * 60) / (src * 100);
			}
		}else if (dstFrame->format == AV_PIX_FMT_YUV420P || dstFrame->format == AV_PIX_FMT_YUVJ420P) {
			av_frame_copy(dstFrame, srcFrame);
			if (dst == 0) {
				memset(dstFrame->data[0], 0, dstFrame->linesize[0] * dstFrame->height);
			}else if (src && (dst != src)) {
				for (int64_t i = 0; i < dstFrame->linesize[0] * dstFrame->height; i++)
					dstFrame->data[0][i] = srcFrame->data[0][i] * (dst * 40 + src * 60) / (src * 100);
			}
		}
	}
}

int WXMediaFileEditWithParam(WXCTSTR wszInput,//输入文件
	int mode,    //画质  0-1-2
	int fps,     //帧率，填充片头片尾需要
	int bitrate, //设置的帧率和码率
	WXCTSTR wszOutput,  //输出文件
	int64_t ptsStart = 0,  //裁剪开始时间，单位ms
	int64_t ptsStop  = 0,   //裁剪结束时间，单位ms
	WXCTSTR wszHead  = nullptr, 
	int64_t ptsHead = 0, //片头图片及长度， 单位ms
	WXCTSTR wszTrail = nullptr, 
	int64_t ptsTaril = 0,//片头图片及长度， 单位ms
	WXCTSTR wszWaterMark = nullptr) {
	WXAutoLock al(g_lockMedia);
	g_bBreak = false;
	int ret = WX_ERROR_ERROR;
	int64_t ptsFile = 0;
	int error = 0;
	int width = 0;
	int height = 0;//要编码的高度和宽度
    int nSampleRate = 0;
    int nChannel = 0;
    int nSize10MS = 0;
    int nAudioBitrate = 96000;
	void *info = WXMediaInfoCreate(wszInput, &error);
	if (info) {
		ptsFile = WXMediaInfoGetFileDuration(info);
		width  = WXMediaInfoGetVideoWidth(info);
		height = WXMediaInfoGetVideoHeight(info);
        nSampleRate = WXMediaInfoGetAudioSampleRate(info);
        nChannel = WXMediaInfoGetAudioChannels(info);
        nSize10MS = nSampleRate * nChannel * 2 / 100;
		nAudioBitrate = WXMediaInfoGetAudioBitrate(info);
		WXMediaInfoDestroy(info);
	}else {
		WXLogA("Parser File Error");
		return ret;
	}
	if (width <= 0 || height <= 0) {
		WXLogA("Video Size Error");
		return ret;
	}
	int  iFps = fps;
	if (iFps == 0)iFps = 24;
	int64_t timeVideo = 1000 / iFps;
	std::shared_ptr<FfmpegMuxer>mFfmpegMuxer = nullptr;
	int64_t ptsCurrVideo = 0;
	int64_t ptsCurrAudio = 0;
	int64_t ptsCurrMax = 0;
    WXString strMp4;
    strMp4.Format(wszOutput);
	if (wszHead  && ptsHead  < 3 * 1000)
		ptsHead = 3 * 1000;

	if (wszTrail && ptsTaril < 3 * 1000)
		ptsTaril = 3 * 1000;

	WXDataBuffer m_AudioData;
	m_AudioData.Init(nullptr, nSize10MS);

	int64_t totalPts = 0;// 总时间

	bool CutFile = false;
	if (ptsStart || ptsStop) {
		CutFile = true;
		totalPts += (ptsStop - ptsStart);//总时间
	}else {
		totalPts += ptsFile;
	}

	totalPts += ptsHead;
	totalPts += ptsTaril;//总长度
	if (totalPts <= 0) {
		WXLogA("Total Time Error");
		totalPts = 1;//异常情况
	}

	WXImageWaterMark pMarkWM(wszWaterMark, width, height);//水印图片

	WXImageWaterMark pMarkHead((wszHead && ptsHead) ? wszHead : nullptr, width, height);//片头图片
	WXImageWaterMark pMarkHeadDst((wszHead && ptsHead) ? wszHead : nullptr, width, height);//片头图片(渐变)

	WXImageWaterMark pMarkTrail((wszTrail&&ptsTaril) ? wszTrail : nullptr, width, height);//片尾图片
	WXImageWaterMark pMarkTrailDst((wszTrail&&ptsTaril) ? wszTrail : nullptr, width, height);//片尾图片(渐变)

	int64_t ptsCurr = 0;
	//片头视频
	AVFrame *pFrameHead    = pMarkHead.m_RgbaFrame.GetFrame();
	AVFrame *pFrameHeadDst = pMarkHeadDst.m_RgbaFrame.GetFrame();

	if (!g_bBreak && pFrameHead) {
		mFfmpegMuxer = std::make_shared< FfmpegMuxer>();
        mFfmpegMuxer->SetVideoConfig(width, height, iFps, bitrate, mode,0, 0, 0);
        mFfmpegMuxer->SetAudioConfig(nSampleRate, nChannel, nAudioBitrate);
		ret = mFfmpegMuxer->Open(strMp4.str(), TRUE);
		if (ret != WX_ERROR_SUCCESS) {
			WXLogW(L"Create FfmpegMuxer Error");
			mFfmpegMuxer = nullptr;
			g_bBreak = TRUE;
		}
		if (mFfmpegMuxer) {
			int64_t ptsAudioTime = 0;
			for (int64_t ptsVideoTime = 0; ptsVideoTime < ptsHead; ptsVideoTime += timeVideo) {
				AVFrameLuma(pFrameHeadDst, ptsHead - ptsVideoTime, pFrameHead, ptsHead);//逐渐变暗
				//if (pWmLogo) {
				//	pWmLogo->DxgiMask(pFrameHeadDst, nPosX, nPosY);
				//}
				pFrameHeadDst->pts = ptsVideoTime;
				mFfmpegMuxer->WriteVideoFrame(pFrameHeadDst);//写入视频
				ptsCurrVideo = ptsVideoTime;//当前视频时间
				ptsCurr = ptsVideoTime;
				g_nRate = 100 * ptsCurr / totalPts;//片头进度条
				while (ptsAudioTime <= ptsVideoTime) {
					mFfmpegMuxer->WriteAudioFrame(m_AudioData.GetBuffer(), nSize10MS); //写入音频
					ptsAudioTime += 10;
					ptsCurrAudio = ptsAudioTime;//当前视频时间
				}
			}
		}
		ptsCurrMax = FFMAX(ptsCurrAudio, ptsCurrVideo);
	}

	//源视频，拷贝或者切割
	AVCodecContext *pVideoCtx = nullptr;
	AVFrame *pVideoFrame = nullptr;

	WXVideoFrame pVideoFrameDecode1;//解码图像
	WXVideoFrame pVideoFrameDecode2;//处理后的图像

	AVRational tbVideo{ 1,1000 }; //转ms

	AVCodecContext *pAudioCtx = nullptr;
	AVFrame *pAudioFrame = nullptr;
	AVFrame *pAudioFrameS16 = nullptr;
	SwrContext* pSwrCtx = nullptr;

	AVRational tbAudio{ 1,1000 }; //转ms

	AVFormatContextPtr FCtxRead(wszInput);
	if (FCtxRead.get() == NULL) {
		g_bBreak = true;
		return -1;
	}
	if (!g_bBreak) {
		//封装格式上下文，统领全局的结构体，保存了视频文件封装格式的相关信息
		if (!g_bBreak) {
			int video_index = -1;
			int audio_index = -1;
			for (int i = 0; i < FCtxRead.get()->nb_streams; i++) {
				if (FCtxRead.get()->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO
					&& pVideoCtx == nullptr) {
					AVCodecContext *ctx = FCtxRead.get()->streams[i]->codec;
					AVCodec *codec = avcodec_find_decoder(ctx->codec_id);
					if (avcodec_open2(ctx, codec, NULL) >= 0) {
						pVideoFrame = av_frame_alloc();
						video_index = i;
						pVideoCtx = ctx;
						tbVideo.den = FCtxRead.get()->streams[i]->time_base.den;
						tbVideo.num = FCtxRead.get()->streams[i]->time_base.num;
					}
				}
				if (FCtxRead.get()->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO
					&& pAudioCtx == nullptr) {
					AVCodecContext *ctx = FCtxRead.get()->streams[i]->codec;
					AVCodec *codec = avcodec_find_decoder(ctx->codec_id);
					if (avcodec_open2(ctx, codec, NULL) >= 0) {
						pAudioFrame = av_frame_alloc();
						audio_index = i;
						pAudioCtx = ctx;
						tbAudio.den = FCtxRead.get()->streams[i]->time_base.den;
						tbAudio.num = FCtxRead.get()->streams[i]->time_base.num;
						if (pAudioCtx->sample_fmt != AV_SAMPLE_FMT_S16) {
							pAudioFrameS16 = WXMediaUtilsAllocAudioFrame(pAudioCtx->sample_fmt, nSampleRate, nChannel, nSampleRate);//48000
							pSwrCtx = WXMediaUtilsAllocSwrCtx(nSampleRate, nChannel, pAudioCtx->sample_fmt, nSampleRate, nChannel, AV_SAMPLE_FMT_S16);
						}
					}
				}
			}
			if (pAudioCtx == nullptr && pVideoCtx == nullptr) {
				WXLogW(L"不存在音视频数据");
				return WX_ERROR_ERROR;
			}

			//视频文件解码+编码
			if (mFfmpegMuxer.get() == nullptr) {
				mFfmpegMuxer = std::make_shared< FfmpegMuxer>();
                mFfmpegMuxer->SetVideoConfig(width, height, iFps, bitrate, mode, 0, 0, 0);
                mFfmpegMuxer->SetAudioConfig(nSampleRate, nChannel, nAudioBitrate);
                ret = mFfmpegMuxer->Open(strMp4.str(), TRUE);
				if (ret != WX_ERROR_SUCCESS) {
					WXLogW(L"Create FfmpegMuxer Error");
					mFfmpegMuxer = nullptr;
					g_bBreak = true;
				}
			}

			 if(!g_bBreak){
				bool CutVideoStop = false;
				bool CutAudioStop = false;
				bool UseFirst = false;
				AVPacket pkt;
				av_init_packet(&pkt);
				while (!g_bBreak && av_read_frame(FCtxRead.get(), &pkt) >= 0) {
					int got_picture = 0;

					if (pkt.stream_index == video_index && pVideoCtx) { //视频处理
						int ret = avcodec_decode_video2(pVideoCtx, pVideoFrame, &got_picture, &pkt);
						if (got_picture) {//时间戳换算为ms

							{
								if (pVideoFrameDecode1.GetFrame() == nullptr) {
									pVideoFrameDecode1.Init((AVPixelFormat)pVideoFrame->format, width, height);
									pVideoFrameDecode2.Init((AVPixelFormat)pVideoFrame->format, width, height);
								}
							}


							int64_t ptsVideo = pVideoFrame->pts * 1000 * tbVideo.num / tbVideo.den;//换算为ms

							if (!CutFile || (CutFile && ptsVideo > ptsStart - 1 && ptsVideo < ptsStop + 1)) {
								
								//去水印
								if (g_bEditDelogo && (pVideoFrame->format == AV_PIX_FMT_YUV420P || pVideoFrame->format == AV_PIX_FMT_YUVJ420P)) {
									av_frame_copy(pVideoFrameDecode1.GetFrame(), pVideoFrame);//拷贝数据
									pMarkWM.MaskPicture(pVideoFrameDecode1.GetFrame());  //添加水印图像
									//编辑时去水印
									WXMediaDelogo(
										pVideoFrameDecode2.GetFrame()->data[0], pVideoFrameDecode2.GetFrame()->linesize[0],
										pVideoFrameDecode1.GetFrame()->data[0], pVideoFrameDecode1.GetFrame()->linesize[0],
										pVideoFrameDecode1.GetFrame()->width, pVideoFrameDecode1.GetFrame()->height,
										g_nEditDelogoX, g_nEditDelogoY, g_nEditDelogoW, g_nEditDelogoH
									);

									WXMediaDelogo(
										pVideoFrameDecode2.GetFrame()->data[1], pVideoFrameDecode2.GetFrame()->linesize[1],
										pVideoFrameDecode1.GetFrame()->data[1], pVideoFrameDecode1.GetFrame()->linesize[1],
										pVideoFrameDecode1.GetFrame()->width / 2, pVideoFrameDecode1.GetFrame()->height / 2,
										g_nEditDelogoX / 2, g_nEditDelogoY / 2, g_nEditDelogoW / 2, g_nEditDelogoH / 2
									);
									WXMediaDelogo(
										pVideoFrameDecode2.GetFrame()->data[2], pVideoFrameDecode2.GetFrame()->linesize[2],
										pVideoFrameDecode1.GetFrame()->data[2], pVideoFrameDecode1.GetFrame()->linesize[2],
										pVideoFrameDecode1.GetFrame()->width / 2, pVideoFrameDecode1.GetFrame()->height / 2,
										g_nEditDelogoX / 2, g_nEditDelogoY / 2, g_nEditDelogoW / 2, g_nEditDelogoH / 2
									);
								}else {
									av_frame_copy(pVideoFrameDecode2.GetFrame(), pVideoFrame);//拷贝数据
									pMarkWM.MaskPicture(pVideoFrameDecode2.GetFrame());  //添加水印图像
								}

								pVideoFrameDecode2.GetFrame()->pts = ptsVideo - ptsStart;
								ptsCurr = pVideoFrameDecode2.GetFrame()->pts + ptsHead;
								int newRate = 100 * ptsCurr / totalPts;//转换
								if (newRate == 100)newRate = 99;
								g_nRate = newRate;

								pVideoFrameDecode2.GetFrame()->pts += ptsCurrMax;
								ptsCurrVideo = pVideoFrameDecode2.GetFrame()->pts;

								if (!UseFirst) {
									UseFirst = true;
									pVideoFrameDecode2.GetFrame()->pict_type = AV_PICTURE_TYPE_I;
									pVideoFrameDecode2.GetFrame()->key_frame = 1;
								}else {
									pVideoFrameDecode2.GetFrame()->pict_type = AV_PICTURE_TYPE_NONE;
									pVideoFrameDecode2.GetFrame()->key_frame = 0;
								}
								mFfmpegMuxer->WriteVideoFrame(pVideoFrameDecode2.GetFrame());//重新编码处理							
							}

							if (CutFile && ptsVideo > ptsStop) {
								CutVideoStop = true;
							}
						}
					}else if (pkt.stream_index == audio_index && pAudioCtx) {  //音频处理
						int ret = avcodec_decode_audio4(pAudioCtx, pAudioFrame, &got_picture, &pkt);
						if (got_picture) {//时间戳换算为ms
							int64_t ptsAudio = pAudioFrame->pts * 1000 * tbAudio.num / tbAudio.den;
							if (!CutFile || (CutFile && ptsAudio > ptsStart - 1 && ptsAudio < ptsStop + 1)) {
								if (pSwrCtx) {
									swr_convert(pSwrCtx, pAudioFrameS16->data, pAudioFrame->nb_samples, (const uint8_t**)pAudioFrame->data, pAudioFrame->nb_samples);
									mFfmpegMuxer->WriteAudioFrame(pAudioFrameS16->data[0], pAudioFrame->nb_samples * 4);
								}else {
									mFfmpegMuxer->WriteAudioFrame(pAudioFrame->data[0], pAudioFrame->linesize[0]);
								}
								ptsCurrAudio += 10;
							}
							if (CutFile && ptsAudio > ptsStop) {
								CutAudioStop = true;
							}
						}
					}

					//释放资源
					av_packet_unref(&pkt);
					if (CutAudioStop && CutVideoStop)
						break;
				}
			}
			ptsCurrMax = FFMAX(ptsCurrVideo, ptsCurrAudio);
		}
	}

	//片尾视频
	AVFrame *pFrameTrail = pMarkTrail.m_RgbaFrame.GetFrame();
	AVFrame *pFrameTrailDst = pMarkTrailDst.m_RgbaFrame.GetFrame();
	if (!g_bBreak && mFfmpegMuxer && pFrameTrail) {
		if (!g_bBreak) {
			int64_t ptsAudioTime = 0;
			int64_t m_no = 0;
			for (int64_t ptsVideoTime = 0; ptsVideoTime < ptsTaril; ptsVideoTime += timeVideo) {
				AVFrameLuma(pFrameTrailDst, ptsVideoTime, pFrameTrail, ptsTaril);
				pFrameTrailDst->pts = ptsVideoTime;
				//pMark.Mask2(pFrameTrailDst);
				//if (pWmLogo) { //片尾全局水印
				//	pWmLogo->DxgiMask(pFrameHeadDst, nPosX, nPosY);
				//}
				pFrameTrailDst->pts += ptsCurrMax;
				if (m_no < 2) { //第一帧要设置为关键帧
					pFrameTrailDst->pict_type = AV_PICTURE_TYPE_I;
					pFrameTrailDst->key_frame = 1;
				}else {
					pFrameTrailDst->pict_type = AV_PICTURE_TYPE_NONE;
					pFrameTrailDst->key_frame = 0;
				}
				m_no++;

				mFfmpegMuxer->WriteVideoFrame(pFrameTrailDst);

				int64_t ptsCurr2 = ptsCurr + ptsVideoTime;
				int newRate = 100 * ptsCurr2 / totalPts;//转换
				if (newRate == 100)newRate = 99;
				g_nRate = newRate;//转换进度

				while (ptsAudioTime <= ptsVideoTime) {
					mFfmpegMuxer->WriteAudioFrame(m_AudioData.GetBuffer(), nSize10MS); //写入音频
					ptsAudioTime += 10;
					ptsCurrAudio += 10;
				}
			}
		}

	}

//	SAFE_DELETE(pWmLogo);
	SAFE_SWR_FREE(pSwrCtx);
	SAFE_AV_FREE(pAudioFrameS16);

	if (pVideoCtx) {
		avcodec_close(pVideoCtx);
		pVideoCtx = nullptr;
	}
	if (pAudioCtx) {
		avcodec_close(pAudioCtx);
		pAudioCtx = nullptr;
	}

	SAFE_AV_FREE(pVideoFrame)

	if (g_bBreak) {  //合并文件,保存参数
		g_nRate = 0;//中断处理
		ret = FFMPEG_ERROR_BREADK;
	}else {
		g_nRate = 100;
		ret = FFMPEG_ERROR_OK;
	}
	if (mFfmpegMuxer.get()) {
		mFfmpegMuxer->Close();
		mFfmpegMuxer = nullptr;
	}
	g_bBreak = FALSE;
	return ret;
}

WXMEDIA_API int WXMediaFileEdit(WXCTSTR wszInput,  //输入文件
	WXCTSTR wszOutput,  //输出文件
	int64_t ptsStart,  //裁剪开始时间，单位ms
	int64_t ptsStop,   //裁剪结束时间，单位ms
	WXCTSTR wszHead, int64_t ptsHead, //片头图片及长度， 单位ms
	WXCTSTR wszTrail, int64_t ptsTaril,//片头图片及长度， 单位ms
	WXCTSTR wszWaterMark) {
	return WXMediaFileEditWithParam(wszInput, MODE_NORMAL, 24,0,wszOutput, ptsStart,ptsStop,wszHead, ptsHead,wszTrail, ptsTaril, wszWaterMark);
}

WXMEDIA_API int WXMediaAddFile(WXCTSTR wszInput,  //输入文件
	WXCTSTR wszOutput,
	WXCTSTR wszHead, int64_t ptsHead, //片头图片及长度， 单位ms
	WXCTSTR wszTrail, int64_t ptsTaril//片头图片及长度， 单位ms
	) {
	return WXMediaFileEditWithParam(wszInput, MODE_NORMAL, 24, 0, wszOutput, 0, 0,
		wszHead, ptsHead,wszTrail, ptsTaril, nullptr);
}


//---------------------------------------------------------------------------------------------------------
//单纯裁剪处理
WXMEDIA_API int WXMediaCutFile(WXCTSTR wszInput,  //输入文件
	int fps, int bitrate,
	WXCTSTR wszOutput,  //输出文件
	int64_t ptsStart,  //裁剪开始时间，单位ms
	int64_t ptsStop   //裁剪结束时间，单位ms
) {
	return WXMediaFileEditWithParam(wszInput, MODE_NORMAL, fps, bitrate, wszOutput, ptsStart, ptsStop,
		nullptr, 0,nullptr, 0, nullptr);
}

WXMEDIA_API int WXMediaFileEdit2(WXCTSTR wszInput,  //输入文件
	WXCTSTR wszOutput,  //输出文件
	int64_t ptsStart,  //裁剪开始时间，单位ms
	int64_t ptsStop   //裁剪结束时间，单位ms
) {
	WXLogW(L"%ws %ws %ws %lld %lld", __FUNCTIONW__, wszInput, wszOutput, ptsStart, ptsStop);
	return WXMediaFileEditWithParam(wszInput, MODE_NORMAL, 24, 0, wszOutput, ptsStart, ptsStop,
		nullptr, 0,nullptr, 0, nullptr);
}


static std::vector<WXString>s_arrFileName;
WXMEDIA_API void WXMediaMergerFileReset() {
	WXAutoLock al(g_lockMedia);
	s_arrFileName.clear();
}

WXMEDIA_API void WXMediaMergerFileAdd(WXCTSTR wszName) {
	WXAutoLock al(g_lockMedia);
	s_arrFileName.push_back(wszName);
}

WXMEDIA_API int WXMediaMergerFileProcess(WXCTSTR wszOutput, int bVip, int nVideoMode, int nDstWidth, int nDstHeight, int nFps) {

	WXAutoLock al(g_lockMedia);

	g_nRate = 0;//Reset

	int countFile = s_arrFileName.size();
	if (countFile < 2) {
		WXLogW(L"s_arrFileName Has %d File", countFile);
		return 0;
	}

	g_bBreak = false;
	int ret = WX_ERROR_ERROR;

	int64_t ptsTotal = 0;//总时长
	std::vector<int64_t>arrTime;

	nDstWidth  = nDstWidth / 4 * 4;
	nDstHeight = nDstHeight / 4 * 4;

	for (size_t index = 0; index < countFile; index++){
		int error = 0;
		void *info = WXMediaInfoCreate(s_arrFileName[index].str(), &error);
		if (info) {
			int64_t ptsFile = WXMediaInfoGetFileDuration(info);
			ptsTotal += ptsFile;
			arrTime.push_back(ptsFile);
			if (index == 0) {
				//以第一个文件的分辨率作为输出
				if (nDstWidth == 0 || nDstHeight == 0) {
					nDstWidth = WXMediaInfoGetVideoWidth(info);
					nDstHeight = WXMediaInfoGetVideoHeight(info);
					WXLogW(L"WXMediaMergerFileProcess Read Video Size %dx%d", nDstWidth, nDstHeight);
				}

				if (nDstWidth <= 0 || nDstHeight <= 0) {
					WXLogW(L"WXMediaMergerFileProcess Video Size Error");
					return ret;
				}
			}
			WXMediaInfoDestroy(info);
		}else {
			WXLogW(L"WXMediaMergerFileProcess Parser File %ws Error", s_arrFileName[index].str());
			return ret;
		}
	}

	//输入则使用24fps
	if (nFps == 0)
		nFps = 24;
	int64_t timeVideo = 1000 / nFps;

	//视频文件解码+编码
	std::shared_ptr< FfmpegMuxer>pMuxer = std::make_shared< FfmpegMuxer>();
	pMuxer->SetVideoConfig(nDstWidth, nDstHeight, nFps, 0, nVideoMode, 0, 0, 0);
	pMuxer->SetAudioConfig(48000, 2, 128000);
	ret = pMuxer->Open(wszOutput);
	if (ret != WX_ERROR_SUCCESS) {
		WXLogW(L"Create FfmpegMuxer Error");
		pMuxer = nullptr;
		g_bBreak = true;
		return WX_ERROR_ERROR;
	}

	int64_t tsLast = 0;//累积时间
	int64_t tsCurrVideo = 0;//当前时间
	int64_t tsCurrAudio = 0;

	int64_t nTime = 0;
	if (!bVip) {
		ptsTotal = ptsTotal * 3 / 10;
	}

	for (size_t index = 0; index < countFile; index++)  {
		AVCodecContext *pVideoCtx = nullptr;
		AVFrame *pVideoFrame = nullptr;

		AVRational tbVideo{ 1,1000 }; //转ms

		AVCodecContext *pAudioCtx = nullptr;//音频解码器
		AVFrame *pAudioFrame = nullptr;//音频解码帧
		AVFrame *pAudioFrameS16 = nullptr;//对应的S16数据
		SwrContext* pSwrCtx = nullptr;//音频转换
		AudioResampler mResampler(L"MediaEdit");//
		WXDataBuffer mData;

		int mSampleRate = 48000;
		int mChannels = 2;

		AVRational tbAudio{ 1,1000 }; //转ms
		WXVideoFrame videoFrame;
		WXVideoConvert conv;
		AVFormatContextPtr FCtxRead(s_arrFileName[index].str());
		//封装格式上下文，统领全局的结构体，保存了视频文件封装格式的相关信息
		if (FCtxRead.get() == NULL) {
			WXLogW(L"[%ws]avformat_open_input error", s_arrFileName[index].str());
			g_bBreak = true;
			return WX_ERROR_ERROR;
		}
		if (!g_bBreak && avformat_find_stream_info(FCtxRead.get(), NULL) < 0) {
			WXLogA("avformat_find_stream_info error");
			g_bBreak = true;
			return WX_ERROR_ERROR;
		}

		int video_index = -1;
		int audio_index = -1;
		for (int i = 0; i < FCtxRead.get()->nb_streams; i++) {
			if (FCtxRead.get()->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO
				&& pVideoCtx == nullptr) {
				AVCodecContext *ctx = FCtxRead.get()->streams[i]->codec;
				AVCodec *codec = avcodec_find_decoder(ctx->codec_id);
				if (avcodec_open2(ctx, codec, NULL) >= 0) {
					pVideoFrame = av_frame_alloc();
					video_index = i;
					pVideoCtx = ctx;
					tbVideo.den = FCtxRead.get()->streams[i]->time_base.den;
					tbVideo.num = FCtxRead.get()->streams[i]->time_base.num;
				}
			}
			if (FCtxRead.get()->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO
				&& pAudioCtx == nullptr) {
				AVCodecContext *ctx = FCtxRead.get()->streams[i]->codec;
				AVCodec *codec = avcodec_find_decoder(ctx->codec_id);
				if (avcodec_open2(ctx, codec, NULL) >= 0) {
					pAudioFrame = av_frame_alloc();
					audio_index = i;
					pAudioCtx = ctx;
					tbAudio.den = FCtxRead.get()->streams[i]->time_base.den;
					tbAudio.num = FCtxRead.get()->streams[i]->time_base.num;
					mSampleRate = ctx->sample_rate;
					mChannels = ctx->channels;
					pAudioFrameS16 = WXMediaUtilsAllocAudioFrame(AV_SAMPLE_FMT_S16, mSampleRate, mChannels, mSampleRate);//48000
					pSwrCtx = WXMediaUtilsAllocSwrCtx(mSampleRate, mChannels, pAudioCtx->sample_fmt,
						mSampleRate, mChannels, AV_SAMPLE_FMT_S16);
					mResampler.Init(FALSE, mSampleRate, mChannels, FALSE, 48000, 2);
					mData.Init(NULL, 192000 * 4);
				}
			}
		}
		if (pAudioCtx == nullptr && pVideoCtx == nullptr) {
			WXLogW(L"不存在音视频数据");
			return WX_ERROR_ERROR;
		}

		int64_t nTime = 0;
		if (!bVip) {
			nTime = arrTime[index] * 3 / 10;
		}
		{
			bool UseFirst = false;
			AVPacket pkt;
			av_init_packet(&pkt);
			while (!g_bBreak && av_read_frame(FCtxRead.get(), &pkt) >= 0) {
				int got_picture = 0;

				if (pkt.stream_index == video_index && pVideoCtx) { //视频处理
					int ret = avcodec_decode_video2(pVideoCtx, pVideoFrame, &got_picture, &pkt);
					if (got_picture) {//时间戳换算为ms

						int64_t ptsVideo = pVideoFrame->pts * 1000 * tbVideo.num / tbVideo.den;//换算为ms

						if (nTime > 0 && ptsVideo > nTime) { //时间限制
															 //释放资源
							av_packet_unref(&pkt);
							break;
						}

						if (videoFrame.GetFrame() == nullptr) {
							videoFrame.Init(AV_PIX_FMT_YUV420P, pVideoFrame->width, pVideoFrame->height);
						}
						conv.Convert(pVideoFrame, videoFrame.GetFrame());

						if (!UseFirst) {
							UseFirst = true;
							videoFrame.GetFrame()->pict_type = AV_PICTURE_TYPE_I;
							videoFrame.GetFrame()->key_frame = 1;
						}
						else {
							videoFrame.GetFrame()->pict_type = AV_PICTURE_TYPE_NONE;
							videoFrame.GetFrame()->key_frame = 0;
						}
						tsCurrVideo = tsLast + ptsVideo;
						int Rate = tsCurrVideo * 100 / ptsTotal;
						g_nRate = MAX(g_nRate, Rate);
						videoFrame.GetFrame()->pts = tsCurrVideo;
						pMuxer->WriteVideoFrame(videoFrame.GetFrame());
					}
				}
				else if (pkt.stream_index == audio_index && pAudioCtx) {  //音频处理
					int ret = avcodec_decode_audio4(pAudioCtx, pAudioFrame, &got_picture, &pkt);
					if (got_picture) {//时间戳换算为ms
						int64_t ptsAudio = pAudioFrame->pts * 1000 * tbAudio.num / tbAudio.den;

						if (nTime > 0 && ptsAudio > nTime) { //时间限制
															 //释放资源
							av_packet_unref(&pkt);
							break;
						}

						tsCurrAudio = tsLast + ptsAudio;
						int Rate = tsCurrAudio * 100 / ptsTotal;
						g_nRate = MAX(g_nRate, Rate);
						AVFrame *dstFrame = pAudioFrame;
						swr_convert(pSwrCtx, pAudioFrameS16->data, pAudioFrame->nb_samples,
							(const uint8_t**)pAudioFrame->data, pAudioFrame->nb_samples);


						mResampler.Write(pAudioFrameS16->data[0], pAudioFrame->nb_samples * 2 * mChannels);

						int outSize = mResampler.Size();
						if (outSize > 0) {
							mResampler.Read(mData.GetBuffer(), outSize);
							pMuxer->WriteAudioFrame(mData.GetBuffer(), outSize);
						}

					}
				}
				//释放资源
				av_packet_unref(&pkt);
			}
		}

		SAFE_SWR_FREE(pSwrCtx);
		SAFE_AV_FREE(pAudioFrameS16);

		if (pVideoCtx) {
			avcodec_close(pVideoCtx);
			pVideoCtx = nullptr;
		}
		if (pAudioCtx) {
			avcodec_close(pAudioCtx);
			pAudioCtx = nullptr;
		}

		SAFE_AV_FREE(pVideoFrame)

		tsLast = MAX(tsCurrAudio, tsCurrVideo) + timeVideo;
	}


	if (g_bBreak) {  //合并文件,保存参数
		g_nRate = 0;//中断处理
		ret = FFMPEG_ERROR_BREADK;
	}else {
		g_nRate = 100;
		ret = FFMPEG_ERROR_OK;
	}

	if (pMuxer.get()){
		pMuxer->Close();
		pMuxer = nullptr;
	}

	g_bBreak = FALSE;
	return ret;
}

//
static int IsA(AVFrame* frame) {
	if (frame->format == AV_PIX_FMT_YUV420P || frame->format == AV_PIX_FMT_YUVJ420P)
	{
		double sum = 0;
		for (int h = 0; h < frame->height; h++) {
			for (int w = 0; w < frame->width; w++) {
				sum += frame->data[0][w + h * frame->linesize[0]];
			}
		}
		double avg = sum / (frame->width * frame->height);
		return avg >= 18;
	}
	return 1;
}


//去除黑屏
WXMEDIA_API int WXMediaFileEdit3(WXCTSTR wszInput, WXCTSTR wszOutput) {

	WXAutoLock al(g_lockMedia);
	g_bBreak = false;
	int ret = WX_ERROR_ERROR;
	int64_t ptsFile = 0;
	int error = 0;
	int width = 0;
	int height = 0;//要编码的高度和宽度
	int nSampleRate = 0;
	int nChannel = 0;
	int nAudioBitrate = 96000;
	void *info = WXMediaInfoCreate(wszInput, &error);
	if (info) {
		ptsFile = WXMediaInfoGetFileDuration(info);
		width = WXMediaInfoGetVideoWidth(info);
		height = WXMediaInfoGetVideoHeight(info);
		nSampleRate = WXMediaInfoGetAudioSampleRate(info);
		nChannel = WXMediaInfoGetAudioChannels(info);
		nAudioBitrate = WXMediaInfoGetAudioBitrate(info);
		WXMediaInfoDestroy(info);
	}
	else {
		WXLogA("Parser File Error");
		return ret;
	}
	if (width <= 0 || height <= 0) {
		WXLogA("Video Size Error");
		return ret;
	}
	int  iFps = 24;
	if (iFps == 0)iFps = 24;
	std::shared_ptr<FfmpegMuxer>mFfmpegMuxer = nullptr;
	int64_t ptsCurrVideo = 0;
	int64_t ptsCurrAudio = 0;
	int64_t ptsCurrMax = 0;
	WXString strMp4;
	strMp4.Format(wszOutput);


	AVCodecContext *pVideoCtx = nullptr;
	AVFrame *pVideoFrame = nullptr;
	AVFrame *pVideoFrameDecode = nullptr;

	AVRational tbVideo{ 1,1000 }; //转ms

	AVCodecContext *pAudioCtx = nullptr;
	AVFrame *pAudioFrame = nullptr;
	AVFrame *pAudioFrameS16 = nullptr;
	SwrContext* pSwrCtx = nullptr;

	AVRational tbAudio{ 1,1000 }; //转ms


	if (!g_bBreak) {
		AVFormatContextPtr FCtxRead(wszInput);
		if (FCtxRead.get() == NULL) {
			g_bBreak = true;
		}

		if (!g_bBreak && avformat_find_stream_info(FCtxRead.get(), NULL) < 0) {
			WXLogA("avformat_find_stream_info error");
			g_bBreak = true;
		}

		if (!g_bBreak) {
			int video_index = -1;
			int audio_index = -1;
			for (int i = 0; i < FCtxRead.get()->nb_streams; i++) {
				if (FCtxRead.get()->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO
					&& pVideoCtx == nullptr) {
					AVCodecContext *ctx = FCtxRead.get()->streams[i]->codec;
					AVCodec *codec = avcodec_find_decoder(ctx->codec_id);
					if (avcodec_open2(ctx, codec, NULL) >= 0) {
						pVideoFrame = av_frame_alloc();
						video_index = i;
						pVideoCtx = ctx;
						tbVideo.den = FCtxRead.get()->streams[i]->time_base.den;
						tbVideo.num = FCtxRead.get()->streams[i]->time_base.num;
					}
				}
				if (FCtxRead.get()->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO
					&& pAudioCtx == nullptr) {
					AVCodecContext *ctx = FCtxRead.get()->streams[i]->codec;
					AVCodec *codec = avcodec_find_decoder(ctx->codec_id);
					if (avcodec_open2(ctx, codec, NULL) >= 0) {
						pAudioFrame = av_frame_alloc();
						audio_index = i;
						pAudioCtx = ctx;
						tbAudio.den = FCtxRead.get()->streams[i]->time_base.den;
						tbAudio.num = FCtxRead.get()->streams[i]->time_base.num;
						if (pAudioCtx->sample_fmt != AV_SAMPLE_FMT_S16) {
							pAudioFrameS16 = WXMediaUtilsAllocAudioFrame(pAudioCtx->sample_fmt, nSampleRate, nChannel, nSampleRate);//48000
							pSwrCtx = WXMediaUtilsAllocSwrCtx(nSampleRate, nChannel, pAudioCtx->sample_fmt, nSampleRate, nChannel, AV_SAMPLE_FMT_S16);
						}
					}
				}
			}
			if (pAudioCtx == nullptr && pVideoCtx == nullptr) {
				WXLogW(L"不存在音视频数据");
				return WX_ERROR_ERROR;
			}

			//视频文件解码+编码
			if (mFfmpegMuxer == nullptr) {
				mFfmpegMuxer = std::make_shared< FfmpegMuxer>();
				mFfmpegMuxer->SetVideoConfig(width, height, iFps, 0, MODE_BEST, 0, 0, 0);
				mFfmpegMuxer->SetAudioConfig(nSampleRate, nChannel, nAudioBitrate);
				ret = mFfmpegMuxer->Open(strMp4.str(), TRUE);
				if (ret != WX_ERROR_SUCCESS) {
					WXLogW(L"Create FfmpegMuxer Error");
					mFfmpegMuxer = nullptr;
					g_bBreak = true;
				}
			}

			if (!g_bBreak) {
				AVPacket pkt;
				av_init_packet(&pkt);
				while (!g_bBreak && av_read_frame(FCtxRead.get(), &pkt) >= 0) {
					int got_picture = 0;

					if (pkt.stream_index == video_index && pVideoCtx) { //视频处理
						int ret = avcodec_decode_video2(pVideoCtx, pVideoFrame, &got_picture, &pkt);
						if (got_picture) {//时间戳换算为ms

							if(IsA(pVideoFrame))
							 {
								if (pVideoFrameDecode == nullptr)
									pVideoFrameDecode = WXMediaUtilsAllocVideoFrame((AVPixelFormat)pVideoFrame->format, width, height);

								int64_t ptsVideo = pVideoFrame->pts * 1000 * tbVideo.num / tbVideo.den;//换算为ms			
								av_frame_copy(pVideoFrameDecode, pVideoFrame);
								pVideoFrameDecode->pts = ptsVideo;
								mFfmpegMuxer->WriteVideoFrame(pVideoFrameDecode);
							}
						}
					}else if (pkt.stream_index == audio_index && pAudioCtx) {  //音频处理
						int ret = avcodec_decode_audio4(pAudioCtx, pAudioFrame, &got_picture, &pkt);
						if (got_picture) {//时间戳换算为ms
							int64_t ptsAudio = pAudioFrame->pts * 1000 * tbAudio.num / tbAudio.den;
							if (pSwrCtx) {
								swr_convert(pSwrCtx, pAudioFrameS16->data, pAudioFrame->nb_samples, (const uint8_t**)pAudioFrame->data, pAudioFrame->nb_samples);
								mFfmpegMuxer->WriteAudioFrame(pAudioFrameS16->data[0], pAudioFrame->nb_samples * 4);
							}
							else {
								mFfmpegMuxer->WriteAudioFrame(pAudioFrame->data[0], pAudioFrame->linesize[0]);
							}
						}
					}
					//释放资源
					av_packet_unref(&pkt);
				}
			}
			ptsCurrMax = FFMAX(ptsCurrVideo, ptsCurrAudio);
		}
	}

	SAFE_SWR_FREE(pSwrCtx);
	SAFE_AV_FREE(pAudioFrameS16);

	if (pVideoCtx) {
		avcodec_close(pVideoCtx);
		pVideoCtx = nullptr;
	}
	if (pAudioCtx) {
		avcodec_close(pAudioCtx);
		pAudioCtx = nullptr;
	}

	SAFE_AV_FREE(pVideoFrame)
	SAFE_AV_FREE(pVideoFrameDecode)

	if (g_bBreak) {  //合并文件,保存参数
		g_nRate = 0;//中断处理
		ret = FFMPEG_ERROR_BREADK;
	}
	else {
		g_nRate = 100;
		ret = FFMPEG_ERROR_OK;
	}

	if (mFfmpegMuxer.get()) {
		mFfmpegMuxer->Close();
		mFfmpegMuxer = nullptr;
	}

	g_bBreak = FALSE;
	return ret;
}

#endif