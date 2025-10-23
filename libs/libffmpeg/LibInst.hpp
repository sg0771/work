/*
第三方库加载
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
typedef void* (*funcHumansegCreate)(int width, int height); //创建对象
typedef int    (*funcHumansegDetect)(void* ptr, uint8_t* pData, int pitch);//智能抠像
typedef void   (*funcHumansegDestroy)(void* ptr);//销毁对象
typedef int    (*funcWXSupportVulkan)();//判断本机是否支持vulkan

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
typedef HRESULT(WINAPI* PFNCoInitializeEx)(LPVOID pvReserved, DWORD dwCoInit);

typedef void (WINAPI* cbLogA)(const char*, va_list);
typedef void (WINAPI* cbLogW)(const wchar_t*, va_list);

class LibInst;
EXTERN_C LibInst* LibInst_GetInst();//单例模式
EXTERN_C void LibInst_LogMsg(cbLogW cbLogW);//日志 
class LibInst {
public:
    //动态库加载
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

        std::wstring GetNameW() {
            return m_strName;
        }
        std::string GetNameU8() {
			std::string strNameU8 = WXBase::UTF16ToUTF8(this->m_strName.c_str());
            return strNameU8;
        }
    private:
        //因为32/64位的原因，第三方DLL可能只是跟当前DLL在同一目录下，而不是在Path路径或者EXE目录下面
        HMODULE OpenLibrary(const wchar_t* filename)
        {
            m_handle = ::LoadLibraryW(filename);

            if (m_handle == nullptr) {
                //当前DLL所在目录
                wchar_t dllPath[MAX_PATH];
                GetModuleFileNameW(WXBase::wxGetSelfModuleHandle()/*NULL*/, dllPath, MAX_PATH); //DLL路径
                std::wstring strDllPath = std::wstring(dllPath);
                size_t found = strDllPath.find_last_of(L"/\\") + 1;
                std::wstring strDllDir = strDllPath.substr(0, found);

                std::wstring strDll = strDllDir + filename;
                m_handle = LoadLibraryW(strDll.c_str());
            }
            if (m_handle){
                m_strName = filename;
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
    std::vector<MyLib*>m_arrLibs;

	cbLogW m_cbLogW = nullptr;
public:
    std::shared_ptr<MyLib> m_libOLE32 = nullptr;//ole32.dll
    PFNCoInitializeEx m_CoInitializeEx = nullptr;
    void LoadOLE32();

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


    void LoadOpenGLFunctions();

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

    //ApowerRec 需要！！
    //系统DLL .Net C++
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

    int m_logmsg = 0;
public:
    static LibInst& GetInst() {
		return *LibInst_GetInst();
    }

    void LogW(const wchar_t* fmt, ...);

    void LogMsg(cbLogW cbLogW);

    //各种库的加载
    LibInst();

    //程序退出使用
    virtual ~LibInst();
};


#endif //