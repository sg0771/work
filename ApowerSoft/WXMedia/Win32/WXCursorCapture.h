/*
鼠标图像处理
*/
#ifndef _WXCursorCapture_H_
#define _WXCursorCapture_H_

#include "WXMediaCpp.h"
#include <cmath>

class WXCursorCapture {
public:
	HCURSOR                        current_cursor;//当前光标

	bool                           m_visible;//是否可见
	POINT                          cursor_pos;//当前位置

	WXVideoFrame m_dataFrame;//鼠标数据

	int m_nType = DXGI_OUTDUPL_POINTER_SHAPE_TYPE_COLOR;//鼠标类型
	int m_nW = 0;
	int m_nH = 0;
	WXDataBuffer m_dxgiMouseBuf;//DXGI 鼠标

	static inline uint8_t bit_to_alpha(uint8_t *data, long pixel, bool invert){
		uint8_t pix_byte = data[pixel / 8];
		bool alpha = (pix_byte >> (7 - pixel % 8) & 1) != 0;
		if (invert) {
			return alpha ? 0xFF : 0;
		}else {
			return alpha ? 0 : 0xFF;
		}
	}

	static inline bool bitmap_has_alpha(uint8_t *data, long num_pixels){
		for (long i = 0; i < num_pixels; i++) {
			if (data[i * 4 + 3] != 0) {
				return true;
			}
		}
		return false;
	}

	static inline void apply_mask(uint8_t *color, uint8_t *mask, long num_pixels){
		for (long i = 0; i < num_pixels; i++)
			color[i * 4 + 3] = bit_to_alpha(mask, i, false);
	}

	static uint8_t *get_bitmap_data(HBITMAP hbmp, BITMAP *bmp){
		if (GetObject(hbmp, sizeof(*bmp), bmp) != 0) {
			uint8_t *output;
			unsigned int size =
				(bmp->bmHeight * bmp->bmWidth * bmp->bmBitsPixel) / 8;

			output = (uint8_t *)malloc(size);
			::GetBitmapBits(hbmp, size, output);
			return output;
		}

		return NULL;
	}

	static inline uint8_t *copy_from_color(ICONINFO *ii, uint32_t *width,uint32_t *height){
		BITMAP bmp_color;
		BITMAP bmp_mask;
		uint8_t *color;
		uint8_t *mask;

		color = get_bitmap_data(ii->hbmColor, &bmp_color);
		if (!color) {
			return NULL;
		}

		if (bmp_color.bmBitsPixel < 32) {
			free(color);
			return NULL;
		}

		mask = get_bitmap_data(ii->hbmMask, &bmp_mask);
		if (mask) {
			long pixels = bmp_color.bmHeight * bmp_color.bmWidth;

			if (!bitmap_has_alpha(color, pixels))
				apply_mask(color, mask, pixels);

			free(mask);
		}

		*width = bmp_color.bmWidth;
		*height = bmp_color.bmHeight;
		return color;
	}

	static inline uint8_t *copy_from_mask(ICONINFO *ii, uint32_t *width,uint32_t *height){
		uint8_t *output;
		uint8_t *mask;
		long pixels;
		long bottom;
		BITMAP bmp;

		mask = get_bitmap_data(ii->hbmMask, &bmp);
		if (!mask) {
			return NULL;
		}

		bmp.bmHeight /= 2;

		pixels = bmp.bmHeight * bmp.bmWidth;
		output = (uint8_t*)malloc(pixels * 4);

		bottom = bmp.bmWidthBytes * bmp.bmHeight;

		for (long i = 0; i < pixels; i++) {
			uint8_t alpha = bit_to_alpha(mask, i, false);
			uint8_t color = bit_to_alpha(mask + bottom, i, true);

			if (!alpha) {
				output[i * 4 + 3] = color;
			}
			else {
				*(uint32_t*)&output[i * 4] = !!color ?
					0xFFFFFFFF : 0xFF000000;
			}
		}

		free(mask);

		*width = bmp.bmWidth;
		*height = bmp.bmHeight;
		return output;
	}

	static inline uint8_t *cursor_capture_icon_bitmap(ICONINFO *ii,uint32_t *width, uint32_t *height){
		uint8_t *output;

		output = copy_from_color(ii, width, height);
		if (!output)
			output = copy_from_mask(ii, width, height);

		return output;
	}

	inline bool cursor_capture_icon(HICON icon) {
		uint8_t *bitmap;
		uint32_t height;
		uint32_t width;
		ICONINFO ii;

		if (!icon) {
			return false;
		}
		if (!GetIconInfo(icon, &ii)) {
			return false;
		}

		bitmap = cursor_capture_icon_bitmap(&ii, &width, &height);
		if (bitmap) {
			if (m_dataFrame.GetWidth() != width || m_dataFrame.GetHeight() != height) {
				m_dataFrame.Init(AV_PIX_FMT_RGB32, width, height);
			}
			memcpy(m_dataFrame.GetFrame()->data[0], bitmap, width*height * 4);

			free(bitmap);
		}
		DeleteObject(ii.hbmColor);
		DeleteObject(ii.hbmMask);
		return true;
	}
public:
	void Capture() {
		CURSORINFO ci = { 0 };
		ci.cbSize = sizeof(ci);

		if (!GetCursorInfo(&ci)) {
			m_visible = false;
			return;
		}

		memcpy(&cursor_pos, &ci.ptScreenPos, sizeof(cursor_pos));
		if (current_cursor == ci.hCursor) {//图标不变
			return;
		}
		if (ci.hCursor == nullptr)
			return;
		HICON icon = ::CopyIcon(ci.hCursor);
		m_visible = cursor_capture_icon(icon);
		current_cursor = ci.hCursor;
		if ((ci.flags & CURSOR_SHOWING) == 0)
			m_visible = false;
		DestroyIcon(icon);
	}

	void Draw(AVFrame* MixFrame, const int PosX, const int PosY) {
		if (m_visible) {
			if (m_nType == DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MONOCHROME) {
				
				uint8_t* pAndMask = m_dxgiMouseBuf.GetBuffer();
				uint8_t* pXorMask = m_dxgiMouseBuf.GetBuffer() + m_nW*m_nH;
				RgbaData::DrawMouseMonoMask(MixFrame, PosX, PosY, pAndMask, pXorMask, m_nW, m_nH);

			}
			else if(m_nType == DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MASKED_COLOR) {
				int* pMask = (int*)m_dxgiMouseBuf.GetBuffer();
				RgbaData::DrawMouseMask(MixFrame, PosX, PosY, pMask, m_nW, m_nH);
			}
			else {
				//RGBA
				RgbaData::DrawMouseRGBA(MixFrame, PosX, PosY, m_dataFrame.GetFrame());
			}
		}

	}
};

#include "WXMediaCpp.h"
#ifdef _WIN32


//文字水印
class WXTextWaterMark {
public:
	TextWaterMarkParam m_param;
	WXVideoFrame m_RgbaFrame;
	int m_iAlpha = 100;
	void Mask(AVFrame *frame, int x, int y) {
		if (frame->format == AV_PIX_FMT_RGB32) {
			RgbaData::MixRect(frame, x, y, m_iAlpha, m_RgbaFrame.GetFrame());
		}
	}

	float smoothstep(float edge0, float edge1, float x) {
		// Scale, bias and saturate x to 0..1 range
		x = (x - edge0) / (edge1 - edge0) < 0 ? 0.0f : (x - edge0) / (edge1 - edge0) > 1 ? 1.0f : (x - edge0) / (edge1 - edge0);
		// Evaluate polynomial
		return x * x * (3 - 2 * x);
	}
	WXTextWaterMark(const TextWaterMarkParam *config) {
		memcpy(&m_param, config, sizeof(TextWaterMarkParam));
		int Length = (int)WXStrlen(m_param.m_wszText);

		if (Length) {
			//m_iDelay = m_param.m_iDelay;
			m_iAlpha = 100;
			HDC hdc = ::GetDC(nullptr);
			HDC hMemDC = ::CreateCompatibleDC(hdc);
			Gdiplus::FontFamily pFontFamily(m_param.m_wszFontName);
			Gdiplus::Font pFont(&pFontFamily, m_param.m_iFontSize, m_param.m_nStyle);
			LOGFONTW logfontW;
			Gdiplus::Graphics graphics(hMemDC);
			pFont.GetLogFontW(&graphics, &logfontW);

			HFONT hFont = ::CreateFontIndirectW(&logfontW);

			HFONT hFontOld = (HFONT)SelectObject(hMemDC, hFont); //选择字体

			SIZE size;
			GetTextExtentPoint32(hMemDC, m_param.m_wszText, Length, &size);
			int dstwidth = (size.cx + 5) / 2 * 2;
			int dstheight = (size.cy + 5) / 2 * 2;

			HBITMAP hBitmap = ::CreateCompatibleBitmap(hdc, dstwidth, dstheight);
			HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);
			SetBkColor(hMemDC, m_param.m_BkColor);//文字颜色
			SetTextColor(hMemDC, m_param.m_iColor);//文字颜色

			TextOut(hMemDC, 3, 3, m_param.m_wszText, Length);
			hBitmap = (HBITMAP)SelectObject(hMemDC, hOldBitmap);

			::SelectObject(hMemDC, hFontOld); //选择旧字体

			m_RgbaFrame.Init(AV_PIX_FMT_RGB32, dstwidth, dstheight);

			::GetDIBits(hMemDC, hBitmap, 0, dstheight, (LPSTR)m_RgbaFrame.GetFrame()->data[0], (BITMAPINFO*)m_RgbaFrame.GetBIH(), DIB_RGB_COLORS); //获取灰度数据

			::DeleteObject(hBitmap);
			::DeleteDC(hMemDC);
			::ReleaseDC(nullptr, hdc);

			::DeleteObject(hFont);

			BYTE r = m_param.m_iColor >> 16;
			BYTE g = (m_param.m_iColor &  0xff00) >> 8;
			BYTE b = m_param.m_iColor & 0xff;

			// 根据颜色填充透明度
			uint8_t * buf = m_RgbaFrame.GetFrame()->data[0];
			for (int j = 0; j < dstheight; j++) {
				for (int i = 0; i < dstwidth; i++) {
					int pos = j * dstwidth * 4 + i * 4;

					float dis = std::sqrt(std::pow(float(buf[pos]-r)/255.0,2) + 
						pow(float(buf[pos + 1] - g)/255.0, 2) + pow(float(buf[pos + 2] - b)/255.0, 2));
					
					buf[pos + 3] = (1 - smoothstep(0.05, 0.4, dis)) * 255;

					//if (dis < 30) {
					//	buf[pos + 3] = 0xff;/*(buf[pos] * 3 + buf[pos + 1] * 6 + buf[pos + 2]) / 10;*/
					//}
				}
			}
		}
	}
};

#endif

#endif
