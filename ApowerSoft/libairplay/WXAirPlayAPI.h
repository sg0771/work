#ifndef __EXPORT_H
#define __EXPORT_H

#include <Windows.h>
#include <stdint.h>

enum RETCODE_AIRPLAY
{
	ErrorCode_Unknow = -1,
	ErrorCode_NoError = 0,
	ErrorCode_AppleServiceNotRunning,
	ErrorCode_StartAirtunesServerFail,
	ErrorCode_StartAirplayServerFail,
	ErrorCode_StartMirrorServerFail,
};

enum WINDOWSHOWSTATUS
{
	LANDSCAPE,//横屏
	PORTRAIT,//竖屏
};

enum CONNECTSTATUS
{
	STARTING,//开始连接
	CONNECTING,//连接中
	CONNECTED,//连接上
	DISCONNECT,//连接断开
};

struct WindowShowStruct
{
	WINDOWSHOWSTATUS eWindowStatus;
	int iScreenW;
	int iScreenH;
	int iIPad;//1 - ipad / 0 - iphone
	int iPro;
	int iOsVersion;//13.5以上 1，13.5以下0
	wchar_t m_wszType[20];//
};

struct ConnectStatusStruct
{
	CONNECTSTATUS eConnectStatus;
	int iConnectType;//0 - play / 1 - mirror
	wchar_t wszDevName[256] = { 0 };
	ConnectStatusStruct() : eConnectStatus(STARTING), iConnectType(1)
	{
	}
};

enum WindowPixel
{
	TYPE_720P,
	TYPE_1080P,
	TYPE_IPHONE6,//iphone 6 / 6s / 7 720 * 1280
	TYPE_IPHONE6PLUS,//iPhone 6 / 6s / 7 plus 1080 * 1920
	TYPE_2K
};

typedef void(*CALLBACKREPORTWINDOWCHANGE) (const WindowShowStruct& stWindowShow, const uint64_t uniqueid);
typedef void(*CALLBACKREPORTBMPDATA) (const unsigned char* bmpdata, int iLen);
typedef int(*CALLBACKREPORTCONNECTINFO) (const ConnectStatusStruct& stConnectStatus, const uint64_t uniqueid);
typedef WindowPixel(*CALLBACKWINDOWPIXEL) (void);
typedef int(*CALLBACKREPORTFRAMEDATA) (const unsigned char* framedata, int width, int height, const uint64_t uniqueid);
typedef int(*CALLBACKNEEDCONVERTVIDEOData)();
typedef uint64_t (*CALLBACKGETPARENTWINDOW)(const uint64_t uniqueid);
//get dev count
typedef int(*CALLBACKGETMAXDEVNUM)();
//check if or not windows exists
typedef int(*CALLBACKCHECKWINDOWEXIST)(const uint64_t uniqueid);

#define MAX_APP_NAME 64
#define MAX_LOG_NAME 256
#define MAX_PC_VERSION 64

//根据低版本iOS12.5.5测试，发现只能支持最长31位的UID
#define MAX_UID 31

typedef struct tagAirplayManagerStruct
{
	wchar_t m_arcAppName[MAX_APP_NAME];
	wchar_t m_arcLogName[MAX_LOG_NAME];
	wchar_t m_arcPcVersion[MAX_PC_VERSION];
	int m_iPort = 46666;
	int m_iDataPort = 46001;
	int m_iMirrorPort = 46000;
	wchar_t m_uid[MAX_UID];
	CALLBACKREPORTWINDOWCHANGE m_CallBackWindowStatus;
	CALLBACKREPORTBMPDATA m_CallBackBmpData;
	CALLBACKREPORTCONNECTINFO m_CallBackConnectInfo;
	CALLBACKWINDOWPIXEL m_CallBackWindowPixel;
	CALLBACKREPORTFRAMEDATA m_CallBackFrameData;
	CALLBACKNEEDCONVERTVIDEOData m_CallBackNeedConvertVideoData;
	CALLBACKGETPARENTWINDOW m_CallBackGetParentWindow;
	CALLBACKGETMAXDEVNUM m_CallBackGetMaxDevNum;
	CALLBACKCHECKWINDOWEXIST m_CallBackCheckWindowExist;
}AirplayManagerStruct;

struct AVFrame;

#if defined (_WIN32) && defined(LIBAIRPLAY_EXPORTS)
# define AIRPLAY_API  EXTERN_C __declspec(dllexport)
#else
# define AIRPLAY_API  EXTERN_C __declspec(dllimport)
#endif
//
AIRPLAY_API int  StartAirplay(const AirplayManagerStruct &stAirplay);
//
AIRPLAY_API void StopAirplay();
//
AIRPLAY_API void DisconnectAirplay(uint64_t uniqueid);
//
AIRPLAY_API void DisconnectAirplayMirror(uint64_t uniqueid);
//
AIRPLAY_API void SetWindowsParentHandle(HWND iHandle);
//
AIRPLAY_API void LockSDL();
//
AIRPLAY_API void UnLockSDL();
//1 - start / 0 - stop
AIRPLAY_API void SetRecording(int iStatus, uint64_t uniqueid);
//
AIRPLAY_API bool GetCurrentWindowsPicture(const char *arcPath);

AIRPLAY_API int  GetRecordWidth(uint64_t uniqueid); //获取宽度
AIRPLAY_API int  GetRecordHeight(uint64_t uniqueid);//获取高度

//设置显示模式
AIRPLAY_API void  SetDisplayMode(int mode);

//设置旋转方式 1:顺时针90  2:顺时针180 3:顺时针270 0:顺时针0或者360
AIRPLAY_API void  SetDisplayRotate(int rotate);

AIRPLAY_API void AirplayMDNSInit();//如果没有bonjour服务，可以使用内置的MDNS服务

//弃用
//默认为true，false时底层强制使用I420模式显示J420解码数据
AIRPLAY_API void WXAirplaySetShowJ420(int bJ420);

//设置是否使用DXVA2硬解码
//1 使用 0不适用，断开后重连有效
AIRPLAY_API void SetH264DecodeHwMode(int mode);


AIRPLAY_API void WXAirplaySetVideoRenderFixed(int fixed);


//直播优化，设置直播窗口的大小
//默认为0
//设置为0
AIRPLAY_API void WXAirplaySetBroadcastSize(int width, int height);//


//针对单路视频的旋转、翻转
AIRPLAY_API void WXAirplaySetDisplayRotateFlip(uint64_t uniqueid, int type);

//针对单路视频的截图
AIRPLAY_API void WXAirplayShotPicture(uint64_t uniqueid, const wchar_t* strName);

#endif
