/*
图片水印
*/

#ifndef _WXImageWaterMark_H_
#define _WXImageWaterMark_H_

#include "WXMediaCpp.h"

class WXImageWaterMark {
public:
	WXVideoFrame m_RgbaFrame;
	WXVideoFrame m_TempFrame;
	int m_iWidth = 0;
	int m_iHeight = 0;
public:
	WXImageWaterMark() {}
	int Init(WXCTSTR strFileName) { //录屏时的水印图片
		Gdiplus::Bitmap bitmap(strFileName);
		m_iWidth  = bitmap.GetWidth();
		m_iHeight = bitmap.GetHeight();
        if (m_iWidth > 0 && m_iHeight > 0) {
            m_TempFrame.Init(AV_PIX_FMT_RGB32, m_iWidth, m_iHeight);
            m_RgbaFrame.Init(AV_PIX_FMT_RGB32, m_iWidth, m_iHeight);
            Gdiplus::BitmapData bmpData;
            Gdiplus::Rect       rect(0, 0, m_iWidth, m_iHeight);
            bitmap.LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bmpData);
            uint8_t* pSrc = (uint8_t*)bmpData.Scan0;
            libyuv::ARGBCopy((uint8_t*)bmpData.Scan0, bmpData.Stride,
                m_RgbaFrame.GetFrame()->data[0], m_RgbaFrame.GetFrame()->linesize[0],
                m_iWidth, m_iHeight);

            libyuv::ARGBAttenuate(m_RgbaFrame.GetFrame()->data[0],
                m_RgbaFrame.GetFrame()->linesize[0],
                m_RgbaFrame.GetFrame()->data[0],
                m_RgbaFrame.GetFrame()->linesize[0],
                m_RgbaFrame.GetFrame()->width,
                m_RgbaFrame.GetFrame()->height);

            bitmap.UnlockBits(&bmpData);
            return 1;
        }
        return 0;
	}

	void DxgiMask(AVFrame *frame, int nPosX, int nPosY) {
		int nPosX1 = nPosX + m_iWidth; //
		if (nPosX1 > frame->width)
			nPosX1 = frame->width;
		int deltaX = nPosX1 - nPosX;

		int nPosY1 = nPosY + m_iHeight;//
		if (nPosY1 > frame->height)
			nPosY1 = frame->height;
		int deltaY = nPosY1 - nPosY;

		libyuv::ARGBCopy(frame->data[0] + nPosX * 4 + nPosY * frame->linesize[0],
			frame->linesize[0],
			m_TempFrame.GetFrame()->data[0],
			m_TempFrame.GetFrame()->linesize[0],
			deltaX, deltaY);

		libyuv::ARGBBlend(
			m_RgbaFrame.GetFrame()->data[0], m_RgbaFrame.GetFrame()->linesize[0],
			m_TempFrame.GetFrame()->data[0], m_TempFrame.GetFrame()->linesize[0],
			frame->data[0] + nPosX * 4 + nPosY * frame->linesize[0], frame->linesize[0],
			deltaX, deltaY
		);
	}

	//起到一个类似遮罩功能
	WXImageWaterMark(WXCTSTR strFileName, int width, int height) {  //视频编辑的水印图片，自动缩放到指定大小
		if (strFileName && width && height) {
			WXLogW(L"%ws SIZE=%dx%d", __FUNCTIONW__, width,height);
			Gdiplus::Bitmap bitmap(strFileName);
			int dstW = bitmap.GetWidth();
			int dstH = bitmap.GetHeight();
			if (dstW && dstH) {
				m_iWidth = width;
				m_iHeight = height;

				m_RgbaFrame.Init(AV_PIX_FMT_RGB32, m_iWidth, m_iHeight);
				m_TempFrame.Init(AV_PIX_FMT_RGB32, m_iWidth, m_iHeight);
				Gdiplus::BitmapData bmpData;
				Gdiplus::Rect       rect(0, 0, dstW, dstH);
				bitmap.LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bmpData);
				uint8_t *pSrc = (uint8_t *)bmpData.Scan0;
				if (dstW == width && dstH == height) { //透明遮罩
					WXLogW(L"%ws [%ws] Create Bitmap ok [%dx%d]", __FUNCTIONW__, strFileName, dstW, dstH);
					libyuv::ARGBCopy((uint8_t *)bmpData.Scan0, bmpData.Stride,
						m_RgbaFrame.GetFrame()->data[0], m_RgbaFrame.GetFrame()->linesize[0],
						dstW, dstH);
				}
				else { //需要缩放
					WXLogW(L"%ws [%ws] Create Bitmap ok [%dx%d]--->[%dx%d]",
						__FUNCTIONW__, strFileName, dstW, dstH, width, height);
					libyuv::ARGBScale((uint8_t *)bmpData.Scan0, bmpData.Stride,
						dstW, dstH,
						m_RgbaFrame.GetFrame()->data[0], m_RgbaFrame.GetFrame()->linesize[0],
						m_iWidth, m_iHeight,
						libyuv::FilterMode::kFilterBilinear);
				}
				bitmap.UnlockBits(&bmpData);
			}
		}
	}

	void MaskPicture(AVFrame *frame) {
		if (m_iWidth == 0 || m_iHeight == 0)return;
		if (frame->format == AV_PIX_FMT_RGB32) {
			RgbaData::DrawPicture(frame, m_RgbaFrame.GetFrame());
		}else if (frame->format == AV_PIX_FMT_YUV420P) {
			libyuv::I420ToARGB(frame->data[0], frame->linesize[0],
				frame->data[1], frame->linesize[1],
				frame->data[2], frame->linesize[2],
				m_TempFrame.GetFrame()->data[0], m_TempFrame.GetFrame()->linesize[0],
				frame->width, frame->height);

			RgbaData::DrawPicture(m_TempFrame.GetFrame(), m_RgbaFrame.GetFrame());

			libyuv::ARGBToI420(m_TempFrame.GetFrame()->data[0], m_TempFrame.GetFrame()->linesize[0],
				frame->data[0], frame->linesize[0],
				frame->data[1], frame->linesize[1],
				frame->data[2], frame->linesize[2],
				frame->width, frame->height);
		}
		else if (frame->format == AV_PIX_FMT_YUVJ420P) {
			libyuv::J420ToARGB(frame->data[0], frame->linesize[0],
				frame->data[1], frame->linesize[1],
				frame->data[2], frame->linesize[2],
				m_TempFrame.GetFrame()->data[0], m_TempFrame.GetFrame()->linesize[0],
				frame->width, frame->height);

			RgbaData::DrawPicture(m_TempFrame.GetFrame(), m_RgbaFrame.GetFrame());

			libyuv::ARGBToJ420(m_TempFrame.GetFrame()->data[0], m_TempFrame.GetFrame()->linesize[0],
				frame->data[0], frame->linesize[0],
				frame->data[1], frame->linesize[1],
				frame->data[2], frame->linesize[2],
				frame->width, frame->height);
		}
	}

};

#endif
