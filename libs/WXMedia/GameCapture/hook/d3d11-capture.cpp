#include <d3d11.h>
#include <dxgi.h>
#include "dxgi-helpers.hpp"
#include "graphics-hook.h"
#include "funchook.h"
#include <atlbase.h>
#include "RGBData.h"

struct d3d11_data {
	ID3D11Device                   *device = NULL; /* do not release */
	ID3D11DeviceContext            *context = NULL; /* do not release */
	uint32_t                       m_iWidth = 0;
	uint32_t                       m_iHeight = 0;
	DXGI_FORMAT                    format;
	bool                           multisampled = false;

	CComPtr<ID3D11Texture2D>       copy_surfaces = NULL;
	CComPtr<ID3D11Texture2D>       textures = NULL;

	bool                   texture_ready[NUM_BUFFERS] = { false };;
	struct shmem_data      *shmem_info = NULL;
	int                    cur_tex = 0;
	int m_iType = WX_RGB32;
	uint8_t* m_pBuf[NUM_BUFFERS] = { NULL };

	char m_szFormat[20];
public:
	bool create_d3d11_stage_surface(ID3D11Texture2D **tex) {
		wxlog("%s", __FUNCTION__);
		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = m_iWidth;
		desc.Height = m_iHeight;
		desc.Format = format;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_STAGING;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
		HRESULT hr = device->CreateTexture2D(&desc, nullptr, tex);
		if (FAILED(hr)) {
			hlog_hr("WXMedia create_d3d11_stage_surface: failed to create texture", hr);
			return false;
		}
		wxlog("%s OK", __FUNCTION__);
		return true;
	}

	bool create_d3d11_tex(uint32_t m_iWidth, uint32_t m_iHeight, ID3D11Texture2D **tex) {
		wxlog("%s", __FUNCTION__);
		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = m_iWidth;
		desc.Height = m_iHeight;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = format;
		desc.BindFlags = D3D11_BIND_RENDER_TARGET;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.MiscFlags = 0;
		HRESULT hr = device->CreateTexture2D(&desc, nullptr, tex);
		if (FAILED(hr)) {
			hlog_hr("WXMedia create_d3d11_tex: failed to create texture", hr);
			wxlog("%s Error", __FUNCTION__);
			return false;
		}
		wxlog("%s OK", __FUNCTION__);
		return true;
	}

	bool d3d11_init_format(IDXGISwapChain *swap, HWND &window){
		wxlog("%s",__FUNCTION__);
		DXGI_SWAP_CHAIN_DESC desc;
		HRESULT hr = swap->GetDesc(&desc);
		if (FAILED(hr)) {
			hlog_hr("d3d11_init_format: swap->GetDesc failed", hr);
			return false;
		}
		format = fix_dxgi_format(desc.BufferDesc.Format);

		strcpy(m_szFormat, "rgb32");

		if (format == DXGI_FORMAT_R8G8B8A8_UNORM) {
			m_iType = WX_BRGA;
			strcpy(m_szFormat, "r8g8b8a8");
		}

		if (format == DXGI_FORMAT_R10G10B10A2_UNORM) {
			m_iType = WX_R10G10B10A2;
			strcpy(m_szFormat, "r10g10b10a2");
		}



		multisampled = desc.SampleDesc.Count > 1;
		window = desc.OutputWindow;
		m_iWidth = desc.BufferDesc.Width;
		m_iHeight = desc.BufferDesc.Height;
		wxlog("%s OK", __FUNCTION__);
		return true;
	}

	 bool d3d11_shmem_init_buffers() {
		 wxlog("%s ", __FUNCTION__);
		bool success = create_d3d11_stage_surface(&copy_surfaces);
		if (!success) {
			wxlog("%s error", __FUNCTION__);
			return false;
		}

		success = create_d3d11_tex(m_iWidth, m_iHeight, &textures);
		if (!success) {
			wxlog("%s error", __FUNCTION__);
			return false;
		}
		wxlog("%s OK", __FUNCTION__);
		return true;
	}

	bool d3d11_shmem_init(HWND window) {
		wxlog("%s", __FUNCTION__);
		if (!d3d11_shmem_init_buffers()) {
			wxlog("%s Error d3d11_shmem_init_buffers", __FUNCTION__);
			return false;
		}
		if (!capture_init_shmem(&shmem_info, window, m_iWidth, m_iHeight, m_iWidth*4, format, false)) {
			wxlog("%s Error capture_init_shmem", __FUNCTION__);
			return false;
		}
		for (size_t i = 0; i < NUM_BUFFERS; i++) {
			m_pBuf[i] = new uint8_t[m_iHeight * m_iWidth*4];
		}
		return true;
	}

	void d3d11_init(IDXGISwapChain *swap) {
		wxlog("%s Begin", __FUNCTION__);
		bool success = true;
		HWND window;
		HRESULT hr = swap->GetDevice(__uuidof(ID3D11Device), (void**)&device);
		if (FAILED(hr)) {
			wxlog("%s Error in swap->GetDevice", __FUNCTION__);
			hlog_hr("d3d11_init: failed to get device from swap", hr);
			return;
		}
		device->Release();
		device->GetImmediateContext(&context);
		context->Release();
		if (!d3d11_init_format(swap, window)) {
			wxlog("%s Error In d3d11_init_format", __FUNCTION__);
			return;
		}
		success = d3d11_shmem_init(window);
		if (!success) {
			wxlog("%s Error In d3d11_shmem_init", __FUNCTION__);
			d3d11_free();
		}else {
			wxlog("d3d11 memory capture successful  Size=%dx%d Pitch=%d Format=%s ", m_iWidth, m_iHeight, m_iWidth * 4, m_szFormat);
		}
	}

	inline void d3d11_copy_texture(ID3D11Resource *dst, ID3D11Resource *src) {
		if (multisampled) {
			context->ResolveSubresource(dst, 0, src, 0, format);
		}else {
			context->CopyResource(dst, src);
		}
	}

	inline void d3d11_shmem_capture(ID3D11Resource *backbuffer) {
		for (size_t i = 0; i < NUM_BUFFERS; i++) {
			if (texture_ready[i]) {
				texture_ready[i] = false;
				shmem_copy_data(i, m_pBuf[i]);
				break;
			}
		}
		int next_tex = (cur_tex == NUM_BUFFERS - 1) ? 0 : cur_tex + 1;

		d3d11_copy_texture(textures, backbuffer);
		d3d11_copy_texture(copy_surfaces, textures);
		{
			if (shmem_texture_data_lock(next_tex)) {
				shmem_texture_data_unlock(next_tex);
			}
			D3D11_MAPPED_SUBRESOURCE map = {};
			HRESULT hr = context->Map(copy_surfaces, 0,  D3D11_MAP_READ_WRITE, 0, &map);
			if (SUCCEEDED(hr)) {

				WXCountFps();

				uint8_t *pVideo = (uint8_t*)map.pData;
				if (HookCheckCamera(m_iType))
					HookDrawCamera(pVideo, m_iWidth, m_iHeight, map.RowPitch, 0, m_iType);

				for(uint32_t i = 0; i < m_iHeight;i++)
					memcpy(m_pBuf[next_tex] + i * m_iWidth*4, pVideo + i * map.RowPitch, m_iWidth*4);//拷贝抓图数据，发给WXMedia

				if (!global_hook_info->m_bCapture) { //写字画图
					HookDrawImage(pVideo, m_iWidth, m_iHeight, map.RowPitch, 0, m_iType);
					HookDrawString(pVideo, m_iWidth, m_iHeight, map.RowPitch, 0, m_iType);

					context->Unmap(copy_surfaces, 0);
					d3d11_copy_texture(textures, copy_surfaces);
					d3d11_copy_texture(backbuffer, textures);
				}else {
					context->Unmap(copy_surfaces, 0);//不做操作
				}
			}else {
				hlog_hr("context->Map : failed to get", hr);
			}

			texture_ready[next_tex] = true;
		}
		cur_tex = next_tex;
	}
};

static struct d3d11_data data = {};

void d3d11_free(void) {
	capture_free();
	for (size_t i = 0; i < NUM_BUFFERS; i++) {
		delete[]data.m_pBuf[i];
	}
	data.copy_surfaces = NULL;
	data.textures = NULL;
	memset(&data, 0, sizeof(data));
	wxlog("----------------- d3d11 capture freed ----------------");
}

void d3d11_capture(void *swap_ptr, void *backbuffer_ptr){
	IDXGIResource *dxgi_backbuffer = (IDXGIResource*)backbuffer_ptr;
	IDXGISwapChain *swap = (IDXGISwapChain*)swap_ptr;
	HRESULT hr;
	if (capture_should_stop()) {
		wxlog("%s capture_should_stop", __FUNCTION__);
		d3d11_free();
		return;
	}
	if (capture_should_init()) {
		wxlog("%s capture_should_init", __FUNCTION__);
		data.d3d11_init(swap);
	}
	if (capture_ready()) {
		CComPtr<ID3D11Resource>backbuffer = NULL;
		hr = dxgi_backbuffer->QueryInterface(__uuidof(ID3D11Resource),(void**)&backbuffer);
		if (FAILED(hr)) {
			hlog_hr("d3d11_shtex_capture: failed to get "  "backbuffer", hr);
			wxlog("%s dxgi_backbuffer->QueryInterface", __FUNCTION__);
			return;
		}
		data.d3d11_shmem_capture(backbuffer);
		backbuffer = NULL;
	}
}
