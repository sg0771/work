//
#include "ML_d3dUtility.h"

#include <assert.h>
#include <timeapi.h>
#include <fstream>
#include <ffms2/ffms.h>
#include "ML_DirectRender.h"
#include <libyuv.h>
#include "../ML_stdafx.h"




#define CHECK_AND_DEL_PTR(p) \
	if(NULL != p){\
		delete p;\
		p = NULL;\
	}

//extern   ErrorLogCallBack errorlog_call_back;
ErrorLogCallBack ML_GetErrorLogCallBack();


static BYTE* blendass(BYTE * buffer, int pitch, int height, ass_image * subimage) {

	if (buffer == NULL) {
		return NULL;
	}
	int cnt = 0;

	auto baseimage_write = buffer;
	SIZE pos = {10000,10000};
	while (subimage) {
		if (subimage->w >= 65535 || subimage->h >= 65535 || subimage->w <= 0 || subimage->h <= 0)
			break;
		//灰度图还原成rgba格式
		int x, y;
		unsigned char opacity = 255 - _a(subimage->color);
		
		unsigned char r = _r(subimage->color);
		unsigned char g = _g(subimage->color);
		unsigned char b = _b(subimage->color);

		unsigned char* src = subimage->bitmap;
		unsigned char* dst;

		auto subargbdata = gen_image(subimage->w, subimage->h);
		if (subargbdata== NULL|| subargbdata->buffer== NULL)
		{
			break;
		}
		dst = subargbdata->buffer ;
		BYTE ks[256] = {};
		for (size_t i = 0; i < 256; i++)
		{
			ks[i] = uint8_t( i * opacity / 255);
		}

		for (y = 0; y < subimage->h; ++y) {
			for (x = 0; x < subimage->w; ++x) {

				if (src[x] == 0)
				{
					continue;
				}
				BYTE k = ks[src[x]];// ((unsigned)src[x])* opacity / 255;
				// possible endianness problems
				dst[x * 4] = b;
				dst[x * 4 + 1] = g;
				dst[x * 4 + 2] = r;
				dst[x * 4 + 3] = k;

			}
			src += subimage->stride;
			dst += subargbdata->stride;
		}

		// 渲染字符的时候， 将字幕框顺便绘制出来方便debug
		//绘制范围为 x0, x0 + 1, y0, y1


		if (subimage->type== 0)
		{
			pos.cx = FFMIN(pos.cx, subimage->dst_x);
			pos.cy = FFMIN(pos.cy, subimage->dst_y);
	
		}
	


		//rgba数据叠加到底图上
		int xdest = subimage->dst_x;


		//height - subimage->h -
		int ydest = subimage->dst_y;
		ydest = (ydest < 0) ? 0 : ydest;
		auto baseimage_read_clip = baseimage_write + 4 * xdest + ydest * pitch;

		auto baseimage_write_clip = baseimage_write + 4 * xdest + ydest * pitch;
		auto over = subargbdata->buffer;

		libyuv::ARGBAttenuate(over, subargbdata->stride, over, subargbdata->stride, subargbdata->width, subargbdata->height);
		libyuv::ARGBBlend(over, subargbdata->stride, baseimage_read_clip, pitch, baseimage_write_clip, pitch, subargbdata->width, subargbdata->height);

		++cnt;

		av_free(subargbdata->buffer);
		av_free(subargbdata);

		subimage = subimage->next;
		if (subimage == nullptr)
			break;
#ifdef _M_X64
		if ((uintptr_t)subimage == 0xdddddddddddddddd || subimage->w >= 65535 || subimage->h >= 65535 || subimage->w <= 0 || subimage->h <= 0)
			break;
#else
		if ((uintptr_t)subimage == 0xdddddddd || subimage->w >= 65535 || subimage->h >= 65535 || subimage->w <= 0 || subimage->h <= 0)
			break;
#endif
	}


	return buffer;
}

DirectRender& DirectRender::Instance()
{
	static DirectRender s_render;
	return s_render;
}
DirectRender::DirectRender()
{
	m_ColorFilter = NULL;
	m_track = NULL;
	//SetPreviewAss("C:\\src\\libass\\compare\\test\\image.ass");
}

DirectRender::~DirectRender()
{
	CHECK_AND_DEL_PTR(m_chromakeyFilter);
	CHECK_AND_DEL_PTR(m_emptyFilter);
	CHECK_AND_DEL_PTR(m_colorAdjustFilter);
	CHECK_AND_DEL_PTR(m_imageEnhanceFilter);
	CHECK_AND_DEL_PTR(m_ColorFilter);
	CHECK_AND_DEL_PTR(m_lutFilter);
	CHECK_AND_DEL_PTR(m_delogoFilter);
}

HRESULT DirectRender::CompileShader()
{
	ID3DFilter::Init(Device);

	m_emptyFilter = new EmptyFilter(Device);

	if(m_emptyFilter->IsInvalid())
	{
		delete m_emptyFilter;
		m_CurrentFilter = new NoneFilter(Device);
		return 0;
	}

	m_CurrentFilter = m_emptyFilter;
	m_colorAdjustFilter = new ColorAdjustFilter(Device);
	IsInvalid = m_colorAdjustFilter->IsInvalid();
	if (IsInvalid)
	{
		return 0;
	}
	m_chromakeyFilter = new ChromaKeyFilter(Device);
	m_imageEnhanceFilter = new ImageEnhanceFilter(Device);
	m_lutFilter				= new lutFilter(Device);
	m_delogoFilter = new DeLogoFilter(Device);

	m_ColorFilter = new IMulD3DFilter(Device);
	m_ColorFilter->AddFilter(m_imageEnhanceFilter);
	m_ColorFilter->AddFilter(m_colorAdjustFilter);
	m_ColorFilter->AddFilter(m_lutFilter);	
	return 0;
}

HRESULT DirectRender::ResetFilterParamenter()
{
	if (IsInvalid)
	{
		return 0;
	}
	

	needRefresh = true;
	m_CurrentFilter = m_emptyFilter;
	return S_OK;
}

HRESULT DirectRender::SetPreviewParameter(PreviewContext context)
{
	if (IsInvalid)
	{
		return 0;
	}
	//Log_info(__FUNCTION__);
	needRefresh = true;
	m_imageEnhanceFilter->SetParameter(&context.imageenhance);
	m_colorAdjustFilter->SetParameter(&context.coloradjust);

	dynamic_cast<lutFilter*>(m_lutFilter)->setLutData(context.lut.lutFilePath);

	m_CurrentFilter = m_ColorFilter;
	return S_OK;
}

HRESULT DirectRender::SetTrackAss(std::vector<AssContext> asscontexts)
{
	WXAutoLock al(m_lckAss);
	m_assTracks.clear();
	for (size_t i = 0; i < asscontexts.size(); i++)
	{
		m_assTracks.emplace_back(asscontexts[i]);
	}
	needRefresh = true;
	return E_NOTIMPL;
}

HRESULT DirectRender::SetPreviewAss(std::string asscontent)
{
	WXAutoLock al(m_lckAss);
	if (m_track != NULL)
	{
		ass_free_track(m_track);
		m_track = NULL;
	}
	if (asscontent.length() <= 0)
		return E_FAIL;
	m_track = AssEngine::Instance().Read(asscontent.c_str());
	needRefresh = true;

	return S_OK;
}

HRESULT DirectRender::SetCacheAss(std::string asscontent)
{
	WXAutoLock al(m_lckAss);


		if (m_cachetrack!= NULL)
		{
			ass_free_track(m_cachetrack);
			m_cachetrack = NULL;
		}
		m_cachetrack = AssEngine::Instance().Read(asscontent.c_str());

		if (m_track!= NULL)
		{
			//遍历一次 track.events, 匹配的events设置标志位
			for (size_t i = 0; i < m_track->n_events; i++)
			{
				m_track->events[i].Start = abs(m_track->events[i].Start);
				m_track->events[i].Duration = abs(m_track->events[i].Duration);
				if (m_cachetrack!= NULL)
				{
					if (strcmp(m_track->events[i].Name, "") != 0)
					{
						for (size_t j = 0; j < m_cachetrack->n_events; j++)
						{
							if (strcmp(m_cachetrack->events[j].Name, m_track->events[i].Name) == 0)
							{
								m_track->events[i].Start = -m_track->events[i].Start;
								m_track->events[i].Duration = -m_track->events[i].Duration;
							}
						}
					}
				}
				
				
			}
		}
		needRefresh = true;


	return E_NOTIMPL;
}

RECT DirectRender::GetAssEventSize(int width, int height, float ts, std::string name)
{
	WXAutoLock al(m_lckAss);
	RECT s = {width, height, 20,20 };

		if (m_track != NULL)
		{
			ass_set_frame_size(AssEngine::Instance().ass_renderer, width, height);
			SIZE location = { width,height };
			SIZE rb = { 0,0 };
			Rect rect =  ass_bound_rect(AssEngine::Instance().ass_renderer, m_track, INT64(ts * 1000), name.c_str());
			s = { rect.x0, rect.y0, rect.x1, rect.y1 };

			needRefresh = true;
		}

	return s;
}

RECT DirectRender::GetAssPreviewSize(int width, int height, float ts, std::string name, std::string asscontent)
{
	WXAutoLock al(m_lckAss);
	RECT s = { width, height, 20,20 };

	auto track = AssEngine::Instance().Read(asscontent.c_str());
	if (track != NULL)
	{
		ass_set_frame_size(AssEngine::Instance().ass_renderer, width, height);
		Rect rect =   ass_bound_rect(AssEngine::Instance().ass_renderer, track, INT64(ts * 1000), name.c_str());
		s = { rect.x0, rect.y0, rect.x1, rect.y1 };
		ass_free_track(track);
	}

	return s;
}



RECT DirectRender::GetAssCursorRect(int width, int height, float ts, int index, int cursorindex)
{
	RECT s = { width, height, 20,20 };
	//s_taskAss->sync([&] {

	//	if (m_track != NULL)
	//	{
	//		ass_set_frame_size(AssEngine::Instance().ass_renderer, width, height);
	//		Rect r= ass_cursor_pos(AssEngine::Instance().ass_renderer, m_track, ts * 1000, cursorindex);
	//		s = RECT{ r.x0, r.y0, r.x1, r.y1 };
	//	}

	//});

	//Log_info("end %s", __FUNCTION__);
	return s;
}


HRESULT DirectRender::SetChromaParamenter(ChromaKeyContext context)
{
	if (IsInvalid)
	{
		return 0;
	}
	
	if (context.color<0)
	{
		ResetFilterParamenter();
		return S_FALSE;
	}
	needRefresh = true;
	m_CurrentFilter = m_chromakeyFilter;
	
	m_chromakeyFilter->SetParameter(&context);

	return S_OK;
}

HRESULT DirectRender::SetColorAdjustParamenter(ColorAdjustContext context)
{
	if (IsInvalid)
	{
		return 0;
	}

	m_CurrentFilter = m_colorAdjustFilter;
	m_colorAdjustFilter->SetParameter(&context);

	needRefresh = true;
	return S_OK;
}

HRESULT DirectRender::SetImageEnhanceParamenter(ImageEnhanceContext context)
{
	if (IsInvalid)
	{
		return 0;
	}
	//Log_info("%s: threshold:%d, degree: %lf", __FUNCTION__, context.threshold, context.degree);
	needRefresh = true;
	m_CurrentFilter = m_imageEnhanceFilter;
	m_CurrentFilter->SetParameter(&context);
	return S_OK;
}


HRESULT DirectRender::SetDeLogoParameter(DeLogoContext context)
{
	if (IsInvalid)
	{
		return 0;
	}
	WXLogA("%s: context.mod[0] %d, context.count %d, context.degree[0] %f, context.alpha[0] %f, context.rects[0].x %f, context.rects[0].y %f, context.rects[0].w %f, context.rects[0].h %f", __FUNCTION__, 
		context.mod[0], context.count, context.degree[0], context.alpha[0], context.rects[0].x, context.rects[0].y, context.rects[0].z, context.rects[0].w);
	needRefresh = true;
	m_CurrentFilter = m_delogoFilter;
	m_CurrentFilter->SetParameter(&context);
	return S_OK;
}

HRESULT DirectRender::SetPreviewSize(int width, int height)
{
	render_width = width;
	render_height = height;
	needupdaterendersize = true;
	return S_OK;
}
extern D3DPRESENT_PARAMETERS d3dpp;
HRESULT DirectRender::ReleaseDevice()
{
	ID3DFilter::UnInit(Device);
	Device->Release();
	return S_OK;
}

HRESULT DirectRender::InitDevice(BOOL debugmode)
{
	
	//Log_info("begin : %s", __FUNCTION__);
	HRESULT hr= d3d::InitD3D(360, 360, &Device);
	
	if(hr != S_OK)
		MessageBox(NULL,
			L"sorry ,your config of D3D is not support current version, please update it",
			L"", MB_OK);


	if (FAILED(hr))
	{
		if (ML_GetErrorLogCallBack())
		{
			ML_GetErrorLogCallBack()((char*)"D3D Error", 0x0fff0001);
			//Log_info("D3D Error", 0x0fff0001);
		}
		return hr;
	}
	render_width = 1280;
	render_height = 720;
	
	hr= CompileShader();

	//Log_info("end : %s", __FUNCTION__);
	return hr;
}

AVFrame* copyFrame = av_frame_alloc();
AVFrame* RealtimeFrame = av_frame_alloc();
#define HD 720
SIZE Get720Size(int width, int height)
{


	SIZE size = { HD,HD };
	if (width < height)
	{
		size.cy = height * HD / width;
		if ( size.cy==1278)
		{
			size.cy = 1280;
		}
	}
	else if (width > height)
	{
		size.cx = width * HD / height;
		if (size.cx == 1278)
		{
			size.cx = 1280;
		}
	}
	return size;
}

IDirect3DSurface9* offscreenSurface;

HRESULT DirectRender::RenderFrame(AVFrame* frame, bool paused)
{
	if (frame == nullptr || frame->data[0] == nullptr)
		return S_OK;

	HRESULT hr = S_OK;

	if ( needRefresh  == false && paused &&lastpts == frame->pts)
	{
		return 1;
	}
	bool flag = 0;
	lastpts = frame->pts;

	{
		WXAutoLock al(m_lckAss);
		if (m_track != NULL || m_assTracks.size() > 0)
		{
			SIZE size_hd = Get720Size(frame->width, frame->height);
			ass_set_frame_size(AssEngine::Instance().ass_renderer, size_hd.cx, size_hd.cy);
			int subtime = frame->pts * 1000 / 20;



			if (copyFrame->width != size_hd.cx || copyFrame->height != size_hd.cy)
			{
				av_frame_unref(copyFrame);
				copyFrame = av_frame_alloc();
				copyFrame->format = frame->format;
				copyFrame->width = size_hd.cx;
				copyFrame->height = size_hd.cy;
				copyFrame->channels = frame->channels;
				copyFrame->channel_layout = frame->channel_layout;
				copyFrame->nb_samples = frame->nb_samples;
				av_frame_get_buffer(copyFrame, 32);
			}

			if (frame->linesize[0] > 0)
			{
				libyuv::ARGBScale(frame->data[0], frame->linesize[0], frame->width, frame->height,
					copyFrame->data[0], copyFrame->linesize[0], copyFrame->width, copyFrame->height, libyuv::kFilterLinear);
			}
			else
			{
				libyuv::ARGBScale(frame->data[0], -frame->linesize[0], frame->width, -frame->height,
					copyFrame->data[0], copyFrame->linesize[0], copyFrame->width, copyFrame->height, libyuv::kFilterLinear);
			}

			frame = copyFrame;
			for (size_t i = 0; i < m_assTracks.size(); i++) //轨道字幕叠加
			{

				ass_image* imgAss  = m_assTracks[i].Render(subtime);
				if (imgAss != NULL)
				{
					blendass(frame->data[0], frame->linesize[0], frame->height, imgAss);
				}
			}
			if (m_track != NULL) //设置的字幕叠加
			{
				ass_image* imgAss = ass_render_frame(AssEngine::Instance().ass_renderer, m_track, subtime, NULL);
				if (imgAss != NULL)
				{
					blendass(frame->data[0], frame->linesize[0], frame->height, imgAss);
				}
			}


			if (m_cachetrack != NULL) //cache字幕叠加
			{
				ass_image* imgCache = ass_render_frame(AssEngine::Instance().ass_renderer, m_cachetrack, subtime, NULL);
				if (imgCache != NULL)
				{
					blendass(frame->data[0], frame->linesize[0], frame->height, imgCache);
				}
			}
		}
	}

	if (m_CurrentFilter)
	{
		m_CurrentFilter->Render({ {frame->data[0],frame->width, frame->height,std::string("inputtexture"),frame->linesize[0] } }, frame->width, frame->height);
		needRefresh = false;
		RealtimeRefresh = false;
	}
	
	if (frame->width>0&&frame->height>0)
	{
		if (offscreenSurface== NULL)
		{
			hr = Device->CreateOffscreenPlainSurface(frame->width, frame->height, D3DFMT_X8R8G8B8, D3DPOOL_SYSTEMMEM, &offscreenSurface, NULL);
			if (FAILED(hr))
				return false;
		}


		hr = Device->GetRenderTargetData(m_CurrentFilter->GetOutputTexture()->GetSurface(), offscreenSurface);

	}
	return hr;
}


 IDirect3DSurface9* DirectRender::GetFrontSurface()
{
	 if (m_CurrentFilter)
	 {
		 return m_CurrentFilter ->GetOutputTexture()->GetSurface();
	 }
	 return NULL;
}