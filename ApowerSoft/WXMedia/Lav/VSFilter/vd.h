

#pragma once

void VDCPUTest();
extern long CPUCheckForExtensions();
extern void VDFastMemcpyAutodetect();
extern long CPUEnableExtensions(long lEnableFlags);

bool BitBltFromI420ToI420(int w, int h, BYTE* dsty, BYTE* dstu, BYTE* dstv, int dstpitch, BYTE* srcy, BYTE* srcu, BYTE* srcv, int srcpitch);
bool BitBltFromI420ToNV12(int w, int h, BYTE* dsty, BYTE* dstu, BYTE* dstv, int dstpitch, BYTE* srcy, BYTE* srcu, BYTE* srcv, int srcpitch);
bool BitBltFromI420ToYUY2(int w, int h, BYTE* dst, int dstpitch, BYTE* srcy, BYTE* srcu, BYTE* srcv, int srcpitch);
bool BitBltFromI420ToYUY2Interlaced(int w, int h, BYTE* dst, int dstpitch, BYTE* srcy, BYTE* srcu, BYTE* srcv, int srcpitch);
bool BitBltFromI420ToRGB(int w, int h, BYTE* dst, int dstpitch, int dbpp, BYTE* srcy, BYTE* srcu, BYTE* srcv, int srcpitch /* TODO: , bool fInterlaced = false */);
bool BitBltFromYUY2ToYUY2(int w, int h, BYTE* dst, int dstpitch, BYTE* src, int srcpitch);
bool BitBltFromYUY2ToRGB(int w, int h, BYTE* dst, int dstpitch, int dbpp, BYTE* src, int srcpitch);
bool BitBltFromRGBToRGB(int w, int h, BYTE* dst, int dstpitch, int dbpp, BYTE* src, int srcpitch, int sbpp);

#ifndef _WIN64
void yuvtoyuy2row_MMX(BYTE* dst, BYTE* srcy, BYTE* srcu, BYTE* srcv, DWORD width);
void yuvtoyuy2row_avg_MMX(BYTE* dst, BYTE* srcy, BYTE* srcu, BYTE* srcv, DWORD width, DWORD pitchuv);
void yv12_yuy2_row_sse2();
void yv12_yuy2_row_sse2_linear();
void yv12_yuy2_row_sse2_linear_interlaced();
void yv12_yuy2_sse2(const BYTE* Y, const BYTE* U, const BYTE* V, int halfstride, unsigned halfwidth, unsigned height, BYTE* YUY2, int d_stride);
void yv12_yuy2_sse2_interlaced(const BYTE* Y, const BYTE* U, const BYTE* V, int halfstride, unsigned halfwidth, unsigned height, BYTE* YUY2, int d_stride);
#endif