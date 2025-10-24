/*
Hook到的游戏画面添加文字图片的操作
*/

#include "RGBData.h"
#include <Windows.h>
#include "graphics-hook.h"
#include "WXMediaCpp.h"
#include <gdiplus.h>

#pragma comment(lib,  "winmm.lib")

#define RGB888_RED      0x00ff0000
#define RGB888_GREEN    0x0000ff00
#define RGB888_BLUE     0x000000ff

struct DXGI_R10G10B10A2 {
	unsigned int r : 10;
	unsigned int g : 10;
	unsigned int b : 10;
	unsigned int a : 2;
};

struct Win_RGB32 {
	unsigned int b : 8;
	unsigned int g : 8;
	unsigned int r : 8;
	unsigned int a : 8;
};

static void R10G10B10A2_To_RGB32_Impl(DXGI_R10G10B10A2 src, Win_RGB32 &dst) {
	dst.b = src.b >> 2;
	dst.g = src.g >> 2;
	dst.r = src.r >> 2;
	dst.a = src.a << 6;
}

static void R10G10B10A2_To_RGB32(const uint8_t* pSrc, int srcPitch,
	uint8_t* pDst, int dstPitch,
	int cx, int cy, int flip
) {
	for (int h1 = 0; h1 < cy; h1++)
	{
		DXGI_R10G10B10A2 *src = (DXGI_R10G10B10A2*)(pSrc + h1 * srcPitch);
		Win_RGB32 *dst = flip ?
			(Win_RGB32*)(pDst + (cy - 1 - h1) * dstPitch) :
			(Win_RGB32*)(pDst + h1 * dstPitch);
		for (int w1 = 0; w1 < cx; w1++) {
			R10G10B10A2_To_RGB32_Impl(src[w1], dst[w1]);
		}
	}
}

static void RGB32_To_R10G10B10A2_Impl(Win_RGB32 src, DXGI_R10G10B10A2 &dst) {
	dst.b = src.b << 2;
	dst.g = src.g << 2;
	dst.r = src.r << 2;
	dst.a = 0;
}

static void RGB32_To_R10G10B10A2(const uint8_t* pSrc, int srcPitch,
	uint8_t* pDst, int dstPitch,
	int cx, int cy, int flip
) {
	for (int h1 = 0; h1 < cy; h1++)
	{
		Win_RGB32 *src = (Win_RGB32*)(pSrc + h1 * srcPitch);
		DXGI_R10G10B10A2 *dst = flip ?
			(DXGI_R10G10B10A2*)(pDst + (cy - 1 - h1) * dstPitch) :
			(DXGI_R10G10B10A2*)(pDst + h1 * dstPitch);
		for (int w1 = 0; w1 < cx; w1++) {
			RGB32_To_R10G10B10A2_Impl(src[w1], dst[w1]);
		}
	}
}

struct DXGI_R8G8B8A8 {
	unsigned int r : 8;
	unsigned int g : 8;
	unsigned int b : 8;
	unsigned int a : 8;
};

static void R8G8B8A8_To_R8G8B8A8_Impl(DXGI_R8G8B8A8 src, Win_RGB32 &dst) {
	dst.b = src.b;
	dst.g = src.g;
	dst.r = src.r;
	dst.a = src.a;
}
static void R8G8B8A8_To_RGB32(const uint8_t* pSrc, int srcPitch,
	uint8_t* pDst, int dstPitch,
	int cx, int cy, int flip
) {
	for (int h1 = 0; h1 < cy; h1++)
	{
		DXGI_R8G8B8A8 *src = (DXGI_R8G8B8A8*)(pSrc + h1 * srcPitch);
		Win_RGB32 *dst = flip ?
			(Win_RGB32*)(pDst + (cy - 1 - h1) * dstPitch) :
			(Win_RGB32*)(pDst + h1 * dstPitch);
		for (int w1 = 0; w1 < cx; w1++) {
			R8G8B8A8_To_R8G8B8A8_Impl(src[w1], dst[w1]);
		}
	}
}

static void RGB32Copy(const uint8_t* pSrc, int srcPitch,
	uint8_t* pDst, int dstPitch,
	int cx, int cy, int flip) {
	for (int i = 0; i < cy; i++) {
		memcpy(pDst + i * dstPitch,
			flip ?
			pSrc + (cy - 1 - i) * srcPitch :
			pSrc + i * srcPitch, cx * 4);

	}
}

static  uint16_t RGB32ToRGB565(uint32_t n888Color)
{
	// 获取RGB单色，并截取高位
	uint8_t cRed = (n888Color & RGB888_RED) >> 19;
	uint8_t cGreen = (n888Color & RGB888_GREEN) >> 10;
	uint8_t cBlue = (n888Color & RGB888_BLUE) >> 3;

	// 连接
	uint16_t n565Color = (cRed << 11) + (cGreen << 5) + (cBlue << 0);
	return n565Color;
}

static void ConvertRGB565(uint32_t *pSrc, uint16_t*pDst, int w, int h) {
	for (int i = 0; i < w * h; i++)
		pDst[i] = RGB32ToRGB565(pSrc[i]);
}

static  uint16_t RGB32ToRGB555(uint32_t n888Color)
{
	// 获取RGB单色，并截取高位
	uint8_t cRed = (n888Color & RGB888_RED) >> 19;
	uint8_t cGreen = (n888Color & RGB888_GREEN) >> 11;
	uint8_t cBlue = (n888Color & RGB888_BLUE) >> 3;

	// 连接
	uint16_t n565Color = (cRed << 10) + (cGreen << 5) + (cBlue << 0);
	return n565Color;
}

static void ConvertRGB555(uint32_t *pSrc, uint16_t*pDst, int w, int h) {
	for (int i = 0; i < w * h; i++)
		pDst[i] = RGB32ToRGB555(pSrc[i]);
}

void ConvertBGRA(uint8_t *pSrc, uint8_t*pDst, int w, int h) {
	for (int i = 0; i < w * h; i++) {
		pDst[i * 4 + 0] = pSrc[i * 4 + 2];
		pDst[i * 4 + 1] = pSrc[i * 4 + 1];
		pDst[i * 4 + 2] = pSrc[i * 4 + 0];
		pDst[i * 4 + 3] = pSrc[i * 4 + 3];
	}
}


class RGBData {
public:
	uint8_t *m_pBuf = nullptr;
	volatile
		int     m_iBufSize = 0;
	int     m_iWidth = 0;
	int     m_iHeight = 0;
	int     m_iType = WX_RGB32;

	HFONT m_font = NULL;
	wchar_t m_strFont[MAX_PATH];
	int  m_iFontSize = 0;
public:
	RGBData() {}
public:
	HFONT GetFont(wchar_t *strFont, int iFontSize) {
		if (m_iFontSize != iFontSize || wcscmp(m_strFont, strFont) != 0) {
			if (m_font) {
				DeleteObject(m_font);
				m_font = NULL;
			}
			wcscpy(m_strFont, strFont);
			m_iFontSize = iFontSize;
			m_font = CreateFont(
				iFontSize, //字体的逻辑高度
				iFontSize * 3 / 5, //逻辑平均字符宽度
				0, //与水平线的角度
				0, //基线方位角度
				FW_REGULAR, //字形：常规
				FALSE, //字形：斜体
				FALSE, //字形：下划线
				FALSE, //字形：粗体
				GB2312_CHARSET, //字符集
				OUT_DEFAULT_PRECIS, //输出精度
				CLIP_DEFAULT_PRECIS, //剪截精度
				PROOF_QUALITY, //输出品质
				FIXED_PITCH | FF_MODERN, //倾斜度
				strFont //字体
			);
		}
		return m_font;
	}

	uint8_t *GetBuffer() {
		return m_pBuf;
	}
	void Init(uint8_t *buf, int width, int height, int format) {
		if (m_pBuf)
			delete[]m_pBuf;
		m_iType = format;
		m_iBufSize = width * height * (m_iType > 5 ? 2 : 4);

		m_iWidth = width;
		m_iHeight = height;
		m_pBuf = new uint8_t[m_iBufSize];
		if (buf != nullptr) {
			if (m_iType == WX_RGB32)
				memcpy(m_pBuf, buf, m_iBufSize);
			else if (m_iType == WX_BRGA)      // RGB32 TO BGRA
				ConvertBGRA(buf, m_pBuf, width, height);
			else if (m_iType == WX_R10G10B10A2)      // RGB32 TO BGRA
				RGB32_To_R10G10B10A2(buf, width * 4, m_pBuf, width * 4, width, height, 0);
			else if (m_iType == WX_RGB565)      // RGB32 TO RGB565
				ConvertRGB565((uint32_t*)buf, (uint16_t*)m_pBuf, width, height);
			else if (m_iType == WX_RGB555)      // RGB32 TO RGB565
				ConvertRGB555((uint32_t*)buf, (uint16_t*)m_pBuf, width, height);
		}
		else {
			memset(m_pBuf, 0, m_iBufSize);
		}
	}
	void Copy(uint8_t *buf, int width, int height) {
		memcpy(m_pBuf, buf, width*height * 4);
	}

	virtual ~RGBData() {
		if (m_pBuf) {
			delete[]m_pBuf;
			m_pBuf = nullptr;
			m_iBufSize = 0;
		}
		if (m_font) {
			DeleteObject(m_font);
			m_font = NULL;
		}
	}

	//type = 1 左上角
	//type = 2 右上角
	//type = 3 左下角
	//type = 4 右下角
	void AlphaDraw(uint8_t *dstBuffer, int dstWidth, int dstHeight, int Pitch, int PosX, int PosY, int flip, int type) {
		int newPosX = 0;
		int newPosY = 0;
		switch (type)
		{
		case 1:
			newPosX = PosX;
			newPosY = PosY;
			break;
		case 2:	
			newPosX = dstWidth - PosX - m_iWidth;
			newPosY = PosY;
			break;
		case 3:
			newPosX = PosX;
			newPosY = dstHeight - PosY - m_iHeight;
			break;
		case 4:	
			newPosX = dstWidth - PosX - m_iWidth;
			newPosY = dstHeight - PosY - m_iHeight;
			break;
		default:
			return;
		}
		m_iType >= 10 ? RGB16_AlphaDraw(dstBuffer, dstWidth, dstHeight, Pitch, newPosX, newPosY, flip) :
			RGB32_AlphaDraw(dstBuffer, dstWidth, dstHeight, Pitch, newPosX, newPosY, flip);
	}

	void RGB16_AlphaDraw(uint8_t *dstBuffer, int width, int height, int Pitch, int PosX, int PosY, int flip) {
		if (PosX >= width || PosY >= height)return;
		if (PosX < 0 || PosY < 0 || m_pBuf == NULL)return;
		int dstWidth = std::min(width, PosX + m_iWidth);
		int dstHeight = std::min(height, PosY + m_iHeight);
		for (int i = PosY; i < dstHeight; i++) {
			for (int j = PosX; j < dstWidth; j++) {
				int posSrc = (i - PosY) * m_iWidth * 2 + (j - PosX) * 2;
				int posDst = flip ? (height - 1 - i) * Pitch + j * 2 : i * Pitch + j * 2;;
				if (m_pBuf[posSrc + 0] != 0 || m_pBuf[posSrc + 1] != 0) { //Alpha 不为0
					dstBuffer[posDst + 0] = m_pBuf[posSrc + 0];
					dstBuffer[posDst + 1] = m_pBuf[posSrc + 1];
				}
			}
		}
	}

	void RGB32_AlphaDraw(uint8_t *dstBuffer, int width, int height, int Pitch, int PosX, int PosY, int flip) {
		if (PosX >= width || PosY >= height)return;
		if (PosX < 0 || PosY < 0 || m_pBuf == NULL)return;
		int dstWidth = std::min(width, PosX + m_iWidth);
		int dstHeight = std::min(height, PosY + m_iHeight);
		for (int i = PosY; i < dstHeight; i++) {
			for (int j = PosX; j < dstWidth; j++) {
				int posSrc = (i - PosY) * m_iWidth * 4 + (j - PosX) * 4;
				int posDst = flip ? (height - 1 - i) * Pitch + j * 4 : i * Pitch + j * 4;
				int sum = m_pBuf[posSrc + 0] + m_pBuf[posSrc + 1] + m_pBuf[posSrc + 2] + m_pBuf[posSrc + 3];
				if (sum != 0) { //Alpha 不为0
					dstBuffer[posDst + 0] = m_pBuf[posSrc + 0];
					dstBuffer[posDst + 1] = m_pBuf[posSrc + 1];
					dstBuffer[posDst + 2] = m_pBuf[posSrc + 2];
					dstBuffer[posDst + 3] = 255;
				}
			}
		}
	}
};


#pragma comment(lib,  "gdiplus.lib")
static ULONG_PTR m_gdiplusToken = 0;

//----------------------------------- 文字水印 ------------------------------------------------
static RGBData g_TextBmp;
bool HookCheckText(int format/* = WX_RGB32*/) {
	if (global_hook_info && global_hook_info->m_bDrawText && global_hook_info->m_bChangeText) {
		global_hook_info->m_bChangeText = 0;

		wchar_t s_szText[MAX_PATH] = { 0 };
		wcscpy(s_szText, global_hook_info->HookMsg);

		HDC hdc = ::GetDC(nullptr);
		HDC hMemDC = ::CreateCompatibleDC(hdc);

		HFONT hFont = g_TextBmp.GetFont(global_hook_info->m_strFont, global_hook_info->m_iFontSize);

		HFONT hFontOld = (HFONT)SelectObject(hMemDC, hFont); //选择字体

		SIZE size;
		::GetTextExtentPoint32(hMemDC, s_szText, (int)wcslen(s_szText), &size);
		int dstwidth = (size.cx + 31) / 2 * 2;
		int dstheight = (size.cy + 11) / 2 * 2;

		HBITMAP hBitmap = ::CreateCompatibleBitmap(hdc, dstwidth, dstheight);
		HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);
		SetBkColor(hMemDC, 0x000000);//文字颜色
		SetTextColor(hMemDC, 0xFFFFFF);//文字颜色

		::TextOut(hMemDC, 15, 5, s_szText, (int)wcslen(s_szText));
		hBitmap = (HBITMAP)SelectObject(hMemDC, hOldBitmap);

		SelectObject(hMemDC, hFontOld); //选择旧字体

		RGBData tmp;
		tmp.Init(NULL, dstwidth, dstheight, WX_RGB32);

		BITMAPINFOHEADER pbi;
		memset(&pbi, 0, sizeof(BITMAPINFOHEADER));
		pbi.biSize = sizeof(BITMAPINFOHEADER);
		pbi.biBitCount = 32;
		pbi.biWidth = dstwidth;
		pbi.biHeight = -dstheight;
		pbi.biPlanes = 1;
		pbi.biSizeImage = dstwidth * dstheight * 4;
		GetDIBits(hMemDC, hBitmap, 0, dstheight, (LPSTR)tmp.GetBuffer(), (BITMAPINFO*)&pbi, DIB_RGB_COLORS); //获取灰度数据
		::DeleteObject(hBitmap);
		::DeleteDC(hMemDC);
		::ReleaseDC(nullptr, hdc);


		uint8_t Btex = GetBValue(global_hook_info->m_colorText);
		uint8_t Gtex = GetGValue(global_hook_info->m_colorText);
		uint8_t Rtex = GetRValue(global_hook_info->m_colorText);
		uint8_t Bbk = GetBValue(global_hook_info->m_colorBk);
		uint8_t Gbk = GetGValue(global_hook_info->m_colorBk);
		uint8_t Rbk = GetRValue(global_hook_info->m_colorBk);
		int* pInt = (int*)tmp.GetBuffer();
		uint8_t*buf = tmp.GetBuffer();
		for (int i = 0; i < dstwidth * dstheight; i++) {
			if (pInt[i] != 0) { //
				buf[i * 4 + 3] = buf[i * 4 + 0];//真实灰度
				buf[i * 4 + 0] = Btex;
				buf[i * 4 + 1] = Gtex;
				buf[i * 4 + 2] = Rtex; //上色
			}
			//else {
			//	buf[i * 4 + 3] = 128;//真实灰度
			//	buf[i * 4 + 0] = Bbk;
			//	buf[i * 4 + 1] = Gbk;
			//	buf[i * 4 + 2] = Rbk; //上色
			//}
		}
		g_TextBmp.Init(buf, dstwidth, dstheight, format);
	}
	if (global_hook_info && global_hook_info->m_bDrawText && g_TextBmp.GetBuffer() != nullptr) {//需要绘制
		return true;
	}
	return false;
}
void HookDrawString(uint8_t*pDst, int width, int height, int Pitch, int flip, int format) {
	if (HookCheckText(format)) {
		g_TextBmp.AlphaDraw(pDst, width, height, Pitch, global_hook_info->m_posxText, global_hook_info->m_posyText, flip, global_hook_info->m_bDrawText);
	}
}

//----------------------------------- 摄像头 ------------------------------------------------
static RGBData g_ImageCamera;;
bool HookCheckCamera(int format/* = WX_RGB32*/) {
	if (global_hook_info && global_hook_info->m_bDrawCamera && global_hook_info->m_bChangeCamera) {
		global_hook_info->m_bChangeCamera = 0;
		if (g_ImageCamera.m_iWidth != global_hook_info->m_nWidthCamera ||
			g_ImageCamera.m_iHeight != global_hook_info->m_nHeightCamera) {
			g_ImageCamera.Init(nullptr, global_hook_info->m_nWidthCamera, global_hook_info->m_nHeightCamera, WX_RGB32);
		}
		g_ImageCamera.Copy(global_hook_info->m_pCamera, global_hook_info->m_nWidthCamera, global_hook_info->m_nHeightCamera);
	}if (global_hook_info && global_hook_info->m_bDrawCamera && g_ImageCamera.GetBuffer() != nullptr) {//需要绘制
		return true;
	}
	return false;
}
void HookDrawCamera(uint8_t*pDst, int width, int height, int Pitch, int flip, int format) {
	if (HookCheckCamera(format)){
		g_ImageCamera.AlphaDraw(pDst, width, height, Pitch,
			global_hook_info->m_nPosXCamera,
			global_hook_info->m_nPosYCamera,
			flip, global_hook_info->m_bDrawCamera);
	}
}


//----------------------------------- 图像水印 ------------------------------------------------
static RGBData g_ImagBmp;;
bool HookCheckImage(int format/* = WX_RGB32*/) {
	if (global_hook_info && global_hook_info->m_bDrawImage && global_hook_info->m_bChangeImage) {
		global_hook_info->m_bChangeImage = 0;
		if (m_gdiplusToken == 0) {
			Gdiplus::GdiplusStartupInput StartupInput;//GDI+初始化
			Gdiplus::GdiplusStartup(&m_gdiplusToken, &StartupInput, NULL);
		}

		Gdiplus::Bitmap bmp(global_hook_info->m_HookImage);
		int dstwidth = bmp.GetWidth() / 2 * 2;
		int dstheight = bmp.GetHeight() / 2 * 2;
		if (dstwidth > 0 && dstheight > 0) {
			RGBData tmp;
			tmp.Init(NULL, dstwidth, dstheight, WX_RGB32);

			Gdiplus::BitmapData bmpData;
			Gdiplus::Rect       rect(0, 0, dstwidth, dstheight);
			bmp.LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppRGB, &bmpData);
			uint8_t *pSrc = (uint8_t *)bmpData.Scan0;
			for (int i = 0; i < dstheight; i++) {
				memcpy(tmp.GetBuffer() + i * dstwidth * 4, pSrc + i * bmpData.Stride, dstwidth * 4);
			}
			bmp.UnlockBits(&bmpData);

			uint8_t * buf = tmp.GetBuffer();
			for (int j = 0; j < dstheight; j++) {
				for (int i = 0; i < dstwidth; i++) {
					int pos = j * dstwidth * 4 + i * 4;
					buf[pos] = buf[pos + 0] * buf[pos + 3] / 255;
					buf[pos + 1] = buf[pos + 1] * buf[pos + 3] / 255;
					buf[pos + 2] = buf[pos + 2] * buf[pos + 3] / 255;
					//buf[pos + 3] = (buf[pos] + buf[pos + 1] + buf[pos + 2])/3;
				}
			}

			g_ImagBmp.Init(tmp.GetBuffer(), dstwidth, dstheight, format);//格式转换
		}
	}
	if (global_hook_info && global_hook_info->m_bDrawImage && g_ImagBmp.GetBuffer() != nullptr) {//需要绘制
		return true;
	}
	return false;
}
void HookDrawImage(uint8_t*pDst, int width, int height, int Pitch, int flip, int format) {
	if (HookCheckImage(format)) {
		g_ImagBmp.AlphaDraw(pDst, width, height, Pitch,
			global_hook_info->m_posxImage,
			global_hook_info->m_posyImage, flip, global_hook_info->m_bDrawImage);
	}
}

//----------------------------------- 计算FPS --------------------------------------------------
static int64_t s_nCountCapture = 0;//总帧数
static DWORD s_tsLast0 = 0;//最后一帧时间戳

static DWORD s_tsLast1 = 0;//最后一帧时间戳

void WXCountFps() {
	s_nCountCapture++;
	DWORD ts = ::timeGetTime();
	if (s_tsLast0 == 0) {
		s_tsLast0 = ts;
	}
	DWORD tsDelay = ts - s_tsLast0;
	if (tsDelay > 980) { //计算时间戳
		if (global_hook_info->m_modeFPS == 0) { //按秒计算FPS
			global_hook_info->m_nFps = (int)(s_nCountCapture * 1000 / tsDelay);//更新帧率
		}
		//清空设置
		s_tsLast0 = ts;
		s_nCountCapture = 0;
	}

	if (s_tsLast1 == 0) {
		s_tsLast1 = ts;
	}
	DWORD tsDelay1 = ts - s_tsLast1;
	if (tsDelay1 > 0) {
		if (global_hook_info->m_modeFPS == 1) {  //按帧刷新
			global_hook_info->m_nFps = (int)(1000 / tsDelay1);//更新帧率
		}
		s_tsLast1 = ts;
	}
}