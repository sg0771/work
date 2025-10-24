#ifndef _LIB_DELOGGO_API_H_
#define _LIB_DELOGGO_API_H_
#include <stdint.h>


#ifdef _WIN32

#include <Windows.h>
#include <tchar.h>
#pragma warning(disable : 4068)

#ifdef LIBDELOGO_EXPORTS
#define DLL_API  
#else
#define DLL_API  
#endif

#else //__APPLE__
#define DLL_API
#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif
#endif

#define WXDELOGO_CAPI EXTERN_C DLL_API
#ifdef __cplusplus
extern "C" {
#endif
#define NUM_DATA_POINTERS 8
#define NUM_DATA_RECTANGLE 4
#define NUM_MAX_RECTANGLE 20

	//用于底层测试  不用管
#define OUTINFO_1_PARAM(fmt, var) { CHAR sOut[256]; CHAR sfmt[100]; sprintf_s(sfmt, "%s%s", "INFO--", fmt); sprintf_s(sOut, (sfmt), var); OutputDebugStringA(sOut); }

	// Supported Pixel format.
	enum WXPixelFMT {
		WX_PIX_FMT_NONE = 0,
		WX_PIX_FMT_YUV420,   ///< planar YUV 4:2:0, 12bpp, YUV420P,NV12,NV21
		WX_PIX_FMT_GRAY8,     ///<        Y        ,  8bpp
		WX_PIX_FMT_RGB,      
		WX_PIX_FMT_RGBA,
		WX_PIX_FMT_NB
	};

	//单个水印去除，基于avframe数据，但是并不依赖相关库,dst与src地址相同，不用拷贝，不同需要拷贝，必须同分辨率
	//dst：目标帧data[]
	//dst_linesize：目标帧linesize[]
	//src：源帧data[]
	//src_linesize：源帧linesize[]
	//w:图像宽
	//h:图像高
	//pix_fmt:图像颜色空间，比较有限
	//logo_x:水印x坐标
	//logo_y:水印y坐标
	//logo_w:水印宽
	//logo_h:水印高
	//band:水印外扩宽
	//logo_h:是否显示去水印框
	WXDELOGO_CAPI int WXDelogo(uint8_t** dst, int* dst_linesize,
		uint8_t** src, int* src_linesize,
		int w, int h, enum WXPixelFMT pix_fmt,
		int logo_x, int logo_y, int logo_w, int logo_h,
		int band, int show);

	//多个水印去除，基于avframe数据，但是并不依赖相关库
	//dst：目标帧data[]
	//dst_linesize：目标帧linesize[]
	//src：源帧data[]
	//src_linesize：源帧linesize[]
	//w:图像宽
	//h:图像高
	//pix_fmt:图像颜色空间，比较有限
	//logo_rectangles:水印数组
	//nb_rects:水印个数
	//band:水印外扩宽
	//logo_h:是否显示去水印框
	WXDELOGO_CAPI int WXDelogos(uint8_t** dst, int* dst_linesize,
		uint8_t** src, int* src_linesize,
		int w, int h, enum WXPixelFMT pix_fmt,
		int *logo_rectangles[NUM_DATA_RECTANGLE], int nb_rects,
		int band, int show);

	//rgb数据去除多水印，输入输出宽高一致
	//dst：目标buf
	//src：源buf
	//width:图像宽
	//height:图像高
	//logo_rectangles:水印数组
	//nb_rects:水印个数
	WXDELOGO_CAPI int WXDelogosRGB(uint8_t *dst, uint8_t *src, int width, int height,
		int *logo_rectangles[NUM_DATA_RECTANGLE], int nb_rects);

	//rgba数据去除多水印，输入输出宽高一致
	//dst：目标buf
	//src：源buf
	//width:图像宽
	//height:图像高
	//logo_rectangles:水印数组
	//nb_rects:水印个数
	WXDELOGO_CAPI int WXDelogosRGBA(uint8_t *dst, uint8_t *src, int width, int height,
		int *logo_rectangles[NUM_DATA_RECTANGLE], int nb_rects);

//使用libyuv用于缩放  依赖LibYUVProg.dll
#define USE_LIBYUV 0
#if USE_LIBYUV 
	//yuv数据去除多水印，输入输出分辨率可以不一致
	//src：源buf
	//in_width:源图像宽
	//in_height:源图像高
	//dst：目标buf
	//out_width:目标图像宽
	//out_height:目标图像高
	//logo_rectangles:水印数组
	//nb_rects:水印个数
	WXDELOGO_CAPI int WXDelogosYUV(uint8_t *src,int in_width, int in_height,
		uint8_t *dst, int out_width, int out_height,
		int(*logo_rectangles)[NUM_DATA_RECTANGLE], int nb_rects);

#endif

#ifdef __cplusplus
};
#endif

#endif /* _LIB_DELOGGO_API_H_ */
