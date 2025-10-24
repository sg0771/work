#ifdef _MSC_VER
#pragma warning(disable : 4214) /* nonstandard extension, non-int bitfield */
#pragma warning(disable : 4054) /* function pointer to data pointer */
#endif

#include <windows.h>

#define COBJMACROS
#include <dxgi.h>
#include <d3d11.h>

#include "gl-decs.h"
#include "graphics-hook.h"
#include "funchook.h"

#define DUMMY_WINDOW_CLASS_NAME L"graphics_hook_gl_dummy_window"


static struct func_hook swap_buffers;
static struct func_hook wgl_swap_layer_buffers;
static struct func_hook wgl_swap_buffers;
static struct func_hook wgl_delete_context;

static bool darkest_dungeon_fix = false;

#include "RGBData.h"

struct gl_data {
	HDC                    hdc = NULL;
	uint32_t               m_iWidth = 0;
	uint32_t               m_iHeight = 0;
	DXGI_FORMAT            format = DXGI_FORMAT_B8G8R8A8_UNORM;
	struct shmem_data      *shmem_info = NULL;
	int                    cur_tex = 0;
	bool                   texture_ready[NUM_BUFFERS] = { false };
	uint8_t *m_pBuf[NUM_BUFFERS] = { NULL };//内存数据
	uint8_t *m_pBufDraw = NULL;//内存数据
};

static HMODULE gl = NULL;
static bool nv_capture_available = false;
static struct gl_data data = {};

static inline bool gl_error(const char *func, const char *str)
{
	GLenum error = glGetError();
	if (error != 0) {
		wxlog("%s: %s: %lu", func, str, error);
		return true;
	}
	return false;
}

static void gl_free(void){
	capture_free();

	for (size_t i = 0; i < NUM_BUFFERS; i++) {
		if (data.m_pBuf[i]) {
			delete[]data.m_pBuf[i];
			data.m_pBuf[i] = NULL;
		}

	}
	if (data.m_pBufDraw) {
		delete[]data.m_pBufDraw;
		data.m_pBufDraw = NULL;
	}

	gl_error("gl_free", "GL error occurred on free");

	memset(&data, 0, sizeof(data));

	wxlog("------------------ gl capture freed ------------------");
}

static inline void *base_get_proc(const char *name)
{
	return (void*)GetProcAddress(gl, name);
}

static inline void *wgl_get_proc(const char *name)
{
	return (void*)jimglGetProcAddress(name);
}

static inline void *get_proc(const char *name)
{
	void *func = wgl_get_proc(name);
	if (!func)
		func = base_get_proc(name);

	return func;
}

static void init_nv_functions(void)
{
	jimglDXSetResourceShareHandleNV =
		(WGLSETRESOURCESHAREHANDLENVPROC)get_proc("wglDXSetResourceShareHandleNV");
	jimglDXOpenDeviceNV = (WGLDXOPENDEVICENVPROC)get_proc("wglDXOpenDeviceNV");
	jimglDXCloseDeviceNV = (WGLDXCLOSEDEVICENVPROC)get_proc("wglDXCloseDeviceNV");
	jimglDXRegisterObjectNV = (WGLDXREGISTEROBJECTNVPROC)get_proc("wglDXRegisterObjectNV");
	jimglDXUnregisterObjectNV = (WGLDXUNREGISTEROBJECTNVPROC)get_proc("wglDXUnregisterObjectNV");
	jimglDXObjectAccessNV = (WGLDXOBJECTACCESSNVPROC)get_proc("wglDXObjectAccessNV");
	jimglDXLockObjectsNV = (WGLDXLOCKOBJECTSNVPROC)get_proc("wglDXLockObjectsNV");
	jimglDXUnlockObjectsNV = (WGLDXUNLOCKOBJECTSNVPROC)get_proc("wglDXUnlockObjectsNV");

	nv_capture_available =
		!!jimglDXSetResourceShareHandleNV &&
		!!jimglDXOpenDeviceNV &&
		!!jimglDXCloseDeviceNV &&
		!!jimglDXRegisterObjectNV &&
		!!jimglDXUnregisterObjectNV &&
		!!jimglDXObjectAccessNV &&
		!!jimglDXLockObjectsNV &&
		!!jimglDXUnlockObjectsNV;

	if (nv_capture_available)
		wxlog("Shared-texture OpenGL capture available");
}

#define GET_PROC(cur_func, ptr, func, type) \
	do { \
		ptr = (type)get_proc(#func); \
		if (!ptr) { \
			wxlog("%s: failed to get function '%s'", #cur_func, \
					#func); \
			success = false; \
		} \
	} while (false)

static bool init_gl_functions(void)
{
	bool success = true;

	jimglGetProcAddress = (WGLGETPROCADDRESSPROC)base_get_proc("wglGetProcAddress");
	if (!jimglGetProcAddress) {
		wxlog("init_gl_functions: failed to get wglGetProcAddress");
		return false;
	}
	GET_PROC(init_gl_functions, glDrawPixels, glDrawPixels, PglDrawPixels);
	GET_PROC(init_gl_functions, glReadPixels, glReadPixels, PglReadPixels);
	GET_PROC(init_gl_functions, glColor3f, glColor3f, PglColor3f);
	GET_PROC(init_gl_functions, glVertex2i, glVertex2i, PglVertex2i);
	GET_PROC(init_gl_functions, glPopMatrix, glPopMatrix, PglPopMatrix);
	GET_PROC(init_gl_functions, glPopAttrib, glPopAttrib, PglPopAttrib);
	GET_PROC(init_gl_functions, glShadeModel, glShadeModel, PglShadeModel);
	GET_PROC(init_gl_functions, glActiveTextureARB, glActiveTextureARB, PglActiveTextureARB);
	GET_PROC(init_gl_functions, glDisable, glDisable, PglDisable);
	GET_PROC(init_gl_functions, glOrtho, glOrtho, PglOrtho);
	GET_PROC(init_gl_functions, glLoadIdentity, glLoadIdentity, PglLoadIdentity);
	GET_PROC(init_gl_functions, glPushMatrix, glPushMatrix, PglPushMatrix);
	GET_PROC(init_gl_functions, glMatrixMode, glMatrixMode, PglMatrixMode);
	GET_PROC(init_gl_functions, glViewport, glViewport, PglViewport);
	GET_PROC(init_gl_functions, glPushAttrib, glPushAttrib, PglPushAttrib);
	GET_PROC(init_gl_functions, glActiveTexture, glActiveTexture, PglActiveTexture);
	GET_PROC(init_gl_functions, glClearColor, glClearColor, PglClearColor);
	GET_PROC(init_gl_functions, glClear, glClear, PglClear);
	GET_PROC(init_gl_functions, glBegin, glBegin, PglBegin);
	GET_PROC(init_gl_functions, glTexCoord2f, glTexCoord2f, PglTexCoord2f);
	GET_PROC(init_gl_functions, glVertex2f, glVertex2f, PglVertex2f);
	GET_PROC(init_gl_functions, glEnd, glEnd, PglEnd);
	GET_PROC(init_gl_functions, glDrawArrays, glDrawArrays, PglDrawArrays);
	GET_PROC(init_gl_functions, jimglMakeCurrent, wglMakeCurrent, WGLMAKECURRENTPROC);
	GET_PROC(init_gl_functions, jimglGetCurrentDC, wglGetCurrentDC, WGLGETCURRENTDCPROC);
	GET_PROC(init_gl_functions, jimglGetCurrentContext,wglGetCurrentContext, WGLGETCURRENTCONTEXTPROC);
	GET_PROC(init_gl_functions, glTexImage2D, glTexImage2D, GLTEXIMAGE2DPROC);
	GET_PROC(init_gl_functions, glReadBuffer, glReadBuffer, GLREADBUFFERPROC);
	GET_PROC(init_gl_functions, glGetTexImage, glGetTexImage, GLGETTEXIMAGEPROC);
	GET_PROC(init_gl_functions, glDrawBuffer, glDrawBuffer, GLDRAWBUFFERPROC);
	GET_PROC(init_gl_functions, glGetError, glGetError, GLGETERRORPROC);
	GET_PROC(init_gl_functions, glBufferData, glBufferData, GLBUFFERDATAARBPROC);
	GET_PROC(init_gl_functions, glDeleteBuffers, glDeleteBuffers, GLDELETEBUFFERSARBPROC);
	GET_PROC(init_gl_functions, glDeleteTextures, glDeleteTextures, GLDELETETEXTURESPROC);
	GET_PROC(init_gl_functions, glGenBuffers, glGenBuffers, GLGENBUFFERSARBPROC);
	GET_PROC(init_gl_functions, glGenTextures, glGenTextures, GLGENTEXTURESPROC);
	GET_PROC(init_gl_functions, glMapBuffer, glMapBuffer, GLMAPBUFFERPROC);
	GET_PROC(init_gl_functions, glUnmapBuffer, glUnmapBuffer, GLUNMAPBUFFERPROC);
	GET_PROC(init_gl_functions, glBindBuffer, glBindBuffer, GLBINDBUFFERPROC);
	GET_PROC(init_gl_functions, glGetIntegerv, glGetIntegerv, GLGETINTEGERVPROC);
	GET_PROC(init_gl_functions, glBindTexture, glBindTexture, GLBINDTEXTUREPROC);
	GET_PROC(init_gl_functions, glGenFramebuffers, glGenFramebuffers, GLGENFRAMEBUFFERSPROC);
	GET_PROC(init_gl_functions, glDeleteFramebuffers, glDeleteFramebuffers, GLDELETEFRAMEBUFFERSPROC);
	GET_PROC(init_gl_functions, glBindFramebuffer, glBindFramebuffer, GLBINDFRAMEBUFFERPROC);
	GET_PROC(init_gl_functions, glBlitFramebuffer, glBlitFramebuffer, GLBLITFRAMEBUFFERPROC);
	GET_PROC(init_gl_functions, glFramebufferTexture2D,glFramebufferTexture2D, GLFRAMEBUFFERTEXTURE2DPROC);

	init_nv_functions();
	return success;
}

static void get_window_size(HDC hdc, uint32_t *m_iWidth, uint32_t *m_iHeight)
{
	HWND hwnd = WindowFromDC(hdc);
        RECT rc = {0};

	if (darkest_dungeon_fix) {
		*m_iWidth = 1920;
		*m_iHeight = 1080;
	} else {
		GetClientRect(hwnd, &rc);
		*m_iWidth = rc.right/4*4;
		*m_iHeight = rc.bottom/4*4;
	}
}


static bool gl_shmem_init(HWND window)
{
	if (!capture_init_shmem(&data.shmem_info, window,
				data.m_iWidth, data.m_iHeight,
				data.m_iWidth * 4, data.format, true)) {
		return false;
	}

	for(int i = 0 ; i < NUM_BUFFERS;i++) {
		data.m_pBuf[i] = new uint8_t[data.m_iWidth * data.m_iHeight * 4];
	}
	data.m_pBufDraw = new uint8_t[data.m_iWidth * data.m_iHeight * 4];


	wxlog("opengl memory capture successful  Size=%dx%d Pitch=%d Format=rgb32 ", data.m_iWidth, data.m_iHeight, data.m_iWidth * 4);
	return true;
}

#define INIT_SUCCESS         0
#define INIT_FAILED         -1
#define INIT_SHTEX_FAILED   -2

static int gl_init(HDC hdc)
{
	HWND window = WindowFromDC(hdc);
	int ret = INIT_FAILED;
	bool success = false;
	RECT rc = {0};

	if (darkest_dungeon_fix) {
		data.m_iWidth = 1920;
		data.m_iHeight = 1080;
	} else {
		GetClientRect(window, &rc);
		data.m_iWidth = rc.right/4*4;
		data.m_iHeight = rc.bottom/4*4;
	}

	data.hdc = hdc;
	data.format = DXGI_FORMAT_B8G8R8A8_UNORM;

	success = gl_shmem_init(window);
	if (!success)
		gl_free();
	else {
		global_hook_info->m_iType = (int)HOOK_TYPE_OPENGL;
		ret = INIT_SUCCESS;
	}
	return ret;
}


static void gl_shmem_capture(void)
{
	for (int i = 0; i < NUM_BUFFERS; i++) {
		if (data.texture_ready[i]) {
			data.texture_ready[i] = false;
			shmem_copy_data(i, data.m_pBuf[i]);
			break;
		}
	}

	int next_tex = (data.cur_tex == NUM_BUFFERS - 1) ? 0 : data.cur_tex + 1;

	glReadBuffer(GL_BACK);
	glReadPixels(0, 0, data.m_iWidth, data.m_iHeight, GL_BGRA, GL_UNSIGNED_BYTE, data.m_pBuf[next_tex]);//获取OPENGL当前数据
	WXCountFps();

	 {
		if (shmem_texture_data_lock(next_tex)) {
			shmem_texture_data_unlock(next_tex);
		}
		if (!global_hook_info->m_bCapture) {
			HookDrawImage((uint8_t*)data.m_pBuf[next_tex], data.m_iWidth, data.m_iHeight, data.m_iWidth * 4, 1, WX_RGB32);
			HookDrawString((uint8_t*)data.m_pBuf[next_tex], data.m_iWidth, data.m_iHeight, data.m_iWidth * 4, 1, WX_RGB32);
			HookDrawCamera((uint8_t*)data.m_pBuf[next_tex], data.m_iWidth, data.m_iHeight, data.m_iWidth * 4, 1, WX_RGB32);
			glDrawPixels(data.m_iWidth, data.m_iHeight, GL_BGRA, GL_UNSIGNED_BYTE, data.m_pBuf[next_tex]);//更新数据
		}
		data.texture_ready[next_tex] = true;
	}
}

static void gl_capture(HDC hdc)
{
	static bool functions_initialized = false;
	static bool critical_failure = false;

	if (critical_failure) {
		return;
	}

	if (!functions_initialized) {
		functions_initialized = init_gl_functions();
		if (!functions_initialized) {
			critical_failure = true;
			return;
		}
	}

	/* reset error flag */
	glGetError();

	if (capture_should_stop()) {
		gl_free();
		return;
	}
	if (capture_should_init()) {
		if (gl_init(hdc) == INIT_SHTEX_FAILED) {
			gl_init(hdc);
		}
	}
	if (capture_ready() && hdc == data.hdc) {
		uint32_t new_m_iWidth;
		uint32_t new_m_iHeight;

		/* reset capture if resized */
		get_window_size(hdc, &new_m_iWidth, &new_m_iHeight);
		if (new_m_iWidth != data.m_iWidth || new_m_iHeight != data.m_iHeight) {
			if (new_m_iWidth != 0 && new_m_iHeight != 0)
				gl_free();
			return;
		}
		gl_shmem_capture();
	}
}

static BOOL WINAPI hook_swap_buffers(HDC hdc)
{
	gl_capture(hdc);
	unhook(&swap_buffers);
	BOOL (WINAPI *call)(HDC) = (BOOL(WINAPI *)(HDC))swap_buffers.call_addr;
	BOOL ret = call(hdc);
	rehook(&swap_buffers);
	return ret;
}

static BOOL WINAPI hook_wgl_swap_buffers(HDC hdc)
{
	gl_capture(hdc);
	unhook(&wgl_swap_buffers);
	BOOL (WINAPI *call)(HDC) = (BOOL(WINAPI *)(HDC))wgl_swap_buffers.call_addr;
	BOOL ret = call(hdc);
	rehook(&wgl_swap_buffers);
	return ret;
}

static BOOL WINAPI hook_wgl_swap_layer_buffers(HDC hdc, UINT planes)
{
	gl_capture(hdc);
	unhook(&wgl_swap_layer_buffers);
	BOOL (WINAPI *call)(HDC, UINT) = (BOOL(WINAPI *)(HDC, UINT))wgl_swap_layer_buffers.call_addr;
	BOOL ret = call(hdc, planes);
	rehook(&wgl_swap_layer_buffers);
	return ret;
}

static BOOL WINAPI hook_wgl_delete_context(HGLRC hrc)
{
	BOOL ret;

	if (capture_active()) {
		HDC last_hdc = jimglGetCurrentDC();
		HGLRC last_hrc = jimglGetCurrentContext();

		jimglMakeCurrent(data.hdc, hrc);
		gl_free();
		jimglMakeCurrent(last_hdc, last_hrc);
	}

	unhook(&wgl_delete_context);
	BOOL (WINAPI *call)(HGLRC) = (BOOL(WINAPI *)(HGLRC))wgl_delete_context.call_addr;
	ret = call(hrc);
	rehook(&wgl_delete_context);

	return ret;
}

static bool gl_register_window(void)
{
	WNDCLASSW wc = {0};
	wc.style = CS_OWNDC;
	wc.hInstance = GetModuleHandle(NULL);
	wc.lpfnWndProc = DefWindowProc;
	wc.lpszClassName = DUMMY_WINDOW_CLASS_NAME;

	if (!RegisterClassW(&wc)) {
		wxlog("gl_register_window: failed to register window class: %d",
				GetLastError());
		return false;
	}

	return true;
}

bool hook_gl(void)
{
	global_hook_info->m_iType = (int)HOOK_TYPE_OPENGL;
	void *wgl_dc_proc;
	void *wgl_slb_proc;
	void *wgl_sb_proc;

	gl = get_system_module("opengl32.dll");
	if (!gl) {
		return false;
	}

	/* "life is feudal: your own" somehow uses both opengl and directx at
	 * the same time, so blacklist it from capturing opengl */
	const char *process_name = get_process_name();
	if (_strcmpi(process_name, "yo_cm_client.exe") == 0 ||
	    _strcmpi(process_name, "cm_client.exe")    == 0) {
		wxlog("Ignoring opengl for game: %s", process_name);
		return true;
	}

	if (!gl_register_window()) {
		return true;
	}

	wgl_dc_proc = base_get_proc("wglDeleteContext");
	wgl_slb_proc = base_get_proc("wglSwapLayerBuffers");
	wgl_sb_proc = base_get_proc("wglSwapBuffers");

	hook_init(&swap_buffers, SwapBuffers, hook_swap_buffers, "SwapBuffers");
	if (wgl_dc_proc) {
		hook_init(&wgl_delete_context, wgl_dc_proc,
				hook_wgl_delete_context,
				"wglDeleteContext");
		rehook(&wgl_delete_context);
	}
	if (wgl_slb_proc) {
		hook_init(&wgl_swap_layer_buffers, wgl_slb_proc,
				hook_wgl_swap_layer_buffers,
				"wglSwapLayerBuffers");
		rehook(&wgl_swap_layer_buffers);
	}
	if (wgl_sb_proc) {
		hook_init(&wgl_swap_buffers, wgl_sb_proc,
				hook_wgl_swap_buffers,
				"wglSwapBuffers");
		rehook(&wgl_swap_buffers);
	}

	rehook(&swap_buffers);
	return true;
}
