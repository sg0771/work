#include <d3d9.h>
#include <d3d11.h>
#include <dxgi.h>
#include <atlbase.h>

#include "graphics-hook.h"
#include "funchook.h"
#include "d3d9-patches.hpp"

#include "RGBData.h"

typedef HRESULT (STDMETHODCALLTYPE *present_t)(IDirect3DDevice9*,
		CONST RECT*, CONST RECT*, HWND, CONST RGNDATA*);
typedef HRESULT (STDMETHODCALLTYPE *present_ex_t)(IDirect3DDevice9*,
		CONST RECT*, CONST RECT*, HWND, CONST RGNDATA*, DWORD);
typedef HRESULT (STDMETHODCALLTYPE *present_swap_t)(IDirect3DSwapChain9*,
		CONST RECT*, CONST RECT*, HWND, CONST RGNDATA*, DWORD);
typedef HRESULT (STDMETHODCALLTYPE *reset_t)(IDirect3DDevice9*,
		D3DPRESENT_PARAMETERS*);
typedef HRESULT (STDMETHODCALLTYPE *reset_ex_t)(IDirect3DDevice9*,
		D3DPRESENT_PARAMETERS*, D3DDISPLAYMODEEX*);

typedef HRESULT (WINAPI *createfactory1_t)(REFIID, void **);

static struct func_hook present;
static struct func_hook present_ex;
static struct func_hook present_swap;
static struct func_hook reset;
static struct func_hook reset_ex;

extern "C" {
	extern TCHAR g_szFileName[MAX_PATH];
}

struct d3d9_data {

	IDirect3DDevice9       *m_device = NULL; /* do not release */
	uint32_t               cx = 0;
	uint32_t               cy = 0;
	D3DFORMAT              d3d9_format;
	DXGI_FORMAT            dxgi_format;

	CComPtr<IDirect3DSurface9> m_pD3dSurface = NULL;
	CComPtr<IDirect3DSurface9> copy_surfaces =   NULL;

	CComPtr<IDirect3DSurface9> copy_surfaces2 = NULL;

	struct shmem_data      *shmem_info = NULL;
	bool                   texture_mapped[NUM_BUFFERS];
	int                    cur_tex;

	int m_iType = WX_RGB32;
	uint8_t* m_pBuf[NUM_BUFFERS] = { NULL }; //原始数据,用于输出
	uint8_t* m_pBufDraw = NULL ;//写字或者画图后的数据，用于显示修改后的数据
};

static struct d3d9_data data = {};


static void d3d9_free()
{
	capture_free();
	data.copy_surfaces = nullptr;
	
	data.m_pD3dSurface = nullptr;

	for (size_t i = 0; i < NUM_BUFFERS; i++) {
		if (data.m_pBuf[i]) {
			delete[]data.m_pBuf[i];
			data.m_pBuf[i] = nullptr;
		}
	}
	if (data.m_pBufDraw) {
		delete[]data.m_pBufDraw;
		data.m_pBufDraw = nullptr;
	}

	memset(&data, 0, sizeof(data));
	wxlog("----------------- d3d9 capture freed -----------------");
}

static DXGI_FORMAT d3d9_to_dxgi_format(D3DFORMAT format){
	switch ((unsigned long)format) {
		case D3DFMT_A2B10G10R10: return DXGI_FORMAT_R10G10B10A2_UNORM;
		case D3DFMT_A8R8G8B8:    return DXGI_FORMAT_B8G8R8A8_UNORM;
		case D3DFMT_X8R8G8B8:    return DXGI_FORMAT_B8G8R8X8_UNORM;
	}
	return DXGI_FORMAT_UNKNOWN;
}

static bool d3d9_shmem_init_buffers()
{
	HRESULT hr = data.m_device->CreateOffscreenPlainSurface(data.cx, data.cy,data.d3d9_format, D3DPOOL_SYSTEMMEM,&data.copy_surfaces, nullptr);
	if (FAILED(hr)) {
		hlog_hr("d3d9_shmem_init_buffers: Failed to create surface",hr);
		return false;
	}

	hr = data.m_device->CreateOffscreenPlainSurface(data.cx, data.cy, data.d3d9_format, D3DPOOL_DEFAULT, &data.copy_surfaces2, nullptr);
	if (FAILED(hr)) {
		hlog_hr("d3d9_shmem_init_buffers: Failed to create surface", hr);
		return false;
	}

	hr = data.m_device->CreateOffscreenPlainSurface(data.cx, data.cy,D3DFMT_X8R8G8B8,D3DPOOL_DEFAULT,&data.m_pD3dSurface,nullptr);
	if (FAILED(hr)) {
		hlog_hr("data.m_device->CreateOffscreenPlainSurface D3DFMT_X8R8G8B8", hr);
		return false;
	}
	data.m_pBufDraw = new uint8_t[data.cx * 4 * data.cy];
	return true;
}


static const char* d3d9_to_string(D3DFORMAT format) {
	switch ((unsigned long)format) {
	case D3DFMT_A2B10G10R10: return "r10g10b10a2";
	case D3DFMT_A8R8G8B8:    return "rgb32";
	case D3DFMT_X8R8G8B8:    return "rgb32";
	}
	return "d3d9_unknown";
}
static bool d3d9_shmem_init(HWND window)
{
	if (!d3d9_shmem_init_buffers()) {
		return false;
	}
	if (!capture_init_shmem(&data.shmem_info, window, 
				data.cx, data.cy, data.cx * 4, data.dxgi_format,
				false)) {
		return false;
	}
	for (size_t i = 0; i < NUM_BUFFERS; i++) {
		data.m_pBuf[i]  = new uint8_t[data.cx * 4 * data.cy];
	}

	wxlog("d3d9 memory capture successful  Size=%dx%d Pitch=%d Format=%s ", data.cx, data.cy, data.cx * 4, d3d9_to_string(data.d3d9_format));
	return true;
}

static bool d3d9_get_swap_desc(D3DPRESENT_PARAMETERS &pp)
{
	IDirect3DSwapChain9 *swap = nullptr;
	HRESULT hr = data.m_device->GetSwapChain(0, &swap);
	if (FAILED(hr)) {
		hlog_hr("d3d9_get_swap_desc: Failed to get swap chain", hr);
		return false;
	}

	hr = swap->GetPresentParameters(&pp);
	swap->Release();

	if (FAILED(hr)) {
		hlog_hr("d3d9_get_swap_desc: Failed to get "
		        "presentation parameters", hr);
		return false;
	}

	return true;
}

static bool d3d9_init_format_backbuffer(HWND &window)
{
	IDirect3DSurface9 *back_buffer = nullptr;
	D3DPRESENT_PARAMETERS pp;
	D3DSURFACE_DESC desc;
	HRESULT hr;

	if (!d3d9_get_swap_desc(pp)) {
		return false;
	}

	hr = data.m_device->GetRenderTarget(0, &back_buffer);
	if (FAILED(hr)) {
		return false;
	}

	hr = back_buffer->GetDesc(&desc);
	back_buffer->Release();

	if (FAILED(hr)) {
		hlog_hr("d3d9_init_format_backbuffer: Failed to get "
		        "backbuffer descriptor", hr);
		return false;
	}

	data.d3d9_format = desc.Format;
	data.dxgi_format = d3d9_to_dxgi_format(desc.Format);

	if (data.dxgi_format == DXGI_FORMAT_R8G8B8A8_UNORM)
		data.m_iType = WX_BRGA;
	if (data.dxgi_format == DXGI_FORMAT_R10G10B10A2_UNORM)
		data.m_iType = WX_R10G10B10A2;

	window = pp.hDeviceWindow;
	data.cx = desc.Width;
	data.cy = desc.Height;
	return true;
}

static bool d3d9_init_format_swapchain(HWND &window)
{
	D3DPRESENT_PARAMETERS pp;
	if (!d3d9_get_swap_desc(pp)) {
		return false;
	}
	data.dxgi_format = d3d9_to_dxgi_format(pp.BackBufferFormat);
	data.d3d9_format = pp.BackBufferFormat;
	window = pp.hDeviceWindow;
	data.cx = pp.BackBufferWidth;
	data.cy = pp.BackBufferHeight;
	return true;
}

static void d3d9_init(IDirect3DDevice9 *device)
{
	bool has_d3d9ex_bool_offset =
		global_hook_info->offsets.d3d9.d3d9_clsoff &&
		global_hook_info->offsets.d3d9.is_d3d9ex_clsoff;
	bool success;
	HWND window = nullptr;
	data.m_device = device;
	if (!d3d9_init_format_backbuffer( window)) {
		if (!d3d9_init_format_swapchain( window)) {
			return;
		}
	}

	success = d3d9_shmem_init(window);
	
	if (!success)
		d3d9_free();
}


static inline void d3d9_shmem_capture(IDirect3DSurface9 *backbuffer)
{
	for (int i = 0; i < NUM_BUFFERS; i++) {
		if (!data.texture_mapped[i]) {
			data.texture_mapped[i] = true;
			shmem_copy_data(i, data.m_pBuf[i]);
			break;
		}
	}

	int next_tex = (data.cur_tex == NUM_BUFFERS - 1) ?  0 : data.cur_tex + 1;

	HRESULT hr = data.m_device->GetRenderTargetData(backbuffer, data.copy_surfaces); //8876086C 饥荒
	if (FAILED(hr)) {
		wxlog("d3d9_shmem_capture: GetRenderTargetData  failed 222");
		hr = data.m_device->GetRenderTargetData(backbuffer, data.copy_surfaces2); //8876086C
		if (FAILED(hr)) {
			hr = data.m_device->StretchRect(backbuffer, NULL, data.copy_surfaces2, NULL, D3DTEXF_NONE);
			hlog_hr("d3d9_shmem_capture: Copy Data From Buffer  failed 3333", hr);
		}
			
	}

	{
		if (shmem_texture_data_lock(next_tex)) {
			data.texture_mapped[next_tex] = false;
			shmem_texture_data_unlock(next_tex);
		}
		data.m_device->BeginScene();
		D3DLOCKED_RECT source;
		hr = data.copy_surfaces->LockRect(&source, 0, D3DLOCK_DONOTWAIT);//RGB32 ?
		if (SUCCEEDED(hr)) {

			uint8_t *bufSrc = (uint8_t*)source.pBits;
			int PitchSrc = source.Pitch;
			 {
				WXCountFps();
				for (uint32_t i = 0; i < data.cy; i++) {
					memcpy(data.m_pBuf[next_tex] + i * data.cx * 4, bufSrc + i * PitchSrc, data.cx * 4);
				}
				 {
					if (HookCheckCamera(data.m_iType)) { //画中画！
						HookDrawCamera(data.m_pBuf[next_tex], data.cx, data.cy, data.cx * 4, 0, data.m_iType);
					}
					//写文字或者写图像
					if (!global_hook_info->m_bCapture) {
						memcpy(data.m_pBufDraw, data.m_pBuf[next_tex], data.cx * 4 * data.cy);
						HookDrawImage(data.m_pBufDraw, data.cx, data.cy, data.cx * 4, 0, data.m_iType);
						HookDrawString(data.m_pBufDraw, data.cx, data.cy, data.cx * 4, 0, data.m_iType);

						D3DLOCKED_RECT fk;
						hr = data.m_pD3dSurface->LockRect(&fk, 0, D3DLOCK_DONOTWAIT);
						if (SUCCEEDED(hr)) {
							uint8_t *bufDst = (uint8_t*)fk.pBits;
							int PitchDst = fk.Pitch;
							for (uint32_t i = 0; i < data.cy; i++) {
								memcpy(bufDst + i * PitchDst, data.m_pBufDraw + i * PitchSrc, data.cx * 4);//拷贝回BackBuffer
							}
							data.m_pD3dSurface->UnlockRect();
							hr = data.m_device->StretchRect(data.m_pD3dSurface, NULL, backbuffer, NULL, D3DTEXF_NONE);
							//把写字和绘图后的数据贴会屏幕
						}
					}
				}

			}
			data.copy_surfaces->UnlockRect();
		}
		data.m_device->EndScene();
	}
	data.cur_tex = next_tex;
}

static void d3d9_capture(IDirect3DDevice9 *device, IDirect3DSurface9 *backbuffer)
{
		global_hook_info->m_iType = (int)HOOK_TYPE_D3D9;
		if (capture_should_stop()) {
			d3d9_free();
			return;
		}
		if (capture_should_init()) {
			d3d9_init(device);
		}
		if (capture_ready()) {
			if (data.m_device != device) {
				d3d9_free();
				return;
			}
			d3d9_shmem_capture(backbuffer);
		}
}

/* this is used just in case Present calls PresentEx or vise versa. */
static int present_recurse = 0;

static inline HRESULT get_backbuffer(IDirect3DDevice9 *device,
	IDirect3DSurface9 **surface)
{
	static bool use_backbuffer = false;
	static bool checked_exceptions = false;

	if (!checked_exceptions) {
		if (_strcmpi(get_process_name(), "hotd_ng.exe") == 0)
			use_backbuffer = true;
		checked_exceptions = true;
	}

	if (use_backbuffer) {
		return device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO,
			surface);
	}
	else {
		return device->GetRenderTarget(0, surface);
	}
}


static inline void present_begin(IDirect3DDevice9 *device,IDirect3DSurface9 *&backbuffer, BOOL  bPass)
{
	if (bPass)return;
	HRESULT hr;

	if (!present_recurse) {
		hr = get_backbuffer(device, &backbuffer);
		if (FAILED(hr)) {
			hlog_hr("d3d9_shmem_capture: Failed to get "
				"backbuffer", hr);
		}
		d3d9_capture(device, backbuffer);
	}
	present_recurse++;
}

static inline void present_end(IDirect3DDevice9 *device,
		IDirect3DSurface9 *backbuffer, BOOL  bPass)
{
	if (bPass)return;
	present_recurse--;
	if (!present_recurse) {
		if (backbuffer)
			backbuffer->Release();
	}
}

static bool hooked_reset = false;
static void setup_reset_hooks(IDirect3DDevice9 *device);

static HRESULT STDMETHODCALLTYPE hook_present(IDirect3DDevice9 *device,
		CONST RECT *src_rect, CONST RECT *dst_rect,
		HWND override_window, CONST RGNDATA *dirty_region)
{
	BOOL bPass = FALSE;
	if (_wcsicmp(g_szFileName,L"DNF")== 0 && src_rect) {
		bPass = TRUE;
	}

	IDirect3DSurface9 *backbuffer = nullptr;
	HRESULT hr;

	if (!hooked_reset)
		setup_reset_hooks(device);

	present_begin(device, backbuffer, bPass);

	unhook(&present);
	present_t call = (present_t)present.call_addr;
	hr = call(device, src_rect, dst_rect, override_window, dirty_region);
	rehook(&present);

	present_end(device, backbuffer, bPass);

	return hr;
}

static HRESULT STDMETHODCALLTYPE hook_present_ex(IDirect3DDevice9 *device,
		CONST RECT *src_rect, CONST RECT *dst_rect,
		HWND override_window, CONST RGNDATA *dirty_region, DWORD flags)
{
	BOOL bPass = FALSE;
	if (_wcsicmp(g_szFileName, L"DNF") == 0 && src_rect) {
		bPass = TRUE;
	}

	IDirect3DSurface9 *backbuffer = nullptr;
	HRESULT hr;

	if (!hooked_reset)
		setup_reset_hooks(device);
	present_begin(device, backbuffer, bPass);

	unhook(&present_ex);
	present_ex_t call = (present_ex_t)present_ex.call_addr;
	hr = call(device, src_rect, dst_rect, override_window, dirty_region,
			flags);
	rehook(&present_ex);

	present_end(device, backbuffer, bPass);

	return hr;
}

static HRESULT STDMETHODCALLTYPE hook_present_swap(IDirect3DSwapChain9 *swap,
		CONST RECT *src_rect, CONST RECT *dst_rect,
		HWND override_window, CONST RGNDATA *dirty_region, DWORD flags)
{
	BOOL bPass = FALSE;
	if (_wcsicmp(g_szFileName, L"DNF") == 0 && src_rect) {
		bPass = TRUE;
	}
	IDirect3DSurface9 *backbuffer = nullptr;
	IDirect3DDevice9 *device = nullptr;
	HRESULT hr;

	if (!present_recurse) {
		hr = swap->GetDevice(&device);
		if (SUCCEEDED(hr)) {
			device->Release();
		}
	}

	if (device) {
		if (!hooked_reset)
			setup_reset_hooks(device);

		present_begin(device, backbuffer, bPass);
	}

	unhook(&present_swap);
	present_swap_t call = (present_swap_t)present_swap.call_addr;
	hr = call(swap, src_rect, dst_rect, override_window, dirty_region,
			flags);
	rehook(&present_swap);

	if (device)
		present_end(device, backbuffer, bPass);

	return hr;
}

static HRESULT STDMETHODCALLTYPE hook_reset(IDirect3DDevice9 *device,
		D3DPRESENT_PARAMETERS *params)
{
	HRESULT hr;

	if (capture_active())
		d3d9_free();

	unhook(&reset);
	reset_t call = (reset_t)reset.call_addr;
	hr = call(device, params);
	rehook(&reset);

	return hr;
}

static HRESULT STDMETHODCALLTYPE hook_reset_ex(IDirect3DDevice9 *device,
		D3DPRESENT_PARAMETERS *params, D3DDISPLAYMODEEX *dmex)
{
	HRESULT hr;

	if (capture_active())
		d3d9_free();

	unhook(&reset_ex);
	reset_ex_t call = (reset_ex_t)reset_ex.call_addr;
	hr = call(device, params, dmex);
	rehook(&reset_ex);

	return hr;
}

static void setup_reset_hooks(IDirect3DDevice9 *device)
{
	IDirect3DDevice9Ex *d3d9ex = nullptr;
	uintptr_t *vtable = *(uintptr_t**)device;
	HRESULT hr;

	hook_init(&reset, (void*)vtable[16], (void*)hook_reset,
			"IDirect3DDevice9::Reset");
	rehook(&reset);

	hr = device->QueryInterface(__uuidof(IDirect3DDevice9Ex),
			(void**)&d3d9ex);
	if (SUCCEEDED(hr)) {
		hook_init(&reset_ex, (void*)vtable[132], (void*)hook_reset_ex,
				"IDirect3DDevice9Ex::ResetEx");
		rehook(&reset_ex);

		d3d9ex->Release();
	}

	hooked_reset = true;
}

typedef HRESULT (WINAPI *d3d9create_ex_t)(UINT, IDirect3D9Ex**);

static bool manually_get_d3d9_addrs(HMODULE d3d9_module,
		void **present_addr,
		void **present_ex_addr,
		void **present_swap_addr)
{
	d3d9create_ex_t create_ex;
	D3DPRESENT_PARAMETERS pp;
	HRESULT hr;

	IDirect3DDevice9Ex *device;
	IDirect3D9Ex *d3d9ex;

	wxlog("D3D9 values invalid, manually obtaining");

	create_ex = (d3d9create_ex_t)GetProcAddress(d3d9_module,
			"Direct3DCreate9Ex");
	if (!create_ex) {
		wxlog("Failed to load Direct3DCreate9Ex");
		return false;
	}
	if (FAILED(create_ex(D3D_SDK_VERSION, &d3d9ex))) {
		wxlog("Failed to create D3D9 context");
		return false;
	}

	memset(&pp, 0, sizeof(pp));
	pp.Windowed                 = 1;
	pp.SwapEffect               = D3DSWAPEFFECT_FLIP;
	pp.BackBufferFormat         = D3DFMT_A8R8G8B8;
	pp.BackBufferCount          = 1;
	pp.hDeviceWindow            = (HWND)dummy_window;
	pp.PresentationInterval     = D3DPRESENT_INTERVAL_IMMEDIATE;

	hr = d3d9ex->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
			dummy_window,
			D3DCREATE_HARDWARE_VERTEXPROCESSING |
			D3DCREATE_NOWINDOWCHANGES, &pp, NULL, &device);
	d3d9ex->Release();

	if (SUCCEEDED(hr)) {
		uintptr_t *vtable = *(uintptr_t**)device;
		IDirect3DSwapChain9 *swap;

		*present_addr = (void*)vtable[17];
		*present_ex_addr = (void*)vtable[121];

		hr = device->GetSwapChain(0, &swap);
		if (SUCCEEDED(hr)) {
			vtable = *(uintptr_t**)swap;
			*present_swap_addr = (void*)vtable[3];

			swap->Release();
		}

		device->Release();
	} else {
		wxlog("Failed to create D3D9 device");
		return false;
	}

	return true;
}

bool hook_d3d9(void)
{
	HMODULE d3d9_module = get_system_module("d3d9.dll");
	uint32_t d3d9_size;
	void *present_addr = nullptr;
	void *present_ex_addr = nullptr;
	void *present_swap_addr = nullptr;

	if (!d3d9_module) {
		return false;
	}

	d3d9_size = module_size(d3d9_module);

	if (global_hook_info->offsets.d3d9.present      < d3d9_size &&
	    global_hook_info->offsets.d3d9.present_ex   < d3d9_size &&
	    global_hook_info->offsets.d3d9.present_swap < d3d9_size) {

		present_addr = get_offset_addr(d3d9_module,
				global_hook_info->offsets.d3d9.present);
		present_ex_addr = get_offset_addr(d3d9_module,
				global_hook_info->offsets.d3d9.present_ex);
		present_swap_addr = get_offset_addr(d3d9_module,
				global_hook_info->offsets.d3d9.present_swap);
	} else {
		if (!dummy_window) {
			return false;
		}

		if (!manually_get_d3d9_addrs(d3d9_module,
					&present_addr,
					&present_ex_addr,
					&present_swap_addr)) {
			wxlog("Failed to get D3D9 values");
			return true;
		}
	}

	if (!present_addr && !present_ex_addr && !present_swap_addr) {
		wxlog("Invalid D3D9 values");
		return true;
	}

	if (present_swap_addr) {
		hook_init(&present_swap, present_swap_addr,
				(void*)hook_present_swap,
				"IDirect3DSwapChain9::Present");
		rehook(&present_swap);
	}
	if (present_ex_addr) {
		hook_init(&present_ex, present_ex_addr,
				(void*)hook_present_ex,
				"IDirect3DDevice9Ex::PresentEx");
		rehook(&present_ex);
	}
	if (present_addr) {
		hook_init(&present, present_addr,
				(void*)hook_present,
				"IDirect3DDevice9::Present");
		rehook(&present);
	}

	wxlog("Hooked D3D9");
	return true;
}
