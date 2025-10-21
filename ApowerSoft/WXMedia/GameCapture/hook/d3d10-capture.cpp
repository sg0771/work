#include <d3d10.h>
#include <dxgi.h>
#include "dxgi-helpers.hpp"
#include "graphics-hook.h"
#include "funchook.h"
#include <atlbase.h>

#include "RGBData.h"

struct d3d10_data {
	ID3D10Device                   *device = NULL; /* do not release */
	uint32_t                       m_iWidth = 0;
	uint32_t                       m_iHeight = 0;
	DXGI_FORMAT                    format;
	bool                           multisampled=false;

	struct shmem_data      *shmem_info = NULL;
	CComPtr<ID3D10Texture2D>copy_surfaces = NULL;
	CComPtr<ID3D10Texture2D>textures = NULL;
	bool                   texture_ready[NUM_BUFFERS] = {false};

	int                    cur_tex = 0;

	int m_iType = WX_RGB32;

	uint8_t* m_pBuf[NUM_BUFFERS] = { NULL };
	//内存Texture，可读写
	 bool create_d3d10_stage_surface(ID3D10Texture2D **tex){
		D3D10_TEXTURE2D_DESC desc = {};
		desc.Width = m_iWidth;
		desc.Height = m_iHeight;
		desc.Format = format;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D10_USAGE_STAGING;
		desc.CPUAccessFlags = D3D10_CPU_ACCESS_READ | D3D10_CPU_ACCESS_WRITE; //读写模式，便于写字

		HRESULT hr = device->CreateTexture2D(&desc, nullptr, tex);
		if (FAILED(hr)) {
			hlog_hr("create_d3d10_stage_surface: failed to create texture", hr);
			return false;
		}
		return true;
	}

	 //显存Texture，保存屏幕数据
	 bool create_d3d10_tex(uint32_t m_iWidth, uint32_t m_iHeight, ID3D10Texture2D **tex) {
		D3D10_TEXTURE2D_DESC desc = {};
		desc.Width = m_iWidth;
		desc.Height = m_iHeight;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = format;
		desc.BindFlags = D3D10_BIND_RENDER_TARGET;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D10_USAGE_DEFAULT;
		desc.MiscFlags = 0;
		HRESULT hr = device->CreateTexture2D(&desc, nullptr, tex);
		if (FAILED(hr)) {
			hlog_hr("create_d3d10_tex: failed to create texture", hr);
			return false;
		}
		return true;
	}

	 inline bool d3d10_init_format(IDXGISwapChain *swap, HWND &window) {
		DXGI_SWAP_CHAIN_DESC desc;
		HRESULT hr = swap->GetDesc(&desc);
		if (FAILED(hr)) {
			hlog_hr("d3d10_init_format: swap->GetDesc failed", hr);
			return false;
		}
		format = fix_dxgi_format(desc.BufferDesc.Format);
		if (format == DXGI_FORMAT_R8G8B8A8_UNORM)
			m_iType = WX_BRGA;
		multisampled = desc.SampleDesc.Count > 1;
		window = desc.OutputWindow;
		m_iWidth = desc.BufferDesc.Width;
		m_iHeight = desc.BufferDesc.Height;
		return true;
	}

	 bool d3d10_shmem_init_buffers()
	{
		bool success = create_d3d10_stage_surface(&copy_surfaces);
		if (!success) {
			wxlog("d3d10_shmem_init_buffers: failed to create copy surface");
			return false;
		}
		success = create_d3d10_tex(m_iWidth, m_iHeight, &textures);
		if (!success) {
			wxlog("d3d10_shmem_init_buffers: failed to create texture");
			return false;
		}
		return true;
	}

	 bool d3d10_shmem_init(HWND window) {
		if (!d3d10_shmem_init_buffers()) {
			return false;
		}
		if (!capture_init_shmem(&shmem_info, window, m_iWidth, m_iHeight, m_iWidth*4, format, false)) {
			return false;
		}
		for (size_t i = 0; i < NUM_BUFFERS; i++) {
			m_pBuf[i] = new uint8_t[m_iHeight * m_iWidth*4];
		}
		wxlog("d3d10 memory capture successful");
		return true;
	}

	 void d3d10_init(IDXGISwapChain *swap) {
		HRESULT hr = swap->GetDevice(__uuidof(ID3D10Device), (void**)&device);
		if (FAILED(hr)) {
			hlog_hr("WXMedia d3d10_init: failed to get device from swap", hr);
			return;
		}
		/* remove the unneeded extra reference */
		device->Release();
		HWND window = NULL;
		if (!d3d10_init_format(swap, window)) {
			return;
		}
		bool success = d3d10_shmem_init(window);
		if (!success)
			d3d10_free();
	}

	 inline void d3d10_copy_texture(ID3D10Resource *dst, ID3D10Resource *src) {
		if (multisampled) {
			device->ResolveSubresource(dst, 0, src, 0, format);
		}
		else {
			device->CopyResource(dst, src);
		}
	}

	 inline void d3d10_shmem_queue_copy() {
		for (size_t i = 0; i < NUM_BUFFERS; i++) {
			if (texture_ready[i]) {
				texture_ready[i] = false;
				shmem_copy_data(i, m_pBuf[i]);
				break;
			}
		}
	}

	inline void d3d10_shmem_capture(ID3D10Resource *backbuffer)
	{
		int next_tex = 0;
		d3d10_shmem_queue_copy();
		next_tex = (cur_tex == NUM_BUFFERS - 1) ? 0 : cur_tex + 1;
		d3d10_copy_texture(textures, backbuffer);//textures 是屏幕格式， 

		{
			if (shmem_texture_data_lock(next_tex)) {
				shmem_texture_data_unlock(next_tex);
			}
			d3d10_copy_texture(textures, textures);//textures 是RGBA格式
			D3D10_MAPPED_TEXTURE2D map = {};
			HRESULT hr = copy_surfaces->Map(0, D3D10_MAP_READ_WRITE, 0, &map);
			if (SUCCEEDED(hr)) {

				WXCountFps();

				uint8_t *pVideo = (uint8_t*)map.pData;

				if (HookCheckCamera(m_iType)) { //画中画优先
					HookDrawCamera(pVideo, m_iWidth, m_iHeight, map.RowPitch, 0, m_iType);
				}

				for (uint32_t i = 0; i < m_iHeight; i++) {
					memcpy(m_pBuf[next_tex] + i * m_iWidth * 4, pVideo + i * map.RowPitch, m_iWidth*4);//输出数据
				}

				if (!global_hook_info->m_bCapture) {
					HookDrawImage(pVideo, m_iWidth, m_iHeight, map.RowPitch, 0, m_iType);
					HookDrawString(pVideo, m_iWidth, m_iHeight, map.RowPitch, 0, m_iType);
					copy_surfaces->Unmap(NULL);
					d3d10_copy_texture(textures, copy_surfaces);
					d3d10_copy_texture(backbuffer, textures);//写字画图后显示
				}else {
					copy_surfaces->Unmap(NULL);
				}
			}
			texture_ready[next_tex] = true;
		}
		cur_tex = next_tex;
	}
};

static struct d3d10_data data = {};

void d3d10_free(void){
	capture_free();
	data.copy_surfaces = NULL;
	data.textures = NULL;
	for (size_t i = 0; i < NUM_BUFFERS; i++) {
		delete []data.m_pBuf[i];
	}
	memset(&data, 0, sizeof(data));
	wxlog("----------------- d3d10 capture freed ----------------");
}



void d3d10_capture(void *swap_ptr, void *backbuffer_ptr){
	IDXGIResource *dxgi_backbuffer = (IDXGIResource*)backbuffer_ptr;
	IDXGISwapChain *swap = (IDXGISwapChain*)swap_ptr;
	if (capture_should_stop()) {
		d3d10_free();
		return;
	}
	if (capture_should_init()) {
		data.d3d10_init(swap);
	}
	if (capture_ready()) {
		CComPtr<ID3D10Resource>backbuffer = NULL;
		HRESULT hr = dxgi_backbuffer->QueryInterface(__uuidof(ID3D10Resource),(void**)&backbuffer);
		if (FAILED(hr)) {
			hlog_hr("d3d10_shtex_capture: failed to get backbuffer", hr);
			return;
		}
		data.d3d10_shmem_capture(backbuffer);
		backbuffer = NULL;
	}
}
