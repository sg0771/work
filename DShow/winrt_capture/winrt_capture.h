#ifndef _WINRT_CAPTURE_H_
#define _WINRT_CAPTURE_H_

#include <stdint.h>


class WgcCapture;
WgcCapture* wgc_capture_create(int nFps);
int    wgc_capture_width(WgcCapture* capture);
int    wgc_capture_height(WgcCapture* capture);
int    wgc_capture_start(WgcCapture* capture);
void   wgc_capture_stop(WgcCapture* capture);

#endif