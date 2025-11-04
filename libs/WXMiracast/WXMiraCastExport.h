#ifndef __WXMIRACASTPROCESS_H
#define __WXMIRACASTPROCESS_H

#ifdef  __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif //  __cplusplus

#ifdef WXMIRACAST_EXPORTS
#define   MIRACAST_API EXTERN_C  __declspec(dllexport)
#else
#define  MIRACAST_API EXTERN_C __declspec(dllimport) 
#endif

enum WINDOWSHOWSTATUS
{
	WSS_LANDSCAPE,//横屏
	WSS_PORTRAIT,//竖屏
};
struct WindowShowStruct
{
	WINDOWSHOWSTATUS eWindowStatus;
	int iScreenW;
	int iScreenH;
};

enum CONNECTSTATUS
{
	STARTING,//开始连接
	CONNECTING,//连接中
	CONNECTED,//连接上
	DISCONNECT,//连接断开
};
struct ConnectStatusStruct
{
	CONNECTSTATUS eConnectStatus;
	int iConnectType;//0 - play / 1 - mirror
	wchar_t *connectip;
	ConnectStatusStruct() : eConnectStatus(STARTING), iConnectType(1)
	{
		connectip = L"127.0.0.1";
	}
};
//内部
typedef int(*WXMIRACASTCALLBACKGETPARENTWINDOW)(const unsigned long long uniqueid);
typedef int(*WXMIRACASTCALLBACKREPORTCONNECTINFO) (const struct ConnectStatusStruct &stConnectStatus, const unsigned long long uniqueid);
typedef void(*WXMIRACASTCALLBACKREPORTWINDOWCHANGE) (const struct WindowShowStruct &stWindowShow, const unsigned long long uniqueid);
typedef void(*WXMIRACASTCALLBACKREPORTISSUPPORT) ();

//外部
typedef void(*WXMirrorCastDataR)(const char* data, int iSize);
typedef int(*WXMirrorCastConnectInfo) (const struct ConnectStatusStruct &tConnectStatus, const unsigned long long uniqueid);
typedef void(*WXMirrorCastIsSupport) ();

struct WXMiraCastManagerStructWinA
{
	//内部渲染
	WXMIRACASTCALLBACKGETPARENTWINDOW M_WXMiraCastCallBackGetParentWindow;
	WXMIRACASTCALLBACKREPORTCONNECTINFO M_WXMiraCastCallBackConnectInfo;
	WXMIRACASTCALLBACKREPORTWINDOWCHANGE M_WXMiraCastCallBackWindowStatus;
	WXMIRACASTCALLBACKREPORTISSUPPORT M_WXMiraCastCallBackIsSupport;
};

struct WXMiraCastManagerStructWinB
{
	//外部渲染
	WXMirrorCastDataR m_WXMirrorCastDataR;
	WXMirrorCastConnectInfo m_WXMirrorCastConnectInfo;
	WXMirrorCastIsSupport m_WXMirrorCastIsSupport;
};

struct WXMiraCastManagerStruct:public WXMiraCastManagerStructWinA,public WXMiraCastManagerStructWinB
{
	wchar_t *m_arcAppName;
	wchar_t *m_arcLogName;
	int m_flag;

};
MIRACAST_API int WXInitMiraCast(const struct WXMiraCastManagerStruct *tCallBackFunc);
MIRACAST_API void WXUninitMiracast();
MIRACAST_API int WXStartMiracast();
MIRACAST_API int WXStopMiraCast();
MIRACAST_API void WXDisconnectMiraCastMirror(const unsigned long long uniqueid);
MIRACAST_API void WXMiraCastSetLogLevel(int iLevel); //设置日志记录级别接口
MIRACAST_API void WXSetMiraCastPassword(const char* pszPwd);//设置密码
MIRACAST_API void WXMirCastSetMaxBufferLength(unsigned long long llUniqueid, int iBufLen);

MIRACAST_API void SetDecodeMode(int mode);//参数 1硬解码，0软解码

#endif // !__WXMIRACASTPROCESS_H
