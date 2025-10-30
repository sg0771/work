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

	//���ڵײ����  ���ù�
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

	//����ˮӡȥ��������avframe���ݣ����ǲ���������ؿ�,dst��src��ַ��ͬ�����ÿ�������ͬ��Ҫ����������ͬ�ֱ���
	//dst��Ŀ��֡data[]
	//dst_linesize��Ŀ��֡linesize[]
	//src��Դ֡data[]
	//src_linesize��Դ֡linesize[]
	//w:ͼ���
	//h:ͼ���
	//pix_fmt:ͼ����ɫ�ռ䣬�Ƚ�����
	//logo_x:ˮӡx����
	//logo_y:ˮӡy����
	//logo_w:ˮӡ��
	//logo_h:ˮӡ��
	//band:ˮӡ������
	//logo_h:�Ƿ���ʾȥˮӡ��
	WXDELOGO_CAPI int WXDelogo(uint8_t** dst, int* dst_linesize,
		uint8_t** src, int* src_linesize,
		int w, int h, enum WXPixelFMT pix_fmt,
		int logo_x, int logo_y, int logo_w, int logo_h,
		int band, int show);

	//���ˮӡȥ��������avframe���ݣ����ǲ���������ؿ�
	//dst��Ŀ��֡data[]
	//dst_linesize��Ŀ��֡linesize[]
	//src��Դ֡data[]
	//src_linesize��Դ֡linesize[]
	//w:ͼ���
	//h:ͼ���
	//pix_fmt:ͼ����ɫ�ռ䣬�Ƚ�����
	//logo_rectangles:ˮӡ����
	//nb_rects:ˮӡ����
	//band:ˮӡ������
	//logo_h:�Ƿ���ʾȥˮӡ��
	WXDELOGO_CAPI int WXDelogos(uint8_t** dst, int* dst_linesize,
		uint8_t** src, int* src_linesize,
		int w, int h, enum WXPixelFMT pix_fmt,
		int *logo_rectangles[NUM_DATA_RECTANGLE], int nb_rects,
		int band, int show);

	//rgb����ȥ����ˮӡ������������һ��
	//dst��Ŀ��buf
	//src��Դbuf
	//width:ͼ���
	//height:ͼ���
	//logo_rectangles:ˮӡ����
	//nb_rects:ˮӡ����
	WXDELOGO_CAPI int WXDelogosRGB(uint8_t *dst, uint8_t *src, int width, int height,
		int *logo_rectangles[NUM_DATA_RECTANGLE], int nb_rects);

	//rgba����ȥ����ˮӡ������������һ��
	//dst��Ŀ��buf
	//src��Դbuf
	//width:ͼ���
	//height:ͼ���
	//logo_rectangles:ˮӡ����
	//nb_rects:ˮӡ����
	WXDELOGO_CAPI int WXDelogosRGBA(uint8_t *dst, uint8_t *src, int width, int height,
		int *logo_rectangles[NUM_DATA_RECTANGLE], int nb_rects);

//ʹ��libyuv��������  ����LibYUVProg.dll
#define USE_LIBYUV 0
#if USE_LIBYUV 
	//yuv����ȥ����ˮӡ����������ֱ��ʿ��Բ�һ��
	//src��Դbuf
	//in_width:Դͼ���
	//in_height:Դͼ���
	//dst��Ŀ��buf
	//out_width:Ŀ��ͼ���
	//out_height:Ŀ��ͼ���
	//logo_rectangles:ˮӡ����
	//nb_rects:ˮӡ����
	WXDELOGO_CAPI int WXDelogosYUV(uint8_t *src,int in_width, int in_height,
		uint8_t *dst, int out_width, int out_height,
		int(*logo_rectangles)[NUM_DATA_RECTANGLE], int nb_rects);

#endif

#ifdef __cplusplus
};
#endif

#endif /* _LIB_DELOGGO_API_H_ */
