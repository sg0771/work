#include <Windows.h>
#include <windowsx.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <float.h>
#include <math.h>
#include <limits.h>
#include <malloc.h>
#include <wchar.h>
#include <shellapi.h>
#include <WinUser.h>

#include "Utils.hpp"

#define GLFW_INCLUDE_NONE
#include "glfw3.h"

#define _GLFW_INSERT_FIRST      0
#define _GLFW_INSERT_LAST       1

#define _GLFW_POLL_PRESENCE     0
#define _GLFW_POLL_AXES         1
#define _GLFW_POLL_BUTTONS      2
#define _GLFW_POLL_ALL          (_GLFW_POLL_AXES | _GLFW_POLL_BUTTONS)

#define _GLFW_MESSAGE_SIZE      1024

typedef int GLFWbool;

typedef struct _GLFWerror       _GLFWerror;
typedef struct _GLFWinitconfig  _GLFWinitconfig;
typedef struct _GLFWwndconfig   _GLFWwndconfig;
typedef struct _GLFWctxconfig   _GLFWctxconfig;
typedef struct _GLFWfbconfig    _GLFWfbconfig;
typedef struct _GLFWcontext     _GLFWcontext;
typedef struct _GLFWwindow      _GLFWwindow;
typedef struct _GLFWlibrary     _GLFWlibrary;
typedef struct _GLFWmonitor     _GLFWmonitor;
typedef struct _GLFWcursor      _GLFWcursor;
typedef struct _GLFWmapelement  _GLFWmapelement;
typedef struct _GLFWmapping     _GLFWmapping;
typedef struct _GLFWtls         _GLFWtls;
typedef struct _GLFWmutex       _GLFWmutex;

typedef void (*_GLFWmakecontextcurrentfun)(_GLFWwindow*);
typedef void (*_GLFWswapbuffersfun)(_GLFWwindow*);
typedef void (*_GLFWswapintervalfun)(int);
typedef int (*_GLFWextensionsupportedfun)(const char*);
typedef GLFWglproc(*_GLFWgetprocaddressfun)(const char*);
typedef void (*_GLFWdestroycontextfun)(_GLFWwindow*);

#define GL_VERSION 0x1f02
#define GL_NONE 0
#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_UNSIGNED_BYTE 0x1401
#define GL_EXTENSIONS 0x1f03
#define GL_NUM_EXTENSIONS 0x821d
#define GL_CONTEXT_FLAGS 0x821e
#define GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT 0x00000001
#define GL_CONTEXT_FLAG_DEBUG_BIT 0x00000002
#define GL_CONTEXT_PROFILE_MASK 0x9126
#define GL_CONTEXT_COMPATIBILITY_PROFILE_BIT 0x00000002
#define GL_CONTEXT_CORE_PROFILE_BIT 0x00000001
#define GL_RESET_NOTIFICATION_STRATEGY_ARB 0x8256
#define GL_LOSE_CONTEXT_ON_RESET_ARB 0x8252
#define GL_NO_RESET_NOTIFICATION_ARB 0x8261
#define GL_CONTEXT_RELEASE_BEHAVIOR 0x82fb
#define GL_CONTEXT_RELEASE_BEHAVIOR_FLUSH 0x82fc
#define GL_CONTEXT_FLAG_NO_ERROR_BIT_KHR 0x00000008

typedef int GLint;
typedef unsigned int GLuint;
typedef unsigned int GLenum;
typedef unsigned int GLbitfield;
typedef unsigned char GLubyte;



#define VK_NULL_HANDLE 0

typedef void* VkInstance;
typedef void* VkPhysicalDevice;
typedef uint64_t VkSurfaceKHR;
typedef uint32_t VkFlags;
typedef uint32_t VkBool32;

typedef enum VkStructureType
{
    VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR = 1000004000,
    VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR = 1000005000,
    VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR = 1000006000,
    VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR = 1000009000,
    VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK = 1000123000,
    VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT = 1000217000,
    VK_STRUCTURE_TYPE_MAX_ENUM = 0x7FFFFFFF
} VkStructureType;

typedef enum VkResult
{
    VK_SUCCESS = 0,
    VK_NOT_READY = 1,
    VK_TIMEOUT = 2,
    VK_EVENT_SET = 3,
    VK_EVENT_RESET = 4,
    VK_INCOMPLETE = 5,
    VK_ERROR_OUT_OF_HOST_MEMORY = -1,
    VK_ERROR_OUT_OF_DEVICE_MEMORY = -2,
    VK_ERROR_INITIALIZATION_FAILED = -3,
    VK_ERROR_DEVICE_LOST = -4,
    VK_ERROR_MEMORY_MAP_FAILED = -5,
    VK_ERROR_LAYER_NOT_PRESENT = -6,
    VK_ERROR_EXTENSION_NOT_PRESENT = -7,
    VK_ERROR_FEATURE_NOT_PRESENT = -8,
    VK_ERROR_INCOMPATIBLE_DRIVER = -9,
    VK_ERROR_TOO_MANY_OBJECTS = -10,
    VK_ERROR_FORMAT_NOT_SUPPORTED = -11,
    VK_ERROR_SURFACE_LOST_KHR = -1000000000,
    VK_SUBOPTIMAL_KHR = 1000001003,
    VK_ERROR_OUT_OF_DATE_KHR = -1000001004,
    VK_ERROR_INCOMPATIBLE_DISPLAY_KHR = -1000003001,
    VK_ERROR_NATIVE_WINDOW_IN_USE_KHR = -1000000001,
    VK_ERROR_VALIDATION_FAILED_EXT = -1000011001,
    VK_RESULT_MAX_ENUM = 0x7FFFFFFF
} VkResult;

typedef struct VkAllocationCallbacks VkAllocationCallbacks;

typedef struct VkExtensionProperties
{
    char            extensionName[256];
    uint32_t        specVersion;
} VkExtensionProperties;

typedef void (APIENTRY* PFN_vkVoidFunction)(void);

#if defined(_GLFW_VULKAN_STATIC)
PFN_vkVoidFunction vkGetInstanceProcAddr(VkInstance, const char*);
VkResult vkEnumerateInstanceExtensionProperties(const char*, uint32_t*, VkExtensionProperties*);
#else
typedef PFN_vkVoidFunction(APIENTRY* PFN_vkGetInstanceProcAddr)(VkInstance, const char*);
typedef VkResult(APIENTRY* PFN_vkEnumerateInstanceExtensionProperties)(const char*, uint32_t*, VkExtensionProperties*);
#define vkEnumerateInstanceExtensionProperties _glfw.vk.EnumerateInstanceExtensionProperties
#define vkGetInstanceProcAddr _glfw.vk.GetInstanceProcAddr
#endif



// We don't need all the fancy stuff
#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

// This is a workaround for the fact that glfw3.h needs to export APIENTRY (for
// example to allow applications to correctly declare a GL_KHR_debug callback)
// but windows.h assumes no one will define APIENTRY before it does
#undef APIENTRY

// GLFW on Windows is Unicode only and does not work in MBCS mode
#ifndef UNICODE
#define UNICODE
#endif

// GLFW requires Windows XP or later
#if WINVER < 0x0501
#undef WINVER
#define WINVER 0x0501
#endif
#if _WIN32_WINNT < 0x0501
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif

// GLFW uses DirectInput8 interfaces
#define DIRECTINPUT_VERSION 0x0800

// GLFW uses OEM cursor resources
#define OEMRESOURCE

#include <wctype.h>
#include <windows.h>
#include <dinput.h>
#include <xinput.h>
#include <dbt.h>

// HACK: Define macros that some windows.h variants don't
#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif
#ifndef WM_DWMCOMPOSITIONCHANGED
#define WM_DWMCOMPOSITIONCHANGED 0x031E
#endif
#ifndef WM_DWMCOLORIZATIONCOLORCHANGED
#define WM_DWMCOLORIZATIONCOLORCHANGED 0x0320
#endif
#ifndef WM_COPYGLOBALDATA
#define WM_COPYGLOBALDATA 0x0049
#endif
#ifndef WM_UNICHAR
#define WM_UNICHAR 0x0109
#endif
#ifndef UNICODE_NOCHAR
#define UNICODE_NOCHAR 0xFFFF
#endif
#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif
#ifndef GET_XBUTTON_WPARAM
#define GET_XBUTTON_WPARAM(w) (HIWORD(w))
#endif
#ifndef EDS_ROTATEDMODE
#define EDS_ROTATEDMODE 0x00000004
#endif
#ifndef DISPLAY_DEVICE_ACTIVE
#define DISPLAY_DEVICE_ACTIVE 0x00000001
#endif
#ifndef _WIN32_WINNT_WINBLUE
#define _WIN32_WINNT_WINBLUE 0x0603
#endif
#ifndef _WIN32_WINNT_WIN8
#define _WIN32_WINNT_WIN8 0x0602
#endif
#ifndef WM_GETDPISCALEDSIZE
#define WM_GETDPISCALEDSIZE 0x02e4
#endif
#ifndef USER_DEFAULT_SCREEN_DPI
#define USER_DEFAULT_SCREEN_DPI 96
#endif
#ifndef OCR_HAND
#define OCR_HAND 32649
#endif

#if WINVER < 0x0601
typedef struct
{
    DWORD cbSize;
    DWORD ExtStatus;
} CHANGEFILTERSTRUCT;
#ifndef MSGFLT_ALLOW
#define MSGFLT_ALLOW 1
#endif
#endif /*Windows 7*/

#if WINVER < 0x0600
#define DWM_BB_ENABLE 0x00000001
#define DWM_BB_BLURREGION 0x00000002
typedef struct
{
    DWORD dwFlags;
    BOOL fEnable;
    HRGN hRgnBlur;
    BOOL fTransitionOnMaximized;
} DWM_BLURBEHIND;
#else
#include <dwmapi.h>
#endif /*Windows Vista*/

//#ifndef DPI_ENUMS_DECLARED
//typedef enum
//{
//    PROCESS_DPI_UNAWARE = 0,
//    PROCESS_SYSTEM_DPI_AWARE = 1,
//    PROCESS_PER_MONITOR_DPI_AWARE = 2
//} PROCESS_DPI_AWARENESS;
//typedef enum
//{
//    MDT_EFFECTIVE_DPI = 0,
//    MDT_ANGULAR_DPI = 1,
//    MDT_RAW_DPI = 2,
//    MDT_DEFAULT = MDT_EFFECTIVE_DPI
//} MONITOR_DPI_TYPE;
//#endif /*DPI_ENUMS_DECLARED*/

#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((HANDLE) -4)
#endif /*DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2*/

// Replacement for versionhelpers.h macros, as we cannot rely on the
// application having a correct embedded manifest
//
#define IsWindowsXPOrGreater()                                 \
    _glfwIsWindowsVersionOrGreaterWin32(HIBYTE(_WIN32_WINNT_WINXP),   \
                                        LOBYTE(_WIN32_WINNT_WINXP), 0)
#define IsWindowsVistaOrGreater()                                     \
    _glfwIsWindowsVersionOrGreaterWin32(HIBYTE(_WIN32_WINNT_VISTA),   \
                                        LOBYTE(_WIN32_WINNT_VISTA), 0)
#define IsWindows7OrGreater()                                         \
    _glfwIsWindowsVersionOrGreaterWin32(HIBYTE(_WIN32_WINNT_WIN7),    \
                                        LOBYTE(_WIN32_WINNT_WIN7), 0)
#define IsWindows8OrGreater()                                         \
    _glfwIsWindowsVersionOrGreaterWin32(HIBYTE(_WIN32_WINNT_WIN8),    \
                                        LOBYTE(_WIN32_WINNT_WIN8), 0)
#define IsWindows8Point1OrGreater()                                   \
    _glfwIsWindowsVersionOrGreaterWin32(HIBYTE(_WIN32_WINNT_WINBLUE), \
                                        LOBYTE(_WIN32_WINNT_WINBLUE), 0)

#define _glfwIsWindows10AnniversaryUpdateOrGreaterWin32() \
    _glfwIsWindows10BuildOrGreaterWin32(14393)
#define _glfwIsWindows10CreatorsUpdateOrGreaterWin32() \
    _glfwIsWindows10BuildOrGreaterWin32(15063)

// HACK: Define macros that some xinput.h variants don't
#ifndef XINPUT_CAPS_WIRELESS
#define XINPUT_CAPS_WIRELESS 0x0002
#endif
#ifndef XINPUT_DEVSUBTYPE_WHEEL
#define XINPUT_DEVSUBTYPE_WHEEL 0x02
#endif
#ifndef XINPUT_DEVSUBTYPE_ARCADE_STICK
#define XINPUT_DEVSUBTYPE_ARCADE_STICK 0x03
#endif
#ifndef XINPUT_DEVSUBTYPE_FLIGHT_STICK
#define XINPUT_DEVSUBTYPE_FLIGHT_STICK 0x04
#endif
#ifndef XINPUT_DEVSUBTYPE_DANCE_PAD
#define XINPUT_DEVSUBTYPE_DANCE_PAD 0x05
#endif
#ifndef XINPUT_DEVSUBTYPE_GUITAR
#define XINPUT_DEVSUBTYPE_GUITAR 0x06
#endif
#ifndef XINPUT_DEVSUBTYPE_DRUM_KIT
#define XINPUT_DEVSUBTYPE_DRUM_KIT 0x08
#endif
#ifndef XINPUT_DEVSUBTYPE_ARCADE_PAD
#define XINPUT_DEVSUBTYPE_ARCADE_PAD 0x13
#endif
#ifndef XUSER_MAX_COUNT
#define XUSER_MAX_COUNT 4
#endif

// HACK: Define macros that some dinput.h variants don't
#ifndef DIDFT_OPTIONAL
#define DIDFT_OPTIONAL 0x80000000
#endif

//OpenGL32.dll
#define  glGetStringi  LibInst::GetInst().m_glGetStringi
#define  glGetIntegerv LibInst::GetInst().m_glGetIntegerv
#define  glGetString   LibInst::GetInst().m_glGetString
#define  glClear       LibInst::GetInst().m_glClear

// user32.dll function pointer typedefs
#define SetProcessDPIAware LibInst::GetInst().m_SetProcessDPIAware
#define ChangeWindowMessageFilterEx LibInst::GetInst().m_ChangeWindowMessageFilterEx
#define EnableNonClientDpiScaling LibInst::GetInst().m_EnableNonClientDpiScaling
#define SetProcessDpiAwarenessContext LibInst::GetInst().m_SetProcessDpiAwarenessContext
#define GetDpiForWindow LibInst::GetInst().m_GetDpiForWindow
#define AdjustWindowRectExForDpi LibInst::GetInst().m_AdjustWindowRectExForDpi
#define GetSystemMetricsForDpi LibInst::GetInst().m_GetSystemMetricsForDpi

// dwmapi.dll function pointer typedefs

#define DwmIsCompositionEnabled LibInst::GetInst().m_DwmIsCompositionEnabled
#define DwmFlush LibInst::GetInst().m_DwmFlush
#define DwmEnableBlurBehindWindow LibInst::GetInst().m_DwmEnableBlurBehindWindow
#define DwmGetColorizationColor LibInst::GetInst().m_DwmGetColorizationColor

// shcore.dll function pointer typedefs
#define SetProcessDpiAwareness LibInst::GetInst().m_SetProcessDpiAwareness
#define GetDpiForMonitor LibInst::GetInst().m_GetDpiForMonitor

// ntdll.dll function pointer typedefs
#define RtlVerifyVersionInfo LibInst::GetInst().m_RtlVerifyVersionInfo


#define WGL_NUMBER_PIXEL_FORMATS_ARB 0x2000
#define WGL_SUPPORT_OPENGL_ARB 0x2010
#define WGL_DRAW_TO_WINDOW_ARB 0x2001
#define WGL_PIXEL_TYPE_ARB 0x2013
#define WGL_TYPE_RGBA_ARB 0x202b
#define WGL_ACCELERATION_ARB 0x2003
#define WGL_NO_ACCELERATION_ARB 0x2025
#define WGL_RED_BITS_ARB 0x2015
#define WGL_RED_SHIFT_ARB 0x2016
#define WGL_GREEN_BITS_ARB 0x2017
#define WGL_GREEN_SHIFT_ARB 0x2018
#define WGL_BLUE_BITS_ARB 0x2019
#define WGL_BLUE_SHIFT_ARB 0x201a
#define WGL_ALPHA_BITS_ARB 0x201b
#define WGL_ALPHA_SHIFT_ARB 0x201c
#define WGL_ACCUM_BITS_ARB 0x201d
#define WGL_ACCUM_RED_BITS_ARB 0x201e
#define WGL_ACCUM_GREEN_BITS_ARB 0x201f
#define WGL_ACCUM_BLUE_BITS_ARB 0x2020
#define WGL_ACCUM_ALPHA_BITS_ARB 0x2021
#define WGL_DEPTH_BITS_ARB 0x2022
#define WGL_STENCIL_BITS_ARB 0x2023
#define WGL_AUX_BUFFERS_ARB 0x2024
#define WGL_STEREO_ARB 0x2012
#define WGL_DOUBLE_BUFFER_ARB 0x2011
#define WGL_SAMPLES_ARB 0x2042
#define WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB 0x20a9
#define WGL_CONTEXT_DEBUG_BIT_ARB 0x00000001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x00000002
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002
#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_FLAGS_ARB 0x2094
#define WGL_CONTEXT_ES2_PROFILE_BIT_EXT 0x00000004
#define WGL_CONTEXT_ROBUST_ACCESS_BIT_ARB 0x00000004
#define WGL_LOSE_CONTEXT_ON_RESET_ARB 0x8252
#define WGL_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB 0x8256
#define WGL_NO_RESET_NOTIFICATION_ARB 0x8261
#define WGL_CONTEXT_RELEASE_BEHAVIOR_ARB 0x2097
#define WGL_CONTEXT_RELEASE_BEHAVIOR_NONE_ARB 0
#define WGL_CONTEXT_RELEASE_BEHAVIOR_FLUSH_ARB 0x2098
#define WGL_CONTEXT_OPENGL_NO_ERROR_ARB 0x31b3
#define WGL_COLORSPACE_EXT 0x309d
#define WGL_COLORSPACE_SRGB_EXT 0x3089

#define ERROR_INVALID_VERSION_ARB 0x2095
#define ERROR_INVALID_PROFILE_ARB 0x2096
#define ERROR_INCOMPATIBLE_DEVICE_CONTEXTS_ARB 0x2054

//Opengl32.dll function pointer typedefs
// WGL extension pointer typedefs
#define wglSwapIntervalEXT LibInst::GetInst().m_wglSwapIntervalEXT
#define wglGetPixelFormatAttribivARB LibInst::GetInst().m_wglGetPixelFormatAttribivARB
#define wglGetExtensionsStringEXT LibInst::GetInst().m_wglGetExtensionsStringEXT
#define wglGetExtensionsStringARB LibInst::GetInst().m_wglGetExtensionsStringARB
#define wglCreateContextAttribsARB LibInst::GetInst().m_wglCreateContextAttribsARB

// opengl32.dll function pointer typedefs

#define wglCreateContext LibInst::GetInst().m_wglCreateContext
#define wglDeleteContext LibInst::GetInst().m_wglDeleteContext
#define wglGetProcAddress LibInst::GetInst().m_wglGetProcAddress
#define wglGetCurrentDC LibInst::GetInst().m_wglGetCurrentDC
#define wglGetCurrentContext LibInst::GetInst().m_wglGetCurrentContext
#define wglMakeCurrent LibInst::GetInst().m_wglMakeCurrent
#define wglShareLists  LibInst::GetInst().m_wglShareLists

#define _GLFW_PLATFORM_CONTEXT_STATE            _GLFWcontextWGL wgl
#define _GLFW_PLATFORM_LIBRARY_CONTEXT_STATE    _GLFWlibraryWGL wgl


// WGL-specific per-context data
//
typedef struct _GLFWcontextWGL
{
    HDC       dc;
    HGLRC     handle;
    int       interval;
} _GLFWcontextWGL;

// WGL-specific global data
//
typedef struct _GLFWlibraryWGL
{
    HINSTANCE                           instance;
    //PFN_wglCreateContext                CreateContext;
    //PFN_wglDeleteContext                DeleteContext;
    //PFN_wglGetProcAddress               GetProcAddress;
    //PFN_wglGetCurrentDC                 GetCurrentDC;
    //PFN_wglGetCurrentContext            GetCurrentContext;
    //PFN_wglMakeCurrent                  MakeCurrent;
    //PFN_wglShareLists                   ShareLists;

    //PFNWGLSWAPINTERVALEXTPROC           SwapIntervalEXT;
    //PFNWGLGETPIXELFORMATATTRIBIVARBPROC GetPixelFormatAttribivARB;
    //PFNWGLGETEXTENSIONSSTRINGEXTPROC    GetExtensionsStringEXT;
    //PFNWGLGETEXTENSIONSSTRINGARBPROC    GetExtensionsStringARB;
    //PFNWGLCREATECONTEXTATTRIBSARBPROC   CreateContextAttribsARB;
    GLFWbool                            EXT_swap_control;
    GLFWbool                            EXT_colorspace;
    GLFWbool                            ARB_multisample;
    GLFWbool                            ARB_framebuffer_sRGB;
    GLFWbool                            EXT_framebuffer_sRGB;
    GLFWbool                            ARB_pixel_format;
    GLFWbool                            ARB_create_context;
    GLFWbool                            ARB_create_context_profile;
    GLFWbool                            EXT_create_context_es2_profile;
    GLFWbool                            ARB_create_context_robustness;
    GLFWbool                            ARB_create_context_no_error;
    GLFWbool                            ARB_context_flush_control;
} _GLFWlibraryWGL;


GLFWbool _glfwInitWGL(void);
void _glfwTerminateWGL(void);
GLFWbool _glfwCreateContextWGL(_GLFWwindow* window,
    const _GLFWctxconfig* ctxconfig,
    const _GLFWfbconfig* fbconfig);



#if !defined(_GLFW_WNDCLASSNAME)
#define _GLFW_WNDCLASSNAME L"GLFW30"
#endif

//#define _glfw_dlopen(name) LoadLibraryA(name)
//#define _glfw_dlclose(handle) FreeLibrary((HMODULE) handle)
//#define _glfw_dlsym(handle, name) GetProcAddress((HMODULE) handle, name)

#define _GLFW_EGL_NATIVE_WINDOW  ((EGLNativeWindowType) window->win32.handle)
#define _GLFW_EGL_NATIVE_DISPLAY EGL_DEFAULT_DISPLAY

#define _GLFW_PLATFORM_WINDOW_STATE         _GLFWwindowWin32  win32
#define _GLFW_PLATFORM_LIBRARY_WINDOW_STATE _GLFWlibraryWin32 win32
#define _GLFW_PLATFORM_LIBRARY_TIMER_STATE  _GLFWtimerWin32   win32
#define _GLFW_PLATFORM_MONITOR_STATE        _GLFWmonitorWin32 win32
#define _GLFW_PLATFORM_CURSOR_STATE         _GLFWcursorWin32  win32
#define _GLFW_PLATFORM_TLS_STATE            _GLFWtlsWin32     win32
#define _GLFW_PLATFORM_MUTEX_STATE          _GLFWmutexWin32   win32


// Win32-specific per-window data
//
typedef struct _GLFWwindowWin32
{
    HWND                handle;
    HICON               bigIcon;
    HICON               smallIcon;

    GLFWbool            cursorTracked;
    GLFWbool            frameAction;
    GLFWbool            iconified;
    GLFWbool            maximized;
    // Whether to enable framebuffer transparency on DWM
    GLFWbool            transparent;
    GLFWbool            scaleToMonitor;

    // Cached size used to filter out duplicate events
    int                 width, height;

    // The last received cursor position, regardless of source
    int                 lastCursorPosX, lastCursorPosY;
    // The last recevied high surrogate when decoding pairs of UTF-16 messages
    WCHAR               highSurrogate;
} _GLFWwindowWin32;

// Win32-specific global data
//
typedef struct _GLFWlibraryWin32
{
    HINSTANCE           instance;
    HWND                helperWindowHandle;
    HDEVNOTIFY          deviceNotificationHandle;
    DWORD               foregroundLockTimeout;
    int                 acquiredMonitorCount;
    char* clipboardString;
    short int           keycodes[512];
    short int           scancodes[GLFW_KEY_LAST + 1];
    char                keynames[GLFW_KEY_LAST + 1][5];
    // Where to place the cursor when re-enabled
    double              restoreCursorPosX, restoreCursorPosY;
    // The window whose disabled cursor mode is active
    _GLFWwindow* disabledCursorWindow;
    RAWINPUT* rawInput;
    int                 rawInputSize;
    UINT                mouseTrailSize;
} _GLFWlibraryWin32;

// Win32-specific per-monitor data
//
typedef struct _GLFWmonitorWin32
{
    HMONITOR            handle;
    // This size matches the static size of DISPLAY_DEVICE.DeviceName
    WCHAR               adapterName[32];
    WCHAR               displayName[32];
    char                publicAdapterName[32];
    char                publicDisplayName[32];
    GLFWbool            modesPruned;
    GLFWbool            modeChanged;
} _GLFWmonitorWin32;

// Win32-specific per-cursor data
//
typedef struct _GLFWcursorWin32
{
    HCURSOR             handle;
} _GLFWcursorWin32;

// Win32-specific global timer data
//
typedef struct _GLFWtimerWin32
{
    uint64_t            frequency;
} _GLFWtimerWin32;

// Win32-specific thread local storage data
//
typedef struct _GLFWtlsWin32
{
    GLFWbool            allocated;
    DWORD               index;
} _GLFWtlsWin32;

// Win32-specific mutex data
//
typedef struct _GLFWmutexWin32
{
    GLFWbool            allocated;
    CRITICAL_SECTION    section;
} _GLFWmutexWin32;


GLFWbool _glfwRegisterWindowClassWin32(void);
void _glfwUnregisterWindowClassWin32(void);

WCHAR* _glfwCreateWideStringFromUTF8Win32(const char* source);
char* _glfwCreateUTF8FromWideStringWin32(const WCHAR* source);
BOOL _glfwIsWindowsVersionOrGreaterWin32(WORD major, WORD minor, WORD sp);
BOOL _glfwIsWindows10BuildOrGreaterWin32(WORD build);
void _glfwInputErrorWin32(int error, const char* description);
void _glfwUpdateKeyNamesWin32(void);

void _glfwPollMonitorsWin32(void);
void _glfwSetVideoModeWin32(_GLFWmonitor* monitor, const GLFWvidmode* desired);
void _glfwRestoreVideoModeWin32(_GLFWmonitor* monitor);
void _glfwGetMonitorContentScaleWin32(HMONITOR handle, float* xscale, float* yscale);



// Constructs a version number string from the public header macros
#define _GLFW_CONCAT_VERSION(m, n, r) #m "." #n "." #r
#define _GLFW_MAKE_VERSION(m, n, r) _GLFW_CONCAT_VERSION(m, n, r)
#define _GLFW_VERSION_NUMBER _GLFW_MAKE_VERSION(GLFW_VERSION_MAJOR, \
                                                GLFW_VERSION_MINOR, \
                                                GLFW_VERSION_REVISION)

// Checks for whether the library has been initialized
#define _GLFW_REQUIRE_INIT()                         \
    if (!_glfw.initialized)                          \
    {                                                \
        _glfwInputError(GLFW_NOT_INITIALIZED, NULL); \
        return;                                      \
    }
#define _GLFW_REQUIRE_INIT_OR_RETURN(x)              \
    if (!_glfw.initialized)                          \
    {                                                \
        _glfwInputError(GLFW_NOT_INITIALIZED, NULL); \
        return x;                                    \
    }

// Swaps the provided pointers
#define _GLFW_SWAP_POINTERS(x, y, type) \
    {                             \
        type* t;                  \
        t = x;                    \
        x = y;                    \
        y = t;                    \
    }

// Per-thread error structure
//
struct _GLFWerror
{
    _GLFWerror* next;
    int             code;
    char            description[_GLFW_MESSAGE_SIZE];
};

// Initialization configuration
//
// Parameters relating to the initialization of the library
//
struct _GLFWinitconfig
{
    GLFWbool      hatButtons;
    struct {
        GLFWbool  menubar;
        GLFWbool  chdir;
    } ns;
};

// Window configuration
//
// Parameters relating to the creation of the window but not directly related
// to the framebuffer.  This is used to pass window creation parameters from
// shared code to the platform API.
//
struct _GLFWwndconfig
{
    int           width;
    int           height;
    const char* title;
    GLFWbool      resizable;
    GLFWbool      visible;
    GLFWbool      decorated;
    GLFWbool      focused;
    GLFWbool      autoIconify;
    GLFWbool      floating;
    GLFWbool      maximized;
    GLFWbool      centerCursor;
    GLFWbool      focusOnShow;
    GLFWbool      scaleToMonitor;
    struct {
        GLFWbool  retina;
        char      frameName[256];
    } ns;
    struct {
        char      className[256];
        char      instanceName[256];
    } x11;
};

// Context configuration
//
// Parameters relating to the creation of the context but not directly related
// to the framebuffer.  This is used to pass context creation parameters from
// shared code to the platform API.
//
struct _GLFWctxconfig
{
    int           client;
    int           source;
    int           major;
    int           minor;
    GLFWbool      forward;
    GLFWbool      debug;
    GLFWbool      noerror;
    int           profile;
    int           robustness;
    int           release;
    _GLFWwindow* share;
    struct {
        GLFWbool  offline;
    } nsgl;
};

// Framebuffer configuration
//
// This describes buffers and their sizes.  It also contains
// a platform-specific ID used to map back to the backend API object.
//
// It is used to pass framebuffer parameters from shared code to the platform
// API and also to enumerate and select available framebuffer configs.
//
struct _GLFWfbconfig
{
    int         redBits;
    int         greenBits;
    int         blueBits;
    int         alphaBits;
    int         depthBits;
    int         stencilBits;
    int         accumRedBits;
    int         accumGreenBits;
    int         accumBlueBits;
    int         accumAlphaBits;
    int         auxBuffers;
    GLFWbool    stereo;
    int         samples;
    GLFWbool    sRGB;
    GLFWbool    doublebuffer;
    GLFWbool    transparent;
    uintptr_t   handle;
};

// Context structure
//
struct _GLFWcontext
{
    int                 client;
    int                 source;
    int                 major, minor, revision;
    GLFWbool            forward, debug, noerror;
    int                 profile;
    int                 robustness;
    int                 release;



    _GLFWmakecontextcurrentfun  makeCurrent;
    _GLFWswapbuffersfun         swapBuffers;
    _GLFWswapintervalfun        swapInterval;
    _GLFWextensionsupportedfun  extensionSupported;
    _GLFWgetprocaddressfun      getProcAddress;
    _GLFWdestroycontextfun      destroy;

    // This is defined in the context API's context.h
    _GLFW_PLATFORM_CONTEXT_STATE;
};

// Window and context structure
//
struct _GLFWwindow
{
    struct _GLFWwindow* next;

    // Window settings and state
    GLFWbool            resizable;
    GLFWbool            decorated;
    GLFWbool            autoIconify;
    GLFWbool            floating;
    GLFWbool            focusOnShow;
    GLFWbool            shouldClose;
    void* userPointer;
    GLFWbool            doublebuffer;
    GLFWvidmode         videoMode;
    _GLFWmonitor* monitor;
    _GLFWcursor* cursor;

    int                 minwidth, minheight;
    int                 maxwidth, maxheight;
    int                 numer, denom;

    GLFWbool            stickyKeys;
    GLFWbool            stickyMouseButtons;
    GLFWbool            lockKeyMods;
    int                 cursorMode;
    char                mouseButtons[GLFW_MOUSE_BUTTON_LAST + 1];
    char                keys[GLFW_KEY_LAST + 1];
    // Virtual cursor position when cursor is disabled
    double              virtualCursorPosX, virtualCursorPosY;
    GLFWbool            rawMouseMotion;

    _GLFWcontext        context;

    // This is defined in the window API's platform.h
    _GLFW_PLATFORM_WINDOW_STATE;
};

// Monitor structure
//
struct _GLFWmonitor
{
    char            name[128];
    void* userPointer;

    // Physical dimensions in millimeters.
    int             widthMM, heightMM;

    // The window whose video mode is current on this monitor
    _GLFWwindow* window;

    GLFWvidmode* modes;
    int             modeCount;
    GLFWvidmode     currentMode;

    GLFWgammaramp   originalRamp;
    GLFWgammaramp   currentRamp;

    // This is defined in the window API's platform.h
    _GLFW_PLATFORM_MONITOR_STATE;
};

// Cursor structure
//
struct _GLFWcursor
{
    _GLFWcursor* next;

    // This is defined in the window API's platform.h
    _GLFW_PLATFORM_CURSOR_STATE;
};

// Gamepad mapping element structure
//
struct _GLFWmapelement
{
    uint8_t         type;
    uint8_t         index;
    int8_t          axisScale;
    int8_t          axisOffset;
};

// Gamepad mapping structure
//
struct _GLFWmapping
{
    char            name[128];
    char            guid[33];
    _GLFWmapelement buttons[15];
    _GLFWmapelement axes[6];
};


// Thread local storage structure
//
struct _GLFWtls
{
    // This is defined in the platform's thread.h
    _GLFW_PLATFORM_TLS_STATE;
};

// Mutex structure
//
struct _GLFWmutex
{
    // This is defined in the platform's thread.h
    _GLFW_PLATFORM_MUTEX_STATE;
};

// Library global data
//
struct _GLFWlibrary
{
    GLFWbool            initialized;

    struct {
        _GLFWinitconfig init;
        _GLFWfbconfig   framebuffer;
        _GLFWwndconfig  window;
        _GLFWctxconfig  context;
        int             refreshRate;
    } hints;

    _GLFWerror* errorListHead;
    _GLFWcursor* cursorListHead;
    _GLFWwindow* windowListHead;

    _GLFWmonitor** monitors;
    int                 monitorCount;

    _GLFWtls            errorSlot;
    _GLFWtls            contextSlot;
    _GLFWmutex          errorLock;


    // This is defined in the window API's platform.h
    _GLFW_PLATFORM_LIBRARY_WINDOW_STATE;
    // This is defined in the context API's context.h
    _GLFW_PLATFORM_LIBRARY_CONTEXT_STATE;
};

// Global state shared between compilation units of GLFW
//
extern _GLFWlibrary _glfw;


//////////////////////////////////////////////////////////////////////////
//////                       GLFW platform API                      //////
//////////////////////////////////////////////////////////////////////////

int _glfwPlatformInit(void);
void _glfwPlatformTerminate(void);
const char* _glfwPlatformGetVersionString(void);

void _glfwPlatformGetCursorPos(_GLFWwindow* window, double* xpos, double* ypos);
void _glfwPlatformSetCursorPos(_GLFWwindow* window, double xpos, double ypos);
void _glfwPlatformSetCursorMode(_GLFWwindow* window, int mode);
void _glfwPlatformSetRawMouseMotion(_GLFWwindow* window, GLFWbool enabled);
GLFWbool _glfwPlatformRawMouseMotionSupported(void);
int _glfwPlatformCreateCursor(_GLFWcursor* cursor,
    const GLFWimage* image, int xhot, int yhot);
void _glfwPlatformDestroyCursor(_GLFWcursor* cursor);
void _glfwPlatformSetCursor(_GLFWwindow* window, _GLFWcursor* cursor);

const char* _glfwPlatformGetScancodeName(int scancode);
int _glfwPlatformGetKeyScancode(int key);

void _glfwPlatformFreeMonitor(_GLFWmonitor* monitor);
void _glfwPlatformGetMonitorPos(_GLFWmonitor* monitor, int* xpos, int* ypos);
void _glfwPlatformGetMonitorContentScale(_GLFWmonitor* monitor,
    float* xscale, float* yscale);
void _glfwPlatformGetMonitorWorkarea(_GLFWmonitor* monitor, int* xpos, int* ypos, int* width, int* height);
GLFWvidmode* _glfwPlatformGetVideoModes(_GLFWmonitor* monitor, int* count);
void _glfwPlatformGetVideoMode(_GLFWmonitor* monitor, GLFWvidmode* mode);
GLFWbool _glfwPlatformGetGammaRamp(_GLFWmonitor* monitor, GLFWgammaramp* ramp);
void _glfwPlatformSetGammaRamp(_GLFWmonitor* monitor, const GLFWgammaramp* ramp);

void _glfwPlatformSetClipboardString(const char* string);
const char* _glfwPlatformGetClipboardString(void);

uint64_t _glfwPlatformGetTimerValue(void);
uint64_t _glfwPlatformGetTimerFrequency(void);

int _glfwPlatformCreateWindow(_GLFWwindow* window,
    const _GLFWwndconfig* wndconfig,
    const _GLFWctxconfig* ctxconfig,
    const _GLFWfbconfig* fbconfig);
void _glfwPlatformDestroyWindow(_GLFWwindow* window);
void _glfwPlatformSetWindowTitle(_GLFWwindow* window, const char* title);
void _glfwPlatformSetWindowIcon(_GLFWwindow* window,
    int count, const GLFWimage* images);
void _glfwPlatformGetWindowPos(_GLFWwindow* window, int* xpos, int* ypos);
void _glfwPlatformSetWindowPos(_GLFWwindow* window, int xpos, int ypos);
void _glfwPlatformGetWindowSize(_GLFWwindow* window, int* width, int* height);
void _glfwPlatformSetWindowSize(_GLFWwindow* window, int width, int height);
void _glfwPlatformSetWindowSizeLimits(_GLFWwindow* window,
    int minwidth, int minheight,
    int maxwidth, int maxheight);
void _glfwPlatformSetWindowAspectRatio(_GLFWwindow* window, int numer, int denom);
void _glfwPlatformGetFramebufferSize(_GLFWwindow* window, int* width, int* height);
void _glfwPlatformGetWindowFrameSize(_GLFWwindow* window,
    int* left, int* top,
    int* right, int* bottom);
void _glfwPlatformGetWindowContentScale(_GLFWwindow* window,
    float* xscale, float* yscale);
void _glfwPlatformIconifyWindow(_GLFWwindow* window);
void _glfwPlatformRestoreWindow(_GLFWwindow* window);
void _glfwPlatformMaximizeWindow(_GLFWwindow* window);
void _glfwPlatformShowWindow(_GLFWwindow* window);
void _glfwPlatformHideWindow(_GLFWwindow* window);
void _glfwPlatformRequestWindowAttention(_GLFWwindow* window);
void _glfwPlatformFocusWindow(_GLFWwindow* window);
void _glfwPlatformSetWindowMonitor(_GLFWwindow* window, _GLFWmonitor* monitor,
    int xpos, int ypos, int width, int height,
    int refreshRate);
int _glfwPlatformWindowFocused(_GLFWwindow* window);
int _glfwPlatformWindowIconified(_GLFWwindow* window);
int _glfwPlatformWindowVisible(_GLFWwindow* window);
int _glfwPlatformWindowMaximized(_GLFWwindow* window);
int _glfwPlatformWindowHovered(_GLFWwindow* window);
int _glfwPlatformFramebufferTransparent(_GLFWwindow* window);
float _glfwPlatformGetWindowOpacity(_GLFWwindow* window);
void _glfwPlatformSetWindowResizable(_GLFWwindow* window, GLFWbool enabled);
void _glfwPlatformSetWindowDecorated(_GLFWwindow* window, GLFWbool enabled);
void _glfwPlatformSetWindowFloating(_GLFWwindow* window, GLFWbool enabled);
void _glfwPlatformSetWindowOpacity(_GLFWwindow* window, float opacity);

void _glfwPlatformPollEvents(void);
void _glfwPlatformWaitEvents(void);
void _glfwPlatformWaitEventsTimeout(double timeout);
void _glfwPlatformPostEmptyEvent(void);

void _glfwPlatformGetRequiredInstanceExtensions(char** extensions);
int _glfwPlatformGetPhysicalDevicePresentationSupport(VkInstance instance,
    VkPhysicalDevice device,
    uint32_t queuefamily);
VkResult _glfwPlatformCreateWindowSurface(VkInstance instance,
    _GLFWwindow* window,
    const VkAllocationCallbacks* allocator,
    VkSurfaceKHR* surface);

GLFWbool _glfwPlatformCreateTls(_GLFWtls* tls);
void _glfwPlatformDestroyTls(_GLFWtls* tls);
void* _glfwPlatformGetTls(_GLFWtls* tls);
void _glfwPlatformSetTls(_GLFWtls* tls, void* value);

GLFWbool _glfwPlatformCreateMutex(_GLFWmutex* mutex);
void _glfwPlatformDestroyMutex(_GLFWmutex* mutex);
void _glfwPlatformLockMutex(_GLFWmutex* mutex);
void _glfwPlatformUnlockMutex(_GLFWmutex* mutex);


//////////////////////////////////////////////////////////////////////////
//////                         GLFW event API                       //////
//////////////////////////////////////////////////////////////////////////

void _glfwInputWindowFocus(_GLFWwindow* window, GLFWbool focused);
void _glfwInputWindowPos(_GLFWwindow* window, int xpos, int ypos);
void _glfwInputWindowSize(_GLFWwindow* window, int width, int height);
void _glfwInputFramebufferSize(_GLFWwindow* window, int width, int height);
void _glfwInputWindowContentScale(_GLFWwindow* window,
    float xscale, float yscale);
void _glfwInputWindowIconify(_GLFWwindow* window, GLFWbool iconified);
void _glfwInputWindowMaximize(_GLFWwindow* window, GLFWbool maximized);
void _glfwInputWindowDamage(_GLFWwindow* window);
void _glfwInputWindowCloseRequest(_GLFWwindow* window);
void _glfwInputWindowMonitor(_GLFWwindow* window, _GLFWmonitor* monitor);

void _glfwInputKey(_GLFWwindow* window,
    int key, int scancode, int action, int mods);
void _glfwInputChar(_GLFWwindow* window,
    uint32_t codepoint, int mods, GLFWbool plain);
void _glfwInputScroll(_GLFWwindow* window, double xoffset, double yoffset);
void _glfwInputMouseClick(_GLFWwindow* window, int button, int action, int mods);
void _glfwInputCursorPos(_GLFWwindow* window, double xpos, double ypos);
void _glfwInputCursorEnter(_GLFWwindow* window, GLFWbool entered);
void _glfwInputDrop(_GLFWwindow* window, int count, const char** names);


void _glfwInputMonitor(_GLFWmonitor* monitor, int action, int placement);
void _glfwInputMonitorWindow(_GLFWmonitor* monitor, _GLFWwindow* window);

#if defined(__GNUC__)
void _glfwInputError(int code, const char* format, ...)
__attribute__((format(printf, 2, 3)));
#else
void _glfwInputError(int code, const char* format, ...);
#endif


//////////////////////////////////////////////////////////////////////////
//////                       GLFW internal API                      //////
//////////////////////////////////////////////////////////////////////////

GLFWbool _glfwStringInExtensionString(const char* string, const char* extensions);
const _GLFWfbconfig* _glfwChooseFBConfig(const _GLFWfbconfig* desired,
    const _GLFWfbconfig* alternatives,
    unsigned int count);
GLFWbool _glfwRefreshContextAttribs(_GLFWwindow* window,
    const _GLFWctxconfig* ctxconfig);
GLFWbool _glfwIsValidContextConfig(const _GLFWctxconfig* ctxconfig);

const GLFWvidmode* _glfwChooseVideoMode(_GLFWmonitor* monitor,
    const GLFWvidmode* desired);
int _glfwCompareVideoModes(const GLFWvidmode* first, const GLFWvidmode* second);
_GLFWmonitor* _glfwAllocMonitor(const char* name, int widthMM, int heightMM);
void _glfwFreeMonitor(_GLFWmonitor* monitor);
void _glfwAllocGammaArrays(GLFWgammaramp* ramp, unsigned int size);
void _glfwFreeGammaArrays(GLFWgammaramp* ramp);
void _glfwSplitBPP(int bpp, int* red, int* green, int* blue);
void _glfwCenterCursorInContentArea(_GLFWwindow* window);

char* _glfw_strdup(const char* source);
int _glfw_min(int a, int b);
int _glfw_max(int a, int b);
float _glfw_fminf(float a, float b);
float _glfw_fmaxf(float a, float b);


// Return the value corresponding to the specified attribute
//
static int findPixelFormatAttribValue(const int* attribs,
    int attribCount,
    const int* values,
    int attrib)
{
    int i;

    for (i = 0; i < attribCount; i++)
    {
        if (attribs[i] == attrib)
            return values[i];
    }

    _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
        "WGL: Unknown pixel format attribute requested");
    return 0;
}

#define addAttrib(a) \
{ \
    assert((size_t) attribCount < sizeof(attribs) / sizeof(attribs[0])); \
    attribs[attribCount++] = a; \
}
#define findAttribValue(a) \
    findPixelFormatAttribValue(attribs, attribCount, values, a)

// Return a list of available and usable framebuffer configs
//
static int choosePixelFormat(_GLFWwindow* window,
    const _GLFWctxconfig* ctxconfig,
    const _GLFWfbconfig* fbconfig)
{
    _GLFWfbconfig* usableConfigs;
    const _GLFWfbconfig* closest;
    int i, pixelFormat, nativeCount, usableCount = 0, attribCount = 0;
    int attribs[40];
    int values[sizeof(attribs) / sizeof(attribs[0])];

    if (_glfw.wgl.ARB_pixel_format)
    {
        const int attrib = WGL_NUMBER_PIXEL_FORMATS_ARB;

        if (!wglGetPixelFormatAttribivARB(window->context.wgl.dc,
            1, 0, 1, &attrib, &nativeCount))
        {
            _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
                "WGL: Failed to retrieve pixel format attribute");
            return 0;
        }

        addAttrib(WGL_SUPPORT_OPENGL_ARB);
        addAttrib(WGL_DRAW_TO_WINDOW_ARB);
        addAttrib(WGL_PIXEL_TYPE_ARB);
        addAttrib(WGL_ACCELERATION_ARB);
        addAttrib(WGL_RED_BITS_ARB);
        addAttrib(WGL_RED_SHIFT_ARB);
        addAttrib(WGL_GREEN_BITS_ARB);
        addAttrib(WGL_GREEN_SHIFT_ARB);
        addAttrib(WGL_BLUE_BITS_ARB);
        addAttrib(WGL_BLUE_SHIFT_ARB);
        addAttrib(WGL_ALPHA_BITS_ARB);
        addAttrib(WGL_ALPHA_SHIFT_ARB);
        addAttrib(WGL_DEPTH_BITS_ARB);
        addAttrib(WGL_STENCIL_BITS_ARB);
        addAttrib(WGL_ACCUM_BITS_ARB);
        addAttrib(WGL_ACCUM_RED_BITS_ARB);
        addAttrib(WGL_ACCUM_GREEN_BITS_ARB);
        addAttrib(WGL_ACCUM_BLUE_BITS_ARB);
        addAttrib(WGL_ACCUM_ALPHA_BITS_ARB);
        addAttrib(WGL_AUX_BUFFERS_ARB);
        addAttrib(WGL_STEREO_ARB);
        addAttrib(WGL_DOUBLE_BUFFER_ARB);

        if (_glfw.wgl.ARB_multisample)
            addAttrib(WGL_SAMPLES_ARB);

        if (ctxconfig->client == GLFW_OPENGL_API)
        {
            if (_glfw.wgl.ARB_framebuffer_sRGB || _glfw.wgl.EXT_framebuffer_sRGB)
                addAttrib(WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB);
        }
        else
        {
            if (_glfw.wgl.EXT_colorspace)
                addAttrib(WGL_COLORSPACE_EXT);
        }
    }
    else
    {
        nativeCount = DescribePixelFormat(window->context.wgl.dc,
            1,
            sizeof(PIXELFORMATDESCRIPTOR),
            NULL);
    }

    usableConfigs = (_GLFWfbconfig*)calloc(nativeCount, sizeof(_GLFWfbconfig));

    for (i = 0; i < nativeCount; i++)
    {
        _GLFWfbconfig* u = usableConfigs + usableCount;
        pixelFormat = i + 1;

        if (_glfw.wgl.ARB_pixel_format)
        {
            // Get pixel format attributes through "modern" extension

            if (!wglGetPixelFormatAttribivARB(window->context.wgl.dc,
                pixelFormat, 0,
                attribCount,
                attribs, values))
            {
                _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
                    "WGL: Failed to retrieve pixel format attributes");

                free(usableConfigs);
                return 0;
            }

            if (!findAttribValue(WGL_SUPPORT_OPENGL_ARB) ||
                !findAttribValue(WGL_DRAW_TO_WINDOW_ARB))
            {
                continue;
            }

            if (findAttribValue(WGL_PIXEL_TYPE_ARB) != WGL_TYPE_RGBA_ARB)
                continue;

            if (findAttribValue(WGL_ACCELERATION_ARB) == WGL_NO_ACCELERATION_ARB)
                continue;

            if (findAttribValue(WGL_DOUBLE_BUFFER_ARB) != fbconfig->doublebuffer)
                continue;

            u->redBits = findAttribValue(WGL_RED_BITS_ARB);
            u->greenBits = findAttribValue(WGL_GREEN_BITS_ARB);
            u->blueBits = findAttribValue(WGL_BLUE_BITS_ARB);
            u->alphaBits = findAttribValue(WGL_ALPHA_BITS_ARB);

            u->depthBits = findAttribValue(WGL_DEPTH_BITS_ARB);
            u->stencilBits = findAttribValue(WGL_STENCIL_BITS_ARB);

            u->accumRedBits = findAttribValue(WGL_ACCUM_RED_BITS_ARB);
            u->accumGreenBits = findAttribValue(WGL_ACCUM_GREEN_BITS_ARB);
            u->accumBlueBits = findAttribValue(WGL_ACCUM_BLUE_BITS_ARB);
            u->accumAlphaBits = findAttribValue(WGL_ACCUM_ALPHA_BITS_ARB);

            u->auxBuffers = findAttribValue(WGL_AUX_BUFFERS_ARB);

            if (findAttribValue(WGL_STEREO_ARB))
                u->stereo = GLFW_TRUE;

            if (_glfw.wgl.ARB_multisample)
                u->samples = findAttribValue(WGL_SAMPLES_ARB);

            if (ctxconfig->client == GLFW_OPENGL_API)
            {
                if (_glfw.wgl.ARB_framebuffer_sRGB ||
                    _glfw.wgl.EXT_framebuffer_sRGB)
                {
                    if (findAttribValue(WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB))
                        u->sRGB = GLFW_TRUE;
                }
            }
            else
            {
                if (_glfw.wgl.EXT_colorspace)
                {
                    if (findAttribValue(WGL_COLORSPACE_EXT) == WGL_COLORSPACE_SRGB_EXT)
                        u->sRGB = GLFW_TRUE;
                }
            }
        }
        else
        {
            // Get pixel format attributes through legacy PFDs

            PIXELFORMATDESCRIPTOR pfd;

            if (!DescribePixelFormat(window->context.wgl.dc,
                pixelFormat,
                sizeof(PIXELFORMATDESCRIPTOR),
                &pfd))
            {
                _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
                    "WGL: Failed to describe pixel format");

                free(usableConfigs);
                return 0;
            }

            if (!(pfd.dwFlags & PFD_DRAW_TO_WINDOW) ||
                !(pfd.dwFlags & PFD_SUPPORT_OPENGL))
            {
                continue;
            }

            if (!(pfd.dwFlags & PFD_GENERIC_ACCELERATED) &&
                (pfd.dwFlags & PFD_GENERIC_FORMAT))
            {
                continue;
            }

            if (pfd.iPixelType != PFD_TYPE_RGBA)
                continue;

            if (!!(pfd.dwFlags & PFD_DOUBLEBUFFER) != fbconfig->doublebuffer)
                continue;

            u->redBits = pfd.cRedBits;
            u->greenBits = pfd.cGreenBits;
            u->blueBits = pfd.cBlueBits;
            u->alphaBits = pfd.cAlphaBits;

            u->depthBits = pfd.cDepthBits;
            u->stencilBits = pfd.cStencilBits;

            u->accumRedBits = pfd.cAccumRedBits;
            u->accumGreenBits = pfd.cAccumGreenBits;
            u->accumBlueBits = pfd.cAccumBlueBits;
            u->accumAlphaBits = pfd.cAccumAlphaBits;

            u->auxBuffers = pfd.cAuxBuffers;

            if (pfd.dwFlags & PFD_STEREO)
                u->stereo = GLFW_TRUE;
        }

        u->handle = pixelFormat;
        usableCount++;
    }

    if (!usableCount)
    {
        _glfwInputError(GLFW_API_UNAVAILABLE,
            "WGL: The driver does not appear to support OpenGL");

        free(usableConfigs);
        return 0;
    }

    closest = _glfwChooseFBConfig(fbconfig, usableConfigs, usableCount);
    if (!closest)
    {
        _glfwInputError(GLFW_FORMAT_UNAVAILABLE,
            "WGL: Failed to find a suitable pixel format");

        free(usableConfigs);
        return 0;
    }

    pixelFormat = (int)closest->handle;
    free(usableConfigs);

    return pixelFormat;
}

#undef addAttrib
#undef findAttribValue

static void makeContextCurrentWGL(_GLFWwindow* window)
{
    if (window)
    {
        if (wglMakeCurrent(window->context.wgl.dc, window->context.wgl.handle))
            _glfwPlatformSetTls(&_glfw.contextSlot, window);
        else
        {
            _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
                "WGL: Failed to make context current");
            _glfwPlatformSetTls(&_glfw.contextSlot, NULL);
        }
    }
    else
    {
        if (!wglMakeCurrent(NULL, NULL))
        {
            _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
                "WGL: Failed to clear current context");
        }

        _glfwPlatformSetTls(&_glfw.contextSlot, NULL);
    }
}

static void swapBuffersWGL(_GLFWwindow* window)
{
    if (!window->monitor)
    {
        if (IsWindowsVistaOrGreater())
        {
            // DWM Composition is always enabled on Win8+
            BOOL enabled = IsWindows8OrGreater();

            // HACK: Use DwmFlush when desktop composition is enabled
            if (enabled ||
                (SUCCEEDED(DwmIsCompositionEnabled(&enabled)) && enabled))
            {
                int count = abs(window->context.wgl.interval);
                while (count--)
                    DwmFlush();
            }
        }
    }

    SwapBuffers(window->context.wgl.dc);
}

static void swapIntervalWGL(int interval)
{
    _GLFWwindow* window = (_GLFWwindow * )_glfwPlatformGetTls(&_glfw.contextSlot);

    window->context.wgl.interval = interval;

    if (!window->monitor)
    {
        if (IsWindowsVistaOrGreater())
        {
            // DWM Composition is always enabled on Win8+
            BOOL enabled = IsWindows8OrGreater();

            // HACK: Disable WGL swap interval when desktop composition is enabled to
            //       avoid interfering with DWM vsync
            if (enabled ||
                (SUCCEEDED(DwmIsCompositionEnabled(&enabled)) && enabled))
                interval = 0;
        }
    }

    if (_glfw.wgl.EXT_swap_control)
        wglSwapIntervalEXT(interval);
}

static int extensionSupportedWGL(const char* extension)
{
    const char* extensions = NULL;

    if (wglGetExtensionsStringARB)
        extensions = wglGetExtensionsStringARB(wglGetCurrentDC());
    else if (wglGetExtensionsStringEXT)
        extensions = wglGetExtensionsStringEXT();

    if (!extensions)
        return GLFW_FALSE;

    return _glfwStringInExtensionString(extension, extensions);
}

static GLFWglproc getProcAddressWGL(const char* procname)
{
    const GLFWglproc proc = (GLFWglproc)wglGetProcAddress(procname);
    if (proc)
        return proc;

    return (GLFWglproc)GetProcAddress(_glfw.wgl.instance, procname);
}

static void destroyContextWGL(_GLFWwindow* window)
{
    if (window->context.wgl.handle)
    {
        wglDeleteContext(window->context.wgl.handle);
        window->context.wgl.handle = NULL;
    }
}


//////////////////////////////////////////////////////////////////////////
//////                       GLFW internal API                      //////
//////////////////////////////////////////////////////////////////////////

// Initialize WGL
//
GLFWbool _glfwInitWGL(void)
{
    PIXELFORMATDESCRIPTOR pfd;
    HGLRC prc, rc;
    HDC pdc, dc;

    if (_glfw.wgl.instance)
        return GLFW_TRUE;

    _glfw.wgl.instance = LibInst::GetInst().m_libOpengl32->Ptr();
    if (!_glfw.wgl.instance)
    {
        _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
            "WGL: Failed to load opengl32.dll");
        return GLFW_FALSE;
    }

    dc = GetDC(_glfw.win32.helperWindowHandle);

    ZeroMemory(&pfd, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;

    if (!SetPixelFormat(dc, ChoosePixelFormat(dc, &pfd), &pfd))
    {
        _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
            "WGL: Failed to set pixel format for dummy context");
        return GLFW_FALSE;
    }

    rc = wglCreateContext(dc);
    if (!rc)
    {
        _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
            "WGL: Failed to create dummy context");
        return GLFW_FALSE;
    }

    pdc = wglGetCurrentDC();
    prc = wglGetCurrentContext();

    if (!wglMakeCurrent(dc, rc))
    {
        _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
            "WGL: Failed to make dummy context current");
        wglMakeCurrent(pdc, prc);
        wglDeleteContext(rc);
        return GLFW_FALSE;
    }

    //// NOTE: Functions must be loaded first as they're needed to retrieve the
    ////       extension string that tells us whether the functions are supported
    //_glfw.wgl.GetExtensionsStringEXT = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)
    //    wglGetProcAddress("wglGetExtensionsStringEXT");
    //_glfw.wgl.GetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)
    //    wglGetProcAddress("wglGetExtensionsStringARB");
    //_glfw.wgl.CreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)
    //    wglGetProcAddress("wglCreateContextAttribsARB");
    //_glfw.wgl.SwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)
    //    wglGetProcAddress("wglSwapIntervalEXT");
    //_glfw.wgl.GetPixelFormatAttribivARB = (PFNWGLGETPIXELFORMATATTRIBIVARBPROC)
    //    wglGetProcAddress("wglGetPixelFormatAttribivARB");

    // NOTE: WGL_ARB_extensions_string and WGL_EXT_extensions_string are not
    //       checked below as we are already using them
    _glfw.wgl.ARB_multisample =
        extensionSupportedWGL("WGL_ARB_multisample");
    _glfw.wgl.ARB_framebuffer_sRGB =
        extensionSupportedWGL("WGL_ARB_framebuffer_sRGB");
    _glfw.wgl.EXT_framebuffer_sRGB =
        extensionSupportedWGL("WGL_EXT_framebuffer_sRGB");
    _glfw.wgl.ARB_create_context =
        extensionSupportedWGL("WGL_ARB_create_context");
    _glfw.wgl.ARB_create_context_profile =
        extensionSupportedWGL("WGL_ARB_create_context_profile");
    _glfw.wgl.EXT_create_context_es2_profile =
        extensionSupportedWGL("WGL_EXT_create_context_es2_profile");
    _glfw.wgl.ARB_create_context_robustness =
        extensionSupportedWGL("WGL_ARB_create_context_robustness");
    _glfw.wgl.ARB_create_context_no_error =
        extensionSupportedWGL("WGL_ARB_create_context_no_error");
    _glfw.wgl.EXT_swap_control =
        extensionSupportedWGL("WGL_EXT_swap_control");
    _glfw.wgl.EXT_colorspace =
        extensionSupportedWGL("WGL_EXT_colorspace");
    _glfw.wgl.ARB_pixel_format =
        extensionSupportedWGL("WGL_ARB_pixel_format");
    _glfw.wgl.ARB_context_flush_control =
        extensionSupportedWGL("WGL_ARB_context_flush_control");

    wglMakeCurrent(pdc, prc);
    wglDeleteContext(rc);
    return GLFW_TRUE;
}

// Terminate WGL
//
void _glfwTerminateWGL(void)
{
    if (_glfw.wgl.instance)
        FreeLibrary(_glfw.wgl.instance);
}

#define setAttrib(a, v) \
{ \
    assert(((size_t) index + 1) < sizeof(attribs) / sizeof(attribs[0])); \
    attribs[index++] = a; \
    attribs[index++] = v; \
}

// Create the OpenGL or OpenGL ES context
//
GLFWbool _glfwCreateContextWGL(_GLFWwindow* window,
    const _GLFWctxconfig* ctxconfig,
    const _GLFWfbconfig* fbconfig)
{
    int attribs[40];
    int pixelFormat;
    PIXELFORMATDESCRIPTOR pfd;
    HGLRC share = NULL;

    if (ctxconfig->share)
        share = ctxconfig->share->context.wgl.handle;

    window->context.wgl.dc = GetDC(window->win32.handle);
    if (!window->context.wgl.dc)
    {
        _glfwInputError(GLFW_PLATFORM_ERROR,
            "WGL: Failed to retrieve DC for window");
        return GLFW_FALSE;
    }

    pixelFormat = choosePixelFormat(window, ctxconfig, fbconfig);
    if (!pixelFormat)
        return GLFW_FALSE;

    if (!DescribePixelFormat(window->context.wgl.dc,
        pixelFormat, sizeof(pfd), &pfd))
    {
        _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
            "WGL: Failed to retrieve PFD for selected pixel format");
        return GLFW_FALSE;
    }

    if (!SetPixelFormat(window->context.wgl.dc, pixelFormat, &pfd))
    {
        _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
            "WGL: Failed to set selected pixel format");
        return GLFW_FALSE;
    }

    if (ctxconfig->client == GLFW_OPENGL_API)
    {
        if (ctxconfig->forward)
        {
            if (!_glfw.wgl.ARB_create_context)
            {
                _glfwInputError(GLFW_VERSION_UNAVAILABLE,
                    "WGL: A forward compatible OpenGL context requested but WGL_ARB_create_context is unavailable");
                return GLFW_FALSE;
            }
        }

        if (ctxconfig->profile)
        {
            if (!_glfw.wgl.ARB_create_context_profile)
            {
                _glfwInputError(GLFW_VERSION_UNAVAILABLE,
                    "WGL: OpenGL profile requested but WGL_ARB_create_context_profile is unavailable");
                return GLFW_FALSE;
            }
        }
    }
    else
    {
        if (!_glfw.wgl.ARB_create_context ||
            !_glfw.wgl.ARB_create_context_profile ||
            !_glfw.wgl.EXT_create_context_es2_profile)
        {
            _glfwInputError(GLFW_API_UNAVAILABLE,
                "WGL: OpenGL ES requested but WGL_ARB_create_context_es2_profile is unavailable");
            return GLFW_FALSE;
        }
    }

    if (_glfw.wgl.ARB_create_context)
    {
        int index = 0, mask = 0, flags = 0;

        if (ctxconfig->client == GLFW_OPENGL_API)
        {
            if (ctxconfig->forward)
                flags |= WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;

            if (ctxconfig->profile == GLFW_OPENGL_CORE_PROFILE)
                mask |= WGL_CONTEXT_CORE_PROFILE_BIT_ARB;
            else if (ctxconfig->profile == GLFW_OPENGL_COMPAT_PROFILE)
                mask |= WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB;
        }
        else
            mask |= WGL_CONTEXT_ES2_PROFILE_BIT_EXT;

        if (ctxconfig->debug)
            flags |= WGL_CONTEXT_DEBUG_BIT_ARB;

        if (ctxconfig->robustness)
        {
            if (_glfw.wgl.ARB_create_context_robustness)
            {
                if (ctxconfig->robustness == GLFW_NO_RESET_NOTIFICATION)
                {
                    setAttrib(WGL_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB,
                        WGL_NO_RESET_NOTIFICATION_ARB);
                }
                else if (ctxconfig->robustness == GLFW_LOSE_CONTEXT_ON_RESET)
                {
                    setAttrib(WGL_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB,
                        WGL_LOSE_CONTEXT_ON_RESET_ARB);
                }

                flags |= WGL_CONTEXT_ROBUST_ACCESS_BIT_ARB;
            }
        }

        if (ctxconfig->release)
        {
            if (_glfw.wgl.ARB_context_flush_control)
            {
                if (ctxconfig->release == GLFW_RELEASE_BEHAVIOR_NONE)
                {
                    setAttrib(WGL_CONTEXT_RELEASE_BEHAVIOR_ARB,
                        WGL_CONTEXT_RELEASE_BEHAVIOR_NONE_ARB);
                }
                else if (ctxconfig->release == GLFW_RELEASE_BEHAVIOR_FLUSH)
                {
                    setAttrib(WGL_CONTEXT_RELEASE_BEHAVIOR_ARB,
                        WGL_CONTEXT_RELEASE_BEHAVIOR_FLUSH_ARB);
                }
            }
        }

        if (ctxconfig->noerror)
        {
            if (_glfw.wgl.ARB_create_context_no_error)
                setAttrib(WGL_CONTEXT_OPENGL_NO_ERROR_ARB, GLFW_TRUE);
        }

        // NOTE: Only request an explicitly versioned context when necessary, as
        //       explicitly requesting version 1.0 does not always return the
        //       highest version supported by the driver
        if (ctxconfig->major != 1 || ctxconfig->minor != 0)
        {
            setAttrib(WGL_CONTEXT_MAJOR_VERSION_ARB, ctxconfig->major);
            setAttrib(WGL_CONTEXT_MINOR_VERSION_ARB, ctxconfig->minor);
        }

        if (flags)
            setAttrib(WGL_CONTEXT_FLAGS_ARB, flags);

        if (mask)
            setAttrib(WGL_CONTEXT_PROFILE_MASK_ARB, mask);

        setAttrib(0, 0);

        window->context.wgl.handle =
            wglCreateContextAttribsARB(window->context.wgl.dc, share, attribs);
        if (!window->context.wgl.handle)
        {
            const DWORD error = GetLastError();

            if (error == (0xc0070000 | ERROR_INVALID_VERSION_ARB))
            {
                if (ctxconfig->client == GLFW_OPENGL_API)
                {
                    _glfwInputError(GLFW_VERSION_UNAVAILABLE,
                        "WGL: Driver does not support OpenGL version %i.%i",
                        ctxconfig->major,
                        ctxconfig->minor);
                }
                else
                {
                    _glfwInputError(GLFW_VERSION_UNAVAILABLE,
                        "WGL: Driver does not support OpenGL ES version %i.%i",
                        ctxconfig->major,
                        ctxconfig->minor);
                }
            }
            else if (error == (0xc0070000 | ERROR_INVALID_PROFILE_ARB))
            {
                _glfwInputError(GLFW_VERSION_UNAVAILABLE,
                    "WGL: Driver does not support the requested OpenGL profile");
            }
            else if (error == (0xc0070000 | ERROR_INCOMPATIBLE_DEVICE_CONTEXTS_ARB))
            {
                _glfwInputError(GLFW_INVALID_VALUE,
                    "WGL: The share context is not compatible with the requested context");
            }
            else
            {
                if (ctxconfig->client == GLFW_OPENGL_API)
                {
                    _glfwInputError(GLFW_VERSION_UNAVAILABLE,
                        "WGL: Failed to create OpenGL context");
                }
                else
                {
                    _glfwInputError(GLFW_VERSION_UNAVAILABLE,
                        "WGL: Failed to create OpenGL ES context");
                }
            }

            return GLFW_FALSE;
        }
    }
    else
    {
        window->context.wgl.handle = wglCreateContext(window->context.wgl.dc);
        if (!window->context.wgl.handle)
        {
            _glfwInputErrorWin32(GLFW_VERSION_UNAVAILABLE,
                "WGL: Failed to create OpenGL context");
            return GLFW_FALSE;
        }

        if (share)
        {
            if (!wglShareLists(share, window->context.wgl.handle))
            {
                _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
                    "WGL: Failed to enable sharing with specified OpenGL context");
                return GLFW_FALSE;
            }
        }
    }

    window->context.makeCurrent = makeContextCurrentWGL;
    window->context.swapBuffers = swapBuffersWGL;
    window->context.swapInterval = swapIntervalWGL;
    window->context.extensionSupported = extensionSupportedWGL;
    window->context.getProcAddress = getProcAddressWGL;
    window->context.destroy = destroyContextWGL;

    return GLFW_TRUE;
}

#undef setAttrib


//////////////////////////////////////////////////////////////////////////
//////                        GLFW native API                       //////
//////////////////////////////////////////////////////////////////////////

GLFWAPI HGLRC glfwGetWGLContext(GLFWwindow* handle)
{
    _GLFWwindow* window = (_GLFWwindow*)handle;
    _GLFW_REQUIRE_INIT_OR_RETURN(NULL);

    if (window->context.source != GLFW_NATIVE_CONTEXT_API)
    {
        _glfwInputError(GLFW_NO_WINDOW_CONTEXT, NULL);
        return NULL;
    }

    return window->context.wgl.handle;
}




GLFWbool _glfwIsValidContextConfig(const _GLFWctxconfig* ctxconfig)
{
    if (ctxconfig->share)
    {
        if (ctxconfig->client == GLFW_NO_API ||
            ctxconfig->share->context.client == GLFW_NO_API)
        {
            _glfwInputError(GLFW_NO_WINDOW_CONTEXT, NULL);
            return GLFW_FALSE;
        }
    }

    if (ctxconfig->source != GLFW_NATIVE_CONTEXT_API &&
        ctxconfig->source != GLFW_EGL_CONTEXT_API &&
        ctxconfig->source != GLFW_OSMESA_CONTEXT_API)
    {
        _glfwInputError(GLFW_INVALID_ENUM,
            "Invalid context creation API 0x%08X",
            ctxconfig->source);
        return GLFW_FALSE;
    }

    if (ctxconfig->client != GLFW_NO_API &&
        ctxconfig->client != GLFW_OPENGL_API &&
        ctxconfig->client != GLFW_OPENGL_ES_API)
    {
        _glfwInputError(GLFW_INVALID_ENUM,
            "Invalid client API 0x%08X",
            ctxconfig->client);
        return GLFW_FALSE;
    }

    if (ctxconfig->client == GLFW_OPENGL_API)
    {
        if ((ctxconfig->major < 1 || ctxconfig->minor < 0) ||
            (ctxconfig->major == 1 && ctxconfig->minor > 5) ||
            (ctxconfig->major == 2 && ctxconfig->minor > 1) ||
            (ctxconfig->major == 3 && ctxconfig->minor > 3))
        {
            // OpenGL 1.0 is the smallest valid version
            // OpenGL 1.x series ended with version 1.5
            // OpenGL 2.x series ended with version 2.1
            // OpenGL 3.x series ended with version 3.3
            // For now, let everything else through

            _glfwInputError(GLFW_INVALID_VALUE,
                "Invalid OpenGL version %i.%i",
                ctxconfig->major, ctxconfig->minor);
            return GLFW_FALSE;
        }

        if (ctxconfig->profile)
        {
            if (ctxconfig->profile != GLFW_OPENGL_CORE_PROFILE &&
                ctxconfig->profile != GLFW_OPENGL_COMPAT_PROFILE)
            {
                _glfwInputError(GLFW_INVALID_ENUM,
                    "Invalid OpenGL profile 0x%08X",
                    ctxconfig->profile);
                return GLFW_FALSE;
            }

            if (ctxconfig->major <= 2 ||
                (ctxconfig->major == 3 && ctxconfig->minor < 2))
            {
                // Desktop OpenGL context profiles are only defined for version 3.2
                // and above

                _glfwInputError(GLFW_INVALID_VALUE,
                    "Context profiles are only defined for OpenGL version 3.2 and above");
                return GLFW_FALSE;
            }
        }

        if (ctxconfig->forward && ctxconfig->major <= 2)
        {
            // Forward-compatible contexts are only defined for OpenGL version 3.0 and above
            _glfwInputError(GLFW_INVALID_VALUE,
                "Forward-compatibility is only defined for OpenGL version 3.0 and above");
            return GLFW_FALSE;
        }
    }
    else if (ctxconfig->client == GLFW_OPENGL_ES_API)
    {
        if (ctxconfig->major < 1 || ctxconfig->minor < 0 ||
            (ctxconfig->major == 1 && ctxconfig->minor > 1) ||
            (ctxconfig->major == 2 && ctxconfig->minor > 0))
        {
            // OpenGL ES 1.0 is the smallest valid version
            // OpenGL ES 1.x series ended with version 1.1
            // OpenGL ES 2.x series ended with version 2.0
            // For now, let everything else through

            _glfwInputError(GLFW_INVALID_VALUE,
                "Invalid OpenGL ES version %i.%i",
                ctxconfig->major, ctxconfig->minor);
            return GLFW_FALSE;
        }
    }

    if (ctxconfig->robustness)
    {
        if (ctxconfig->robustness != GLFW_NO_RESET_NOTIFICATION &&
            ctxconfig->robustness != GLFW_LOSE_CONTEXT_ON_RESET)
        {
            _glfwInputError(GLFW_INVALID_ENUM,
                "Invalid context robustness mode 0x%08X",
                ctxconfig->robustness);
            return GLFW_FALSE;
        }
    }

    if (ctxconfig->release)
    {
        if (ctxconfig->release != GLFW_RELEASE_BEHAVIOR_NONE &&
            ctxconfig->release != GLFW_RELEASE_BEHAVIOR_FLUSH)
        {
            _glfwInputError(GLFW_INVALID_ENUM,
                "Invalid context release behavior 0x%08X",
                ctxconfig->release);
            return GLFW_FALSE;
        }
    }

    return GLFW_TRUE;
}

// Chooses the framebuffer config that best matches the desired one
//
const _GLFWfbconfig* _glfwChooseFBConfig(const _GLFWfbconfig* desired,
    const _GLFWfbconfig* alternatives,
    unsigned int count)
{
    unsigned int i;
    unsigned int missing, leastMissing = UINT_MAX;
    unsigned int colorDiff, leastColorDiff = UINT_MAX;
    unsigned int extraDiff, leastExtraDiff = UINT_MAX;
    const _GLFWfbconfig* current;
    const _GLFWfbconfig* closest = NULL;

    for (i = 0; i < count; i++)
    {
        current = alternatives + i;

        if (desired->stereo > 0 && current->stereo == 0)
        {
            // Stereo is a hard constraint
            continue;
        }

        // Count number of missing buffers
        {
            missing = 0;

            if (desired->alphaBits > 0 && current->alphaBits == 0)
                missing++;

            if (desired->depthBits > 0 && current->depthBits == 0)
                missing++;

            if (desired->stencilBits > 0 && current->stencilBits == 0)
                missing++;

            if (desired->auxBuffers > 0 &&
                current->auxBuffers < desired->auxBuffers)
            {
                missing += desired->auxBuffers - current->auxBuffers;
            }

            if (desired->samples > 0 && current->samples == 0)
            {
                // Technically, several multisampling buffers could be
                // involved, but that's a lower level implementation detail and
                // not important to us here, so we count them as one
                missing++;
            }

            if (desired->transparent != current->transparent)
                missing++;
        }

        // These polynomials make many small channel size differences matter
        // less than one large channel size difference

        // Calculate color channel size difference value
        {
            colorDiff = 0;

            if (desired->redBits != GLFW_DONT_CARE)
            {
                colorDiff += (desired->redBits - current->redBits) *
                    (desired->redBits - current->redBits);
            }

            if (desired->greenBits != GLFW_DONT_CARE)
            {
                colorDiff += (desired->greenBits - current->greenBits) *
                    (desired->greenBits - current->greenBits);
            }

            if (desired->blueBits != GLFW_DONT_CARE)
            {
                colorDiff += (desired->blueBits - current->blueBits) *
                    (desired->blueBits - current->blueBits);
            }
        }

        // Calculate non-color channel size difference value
        {
            extraDiff = 0;

            if (desired->alphaBits != GLFW_DONT_CARE)
            {
                extraDiff += (desired->alphaBits - current->alphaBits) *
                    (desired->alphaBits - current->alphaBits);
            }

            if (desired->depthBits != GLFW_DONT_CARE)
            {
                extraDiff += (desired->depthBits - current->depthBits) *
                    (desired->depthBits - current->depthBits);
            }

            if (desired->stencilBits != GLFW_DONT_CARE)
            {
                extraDiff += (desired->stencilBits - current->stencilBits) *
                    (desired->stencilBits - current->stencilBits);
            }

            if (desired->accumRedBits != GLFW_DONT_CARE)
            {
                extraDiff += (desired->accumRedBits - current->accumRedBits) *
                    (desired->accumRedBits - current->accumRedBits);
            }

            if (desired->accumGreenBits != GLFW_DONT_CARE)
            {
                extraDiff += (desired->accumGreenBits - current->accumGreenBits) *
                    (desired->accumGreenBits - current->accumGreenBits);
            }

            if (desired->accumBlueBits != GLFW_DONT_CARE)
            {
                extraDiff += (desired->accumBlueBits - current->accumBlueBits) *
                    (desired->accumBlueBits - current->accumBlueBits);
            }

            if (desired->accumAlphaBits != GLFW_DONT_CARE)
            {
                extraDiff += (desired->accumAlphaBits - current->accumAlphaBits) *
                    (desired->accumAlphaBits - current->accumAlphaBits);
            }

            if (desired->samples != GLFW_DONT_CARE)
            {
                extraDiff += (desired->samples - current->samples) *
                    (desired->samples - current->samples);
            }

            if (desired->sRGB && !current->sRGB)
                extraDiff++;
        }

        // Figure out if the current one is better than the best one found so far
        // Least number of missing buffers is the most important heuristic,
        // then color buffer size match and lastly size match for other buffers

        if (missing < leastMissing)
            closest = current;
        else if (missing == leastMissing)
        {
            if ((colorDiff < leastColorDiff) ||
                (colorDiff == leastColorDiff && extraDiff < leastExtraDiff))
            {
                closest = current;
            }
        }

        if (current == closest)
        {
            leastMissing = missing;
            leastColorDiff = colorDiff;
            leastExtraDiff = extraDiff;
        }
    }

    return closest;
}

// Retrieves the attributes of the current context
//
GLFWbool _glfwRefreshContextAttribs(_GLFWwindow* window,
    const _GLFWctxconfig* ctxconfig)
{
    int i;
    _GLFWwindow* previous;
    const char* version;
    const char* prefixes[] =
    {
        "OpenGL ES-CM ",
        "OpenGL ES-CL ",
        "OpenGL ES ",
        NULL
    };

    window->context.source = ctxconfig->source;
    window->context.client = GLFW_OPENGL_API;

    previous = (_GLFWwindow * )_glfwPlatformGetTls(&_glfw.contextSlot);
    glfwMakeContextCurrent((GLFWwindow*)window);


    if (!glGetIntegerv || !glGetString)
    {
        _glfwInputError(GLFW_PLATFORM_ERROR, "Entry point retrieval is broken");
        glfwMakeContextCurrent((GLFWwindow*)previous);
        return GLFW_FALSE;
    }

    version = (const char*)glGetString(GL_VERSION);
    if (!version)
    {
        if (ctxconfig->client == GLFW_OPENGL_API)
        {
            _glfwInputError(GLFW_PLATFORM_ERROR,
                "OpenGL version string retrieval is broken");
        }
        else
        {
            _glfwInputError(GLFW_PLATFORM_ERROR,
                "OpenGL ES version string retrieval is broken");
        }

        glfwMakeContextCurrent((GLFWwindow*)previous);
        return GLFW_FALSE;
    }

    for (i = 0; prefixes[i]; i++)
    {
        const size_t length = strlen(prefixes[i]);

        if (strncmp(version, prefixes[i], length) == 0)
        {
            version += length;
            window->context.client = GLFW_OPENGL_ES_API;
            break;
        }
    }

    if (!sscanf(version, "%d.%d.%d",
        &window->context.major,
        &window->context.minor,
        &window->context.revision))
    {
        if (window->context.client == GLFW_OPENGL_API)
        {
            _glfwInputError(GLFW_PLATFORM_ERROR,
                "No version found in OpenGL version string");
        }
        else
        {
            _glfwInputError(GLFW_PLATFORM_ERROR,
                "No version found in OpenGL ES version string");
        }

        glfwMakeContextCurrent((GLFWwindow*)previous);
        return GLFW_FALSE;
    }

    if (window->context.major < ctxconfig->major ||
        (window->context.major == ctxconfig->major &&
            window->context.minor < ctxconfig->minor))
    {
        // The desired OpenGL version is greater than the actual version
        // This only happens if the machine lacks {GLX|WGL}_ARB_create_context
        // /and/ the user has requested an OpenGL version greater than 1.0

        // For API consistency, we emulate the behavior of the
        // {GLX|WGL}_ARB_create_context extension and fail here

        if (window->context.client == GLFW_OPENGL_API)
        {
            _glfwInputError(GLFW_VERSION_UNAVAILABLE,
                "Requested OpenGL version %i.%i, got version %i.%i",
                ctxconfig->major, ctxconfig->minor,
                window->context.major, window->context.minor);
        }
        else
        {
            _glfwInputError(GLFW_VERSION_UNAVAILABLE,
                "Requested OpenGL ES version %i.%i, got version %i.%i",
                ctxconfig->major, ctxconfig->minor,
                window->context.major, window->context.minor);
        }

        glfwMakeContextCurrent((GLFWwindow*)previous);
        return GLFW_FALSE;
    }

    if (window->context.major >= 3)
    {
        if (!glGetStringi)
        {
            _glfwInputError(GLFW_PLATFORM_ERROR,
                "Entry point retrieval is broken");
            glfwMakeContextCurrent((GLFWwindow*)previous);
            return GLFW_FALSE;
        }
    }

    if (window->context.client == GLFW_OPENGL_API)
    {
        // Read back context flags (OpenGL 3.0 and above)
        if (window->context.major >= 3)
        {
            GLint flags;
            glGetIntegerv(GL_CONTEXT_FLAGS, &flags);

            if (flags & GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT)
                window->context.forward = GLFW_TRUE;

            if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
                window->context.debug = GLFW_TRUE;
            else if (glfwExtensionSupported("GL_ARB_debug_output") &&
                ctxconfig->debug)
            {
                // HACK: This is a workaround for older drivers (pre KHR_debug)
                //       not setting the debug bit in the context flags for
                //       debug contexts
                window->context.debug = GLFW_TRUE;
            }

            if (flags & GL_CONTEXT_FLAG_NO_ERROR_BIT_KHR)
                window->context.noerror = GLFW_TRUE;
        }

        // Read back OpenGL context profile (OpenGL 3.2 and above)
        if (window->context.major >= 4 ||
            (window->context.major == 3 && window->context.minor >= 2))
        {
            GLint mask;
            glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &mask);

            if (mask & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT)
                window->context.profile = GLFW_OPENGL_COMPAT_PROFILE;
            else if (mask & GL_CONTEXT_CORE_PROFILE_BIT)
                window->context.profile = GLFW_OPENGL_CORE_PROFILE;
            else if (glfwExtensionSupported("GL_ARB_compatibility"))
            {
                // HACK: This is a workaround for the compatibility profile bit
                //       not being set in the context flags if an OpenGL 3.2+
                //       context was created without having requested a specific
                //       version
                window->context.profile = GLFW_OPENGL_COMPAT_PROFILE;
            }
        }

        // Read back robustness strategy
        if (glfwExtensionSupported("GL_ARB_robustness"))
        {
            // NOTE: We avoid using the context flags for detection, as they are
            //       only present from 3.0 while the extension applies from 1.1

            GLint strategy;
            glGetIntegerv(GL_RESET_NOTIFICATION_STRATEGY_ARB,
                &strategy);

            if (strategy == GL_LOSE_CONTEXT_ON_RESET_ARB)
                window->context.robustness = GLFW_LOSE_CONTEXT_ON_RESET;
            else if (strategy == GL_NO_RESET_NOTIFICATION_ARB)
                window->context.robustness = GLFW_NO_RESET_NOTIFICATION;
        }
    }
    else
    {
        // Read back robustness strategy
        if (glfwExtensionSupported("GL_EXT_robustness"))
        {
            // NOTE: The values of these constants match those of the OpenGL ARB
            //       one, so we can reuse them here

            GLint strategy;
            glGetIntegerv(GL_RESET_NOTIFICATION_STRATEGY_ARB,
                &strategy);

            if (strategy == GL_LOSE_CONTEXT_ON_RESET_ARB)
                window->context.robustness = GLFW_LOSE_CONTEXT_ON_RESET;
            else if (strategy == GL_NO_RESET_NOTIFICATION_ARB)
                window->context.robustness = GLFW_NO_RESET_NOTIFICATION;
        }
    }

    if (glfwExtensionSupported("GL_KHR_context_flush_control"))
    {
        GLint behavior;
        glGetIntegerv(GL_CONTEXT_RELEASE_BEHAVIOR, &behavior);

        if (behavior == GL_NONE)
            window->context.release = GLFW_RELEASE_BEHAVIOR_NONE;
        else if (behavior == GL_CONTEXT_RELEASE_BEHAVIOR_FLUSH)
            window->context.release = GLFW_RELEASE_BEHAVIOR_FLUSH;
    }

    // Clearing the front buffer to black to avoid garbage pixels left over from
    // previous uses of our bit of VRAM
    {

        glClear(GL_COLOR_BUFFER_BIT);

        if (window->doublebuffer)
            window->context.swapBuffers(window);
    }

    glfwMakeContextCurrent((GLFWwindow*)previous);
    return GLFW_TRUE;
}

// Searches an extension string for the specified extension
//
GLFWbool _glfwStringInExtensionString(const char* string, const char* extensions)
{
    const char* start = extensions;

    for (;;)
    {
        const char* where;
        const char* terminator;

        where = strstr(start, string);
        if (!where)
            return GLFW_FALSE;

        terminator = where + strlen(string);
        if (where == start || *(where - 1) == ' ')
        {
            if (*terminator == ' ' || *terminator == '\0')
                break;
        }

        start = terminator;
    }

    return GLFW_TRUE;
}


//////////////////////////////////////////////////////////////////////////
//////                        GLFW public API                       //////
//////////////////////////////////////////////////////////////////////////

GLFWAPI void glfwMakeContextCurrent(GLFWwindow* handle)
{
    _GLFWwindow* window = (_GLFWwindow*)handle;
    _GLFWwindow* previous;

    _GLFW_REQUIRE_INIT();

    previous =(_GLFWwindow *) _glfwPlatformGetTls(&_glfw.contextSlot);

    if (window && window->context.client == GLFW_NO_API)
    {
        _glfwInputError(GLFW_NO_WINDOW_CONTEXT,
            "Cannot make current with a window that has no OpenGL or OpenGL ES context");
        return;
    }

    if (previous)
    {
        if (!window || window->context.source != previous->context.source)
            previous->context.makeCurrent(NULL);
    }

    if (window)
        window->context.makeCurrent(window);
}

GLFWAPI GLFWwindow* glfwGetCurrentContext(void)
{
    _GLFW_REQUIRE_INIT_OR_RETURN(NULL);
    return (GLFWwindow* )_glfwPlatformGetTls(&_glfw.contextSlot);
}

GLFWAPI void glfwSwapBuffers(GLFWwindow* handle)
{
    _GLFWwindow* window = (_GLFWwindow*)handle;
    assert(window != NULL);

    _GLFW_REQUIRE_INIT();

    if (window->context.client == GLFW_NO_API)
    {
        _glfwInputError(GLFW_NO_WINDOW_CONTEXT,
            "Cannot swap buffers of a window that has no OpenGL or OpenGL ES context");
        return;
    }

    window->context.swapBuffers(window);
}

GLFWAPI void glfwSwapInterval(int interval)
{
    _GLFWwindow* window;

    _GLFW_REQUIRE_INIT();

    window = (_GLFWwindow * )_glfwPlatformGetTls(&_glfw.contextSlot);
    if (!window)
    {
        _glfwInputError(GLFW_NO_CURRENT_CONTEXT,
            "Cannot set swap interval without a current OpenGL or OpenGL ES context");
        return;
    }

    window->context.swapInterval(interval);
}

GLFWAPI int glfwExtensionSupported(const char* extension)
{
    _GLFWwindow* window;
    assert(extension != NULL);

    _GLFW_REQUIRE_INIT_OR_RETURN(GLFW_FALSE);

    window =(_GLFWwindow *) _glfwPlatformGetTls(&_glfw.contextSlot);
    if (!window)
    {
        _glfwInputError(GLFW_NO_CURRENT_CONTEXT,
            "Cannot query extension without a current OpenGL or OpenGL ES context");
        return GLFW_FALSE;
    }

    if (*extension == '\0')
    {
        _glfwInputError(GLFW_INVALID_VALUE, "Extension name cannot be an empty string");
        return GLFW_FALSE;
    }

    if (window->context.major >= 3)
    {
        int i;
        GLint count;

        // Check if extension is in the modern OpenGL extensions string list

        glGetIntegerv(GL_NUM_EXTENSIONS, &count);

        for (i = 0; i < count; i++)
        {
            const char* en = (const char*)
                glGetStringi(GL_EXTENSIONS, i);
            if (!en)
            {
                _glfwInputError(GLFW_PLATFORM_ERROR,
                    "Extension string retrieval is broken");
                return GLFW_FALSE;
            }

            if (strcmp(en, extension) == 0)
                return GLFW_TRUE;
        }
    }
    else
    {
        // Check if extension is in the old style OpenGL extensions string

        const char* extensions = (const char*)glGetString(GL_EXTENSIONS);
        if (!extensions)
        {
            _glfwInputError(GLFW_PLATFORM_ERROR,
                "Extension string retrieval is broken");
            return GLFW_FALSE;
        }

        if (_glfwStringInExtensionString(extension, extensions))
            return GLFW_TRUE;
    }

    // Check if extension is in the platform-specific string
    return window->context.extensionSupported(extension);
}

GLFWAPI GLFWglproc glfwGetProcAddress(const char* procname)
{
    _GLFWwindow* window;
    assert(procname != NULL);

    _GLFW_REQUIRE_INIT_OR_RETURN(NULL);

    window = (_GLFWwindow*) _glfwPlatformGetTls(&_glfw.contextSlot);
    if (!window)
    {
        _glfwInputError(GLFW_NO_CURRENT_CONTEXT,
            "Cannot query entry point without a current OpenGL or OpenGL ES context");
        return NULL;
    }

    return window->context.getProcAddress(procname);
}



// Returns the window style for the specified window
//
static DWORD getWindowStyle(const _GLFWwindow* window)
{
    DWORD style = WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

    if (window->monitor)
        style |= WS_POPUP;
    else
    {
        style |= WS_SYSMENU | WS_MINIMIZEBOX;

        if (window->decorated)
        {
            style |= WS_CAPTION;

            if (window->resizable)
                style |= WS_MAXIMIZEBOX | WS_THICKFRAME;
        }
        else
            style |= WS_POPUP;
    }

    return style;
}

// Returns the extended window style for the specified window
//
static DWORD getWindowExStyle(const _GLFWwindow* window)
{
    DWORD style = WS_EX_APPWINDOW;

    if (window->monitor || window->floating)
        style |= WS_EX_TOPMOST;

    return style;
}

// Returns the image whose area most closely matches the desired one
//
static const GLFWimage* chooseImage(int count, const GLFWimage* images,
    int width, int height)
{
    int i, leastDiff = INT_MAX;
    const GLFWimage* closest = NULL;

    for (i = 0; i < count; i++)
    {
        const int currDiff = abs(images[i].width * images[i].height -
            width * height);
        if (currDiff < leastDiff)
        {
            closest = images + i;
            leastDiff = currDiff;
        }
    }

    return closest;
}

// Creates an RGBA icon or cursor
//
static HICON createIcon(const GLFWimage* image,
    int xhot, int yhot, GLFWbool icon)
{
    int i;
    HDC dc;
    HICON handle;
    HBITMAP color, mask;
    BITMAPV5HEADER bi;
    ICONINFO ii;
    unsigned char* target = NULL;
    unsigned char* source = image->pixels;

    ZeroMemory(&bi, sizeof(bi));
    bi.bV5Size = sizeof(bi);
    bi.bV5Width = image->width;
    bi.bV5Height = -image->height;
    bi.bV5Planes = 1;
    bi.bV5BitCount = 32;
    bi.bV5Compression = BI_BITFIELDS;
    bi.bV5RedMask = 0x00ff0000;
    bi.bV5GreenMask = 0x0000ff00;
    bi.bV5BlueMask = 0x000000ff;
    bi.bV5AlphaMask = 0xff000000;

    dc = GetDC(NULL);
    color = CreateDIBSection(dc,
        (BITMAPINFO*)&bi,
        DIB_RGB_COLORS,
        (void**)&target,
        NULL,
        (DWORD)0);
    ReleaseDC(NULL, dc);

    if (!color)
    {
        _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
            "Win32: Failed to create RGBA bitmap");
        return NULL;
    }

    mask = CreateBitmap(image->width, image->height, 1, 1, NULL);
    if (!mask)
    {
        _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
            "Win32: Failed to create mask bitmap");
        DeleteObject(color);
        return NULL;
    }

    for (i = 0; i < image->width * image->height; i++)
    {
        target[0] = source[2];
        target[1] = source[1];
        target[2] = source[0];
        target[3] = source[3];
        target += 4;
        source += 4;
    }

    ZeroMemory(&ii, sizeof(ii));
    ii.fIcon = icon;
    ii.xHotspot = xhot;
    ii.yHotspot = yhot;
    ii.hbmMask = mask;
    ii.hbmColor = color;

    handle = CreateIconIndirect(&ii);

    DeleteObject(color);
    DeleteObject(mask);

    if (!handle)
    {
        if (icon)
        {
            _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
                "Win32: Failed to create icon");
        }
        else
        {
            _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
                "Win32: Failed to create cursor");
        }
    }

    return handle;
}

// Translate content area size to full window size according to styles and DPI
//
static void getFullWindowSize(DWORD style, DWORD exStyle,
    int contentWidth, int contentHeight,
    int* fullWidth, int* fullHeight,
    UINT dpi)
{
    RECT rect = { 0, 0, contentWidth, contentHeight };

    if (_glfwIsWindows10AnniversaryUpdateOrGreaterWin32())
        AdjustWindowRectExForDpi(&rect, style, FALSE, exStyle, dpi);
    else
        AdjustWindowRectEx(&rect, style, FALSE, exStyle);

    *fullWidth = rect.right - rect.left;
    *fullHeight = rect.bottom - rect.top;
}

// Enforce the content area aspect ratio based on which edge is being dragged
//
static void applyAspectRatio(_GLFWwindow* window, int edge, RECT* area)
{
    int xoff, yoff;
    UINT dpi = USER_DEFAULT_SCREEN_DPI;
    const float ratio = (float)window->numer / (float)window->denom;

    if (_glfwIsWindows10AnniversaryUpdateOrGreaterWin32())
        dpi = GetDpiForWindow(window->win32.handle);

    getFullWindowSize(getWindowStyle(window), getWindowExStyle(window),
        0, 0, &xoff, &yoff, dpi);

    if (edge == WMSZ_LEFT || edge == WMSZ_BOTTOMLEFT ||
        edge == WMSZ_RIGHT || edge == WMSZ_BOTTOMRIGHT)
    {
        area->bottom = area->top + yoff +
            (int)((area->right - area->left - xoff) / ratio);
    }
    else if (edge == WMSZ_TOPLEFT || edge == WMSZ_TOPRIGHT)
    {
        area->top = area->bottom - yoff -
            (int)((area->right - area->left - xoff) / ratio);
    }
    else if (edge == WMSZ_TOP || edge == WMSZ_BOTTOM)
    {
        area->right = area->left + xoff +
            (int)((area->bottom - area->top - yoff) * ratio);
    }
}

// Updates the cursor image according to its cursor mode
//
static void updateCursorImage(_GLFWwindow* window)
{
    if (window->cursorMode == GLFW_CURSOR_NORMAL)
    {
        if (window->cursor)
            SetCursor(window->cursor->win32.handle);
        else
            SetCursor(LoadCursorW(NULL, IDC_ARROW));
    }
    else
        SetCursor(NULL);
}

// Updates the cursor clip rect
//
static void updateClipRect(_GLFWwindow* window)
{
    if (window)
    {
        RECT clipRect;
        GetClientRect(window->win32.handle, &clipRect);
        ClientToScreen(window->win32.handle, (POINT*)&clipRect.left);
        ClientToScreen(window->win32.handle, (POINT*)&clipRect.right);
        ClipCursor(&clipRect);
    }
    else
        ClipCursor(NULL);
}

// Enables WM_INPUT messages for the mouse for the specified window
//
static void enableRawMouseMotion(_GLFWwindow* window)
{
    const RAWINPUTDEVICE rid = { 0x01, 0x02, 0, window->win32.handle };

    if (!RegisterRawInputDevices(&rid, 1, sizeof(rid)))
    {
        _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
            "Win32: Failed to register raw input device");
    }
}

// Disables WM_INPUT messages for the mouse
//
static void disableRawMouseMotion(_GLFWwindow* window)
{
    const RAWINPUTDEVICE rid = { 0x01, 0x02, RIDEV_REMOVE, NULL };

    if (!RegisterRawInputDevices(&rid, 1, sizeof(rid)))
    {
        _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
            "Win32: Failed to remove raw input device");
    }
}

// Apply disabled cursor mode to a focused window
//
static void disableCursor(_GLFWwindow* window)
{
    _glfw.win32.disabledCursorWindow = window;
    _glfwPlatformGetCursorPos(window,
        &_glfw.win32.restoreCursorPosX,
        &_glfw.win32.restoreCursorPosY);
    updateCursorImage(window);
    _glfwCenterCursorInContentArea(window);
    updateClipRect(window);

    if (window->rawMouseMotion)
        enableRawMouseMotion(window);
}

// Exit disabled cursor mode for the specified window
//
static void enableCursor(_GLFWwindow* window)
{
    if (window->rawMouseMotion)
        disableRawMouseMotion(window);

    _glfw.win32.disabledCursorWindow = NULL;
    updateClipRect(NULL);
    _glfwPlatformSetCursorPos(window,
        _glfw.win32.restoreCursorPosX,
        _glfw.win32.restoreCursorPosY);
    updateCursorImage(window);
}

// Returns whether the cursor is in the content area of the specified window
//
static GLFWbool cursorInContentArea(_GLFWwindow* window)
{
    RECT area;
    POINT pos;

    if (!GetCursorPos(&pos))
        return GLFW_FALSE;

    if (WindowFromPoint(pos) != window->win32.handle)
        return GLFW_FALSE;

    GetClientRect(window->win32.handle, &area);
    ClientToScreen(window->win32.handle, (POINT*)&area.left);
    ClientToScreen(window->win32.handle, (POINT*)&area.right);

    return PtInRect(&area, pos);
}

// Update native window styles to match attributes
//
static void updateWindowStyles(const _GLFWwindow* window)
{
    RECT rect;
    DWORD style = GetWindowLongW(window->win32.handle, GWL_STYLE);
    style &= ~(WS_OVERLAPPEDWINDOW | WS_POPUP);
    style |= getWindowStyle(window);

    GetClientRect(window->win32.handle, &rect);

    if (_glfwIsWindows10AnniversaryUpdateOrGreaterWin32())
    {
        AdjustWindowRectExForDpi(&rect, style, FALSE,
            getWindowExStyle(window),
            GetDpiForWindow(window->win32.handle));
    }
    else
        AdjustWindowRectEx(&rect, style, FALSE, getWindowExStyle(window));

    ClientToScreen(window->win32.handle, (POINT*)&rect.left);
    ClientToScreen(window->win32.handle, (POINT*)&rect.right);
    SetWindowLongW(window->win32.handle, GWL_STYLE, style);
    SetWindowPos(window->win32.handle, HWND_TOP,
        rect.left, rect.top,
        rect.right - rect.left, rect.bottom - rect.top,
        SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOZORDER);
}

// Update window framebuffer transparency
//
static void updateFramebufferTransparency(const _GLFWwindow* window)
{
    BOOL composition, opaque;
    DWORD color;

    if (!IsWindowsVistaOrGreater())
        return;

    if (FAILED(DwmIsCompositionEnabled(&composition)) || !composition)
        return;

    if (IsWindows8OrGreater() ||
        (SUCCEEDED(DwmGetColorizationColor(&color, &opaque)) && !opaque))
    {
        HRGN region = CreateRectRgn(0, 0, -1, -1);
        DWM_BLURBEHIND bb = { 0 };
        bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
        bb.hRgnBlur = region;
        bb.fEnable = TRUE;

        DwmEnableBlurBehindWindow(window->win32.handle, &bb);
        DeleteObject(region);
    }
    else
    {
        // HACK: Disable framebuffer transparency on Windows 7 when the
        //       colorization color is opaque, because otherwise the window
        //       contents is blended additively with the previous frame instead
        //       of replacing it
        DWM_BLURBEHIND bb = { 0 };
        bb.dwFlags = DWM_BB_ENABLE;
        DwmEnableBlurBehindWindow(window->win32.handle, &bb);
    }
}

// Retrieves and translates modifier keys
//
static int getKeyMods(void)
{
    int mods = 0;

    if (GetKeyState(VK_SHIFT) & 0x8000)
        mods |= GLFW_MOD_SHIFT;
    if (GetKeyState(VK_CONTROL) & 0x8000)
        mods |= GLFW_MOD_CONTROL;
    if (GetKeyState(VK_MENU) & 0x8000)
        mods |= GLFW_MOD_ALT;
    if ((GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN)) & 0x8000)
        mods |= GLFW_MOD_SUPER;
    if (GetKeyState(VK_CAPITAL) & 1)
        mods |= GLFW_MOD_CAPS_LOCK;
    if (GetKeyState(VK_NUMLOCK) & 1)
        mods |= GLFW_MOD_NUM_LOCK;

    return mods;
}

static void fitToMonitor(_GLFWwindow* window)
{
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfoW(window->monitor->win32.handle, &mi);
    SetWindowPos(window->win32.handle, HWND_TOPMOST,
        mi.rcMonitor.left,
        mi.rcMonitor.top,
        mi.rcMonitor.right - mi.rcMonitor.left,
        mi.rcMonitor.bottom - mi.rcMonitor.top,
        SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS);
}

// Make the specified window and its video mode active on its monitor
//
static void acquireMonitor(_GLFWwindow* window)
{
    if (!_glfw.win32.acquiredMonitorCount)
    {
        SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED);

        // HACK: When mouse trails are enabled the cursor becomes invisible when
        //       the OpenGL ICD switches to page flipping
        if (IsWindowsXPOrGreater())
        {
            SystemParametersInfoW(SPI_GETMOUSETRAILS, 0, &_glfw.win32.mouseTrailSize, 0);
            SystemParametersInfoW(SPI_SETMOUSETRAILS, 0, 0, 0);
        }
    }

    if (!window->monitor->window)
        _glfw.win32.acquiredMonitorCount++;

    _glfwSetVideoModeWin32(window->monitor, &window->videoMode);
    _glfwInputMonitorWindow(window->monitor, window);
}

// Remove the window and restore the original video mode
//
static void releaseMonitor(_GLFWwindow* window)
{
    if (window->monitor->window != window)
        return;

    _glfw.win32.acquiredMonitorCount--;
    if (!_glfw.win32.acquiredMonitorCount)
    {
        SetThreadExecutionState(ES_CONTINUOUS);

        // HACK: Restore mouse trail length saved in acquireMonitor
        if (IsWindowsXPOrGreater())
            SystemParametersInfoW(SPI_SETMOUSETRAILS, _glfw.win32.mouseTrailSize, 0, 0);
    }

    _glfwInputMonitorWindow(window->monitor, NULL);
    _glfwRestoreVideoModeWin32(window->monitor);
}

// Manually maximize the window, for when SW_MAXIMIZE cannot be used
//
static void maximizeWindowManually(_GLFWwindow* window)
{
    RECT rect;
    DWORD style;
    MONITORINFO mi = { sizeof(mi) };

    GetMonitorInfoW(MonitorFromWindow(window->win32.handle,
        MONITOR_DEFAULTTONEAREST), &mi);

    rect = mi.rcWork;

    if (window->maxwidth != GLFW_DONT_CARE && window->maxheight != GLFW_DONT_CARE)
    {
        rect.right = _glfw_min(rect.right, rect.left + window->maxwidth);
        rect.bottom = _glfw_min(rect.bottom, rect.top + window->maxheight);
    }

    style = GetWindowLongW(window->win32.handle, GWL_STYLE);
    style |= WS_MAXIMIZE;
    SetWindowLongW(window->win32.handle, GWL_STYLE, style);

    if (window->decorated)
    {
        const DWORD exStyle = GetWindowLongW(window->win32.handle, GWL_EXSTYLE);

        if (_glfwIsWindows10AnniversaryUpdateOrGreaterWin32())
        {
            const UINT dpi = GetDpiForWindow(window->win32.handle);
            AdjustWindowRectExForDpi(&rect, style, FALSE, exStyle, dpi);
            OffsetRect(&rect, 0, GetSystemMetricsForDpi(SM_CYCAPTION, dpi));
        }
        else
        {
            AdjustWindowRectEx(&rect, style, FALSE, exStyle);
            OffsetRect(&rect, 0, GetSystemMetrics(SM_CYCAPTION));
        }

        rect.bottom = _glfw_min(rect.bottom, mi.rcWork.bottom);
    }

    SetWindowPos(window->win32.handle, HWND_TOP,
        rect.left,
        rect.top,
        rect.right - rect.left,
        rect.bottom - rect.top,
        SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED);
}



// Creates the GLFW window
//
static int createNativeWindow(_GLFWwindow* window,
    const _GLFWwndconfig* wndconfig,
    const _GLFWfbconfig* fbconfig)
{
    int xpos, ypos, fullWidth, fullHeight;
    WCHAR* wideTitle;
    DWORD style = getWindowStyle(window);
    DWORD exStyle = getWindowExStyle(window);

    if (window->monitor)
    {
        MONITORINFO mi = { sizeof(mi) };
        GetMonitorInfoW(window->monitor->win32.handle, &mi);

        // NOTE: This window placement is temporary and approximate, as the
        //       correct position and size cannot be known until the monitor
        //       video mode has been picked in _glfwSetVideoModeWin32
        xpos = mi.rcMonitor.left;
        ypos = mi.rcMonitor.top;
        fullWidth = mi.rcMonitor.right - mi.rcMonitor.left;
        fullHeight = mi.rcMonitor.bottom - mi.rcMonitor.top;
    }
    else
    {
        xpos = CW_USEDEFAULT;
        ypos = CW_USEDEFAULT;

        window->win32.maximized = wndconfig->maximized;
        if (wndconfig->maximized)
            style |= WS_MAXIMIZE;

        getFullWindowSize(style, exStyle,
            wndconfig->width, wndconfig->height,
            &fullWidth, &fullHeight,
            USER_DEFAULT_SCREEN_DPI);
    }

    wideTitle = _glfwCreateWideStringFromUTF8Win32(wndconfig->title);
    if (!wideTitle)
        return GLFW_FALSE;

    window->win32.handle = CreateWindowExW(exStyle,
        _GLFW_WNDCLASSNAME,
        wideTitle,
        style,
        xpos, ypos,
        fullWidth, fullHeight,
        NULL, // No parent window
        NULL, // No window menu
        _glfw.win32.instance,
        (LPVOID)wndconfig);

    free(wideTitle);

    if (!window->win32.handle)
    {
        _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
            "Win32: Failed to create window");
        return GLFW_FALSE;
    }

    SetPropW(window->win32.handle, L"GLFW", window);

    if (IsWindows7OrGreater())
    {
        ChangeWindowMessageFilterEx(window->win32.handle,
            WM_DROPFILES, MSGFLT_ALLOW, NULL);
        ChangeWindowMessageFilterEx(window->win32.handle,
            WM_COPYDATA, MSGFLT_ALLOW, NULL);
        ChangeWindowMessageFilterEx(window->win32.handle,
            WM_COPYGLOBALDATA, MSGFLT_ALLOW, NULL);
    }

    window->win32.scaleToMonitor = wndconfig->scaleToMonitor;

    if (!window->monitor)
    {
        RECT rect = { 0, 0, wndconfig->width, wndconfig->height };
        WINDOWPLACEMENT wp = { sizeof(wp) };
        const HMONITOR mh = MonitorFromWindow(window->win32.handle,
            MONITOR_DEFAULTTONEAREST);

        // Adjust window rect to account for DPI scaling of the window frame and
        // (if enabled) DPI scaling of the content area
        // This cannot be done until we know what monitor the window was placed on
        // Only update the restored window rect as the window may be maximized

        if (wndconfig->scaleToMonitor)
        {
            float xscale, yscale;
            _glfwGetMonitorContentScaleWin32(mh, &xscale, &yscale);

            if (xscale > 0.f && yscale > 0.f)
            {
                rect.right = (int)(rect.right * xscale);
                rect.bottom = (int)(rect.bottom * yscale);
            }
        }

        if (_glfwIsWindows10AnniversaryUpdateOrGreaterWin32())
        {
            AdjustWindowRectExForDpi(&rect, style, FALSE, exStyle,
                GetDpiForWindow(window->win32.handle));
        }
        else
            AdjustWindowRectEx(&rect, style, FALSE, exStyle);

        GetWindowPlacement(window->win32.handle, &wp);
        OffsetRect(&rect,
            wp.rcNormalPosition.left - rect.left,
            wp.rcNormalPosition.top - rect.top);

        wp.rcNormalPosition = rect;
        wp.showCmd = SW_HIDE;
        SetWindowPlacement(window->win32.handle, &wp);

        // Adjust rect of maximized undecorated window, because by default Windows will
        // make such a window cover the whole monitor instead of its workarea

        if (wndconfig->maximized && !wndconfig->decorated)
        {
            MONITORINFO mi = { sizeof(mi) };
            GetMonitorInfoW(mh, &mi);

            SetWindowPos(window->win32.handle, HWND_TOP,
                mi.rcWork.left,
                mi.rcWork.top,
                mi.rcWork.right - mi.rcWork.left,
                mi.rcWork.bottom - mi.rcWork.top,
                SWP_NOACTIVATE | SWP_NOZORDER);
        }
    }

    //DragAcceptFiles(window->win32.handle, TRUE);

    if (fbconfig->transparent)
    {
        updateFramebufferTransparency(window);
        window->win32.transparent = GLFW_TRUE;
    }

    _glfwPlatformGetWindowSize(window, &window->win32.width, &window->win32.height);

    return GLFW_TRUE;
}


//////////////////////////////////////////////////////////////////////////
//////                       GLFW internal API                      //////
//////////////////////////////////////////////////////////////////////////

// Registers the GLFW window class
//
GLFWbool _glfwRegisterWindowClassWin32(void)
{
    WNDCLASSEXW wc;

    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = _glfw.win32.instance;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.lpszClassName = _GLFW_WNDCLASSNAME;

    // Load user-provided icon if available
    wc.hIcon = (HICON)LoadImageW(GetModuleHandleW(NULL),
        L"GLFW_ICON", IMAGE_ICON,
        0, 0, LR_DEFAULTSIZE | LR_SHARED);
    if (!wc.hIcon)
    {
        // No user-provided icon found, load default icon
        wc.hIcon = (HICON)LoadImageW(NULL,
            IDI_APPLICATION, IMAGE_ICON,
            0, 0, LR_DEFAULTSIZE | LR_SHARED);
    }

    if (!RegisterClassExW(&wc))
    {
        _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
            "Win32: Failed to register window class");
        return GLFW_FALSE;
    }

    return GLFW_TRUE;
}

// Unregisters the GLFW window class
//
void _glfwUnregisterWindowClassWin32(void)
{
    UnregisterClassW(_GLFW_WNDCLASSNAME, _glfw.win32.instance);
}


//////////////////////////////////////////////////////////////////////////
//////                       GLFW platform API                      //////
//////////////////////////////////////////////////////////////////////////

int _glfwPlatformCreateWindow(_GLFWwindow* window,
    const _GLFWwndconfig* wndconfig,
    const _GLFWctxconfig* ctxconfig,
    const _GLFWfbconfig* fbconfig)
{
    if (!createNativeWindow(window, wndconfig, fbconfig))
        return GLFW_FALSE;

    if (ctxconfig->client != GLFW_NO_API)
    {
        if (ctxconfig->source == GLFW_NATIVE_CONTEXT_API)
        {
            if (!_glfwInitWGL())
                return GLFW_FALSE;
            if (!_glfwCreateContextWGL(window, ctxconfig, fbconfig))
                return GLFW_FALSE;
        }
        if (!_glfwRefreshContextAttribs(window, ctxconfig))
            return GLFW_FALSE;
    }

    if (window->monitor)
    {
        _glfwPlatformShowWindow(window);
        _glfwPlatformFocusWindow(window);
        acquireMonitor(window);
        fitToMonitor(window);

        if (wndconfig->centerCursor)
            _glfwCenterCursorInContentArea(window);
    }
    else
    {
        if (wndconfig->visible)
        {
            _glfwPlatformShowWindow(window);
            if (wndconfig->focused)
                _glfwPlatformFocusWindow(window);
        }
    }

    return GLFW_TRUE;
}

void _glfwPlatformDestroyWindow(_GLFWwindow* window)
{
    if (window->monitor)
        releaseMonitor(window);

    if (window->context.destroy)
        window->context.destroy(window);

    if (_glfw.win32.disabledCursorWindow == window)
        _glfw.win32.disabledCursorWindow = NULL;

    if (window->win32.handle)
    {
        RemovePropW(window->win32.handle, L"GLFW");
        DestroyWindow(window->win32.handle);
        window->win32.handle = NULL;
    }

    if (window->win32.bigIcon)
        DestroyIcon(window->win32.bigIcon);

    if (window->win32.smallIcon)
        DestroyIcon(window->win32.smallIcon);
}

void _glfwPlatformSetWindowTitle(_GLFWwindow* window, const char* title)
{
    WCHAR* wideTitle = _glfwCreateWideStringFromUTF8Win32(title);
    if (!wideTitle)
        return;

    SetWindowTextW(window->win32.handle, wideTitle);
    free(wideTitle);
}

void _glfwPlatformSetWindowIcon(_GLFWwindow* window,
    int count, const GLFWimage* images)
{
    HICON bigIcon = NULL, smallIcon = NULL;

    if (count)
    {
        const GLFWimage* bigImage = chooseImage(count, images,
            GetSystemMetrics(SM_CXICON),
            GetSystemMetrics(SM_CYICON));
        const GLFWimage* smallImage = chooseImage(count, images,
            GetSystemMetrics(SM_CXSMICON),
            GetSystemMetrics(SM_CYSMICON));

        bigIcon = createIcon(bigImage, 0, 0, GLFW_TRUE);
        smallIcon = createIcon(smallImage, 0, 0, GLFW_TRUE);
    }
    else
    {
        bigIcon = (HICON)GetClassLongPtrW(window->win32.handle, GCLP_HICON);
        smallIcon = (HICON)GetClassLongPtrW(window->win32.handle, GCLP_HICONSM);
    }

    SendMessageW(window->win32.handle, WM_SETICON, ICON_BIG, (LPARAM)bigIcon);
    SendMessageW(window->win32.handle, WM_SETICON, ICON_SMALL, (LPARAM)smallIcon);

    if (window->win32.bigIcon)
        DestroyIcon(window->win32.bigIcon);

    if (window->win32.smallIcon)
        DestroyIcon(window->win32.smallIcon);

    if (count)
    {
        window->win32.bigIcon = bigIcon;
        window->win32.smallIcon = smallIcon;
    }
}

void _glfwPlatformGetWindowPos(_GLFWwindow* window, int* xpos, int* ypos)
{
    POINT pos = { 0, 0 };
    ClientToScreen(window->win32.handle, &pos);

    if (xpos)
        *xpos = pos.x;
    if (ypos)
        *ypos = pos.y;
}

void _glfwPlatformSetWindowPos(_GLFWwindow* window, int xpos, int ypos)
{
    RECT rect = { xpos, ypos, xpos, ypos };

    if (_glfwIsWindows10AnniversaryUpdateOrGreaterWin32())
    {
        AdjustWindowRectExForDpi(&rect, getWindowStyle(window),
            FALSE, getWindowExStyle(window),
            GetDpiForWindow(window->win32.handle));
    }
    else
    {
        AdjustWindowRectEx(&rect, getWindowStyle(window),
            FALSE, getWindowExStyle(window));
    }

    SetWindowPos(window->win32.handle, NULL, rect.left, rect.top, 0, 0,
        SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
}

void _glfwPlatformGetWindowSize(_GLFWwindow* window, int* width, int* height)
{
    RECT area;
    GetClientRect(window->win32.handle, &area);

    if (width)
        *width = area.right;
    if (height)
        *height = area.bottom;
}

void _glfwPlatformSetWindowSize(_GLFWwindow* window, int width, int height)
{
    if (window->monitor)
    {
        if (window->monitor->window == window)
        {
            acquireMonitor(window);
            fitToMonitor(window);
        }
    }
    else
    {
        RECT rect = { 0, 0, width, height };

        if (_glfwIsWindows10AnniversaryUpdateOrGreaterWin32())
        {
            AdjustWindowRectExForDpi(&rect, getWindowStyle(window),
                FALSE, getWindowExStyle(window),
                GetDpiForWindow(window->win32.handle));
        }
        else
        {
            AdjustWindowRectEx(&rect, getWindowStyle(window),
                FALSE, getWindowExStyle(window));
        }

        SetWindowPos(window->win32.handle, HWND_TOP,
            0, 0, rect.right - rect.left, rect.bottom - rect.top,
            SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOZORDER);
    }
}

void _glfwPlatformSetWindowSizeLimits(_GLFWwindow* window,
    int minwidth, int minheight,
    int maxwidth, int maxheight)
{
    RECT area;

    if ((minwidth == GLFW_DONT_CARE || minheight == GLFW_DONT_CARE) &&
        (maxwidth == GLFW_DONT_CARE || maxheight == GLFW_DONT_CARE))
    {
        return;
    }

    GetWindowRect(window->win32.handle, &area);
    MoveWindow(window->win32.handle,
        area.left, area.top,
        area.right - area.left,
        area.bottom - area.top, TRUE);
}

void _glfwPlatformSetWindowAspectRatio(_GLFWwindow* window, int numer, int denom)
{
    RECT area;

    if (numer == GLFW_DONT_CARE || denom == GLFW_DONT_CARE)
        return;

    GetWindowRect(window->win32.handle, &area);
    applyAspectRatio(window, WMSZ_BOTTOMRIGHT, &area);
    MoveWindow(window->win32.handle,
        area.left, area.top,
        area.right - area.left,
        area.bottom - area.top, TRUE);
}

void _glfwPlatformGetFramebufferSize(_GLFWwindow* window, int* width, int* height)
{
    _glfwPlatformGetWindowSize(window, width, height);
}

void _glfwPlatformGetWindowFrameSize(_GLFWwindow* window,
    int* left, int* top,
    int* right, int* bottom)
{
    RECT rect;
    int width, height;

    _glfwPlatformGetWindowSize(window, &width, &height);
    SetRect(&rect, 0, 0, width, height);

    if (_glfwIsWindows10AnniversaryUpdateOrGreaterWin32())
    {
        AdjustWindowRectExForDpi(&rect, getWindowStyle(window),
            FALSE, getWindowExStyle(window),
            GetDpiForWindow(window->win32.handle));
    }
    else
    {
        AdjustWindowRectEx(&rect, getWindowStyle(window),
            FALSE, getWindowExStyle(window));
    }

    if (left)
        *left = -rect.left;
    if (top)
        *top = -rect.top;
    if (right)
        *right = rect.right - width;
    if (bottom)
        *bottom = rect.bottom - height;
}

void _glfwPlatformGetWindowContentScale(_GLFWwindow* window,
    float* xscale, float* yscale)
{
     HMONITOR handle = MonitorFromWindow(window->win32.handle,
        MONITOR_DEFAULTTONEAREST);
    _glfwGetMonitorContentScaleWin32(handle, xscale, yscale);
}

void _glfwPlatformIconifyWindow(_GLFWwindow* window)
{
    ShowWindow(window->win32.handle, SW_MINIMIZE);
}

void _glfwPlatformRestoreWindow(_GLFWwindow* window)
{
    ShowWindow(window->win32.handle, SW_RESTORE);
}

void _glfwPlatformMaximizeWindow(_GLFWwindow* window)
{
    if (IsWindowVisible(window->win32.handle))
        ShowWindow(window->win32.handle, SW_MAXIMIZE);
    else
        maximizeWindowManually(window);
}

void _glfwPlatformShowWindow(_GLFWwindow* window)
{
    ShowWindow(window->win32.handle, SW_SHOWNA);
}

void _glfwPlatformHideWindow(_GLFWwindow* window)
{
    ShowWindow(window->win32.handle, SW_HIDE);
}

void _glfwPlatformRequestWindowAttention(_GLFWwindow* window)
{
    FlashWindow(window->win32.handle, TRUE);
}

void _glfwPlatformFocusWindow(_GLFWwindow* window)
{
    BringWindowToTop(window->win32.handle);
    SetForegroundWindow(window->win32.handle);
    SetFocus(window->win32.handle);
}

void _glfwPlatformSetWindowMonitor(_GLFWwindow* window,
    _GLFWmonitor* monitor,
    int xpos, int ypos,
    int width, int height,
    int refreshRate)
{
    if (window->monitor == monitor)
    {
        if (monitor)
        {
            if (monitor->window == window)
            {
                acquireMonitor(window);
                fitToMonitor(window);
            }
        }
        else
        {
            RECT rect = { xpos, ypos, xpos + width, ypos + height };

            if (_glfwIsWindows10AnniversaryUpdateOrGreaterWin32())
            {
                AdjustWindowRectExForDpi(&rect, getWindowStyle(window),
                    FALSE, getWindowExStyle(window),
                    GetDpiForWindow(window->win32.handle));
            }
            else
            {
                AdjustWindowRectEx(&rect, getWindowStyle(window),
                    FALSE, getWindowExStyle(window));
            }

            SetWindowPos(window->win32.handle, HWND_TOP,
                rect.left, rect.top,
                rect.right - rect.left, rect.bottom - rect.top,
                SWP_NOCOPYBITS | SWP_NOACTIVATE | SWP_NOZORDER);
        }

        return;
    }

    if (window->monitor)
        releaseMonitor(window);

    _glfwInputWindowMonitor(window, monitor);

    if (window->monitor)
    {
        MONITORINFO mi = { sizeof(mi) };
        UINT flags = SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOCOPYBITS;

        if (window->decorated)
        {
            DWORD style = GetWindowLongW(window->win32.handle, GWL_STYLE);
            style &= ~WS_OVERLAPPEDWINDOW;
            style |= getWindowStyle(window);
            SetWindowLongW(window->win32.handle, GWL_STYLE, style);
            flags |= SWP_FRAMECHANGED;
        }

        acquireMonitor(window);

        GetMonitorInfoW(window->monitor->win32.handle, &mi);
        SetWindowPos(window->win32.handle, HWND_TOPMOST,
            mi.rcMonitor.left,
            mi.rcMonitor.top,
            mi.rcMonitor.right - mi.rcMonitor.left,
            mi.rcMonitor.bottom - mi.rcMonitor.top,
            flags);
    }
    else
    {
        HWND after;
        RECT rect = { xpos, ypos, xpos + width, ypos + height };
        DWORD style = GetWindowLongW(window->win32.handle, GWL_STYLE);
        UINT flags = SWP_NOACTIVATE | SWP_NOCOPYBITS;

        if (window->decorated)
        {
            style &= ~WS_POPUP;
            style |= getWindowStyle(window);
            SetWindowLongW(window->win32.handle, GWL_STYLE, style);

            flags |= SWP_FRAMECHANGED;
        }

        if (window->floating)
            after = HWND_TOPMOST;
        else
            after = HWND_NOTOPMOST;

        if (_glfwIsWindows10AnniversaryUpdateOrGreaterWin32())
        {
            AdjustWindowRectExForDpi(&rect, getWindowStyle(window),
                FALSE, getWindowExStyle(window),
                GetDpiForWindow(window->win32.handle));
        }
        else
        {
            AdjustWindowRectEx(&rect, getWindowStyle(window),
                FALSE, getWindowExStyle(window));
        }

        SetWindowPos(window->win32.handle, after,
            rect.left, rect.top,
            rect.right - rect.left, rect.bottom - rect.top,
            flags);
    }
}

int _glfwPlatformWindowFocused(_GLFWwindow* window)
{
    return window->win32.handle == GetActiveWindow();
}

int _glfwPlatformWindowIconified(_GLFWwindow* window)
{
    return IsIconic(window->win32.handle);
}

int _glfwPlatformWindowVisible(_GLFWwindow* window)
{
    return IsWindowVisible(window->win32.handle);
}

int _glfwPlatformWindowMaximized(_GLFWwindow* window)
{
    return IsZoomed(window->win32.handle);
}

int _glfwPlatformWindowHovered(_GLFWwindow* window)
{
    return cursorInContentArea(window);
}

int _glfwPlatformFramebufferTransparent(_GLFWwindow* window)
{
    BOOL composition, opaque;
    DWORD color;

    if (!window->win32.transparent)
        return GLFW_FALSE;

    if (!IsWindowsVistaOrGreater())
        return GLFW_FALSE;

    if (FAILED(DwmIsCompositionEnabled(&composition)) || !composition)
        return GLFW_FALSE;

    if (!IsWindows8OrGreater())
    {
        // HACK: Disable framebuffer transparency on Windows 7 when the
        //       colorization color is opaque, because otherwise the window
        //       contents is blended additively with the previous frame instead
        //       of replacing it
        if (FAILED(DwmGetColorizationColor(&color, &opaque)) || opaque)
            return GLFW_FALSE;
    }

    return GLFW_TRUE;
}

void _glfwPlatformSetWindowResizable(_GLFWwindow* window, GLFWbool enabled)
{
    updateWindowStyles(window);
}

void _glfwPlatformSetWindowDecorated(_GLFWwindow* window, GLFWbool enabled)
{
    updateWindowStyles(window);
}

void _glfwPlatformSetWindowFloating(_GLFWwindow* window, GLFWbool enabled)
{
    const HWND after = enabled ? HWND_TOPMOST : HWND_NOTOPMOST;
    SetWindowPos(window->win32.handle, after, 0, 0, 0, 0,
        SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
}

float _glfwPlatformGetWindowOpacity(_GLFWwindow* window)
{
    BYTE alpha;
    DWORD flags;

    if ((GetWindowLongW(window->win32.handle, GWL_EXSTYLE) & WS_EX_LAYERED) &&
        GetLayeredWindowAttributes(window->win32.handle, NULL, &alpha, &flags))
    {
        if (flags & LWA_ALPHA)
            return alpha / 255.f;
    }

    return 1.f;
}

void _glfwPlatformSetWindowOpacity(_GLFWwindow* window, float opacity)
{
    if (opacity < 1.f)
    {
        const BYTE alpha = (BYTE)(255 * opacity);
        DWORD style = GetWindowLongW(window->win32.handle, GWL_EXSTYLE);
        style |= WS_EX_LAYERED;
        SetWindowLongW(window->win32.handle, GWL_EXSTYLE, style);
        SetLayeredWindowAttributes(window->win32.handle, 0, alpha, LWA_ALPHA);
    }
    else
    {
        DWORD style = GetWindowLongW(window->win32.handle, GWL_EXSTYLE);
        style &= ~WS_EX_LAYERED;
        SetWindowLongW(window->win32.handle, GWL_EXSTYLE, style);
    }
}

void _glfwPlatformSetRawMouseMotion(_GLFWwindow* window, GLFWbool enabled)
{
    if (_glfw.win32.disabledCursorWindow != window)
        return;

    if (enabled)
        enableRawMouseMotion(window);
    else
        disableRawMouseMotion(window);
}

GLFWbool _glfwPlatformRawMouseMotionSupported(void)
{
    return GLFW_TRUE;
}

void _glfwPlatformPollEvents(void)
{
    MSG msg;
    HWND handle;
    _GLFWwindow* window;

    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
        {
            // NOTE: While GLFW does not itself post WM_QUIT, other processes
            //       may post it to this one, for example Task Manager
            // HACK: Treat WM_QUIT as a close on all windows

            window = _glfw.windowListHead;
            while (window)
            {
                _glfwInputWindowCloseRequest(window);
                window = window->next;
            }
        }
        else
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    // HACK: Release modifier keys that the system did not emit KEYUP for
    // NOTE: Shift keys on Windows tend to "stick" when both are pressed as
    //       no key up message is generated by the first key release
    // NOTE: Windows key is not reported as released by the Win+V hotkey
    //       Other Win hotkeys are handled implicitly by _glfwInputWindowFocus
    //       because they change the input focus
    // NOTE: The other half of this is in the WM_*KEY* handler in windowProc
    handle = GetActiveWindow();
    if (handle)
    {
        window =(_GLFWwindow*) GetPropW(handle, L"GLFW");
        if (window)
        {
            int i;
            const int keys[4][2] =
            {
                { VK_LSHIFT, GLFW_KEY_LEFT_SHIFT },
                { VK_RSHIFT, GLFW_KEY_RIGHT_SHIFT },
                { VK_LWIN, GLFW_KEY_LEFT_SUPER },
                { VK_RWIN, GLFW_KEY_RIGHT_SUPER }
            };

            for (i = 0; i < 4; i++)
            {
                const int vk = keys[i][0];
                const int key = keys[i][1];
                const int scancode = _glfw.win32.scancodes[key];

                if ((GetKeyState(vk) & 0x8000))
                    continue;
                if (window->keys[key] != GLFW_PRESS)
                    continue;

                _glfwInputKey(window, key, scancode, GLFW_RELEASE, getKeyMods());
            }
        }
    }

    window = _glfw.win32.disabledCursorWindow;
    if (window)
    {
        int width, height;
        _glfwPlatformGetWindowSize(window, &width, &height);

        // NOTE: Re-center the cursor only if it has moved since the last call,
        //       to avoid breaking glfwWaitEvents with WM_MOUSEMOVE
        if (window->win32.lastCursorPosX != width / 2 ||
            window->win32.lastCursorPosY != height / 2)
        {
            _glfwPlatformSetCursorPos(window, width / 2, height / 2);
        }
    }
}

void _glfwPlatformWaitEvents(void)
{
    WaitMessage();

    _glfwPlatformPollEvents();
}

void _glfwPlatformWaitEventsTimeout(double timeout)
{
    MsgWaitForMultipleObjects(0, NULL, FALSE, (DWORD)(timeout * 1e3), QS_ALLEVENTS);

    _glfwPlatformPollEvents();
}

void _glfwPlatformPostEmptyEvent(void)
{
    PostMessageW(_glfw.win32.helperWindowHandle, WM_NULL, 0, 0);
}

void _glfwPlatformGetCursorPos(_GLFWwindow* window, double* xpos, double* ypos)
{
    POINT pos;

    if (GetCursorPos(&pos))
    {
        ScreenToClient(window->win32.handle, &pos);

        if (xpos)
            *xpos = pos.x;
        if (ypos)
            *ypos = pos.y;
    }
}

void _glfwPlatformSetCursorPos(_GLFWwindow* window, double xpos, double ypos)
{
    POINT pos = { (int)xpos, (int)ypos };

    // Store the new position so it can be recognized later
    window->win32.lastCursorPosX = pos.x;
    window->win32.lastCursorPosY = pos.y;

    ClientToScreen(window->win32.handle, &pos);
    SetCursorPos(pos.x, pos.y);
}

void _glfwPlatformSetCursorMode(_GLFWwindow* window, int mode)
{
    if (mode == GLFW_CURSOR_DISABLED)
    {
        if (_glfwPlatformWindowFocused(window))
            disableCursor(window);
    }
    else if (_glfw.win32.disabledCursorWindow == window)
        enableCursor(window);
    else if (cursorInContentArea(window))
        updateCursorImage(window);
}

const char* _glfwPlatformGetScancodeName(int scancode)
{
    if (scancode < 0 || scancode >(KF_EXTENDED | 0xff) ||
        _glfw.win32.keycodes[scancode] == GLFW_KEY_UNKNOWN)
    {
        _glfwInputError(GLFW_INVALID_VALUE, "Invalid scancode %i", scancode);
        return NULL;
    }

    return _glfw.win32.keynames[_glfw.win32.keycodes[scancode]];
}

int _glfwPlatformGetKeyScancode(int key)
{
    return _glfw.win32.scancodes[key];
}

int _glfwPlatformCreateCursor(_GLFWcursor* cursor,
    const GLFWimage* image,
    int xhot, int yhot)
{
    cursor->win32.handle = (HCURSOR)createIcon(image, xhot, yhot, GLFW_FALSE);
    if (!cursor->win32.handle)
        return GLFW_FALSE;

    return GLFW_TRUE;
}


void _glfwPlatformDestroyCursor(_GLFWcursor* cursor)
{
    if (cursor->win32.handle)
        DestroyIcon((HICON)cursor->win32.handle);
}

void _glfwPlatformSetCursor(_GLFWwindow* window, _GLFWcursor* cursor)
{
    if (cursorInContentArea(window))
        updateCursorImage(window);
}

void _glfwPlatformSetClipboardString(const char* string)
{
    int characterCount;
    HANDLE object;
    WCHAR* buffer;

    characterCount = MultiByteToWideChar(CP_UTF8, 0, string, -1, NULL, 0);
    if (!characterCount)
        return;

    object = GlobalAlloc(GMEM_MOVEABLE, characterCount * sizeof(WCHAR));
    if (!object)
    {
        _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
            "Win32: Failed to allocate global handle for clipboard");
        return;
    }

    buffer = (WCHAR*)GlobalLock(object);
    if (!buffer)
    {
        _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
            "Win32: Failed to lock global handle");
        GlobalFree(object);
        return;
    }

    MultiByteToWideChar(CP_UTF8, 0, string, -1, buffer, characterCount);
    GlobalUnlock(object);

    if (!OpenClipboard(_glfw.win32.helperWindowHandle))
    {
        _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
            "Win32: Failed to open clipboard");
        GlobalFree(object);
        return;
    }

    EmptyClipboard();
    SetClipboardData(CF_UNICODETEXT, object);
    CloseClipboard();
}

const char* _glfwPlatformGetClipboardString(void)
{
    HANDLE object;
    WCHAR* buffer;

    if (!OpenClipboard(_glfw.win32.helperWindowHandle))
    {
        _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
            "Win32: Failed to open clipboard");
        return NULL;
    }

    object = GetClipboardData(CF_UNICODETEXT);
    if (!object)
    {
        _glfwInputErrorWin32(GLFW_FORMAT_UNAVAILABLE,
            "Win32: Failed to convert clipboard to string");
        CloseClipboard();
        return NULL;
    }

    buffer = (WCHAR*)GlobalLock(object);
    if (!buffer)
    {
        _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
            "Win32: Failed to lock global handle");
        CloseClipboard();
        return NULL;
    }

    free(_glfw.win32.clipboardString);
    _glfw.win32.clipboardString = _glfwCreateUTF8FromWideStringWin32(buffer);

    GlobalUnlock(object);
    CloseClipboard();

    return _glfw.win32.clipboardString;
}


//////////////////////////////////////////////////////////////////////////
//////                        GLFW native API                       //////
//////////////////////////////////////////////////////////////////////////

GLFWAPI HWND glfwGetWin32Window(GLFWwindow* handle)
{
    _GLFWwindow* window = (_GLFWwindow*)handle;
    _GLFW_REQUIRE_INIT_OR_RETURN(NULL);
    return window->win32.handle;
}


GLFWbool _glfwPlatformCreateTls(_GLFWtls* tls)
{
    assert(tls->win32.allocated == GLFW_FALSE);

    tls->win32.index = TlsAlloc();
    if (tls->win32.index == TLS_OUT_OF_INDEXES)
    {
        _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
            "Win32: Failed to allocate TLS index");
        return GLFW_FALSE;
    }

    tls->win32.allocated = GLFW_TRUE;
    return GLFW_TRUE;
}

void _glfwPlatformDestroyTls(_GLFWtls* tls)
{
    if (tls->win32.allocated)
        TlsFree(tls->win32.index);
    memset(tls, 0, sizeof(_GLFWtls));
}

void* _glfwPlatformGetTls(_GLFWtls* tls)
{
    assert(tls->win32.allocated == GLFW_TRUE);
    return TlsGetValue(tls->win32.index);
}

void _glfwPlatformSetTls(_GLFWtls* tls, void* value)
{
    assert(tls->win32.allocated == GLFW_TRUE);
    TlsSetValue(tls->win32.index, value);
}

GLFWbool _glfwPlatformCreateMutex(_GLFWmutex* mutex)
{
    assert(mutex->win32.allocated == GLFW_FALSE);
    InitializeCriticalSection(&mutex->win32.section);
    return mutex->win32.allocated = GLFW_TRUE;
}

void _glfwPlatformDestroyMutex(_GLFWmutex* mutex)
{
    if (mutex->win32.allocated)
        DeleteCriticalSection(&mutex->win32.section);
    memset(mutex, 0, sizeof(_GLFWmutex));
}

void _glfwPlatformLockMutex(_GLFWmutex* mutex)
{
    assert(mutex->win32.allocated == GLFW_TRUE);
    EnterCriticalSection(&mutex->win32.section);
}

void _glfwPlatformUnlockMutex(_GLFWmutex* mutex)
{
    assert(mutex->win32.allocated == GLFW_TRUE);
    LeaveCriticalSection(&mutex->win32.section);
}



static const GUID _glfw_GUID_DEVINTERFACE_HID =
{ 0x4d1e55b2,0xf16f,0x11cf,{0x88,0xcb,0x00,0x11,0x11,0x00,0x00,0x30} };

#define GUID_DEVINTERFACE_HID _glfw_GUID_DEVINTERFACE_HID


static GLFWbool loadLibraries(void)
{
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (const WCHAR*)&_glfw,
        (HMODULE*)&_glfw.win32.instance))
    {
        _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
            "Win32: Failed to retrieve own module handle");
        return GLFW_FALSE;
    }

    return GLFW_TRUE;
}

// Unload used libraries (DLLs)
//
static void freeLibraries(void)
{



}

// Create key code translation tables
//
static void createKeyTables(void)
{
    int scancode;

    memset(_glfw.win32.keycodes, -1, sizeof(_glfw.win32.keycodes));
    memset(_glfw.win32.scancodes, -1, sizeof(_glfw.win32.scancodes));

    _glfw.win32.keycodes[0x00B] = GLFW_KEY_0;
    _glfw.win32.keycodes[0x002] = GLFW_KEY_1;
    _glfw.win32.keycodes[0x003] = GLFW_KEY_2;
    _glfw.win32.keycodes[0x004] = GLFW_KEY_3;
    _glfw.win32.keycodes[0x005] = GLFW_KEY_4;
    _glfw.win32.keycodes[0x006] = GLFW_KEY_5;
    _glfw.win32.keycodes[0x007] = GLFW_KEY_6;
    _glfw.win32.keycodes[0x008] = GLFW_KEY_7;
    _glfw.win32.keycodes[0x009] = GLFW_KEY_8;
    _glfw.win32.keycodes[0x00A] = GLFW_KEY_9;
    _glfw.win32.keycodes[0x01E] = GLFW_KEY_A;
    _glfw.win32.keycodes[0x030] = GLFW_KEY_B;
    _glfw.win32.keycodes[0x02E] = GLFW_KEY_C;
    _glfw.win32.keycodes[0x020] = GLFW_KEY_D;
    _glfw.win32.keycodes[0x012] = GLFW_KEY_E;
    _glfw.win32.keycodes[0x021] = GLFW_KEY_F;
    _glfw.win32.keycodes[0x022] = GLFW_KEY_G;
    _glfw.win32.keycodes[0x023] = GLFW_KEY_H;
    _glfw.win32.keycodes[0x017] = GLFW_KEY_I;
    _glfw.win32.keycodes[0x024] = GLFW_KEY_J;
    _glfw.win32.keycodes[0x025] = GLFW_KEY_K;
    _glfw.win32.keycodes[0x026] = GLFW_KEY_L;
    _glfw.win32.keycodes[0x032] = GLFW_KEY_M;
    _glfw.win32.keycodes[0x031] = GLFW_KEY_N;
    _glfw.win32.keycodes[0x018] = GLFW_KEY_O;
    _glfw.win32.keycodes[0x019] = GLFW_KEY_P;
    _glfw.win32.keycodes[0x010] = GLFW_KEY_Q;
    _glfw.win32.keycodes[0x013] = GLFW_KEY_R;
    _glfw.win32.keycodes[0x01F] = GLFW_KEY_S;
    _glfw.win32.keycodes[0x014] = GLFW_KEY_T;
    _glfw.win32.keycodes[0x016] = GLFW_KEY_U;
    _glfw.win32.keycodes[0x02F] = GLFW_KEY_V;
    _glfw.win32.keycodes[0x011] = GLFW_KEY_W;
    _glfw.win32.keycodes[0x02D] = GLFW_KEY_X;
    _glfw.win32.keycodes[0x015] = GLFW_KEY_Y;
    _glfw.win32.keycodes[0x02C] = GLFW_KEY_Z;

    _glfw.win32.keycodes[0x028] = GLFW_KEY_APOSTROPHE;
    _glfw.win32.keycodes[0x02B] = GLFW_KEY_BACKSLASH;
    _glfw.win32.keycodes[0x033] = GLFW_KEY_COMMA;
    _glfw.win32.keycodes[0x00D] = GLFW_KEY_EQUAL;
    _glfw.win32.keycodes[0x029] = GLFW_KEY_GRAVE_ACCENT;
    _glfw.win32.keycodes[0x01A] = GLFW_KEY_LEFT_BRACKET;
    _glfw.win32.keycodes[0x00C] = GLFW_KEY_MINUS;
    _glfw.win32.keycodes[0x034] = GLFW_KEY_PERIOD;
    _glfw.win32.keycodes[0x01B] = GLFW_KEY_RIGHT_BRACKET;
    _glfw.win32.keycodes[0x027] = GLFW_KEY_SEMICOLON;
    _glfw.win32.keycodes[0x035] = GLFW_KEY_SLASH;
    _glfw.win32.keycodes[0x056] = GLFW_KEY_WORLD_2;

    _glfw.win32.keycodes[0x00E] = GLFW_KEY_BACKSPACE;
    _glfw.win32.keycodes[0x153] = GLFW_KEY_DELETE;
    _glfw.win32.keycodes[0x14F] = GLFW_KEY_END;
    _glfw.win32.keycodes[0x01C] = GLFW_KEY_ENTER;
    _glfw.win32.keycodes[0x001] = GLFW_KEY_ESCAPE;
    _glfw.win32.keycodes[0x147] = GLFW_KEY_HOME;
    _glfw.win32.keycodes[0x152] = GLFW_KEY_INSERT;
    _glfw.win32.keycodes[0x15D] = GLFW_KEY_MENU;
    _glfw.win32.keycodes[0x151] = GLFW_KEY_PAGE_DOWN;
    _glfw.win32.keycodes[0x149] = GLFW_KEY_PAGE_UP;
    _glfw.win32.keycodes[0x045] = GLFW_KEY_PAUSE;
    _glfw.win32.keycodes[0x039] = GLFW_KEY_SPACE;
    _glfw.win32.keycodes[0x00F] = GLFW_KEY_TAB;
    _glfw.win32.keycodes[0x03A] = GLFW_KEY_CAPS_LOCK;
    _glfw.win32.keycodes[0x145] = GLFW_KEY_NUM_LOCK;
    _glfw.win32.keycodes[0x046] = GLFW_KEY_SCROLL_LOCK;
    _glfw.win32.keycodes[0x03B] = GLFW_KEY_F1;
    _glfw.win32.keycodes[0x03C] = GLFW_KEY_F2;
    _glfw.win32.keycodes[0x03D] = GLFW_KEY_F3;
    _glfw.win32.keycodes[0x03E] = GLFW_KEY_F4;
    _glfw.win32.keycodes[0x03F] = GLFW_KEY_F5;
    _glfw.win32.keycodes[0x040] = GLFW_KEY_F6;
    _glfw.win32.keycodes[0x041] = GLFW_KEY_F7;
    _glfw.win32.keycodes[0x042] = GLFW_KEY_F8;
    _glfw.win32.keycodes[0x043] = GLFW_KEY_F9;
    _glfw.win32.keycodes[0x044] = GLFW_KEY_F10;
    _glfw.win32.keycodes[0x057] = GLFW_KEY_F11;
    _glfw.win32.keycodes[0x058] = GLFW_KEY_F12;
    _glfw.win32.keycodes[0x064] = GLFW_KEY_F13;
    _glfw.win32.keycodes[0x065] = GLFW_KEY_F14;
    _glfw.win32.keycodes[0x066] = GLFW_KEY_F15;
    _glfw.win32.keycodes[0x067] = GLFW_KEY_F16;
    _glfw.win32.keycodes[0x068] = GLFW_KEY_F17;
    _glfw.win32.keycodes[0x069] = GLFW_KEY_F18;
    _glfw.win32.keycodes[0x06A] = GLFW_KEY_F19;
    _glfw.win32.keycodes[0x06B] = GLFW_KEY_F20;
    _glfw.win32.keycodes[0x06C] = GLFW_KEY_F21;
    _glfw.win32.keycodes[0x06D] = GLFW_KEY_F22;
    _glfw.win32.keycodes[0x06E] = GLFW_KEY_F23;
    _glfw.win32.keycodes[0x076] = GLFW_KEY_F24;
    _glfw.win32.keycodes[0x038] = GLFW_KEY_LEFT_ALT;
    _glfw.win32.keycodes[0x01D] = GLFW_KEY_LEFT_CONTROL;
    _glfw.win32.keycodes[0x02A] = GLFW_KEY_LEFT_SHIFT;
    _glfw.win32.keycodes[0x15B] = GLFW_KEY_LEFT_SUPER;
    _glfw.win32.keycodes[0x137] = GLFW_KEY_PRINT_SCREEN;
    _glfw.win32.keycodes[0x138] = GLFW_KEY_RIGHT_ALT;
    _glfw.win32.keycodes[0x11D] = GLFW_KEY_RIGHT_CONTROL;
    _glfw.win32.keycodes[0x036] = GLFW_KEY_RIGHT_SHIFT;
    _glfw.win32.keycodes[0x15C] = GLFW_KEY_RIGHT_SUPER;
    _glfw.win32.keycodes[0x150] = GLFW_KEY_DOWN;
    _glfw.win32.keycodes[0x14B] = GLFW_KEY_LEFT;
    _glfw.win32.keycodes[0x14D] = GLFW_KEY_RIGHT;
    _glfw.win32.keycodes[0x148] = GLFW_KEY_UP;

    _glfw.win32.keycodes[0x052] = GLFW_KEY_KP_0;
    _glfw.win32.keycodes[0x04F] = GLFW_KEY_KP_1;
    _glfw.win32.keycodes[0x050] = GLFW_KEY_KP_2;
    _glfw.win32.keycodes[0x051] = GLFW_KEY_KP_3;
    _glfw.win32.keycodes[0x04B] = GLFW_KEY_KP_4;
    _glfw.win32.keycodes[0x04C] = GLFW_KEY_KP_5;
    _glfw.win32.keycodes[0x04D] = GLFW_KEY_KP_6;
    _glfw.win32.keycodes[0x047] = GLFW_KEY_KP_7;
    _glfw.win32.keycodes[0x048] = GLFW_KEY_KP_8;
    _glfw.win32.keycodes[0x049] = GLFW_KEY_KP_9;
    _glfw.win32.keycodes[0x04E] = GLFW_KEY_KP_ADD;
    _glfw.win32.keycodes[0x053] = GLFW_KEY_KP_DECIMAL;
    _glfw.win32.keycodes[0x135] = GLFW_KEY_KP_DIVIDE;
    _glfw.win32.keycodes[0x11C] = GLFW_KEY_KP_ENTER;
    _glfw.win32.keycodes[0x059] = GLFW_KEY_KP_EQUAL;
    _glfw.win32.keycodes[0x037] = GLFW_KEY_KP_MULTIPLY;
    _glfw.win32.keycodes[0x04A] = GLFW_KEY_KP_SUBTRACT;

    for (scancode = 0; scancode < 512; scancode++)
    {
        if (_glfw.win32.keycodes[scancode] > 0)
            _glfw.win32.scancodes[_glfw.win32.keycodes[scancode]] = scancode;
    }
}

// Creates a dummy window for behind-the-scenes work
//
static GLFWbool createHelperWindow(void)
{
    MSG msg;

    _glfw.win32.helperWindowHandle =
        CreateWindowExW(WS_EX_OVERLAPPEDWINDOW,
            _GLFW_WNDCLASSNAME,
            L"GLFW message window",
            WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
            0, 0, 1, 1,
            NULL, NULL,
            _glfw.win32.instance,
            NULL);

    if (!_glfw.win32.helperWindowHandle)
    {
        _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
            "Win32: Failed to create helper window");
        return GLFW_FALSE;
    }

    // HACK: The command to the first ShowWindow call is ignored if the parent
    //       process passed along a STARTUPINFO, so clear that with a no-op call
    ShowWindow(_glfw.win32.helperWindowHandle, SW_HIDE);

    // Register for HID device notifications
    {
        DEV_BROADCAST_DEVICEINTERFACE_W dbi;
        ZeroMemory(&dbi, sizeof(dbi));
        dbi.dbcc_size = sizeof(dbi);
        dbi.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
        dbi.dbcc_classguid = GUID_DEVINTERFACE_HID;

        _glfw.win32.deviceNotificationHandle =
            RegisterDeviceNotificationW(_glfw.win32.helperWindowHandle,
                (DEV_BROADCAST_HDR*)&dbi,
                DEVICE_NOTIFY_WINDOW_HANDLE);
    }

    while (PeekMessageW(&msg, _glfw.win32.helperWindowHandle, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return GLFW_TRUE;
}


WCHAR* _glfwCreateWideStringFromUTF8Win32(const char* source)
{
    WCHAR* target;
    int count;

    count = MultiByteToWideChar(CP_UTF8, 0, source, -1, NULL, 0);
    if (!count)
    {
        _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
            "Win32: Failed to convert string from UTF-8");
        return NULL;
    }

    target = (WCHAR*)calloc(count, sizeof(WCHAR));

    if (!MultiByteToWideChar(CP_UTF8, 0, source, -1, target, count))
    {
        _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
            "Win32: Failed to convert string from UTF-8");
        free(target);
        return NULL;
    }

    return target;
}

// Returns a UTF-8 string version of the specified wide string
//
char* _glfwCreateUTF8FromWideStringWin32(const WCHAR* source)
{
    char* target;
    int size;

    size = WideCharToMultiByte(CP_UTF8, 0, source, -1, NULL, 0, NULL, NULL);
    if (!size)
    {
        _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
            "Win32: Failed to convert string to UTF-8");
        return NULL;
    }

    target = (char*)calloc(size, 1);

    if (!WideCharToMultiByte(CP_UTF8, 0, source, -1, target, size, NULL, NULL))
    {
        _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
            "Win32: Failed to convert string to UTF-8");
        free(target);
        return NULL;
    }

    return target;
}

// Reports the specified error, appending information about the last Win32 error
//
void _glfwInputErrorWin32(int error, const char* description)
{
    WCHAR buffer[_GLFW_MESSAGE_SIZE] = L"";
    char message[_GLFW_MESSAGE_SIZE] = "";

    FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS |
        FORMAT_MESSAGE_MAX_WIDTH_MASK,
        NULL,
        GetLastError() & 0xffff,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        buffer,
        sizeof(buffer) / sizeof(WCHAR),
        NULL);
    WideCharToMultiByte(CP_UTF8, 0, buffer, -1, message, sizeof(message), NULL, NULL);

    _glfwInputError(error, "%s: %s", description, message);
}

// Updates key names according to the current keyboard layout
//
void _glfwUpdateKeyNamesWin32(void)
{
    int key;
    BYTE state[256] = { 0 };

    memset(_glfw.win32.keynames, 0, sizeof(_glfw.win32.keynames));

    for (key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; key++)
    {
        UINT vk;
        int scancode, length;
        WCHAR chars[16];

        scancode = _glfw.win32.scancodes[key];
        if (scancode == -1)
            continue;

        if (key >= GLFW_KEY_KP_0 && key <= GLFW_KEY_KP_ADD)
        {
            const UINT vks[] = {
                VK_NUMPAD0,  VK_NUMPAD1,  VK_NUMPAD2, VK_NUMPAD3,
                VK_NUMPAD4,  VK_NUMPAD5,  VK_NUMPAD6, VK_NUMPAD7,
                VK_NUMPAD8,  VK_NUMPAD9,  VK_DECIMAL, VK_DIVIDE,
                VK_MULTIPLY, VK_SUBTRACT, VK_ADD
            };

            vk = vks[key - GLFW_KEY_KP_0];
        }
        else
            vk = MapVirtualKeyW(scancode, MAPVK_VSC_TO_VK);

        length = ToUnicode(vk, scancode, state,
            chars, sizeof(chars) / sizeof(WCHAR),
            0);

        if (length == -1)
        {
            // This is a dead key, so we need a second simulated key press
            // to make it output its own character (usually a diacritic)
            length = ToUnicode(vk, scancode, state,
                chars, sizeof(chars) / sizeof(WCHAR),
                0);
        }

        if (length < 1)
            continue;

        WideCharToMultiByte(CP_UTF8, 0, chars, 1,
            _glfw.win32.keynames[key],
            sizeof(_glfw.win32.keynames[key]),
            NULL, NULL);
    }
}

// Replacement for IsWindowsVersionOrGreater, as we cannot rely on the
// application having a correct embedded manifest
//
BOOL _glfwIsWindowsVersionOrGreaterWin32(WORD major, WORD minor, WORD sp)
{
    OSVERSIONINFOEXW osvi = { sizeof(osvi), major, minor, 0, 0, {0}, sp };
    DWORD mask = VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR;
    ULONGLONG cond = VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL);
    cond = VerSetConditionMask(cond, VER_MINORVERSION, VER_GREATER_EQUAL);
    cond = VerSetConditionMask(cond, VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);
    // HACK: Use RtlVerifyVersionInfo instead of VerifyVersionInfoW as the
    //       latter lies unless the user knew to embed a non-default manifest
    //       announcing support for Windows 10 via supportedOS GUID
    return RtlVerifyVersionInfo(&osvi, mask, cond) == 0;
}

// Checks whether we are on at least the specified build of Windows 10
//
BOOL _glfwIsWindows10BuildOrGreaterWin32(WORD build)
{
    OSVERSIONINFOEXW osvi = { sizeof(osvi), 10, 0, build };
    DWORD mask = VER_MAJORVERSION | VER_MINORVERSION | VER_BUILDNUMBER;
    ULONGLONG cond = VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL);
    cond = VerSetConditionMask(cond, VER_MINORVERSION, VER_GREATER_EQUAL);
    cond = VerSetConditionMask(cond, VER_BUILDNUMBER, VER_GREATER_EQUAL);
    // HACK: Use RtlVerifyVersionInfo instead of VerifyVersionInfoW as the
    //       latter lies unless the user knew to embed a non-default manifest
    //       announcing support for Windows 10 via supportedOS GUID
    return RtlVerifyVersionInfo(&osvi, mask, cond) == 0;
}


//////////////////////////////////////////////////////////////////////////
//////                       GLFW platform API                      //////
//////////////////////////////////////////////////////////////////////////

int _glfwPlatformInit(void)
{
    // To make SetForegroundWindow work as we want, we need to fiddle
    // with the FOREGROUNDLOCKTIMEOUT system setting (we do this as early
    // as possible in the hope of still being the foreground process)
    SystemParametersInfoW(SPI_GETFOREGROUNDLOCKTIMEOUT, 0,
        &_glfw.win32.foregroundLockTimeout, 0);
    SystemParametersInfoW(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, UIntToPtr(0),
        SPIF_SENDCHANGE);

    if (!loadLibraries())
        return GLFW_FALSE;

    createKeyTables();
    _glfwUpdateKeyNamesWin32();

    if (_glfwIsWindows10CreatorsUpdateOrGreaterWin32())
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    else if (IsWindows8Point1OrGreater())
        SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
    else if (IsWindowsVistaOrGreater())
        SetProcessDPIAware();

    if (!_glfwRegisterWindowClassWin32())
        return GLFW_FALSE;

    if (!createHelperWindow())
        return GLFW_FALSE;

    _glfwPollMonitorsWin32();
    return GLFW_TRUE;
}

void _glfwPlatformTerminate(void)
{
    if (_glfw.win32.deviceNotificationHandle)
        UnregisterDeviceNotification(_glfw.win32.deviceNotificationHandle);

    if (_glfw.win32.helperWindowHandle)
        DestroyWindow(_glfw.win32.helperWindowHandle);

    _glfwUnregisterWindowClassWin32();

    // Restore previous foreground lock timeout system setting
    SystemParametersInfoW(SPI_SETFOREGROUNDLOCKTIMEOUT, 0,
        UIntToPtr(_glfw.win32.foregroundLockTimeout),
        SPIF_SENDCHANGE);

    free(_glfw.win32.clipboardString);
    free(_glfw.win32.rawInput);

    _glfwTerminateWGL();

}

const char* _glfwPlatformGetVersionString(void)
{
    return _GLFW_VERSION_NUMBER " Win32 WGL EGL OSMesa"
#if defined(__MINGW32__)
        " MinGW"
#elif defined(_MSC_VER)
        " VisualC"
#endif
#if defined(_GLFW_USE_HYBRID_HPG) || defined(_GLFW_USE_OPTIMUS_HPG)
        " hybrid-GPU"
#endif
#if defined(_GLFW_BUILD_DLL)
        " DLL"
#endif
        ;
}






// Callback for EnumDisplayMonitors in createMonitor
//
static BOOL CALLBACK monitorCallback(HMONITOR handle,
    HDC dc,
    RECT* rect,
    LPARAM data)
{
    MONITORINFOEXW mi;
    ZeroMemory(&mi, sizeof(mi));
    mi.cbSize = sizeof(mi);

    if (GetMonitorInfoW(handle, (MONITORINFO*)&mi))
    {
        _GLFWmonitor* monitor = (_GLFWmonitor*)data;
        if (wcscmp(mi.szDevice, monitor->win32.adapterName) == 0)
            monitor->win32.handle = handle;
    }

    return TRUE;
}

// Create monitor from an adapter and (optionally) a display
//
static _GLFWmonitor* createMonitor(DISPLAY_DEVICEW* adapter,
    DISPLAY_DEVICEW* display)
{
    _GLFWmonitor* monitor;
    int widthMM, heightMM;
    char* name;
    HDC dc;
    DEVMODEW dm;
    RECT rect;

    if (display)
        name = _glfwCreateUTF8FromWideStringWin32(display->DeviceString);
    else
        name = _glfwCreateUTF8FromWideStringWin32(adapter->DeviceString);
    if (!name)
        return NULL;

    ZeroMemory(&dm, sizeof(dm));
    dm.dmSize = sizeof(dm);
    EnumDisplaySettingsW(adapter->DeviceName, ENUM_CURRENT_SETTINGS, &dm);

    dc = CreateDCW(L"DISPLAY", adapter->DeviceName, NULL, NULL);

    if (IsWindows8Point1OrGreater())
    {
        widthMM = GetDeviceCaps(dc, HORZSIZE);
        heightMM = GetDeviceCaps(dc, VERTSIZE);
    }
    else
    {
        widthMM = (int)(dm.dmPelsWidth * 25.4f / GetDeviceCaps(dc, LOGPIXELSX));
        heightMM = (int)(dm.dmPelsHeight * 25.4f / GetDeviceCaps(dc, LOGPIXELSY));
    }

    DeleteDC(dc);

    monitor = _glfwAllocMonitor(name, widthMM, heightMM);
    free(name);

    if (adapter->StateFlags & DISPLAY_DEVICE_MODESPRUNED)
        monitor->win32.modesPruned = GLFW_TRUE;

    wcscpy(monitor->win32.adapterName, adapter->DeviceName);
    WideCharToMultiByte(CP_UTF8, 0,
        adapter->DeviceName, -1,
        monitor->win32.publicAdapterName,
        sizeof(monitor->win32.publicAdapterName),
        NULL, NULL);

    if (display)
    {
        wcscpy(monitor->win32.displayName, display->DeviceName);
        WideCharToMultiByte(CP_UTF8, 0,
            display->DeviceName, -1,
            monitor->win32.publicDisplayName,
            sizeof(monitor->win32.publicDisplayName),
            NULL, NULL);
    }

    rect.left = dm.dmPosition.x;
    rect.top = dm.dmPosition.y;
    rect.right = dm.dmPosition.x + dm.dmPelsWidth;
    rect.bottom = dm.dmPosition.y + dm.dmPelsHeight;

    EnumDisplayMonitors(NULL, &rect, monitorCallback, (LPARAM)monitor);
    return monitor;
}


//////////////////////////////////////////////////////////////////////////
//////                       GLFW internal API                      //////
//////////////////////////////////////////////////////////////////////////

// Poll for changes in the set of connected monitors
//
void _glfwPollMonitorsWin32(void)
{
    int i, disconnectedCount;
    _GLFWmonitor** disconnected = NULL;
    DWORD adapterIndex, displayIndex;
    DISPLAY_DEVICEW adapter, display;
    _GLFWmonitor* monitor;

    disconnectedCount = _glfw.monitorCount;
    if (disconnectedCount)
    {
        disconnected = (_GLFWmonitor**)calloc(_glfw.monitorCount, sizeof(_GLFWmonitor*));
        memcpy(disconnected,
            _glfw.monitors,
            _glfw.monitorCount * sizeof(_GLFWmonitor*));
    }

    for (adapterIndex = 0; ; adapterIndex++)
    {
        int type = _GLFW_INSERT_LAST;

        ZeroMemory(&adapter, sizeof(adapter));
        adapter.cb = sizeof(adapter);

        if (!EnumDisplayDevicesW(NULL, adapterIndex, &adapter, 0))
            break;

        if (!(adapter.StateFlags & DISPLAY_DEVICE_ACTIVE))
            continue;

        if (adapter.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
            type = _GLFW_INSERT_FIRST;

        for (displayIndex = 0; ; displayIndex++)
        {
            ZeroMemory(&display, sizeof(display));
            display.cb = sizeof(display);

            if (!EnumDisplayDevicesW(adapter.DeviceName, displayIndex, &display, 0))
                break;

            if (!(display.StateFlags & DISPLAY_DEVICE_ACTIVE))
                continue;

            for (i = 0; i < disconnectedCount; i++)
            {
                if (disconnected[i] &&
                    wcscmp(disconnected[i]->win32.displayName,
                        display.DeviceName) == 0)
                {
                    disconnected[i] = NULL;
                    // handle may have changed, update
                    EnumDisplayMonitors(NULL, NULL, monitorCallback, (LPARAM)_glfw.monitors[i]);
                    break;
                }
            }

            if (i < disconnectedCount)
                continue;

            monitor = createMonitor(&adapter, &display);
            if (!monitor)
            {
                free(disconnected);
                return;
            }

            _glfwInputMonitor(monitor, GLFW_CONNECTED, type);

            type = _GLFW_INSERT_LAST;
        }

        // HACK: If an active adapter does not have any display devices
        //       (as sometimes happens), add it directly as a monitor
        if (displayIndex == 0)
        {
            for (i = 0; i < disconnectedCount; i++)
            {
                if (disconnected[i] &&
                    wcscmp(disconnected[i]->win32.adapterName,
                        adapter.DeviceName) == 0)
                {
                    disconnected[i] = NULL;
                    break;
                }
            }

            if (i < disconnectedCount)
                continue;

            monitor = createMonitor(&adapter, NULL);
            if (!monitor)
            {
                free(disconnected);
                return;
            }

            _glfwInputMonitor(monitor, GLFW_CONNECTED, type);
        }
    }

    for (i = 0; i < disconnectedCount; i++)
    {
        if (disconnected[i])
            _glfwInputMonitor(disconnected[i], GLFW_DISCONNECTED, 0);
    }

    free(disconnected);
}

// Change the current video mode
//
void _glfwSetVideoModeWin32(_GLFWmonitor* monitor, const GLFWvidmode* desired)
{
    GLFWvidmode current;
    const GLFWvidmode* best;
    DEVMODEW dm;
    LONG result;

    best = _glfwChooseVideoMode(monitor, desired);
    _glfwPlatformGetVideoMode(monitor, &current);
    if (_glfwCompareVideoModes(&current, best) == 0)
        return;

    ZeroMemory(&dm, sizeof(dm));
    dm.dmSize = sizeof(dm);
    dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL |
        DM_DISPLAYFREQUENCY;
    dm.dmPelsWidth = best->width;
    dm.dmPelsHeight = best->height;
    dm.dmBitsPerPel = best->redBits + best->greenBits + best->blueBits;
    dm.dmDisplayFrequency = best->refreshRate;

    if (dm.dmBitsPerPel < 15 || dm.dmBitsPerPel >= 24)
        dm.dmBitsPerPel = 32;

    result = ChangeDisplaySettingsExW(monitor->win32.adapterName,
        &dm,
        NULL,
        CDS_FULLSCREEN,
        NULL);
    if (result == DISP_CHANGE_SUCCESSFUL)
        monitor->win32.modeChanged = GLFW_TRUE;
    else
    {
        const char* description = "Unknown error";

        if (result == DISP_CHANGE_BADDUALVIEW)
            description = "The system uses DualView";
        else if (result == DISP_CHANGE_BADFLAGS)
            description = "Invalid flags";
        else if (result == DISP_CHANGE_BADMODE)
            description = "Graphics mode not supported";
        else if (result == DISP_CHANGE_BADPARAM)
            description = "Invalid parameter";
        else if (result == DISP_CHANGE_FAILED)
            description = "Graphics mode failed";
        else if (result == DISP_CHANGE_NOTUPDATED)
            description = "Failed to write to registry";
        else if (result == DISP_CHANGE_RESTART)
            description = "Computer restart required";

        _glfwInputError(GLFW_PLATFORM_ERROR,
            "Win32: Failed to set video mode: %s",
            description);
    }
}

// Restore the previously saved (original) video mode
//
void _glfwRestoreVideoModeWin32(_GLFWmonitor* monitor)
{
    if (monitor->win32.modeChanged)
    {
        ChangeDisplaySettingsExW(monitor->win32.adapterName,
            NULL, NULL, CDS_FULLSCREEN, NULL);
        monitor->win32.modeChanged = GLFW_FALSE;
    }
}

void _glfwGetMonitorContentScaleWin32(HMONITOR handle, float* xscale, float* yscale)
{
    UINT xdpi, ydpi;

    if (xscale)
        *xscale = 0.f;
    if (yscale)
        *yscale = 0.f;

    if (IsWindows8Point1OrGreater())
    {
        if (GetDpiForMonitor(handle, MDT_EFFECTIVE_DPI, &xdpi, &ydpi) != S_OK)
        {
            _glfwInputError(GLFW_PLATFORM_ERROR, "Win32: Failed to query monitor DPI");
            return;
        }
    }
    else
    {
        const HDC dc = GetDC(NULL);
        xdpi = GetDeviceCaps(dc, LOGPIXELSX);
        ydpi = GetDeviceCaps(dc, LOGPIXELSY);
        ReleaseDC(NULL, dc);
    }

    if (xscale)
        *xscale = xdpi / (float)USER_DEFAULT_SCREEN_DPI;
    if (yscale)
        *yscale = ydpi / (float)USER_DEFAULT_SCREEN_DPI;
}


//////////////////////////////////////////////////////////////////////////
//////                       GLFW platform API                      //////
//////////////////////////////////////////////////////////////////////////

void _glfwPlatformFreeMonitor(_GLFWmonitor* monitor)
{
}

void _glfwPlatformGetMonitorPos(_GLFWmonitor* monitor, int* xpos, int* ypos)
{
    DEVMODEW dm;
    ZeroMemory(&dm, sizeof(dm));
    dm.dmSize = sizeof(dm);

    EnumDisplaySettingsExW(monitor->win32.adapterName,
        ENUM_CURRENT_SETTINGS,
        &dm,
        EDS_ROTATEDMODE);

    if (xpos)
        *xpos = dm.dmPosition.x;
    if (ypos)
        *ypos = dm.dmPosition.y;
}

void _glfwPlatformGetMonitorContentScale(_GLFWmonitor* monitor,
    float* xscale, float* yscale)
{
    _glfwGetMonitorContentScaleWin32(monitor->win32.handle, xscale, yscale);
}

void _glfwPlatformGetMonitorWorkarea(_GLFWmonitor* monitor,
    int* xpos, int* ypos,
    int* width, int* height)
{
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfoW(monitor->win32.handle, &mi);

    if (xpos)
        *xpos = mi.rcWork.left;
    if (ypos)
        *ypos = mi.rcWork.top;
    if (width)
        *width = mi.rcWork.right - mi.rcWork.left;
    if (height)
        *height = mi.rcWork.bottom - mi.rcWork.top;
}

GLFWvidmode* _glfwPlatformGetVideoModes(_GLFWmonitor* monitor, int* count)
{
    int modeIndex = 0, size = 0;
    GLFWvidmode* result = NULL;

    *count = 0;

    for (;;)
    {
        int i;
        GLFWvidmode mode;
        DEVMODEW dm;

        ZeroMemory(&dm, sizeof(dm));
        dm.dmSize = sizeof(dm);

        if (!EnumDisplaySettingsW(monitor->win32.adapterName, modeIndex, &dm))
            break;

        modeIndex++;

        // Skip modes with less than 15 BPP
        if (dm.dmBitsPerPel < 15)
            continue;

        mode.width = dm.dmPelsWidth;
        mode.height = dm.dmPelsHeight;
        mode.refreshRate = dm.dmDisplayFrequency;
        _glfwSplitBPP(dm.dmBitsPerPel,
            &mode.redBits,
            &mode.greenBits,
            &mode.blueBits);

        for (i = 0; i < *count; i++)
        {
            if (_glfwCompareVideoModes(result + i, &mode) == 0)
                break;
        }

        // Skip duplicate modes
        if (i < *count)
            continue;

        if (monitor->win32.modesPruned)
        {
            // Skip modes not supported by the connected displays
            if (ChangeDisplaySettingsExW(monitor->win32.adapterName,
                &dm,
                NULL,
                CDS_TEST,
                NULL) != DISP_CHANGE_SUCCESSFUL)
            {
                continue;
            }
        }

        if (*count == size)
        {
            size += 128;
            result = (GLFWvidmode*)realloc(result, size * sizeof(GLFWvidmode));
        }

        (*count)++;
        result[*count - 1] = mode;
    }

    if (!*count)
    {
        // HACK: Report the current mode if no valid modes were found
        result = (GLFWvidmode*)calloc(1, sizeof(GLFWvidmode));
        _glfwPlatformGetVideoMode(monitor, result);
        *count = 1;
    }

    return result;
}

void _glfwPlatformGetVideoMode(_GLFWmonitor* monitor, GLFWvidmode* mode)
{
    DEVMODEW dm;
    ZeroMemory(&dm, sizeof(dm));
    dm.dmSize = sizeof(dm);

    EnumDisplaySettingsW(monitor->win32.adapterName, ENUM_CURRENT_SETTINGS, &dm);

    mode->width = dm.dmPelsWidth;
    mode->height = dm.dmPelsHeight;
    mode->refreshRate = dm.dmDisplayFrequency;
    _glfwSplitBPP(dm.dmBitsPerPel,
        &mode->redBits,
        &mode->greenBits,
        &mode->blueBits);
}

GLFWbool _glfwPlatformGetGammaRamp(_GLFWmonitor* monitor, GLFWgammaramp* ramp)
{
    HDC dc;
    WORD values[3][256];

    dc = CreateDCW(L"DISPLAY", monitor->win32.adapterName, NULL, NULL);
    GetDeviceGammaRamp(dc, values);
    DeleteDC(dc);

    _glfwAllocGammaArrays(ramp, 256);

    memcpy(ramp->red, values[0], sizeof(values[0]));
    memcpy(ramp->green, values[1], sizeof(values[1]));
    memcpy(ramp->blue, values[2], sizeof(values[2]));

    return GLFW_TRUE;
}

void _glfwPlatformSetGammaRamp(_GLFWmonitor* monitor, const GLFWgammaramp* ramp)
{
    HDC dc;
    WORD values[3][256];

    if (ramp->size != 256)
    {
        _glfwInputError(GLFW_PLATFORM_ERROR,
            "Win32: Gamma ramp size must be 256");
        return;
    }

    memcpy(values[0], ramp->red, sizeof(values[0]));
    memcpy(values[1], ramp->green, sizeof(values[1]));
    memcpy(values[2], ramp->blue, sizeof(values[2]));

    dc = CreateDCW(L"DISPLAY", monitor->win32.adapterName, NULL, NULL);
    SetDeviceGammaRamp(dc, values);
    DeleteDC(dc);
}


//////////////////////////////////////////////////////////////////////////
//////                        GLFW native API                       //////
//////////////////////////////////////////////////////////////////////////

GLFWAPI const char* glfwGetWin32Adapter(GLFWmonitor* handle)
{
    _GLFWmonitor* monitor = (_GLFWmonitor*)handle;
    _GLFW_REQUIRE_INIT_OR_RETURN(NULL);
    return monitor->win32.publicAdapterName;
}

GLFWAPI const char* glfwGetWin32Monitor(GLFWmonitor* handle)
{
    _GLFWmonitor* monitor = (_GLFWmonitor*)handle;
    _GLFW_REQUIRE_INIT_OR_RETURN(NULL);
    return monitor->win32.publicDisplayName;
}




void _glfwInputWindowFocus(_GLFWwindow* window, GLFWbool focused)
{

    if (!focused)
    {
        int key, button;

        for (key = 0; key <= GLFW_KEY_LAST; key++)
        {
            if (window->keys[key] == GLFW_PRESS)
            {
                const int scancode = _glfwPlatformGetKeyScancode(key);
                _glfwInputKey(window, key, scancode, GLFW_RELEASE, 0);
            }
        }

        for (button = 0; button <= GLFW_MOUSE_BUTTON_LAST; button++)
        {
            if (window->mouseButtons[button] == GLFW_PRESS)
                _glfwInputMouseClick(window, button, GLFW_RELEASE, 0);
        }
    }
}

// Notifies shared code that a window has moved
// The position is specified in content area relative screen coordinates
//
void _glfwInputWindowPos(_GLFWwindow* window, int x, int y)
{

}

// Notifies shared code that a window has been resized
// The size is specified in screen coordinates
//
void _glfwInputWindowSize(_GLFWwindow* window, int width, int height)
{

}

// Notifies shared code that a window has been iconified or restored
//
void _glfwInputWindowIconify(_GLFWwindow* window, GLFWbool iconified)
{

}

// Notifies shared code that a window has been maximized or restored
//
void _glfwInputWindowMaximize(_GLFWwindow* window, GLFWbool maximized)
{

}

// Notifies shared code that a window framebuffer has been resized
// The size is specified in pixels
//
void _glfwInputFramebufferSize(_GLFWwindow* window, int width, int height)
{

}

// Notifies shared code that a window content scale has changed
// The scale is specified as the ratio between the current and default DPI
//
void _glfwInputWindowContentScale(_GLFWwindow* window, float xscale, float yscale)
{


}

// Notifies shared code that the window contents needs updating
//
void _glfwInputWindowDamage(_GLFWwindow* window)
{

}

// Notifies shared code that the user wishes to close a window
//
void _glfwInputWindowCloseRequest(_GLFWwindow* window)
{
    window->shouldClose = GLFW_TRUE;
}

// Notifies shared code that a window has changed its desired monitor
//
void _glfwInputWindowMonitor(_GLFWwindow* window, _GLFWmonitor* monitor)
{
    window->monitor = monitor;
}

//////////////////////////////////////////////////////////////////////////
//////                        GLFW public API                       //////
//////////////////////////////////////////////////////////////////////////

GLFWAPI GLFWwindow* glfwCreateWindow(int width, int height,
    const char* title,
    GLFWmonitor* monitor,
    GLFWwindow* share)
{
    _GLFWfbconfig fbconfig;
    _GLFWctxconfig ctxconfig;
    _GLFWwndconfig wndconfig;
    _GLFWwindow* window;

    assert(title != NULL);
    assert(width >= 0);
    assert(height >= 0);

    _GLFW_REQUIRE_INIT_OR_RETURN(NULL);

    if (width <= 0 || height <= 0)
    {
        _glfwInputError(GLFW_INVALID_VALUE,
            "Invalid window size %ix%i",
            width, height);

        return NULL;
    }

    fbconfig = _glfw.hints.framebuffer;
    ctxconfig = _glfw.hints.context;
    wndconfig = _glfw.hints.window;

    wndconfig.width = width;
    wndconfig.height = height;
    wndconfig.title = title;
    ctxconfig.share = (_GLFWwindow*)share;

    if (!_glfwIsValidContextConfig(&ctxconfig))
        return NULL;

    window =( _GLFWwindow *) calloc(1, sizeof(_GLFWwindow));
    window->next = _glfw.windowListHead;
    _glfw.windowListHead = window;

    window->videoMode.width = width;
    window->videoMode.height = height;
    window->videoMode.redBits = fbconfig.redBits;
    window->videoMode.greenBits = fbconfig.greenBits;
    window->videoMode.blueBits = fbconfig.blueBits;
    window->videoMode.refreshRate = _glfw.hints.refreshRate;

    window->monitor = (_GLFWmonitor*)monitor;
    window->resizable = wndconfig.resizable;
    window->decorated = wndconfig.decorated;
    window->autoIconify = wndconfig.autoIconify;
    window->floating = wndconfig.floating;
    window->focusOnShow = wndconfig.focusOnShow;
    window->cursorMode = GLFW_CURSOR_NORMAL;

    window->doublebuffer = fbconfig.doublebuffer;

    window->minwidth = GLFW_DONT_CARE;
    window->minheight = GLFW_DONT_CARE;
    window->maxwidth = GLFW_DONT_CARE;
    window->maxheight = GLFW_DONT_CARE;
    window->numer = GLFW_DONT_CARE;
    window->denom = GLFW_DONT_CARE;

    // Open the actual window and create its context
    if (!_glfwPlatformCreateWindow(window, &wndconfig, &ctxconfig, &fbconfig))
    {
        glfwDestroyWindow((GLFWwindow*)window);
        return NULL;
    }

    return (GLFWwindow*)window;
}

void glfwDefaultWindowHints(void)
{
    _GLFW_REQUIRE_INIT();

    // The default is OpenGL with minimum version 1.0
    memset(&_glfw.hints.context, 0, sizeof(_glfw.hints.context));
    _glfw.hints.context.client = GLFW_OPENGL_API;
    _glfw.hints.context.source = GLFW_NATIVE_CONTEXT_API;
    _glfw.hints.context.major = 1;
    _glfw.hints.context.minor = 0;

    // The default is a focused, visible, resizable window with decorations
    memset(&_glfw.hints.window, 0, sizeof(_glfw.hints.window));
    _glfw.hints.window.resizable = GLFW_TRUE;
    _glfw.hints.window.visible = GLFW_TRUE;
    _glfw.hints.window.decorated = GLFW_TRUE;
    _glfw.hints.window.focused = GLFW_TRUE;
    _glfw.hints.window.autoIconify = GLFW_TRUE;
    _glfw.hints.window.centerCursor = GLFW_TRUE;
    _glfw.hints.window.focusOnShow = GLFW_TRUE;

    // The default is 24 bits of color, 24 bits of depth and 8 bits of stencil,
    // double buffered
    memset(&_glfw.hints.framebuffer, 0, sizeof(_glfw.hints.framebuffer));
    _glfw.hints.framebuffer.redBits = 8;
    _glfw.hints.framebuffer.greenBits = 8;
    _glfw.hints.framebuffer.blueBits = 8;
    _glfw.hints.framebuffer.alphaBits = 8;
    _glfw.hints.framebuffer.depthBits = 24;
    _glfw.hints.framebuffer.stencilBits = 8;
    _glfw.hints.framebuffer.doublebuffer = GLFW_TRUE;

    // The default is to select the highest available refresh rate
    _glfw.hints.refreshRate = GLFW_DONT_CARE;

    // The default is to use full Retina resolution framebuffers
    _glfw.hints.window.ns.retina = GLFW_TRUE;
}

GLFWAPI void glfwWindowHint(int hint, int value)
{
    _GLFW_REQUIRE_INIT();

    switch (hint)
    {
    case GLFW_RED_BITS:
        _glfw.hints.framebuffer.redBits = value;
        return;
    case GLFW_GREEN_BITS:
        _glfw.hints.framebuffer.greenBits = value;
        return;
    case GLFW_BLUE_BITS:
        _glfw.hints.framebuffer.blueBits = value;
        return;
    case GLFW_ALPHA_BITS:
        _glfw.hints.framebuffer.alphaBits = value;
        return;
    case GLFW_DEPTH_BITS:
        _glfw.hints.framebuffer.depthBits = value;
        return;
    case GLFW_STENCIL_BITS:
        _glfw.hints.framebuffer.stencilBits = value;
        return;
    case GLFW_ACCUM_RED_BITS:
        _glfw.hints.framebuffer.accumRedBits = value;
        return;
    case GLFW_ACCUM_GREEN_BITS:
        _glfw.hints.framebuffer.accumGreenBits = value;
        return;
    case GLFW_ACCUM_BLUE_BITS:
        _glfw.hints.framebuffer.accumBlueBits = value;
        return;
    case GLFW_ACCUM_ALPHA_BITS:
        _glfw.hints.framebuffer.accumAlphaBits = value;
        return;
    case GLFW_AUX_BUFFERS:
        _glfw.hints.framebuffer.auxBuffers = value;
        return;
    case GLFW_STEREO:
        _glfw.hints.framebuffer.stereo = value ? GLFW_TRUE : GLFW_FALSE;
        return;
    case GLFW_DOUBLEBUFFER:
        _glfw.hints.framebuffer.doublebuffer = value ? GLFW_TRUE : GLFW_FALSE;
        return;
    case GLFW_TRANSPARENT_FRAMEBUFFER:
        _glfw.hints.framebuffer.transparent = value ? GLFW_TRUE : GLFW_FALSE;
        return;
    case GLFW_SAMPLES:
        _glfw.hints.framebuffer.samples = value;
        return;
    case GLFW_SRGB_CAPABLE:
        _glfw.hints.framebuffer.sRGB = value ? GLFW_TRUE : GLFW_FALSE;
        return;
    case GLFW_RESIZABLE:
        _glfw.hints.window.resizable = value ? GLFW_TRUE : GLFW_FALSE;
        return;
    case GLFW_DECORATED:
        _glfw.hints.window.decorated = value ? GLFW_TRUE : GLFW_FALSE;
        return;
    case GLFW_FOCUSED:
        _glfw.hints.window.focused = value ? GLFW_TRUE : GLFW_FALSE;
        return;
    case GLFW_AUTO_ICONIFY:
        _glfw.hints.window.autoIconify = value ? GLFW_TRUE : GLFW_FALSE;
        return;
    case GLFW_FLOATING:
        _glfw.hints.window.floating = value ? GLFW_TRUE : GLFW_FALSE;
        return;
    case GLFW_MAXIMIZED:
        _glfw.hints.window.maximized = value ? GLFW_TRUE : GLFW_FALSE;
        return;
    case GLFW_VISIBLE:
        _glfw.hints.window.visible = value ? GLFW_TRUE : GLFW_FALSE;
        return;
    case GLFW_COCOA_RETINA_FRAMEBUFFER:
        _glfw.hints.window.ns.retina = value ? GLFW_TRUE : GLFW_FALSE;
        return;
    case GLFW_COCOA_GRAPHICS_SWITCHING:
        _glfw.hints.context.nsgl.offline = value ? GLFW_TRUE : GLFW_FALSE;
        return;
    case GLFW_SCALE_TO_MONITOR:
        _glfw.hints.window.scaleToMonitor = value ? GLFW_TRUE : GLFW_FALSE;
        return;
    case GLFW_CENTER_CURSOR:
        _glfw.hints.window.centerCursor = value ? GLFW_TRUE : GLFW_FALSE;
        return;
    case GLFW_FOCUS_ON_SHOW:
        _glfw.hints.window.focusOnShow = value ? GLFW_TRUE : GLFW_FALSE;
        return;
    case GLFW_CLIENT_API:
        _glfw.hints.context.client = value;
        return;
    case GLFW_CONTEXT_CREATION_API:
        _glfw.hints.context.source = value;
        return;
    case GLFW_CONTEXT_VERSION_MAJOR:
        _glfw.hints.context.major = value;
        return;
    case GLFW_CONTEXT_VERSION_MINOR:
        _glfw.hints.context.minor = value;
        return;
    case GLFW_CONTEXT_ROBUSTNESS:
        _glfw.hints.context.robustness = value;
        return;
    case GLFW_OPENGL_FORWARD_COMPAT:
        _glfw.hints.context.forward = value ? GLFW_TRUE : GLFW_FALSE;
        return;
    case GLFW_OPENGL_DEBUG_CONTEXT:
        _glfw.hints.context.debug = value ? GLFW_TRUE : GLFW_FALSE;
        return;
    case GLFW_CONTEXT_NO_ERROR:
        _glfw.hints.context.noerror = value ? GLFW_TRUE : GLFW_FALSE;
        return;
    case GLFW_OPENGL_PROFILE:
        _glfw.hints.context.profile = value;
        return;
    case GLFW_CONTEXT_RELEASE_BEHAVIOR:
        _glfw.hints.context.release = value;
        return;
    case GLFW_REFRESH_RATE:
        _glfw.hints.refreshRate = value;
        return;
    }

    _glfwInputError(GLFW_INVALID_ENUM, "Invalid window hint 0x%08X", hint);
}

GLFWAPI void glfwWindowHintString(int hint, const char* value)
{
    assert(value != NULL);

    _GLFW_REQUIRE_INIT();

    switch (hint)
    {
    case GLFW_COCOA_FRAME_NAME:
        strncpy(_glfw.hints.window.ns.frameName, value,
            sizeof(_glfw.hints.window.ns.frameName) - 1);
        return;
    case GLFW_X11_CLASS_NAME:
        strncpy(_glfw.hints.window.x11.className, value,
            sizeof(_glfw.hints.window.x11.className) - 1);
        return;
    case GLFW_X11_INSTANCE_NAME:
        strncpy(_glfw.hints.window.x11.instanceName, value,
            sizeof(_glfw.hints.window.x11.instanceName) - 1);
        return;
    }

    _glfwInputError(GLFW_INVALID_ENUM, "Invalid window hint string 0x%08X", hint);
}

GLFWAPI void glfwDestroyWindow(GLFWwindow* handle)
{
    _GLFWwindow* window = (_GLFWwindow*)handle;

    _GLFW_REQUIRE_INIT();

    // Allow closing of NULL (to match the behavior of free)
    if (window == NULL)
        return;

    // Clear all callbacks to avoid exposing a half torn-down window object


    // The window's context must not be current on another thread when the
    // window is destroyed
    if (window == _glfwPlatformGetTls(&_glfw.contextSlot))
        glfwMakeContextCurrent(NULL);

    _glfwPlatformDestroyWindow(window);

    // Unlink window from global linked list
    {
        _GLFWwindow** prev = &_glfw.windowListHead;

        while (*prev != window)
            prev = &((*prev)->next);

        *prev = window->next;
    }

    free(window);
}

GLFWAPI int glfwWindowShouldClose(GLFWwindow* handle)
{
    _GLFWwindow* window = (_GLFWwindow*)handle;
    assert(window != NULL);

    _GLFW_REQUIRE_INIT_OR_RETURN(0);
    return window->shouldClose;
}

GLFWAPI void glfwSetWindowShouldClose(GLFWwindow* handle, int value)
{
    _GLFWwindow* window = (_GLFWwindow*)handle;
    assert(window != NULL);

    _GLFW_REQUIRE_INIT();
    window->shouldClose = value;
}

GLFWAPI void glfwSetWindowTitle(GLFWwindow* handle, const char* title)
{
    _GLFWwindow* window = (_GLFWwindow*)handle;
    assert(window != NULL);
    assert(title != NULL);

    _GLFW_REQUIRE_INIT();
    _glfwPlatformSetWindowTitle(window, title);
}

GLFWAPI void glfwSetWindowIcon(GLFWwindow* handle,
    int count, const GLFWimage* images)
{
    int i;
    _GLFWwindow* window = (_GLFWwindow*)handle;

    assert(window != NULL);
    assert(count >= 0);
    assert(count == 0 || images != NULL);

    _GLFW_REQUIRE_INIT();

    if (count < 0)
    {
        _glfwInputError(GLFW_INVALID_VALUE, "Invalid image count for window icon");
        return;
    }

    for (i = 0; i < count; i++)
    {
        assert(images[i].pixels != NULL);

        if (images[i].width <= 0 || images[i].height <= 0)
        {
            _glfwInputError(GLFW_INVALID_VALUE,
                "Invalid image dimensions for window icon");
            return;
        }
    }

    _glfwPlatformSetWindowIcon(window, count, images);
}

GLFWAPI void glfwGetWindowPos(GLFWwindow* handle, int* xpos, int* ypos)
{
    _GLFWwindow* window = (_GLFWwindow*)handle;
    assert(window != NULL);

    if (xpos)
        *xpos = 0;
    if (ypos)
        *ypos = 0;

    _GLFW_REQUIRE_INIT();
    _glfwPlatformGetWindowPos(window, xpos, ypos);
}

GLFWAPI void glfwSetWindowPos(GLFWwindow* handle, int xpos, int ypos)
{
    _GLFWwindow* window = (_GLFWwindow*)handle;
    assert(window != NULL);

    _GLFW_REQUIRE_INIT();

    if (window->monitor)
        return;

    _glfwPlatformSetWindowPos(window, xpos, ypos);
}

GLFWAPI void glfwGetWindowSize(GLFWwindow* handle, int* width, int* height)
{
    _GLFWwindow* window = (_GLFWwindow*)handle;
    assert(window != NULL);

    if (width)
        *width = 0;
    if (height)
        *height = 0;

    _GLFW_REQUIRE_INIT();
    _glfwPlatformGetWindowSize(window, width, height);
}

GLFWAPI void glfwSetWindowSize(GLFWwindow* handle, int width, int height)
{
    _GLFWwindow* window = (_GLFWwindow*)handle;
    assert(window != NULL);
    assert(width >= 0);
    assert(height >= 0);

    _GLFW_REQUIRE_INIT();

    window->videoMode.width = width;
    window->videoMode.height = height;

    _glfwPlatformSetWindowSize(window, width, height);
}

GLFWAPI void glfwSetWindowSizeLimits(GLFWwindow* handle,
    int minwidth, int minheight,
    int maxwidth, int maxheight)
{
    _GLFWwindow* window = (_GLFWwindow*)handle;
    assert(window != NULL);

    _GLFW_REQUIRE_INIT();

    if (minwidth != GLFW_DONT_CARE && minheight != GLFW_DONT_CARE)
    {
        if (minwidth < 0 || minheight < 0)
        {
            _glfwInputError(GLFW_INVALID_VALUE,
                "Invalid window minimum size %ix%i",
                minwidth, minheight);
            return;
        }
    }

    if (maxwidth != GLFW_DONT_CARE && maxheight != GLFW_DONT_CARE)
    {
        if (maxwidth < 0 || maxheight < 0 ||
            maxwidth < minwidth || maxheight < minheight)
        {
            _glfwInputError(GLFW_INVALID_VALUE,
                "Invalid window maximum size %ix%i",
                maxwidth, maxheight);
            return;
        }
    }

    window->minwidth = minwidth;
    window->minheight = minheight;
    window->maxwidth = maxwidth;
    window->maxheight = maxheight;

    if (window->monitor || !window->resizable)
        return;

    _glfwPlatformSetWindowSizeLimits(window,
        minwidth, minheight,
        maxwidth, maxheight);
}

GLFWAPI void glfwSetWindowAspectRatio(GLFWwindow* handle, int numer, int denom)
{
    _GLFWwindow* window = (_GLFWwindow*)handle;
    assert(window != NULL);
    assert(numer != 0);
    assert(denom != 0);

    _GLFW_REQUIRE_INIT();

    if (numer != GLFW_DONT_CARE && denom != GLFW_DONT_CARE)
    {
        if (numer <= 0 || denom <= 0)
        {
            _glfwInputError(GLFW_INVALID_VALUE,
                "Invalid window aspect ratio %i:%i",
                numer, denom);
            return;
        }
    }

    window->numer = numer;
    window->denom = denom;

    if (window->monitor || !window->resizable)
        return;

    _glfwPlatformSetWindowAspectRatio(window, numer, denom);
}

GLFWAPI void glfwGetFramebufferSize(GLFWwindow* handle, int* width, int* height)
{
    _GLFWwindow* window = (_GLFWwindow*)handle;
    assert(window != NULL);

    if (width)
        *width = 0;
    if (height)
        *height = 0;

    _GLFW_REQUIRE_INIT();
    _glfwPlatformGetFramebufferSize(window, width, height);
}

GLFWAPI void glfwGetWindowFrameSize(GLFWwindow* handle,
    int* left, int* top,
    int* right, int* bottom)
{
    _GLFWwindow* window = (_GLFWwindow*)handle;
    assert(window != NULL);

    if (left)
        *left = 0;
    if (top)
        *top = 0;
    if (right)
        *right = 0;
    if (bottom)
        *bottom = 0;

    _GLFW_REQUIRE_INIT();
    _glfwPlatformGetWindowFrameSize(window, left, top, right, bottom);
}

GLFWAPI void glfwGetWindowContentScale(GLFWwindow* handle,
    float* xscale, float* yscale)
{
    _GLFWwindow* window = (_GLFWwindow*)handle;
    assert(window != NULL);

    if (xscale)
        *xscale = 0.f;
    if (yscale)
        *yscale = 0.f;

    _GLFW_REQUIRE_INIT();
    _glfwPlatformGetWindowContentScale(window, xscale, yscale);
}

GLFWAPI float glfwGetWindowOpacity(GLFWwindow* handle)
{
    _GLFWwindow* window = (_GLFWwindow*)handle;
    assert(window != NULL);

    _GLFW_REQUIRE_INIT_OR_RETURN(1.f);
    return _glfwPlatformGetWindowOpacity(window);
}

GLFWAPI void glfwSetWindowOpacity(GLFWwindow* handle, float opacity)
{
    _GLFWwindow* window = (_GLFWwindow*)handle;
    assert(window != NULL);
    assert(opacity == opacity);
    assert(opacity >= 0.f);
    assert(opacity <= 1.f);

    _GLFW_REQUIRE_INIT();

    if (opacity != opacity || opacity < 0.f || opacity > 1.f)
    {
        _glfwInputError(GLFW_INVALID_VALUE, "Invalid window opacity %f", opacity);
        return;
    }

    _glfwPlatformSetWindowOpacity(window, opacity);
}

GLFWAPI void glfwIconifyWindow(GLFWwindow* handle)
{
    _GLFWwindow* window = (_GLFWwindow*)handle;
    assert(window != NULL);

    _GLFW_REQUIRE_INIT();
    _glfwPlatformIconifyWindow(window);
}

GLFWAPI void glfwRestoreWindow(GLFWwindow* handle)
{
    _GLFWwindow* window = (_GLFWwindow*)handle;
    assert(window != NULL);

    _GLFW_REQUIRE_INIT();
    _glfwPlatformRestoreWindow(window);
}

GLFWAPI void glfwMaximizeWindow(GLFWwindow* handle)
{
    _GLFWwindow* window = (_GLFWwindow*)handle;
    assert(window != NULL);

    _GLFW_REQUIRE_INIT();

    if (window->monitor)
        return;

    _glfwPlatformMaximizeWindow(window);
}

GLFWAPI void glfwShowWindow(GLFWwindow* handle)
{
    _GLFWwindow* window = (_GLFWwindow*)handle;
    assert(window != NULL);

    _GLFW_REQUIRE_INIT();

    if (window->monitor)
        return;

    _glfwPlatformShowWindow(window);

    if (window->focusOnShow)
        _glfwPlatformFocusWindow(window);
}

GLFWAPI void glfwRequestWindowAttention(GLFWwindow* handle)
{
    _GLFWwindow* window = (_GLFWwindow*)handle;
    assert(window != NULL);

    _GLFW_REQUIRE_INIT();

    _glfwPlatformRequestWindowAttention(window);
}

GLFWAPI void glfwHideWindow(GLFWwindow* handle)
{
    _GLFWwindow* window = (_GLFWwindow*)handle;
    assert(window != NULL);

    _GLFW_REQUIRE_INIT();

    if (window->monitor)
        return;

    _glfwPlatformHideWindow(window);
}

GLFWAPI void glfwFocusWindow(GLFWwindow* handle)
{
    _GLFWwindow* window = (_GLFWwindow*)handle;
    assert(window != NULL);

    _GLFW_REQUIRE_INIT();

    _glfwPlatformFocusWindow(window);
}

GLFWAPI int glfwGetWindowAttrib(GLFWwindow* handle, int attrib)
{
    _GLFWwindow* window = (_GLFWwindow*)handle;
    assert(window != NULL);

    _GLFW_REQUIRE_INIT_OR_RETURN(0);

    switch (attrib)
    {
    case GLFW_FOCUSED:
        return _glfwPlatformWindowFocused(window);
    case GLFW_ICONIFIED:
        return _glfwPlatformWindowIconified(window);
    case GLFW_VISIBLE:
        return _glfwPlatformWindowVisible(window);
    case GLFW_MAXIMIZED:
        return _glfwPlatformWindowMaximized(window);
    case GLFW_HOVERED:
        return _glfwPlatformWindowHovered(window);
    case GLFW_FOCUS_ON_SHOW:
        return window->focusOnShow;
    case GLFW_TRANSPARENT_FRAMEBUFFER:
        return _glfwPlatformFramebufferTransparent(window);
    case GLFW_RESIZABLE:
        return window->resizable;
    case GLFW_DECORATED:
        return window->decorated;
    case GLFW_FLOATING:
        return window->floating;
    case GLFW_AUTO_ICONIFY:
        return window->autoIconify;
    case GLFW_CLIENT_API:
        return window->context.client;
    case GLFW_CONTEXT_CREATION_API:
        return window->context.source;
    case GLFW_CONTEXT_VERSION_MAJOR:
        return window->context.major;
    case GLFW_CONTEXT_VERSION_MINOR:
        return window->context.minor;
    case GLFW_CONTEXT_REVISION:
        return window->context.revision;
    case GLFW_CONTEXT_ROBUSTNESS:
        return window->context.robustness;
    case GLFW_OPENGL_FORWARD_COMPAT:
        return window->context.forward;
    case GLFW_OPENGL_DEBUG_CONTEXT:
        return window->context.debug;
    case GLFW_OPENGL_PROFILE:
        return window->context.profile;
    case GLFW_CONTEXT_RELEASE_BEHAVIOR:
        return window->context.release;
    case GLFW_CONTEXT_NO_ERROR:
        return window->context.noerror;
    }

    _glfwInputError(GLFW_INVALID_ENUM, "Invalid window attribute 0x%08X", attrib);
    return 0;
}

GLFWAPI void glfwSetWindowAttrib(GLFWwindow* handle, int attrib, int value)
{
    _GLFWwindow* window = (_GLFWwindow*)handle;
    assert(window != NULL);

    _GLFW_REQUIRE_INIT();

    value = value ? GLFW_TRUE : GLFW_FALSE;

    if (attrib == GLFW_AUTO_ICONIFY)
        window->autoIconify = value;
    else if (attrib == GLFW_RESIZABLE)
    {
        if (window->resizable == value)
            return;

        window->resizable = value;
        if (!window->monitor)
            _glfwPlatformSetWindowResizable(window, value);
    }
    else if (attrib == GLFW_DECORATED)
    {
        if (window->decorated == value)
            return;

        window->decorated = value;
        if (!window->monitor)
            _glfwPlatformSetWindowDecorated(window, value);
    }
    else if (attrib == GLFW_FLOATING)
    {
        if (window->floating == value)
            return;

        window->floating = value;
        if (!window->monitor)
            _glfwPlatformSetWindowFloating(window, value);
    }
    else if (attrib == GLFW_FOCUS_ON_SHOW)
        window->focusOnShow = value;
    else
        _glfwInputError(GLFW_INVALID_ENUM, "Invalid window attribute 0x%08X", attrib);
}

GLFWAPI GLFWmonitor* glfwGetWindowMonitor(GLFWwindow* handle)
{
    _GLFWwindow* window = (_GLFWwindow*)handle;
    assert(window != NULL);

    _GLFW_REQUIRE_INIT_OR_RETURN(NULL);
    return (GLFWmonitor*)window->monitor;
}

GLFWAPI void glfwSetWindowMonitor(GLFWwindow* wh,
    GLFWmonitor* mh,
    int xpos, int ypos,
    int width, int height,
    int refreshRate)
{
    _GLFWwindow* window = (_GLFWwindow*)wh;
    _GLFWmonitor* monitor = (_GLFWmonitor*)mh;
    assert(window != NULL);
    assert(width >= 0);
    assert(height >= 0);

    _GLFW_REQUIRE_INIT();

    if (width <= 0 || height <= 0)
    {
        _glfwInputError(GLFW_INVALID_VALUE,
            "Invalid window size %ix%i",
            width, height);
        return;
    }

    if (refreshRate < 0 && refreshRate != GLFW_DONT_CARE)
    {
        _glfwInputError(GLFW_INVALID_VALUE,
            "Invalid refresh rate %i",
            refreshRate);
        return;
    }

    window->videoMode.width = width;
    window->videoMode.height = height;
    window->videoMode.refreshRate = refreshRate;

    _glfwPlatformSetWindowMonitor(window, monitor,
        xpos, ypos, width, height,
        refreshRate);
}

GLFWAPI void glfwSetWindowUserPointer(GLFWwindow* handle, void* pointer)
{
    _GLFWwindow* window = (_GLFWwindow*)handle;
    assert(window != NULL);

    _GLFW_REQUIRE_INIT();
    window->userPointer = pointer;
}

GLFWAPI void* glfwGetWindowUserPointer(GLFWwindow* handle)
{
    _GLFWwindow* window = (_GLFWwindow*)handle;
    assert(window != NULL);

    _GLFW_REQUIRE_INIT_OR_RETURN(NULL);
    return window->userPointer;
}


GLFWAPI void glfwPollEvents(void)
{
    _GLFW_REQUIRE_INIT();
    _glfwPlatformPollEvents();
}

GLFWAPI void glfwWaitEvents(void)
{
    _GLFW_REQUIRE_INIT();
    _glfwPlatformWaitEvents();
}

GLFWAPI void glfwWaitEventsTimeout(double timeout)
{
    _GLFW_REQUIRE_INIT();
    assert(timeout == timeout);
    assert(timeout >= 0.0);
    assert(timeout <= DBL_MAX);

    if (timeout != timeout || timeout < 0.0 || timeout > DBL_MAX)
    {
        _glfwInputError(GLFW_INVALID_VALUE, "Invalid time %f", timeout);
        return;
    }

    _glfwPlatformWaitEventsTimeout(timeout);
}

GLFWAPI void glfwPostEmptyEvent(void)
{
    _GLFW_REQUIRE_INIT();
    _glfwPlatformPostEmptyEvent();
}



static int compareVideoModes(const void* fp, const void* sp)
{
    const GLFWvidmode* fm = (const GLFWvidmode*)fp;
    const GLFWvidmode* sm = (const GLFWvidmode*)sp;
    const int fbpp = fm->redBits + fm->greenBits + fm->blueBits;
    const int sbpp = sm->redBits + sm->greenBits + sm->blueBits;
    const int farea = fm->width * fm->height;
    const int sarea = sm->width * sm->height;

    // First sort on color bits per pixel
    if (fbpp != sbpp)
        return fbpp - sbpp;

    // Then sort on screen area
    if (farea != sarea)
        return farea - sarea;

    // Then sort on width
    if (fm->width != sm->width)
        return fm->width - sm->width;

    // Lastly sort on refresh rate
    return fm->refreshRate - sm->refreshRate;
}

// Retrieves the available modes for the specified monitor
//
static GLFWbool refreshVideoModes(_GLFWmonitor* monitor)
{
    int modeCount;
    GLFWvidmode* modes;

    if (monitor->modes)
        return GLFW_TRUE;

    modes = _glfwPlatformGetVideoModes(monitor, &modeCount);
    if (!modes)
        return GLFW_FALSE;

    qsort(modes, modeCount, sizeof(GLFWvidmode), compareVideoModes);

    free(monitor->modes);
    monitor->modes = modes;
    monitor->modeCount = modeCount;

    return GLFW_TRUE;
}


//////////////////////////////////////////////////////////////////////////
//////                         GLFW event API                       //////
//////////////////////////////////////////////////////////////////////////

// Notifies shared code of a monitor connection or disconnection
//
void _glfwInputMonitor(_GLFWmonitor* monitor, int action, int placement)
{
    if (action == GLFW_CONNECTED)
    {
        _glfw.monitorCount++;
        _glfw.monitors =(_GLFWmonitor**)
            realloc(_glfw.monitors, sizeof(_GLFWmonitor*) * _glfw.monitorCount);

        if (placement == _GLFW_INSERT_FIRST)
        {
            memmove(_glfw.monitors + 1,
                _glfw.monitors,
                ((size_t)_glfw.monitorCount - 1) * sizeof(_GLFWmonitor*));
            _glfw.monitors[0] = monitor;
        }
        else
            _glfw.monitors[_glfw.monitorCount - 1] = monitor;
    }
    else if (action == GLFW_DISCONNECTED)
    {
        int i;
        _GLFWwindow* window;

        for (window = _glfw.windowListHead; window; window = window->next)
        {
            if (window->monitor == monitor)
            {
                int width, height, xoff, yoff;
                _glfwPlatformGetWindowSize(window, &width, &height);
                _glfwPlatformSetWindowMonitor(window, NULL, 0, 0, width, height, 0);
                _glfwPlatformGetWindowFrameSize(window, &xoff, &yoff, NULL, NULL);
                _glfwPlatformSetWindowPos(window, xoff, yoff);
            }
        }

        for (i = 0; i < _glfw.monitorCount; i++)
        {
            if (_glfw.monitors[i] == monitor)
            {
                _glfw.monitorCount--;
                memmove(_glfw.monitors + i,
                    _glfw.monitors + i + 1,
                    ((size_t)_glfw.monitorCount - i) * sizeof(_GLFWmonitor*));
                break;
            }
        }
    }

    if (action == GLFW_DISCONNECTED)
        _glfwFreeMonitor(monitor);
}

// Notifies shared code that a full screen window has acquired or released
// a monitor
//
void _glfwInputMonitorWindow(_GLFWmonitor* monitor, _GLFWwindow* window)
{
    monitor->window = window;
}


//////////////////////////////////////////////////////////////////////////
//////                       GLFW internal API                      //////
//////////////////////////////////////////////////////////////////////////

// Allocates and returns a monitor object with the specified name and dimensions
//
_GLFWmonitor* _glfwAllocMonitor(const char* name, int widthMM, int heightMM)
{
    _GLFWmonitor* monitor = (_GLFWmonitor*)calloc(1, sizeof(_GLFWmonitor));
    monitor->widthMM = widthMM;
    monitor->heightMM = heightMM;

    strncpy(monitor->name, name, sizeof(monitor->name) - 1);

    return monitor;
}

// Frees a monitor object and any data associated with it
//
void _glfwFreeMonitor(_GLFWmonitor* monitor)
{
    if (monitor == NULL)
        return;

    _glfwPlatformFreeMonitor(monitor);

    _glfwFreeGammaArrays(&monitor->originalRamp);
    _glfwFreeGammaArrays(&monitor->currentRamp);

    free(monitor->modes);
    free(monitor);
}

// Allocates red, green and blue value arrays of the specified size
//
void _glfwAllocGammaArrays(GLFWgammaramp* ramp, unsigned int size)
{
    ramp->red   = (unsigned short*)calloc(size, sizeof(unsigned short));
    ramp->green = (unsigned short*)calloc(size, sizeof(unsigned short));
    ramp->blue  = (unsigned short*)calloc(size, sizeof(unsigned short));
    ramp->size  = size;
}

// Frees the red, green and blue value arrays and clears the struct
//
void _glfwFreeGammaArrays(GLFWgammaramp* ramp)
{
    free(ramp->red);
    free(ramp->green);
    free(ramp->blue);

    memset(ramp, 0, sizeof(GLFWgammaramp));
}

// Chooses the video mode most closely matching the desired one
//
const GLFWvidmode* _glfwChooseVideoMode(_GLFWmonitor* monitor,
    const GLFWvidmode* desired)
{
    int i;
    unsigned int sizeDiff, leastSizeDiff = UINT_MAX;
    unsigned int rateDiff, leastRateDiff = UINT_MAX;
    unsigned int colorDiff, leastColorDiff = UINT_MAX;
    const GLFWvidmode* current;
    const GLFWvidmode* closest = NULL;

    if (!refreshVideoModes(monitor))
        return NULL;

    for (i = 0; i < monitor->modeCount; i++)
    {
        current = monitor->modes + i;

        colorDiff = 0;

        if (desired->redBits != GLFW_DONT_CARE)
            colorDiff += abs(current->redBits - desired->redBits);
        if (desired->greenBits != GLFW_DONT_CARE)
            colorDiff += abs(current->greenBits - desired->greenBits);
        if (desired->blueBits != GLFW_DONT_CARE)
            colorDiff += abs(current->blueBits - desired->blueBits);

        sizeDiff = abs((current->width - desired->width) *
            (current->width - desired->width) +
            (current->height - desired->height) *
            (current->height - desired->height));

        if (desired->refreshRate != GLFW_DONT_CARE)
            rateDiff = abs(current->refreshRate - desired->refreshRate);
        else
            rateDiff = UINT_MAX - current->refreshRate;

        if ((colorDiff < leastColorDiff) ||
            (colorDiff == leastColorDiff && sizeDiff < leastSizeDiff) ||
            (colorDiff == leastColorDiff && sizeDiff == leastSizeDiff && rateDiff < leastRateDiff))
        {
            closest = current;
            leastSizeDiff = sizeDiff;
            leastRateDiff = rateDiff;
            leastColorDiff = colorDiff;
        }
    }

    return closest;
}

// Performs lexical comparison between two @ref GLFWvidmode structures
//
int _glfwCompareVideoModes(const GLFWvidmode* fm, const GLFWvidmode* sm)
{
    return compareVideoModes(fm, sm);
}

// Splits a color depth into red, green and blue bit depths
//
void _glfwSplitBPP(int bpp, int* red, int* green, int* blue)
{
    int delta;

    // We assume that by 32 the user really meant 24
    if (bpp == 32)
        bpp = 24;

    // Convert "bits per pixel" to red, green & blue sizes

    *red = *green = *blue = bpp / 3;
    delta = bpp - (*red * 3);
    if (delta >= 1)
        *green = *green + 1;

    if (delta == 2)
        *red = *red + 1;
}


//////////////////////////////////////////////////////////////////////////
//////                        GLFW public API                       //////
//////////////////////////////////////////////////////////////////////////

GLFWAPI GLFWmonitor** glfwGetMonitors(int* count)
{
    assert(count != NULL);

    *count = 0;

    _GLFW_REQUIRE_INIT_OR_RETURN(NULL);

    *count = _glfw.monitorCount;
    return (GLFWmonitor**)_glfw.monitors;
}

GLFWAPI GLFWmonitor* glfwGetPrimaryMonitor(void)
{
    _GLFW_REQUIRE_INIT_OR_RETURN(NULL);

    if (!_glfw.monitorCount)
        return NULL;

    return (GLFWmonitor*)_glfw.monitors[0];
}

GLFWAPI void glfwGetMonitorPos(GLFWmonitor* handle, int* xpos, int* ypos)
{
    _GLFWmonitor* monitor = (_GLFWmonitor*)handle;
    assert(monitor != NULL);

    if (xpos)
        *xpos = 0;
    if (ypos)
        *ypos = 0;

    _GLFW_REQUIRE_INIT();

    _glfwPlatformGetMonitorPos(monitor, xpos, ypos);
}

GLFWAPI void glfwGetMonitorWorkarea(GLFWmonitor* handle,
    int* xpos, int* ypos,
    int* width, int* height)
{
    _GLFWmonitor* monitor = (_GLFWmonitor*)handle;
    assert(monitor != NULL);

    if (xpos)
        *xpos = 0;
    if (ypos)
        *ypos = 0;
    if (width)
        *width = 0;
    if (height)
        *height = 0;

    _GLFW_REQUIRE_INIT();

    _glfwPlatformGetMonitorWorkarea(monitor, xpos, ypos, width, height);
}

GLFWAPI void glfwGetMonitorPhysicalSize(GLFWmonitor* handle, int* widthMM, int* heightMM)
{
    _GLFWmonitor* monitor = (_GLFWmonitor*)handle;
    assert(monitor != NULL);

    if (widthMM)
        *widthMM = 0;
    if (heightMM)
        *heightMM = 0;

    _GLFW_REQUIRE_INIT();

    if (widthMM)
        *widthMM = monitor->widthMM;
    if (heightMM)
        *heightMM = monitor->heightMM;
}

GLFWAPI void glfwGetMonitorContentScale(GLFWmonitor* handle,
    float* xscale, float* yscale)
{
    _GLFWmonitor* monitor = (_GLFWmonitor*)handle;
    assert(monitor != NULL);

    if (xscale)
        *xscale = 0.f;
    if (yscale)
        *yscale = 0.f;

    _GLFW_REQUIRE_INIT();
    _glfwPlatformGetMonitorContentScale(monitor, xscale, yscale);
}

GLFWAPI const char* glfwGetMonitorName(GLFWmonitor* handle)
{
    _GLFWmonitor* monitor = (_GLFWmonitor*)handle;
    assert(monitor != NULL);

    _GLFW_REQUIRE_INIT_OR_RETURN(NULL);
    return monitor->name;
}

GLFWAPI void glfwSetMonitorUserPointer(GLFWmonitor* handle, void* pointer)
{
    _GLFWmonitor* monitor = (_GLFWmonitor*)handle;
    assert(monitor != NULL);

    _GLFW_REQUIRE_INIT();
    monitor->userPointer = pointer;
}

GLFWAPI void* glfwGetMonitorUserPointer(GLFWmonitor* handle)
{
    _GLFWmonitor* monitor = (_GLFWmonitor*)handle;
    assert(monitor != NULL);

    _GLFW_REQUIRE_INIT_OR_RETURN(NULL);
    return monitor->userPointer;
}



GLFWAPI const GLFWvidmode* glfwGetVideoModes(GLFWmonitor* handle, int* count)
{
    _GLFWmonitor* monitor = (_GLFWmonitor*)handle;
    assert(monitor != NULL);
    assert(count != NULL);

    *count = 0;

    _GLFW_REQUIRE_INIT_OR_RETURN(NULL);

    if (!refreshVideoModes(monitor))
        return NULL;

    *count = monitor->modeCount;
    return monitor->modes;
}

GLFWAPI const GLFWvidmode* glfwGetVideoMode(GLFWmonitor* handle)
{
    _GLFWmonitor* monitor = (_GLFWmonitor*)handle;
    assert(monitor != NULL);

    _GLFW_REQUIRE_INIT_OR_RETURN(NULL);

    _glfwPlatformGetVideoMode(monitor, &monitor->currentMode);
    return &monitor->currentMode;
}

GLFWAPI void glfwSetGamma(GLFWmonitor* handle, float gamma)
{
    unsigned int i;
    unsigned short* values;
    GLFWgammaramp ramp;
    const GLFWgammaramp* original;
    assert(handle != NULL);
    assert(gamma > 0.f);
    assert(gamma <= FLT_MAX);

    _GLFW_REQUIRE_INIT();

    if (gamma != gamma || gamma <= 0.f || gamma > FLT_MAX)
    {
        _glfwInputError(GLFW_INVALID_VALUE, "Invalid gamma value %f", gamma);
        return;
    }

    original = glfwGetGammaRamp(handle);
    if (!original)
        return;

    values = (unsigned short*)calloc(original->size, sizeof(unsigned short));

    for (i = 0; i < original->size; i++)
    {
        float value;

        // Calculate intensity
        value = i / (float)(original->size - 1);
        // Apply gamma curve
        value = powf(value, 1.f / gamma) * 65535.f + 0.5f;
        // Clamp to value range
        value = _glfw_fminf(value, 65535.f);

        values[i] = (unsigned short)value;
    }

    ramp.red = values;
    ramp.green = values;
    ramp.blue = values;
    ramp.size = original->size;

    glfwSetGammaRamp(handle, &ramp);
    free(values);
}

GLFWAPI const GLFWgammaramp* glfwGetGammaRamp(GLFWmonitor* handle)
{
    _GLFWmonitor* monitor = (_GLFWmonitor*)handle;
    assert(monitor != NULL);

    _GLFW_REQUIRE_INIT_OR_RETURN(NULL);

    _glfwFreeGammaArrays(&monitor->currentRamp);
    if (!_glfwPlatformGetGammaRamp(monitor, &monitor->currentRamp))
        return NULL;

    return &monitor->currentRamp;
}

GLFWAPI void glfwSetGammaRamp(GLFWmonitor* handle, const GLFWgammaramp* ramp)
{
    _GLFWmonitor* monitor = (_GLFWmonitor*)handle;
    assert(monitor != NULL);
    assert(ramp != NULL);
    assert(ramp->size > 0);
    assert(ramp->red != NULL);
    assert(ramp->green != NULL);
    assert(ramp->blue != NULL);

    _GLFW_REQUIRE_INIT();

    if (ramp->size <= 0)
    {
        _glfwInputError(GLFW_INVALID_VALUE,
            "Invalid gamma ramp size %i",
            ramp->size);
        return;
    }

    if (!monitor->originalRamp.size)
    {
        if (!_glfwPlatformGetGammaRamp(monitor, &monitor->originalRamp))
            return;
    }

    _glfwPlatformSetGammaRamp(monitor, ramp);
}



// Internal key state used for sticky keys
#define _GLFW_STICK 3

//
void _glfwInputKey(_GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key >= 0 && key <= GLFW_KEY_LAST)
    {
        GLFWbool repeated = GLFW_FALSE;

        if (action == GLFW_RELEASE && window->keys[key] == GLFW_RELEASE)
            return;

        if (action == GLFW_PRESS && window->keys[key] == GLFW_PRESS)
            repeated = GLFW_TRUE;

        if (action == GLFW_RELEASE && window->stickyKeys)
            window->keys[key] = _GLFW_STICK;
        else
            window->keys[key] = (char)action;

        if (repeated)
            action = GLFW_REPEAT;
    }

    if (!window->lockKeyMods)
        mods &= ~(GLFW_MOD_CAPS_LOCK | GLFW_MOD_NUM_LOCK);

}

// Notifies shared code of a Unicode codepoint input event
// The 'plain' parameter determines whether to emit a regular character event
//
void _glfwInputChar(_GLFWwindow* window, uint32_t codepoint, int mods, GLFWbool plain)
{
    if (codepoint < 32 || (codepoint > 126 && codepoint < 160))
        return;

    if (!window->lockKeyMods)
        mods &= ~(GLFW_MOD_CAPS_LOCK | GLFW_MOD_NUM_LOCK);

}

// Notifies shared code of a scroll event
//
void _glfwInputScroll(_GLFWwindow* window, double xoffset, double yoffset)
{

}

// Notifies shared code of a mouse button click event
//
void _glfwInputMouseClick(_GLFWwindow* window, int button, int action, int mods)
{
    if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST)
        return;

    if (!window->lockKeyMods)
        mods &= ~(GLFW_MOD_CAPS_LOCK | GLFW_MOD_NUM_LOCK);

    if (action == GLFW_RELEASE && window->stickyMouseButtons)
        window->mouseButtons[button] = _GLFW_STICK;
    else
        window->mouseButtons[button] = (char)action;

}

// Notifies shared code of a cursor motion event
// The position is specified in content area relative screen coordinates
//
void _glfwInputCursorPos(_GLFWwindow* window, double xpos, double ypos)
{
    if (window->virtualCursorPosX == xpos && window->virtualCursorPosY == ypos)
        return;

    window->virtualCursorPosX = xpos;
    window->virtualCursorPosY = ypos;

}

// Notifies shared code of a cursor enter/leave event
//
void _glfwInputCursorEnter(_GLFWwindow* window, GLFWbool entered)
{
}

// Notifies shared code of files or directories dropped on a window
//
void _glfwInputDrop(_GLFWwindow* window, int count, const char** paths)
{
}





// Notifies shared code of the new value of a joystick hat


//////////////////////////////////////////////////////////////////////////
//////                       GLFW internal API                      //////
//////////////////////////////////////////////////////////////////////////




// Center the cursor in the content area of the specified window
//
void _glfwCenterCursorInContentArea(_GLFWwindow* window)
{
    int width, height;

    _glfwPlatformGetWindowSize(window, &width, &height);
    _glfwPlatformSetCursorPos(window, width / 2.0, height / 2.0);
}


//////////////////////////////////////////////////////////////////////////
//////                        GLFW public API                       //////
//////////////////////////////////////////////////////////////////////////

GLFWAPI int glfwGetInputMode(GLFWwindow* handle, int mode)
{
    _GLFWwindow* window = (_GLFWwindow*)handle;
    assert(window != NULL);

    _GLFW_REQUIRE_INIT_OR_RETURN(0);

    switch (mode)
    {
    case GLFW_CURSOR:
        return window->cursorMode;
    case GLFW_STICKY_KEYS:
        return window->stickyKeys;
    case GLFW_STICKY_MOUSE_BUTTONS:
        return window->stickyMouseButtons;
    case GLFW_LOCK_KEY_MODS:
        return window->lockKeyMods;
    case GLFW_RAW_MOUSE_MOTION:
        return window->rawMouseMotion;
    }

    _glfwInputError(GLFW_INVALID_ENUM, "Invalid input mode 0x%08X", mode);
    return 0;
}

GLFWAPI void glfwSetInputMode(GLFWwindow* handle, int mode, int value)
{
    _GLFWwindow* window = (_GLFWwindow*)handle;
    assert(window != NULL);

    _GLFW_REQUIRE_INIT();

    if (mode == GLFW_CURSOR)
    {
        if (value != GLFW_CURSOR_NORMAL &&
            value != GLFW_CURSOR_HIDDEN &&
            value != GLFW_CURSOR_DISABLED)
        {
            _glfwInputError(GLFW_INVALID_ENUM,
                "Invalid cursor mode 0x%08X",
                value);
            return;
        }

        if (window->cursorMode == value)
            return;

        window->cursorMode = value;

        _glfwPlatformGetCursorPos(window,
            &window->virtualCursorPosX,
            &window->virtualCursorPosY);
        _glfwPlatformSetCursorMode(window, value);
    }
    else if (mode == GLFW_STICKY_KEYS)
    {
        value = value ? GLFW_TRUE : GLFW_FALSE;
        if (window->stickyKeys == value)
            return;

        if (!value)
        {
            int i;

            // Release all sticky keys
            for (i = 0; i <= GLFW_KEY_LAST; i++)
            {
                if (window->keys[i] == _GLFW_STICK)
                    window->keys[i] = GLFW_RELEASE;
            }
        }

        window->stickyKeys = value;
    }
    else if (mode == GLFW_STICKY_MOUSE_BUTTONS)
    {
        value = value ? GLFW_TRUE : GLFW_FALSE;
        if (window->stickyMouseButtons == value)
            return;

        if (!value)
        {
            int i;

            // Release all sticky mouse buttons
            for (i = 0; i <= GLFW_MOUSE_BUTTON_LAST; i++)
            {
                if (window->mouseButtons[i] == _GLFW_STICK)
                    window->mouseButtons[i] = GLFW_RELEASE;
            }
        }

        window->stickyMouseButtons = value;
    }
    else if (mode == GLFW_LOCK_KEY_MODS)
    {
        window->lockKeyMods = value ? GLFW_TRUE : GLFW_FALSE;
    }
    else if (mode == GLFW_RAW_MOUSE_MOTION)
    {
        if (!_glfwPlatformRawMouseMotionSupported())
        {
            _glfwInputError(GLFW_PLATFORM_ERROR,
                "Raw mouse motion is not supported on this system");
            return;
        }

        value = value ? GLFW_TRUE : GLFW_FALSE;
        if (window->rawMouseMotion == value)
            return;

        window->rawMouseMotion = value;
        _glfwPlatformSetRawMouseMotion(window, value);
    }
    else
        _glfwInputError(GLFW_INVALID_ENUM, "Invalid input mode 0x%08X", mode);
}

GLFWAPI int glfwRawMouseMotionSupported(void)
{
    _GLFW_REQUIRE_INIT_OR_RETURN(GLFW_FALSE);
    return _glfwPlatformRawMouseMotionSupported();
}

GLFWAPI const char* glfwGetKeyName(int key, int scancode)
{
    _GLFW_REQUIRE_INIT_OR_RETURN(NULL);

    if (key != GLFW_KEY_UNKNOWN)
    {
        if (key != GLFW_KEY_KP_EQUAL &&
            (key < GLFW_KEY_KP_0 || key > GLFW_KEY_KP_ADD) &&
            (key < GLFW_KEY_APOSTROPHE || key > GLFW_KEY_WORLD_2))
        {
            return NULL;
        }

        scancode = _glfwPlatformGetKeyScancode(key);
    }

    return _glfwPlatformGetScancodeName(scancode);
}

GLFWAPI int glfwGetKeyScancode(int key)
{
    _GLFW_REQUIRE_INIT_OR_RETURN(-1);

    if (key < GLFW_KEY_SPACE || key > GLFW_KEY_LAST)
    {
        _glfwInputError(GLFW_INVALID_ENUM, "Invalid key %i", key);
        return GLFW_RELEASE;
    }

    return _glfwPlatformGetKeyScancode(key);
}

GLFWAPI int glfwGetKey(GLFWwindow* handle, int key)
{
    _GLFWwindow* window = (_GLFWwindow*)handle;
    assert(window != NULL);

    _GLFW_REQUIRE_INIT_OR_RETURN(GLFW_RELEASE);

    if (key < GLFW_KEY_SPACE || key > GLFW_KEY_LAST)
    {
        _glfwInputError(GLFW_INVALID_ENUM, "Invalid key %i", key);
        return GLFW_RELEASE;
    }

    if (window->keys[key] == _GLFW_STICK)
    {
        // Sticky mode: release key now
        window->keys[key] = GLFW_RELEASE;
        return GLFW_PRESS;
    }

    return (int)window->keys[key];
}

GLFWAPI int glfwGetMouseButton(GLFWwindow* handle, int button)
{
    _GLFWwindow* window = (_GLFWwindow*)handle;
    assert(window != NULL);

    _GLFW_REQUIRE_INIT_OR_RETURN(GLFW_RELEASE);

    if (button < GLFW_MOUSE_BUTTON_1 || button > GLFW_MOUSE_BUTTON_LAST)
    {
        _glfwInputError(GLFW_INVALID_ENUM, "Invalid mouse button %i", button);
        return GLFW_RELEASE;
    }

    if (window->mouseButtons[button] == _GLFW_STICK)
    {
        // Sticky mode: release mouse button now
        window->mouseButtons[button] = GLFW_RELEASE;
        return GLFW_PRESS;
    }

    return (int)window->mouseButtons[button];
}

GLFWAPI void glfwGetCursorPos(GLFWwindow* handle, double* xpos, double* ypos)
{
    _GLFWwindow* window = (_GLFWwindow*)handle;
    assert(window != NULL);

    if (xpos)
        *xpos = 0;
    if (ypos)
        *ypos = 0;

    _GLFW_REQUIRE_INIT();

    if (window->cursorMode == GLFW_CURSOR_DISABLED)
    {
        if (xpos)
            *xpos = window->virtualCursorPosX;
        if (ypos)
            *ypos = window->virtualCursorPosY;
    }
    else
        _glfwPlatformGetCursorPos(window, xpos, ypos);
}

GLFWAPI void glfwSetCursorPos(GLFWwindow* handle, double xpos, double ypos)
{
    _GLFWwindow* window = (_GLFWwindow*)handle;
    assert(window != NULL);

    _GLFW_REQUIRE_INIT();

    if (xpos != xpos || xpos < -DBL_MAX || xpos > DBL_MAX ||
        ypos != ypos || ypos < -DBL_MAX || ypos > DBL_MAX)
    {
        _glfwInputError(GLFW_INVALID_VALUE,
            "Invalid cursor position %f %f",
            xpos, ypos);
        return;
    }

    if (!_glfwPlatformWindowFocused(window))
        return;

    if (window->cursorMode == GLFW_CURSOR_DISABLED)
    {
        // Only update the accumulated position if the cursor is disabled
        window->virtualCursorPosX = xpos;
        window->virtualCursorPosY = ypos;
    }
    else
    {
        // Update system cursor position
        _glfwPlatformSetCursorPos(window, xpos, ypos);
    }
}

GLFWAPI GLFWcursor* glfwCreateCursor(const GLFWimage* image, int xhot, int yhot)
{
    _GLFWcursor* cursor;

    assert(image != NULL);
    assert(image->pixels != NULL);

    _GLFW_REQUIRE_INIT_OR_RETURN(NULL);

    if (image->width <= 0 || image->height <= 0)
    {
        _glfwInputError(GLFW_INVALID_VALUE, "Invalid image dimensions for cursor");
        return NULL;
    }

    cursor =(_GLFWcursor*) calloc(1, sizeof(_GLFWcursor));
    cursor->next = _glfw.cursorListHead;
    _glfw.cursorListHead = cursor;

    if (!_glfwPlatformCreateCursor(cursor, image, xhot, yhot))
    {
        glfwDestroyCursor((GLFWcursor*)cursor);
        return NULL;
    }

    return (GLFWcursor*)cursor;
}


GLFWAPI void glfwDestroyCursor(GLFWcursor* handle)
{
    _GLFWcursor* cursor = (_GLFWcursor*)handle;

    _GLFW_REQUIRE_INIT();

    if (cursor == NULL)
        return;

    // Make sure the cursor is not being used by any window
    {
        _GLFWwindow* window;

        for (window = _glfw.windowListHead; window; window = window->next)
        {
            if (window->cursor == cursor)
                glfwSetCursor((GLFWwindow*)window, NULL);
        }
    }

    _glfwPlatformDestroyCursor(cursor);

    // Unlink cursor from global linked list
    {
        _GLFWcursor** prev = &_glfw.cursorListHead;

        while (*prev != cursor)
            prev = &((*prev)->next);

        *prev = cursor->next;
    }

    free(cursor);
}

GLFWAPI void glfwSetCursor(GLFWwindow* windowHandle, GLFWcursor* cursorHandle)
{
    _GLFWwindow* window = (_GLFWwindow*)windowHandle;
    _GLFWcursor* cursor = (_GLFWcursor*)cursorHandle;
    assert(window != NULL);

    _GLFW_REQUIRE_INIT();

    window->cursor = cursor;

    _glfwPlatformSetCursor(window, cursor);
}



GLFWAPI void glfwSetClipboardString(GLFWwindow* handle, const char* string)
{
    assert(string != NULL);

    _GLFW_REQUIRE_INIT();
    _glfwPlatformSetClipboardString(string);
}

GLFWAPI const char* glfwGetClipboardString(GLFWwindow* handle)
{
    _GLFW_REQUIRE_INIT_OR_RETURN(NULL);
    return _glfwPlatformGetClipboardString();
}




_GLFWlibrary _glfw = { GLFW_FALSE };

// These are outside of _glfw so they can be used before initialization and
// after termination without special handling when _glfw is cleared to zero
//
static _GLFWerror _glfwMainThreadError;
static GLFWerrorfun _glfwErrorCallback;
static _GLFWinitconfig _glfwInitHints =
{
    GLFW_TRUE,      // hat buttons
    {
        GLFW_TRUE,  // macOS menu bar
        GLFW_TRUE   // macOS bundle chdir
    }
};

// Terminate the library
//
static void glfw_terminate(void)
{
    int i;

    while (_glfw.windowListHead)
        glfwDestroyWindow((GLFWwindow*) _glfw.windowListHead);

    while (_glfw.cursorListHead)
        glfwDestroyCursor((GLFWcursor*) _glfw.cursorListHead);

    for (i = 0;  i < _glfw.monitorCount;  i++)
    {
        _GLFWmonitor* monitor = _glfw.monitors[i];
        if (monitor->originalRamp.size)
            _glfwPlatformSetGammaRamp(monitor, &monitor->originalRamp);
        _glfwFreeMonitor(monitor);
    }

    free(_glfw.monitors);
    _glfw.monitors = NULL;
    _glfw.monitorCount = 0;

    _glfwPlatformTerminate();

    _glfw.initialized = GLFW_FALSE;

    while (_glfw.errorListHead)
    {
        _GLFWerror* error = _glfw.errorListHead;
        _glfw.errorListHead = error->next;
        free(error);
    }

    _glfwPlatformDestroyTls(&_glfw.contextSlot);
    _glfwPlatformDestroyTls(&_glfw.errorSlot);
    _glfwPlatformDestroyMutex(&_glfw.errorLock);

    memset(&_glfw, 0, sizeof(_glfw));
}


//////////////////////////////////////////////////////////////////////////
//////                       GLFW internal API                      //////
//////////////////////////////////////////////////////////////////////////

// Encode a Unicode code point to a UTF-8 stream
// Based on cutef8 by Jeff Bezanson (Public Domain)
//
size_t _glfwEncodeUTF8(char* s, uint32_t codepoint)
{
    size_t count = 0;

    if (codepoint < 0x80)
        s[count++] = (char) codepoint;
    else if (codepoint < 0x800)
    {
        s[count++] = (codepoint >> 6) | 0xc0;
        s[count++] = (codepoint & 0x3f) | 0x80;
    }
    else if (codepoint < 0x10000)
    {
        s[count++] = (codepoint >> 12) | 0xe0;
        s[count++] = ((codepoint >> 6) & 0x3f) | 0x80;
        s[count++] = (codepoint & 0x3f) | 0x80;
    }
    else if (codepoint < 0x110000)
    {
        s[count++] = (codepoint >> 18) | 0xf0;
        s[count++] = ((codepoint >> 12) & 0x3f) | 0x80;
        s[count++] = ((codepoint >> 6) & 0x3f) | 0x80;
        s[count++] = (codepoint & 0x3f) | 0x80;
    }

    return count;
}

// Splits and translates a text/uri-list into separate file paths
// NOTE: This function destroys the provided string
//
char** _glfwParseUriList(char* text, int* count)
{
    const char* prefix = "file://";
    char** paths = NULL;
    char* line;

    *count = 0;

    while ((line = strtok(text, "\r\n")))
    {
        char* path;

        text = NULL;

        if (line[0] == '#')
            continue;

        if (strncmp(line, prefix, strlen(prefix)) == 0)
        {
            line += strlen(prefix);
            // TODO: Validate hostname
            while (*line != '/')
                line++;
        }

        (*count)++;

        path = (char*)calloc(strlen(line) + 1, 1);
        paths = (char**)realloc(paths, *count * sizeof(char*));
        paths[*count - 1] = path;

        while (*line)
        {
            if (line[0] == '%' && line[1] && line[2])
            {
                const char digits[3] = { line[1], line[2], '\0' };
                *path = (char) strtol(digits, NULL, 16);
                line += 2;
            }
            else
                *path = *line;

            path++;
            line++;
        }
    }

    return paths;
}

char* _glfw_strdup(const char* source)
{
    const size_t length = strlen(source);
    char* result = (char*)calloc(length + 1, 1);
    strcpy(result, source);
    return result;
}

int _glfw_min(int a, int b)
{
    return a < b ? a : b;
}

int _glfw_max(int a, int b)
{
    return a > b ? a : b;
}

float _glfw_fminf(float a, float b)
{
    if (a != a)
        return b;
    else if (b != b)
        return a;
    else if (a < b)
        return a;
    else
        return b;
}

float _glfw_fmaxf(float a, float b)
{
    if (a != a)
        return b;
    else if (b != b)
        return a;
    else if (a > b)
        return a;
    else
        return b;
}


//////////////////////////////////////////////////////////////////////////
//////                         GLFW event API                       //////
//////////////////////////////////////////////////////////////////////////

// Notifies shared code of an error
//
void _glfwInputError(int code, const char* format, ...)
{
    _GLFWerror* error;
    char description[_GLFW_MESSAGE_SIZE];

    if (format)
    {
        va_list vl;

        va_start(vl, format);
        vsnprintf(description, sizeof(description), format, vl);
        va_end(vl);

        description[sizeof(description) - 1] = '\0';
    }
    else
    {
        if (code == GLFW_NOT_INITIALIZED)
            strcpy(description, "The GLFW library is not initialized");
        else if (code == GLFW_NO_CURRENT_CONTEXT)
            strcpy(description, "There is no current context");
        else if (code == GLFW_INVALID_ENUM)
            strcpy(description, "Invalid argument for enum parameter");
        else if (code == GLFW_INVALID_VALUE)
            strcpy(description, "Invalid value for parameter");
        else if (code == GLFW_OUT_OF_MEMORY)
            strcpy(description, "Out of memory");
        else if (code == GLFW_API_UNAVAILABLE)
            strcpy(description, "The requested API is unavailable");
        else if (code == GLFW_VERSION_UNAVAILABLE)
            strcpy(description, "The requested API version is unavailable");
        else if (code == GLFW_PLATFORM_ERROR)
            strcpy(description, "A platform-specific error occurred");
        else if (code == GLFW_FORMAT_UNAVAILABLE)
            strcpy(description, "The requested format is unavailable");
        else if (code == GLFW_NO_WINDOW_CONTEXT)
            strcpy(description, "The specified window has no context");
        else
            strcpy(description, "ERROR: UNKNOWN GLFW ERROR");
    }

    if (_glfw.initialized)
    {
        error = (_GLFWerror*)_glfwPlatformGetTls(&_glfw.errorSlot);
        if (!error)
        {
            error = (_GLFWerror*)calloc(1, sizeof(_GLFWerror));
            _glfwPlatformSetTls(&_glfw.errorSlot, error);
            _glfwPlatformLockMutex(&_glfw.errorLock);
            error->next = _glfw.errorListHead;
            _glfw.errorListHead = error;
            _glfwPlatformUnlockMutex(&_glfw.errorLock);
        }
    }
    else
        error = &_glfwMainThreadError;

    error->code = code;
    strcpy(error->description, description);

    if (_glfwErrorCallback)
        _glfwErrorCallback(code, description);
}


//////////////////////////////////////////////////////////////////////////
//////                        GLFW public API                       //////
//////////////////////////////////////////////////////////////////////////

GLFWAPI int glfwInit(void)
{
    if (_glfw.initialized)
        return GLFW_TRUE;

    memset(&_glfw, 0, sizeof(_glfw));
    _glfw.hints.init = _glfwInitHints;

    if (!_glfwPlatformInit())
    {
        glfw_terminate();
        return GLFW_FALSE;
    }

    if (!_glfwPlatformCreateMutex(&_glfw.errorLock) ||
        !_glfwPlatformCreateTls(&_glfw.errorSlot) ||
        !_glfwPlatformCreateTls(&_glfw.contextSlot))
    {
        glfw_terminate();
        return GLFW_FALSE;
    }

    _glfwPlatformSetTls(&_glfw.errorSlot, &_glfwMainThreadError);

    _glfw.initialized = GLFW_TRUE;

    glfwDefaultWindowHints();
    return GLFW_TRUE;
}

GLFWAPI void glfwTerminate(void)
{
    if (!_glfw.initialized)
        return;

    glfw_terminate();
}

GLFWAPI void glfwInitHint(int hint, int value)
{
    switch (hint)
    {
        case GLFW_JOYSTICK_HAT_BUTTONS:
            _glfwInitHints.hatButtons = value;
            return;
        case GLFW_COCOA_CHDIR_RESOURCES:
            _glfwInitHints.ns.chdir = value;
            return;
        case GLFW_COCOA_MENUBAR:
            _glfwInitHints.ns.menubar = value;
            return;
    }

    _glfwInputError(GLFW_INVALID_ENUM,
                    "Invalid init hint 0x%08X", hint);
}

GLFWAPI void glfwGetVersion(int* major, int* minor, int* rev)
{
    if (major != NULL)
        *major = GLFW_VERSION_MAJOR;
    if (minor != NULL)
        *minor = GLFW_VERSION_MINOR;
    if (rev != NULL)
        *rev = GLFW_VERSION_REVISION;
}

GLFWAPI const char* glfwGetVersionString(void)
{
    return _glfwPlatformGetVersionString();
}

GLFWAPI int glfwGetError(const char** description)
{
    _GLFWerror* error;
    int code = GLFW_NO_ERROR;

    if (description)
        *description = NULL;

    if (_glfw.initialized)
        error = (_GLFWerror*)_glfwPlatformGetTls(&_glfw.errorSlot);
    else
        error = &_glfwMainThreadError;

    if (error)
    {
        code = error->code;
        error->code = GLFW_NO_ERROR;
        if (description && code)
            *description = error->description;
    }

    return code;
}



