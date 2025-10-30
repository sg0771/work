#include <MediaLibAPI.h>
#include "ML_D3DRender.h"
#include "ML_DirectRender.h"
#include <SDL2/SDL.h>
extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/frame.h>
}


EXTERN_C HRESULT SetPreviewSize(int width, int height)
{
	return DirectRender::Instance().SetPreviewSize(width, height);
}
EXTERN_C HRESULT InitDevice(BOOL mode)
{
	return DirectRender::Instance().InitDevice(mode);
}


EXTERN_C void* GetFrontSurface()
{
	return DirectRender::Instance().GetFrontSurface();
}

EXTERN_C HRESULT ResetFilterParamenter()
{
	return DirectRender::Instance().ResetFilterParamenter();
}
EXTERN_C HRESULT RenderFrame(AVFrame* frame, bool paused) {
	if (frame == nullptr || frame->data[0] == nullptr)
		return S_OK;
	return DirectRender::Instance().RenderFrame(frame, paused);
}

EXTERN_C HRESULT ForceRefresh() {

	return DirectRender::Instance().ForceRefresh();
}

EXTERN_C HRESULT SetPreviewAss(char* path)
{
	return DirectRender::Instance().SetPreviewAss(path);
}
HRESULT SetTrackAss(std::vector<AssContext> asscontexts)
{
	return DirectRender::Instance().SetTrackAss(asscontexts);
}
EXTERN_C HRESULT SetCacheAss(char* content)
{
	return DirectRender::Instance().SetCacheAss(content);
}
EXTERN_C HRESULT SetDeLogoParameter(DeLogoContext context)
{
	return DirectRender::Instance().SetDeLogoParameter(context);
}
EXTERN_C HRESULT SetChromaParamenter(ChromaKeyContext context)
{
	return DirectRender::Instance().SetChromaParamenter(context);
}
EXTERN_C RECT GetAssEventSize(int width, int height, float ts , char* name )
{
	return DirectRender::Instance().GetAssEventSize(width, height, ts, name);
}

EXTERN_C RECT GetAssPreviewSize(int width, int height, float ts, char* name, char* content)
{
	return DirectRender::Instance().GetAssPreviewSize(width, height, ts,name, content);
}


EXTERN_C RECT GetAssCursorRect(int width, int height, float ts, int index, int cursorindex)

{
	return DirectRender::Instance().GetAssCursorRect(width, height, ts, index, cursorindex);
}

EXTERN_C HRESULT SetColorAdjustParamenter(ColorAdjustContext context)
{
	return DirectRender::Instance().SetColorAdjustParamenter(context);
}
EXTERN_C HRESULT SetImageEnhanceParamenter(ImageEnhanceContext context)
{
	return DirectRender::Instance().SetImageEnhanceParamenter(context);
}
EXTERN_C HRESULT SetParameter(float Saturation, float Hue, float Brightness, float Contrast, float Hightlights, float Shadows)
{
	return DirectRender::Instance().SetColorAdjustParamenter(
		{Hue, Saturation, Brightness, Contrast, Hightlights, Shadows});
}
EXTERN_C HRESULT SetPreviewParameter(PreviewContext context)
{
	return DirectRender::Instance().SetPreviewParameter(context);
}