#pragma once

// TODO:  在此处引用程序需要的其他头文件
#include  <Windows.h>
#include "d3d9.h"
#include <d3dx9/d3dx9.h>

#include <stdbool.h>
#include <MediaLibAPI.h>

struct AVFrame;
MEDIALIB_API  HRESULT RenderFrame(AVFrame* frame, bool paused);

MEDIALIB_API  HRESULT ForceRefresh();
MEDIALIB_API  void* GetFrontSurface();
MEDIALIB_API  HRESULT ResetFilterParamenter();
MEDIALIB_API  HRESULT SetChromaParamenter(ChromaKeyContext context);
MEDIALIB_API  HRESULT SetColorAdjustParamenter(ColorAdjustContext context);
MEDIALIB_API  HRESULT SetImageEnhanceParamenter(ImageEnhanceContext context);
MEDIALIB_API  HRESULT SetParameter(float Saturation, float Hue, float Brightness, float Contrast, float Hightlights, float Shadows);
MEDIALIB_API  HRESULT SetPreviewParameter(PreviewContext context);
MEDIALIB_API  HRESULT SetPreviewAss(char* context);
MEDIALIB_API  HRESULT SetCacheAss(char* context);

MEDIALIB_API  HRESULT SetDeLogoParameter(DeLogoContext context);
MEDIALIB_API  RECT GetAssPreviewSize(int width, int height, float ts, char* name, char* content);
MEDIALIB_API  RECT GetAssEventSize(int framewidth, int frameheight, float ts, char* name);

MEDIALIB_API  RECT GetAssCursorRect(int width, int height, float ts, int index, int cursorindex);

MEDIALIB_API  HRESULT InitDevice(BOOL mode);