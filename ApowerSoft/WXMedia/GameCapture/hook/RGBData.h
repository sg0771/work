/*
RGBA数据格式转换
*/

#ifndef  _RGB_DATA_API_H_
#define  _RGB_DATA_API_H_

#include <stdint.h>

#define WX_RGB32  0
#define WX_RGBA  0
#define WX_BRGA  1 //....
#define WX_R10G10B10A2  2 //....
#define WX_RGB565 10
#define WX_RGB555 11

bool HookCheckText(int format);
void HookDrawString(uint8_t*pDst, int width, int height, int Pitch, int flip, int format);

bool HookCheckImage(int format);
void HookDrawImage(uint8_t*pDst, int width, int height, int Pitch, int flip, int format);

bool HookCheckCamera(int format);
void HookDrawCamera(uint8_t*pDst, int width, int height, int Pitch, int flip, int format);

void WXCountFps();//计算FPS

#endif