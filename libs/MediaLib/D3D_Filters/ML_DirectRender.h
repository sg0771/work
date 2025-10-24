#pragma once
#include "./ML_d3dUtility.h"
#include "./ML_Texture.h"
#include "./ML_Shader.h"
#include "./ML_D3DFilter.h"
#include "./ML_ChromaKeyFilter.hpp"
#include "./ML_EmptyFilter.h"
#include "./ML_ColorAdjustFilter.hpp"
#include "./ML_ImageEnhanceFilter.h"
#include "./ML_lut.h"
#include "./ML_NoneFilter.hpp"
#include <libass\ass.h>
#include "./ML_AssSubtitle.h"
#include "./ML_DelogoFilter.hpp"

#define _r(c)  ((c)>>24)
#define _g(c)  (((c)>>16)&0xFF)
#define _b(c)  (((c)>>8)&0xFF)
#define _a(c)  ((c)&0xFF)

  struct FpsInfo {
	float  fps; //我们需要计算的FPS值
	int     frameCount;//帧数
	float  currentTime;//当前时间
	float  lastTime;//持续时间
};

  struct Vertex
  {
	  Vertex() {}
	  Vertex(
		  float x, float y, float z,
		  float nx, float ny, float nz,
		  float u, float v)
	  {
		  _x = x;  _y = y;  _z = z;
		  _nx = nx; _ny = ny; _nz = nz;
		  _u = u;  _v = v;
	  }
	  float _x, _y, _z;
	  float _nx, _ny, _nz;
	  float _u, _v;
	  static const DWORD FVF;

  };


class DirectRender
{
	int width;
	int height;

	IDirect3DDevice9Ex *Device;

	FpsInfo fps_info;
	D3DVIEWPORT9 viewport;
	int framewidth;
	int frameheight;

	int render_width;
	int render_height ;



	ID3DFilter* m_colorAdjustFilter;
	ID3DFilter* m_chromakeyFilter;
	ID3DFilter* m_emptyFilter;
	ID3DFilter* m_imageEnhanceFilter;
	ID3DFilter* m_lutFilter;
	IMulD3DFilter* m_ColorFilter;
	ID3DFilter* m_CurrentFilter;
	ID3DFilter* m_delogoFilter;
	

	
	bool needupdaterendersize = true;
	bool needRefresh = true;
	bool RealtimeRefresh = false;
	int RealtimeRefreshIndex = 0;

	int64_t lastpts;
	bool IsInvalid = false;
	ASS_Track* m_track  = NULL;
	std::vector<ASSTrackHandler> m_assTracks;//字幕轨道
	ASS_Track* m_cachetrack = NULL;

	WXLocker m_lckAss;
public:
	static DirectRender& Instance();
	int64_t GetLastPTS() { return lastpts; }
	HRESULT SetPreviewSize(int width, int height);
	HRESULT InitDevice(BOOL debugmode);
	
	HRESULT ReleaseDevice();
	HRESULT ResetFilterParamenter();
	HRESULT SetChromaParamenter(ChromaKeyContext context);
	HRESULT SetColorAdjustParamenter(ColorAdjustContext context);
	HRESULT SetImageEnhanceParamenter(ImageEnhanceContext context);
	HRESULT SetDeLogoParameter(DeLogoContext context);
	HRESULT SetPreviewParameter(PreviewContext context);
	HRESULT SetPreviewAss(std::string asscontent);
	HRESULT SetTrackAss(std::vector<AssContext> asscontexts);
	HRESULT SetCacheAss(std::string asscontent);
	RECT GetAssEventSize(int framewidth, int frameheight, float ts , std::string name);
	RECT GetAssPreviewSize(int width, int height, float ts, std::string name , std::string asscontent);
	RECT GetAssCursorRect(int framewidth, int frameheight, float ts, int index, int cursorindex);
	IDirect3DSurface9* GetFrontSurface();
	HRESULT RenderFrame(AVFrame* frame, bool paused);
	inline HRESULT ForceRefresh() {
		needRefresh = true;
		return S_OK;
	}
	HRESULT Setup();
	HRESULT CompileShader();
	
private:
	DirectRender();
	~DirectRender();
	inline const static uint8_t VertexShaderContent[164] = {
	0x00, 0x03, 0xFE, 0xFF, 0xFE, 0xFF, 0x14, 0x00, 0x43, 0x54, 0x41, 0x42, 0x1C, 0x00, 0x00, 0x00,
	0x23, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFE, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x01, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x76, 0x73, 0x5F, 0x33, 0x5F, 0x30, 0x00, 0x4D,
	0x69, 0x63, 0x72, 0x6F, 0x73, 0x6F, 0x66, 0x74, 0x20, 0x28, 0x52, 0x29, 0x20, 0x48, 0x4C, 0x53,
	0x4C, 0x20, 0x53, 0x68, 0x61, 0x64, 0x65, 0x72, 0x20, 0x43, 0x6F, 0x6D, 0x70, 0x69, 0x6C, 0x65,
	0x72, 0x20, 0x31, 0x30, 0x2E, 0x31, 0x00, 0xAB, 0x1F, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x80,
	0x00, 0x00, 0x0F, 0x90, 0x1F, 0x00, 0x00, 0x02, 0x05, 0x00, 0x00, 0x80, 0x01, 0x00, 0x0F, 0x90,
	0x1F, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x0F, 0xE0, 0x1F, 0x00, 0x00, 0x02,
	0x05, 0x00, 0x00, 0x80, 0x01, 0x00, 0x0F, 0xE0, 0x01, 0x00, 0x00, 0x02, 0x00, 0x00, 0x0F, 0xE0,
	0x00, 0x00, 0xE4, 0x90, 0x01, 0x00, 0x00, 0x02, 0x01, 0x00, 0x0F, 0xE0, 0x01, 0x00, 0xE4, 0x90,
	0xFF, 0xFF, 0x00, 0x00
	};
};
