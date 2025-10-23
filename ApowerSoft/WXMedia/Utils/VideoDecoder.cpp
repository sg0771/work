/*
H264新解码器,基于ffmpeg
*/

#include "WXMediaCpp.h"

class  WXVideoDecoder {
public:
	//硬解码专用
	enum AVPixelFormat m_hw_pix_fmt = AV_PIX_FMT_NONE;
	AVBufferRef* m_hw_device_ctx = NULL;
	AVFrame* m_pSwFrame = nullptr;//解码帧
	static enum AVPixelFormat get_hw_format(AVCodecContext* ctx, const enum AVPixelFormat* pix_fmts) {
		WXVideoDecoder* pThis = (WXVideoDecoder*)ctx->opaque;
		for (const enum AVPixelFormat* p = pix_fmts; *p != AV_PIX_FMT_NONE; p++) {
			if (*p == pThis->m_hw_pix_fmt)
				return *p;
		}
		return AV_PIX_FMT_NONE;
	}
private:
	WXLocker m_mutex;

	AVCodecContext* m_pCtx = nullptr;//解码器
	AVFrame* m_pFrame = nullptr;//解码帧

	int m_vidoe_format = AV_PIX_FMT_NONE;
	WXVideoFrame m_pVideoFrame[4];//YUV Video Image Data
	WXVideoFrame m_pBlackFrame[4];//Black Data
	int  m_iWidth = 0;
	int  m_iHeight = 0;
	WXDataBuffer m_extra;
	BOOL m_bHW = FALSE;
	BOOL m_bGetPic = FALSE;//解码图像

	WXVideoFrame   m_FreshFrame;//iPhone11 iOS 13.0 会因为几秒切换分辨率而闪烁！  828-1192--->888*1920
	WXVideoConvert m_conv;
	BOOL m_bH264 = TRUE;

	BOOL OpenImpl() {
		AVCodec* codec = avcodec_find_decoder(m_bH264?AV_CODEC_ID_H264: AV_CODEC_ID_H265);
		m_pCtx = avcodec_alloc_context3(codec);
		if (m_extra.GetBuffer() && m_extra.m_iBufSize) {
			int  w = 0;
			int  h = 0;
			int  profile = 0;
			VideoDecoderGetSize(m_bH264, m_extra.GetBuffer(), m_extra.m_iBufSize, &w, &h,&profile);
			if (w && h) {
				m_pCtx->width = w;
				m_pCtx->height = h;
				m_pCtx->coded_width = FFALIGN(w, 32);
				m_pCtx->coded_height = FFALIGN(h, 32);
				m_pCtx->profile = profile;
				m_pCtx->extradata = (uint8_t*)av_malloc(m_extra.m_iBufSize);
				memcpy(m_pCtx->extradata, m_extra.GetBuffer(), m_extra.m_iBufSize);
				m_pCtx->extradata_size = m_extra.m_iBufSize;
			}
		}

		BOOL bSupportHW = FALSE;
		if (m_bHW) {
			//"d3d11va"
			BOOL bGetHWFmt = FALSE;
			//DXVA2 最稳定 
			enum AVHWDeviceType typeHD = av_hwdevice_find_type_by_name("dxva2");//OK;
			if (typeHD != AV_HWDEVICE_TYPE_NONE) {
				for (int index = 0; ; index++) { // 查找解码输出格式
					const AVCodecHWConfig* config = avcodec_get_hw_config(codec, index);
					if (config
						&& (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX)
						&& config->device_type == typeHD) {
						WXLogW(L"Find HW Output!");
						m_hw_pix_fmt = config->pix_fmt;
						bGetHWFmt = TRUE;
						break;
					}
					else if (config == NULL) {
						WXLogW(L"Not Find HW config!");
						bGetHWFmt = FALSE;
						break;
					}
				}
				if (bGetHWFmt) {
					int err = av_hwdevice_ctx_create(&m_hw_device_ctx, typeHD, NULL, NULL, 0);
					if (err >= 0) {
						m_pCtx->opaque = this;
						m_pCtx->hw_device_ctx = av_buffer_ref(m_hw_device_ctx);
						m_pCtx->get_format = get_hw_format;
						bSupportHW = TRUE;
						WXLogW(L"Ffmpeg Hardware H2645 Decoder");
					}
				}
			}
		}

		if (!bSupportHW) {
			m_bHW = FALSE;
			WXLogW(L"Ffmpeg Software H2645 Decoder");
			m_pCtx->thread_count = std::thread::hardware_concurrency() / 2;//多线程解码
		}
		int ret = avcodec_open2(m_pCtx, codec, nullptr);
		if (ret >= 0) {
			m_pFrame = av_frame_alloc();
			if (m_bHW) {
				m_pSwFrame = av_frame_alloc();
			}
			WXLogW(L"---- %ws [%dx%d], HW=%d OK", __FUNCTIONW__, m_iWidth, m_iHeight, m_bHW);
			return TRUE;
		}
		avcodec_free_context(&m_pCtx);
		m_pCtx = nullptr;
		WXLogW(L"%ws Error", __FUNCTIONW__);
		return FALSE;
	}
public:
	virtual int  Open(BOOL bH264, uint8_t* pExtradata, int nExtraSize, int bHW) {
		WXLogW(L"%ws Start", __FUNCTIONW__);
		WXAutoLock al(m_mutex);
		m_extra.Init(pExtradata, nExtraSize);
		m_bHW = bHW;
		m_bH264 = bH264;
		return OpenImpl();
	}
	virtual int  GetWidth() {
		return m_iWidth;
	}
	virtual int  GetHeight() {
		return m_iHeight;
	}
	virtual void Close() {
		WXAutoLock al(m_mutex);
		//WXLogW(L"%ws", __FUNCTIONW__);
		BEGIN_LOG_FUNC;
		if (m_pCtx) {
			if (m_pCtx) {
				avcodec_close(m_pCtx);
				avcodec_free_context(&m_pCtx);
				m_pCtx = nullptr;
			}
		}
		SAFE_AV_FREE(m_pFrame)
		SAFE_AV_FREE(m_pSwFrame)
	}

	virtual int SendPacket(uint8_t* pBuf, int nSize, int64_t pts) {
		WXAutoLock al(m_mutex);

		if (m_pCtx) {
			m_bGetPic = FALSE;
			AVPacket pkt;
			av_init_packet(&pkt);
			pkt.data = pBuf;
			pkt.size = nSize;
			pkt.pts = pts;
			pkt.dts = pts;
			int ret = avcodec_send_packet(m_pCtx, &pkt);
			if (ret < 0) {
				WXLogW(L"%ws avcodec_send_packet Error %d", __FUNCTIONW__, ret);
				return m_bGetPic;
			}
			while (TRUE) {
				ret = avcodec_receive_frame(m_pCtx, m_pFrame);
				if (ret >= 0) {
					if (m_pFrame->width > 0 && m_pFrame->height > 0) {
						m_bGetPic = TRUE;
						AVFrame* dstFrame = m_pFrame;
						if (dstFrame->width / 4 * 4 != m_iWidth ||
							dstFrame->height / 4 * 4 != m_iHeight ||
							m_pVideoFrame[0].GetFrame() == nullptr) {

							if (m_bHW) {
								SAFE_AV_FREE(m_pSwFrame)
								m_pSwFrame = av_frame_alloc();
							}

							//分辨率切换
							m_iWidth = dstFrame->width / 4 * 4;
							m_iHeight = dstFrame->height / 4 * 4;

							if (m_vidoe_format == AV_PIX_FMT_NONE) {
								m_vidoe_format = AV_PIX_FMT_YUV420P;
								if (m_pFrame->color_range == AVCOL_RANGE_JPEG) {
									m_vidoe_format = AV_PIX_FMT_YUVJ420P;
								}
							}
							m_pVideoFrame[0].Init(m_vidoe_format, m_iWidth, m_iHeight);
							m_pBlackFrame[0].Init(m_vidoe_format, m_iWidth, m_iHeight);
							m_pVideoFrame[1].Init(m_vidoe_format, m_iHeight, m_iWidth);
							m_pBlackFrame[1].Init(m_vidoe_format, m_iHeight, m_iWidth);
							m_pVideoFrame[2].Init(m_vidoe_format, m_iWidth, m_iHeight);
							m_pBlackFrame[2].Init(m_vidoe_format, m_iWidth, m_iHeight);
							m_pVideoFrame[3].Init(m_vidoe_format, m_iHeight, m_iWidth);
							m_pBlackFrame[3].Init(m_vidoe_format, m_iHeight, m_iWidth);
						}
						if (m_bHW) { //显存数据拷贝到内存
							dstFrame = m_pSwFrame;
							int ret = av_hwframe_transfer_data(dstFrame, m_pFrame, 0);
							if (ret < 0)
								break;
						}


						//NV12
						if (dstFrame->format == AV_PIX_FMT_NV12) {
							libyuv::NV12ToI420(
								dstFrame->data[0], dstFrame->linesize[0],
								dstFrame->data[1], dstFrame->linesize[1],
								m_pVideoFrame[0].GetFrame()->data[0], m_pVideoFrame[0].GetFrame()->linesize[0],
								m_pVideoFrame[0].GetFrame()->data[1], m_pVideoFrame[0].GetFrame()->linesize[1],
								m_pVideoFrame[0].GetFrame()->data[2], m_pVideoFrame[0].GetFrame()->linesize[2],
								m_iWidth, m_iHeight
							);
						}
						else if (dstFrame->format == AV_PIX_FMT_YUV420P ||
							dstFrame->format == AV_PIX_FMT_YUVJ420P) {
							libyuv::I420Copy(
								dstFrame->data[0], dstFrame->linesize[0],
								dstFrame->data[1], dstFrame->linesize[1],
								dstFrame->data[2], dstFrame->linesize[2],
								m_pVideoFrame[0].GetFrame()->data[0], m_pVideoFrame[0].GetFrame()->linesize[0],
								m_pVideoFrame[0].GetFrame()->data[1], m_pVideoFrame[0].GetFrame()->linesize[1],
								m_pVideoFrame[0].GetFrame()->data[2], m_pVideoFrame[0].GetFrame()->linesize[2],
								m_iWidth, m_iHeight
							);
						}
					}
					continue;
				}
				else {
					break;
				}
			}
			return m_bGetPic;
		}
		return FALSE;
	}

	virtual AVFrame* GrabFrame(int type, int rotate, int _dstWidth, int _dstHeight) {
		WXAutoLock al(m_mutex);
		AVFrame* dstFrame = nullptr;
		if (m_pCtx) {
			//旋转处理
			if (type == TYPE_CURR_FRAME || type == TYPE_LAST_FRAME) {
				dstFrame = m_pVideoFrame[rotate].GetFrame();
				if (rotate != RENDER_ROTATE_NONE) {
					libyuv::I420Rotate(
						m_pVideoFrame[0].GetFrame()->data[0], m_pVideoFrame[0].GetFrame()->linesize[0],
						m_pVideoFrame[0].GetFrame()->data[1], m_pVideoFrame[0].GetFrame()->linesize[1],
						m_pVideoFrame[0].GetFrame()->data[2], m_pVideoFrame[0].GetFrame()->linesize[2],
						dstFrame->data[0], dstFrame->linesize[0],
						dstFrame->data[1], dstFrame->linesize[1],
						dstFrame->data[2], dstFrame->linesize[2],
						m_iWidth, m_iHeight,
						(libyuv::RotationMode)(rotate * 90)
					);
				}
			}
			else if (type == TYPE_BLACK_FRAME) {
				dstFrame = m_pBlackFrame[rotate].GetFrame();
			}
		}

		//表示使用默认分辨率
		if (_dstWidth == 0 || _dstHeight == 0) {
			//iPhone11 iOS 13 Patch!
			if (dstFrame != nullptr && dstFrame->width == 1792 && dstFrame->height == 828) {
				if (m_FreshFrame.GetFrame() == nullptr ||
					m_FreshFrame.GetWidth() != 1920 ||
					m_FreshFrame.GetHeight() != 1080) {
					m_FreshFrame.Init(dstFrame->format, 1920, 888);
					WXLogA("-------- Patch For iPhone11 iOS 13 [828x1792]--->[888x1920]");
				}
				AVFrame* srcFrame = dstFrame;
				dstFrame = m_FreshFrame.GetFrame();
				libyuv::I420Scale(
					srcFrame->data[0], srcFrame->linesize[0],
					srcFrame->data[1], srcFrame->linesize[1],
					srcFrame->data[2], srcFrame->linesize[2],
					srcFrame->width, srcFrame->height,
					dstFrame->data[0], dstFrame->linesize[0],
					dstFrame->data[1], dstFrame->linesize[1],
					dstFrame->data[2], dstFrame->linesize[2],
					dstFrame->width, dstFrame->height,
					libyuv::FilterMode::kFilterBilinear
				);
			}
		}
		else { //指定分辨率输出，可能变形
			if (m_FreshFrame.GetFrame() == nullptr ||
				m_FreshFrame.GetWidth() != _dstWidth ||
				m_FreshFrame.GetHeight() != _dstHeight) {
				m_FreshFrame.Init(dstFrame->format, _dstWidth, _dstHeight);
			}
			AVFrame* srcFrame = dstFrame;
			dstFrame = m_FreshFrame.GetFrame();
			libyuv::I420Scale(
				srcFrame->data[0], srcFrame->linesize[0],
				srcFrame->data[1], srcFrame->linesize[1],
				srcFrame->data[2], srcFrame->linesize[2],
				srcFrame->width, srcFrame->height,
				dstFrame->data[0], dstFrame->linesize[0],
				dstFrame->data[1], dstFrame->linesize[1],
				dstFrame->data[2], dstFrame->linesize[2],
				dstFrame->width, dstFrame->height,
				libyuv::FilterMode::kFilterBilinear
			);
		}

		if (dstFrame != NULL)
		{
			dstFrame->pkt_pts = m_pVideoFrame[0].GetFrame()->pkt_pts;
			dstFrame->pts = m_pVideoFrame[0].GetFrame()->pts;
		}
		return dstFrame;
	}
};

WXMEDIA_API void* WXVideoDecoderCreate(int bH264,int bHw, uint8_t* extradata, int extrasize) {
	BEGIN_LOG_FUNC
	WXVideoDecoder* dec = new WXVideoDecoder;
	BOOL bOpen = dec->Open(TRUE, extradata, extrasize, bHw);
	if (bOpen) {
		return dec;
	}
	if (bHw) { //硬解码无法打开切换软解
		bOpen = dec->Open(TRUE, extradata, extrasize, false);
		if (bOpen) {
			return dec;
		}
	}
	delete dec;
	return NULL;
}

WXMEDIA_API void  WXVideoDecoderDestroy(void* ptr) {
	if (ptr) {
		WXVideoDecoder* dec = (WXVideoDecoder*)ptr;
		dec->Close();
		delete dec;
	}
}

WXMEDIA_API int   WXVideoDecoderGetWidth(void* ptr) {
	if (ptr) {
		WXVideoDecoder* dec = (WXVideoDecoder*)ptr;
		return dec->GetWidth();
	}
	return 0;
}

WXMEDIA_API int   WXVideoDecoderGetHeight(void* ptr) {
	if (ptr) {
		WXVideoDecoder* dec = (WXVideoDecoder*)ptr;
		return dec->GetHeight();
	}
	return 0;
}

WXMEDIA_API int   WXVideoDecoderSendPacket(void* ptr, uint8_t* buf, int size, int64_t pts) {
	if (ptr) {
		WXVideoDecoder* dec = (WXVideoDecoder*)ptr;
		return dec->SendPacket(buf, size, pts);
	}
	return 0;
}

WXMEDIA_API struct AVFrame* WXVideoDecoderGetFrame(void* ptr, int type) {
	if (ptr) {
		WXVideoDecoder* dec = (WXVideoDecoder*)ptr;
		return dec->GrabFrame(type, RENDER_ROTATE_NONE, 0, 0);
	}
	return nullptr;
}

WXMEDIA_API struct AVFrame* WXVideoDecoderGetFrame2(void* ptr, int type, int rotate) {
	if (ptr) {
		WXVideoDecoder* dec = (WXVideoDecoder*)ptr;
		return dec->GrabFrame(type, rotate, 0, 0);
	}
	return nullptr;
}

WXMEDIA_API struct AVFrame* WXVideoDecoderGetFrameEx(void* ptr, int type, int rotate, int dstWdith, int dstHeight) {
	if (ptr) {
		WXVideoDecoder* dec = (WXVideoDecoder*)ptr;
		return dec->GrabFrame(type, rotate, dstWdith, dstHeight);
	}
	return nullptr;
}



