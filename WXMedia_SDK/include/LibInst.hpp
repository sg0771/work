/*
�����������
*/
#ifndef  _LibInst_H
#define  _LibInst_H

#include <Windows.h>
#include <WinUser.h>
#include <winnt.h>
#include <memory>
#include <d3d9.h>
#include <d3dx9/d3dx9.h>
#include <d3d11.h>
#include <dxgi.h>
#include <strmif.h>
#include <atlbase.h>
#include <dvdmedia.h>
#include <amvideo.h>
#include <control.h>
#include <uuids.h>
#include <ks.h>
#include <ksmedia.h>
#include <ddraw.h>
#include <dbghelp.h>
#include <mmsystem.h>
#include <dsound.h>
#include <Windows.Graphics.Capture.Interop.h>
#include <Windows.graphics.directx.direct3d11.interop.h>
#include <winrt/Windows.Foundation.Metadata.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.System.h>
#include <wrl/client.h>
#include <WXLog.h>
#include <WXBase.h>

// Build a perspective projection matrix. (left-handed)
typedef D3DXMATRIX* (WINAPI* cbD3DXMatrixPerspectiveFovLH
    )(D3DXMATRIX* pOut, FLOAT fovy, FLOAT Aspect, FLOAT zn, FLOAT zf);

typedef HRESULT(WINAPI* cbD3DXGetShaderConstantTable)(CONST DWORD* pFunction, LPD3DXCONSTANTTABLE* ppConstantTable);

typedef HRESULT(WINAPI* cbD3DXCreateTexture)(
    LPDIRECT3DDEVICE9     pDevice,
    UINT           Width,
    UINT           Height,
    UINT           MipLevels,
    DWORD           Usage,
    D3DFORMAT         Format,
    D3DPOOL          Pool,
    LPDIRECT3DTEXTURE9* ppTexture);

typedef HRESULT(WINAPI* cbD3DXCreateVolumeTexture)(
    LPDIRECT3DDEVICE9     pDevice,
    UINT           Width,
    UINT           Height,
    UINT           Depth,
    UINT           MipLevels,
    DWORD           Usage,
    D3DFORMAT         Format,
    D3DPOOL          Pool,
    LPDIRECT3DVOLUMETEXTURE9* ppVolumeTexture);

typedef HRESULT(WINAPI* cbD3DXCompileShader)(
    LPCSTR             pSrcData,
    UINT              SrcDataLen,
    CONST D3DXMACRO* pDefines,
    LPD3DXINCLUDE          pInclude,
    LPCSTR             pFunctionName,
    LPCSTR             pProfile,
    DWORD              Flags,
    LPD3DXBUFFER* ppShader,
    LPD3DXBUFFER* ppErrorMsgs,
    LPD3DXCONSTANTTABLE* ppConstantTable);

typedef HRESULT(WINAPI* cbD3DXCreateFontIndirectW)(
    LPDIRECT3DDEVICE9    pDevice,
    CONST D3DXFONT_DESCW* pDesc,
    LPD3DXFONT* ppFont);

typedef HRESULT(WINAPI* cbDirectDrawCreate)(GUID FAR* lpGUID, LPDIRECTDRAW FAR* lplpDD, IUnknown FAR* pUnkOuter);


typedef IDirect3D9* (WINAPI* cbDirect3DCreate9)(UINT SDKVersion);
typedef HRESULT(WINAPI* cbDirect3DCreate9Ex)(UINT SDKVersion, IDirect3D9Ex**);

typedef HRESULT(WINAPI* cbCreateDXGIFactory1)(REFIID riid, _COM_Outptr_ void** ppFactory);

typedef HRESULT(WINAPI* cbD3D11CreateDevice)(
    _In_opt_ IDXGIAdapter* pAdapter,
    D3D_DRIVER_TYPE DriverType,
    HMODULE Software,
    UINT Flags,
    _In_reads_opt_(FeatureLevels) CONST D3D_FEATURE_LEVEL* pFeatureLevels,
    UINT FeatureLevels,
    UINT SDKVersion,
    _COM_Outptr_opt_ ID3D11Device** ppDevice,
    _Out_opt_ D3D_FEATURE_LEVEL* pFeatureLevel,
    _COM_Outptr_opt_ ID3D11DeviceContext** ppImmediateContext);


typedef HRESULT(WINAPI* cbCreateDirect3D11DeviceFromDXGIDevice)(
    _In_         IDXGIDevice* dxgiDevice,
    _COM_Outptr_ IInspectable** graphicsDevice);

typedef HRESULT(WINAPI* cbDirectSoundCreate8)(LPCGUID pcGuidDevice, LPDIRECTSOUND8* ppDS8, LPUNKNOWN pUnkOuter);

//Version.dll

typedef DWORD(WINAPI* cbGetFileVersionInfoSizeW)(LPCWSTR lptstrFilename, LPDWORD lpdwHandle);

typedef BOOL(APIENTRY* cbGetFileVersionInfoW)(
    _In_        LPCWSTR lptstrFilename, /* Filename of version stamped file */
    _Reserved_     DWORD dwHandle,     /* Information from GetFileVersionSize */
    _In_        DWORD dwLen,       /* Length of buffer for info */
    _Out_writes_bytes_(dwLen) LPVOID lpData      /* Buffer to place the data structure */
    );

typedef BOOL(APIENTRY* cbVerQueryValueW)(
    _In_ LPCVOID pBlock,
    _In_ LPCWSTR lpSubBlock,
    _Outptr_result_buffer_(_Inexpressible_("buffer can be PWSTR or DWORD*")) LPVOID* lplpBuffer,
    _Out_ PUINT puLen
    );


//dbghelp.dll

typedef BOOL(WINAPI* cbMakeSureDirectoryPathExists)(PCSTR DirPath);

typedef BOOL(WINAPI* cbMiniDumpWriteDump)(
    _In_ HANDLE hProcess,
    _In_ DWORD ProcessId,
    _In_ HANDLE hFile,
    _In_ MINIDUMP_TYPE DumpType,
    _In_opt_ PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
    _In_opt_ PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
    _In_opt_ PMINIDUMP_CALLBACK_INFORMATION CallbackParam
    );


//Humanseg.dll 
// MNN+humanseg
typedef void*  (*funcHumansegCreate)(int width, int height); //��������
typedef int    (*funcHumansegDetect)(void* ptr, uint8_t* pData, int pitch);//���ܿ���
typedef void   (*funcHumansegDestroy)(void* ptr);//���ٶ���
typedef int    (*funcWXSupportVulkan)();//�жϱ����Ƿ�֧��vulkan

#include <dxva2api.h>
typedef HRESULT(WINAPI* cbDXVA2CreateDirect3DDeviceManager9)(
    UINT* pResetToken, IDirect3DDeviceManager9** ppDeviceManager
);

//WinRT API
typedef BOOL(WINAPI* PFNRoOriginateLanguageException)(int32_t, void*, void*);
typedef BOOL(WINAPI* PFNRoGetActivationFactory)(void*, winrt::guid const&, void**);

//ntdll.dll
typedef LONG(WINAPI* PFNRtlGetVersion)(PRTL_OSVERSIONINFOW);
typedef void(WINAPI* PFNRtlGetNtVersionNumbers)(DWORD*, DWORD*, DWORD*);
typedef LONG(WINAPI* PFNRtlVerifyVersionInfo)(OSVERSIONINFOEXW*, ULONG, ULONGLONG);

//shcore.dll
#ifndef DPI_ENUMS_DECLARED
typedef enum
{
    PROCESS_DPI_UNAWARE = 0,
    PROCESS_SYSTEM_DPI_AWARE = 1,
    PROCESS_PER_MONITOR_DPI_AWARE = 2
} PROCESS_DPI_AWARENESS;
typedef enum
{
    MDT_EFFECTIVE_DPI = 0,
    MDT_ANGULAR_DPI = 1,
    MDT_RAW_DPI = 2,
    MDT_DEFAULT = MDT_EFFECTIVE_DPI
} MONITOR_DPI_TYPE;
#endif /*DPI_ENUMS_DECLARED*/
typedef HRESULT(WINAPI* PFN_SetProcessDpiAwareness)(PROCESS_DPI_AWARENESS);
typedef HRESULT(WINAPI* PFN_GetDpiForMonitor)(HMONITOR, MONITOR_DPI_TYPE, UINT*, UINT*);

//user32.dll
typedef BOOL(WINAPI* PFN_SetProcessDPIAware)(void);


typedef struct wxtagCHANGEFILTERSTRUCT {
    DWORD cbSize;
    DWORD ExtStatus;
} WXCHANGEFILTERSTRUCT, * WXPCHANGEFILTERSTRUCT;
typedef BOOL(WINAPI* PFN_ChangeWindowMessageFilterEx)(HWND, UINT, DWORD, WXPCHANGEFILTERSTRUCT);
typedef BOOL(WINAPI* PFN_EnableNonClientDpiScaling)(HWND);
typedef BOOL(WINAPI* PFN_SetProcessDpiAwarenessContext)(HANDLE);
typedef UINT(WINAPI* PFN_GetDpiForWindow)(HWND);
typedef BOOL(WINAPI* PFN_AdjustWindowRectExForDpi)(LPRECT, DWORD, BOOL, DWORD, UINT);
typedef int (WINAPI* PFN_GetSystemMetricsForDpi)(int, UINT);

//dwmapi.dll
#include <dwmapi.h>
typedef HRESULT(WINAPI* PFN_DwmIsCompositionEnabled)(BOOL*);
typedef HRESULT(WINAPI* PFN_DwmFlush)(VOID);
typedef HRESULT(WINAPI* PFN_DwmEnableBlurBehindWindow)(HWND, const DWM_BLURBEHIND*);
typedef HRESULT(WINAPI* PFN_DwmGetColorizationColor)(DWORD*, BOOL*);

typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef signed char GLbyte;
typedef short GLshort;
typedef int GLint;
typedef int GLsizei;
typedef unsigned char GLubyte;
typedef unsigned short GLushort;
typedef unsigned int GLuint;
typedef float GLfloat;
typedef float GLclampf;
typedef double GLdouble;
typedef double GLclampd;
typedef void GLvoid;

typedef void (APIENTRY* PFNGLCLEARPROC)(GLbitfield);//glClear
typedef const GLubyte* (APIENTRY* PFNGLGETSTRINGPROC)(GLenum);//GetString
typedef void (APIENTRY* PFNGLGETINTEGERVPROC)(GLenum, GLint*);//GetIntegerv
typedef const GLubyte* (APIENTRY* PFNGLGETSTRINGIPROC)(GLenum, GLuint);//glGetStringi

typedef HGLRC(WINAPI* PFN_wglCreateContext)(HDC);
typedef BOOL(WINAPI* PFN_wglDeleteContext)(HGLRC);
typedef PROC(WINAPI* PFN_wglGetProcAddress)(LPCSTR);
typedef HDC(WINAPI* PFN_wglGetCurrentDC)(void);
typedef HGLRC(WINAPI* PFN_wglGetCurrentContext)(void);
typedef BOOL(WINAPI* PFN_wglMakeCurrent)(HDC, HGLRC);
typedef BOOL(WINAPI* PFN_wglShareLists)(HGLRC, HGLRC);

typedef BOOL(WINAPI* PFNWGLSWAPINTERVALEXTPROC)(int);
typedef BOOL(WINAPI* PFNWGLGETPIXELFORMATATTRIBIVARBPROC)(HDC, int, int, UINT, const int*, int*);
typedef const char* (WINAPI* PFNWGLGETEXTENSIONSSTRINGEXTPROC)(void);
typedef const char* (WINAPI* PFNWGLGETEXTENSIONSSTRINGARBPROC)(HDC);
typedef HGLRC(WINAPI* PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC, HGLRC, const int*);

typedef void (WINAPI* PFNglTexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels);

typedef void (WINAPI* PFNglDrawBuffer)(GLenum mode);
typedef void (WINAPI* PFNglDrawArrays)(GLenum mode, GLint first, GLsizei count);
typedef void (WINAPI* PFNglDisable)(GLenum cap);
typedef void (WINAPI* PFNglDrawElements)(GLenum mode, GLsizei count, GLenum type, const void* indices);
typedef void (WINAPI* PFNglFinish)(void);
typedef void (WINAPI* PFNglFlush)(void);
typedef void (WINAPI* PFNglReadBuffer)(GLenum mode);
typedef void (WINAPI* PFNglReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void* pixels);
typedef void (WINAPI* PFNglTexParameteri)(GLenum target, GLenum pname, GLint param);
typedef void (WINAPI* PFNglTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels);
typedef void (WINAPI* PFNglViewport)(GLint x, GLint y, GLsizei width, GLsizei height);

typedef void (WINAPI* PFNglBindTexture)(GLenum target, GLuint texture);
typedef void (WINAPI* PFNglCopyTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
typedef void (WINAPI* PFNglDrawBuffer)(GLenum mode);
typedef GLenum(WINAPI* PFNglGetError)(void);
typedef void (WINAPI* PFNglDeleteTextures)(GLsizei n, const GLuint* textures);
typedef void (WINAPI* PFNglGenTextures)(GLsizei n, GLuint* textures);
typedef void (WINAPI* PFNglBlendFunc)(GLenum sfactor, GLenum dfactor);
typedef void (WINAPI* PFNglEnable)(GLenum cap);

typedef void (WINAPI* PFNglClearColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
typedef void (WINAPI* PFNglGetFloatv)(GLenum pname, GLfloat* params);
typedef void (WINAPI* PFNglTexParameterfv)(GLenum target, GLenum pname, const GLfloat* params);


//ole32.dll
//com init
typedef HRESULT (WINAPI* PFNCoInitializeEx)(LPVOID pvReserved,DWORD dwCoInit);

class MyLib {
public:
    bool Enabled() const
    {
        return this->m_handle != nullptr;
    }
    MyLib(const wchar_t* filename)
    {
        this->m_handle = this->OpenLibrary(filename);
    }

    virtual ~MyLib()
    {
        if (this->m_handle != nullptr) {
            this->CloseLibrary();
        }
    }

    void* GetFunction(const std::string& symbol_name)
    {
        if (this->m_handle == nullptr)
            return nullptr;
        return this->GetSymbol(symbol_name);
    }
    HMODULE Ptr() const
    {
        return this->m_handle;
    }

    std::wstring GetName() {
        return m_strName;
    }
private:
    HMODULE OpenLibrary(const wchar_t* filename)
    {
        m_handle = ::LoadLibraryW(filename);

        if (m_handle == nullptr) {
            wchar_t dllPath[MAX_PATH];
            GetModuleFileNameW(WXBase::wxGetSelfModuleHandle()/*NULL*/, dllPath, MAX_PATH); //DLL·��
            std::wstring strDllPath = std::wstring(dllPath);
            int found = strDllPath.find_last_of(L"/\\") + 1;
            std::wstring strDllDir = strDllPath.substr(0, found);

            std::wstring strDll = strDllDir + filename;
            m_handle = LoadLibraryW(strDll.c_str());
        }
        if (m_handle)
        {
            WXLogW(L"LoadLibrary %ws OK", filename);
            m_strName = filename;
        }
        else {
            WXLogW(L"LoadLibrary %ws Error", filename);
        }
        return m_handle;
    }

    void  CloseLibrary() {
        if (m_handle) {
            ::FreeLibrary(m_handle);
            m_handle = nullptr;
        }
    }

    void* GetSymbol(const std::string& symbol_name)
    {
        void* p = reinterpret_cast<void*>(::GetProcAddress(m_handle, symbol_name.c_str()));
        return p;
    }
    HMODULE m_handle = nullptr;
    std::wstring m_strName = L"";

};

class LibInst {

    std::vector<MyLib*>m_arrLibs;
public:
    std::shared_ptr<MyLib> m_libOLE32 = nullptr;//ole32.dll
    PFNCoInitializeEx m_CoInitializeEx = nullptr;
    void LoadOLE32() {
        m_libOLE32 = std::shared_ptr<MyLib>(new MyLib(L"ole32.dll"));
        if (m_libOLE32->Enabled()) {
            m_arrLibs.push_back(m_libOLE32.get());
            m_CoInitializeEx = (PFNCoInitializeEx)m_libOLE32->GetFunction("CoInitializeEx");
            m_CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY);
        }
        else {
            m_libOLE32 = nullptr;
        }
    }

    std::shared_ptr<MyLib> m_libOpengl32 = nullptr;//opengl32.dll
	PFNGLCLEARPROC m_glClear = nullptr;
	PFNGLGETSTRINGPROC m_glGetString = nullptr;
	PFNGLGETINTEGERVPROC m_glGetIntegerv = nullptr;
	PFNGLGETSTRINGIPROC m_glGetStringi = nullptr;

    PFN_wglCreateContext m_wglCreateContext = nullptr;
	PFN_wglDeleteContext m_wglDeleteContext = nullptr;
    PFN_wglGetProcAddress m_wglGetProcAddress = nullptr;
    PFN_wglGetCurrentDC m_wglGetCurrentDC = nullptr;
    PFN_wglGetCurrentContext m_wglGetCurrentContext = nullptr;
    PFN_wglMakeCurrent m_wglMakeCurrent = nullptr;
    PFN_wglShareLists m_wglShareLists = nullptr;

	PFNWGLSWAPINTERVALEXTPROC m_wglSwapIntervalEXT = nullptr;
	PFNWGLGETPIXELFORMATATTRIBIVARBPROC m_wglGetPixelFormatAttribivARB = nullptr;
	PFNWGLGETEXTENSIONSSTRINGEXTPROC m_wglGetExtensionsStringEXT = nullptr;
	PFNWGLGETEXTENSIONSSTRINGARBPROC m_wglGetExtensionsStringARB = nullptr;
	PFNWGLCREATECONTEXTATTRIBSARBPROC m_wglCreateContextAttribsARB = nullptr;

	PFNglTexImage2D m_glTexImage2D = nullptr;
	PFNglDrawBuffer m_glDrawBuffer = nullptr;
	PFNglDrawArrays m_glDrawArrays = nullptr;
	PFNglDisable m_glDisable = nullptr;
	PFNglDrawElements m_glDrawElements = nullptr;
	PFNglFinish m_glFinish = nullptr;
	PFNglFlush m_glFlush = nullptr;
	PFNglReadBuffer m_glReadBuffer = nullptr;
	PFNglReadPixels m_glReadPixels = nullptr;
	PFNglTexParameteri m_glTexParameteri = nullptr;
	PFNglTexSubImage2D m_glTexSubImage2D = nullptr;
	PFNglViewport m_glViewport = nullptr;
	PFNglBindTexture m_glBindTexture = nullptr;
	PFNglCopyTexSubImage2D m_glCopyTexSubImage2D = nullptr;
	PFNglGetError m_glGetError = nullptr;
	PFNglDeleteTextures m_glDeleteTextures = nullptr;
	PFNglGenTextures m_glGenTextures = nullptr;
	PFNglBlendFunc m_glBlendFunc = nullptr;
	PFNglEnable m_glEnable = nullptr;
	PFNglClearColor m_glClearColor = nullptr;
	PFNglGetFloatv m_glGetFloatv = nullptr;
	PFNglTexParameterfv m_glTexParameterfv = nullptr;


    void LoadOpenGLFunctions() {
        m_libOpengl32 = std::shared_ptr<MyLib>(new MyLib(L"opengl32.dll"));
        if (m_libOpengl32->Enabled()) {
            m_arrLibs.push_back(m_libOpengl32.get());
            m_glClear = (PFNGLCLEARPROC)m_libOpengl32->GetFunction("glClear");
            m_glGetString = (PFNGLGETSTRINGPROC)m_libOpengl32->GetFunction("glGetString");
            m_glGetIntegerv = (PFNGLGETINTEGERVPROC)m_libOpengl32->GetFunction("glGetIntegerv");
            m_glGetStringi = (PFNGLGETSTRINGIPROC)m_libOpengl32->GetFunction("glGetStringi");

            m_wglCreateContext = (PFN_wglCreateContext)m_libOpengl32->GetFunction("wglCreateContext");
            m_wglDeleteContext = (PFN_wglDeleteContext)m_libOpengl32->GetFunction("wglDeleteContext");
            m_wglGetProcAddress = (PFN_wglGetProcAddress)m_libOpengl32->GetFunction("wglGetProcAddress");
            m_wglGetCurrentDC = (PFN_wglGetCurrentDC)m_libOpengl32->GetFunction("wglGetCurrentDC");
            m_wglGetCurrentContext = (PFN_wglGetCurrentContext)m_libOpengl32->GetFunction("wglGetCurrentContext");
            m_wglMakeCurrent = (PFN_wglMakeCurrent)m_libOpengl32->GetFunction("wglMakeCurrent");
            m_wglShareLists = (PFN_wglShareLists)m_libOpengl32->GetFunction("wglShareLists");

            m_wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)m_libOpengl32->GetFunction("wglSwapIntervalEXT");
            m_wglGetPixelFormatAttribivARB = (PFNWGLGETPIXELFORMATATTRIBIVARBPROC)m_libOpengl32->GetFunction("wglGetPixelFormatAttribivARB");
            m_wglGetExtensionsStringEXT = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)m_libOpengl32->GetFunction("wglGetExtensionsStringEXT");
            m_wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)m_libOpengl32->GetFunction("wglGetExtensionsStringARB");
            m_wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)m_libOpengl32->GetFunction("wglCreateContextAttribsARB");

            m_glTexImage2D = (PFNglTexImage2D)m_libOpengl32->GetFunction("glTexImage2D");
            m_glDrawBuffer = (PFNglDrawBuffer)m_libOpengl32->GetFunction("glDrawBuffer");
            m_glDrawArrays = (PFNglDrawArrays)m_libOpengl32->GetFunction("glDrawArrays");
            m_glDisable = (PFNglDisable)m_libOpengl32->GetFunction("glDisable");
            m_glDrawElements = (PFNglDrawElements)m_libOpengl32->GetFunction("glDrawElements");
            m_glFinish = (PFNglFinish)m_libOpengl32->GetFunction("glFinish");
            m_glFlush = (PFNglFlush)m_libOpengl32->GetFunction("glFlush");
            m_glReadBuffer = (PFNglReadBuffer)m_libOpengl32->GetFunction("glReadBuffer");
            m_glReadPixels = (PFNglReadPixels)m_libOpengl32->GetFunction("glReadPixels");
            m_glTexParameteri = (PFNglTexParameteri)m_libOpengl32->GetFunction("glTexParameteri");
            m_glTexSubImage2D = (PFNglTexSubImage2D)m_libOpengl32->GetFunction("glTexSubImage2D");
            m_glViewport = (PFNglViewport)m_libOpengl32->GetFunction("glViewport");
            m_glBindTexture = (PFNglBindTexture)m_libOpengl32->GetFunction("glBindTexture");
            m_glCopyTexSubImage2D = (PFNglCopyTexSubImage2D)m_libOpengl32->GetFunction("glCopyTexSubImage2D");
            m_glGetError = (PFNglGetError)m_libOpengl32->GetFunction("glGetError");
            m_glDeleteTextures = (PFNglDeleteTextures)m_libOpengl32->GetFunction("glDeleteTextures");
            m_glGenTextures = (PFNglGenTextures)m_libOpengl32->GetFunction("glGenTextures");
            m_glBlendFunc = (PFNglBlendFunc)m_libOpengl32->GetFunction("glBlendFunc");
            m_glEnable = (PFNglEnable)m_libOpengl32->GetFunction("glEnable");
            m_glClearColor = (PFNglClearColor)m_libOpengl32->GetFunction("glClearColor");
            m_glGetFloatv = (PFNglGetFloatv)m_libOpengl32->GetFunction("glGetFloatv");
            m_glTexParameterfv = (PFNglTexParameterfv)m_libOpengl32->GetFunction("glTexParameterfv");

        }
        else
        {
            m_libOpengl32 = nullptr;
        }
    }

    std::shared_ptr<MyLib> m_libUser32 = nullptr;//user32.dll
    PFN_SetProcessDPIAware          m_SetProcessDPIAware = nullptr;
    PFN_ChangeWindowMessageFilterEx m_ChangeWindowMessageFilterEx = nullptr;
    PFN_EnableNonClientDpiScaling   m_EnableNonClientDpiScaling = nullptr;
    PFN_SetProcessDpiAwarenessContext m_SetProcessDpiAwarenessContext = nullptr;
    PFN_GetDpiForWindow             m_GetDpiForWindow = nullptr;
    PFN_AdjustWindowRectExForDpi    m_AdjustWindowRectExForDpi = nullptr;
    PFN_GetSystemMetricsForDpi      m_GetSystemMetricsForDpi = nullptr;

    std::shared_ptr<MyLib> m_libDwmapi = nullptr;//dwmapi.dll
    PFN_DwmIsCompositionEnabled     m_DwmIsCompositionEnabled = nullptr;
    PFN_DwmFlush                    m_DwmFlush = nullptr;
    PFN_DwmEnableBlurBehindWindow   m_DwmEnableBlurBehindWindow = nullptr;
    PFN_DwmGetColorizationColor     m_DwmGetColorizationColor = nullptr;

    std::shared_ptr<MyLib> m_libShcore = nullptr;//shcore.dll
    PFN_SetProcessDpiAwareness m_SetProcessDpiAwareness = nullptr;
    PFN_GetDpiForMonitor m_GetDpiForMonitor = nullptr;

	std::shared_ptr<MyLib> m_libCombase = nullptr;//WinRT API
    PFNRoOriginateLanguageException m_RoOriginateLanguageException = nullptr;
	PFNRoGetActivationFactory m_RoGetActivationFactory = nullptr;

    std::shared_ptr<MyLib> m_libHumanseg = nullptr;//Humanseg.dll
    funcHumansegCreate  mHumansegCreate = nullptr;
    funcHumansegDetect  mHumansegDetect = nullptr;
    funcHumansegDestroy mHumansegDestroy = nullptr;
    funcWXSupportVulkan   mWXSupportVulkan = nullptr;

    //ApowerRec 
    //ϵͳDLL .Net C++
    std::shared_ptr<MyLib> m_libWPFGFX = nullptr;//("wpfgfx_v0400.dll");

    //DXSDK
    std::shared_ptr<MyLib> m_libD3DCOMPILER = nullptr;//("d3dcompiler_47.dll");
    std::shared_ptr<MyLib>m_libWINDOWSCODECS = nullptr;//("WINDOWSCODECS.dll");

    //VC++ 
    std::shared_ptr<MyLib> m_libVCRUNTIME140_CLR0400 = nullptr;//("VCRUNTIME140_CLR0400.dll");
    std::shared_ptr<MyLib> m_libUCRTBASE_CLR0400 = nullptr;//("UCRTBASE_CLR0400.dll");
    std::shared_ptr<MyLib> m_libMSVCP140_CLR0400 = nullptr;// ("MSVCP140_CLR0400.dll");
    
    std::shared_ptr<MyLib> m_libNTDLL = nullptr;
    PFNRtlGetVersion m_RtlGetVersion = nullptr;
    PFNRtlGetNtVersionNumbers m_RtlGetNtVersionNumbers = nullptr;
	PFNRtlVerifyVersionInfo m_RtlVerifyVersionInfo = nullptr;

    std::shared_ptr<MyLib> m_libDbghelp = nullptr;//("dbghelp.dll");
    cbMiniDumpWriteDump mMiniDumpWriteDump = nullptr;
    cbMakeSureDirectoryPathExists mMakeSureDirectoryPathExists = nullptr;

    std::shared_ptr<MyLib> m_libVersion = nullptr;// ("version.dll");
    cbGetFileVersionInfoSizeW mGetFileVersionInfoSizeW = nullptr;
    cbGetFileVersionInfoW mGetFileVersionInfoW = nullptr;
    cbVerQueryValueW mVerQueryValueW = nullptr;

    std::shared_ptr<MyLib> m_libDSound = nullptr;//("dsound.dll");
    cbDirectSoundCreate8 mDirectSoundCreate8 = nullptr;

    std::shared_ptr<MyLib> m_libD3D9 = nullptr;//("d3d9.dll");
    cbDirect3DCreate9 mDirect3DCreate9 = nullptr;
    cbDirect3DCreate9Ex mDirect3DCreate9Ex = nullptr;

    std::shared_ptr<MyLib> m_libD3DX9 = nullptr;//("d3dx9_43.dll");
    cbD3DXCompileShader mD3DXCompileShader = nullptr;
    cbD3DXCreateFontIndirectW mD3DXCreateFontIndirectW = nullptr;
    cbD3DXMatrixPerspectiveFovLH mD3DXMatrixPerspectiveFovLH = nullptr;
    cbD3DXGetShaderConstantTable mD3DXGetShaderConstantTable = nullptr;
    cbD3DXCreateTexture mD3DXCreateTexture = nullptr;
    cbD3DXCreateVolumeTexture mD3DXCreateVolumeTexture = nullptr;

    std::shared_ptr<MyLib> m_libD3D11 = nullptr;//("d3d11.dll");
    cbD3D11CreateDevice mD3D11CreateDevice = nullptr;
    cbCreateDirect3D11DeviceFromDXGIDevice mCreateDirect3D11DeviceFromDXGIDevice = nullptr;

    std::shared_ptr<MyLib> m_libDXGI = nullptr;//("dxgi.dll");
    cbCreateDXGIFactory1 mCreateDXGIFactory1 = nullptr;

    std::shared_ptr<MyLib> m_libDXVA2 = nullptr;//("dxva2.dll");
    cbDXVA2CreateDirect3DDeviceManager9 mDXVA2CreateDirect3DDeviceManager9 = nullptr;


    std::shared_ptr<MyLib> m_libDraw = nullptr;//("ddraw.dll");
    cbDirectDrawCreate mDirectDrawCreate = nullptr;
public:
    static LibInst& GetInst(){
        static LibInst s_inst;
        return s_inst;
    }
    void LogMsg() {
        for (auto obj : m_arrLibs)
        {
            WXLogW(L"LoadLibrary %ws Init", obj->GetName().c_str());
        }
    }

    LibInst() {
        LoadOLE32();//COM 

        LoadOpenGLFunctions();//OpenGL

        m_libCombase = std::shared_ptr<MyLib>(new MyLib(L"combase.dll"));
        if (m_libCombase->Enabled()) {
            m_arrLibs.push_back(m_libCombase.get());
            m_RoOriginateLanguageException = (PFNRoOriginateLanguageException)m_libCombase->GetFunction("RoOriginateLanguageException");
            m_RoGetActivationFactory = (PFNRoGetActivationFactory)m_libCombase->GetFunction("RoGetActivationFactory");
        }
        else {
            m_libCombase = nullptr;
        }

        //MNN Humanseg
        m_libHumanseg = std::shared_ptr<MyLib>(new MyLib(L"Humanseg.dll"));
        if (m_libHumanseg->Enabled()) {
            m_arrLibs.push_back(m_libHumanseg.get());
            mHumansegCreate = (funcHumansegCreate)m_libHumanseg->GetFunction("HumansegCreate");
            mHumansegDetect = (funcHumansegDetect)m_libHumanseg->GetFunction("HumansegDetect");
            mHumansegDestroy = (funcHumansegDestroy)m_libHumanseg->GetFunction("HumansegDestroy");
            mWXSupportVulkan = (funcWXSupportVulkan)m_libHumanseg->GetFunction("WXSupportVulkan");
        }
        else {
            m_libHumanseg = nullptr;
        }

        m_libD3D9 = std::shared_ptr<MyLib>(new MyLib(L"d3d9.dll"));
        if (m_libD3D9->Enabled()) {
            //D3D9.dll
            mDirect3DCreate9 = (cbDirect3DCreate9)m_libD3D9->GetFunction("Direct3DCreate9");
            mDirect3DCreate9Ex = (cbDirect3DCreate9Ex)m_libD3D9->GetFunction("Direct3DCreate9Ex");

            CComPtr<IDirect3D9Ex> pD3DEx = nullptr;
            HRESULT hr = mDirect3DCreate9Ex(D3D_SDK_VERSION, &pD3DEx);
            if (FAILED(hr)) {
                mDirect3DCreate9 = nullptr;
                mDirect3DCreate9Ex = nullptr;
                m_libD3D9 = nullptr;
            }
            else {
                m_arrLibs.push_back(m_libD3D9.get());
            }
        }

        if (m_libD3D9) { //D3DX9
            m_libD3DX9 = std::shared_ptr<MyLib>(new MyLib(L"d3dx9_43.dll"));
            //D3DX9.dll
            if (m_libD3DX9->Enabled()) {
                m_arrLibs.push_back(m_libD3DX9.get());
                mD3DXCompileShader = (cbD3DXCompileShader)m_libD3DX9->GetFunction("D3DXCompileShader");
                mD3DXCreateFontIndirectW = (cbD3DXCreateFontIndirectW)m_libD3DX9->GetFunction("D3DXCreateFontIndirectW");

                mD3DXMatrixPerspectiveFovLH = (cbD3DXMatrixPerspectiveFovLH)m_libD3DX9->GetFunction("D3DXMatrixPerspectiveFovLH");
                mD3DXGetShaderConstantTable = (cbD3DXGetShaderConstantTable)m_libD3DX9->GetFunction("D3DXGetShaderConstantTable");
                mD3DXCreateTexture = (cbD3DXCreateTexture)m_libD3DX9->GetFunction("D3DXCreateTexture");
                mD3DXCreateVolumeTexture = (cbD3DXCreateVolumeTexture)m_libD3DX9->GetFunction("D3DXCreateVolumeTexture");
            }
            else {
                m_libD3DX9 = nullptr;
            }
            m_libD3D11 = std::shared_ptr<MyLib>(new MyLib(L"d3d11.dll"));
            if (m_libD3D11->Enabled()) {
                m_arrLibs.push_back(m_libD3D11.get());
                mD3D11CreateDevice = (cbD3D11CreateDevice)m_libD3D11->GetFunction("D3D11CreateDevice");
                mCreateDirect3D11DeviceFromDXGIDevice = (cbCreateDirect3D11DeviceFromDXGIDevice)m_libD3D11->GetFunction("CreateDirect3D11DeviceFromDXGIDevice");
            }
            else {
                m_libD3D11 = nullptr;
            }

            m_libDXGI = std::shared_ptr<MyLib>(new MyLib(L"dxgi.dll"));
            if (m_libDXGI->Enabled()) {
                m_arrLibs.push_back(m_libDXGI.get());
                mCreateDXGIFactory1 = (cbCreateDXGIFactory1)m_libDXGI->GetFunction("CreateDXGIFactory1");
            }
            else {
                m_libDXGI = nullptr;
            }
        }
        m_libDXVA2 = std::shared_ptr<MyLib>(new MyLib(L"dxva2.dll"));
        if (m_libDXVA2->Enabled()) {
            m_arrLibs.push_back(m_libDXVA2.get());
            mDXVA2CreateDirect3DDeviceManager9 = (cbDXVA2CreateDirect3DDeviceManager9)m_libDXVA2->GetFunction("DXVA2CreateDirect3DDeviceManager9");
        }
        else {
            m_libDXVA2 = nullptr;
        }

        //DDRAW 
        m_libDraw = std::shared_ptr<MyLib>(new MyLib(L"ddraw.dll"));
        if (m_libDraw->Enabled()) {
            m_arrLibs.push_back(m_libDraw.get());
            mDirectDrawCreate = (cbDirectDrawCreate)m_libDraw->GetFunction("DirectDrawCreate");
        }
        else {
            m_libDraw = nullptr;
        }

        //DSound 
        m_libDSound = std::shared_ptr<MyLib>(new MyLib(L"dsound.dll"));
        if (m_libDSound->Enabled()) {
            mDirectSoundCreate8 = (cbDirectSoundCreate8)m_libDSound->GetFunction("DirectSoundCreate8");
            CComPtr<IDirectSound8> pDS = NULL;
            HRESULT hr = mDirectSoundCreate8(NULL, &pDS, NULL);
            if (FAILED(hr)) { 
                mDirectSoundCreate8 = nullptr;
                m_libDSound = nullptr;
            }
            else {

                m_arrLibs.push_back(m_libDSound.get());
            }
        }
        else {
            m_libDSound = nullptr;
        }

        //version.dll
        m_libVersion = std::shared_ptr<MyLib>(new MyLib(L"version.dll"));
        if (m_libVersion->Enabled()) {
            m_arrLibs.push_back(m_libVersion.get());
            mGetFileVersionInfoSizeW = (cbGetFileVersionInfoSizeW)m_libVersion->GetFunction("GetFileVersionInfoSizeW");
            mGetFileVersionInfoW = (cbGetFileVersionInfoW)m_libVersion->GetFunction("GetFileVersionInfoW");
            mVerQueryValueW = (cbVerQueryValueW)m_libVersion->GetFunction("VerQueryValueW");
        }
        else {
            m_libVersion = nullptr;
        }

        //DUMP
        m_libDbghelp = std::shared_ptr<MyLib>(new MyLib(L"dbghelp.dll"));
        if (m_libDbghelp->Enabled()) {
            m_arrLibs.push_back(m_libDbghelp.get());
            mMiniDumpWriteDump = (cbMiniDumpWriteDump)m_libDbghelp->GetFunction("MiniDumpWriteDump");

            mMakeSureDirectoryPathExists = (cbMakeSureDirectoryPathExists)m_libDbghelp->GetFunction("MakeSureDirectoryPathExists");
        }
        else {
            m_libDbghelp = nullptr;
        }


        m_libShcore = std::shared_ptr<MyLib>(new MyLib(L"shcore.dll"));//("shcore.dll");
        if (m_libShcore->Enabled()) {
            m_arrLibs.push_back(m_libShcore.get());
            m_SetProcessDpiAwareness = (PFN_SetProcessDpiAwareness)m_libShcore->GetFunction("SetProcessDpiAwareness");
            m_GetDpiForMonitor = (PFN_GetDpiForMonitor)m_libShcore->GetFunction("GetDpiForMonitor");
        }
        else {
            m_libShcore = nullptr;
        }


        m_libUser32 = std::shared_ptr<MyLib>(new MyLib(L"user32.dll"));//("user32.dll");
        if (m_libUser32->Enabled()) {
            m_arrLibs.push_back(m_libUser32.get());
            m_SetProcessDPIAware = (PFN_SetProcessDPIAware)m_libUser32->GetFunction("SetProcessDPIAware");
            m_ChangeWindowMessageFilterEx = (PFN_ChangeWindowMessageFilterEx)m_libUser32->GetFunction("ChangeWindowMessageFilterEx");
            m_EnableNonClientDpiScaling = (PFN_EnableNonClientDpiScaling)m_libUser32->GetFunction("EnableNonClientDpiScaling");
            m_SetProcessDpiAwarenessContext = (PFN_SetProcessDpiAwarenessContext)m_libUser32->GetFunction("SetProcessDpiAwarenessContext");
            m_GetDpiForWindow = (PFN_GetDpiForWindow)m_libUser32->GetFunction("GetDpiForWindow");
            m_AdjustWindowRectExForDpi = (PFN_AdjustWindowRectExForDpi)m_libUser32->GetFunction("AdjustWindowRectExForDpi");
            m_GetSystemMetricsForDpi = (PFN_GetSystemMetricsForDpi)m_libUser32->GetFunction("GetSystemMetricsForDpi");
        }
        else {
            m_libUser32 = nullptr;
        }


        m_libDwmapi = std::shared_ptr<MyLib>(new MyLib(L"dwmapi.dll"));//("user32.dll");
        if (m_libDwmapi->Enabled()) {
            m_arrLibs.push_back(m_libDwmapi.get());
            m_DwmIsCompositionEnabled = (PFN_DwmIsCompositionEnabled)m_libDwmapi->GetFunction("DwmIsCompositionEnabled");
            m_DwmFlush = (PFN_DwmFlush)m_libDwmapi->GetFunction("DwmFlush");
            m_DwmEnableBlurBehindWindow = (PFN_DwmEnableBlurBehindWindow)m_libDwmapi->GetFunction("DwmEnableBlurBehindWindow");
            m_DwmGetColorizationColor = (PFN_DwmGetColorizationColor)m_libDwmapi->GetFunction("DwmGetColorizationColor");
        }
        else {
            m_libDwmapi = nullptr;
        }


        //VC++ 
        m_libNTDLL = std::shared_ptr<MyLib>(new MyLib(L"NTDLL.dll"));;//("NTDLL.dll");
        if (m_libNTDLL->Enabled()) {
            m_arrLibs.push_back(m_libNTDLL.get());
            m_RtlGetVersion = (PFNRtlGetVersion)m_libNTDLL->GetFunction("RtlGetVersion");
            m_RtlGetNtVersionNumbers = (PFNRtlGetNtVersionNumbers)m_libNTDLL->GetFunction("RtlGetNtVersionNumbers");
            m_RtlVerifyVersionInfo = (PFNRtlVerifyVersionInfo)m_libNTDLL->GetFunction("RtlVerifyVersionInfo");
        }
        else {
            m_libNTDLL = nullptr;
        }
        m_libVCRUNTIME140_CLR0400 = std::shared_ptr<MyLib>(new MyLib(L"VCRUNTIME140_CLR0400.dll"));
        if (m_libVCRUNTIME140_CLR0400->Enabled()) {
            m_arrLibs.push_back(m_libVCRUNTIME140_CLR0400.get());
        }
        m_libUCRTBASE_CLR0400 = std::shared_ptr<MyLib>(new MyLib(L"UCRTBASE_CLR0400.dll"));
        if (m_libUCRTBASE_CLR0400->Enabled()) {
            m_arrLibs.push_back(m_libUCRTBASE_CLR0400.get());
        }
        m_libMSVCP140_CLR0400 = std::shared_ptr<MyLib>(new MyLib(L"MSVCP140_CLR0400.dll"));
        if (m_libMSVCP140_CLR0400->Enabled()) {
            m_arrLibs.push_back(m_libMSVCP140_CLR0400.get());
        }

    }

    virtual ~LibInst() {
        exit(-1);
    }
};

#endif //