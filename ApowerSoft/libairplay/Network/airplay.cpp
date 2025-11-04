
//mirror
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "../plist/plist.h"

extern "C" {
#include "airplay.h"

	typedef struct rsakey_s rsakey_t;

	rsakey_t* rsakey_init(const unsigned char* modulus, int mod_len,
		const unsigned char* pub_exp, int pub_len,
		const unsigned char* priv_exp, int priv_len,
		const unsigned char* p, int p_len,
		const unsigned char* q, int q_len,
		const unsigned char* dP, int dP_len,
		const unsigned char* dQ, int dQ_len,
		const unsigned char* qInv, int qInv_len);
	rsakey_t* rsakey_init_pem(const char* pemstr);

	int rsakey_sign(rsakey_t* rsakey, char* dst, int dstlen, const char* b64digest,
		unsigned char* ipaddr, int ipaddrlen,
		unsigned char* hwaddr, int hwaddrlen);

	int rsakey_base64_decode(rsakey_t* rsakey, unsigned char** output, const char* b64input);
	int rsakey_decrypt(rsakey_t* rsakey, unsigned char* dst, int dstlen, const char* b64input);
	int rsakey_parseiv(rsakey_t* rsakey, unsigned char* dst, int dstlen, const char* b64input);

	void rsakey_destroy(rsakey_t* rsakey);

	void digest_generate_nonce(char* result, int resultlen);
	int digest_is_valid(const char* our_realm, const char* password,
		const char* our_nonce, const char* method,
		const char* our_uri, const char* authorization);

#include "httpd.h"

}
#include "openssl/aes.h"
#include "openssl/modes.h"
#include "AirPlayServer.h"
#include "NetworkServices.h"
#include "mycommon.h"
#include "../WXAirPlayAPI.h"

#include <WXBase.h>
#include <WXMedia.h>
#include <FfmpegIncludes.h>

#include <ws2tcpip.h>
#include <windows.h>
#ifndef snprintf
#define snprintf _snprintf
#endif

#define SYSTEM_GET_PAGESIZE(ret) do {\
	SYSTEM_INFO si;\
	GetSystemInfo(&si);\
	ret = si.dwPageSize;\
} while(0)
#define SYSTEM_GET_TIME(ret) ret = timeGetTime()

#define ALIGNED_MALLOC(memptr, alignment, size) do {\
	char *ptr = malloc(sizeof(void*) + (size) + (alignment)-1);\
	memptr = NULL;\
	if (ptr) {\
		size_t ptrval = (size_t)ptr + sizeof(void*) + (alignment)-1;\
		ptrval = ptrval / (alignment) * (alignment);\
		memptr = (void *)ptrval;\
		*(((void **)memptr)-1) = ptr;\
	}\
} while(0)
#define ALIGNED_FREE(memptr) free(*(((void **)memptr)-1))





#define SOCKET_GET_ERROR()      WSAGetLastError()
#define SOCKET_SET_ERROR(value) WSASetLastError(value)
#define SOCKET_ERRORNAME(name)  WSA##name

#define WSAEAGAIN WSAEWOULDBLOCK
#define WSAENOMEM WSA_NOT_ENOUGH_MEMORY

#include <windows.h>




#define GLOBAL_FEATURES 0x7
#define GLOBAL_FEATURES_AIRPLAY 0x29ff

#define MAX_HWADDR_LEN 6

//系统库
#pragma comment(lib, "winmm.lib")
#pragma comment(lib,"ws2_32.lib")    // Socket静态库,取本机IP用
#pragma comment(lib,"IPHlpApi.lib")    // 发送ARP报文要用的静态库,取MAC用

//第三方
#pragma comment(lib, "libcurl.lib")
#pragma comment(lib, "Crypt32.lib")

#pragma comment(lib, "libeay32.lib")
#pragma comment(lib, "libssl.lib")

#pragma comment(lib, "WXMedia.lib")
#pragma  comment(lib,"dnssd.lib")

#pragma  comment(lib,"winmm.lib")
#pragma  comment(lib,"ws2_32.lib")


#pragma comment(lib, "libffmpeg.lib")
#pragma comment(lib, "Wldap32.lib")
#pragma comment(lib, "ComCtl32.lib") 

#include <ffmpeg-config.h>



EXTERN_C int netutils_init();
EXTERN_C void netutils_cleanup();


#include "WXMedia.h"


/* Actually 345 bytes for 2048-bit key */
#define MAX_SIGNATURE_LEN 512

/* Let's just decide on some length */
#define MAX_PASSWORD_LEN 64

/* MD5 as hex fits here */
#define MAX_NONCE_LEN 32

#define MAX_PACKET_LEN 4096

#define RECEIVEBUFFER 1024

#define AIRPLAY_STATUS_OK                  200
#define AIRPLAY_STATUS_SWITCHING_PROTOCOLS 101
#define AIRPLAY_STATUS_NEED_AUTH           401
#define AIRPLAY_STATUS_NOT_FOUND           404
#define AIRPLAY_STATUS_METHOD_NOT_ALLOWED  405
#define AIRPLAY_STATUS_PRECONDITION_FAILED 412
#define AIRPLAY_STATUS_NOT_IMPLEMENTED     501
#define AIRPLAY_STATUS_NO_RESPONSE_NEEDED  1000

#define EVENT_NONE     -1
#define EVENT_PLAYING   0
#define EVENT_PAUSED    1
#define EVENT_LOADING   2
#define EVENT_STOPPED   3

#define RSA_KEY " \
-----BEGIN RSA PRIVATE KEY-----\
MIIEpQIBAAKCAQEA59dE8qLieItsH1WgjrcFRKj6eUWqi+bGLOX1HL3U3GhC/j0Qg90u3sG/1CUt\
wC5vOYvfDmFI6oSFXi5ELabWJmT2dKHzBJKa3k9ok+8t9ucRqMd6DZHJ2YCCLlDRKSKv6kDqnw4U\
wPdpOMXziC/AMj3Z/lUVX1G7WSHCAWKf1zNS1eLvqr+boEjXuBOitnZ/bDzPHrTOZz0Dew0uowxf\
/+sG+NCK3eQJVxqcaJ/vEHKIVd2M+5qL71yJQ+87X6oV3eaYvt3zWZYD6z5vYTcrtij2VZ9Zmni/\
UAaHqn9JdsBWLUEpVviYnhimNVvYFZeCXg/IdTQ+x4IRdiXNv5hEewIDAQABAoIBAQDl8Axy9XfW\
BLmkzkEiqoSwF0PsmVrPzH9KsnwLGH+QZlvjWd8SWYGN7u1507HvhF5N3drJoVU3O14nDY4TFQAa\
LlJ9VM35AApXaLyY1ERrN7u9ALKd2LUwYhM7Km539O4yUFYikE2nIPscEsA5ltpxOgUGCY7b7ez5\
NtD6nL1ZKauw7aNXmVAvmJTcuPxWmoktF3gDJKK2wxZuNGcJE0uFQEG4Z3BrWP7yoNuSK3dii2jm\
lpPHr0O/KnPQtzI3eguhe0TwUem/eYSdyzMyVx/YpwkzwtYL3sR5k0o9rKQLtvLzfAqdBxBurciz\
aaA/L0HIgAmOit1GJA2saMxTVPNhAoGBAPfgv1oeZxgxmotiCcMXFEQEWflzhWYTsXrhUIuz5jFu\
a39GLS99ZEErhLdrwj8rDDViRVJ5skOp9zFvlYAHs0xh92ji1E7V/ysnKBfsMrPkk5KSKPrnjndM\
oPdevWnVkgJ5jxFuNgxkOLMuG9i53B4yMvDTCRiIPMQ++N2iLDaRAoGBAO9v//mU8eVkQaoANf0Z\
oMjW8CN4xwWA2cSEIHkd9AfFkftuv8oyLDCG3ZAf0vrhrrtkrfa7ef+AUb69DNggq4mHQAYBp7L+\
k5DKzJrKuO0r+R0YbY9pZD1+/g9dVt91d6LQNepUE/yY2PP5CNoFmjedpLHMOPFdVgqDzDFxU8hL\
AoGBANDrr7xAJbqBjHVwIzQ4To9pb4BNeqDndk5Qe7fT3+/H1njGaC0/rXE0Qb7q5ySgnsCb3DvA\
cJyRM9SJ7OKlGt0FMSdJD5KG0XPIpAVNwgpXXH5MDJg09KHeh0kXo+QA6viFBi21y340NonnEfdf\
54PX4ZGS/Xac1UK+pLkBB+zRAoGAf0AY3H3qKS2lMEI4bzEFoHeK3G895pDaK3TFBVmD7fV0Zhov\
17fegFPMwOII8MisYm9ZfT2Z0s5Ro3s5rkt+nvLAdfC/PYPKzTLalpGSwomSNYJcB9HNMlmhkGzc\
1JnLYT4iyUyx6pcZBmCd8bD0iwY/FzcgNDaUmbX9+XDvRA0CgYEAkE7pIPlE71qvfJQgoA9em0gI\
LAuE4Pu13aKiJnfft7hIjbK+5kyb3TysZvoyDnb3HOKvInK7vXbKuU4ISgxB2bB3HcYzQMGsz1qJ\
2gG0N5hvJpzwwhbhXqFKA4zaaSrw622wDniAK5MlIE0tIAKKP4yxNGjoD2QYjhBGuhvkWKY=\
-----END RSA PRIVATE KEY-----"

#define STREAM_INFO  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"\
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\r\n"\
"<plist version=\"1.0\">\r\n"\
"<dict>\r\n"\
"<key>width</key>\r\n"\
"<integer>1280</integer>\r\n"\
"<key>height</key>\r\n"\
"<integer>720</integer>\r\n"\
"<key>version</key>\r\n"\
"<string>110.92</string>\r\n"\
"<key>refreshRate</key>\r\n"\
"<integer>60</integer>\r\n"\
"<key>maxFPS</key>\r\n"\
"<integer>60</integer>\r\n"\
"</dict>\r\n"\
"</plist>\r\n"

#define PLAYBACK_INFO  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"\
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\r\n"\
"<plist version=\"1.0\">\r\n"\
"<dict>\r\n"\
"<key>duration</key>\r\n"\
"<real>%f</real>\r\n"\
"<key>loadedTimeRanges</key>\r\n"\
"<array>\r\n"\
"\t\t<dict>\r\n"\
"\t\t\t<key>duration</key>\r\n"\
"\t\t\t<real>%f</real>\r\n"\
"\t\t\t<key>start</key>\r\n"\
"\t\t\t<real>0.0</real>\r\n"\
"\t\t</dict>\r\n"\
"</array>\r\n"\
"<key>playbackBufferEmpty</key>\r\n"\
"<true/>\r\n"\
"<key>playbackBufferFull</key>\r\n"\
"<false/>\r\n"\
"<key>playbackLikelyToKeepUp</key>\r\n"\
"<true/>\r\n"\
"<key>position</key>\r\n"\
"<real>%f</real>\r\n"\
"<key>rate</key>\r\n"\
"<real>%d</real>\r\n"\
"<key>readyToPlay</key>\r\n"\
"<true/>\r\n"\
"<key>seekableTimeRanges</key>\r\n"\
"<array>\r\n"\
"\t\t<dict>\r\n"\
"\t\t\t<key>duration</key>\r\n"\
"\t\t\t<real>%f</real>\r\n"\
"\t\t\t<key>start</key>\r\n"\
"\t\t\t<real>0.0</real>\r\n"\
"\t\t</dict>\r\n"\
"</array>\r\n"\
"</dict>\r\n"\
"</plist>\r\n"

#define PLAYBACK_INFO_NOT_READY  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"\
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\r\n"\
"<plist version=\"1.0\">\r\n"\
"<dict>\r\n"\
"<key>readyToPlay</key>\r\n"\
"<false/>\r\n"\
"</dict>\r\n"\
"</plist>\r\n"

#define SERVER_INFO  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"\
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\r\n"\
"<plist version=\"1.0\">\r\n"\
"<dict>\r\n"\
"<key>deviceid</key>\r\n"\
"<string>%s</string>\r\n"\
"<key>features</key>\r\n"\
"<integer>119</integer>\r\n"\
"<key>model</key>\r\n"\
"<string>Kodi,1</string>\r\n"\
"<key>protovers</key>\r\n"\
"<string>1.0</string>\r\n"\
"<key>srcvers</key>\r\n"\
"<string>"AIRPLAY_SERVER_VERSION_STR"</string>\r\n"\
"</dict>\r\n"\
"</plist>\r\n"

#define EVENT_INFO "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\r\n"\
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n\r\n"\
"<plist version=\"1.0\">\r\n"\
"<dict>\r\n"\
"<key>category</key>\r\n"\
"<string>video</string>\r\n"\
"<key>sessionID</key>\r\n"\
"<integer>%d</integer>\r\n"\
"<key>state</key>\r\n"\
"<string>%s</string>\r\n"\
"</dict>\r\n"\
"</plist>\r\n"\

#define AUTH_REALM "AirPlay"
#define AUTH_REQUIRED "WWW-Authenticate: Digest realm=\""  AUTH_REALM  "\", nonce=\"%s\"\r\n"
#define SERVER_PORT 20992

static int s_bShowJ420 = 0;
static AVFrame* g_pFrameDecode;
/*extern */int g_bFixed = FALSE;
/*extern */int g_nModeVideoRender = RENDER_TYPE_D3DX;

//全面屏手机
//mode-name
static std::map<std::string, std::string>s_mapBezelLessDevice;

//非全面屏手机
static std::map<std::string, std::string>s_mapNonBezelLessDevice;
static void InitDeviceList() {

	//假定后续苹果手机都是全面屏，只需要过滤非全面屏
	s_mapNonBezelLessDevice.clear();
	s_mapNonBezelLessDevice["iPhone1,1"] = "iPhone";
	s_mapNonBezelLessDevice["iPhone1,2"] = "iPhone 3G";
	s_mapNonBezelLessDevice["iPhone2,1"] = "iPhone 3GS";
	s_mapNonBezelLessDevice["iPhone3,1"] = "iPhone 4";
	s_mapNonBezelLessDevice["iPhone3,2"] = "iPhone 4";
	s_mapNonBezelLessDevice["iPhone3,3"] = "iPhone 4";
	s_mapNonBezelLessDevice["iPhone4,1"] = "iPhone 4S";
	s_mapNonBezelLessDevice["iPhone5,1"] = "iPhone 5";
	s_mapNonBezelLessDevice["iPhone5,2"] = "iPhone 5";
	s_mapNonBezelLessDevice["iPhone5,3"] = "iPhone 5c";
	s_mapNonBezelLessDevice["iPhone5,4"] = "iPhone 5c";
	s_mapNonBezelLessDevice["iPhone6,1"] = "iPhone 5s";
	s_mapNonBezelLessDevice["iPhone6,2"] = "iPhone 5s";
	s_mapNonBezelLessDevice["iPhone7,2"] = "iPhone 6";
	s_mapNonBezelLessDevice["iPhone7,1"] = "iPhone 6 Plus";
	s_mapNonBezelLessDevice["iPhone8,1"] = "iPhone 6s";
	s_mapNonBezelLessDevice["iPhone8,2"] = "iPhone 6s Plus";
	s_mapNonBezelLessDevice["iPhone8,4"] = "iPhone SE";
	s_mapNonBezelLessDevice["iPhone9,1"] = "iPhone 7";
	s_mapNonBezelLessDevice["iPhone9,3"] = "iPhone 7";
	s_mapNonBezelLessDevice["iPhone9,2"] = "iPhone 7 Plus";
	s_mapNonBezelLessDevice["iPhone9,4"] = "iPhone 7 Plus";
	s_mapNonBezelLessDevice["iPhone10,1"] = "iPhone 8";
	s_mapNonBezelLessDevice["iPhone10,4"] = "iPhone 8";
	s_mapNonBezelLessDevice["iPhone10,2"] = "iPhone 8 Plus";
	s_mapNonBezelLessDevice["iPhone10,5"] = "iPhone 8 Plus";
	s_mapNonBezelLessDevice["iPhone12,8"] = "iPhone SE2";

#if 0
	s_mapBezelLessDevice.clear();
	//硬编码全面屏手机名单
	s_mapBezelLessDevice["iPhone10,3"] = "iPhone X";
	s_mapBezelLessDevice["iPhone10,6"] = "iPhone X";
	s_mapBezelLessDevice["iPhone11,2"] = "iPhone XS";
	s_mapBezelLessDevice["iPhone11,4"] = "iPhone XS Max";
	s_mapBezelLessDevice["iPhone11,6"] = "iPhone XS Max";
	s_mapBezelLessDevice["iPhone11,8"] = "iPhone XR";
	s_mapBezelLessDevice["iPhone12,1"] = "iPhone 11";
	s_mapBezelLessDevice["iPhone12,3"] = "iPhone 11 Pro";
	s_mapBezelLessDevice["iPhone12,5"] = "iPhone 11 Pro Max";
	s_mapBezelLessDevice["iPhone13,1"] = "iPhone 12 Mini";
	s_mapBezelLessDevice["iPhone13,2"] = "iPhone 12";
	s_mapBezelLessDevice["iPhone13,3"] = "iPhone 12 Pro";
	s_mapBezelLessDevice["iPhone13,4"] = "iPhone 12 Pro Max";
	s_mapBezelLessDevice["iPhone14,2"] = "iPhone 13 Pro";
	s_mapBezelLessDevice["iPhone14,3"] = "iPhone 13 Pro Max";
	s_mapBezelLessDevice["iPhone14,4"] = "iPhone 13 Mini";
	s_mapBezelLessDevice["iPhone14,5"] = "iPhone 13";
#endif

}

//0 表示普通手机, 1 表示iPad, 2 表示全面屏手机
int  AirplayGetIpadType(const std::string strName) {
	if (strName.find("iPhone") != std::string::npos) { //iPhone
		if (s_mapNonBezelLessDevice.count(strName) != 0) {
			return 0;
		}
		return 2;
	}
	return 1;//iPad
}

int  AirplayGetOsVersionType(const std::string strOsVersion) {
	if (strncmp(strOsVersion.c_str(), "13.5", 4) >= 0 ){
		return 1;//大于13.5版本
	}
	else {
		return 0;//小于13.5版本
	}
}


struct airplay_s {
	/* Callbacks for audio */
	airplay_callbacks_t callbacks;

	/* HTTP daemon and RSA key */
	httpd_t *httpd;
	rsakey_t *rsakey;

	httpd_t *mirror_server;

	/* Hardware address information */
	uint8_t hwaddr[MAX_HWADDR_LEN];
	int hwaddrlen;

	/* Password information */
	char password[MAX_PASSWORD_LEN + 1];
};

typedef struct airplay_conn_s {
	airplay_t *airplay;
	//raop_rtp_t *airplay_rtp;
	uint8_t *local;
	int locallen;
	uint8_t *remote;
	int remotelen;
	char nonce[MAX_NONCE_LEN + 1];
	/* for mirror stream */
	uint8_t aeskey[16];
	uint8_t iv[16];
	uint8_t buffer[MAX_PACKET_LEN];
	int pos;
}airplay_conn_t;

static int s_nBroadcastWidth = 0;
static int s_nBroadcastHeight = 0;
AIRPLAY_API void WXAirplaySetBroadcastSize(int width, int height) {
	s_nBroadcastWidth = width;
	s_nBroadcastHeight = height;
}

static WXLocker s_lockHttp;//HTTP连接锁
static WXLocker s_lockMirrorStatus;
static uint8_t *s_pBuf31 = NULL;
static uint8_t *s_pBuf32 = NULL;

///*extern */int g_nVideoRotate = RENDER_ROTATE_NONE;
static std::map<uint64_t, int>s_MapVideoRotateFilp;//视频渲染对象

static std::map<uint64_t, int> s_MirrorStopMap;
static std::map<uint64_t, int> s_HttpStreamMap;

//视频流的分辨率
static std::map<uint64_t, int>s_MapWidth;
static std::map<uint64_t, int>s_MapHeight;

//操作完全加锁
static std::map<uint64_t, WXLocker> s_mapLockVideoRender;
static std::map<uint64_t, void *>g_MapVideoRender;//视频渲染对象

												  //--------------------------------------------------------------
												  //截图操作
static WXLocker s_lockPicture;
static std::map<uint64_t, int>s_mapShotPicture;
static std::map<uint64_t, std::wstring>s_mapStrPicture;

void WXAirplayVideoRenderDisplay(uint64_t uniqueid, AVFrame *frame) {
	//检测是否改变分辨率
	if (frame) {
		//截图操作
		{
			WXAutoLock al(s_lockPicture);
			if (s_mapShotPicture[uniqueid]) {
				WXMediaUtilsSaveAsPicture(frame, s_mapStrPicture[uniqueid].c_str(), 100);
				s_mapShotPicture[uniqueid] = 0;
			}
		}

		//推送录屏数据
		if (CNetworkServices::Get().GetRecordStatus(uniqueid) != 0) {
			WXAirplayPush(frame);
		}

		//分辨率改变，回调UI
		if (s_MapWidth.count(uniqueid) == 0 ||
			frame->width != s_MapWidth[uniqueid] ||
			frame->height != s_MapHeight[uniqueid]) {
			if (CNetworkServices::Get().m_stAirplay.m_CallBackWindowStatus != NULL) {
				WindowShowStruct stWindowShow;
				stWindowShow.eWindowStatus = frame->width > frame->height ? LANDSCAPE : PORTRAIT;
				stWindowShow.iScreenW = frame->width;
				stWindowShow.iScreenH = frame->height;
				std::string strDevice = CNetworkServices::Get().m_mapModel[uniqueid];
				std::string strOsVersion = CNetworkServices::Get().m_mapOsVersion[uniqueid];
				WXString wxstrDevice;
				wxstrDevice.Format("%s", strDevice.c_str());
				wcscpy(stWindowShow.m_wszType, wxstrDevice.str());
				stWindowShow.iIPad = AirplayGetIpadType(strDevice);
				stWindowShow.iOsVersion = AirplayGetOsVersionType(strOsVersion);//系统类型
				CNetworkServices::Get().m_stAirplay.m_CallBackWindowStatus(stWindowShow, uniqueid);
			}
			s_MapWidth[uniqueid] = frame->width;
			s_MapHeight[uniqueid] = frame->height;
		}
	}

	//创建视频显示对象
	WXAutoLock al(s_mapLockVideoRender[uniqueid]);
	if (g_MapVideoRender.count(uniqueid) == 0) { //对象不存在
		HWND hwnd = (HWND)CNetworkServices::Get().m_stAirplay.m_CallBackGetParentWindow(uniqueid);
		if (s_nBroadcastWidth && s_nBroadcastHeight) {
			WXLogWriteNew("%s WXVideoRenderCreateEx %lld [%dx%d]", __FUNCTION__, uniqueid, s_nBroadcastWidth, s_nBroadcastHeight);
			g_MapVideoRender[uniqueid] = WXVideoRenderCreateEx(hwnd, s_nBroadcastWidth, s_nBroadcastHeight);
		}
		else if (frame) {
			WXLogWriteNew("%s WXVideoRenderCreate %lld [%dx%d]", __FUNCTION__, uniqueid, frame->width, frame->height);
			g_MapVideoRender[uniqueid] = WXVideoRenderCreate(hwnd, frame->width, frame->height);
		}
	}
	if (g_MapVideoRender.count(uniqueid) != 0) {
		// 显示图像数据
		WXVideoRenderChangeMode(g_MapVideoRender[uniqueid], g_nModeVideoRender);
		WXVideoRenderDisplay(g_MapVideoRender[uniqueid], frame, g_bFixed, s_MapVideoRotateFilp[uniqueid]);
	}
}

void WXAirplayVideoRenderDestroy(uint64_t uniqueid) {
	WXAutoLock al(s_mapLockVideoRender[uniqueid]);
	if (g_MapVideoRender.count(uniqueid)) { //对象存在
		WXLogWriteNew("WXAirplayVideoRenderDestroy %lld AAAAAAAAAA ", uniqueid);
		WXVideoRenderDestroy(g_MapVideoRender[uniqueid]);
		g_MapVideoRender[uniqueid] = NULL;
		g_MapVideoRender.erase(uniqueid);
		s_MapWidth[uniqueid] = 0;
		s_MapWidth.erase(uniqueid);
		s_MapHeight[uniqueid] = 0;
		s_MapHeight.erase(uniqueid);
		WXLogWriteNew("WXVideoRenderDestroy %lld BBBBBBBBBB", uniqueid);
	}
}


class HandleMirroringStream;
static std::map<uint64_t, HandleMirroringStream*>s_MapMirror;//对象

static void set_mirror_status(uint64_t uniqueid, int iStatus)
{
	WXAutoLock lock(s_lockMirrorStatus);

	//g_MirrorStop = iStatus;
	if (uniqueid != 0)
	{
		s_MirrorStopMap[uniqueid] = iStatus;
	}
	else
	{
		for (std::map<uint64_t, int>::iterator itrBegin = s_MirrorStopMap.begin(); itrBegin != s_MirrorStopMap.end(); ++itrBegin)
		{
			itrBegin->second = iStatus;
		}
	}
}

static int get_mirror_status(uint64_t uniqueid)
{
	return s_MirrorStopMap[uniqueid];
}

static int fairplay_sock_fd = 0;

static int get_fairplay_socket() {
	struct sockaddr_in ser_addr;

	if (fairplay_sock_fd > 0) 
		return fairplay_sock_fd;

	memset(&ser_addr, 0, sizeof(ser_addr));
	ser_addr.sin_family = AF_INET;

	//ser_addr.sin_addr.s_addr = inet_addr("5.10.77.2");//

	char keyserver[] = "airplaykeycenter.apowersoft.com";
	char keyserver2[] = "airplaykeycenter2.apowersoft.com";
	//
	struct hostent* hostname;
	hostname = gethostbyname(keyserver);
	if (hostname == NULL)
	{
		WXLogWriteNew("gethostbyname fail");
		return 0;
	}
	ser_addr.sin_addr.s_addr = *(unsigned long *)hostname->h_addr;
	ser_addr.sin_port = htons(SERVER_PORT);

	fairplay_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fairplay_sock_fd <= 0)
	{
		fprintf(stderr, "%s:%d, create socket failed", __FILE__, __LINE__);
		return 0;
	}

	if (connect(fairplay_sock_fd, (struct sockaddr *)&ser_addr, sizeof(ser_addr)) < 0)
	{
		fprintf(stderr, "%s:%d, create socket failed", __FILE__, __LINE__);
		//
		struct hostent* hostname;
		hostname = gethostbyname(keyserver2);
		if (hostname == NULL)
		{
			WXLogWriteNew("gethostbyname fail");
			return 0;
		}
		ser_addr.sin_addr.s_addr = *(unsigned long *)hostname->h_addr;
		if (connect(fairplay_sock_fd, (struct sockaddr *)&ser_addr, sizeof(ser_addr)) < 0)
		{
			fprintf(stderr, "%s:%d, create socket2 failed", __FILE__, __LINE__);
			fairplay_sock_fd = 0;
		}
	}

	return fairplay_sock_fd;
}

static void close_fairplay_socket()
{
	if (fairplay_sock_fd > 0) closesocket(fairplay_sock_fd);/*close(fairplay_sock_fd);*/
	fairplay_sock_fd = 0;
}



//class
class HandleMirroringStream
{
	WXLocker m_lockMirrorData;
	std::queue<WXDataBuffer*> m_queueMirrorData;
	WXVideoFrame m_I420Frame;
	void *m_pDecoder = NULL;
public:

	void MirrorDataPush(uint8_t *data, int len, INT64 pts) {
		WXDataBuffer*obj = new WXDataBuffer;
		obj->Init(data, len, pts);
		WXAutoLock al(m_lockMirrorData);		
		m_queueMirrorData.push(obj);
	}
	WXDataBuffer*  MirrorDataPop() {
		WXAutoLock al(m_lockMirrorData);

		if (m_queueMirrorData.empty())
		{
			return nullptr;
		}
		WXDataBuffer* obj = m_queueMirrorData.front();
		m_queueMirrorData.pop();
		return obj;
	}
	int MirrorDataSize() {
		WXAutoLock al(m_lockMirrorData);
		return m_queueMirrorData.size();
	}
	void MirrorDataClean() {
		WXAutoLock al(m_lockMirrorData);
		while (!m_queueMirrorData.empty()) {
			WXDataBuffer* obj = m_queueMirrorData.front();
			m_queueMirrorData.pop();
			delete obj;
		}
	}
public:

	HandleMirroringStream(const char *strParam1, const char *strParam2, const char* remoteip)
	{
		m_strParam1.assign(strParam1, 16);
		m_strParam2.assign(strParam2, 16);
		memset(&m_key, 0, sizeof(AES_KEY));
		AES_set_encrypt_key((const uint8_t*)m_strParam1.c_str(), 128, &m_key);
		m_strRemoteIp.assign(remoteip, 4);
	}

	virtual ~HandleMirroringStream()
	{
	}
public:

	void DrawBlack() {
		if (m_pDecoder != NULL) {
			AVFrame * pVideoFrame = WXH264DecGetFrame(m_pDecoder, TYPE_BLACK_FRAME);
			WXAirplayVideoRenderDisplay(m_uniqueid, pVideoFrame);
		}
	}

#define VICKY_DEBUG_TEST 0
	int drapframecount=1;
	int drawframecount=1;
	INT64 delay=0;
	int highdelay=0;


#if VICKY_DEBUG_TEST 
	//勿删 vicky调试信息
	int frame_count = 0;
	int frame_count_empty = 0;
	int frame_count_min = 0;
	int frame_count_max = 0;
	int frame_count_size = 0;
	int frame_count_size1 = 0;
	int64_t lasttime_sec = 0;
#endif
	int64_t currentTime = 0;
	int64_t lasttime = 0;

	int64_t currentrenderTime = 0;
	int64_t lastrendertime = 0;
	int64_t avggpurendertime;
	int drop_count = 0;

	//解码函数
	void DecodeVideoFrame(uint8_t *outbuf, int size, INT64 pts)
	{

		currentTime = WXGetTimeMs();
#if VICKY_DEBUG_TEST 
		//勿删 vicky调试信息
		if (currentTime - lasttime_sec > 1000)
		{
			WXLogWriteNew("fpss  %d\n  empty=%d  min=%d max=%d size=%d size1=%d",
				frame_count, frame_count_empty, frame_count_min, frame_count_max,
				frame_count_size, frame_count_size1);
			lasttime_sec = currentTime;
			frame_count = 0;
			frame_count_empty = 0;
			frame_count_min = 0;
			frame_count_max = 0;
			frame_count_size = 0;
			frame_count_size1 = 0;
		}
		{
			WXAutoLock al(m_lockMirrorData);
			WXLogWriteNew("fpssnew  size=%d",
				m_queueMirrorData.size());
		}
#endif

		int bRet = WXH264DecSendPacket(m_pDecoder, (uint8_t*)outbuf, size, pts);

		if (bRet) {  //解码正常
			int Dw = WXH264DecGetWidth(m_pDecoder);
			int Dh = WXH264DecGetHeight(m_pDecoder);
			if (Dw == 0 || Dh == 0) {
				return;
			}

			if ((m_iWidth != Dw || m_iHeight != Dh)) {
				WXLogWriteNew("H264 DecodeFrame Change Render Size Old=%dx%d New=%dx%d ", m_iWidth, m_iHeight, Dw, Dh);
				m_iWidth = Dw;
				m_iHeight = Dh;
				ConnectStatusStruct stConnect;
				stConnect.eConnectStatus = CONNECTED;
				stConnect.iConnectType = 1;
				if (CNetworkServices::Get().m_stAirplay.m_CallBackConnectInfo != NULL)
				{
					//not allow connect
					if (CNetworkServices::Get().m_stAirplay.m_CallBackConnectInfo(stConnect, m_uniqueid) == 0)
					{
						set_mirror_status(m_uniqueid, 1);
						CNetworkServices::Get().DisconnectMirrorNew(m_uniqueid);
						WXLogWriteNew("connect pool is full");
						return;
					}
				}
			}
			//解码数据其实都是J420 .。。 
			AVFrame* pFrameDecode = WXH264DecGetFrame(m_pDecoder, TYPE_CURR_FRAME);//J420 ?
			
			if (pFrameDecode) {
				pFrameDecode->format = AV_PIX_FMT_YUVJ420P;
				WXAirplayVideoRenderDisplay(m_uniqueid, pFrameDecode);
			}
		}
	}

	//Mirror 数据处理
	void MirrorDataFunc()
	{
		//uint8_t arcData[1025] = { 0 };
		int iBufferLen = 1024 * 1024 * 5;//视频帧最大值，经验值。。

		WXDataBuffer parcNeedData;//加密数据
		parcNeedData.Init(nullptr, iBufferLen,0);

		WXDataBuffer avcData;//H264数据
		avcData.Init(nullptr, iBufferLen,0);

		WXDataBuffer extraData;//sps+pps
		extraData.Init(nullptr, 200,0);

		unsigned int iNeedSize = 0;

		char ecount_buf[AES_BLOCK_SIZE] = { 0 };
		unsigned int m_num = 0;
		int64_t m_nTime1 = WXGetTimeMs();
		int64_t m_nTime2 = 0;

		std::string strPayLoadLast = "";


		while (TRUE) {
			//stop mirroring stream
			if (get_mirror_status(m_uniqueid) != 0)
			{
				WXLogWriteNew("begin disconnect mirror");
				CAirPlayServer::SetMirroring(0);
				//force exit
				if (get_mirror_status(m_uniqueid) != 2)
				{
					ConnectStatusStruct stConnect;
					stConnect.eConnectStatus = DISCONNECT;
					stConnect.iConnectType = 1;
					if (CNetworkServices::Get().m_stAirplay.m_CallBackConnectInfo != NULL)
					{
						CNetworkServices::Get().m_stAirplay.m_CallBackConnectInfo(stConnect, m_uniqueid);
					}
					SetWindowFinalize(m_strRemoteIp.c_str());
				}
				MirrorDataClean();
				WXLogWriteNew("disconnect mirror success");
				break;
			}

			WXDataBuffer* obj = MirrorDataPop();
			if (obj) {
				unsigned int size = 0;
				unsigned short type = 0;

				//get current time
				//m_nTime1 = WXGetTimeMs();//刷新时间

				//int iCurrVideoSize = obj->m_iBufSize;
				////handle h264 stream
				//if (iNeedSize + iCurrVideoSize >= iBufferLen)
				//{
				//	printf("error happening 1\n");
				//	WXLogWriteNew("THREAD_JOIN----iNeedSize:%d + iCurVideoSize:%d >= iBufferLen:%d",
				//		iNeedSize, iCurrVideoSize, iBufferLen);
				//	delete obj;
				//	break;
				//}

				//memcpy(parcNeedData.m_pBuf + iNeedSize, obj->m_pBuf, iCurrVideoSize);
				//delete obj;
				//iNeedSize += iCurrVideoSize;

				const char *data = (const char*)obj->GetBuffer();// (const char*)parcNeedData.m_pBuf;
				size = *(unsigned int*)data;
				////stream size
				//if ((size + 128) > iNeedSize)
				//{
				//	continue;
				//}
				type = *(unsigned short*)(data + 4);
				//type1 = *(unsigned short*)(data + 6);
				//timestamp = *(uint64_t*)(data + 8);
				//std::string strPayLoad = "";
				//if (size != 0)
				//{
				//	strPayLoad.assign(data + 128, size);
				//}
				//video data
				if ((type == 0 || type == 0x1000) && size &&  m_pDecoder != NULL)
				{
					avcData.m_pts = obj->m_pts;
					CRYPTO_ctr128_encrypt((const uint8_t*)(data + 128), avcData.GetBuffer(), size, &m_key, (uint8_t*)m_strParam2.c_str(),
						(uint8_t*)ecount_buf, &m_num,
						(block128_f)AES_encrypt);

					DecodeVideoFrame(avcData.GetBuffer(), size, avcData.m_pts);//正常解码显示
				}
				//codec data
				else if (type == 1)
				{
					//check last condec_context change or not
					int ichange = 0;
					if (strPayLoadLast.size() != size/*strPayLoad.size()*//* || CAirPlayServer::IsAirplayPlaying() == 1*/) {
						ichange = 1;
					}
					else if (strPayLoadLast.size() == size/*strPayLoad.size()*/) {
						for (int i = 0; i < size/*strPayLoad.size()*/; i++) {
							if ((uint8_t)strPayLoadLast.c_str()[i] != (uint8_t)/*strPayLoad.c_str()*/data[128 + i]) {
								ichange = 1;
								break;
							}
						}
					}

					if (ichange != 0)
					{
						//first codec packet
						strPayLoadLast.assign(data + 128, size);// = strPayLoad;
						if (m_pDecoder != NULL) {
							DecodeVideoFrame((uint8_t*)(data + 128)/*strPayLoad.c_str()*/, size/*strPayLoad.size()*/, obj->m_pts);//SPS+PPS数据帧
						}
						else {
							int len = size;// strPayLoad.size();
							if (len != 0)
							{
								uint8_t *pExtraData = (uint8_t *)(data + 128)/*strPayLoad.c_str()*/;
								int iExtraSize = len;

								ConnectStatusStruct stConnect;
								stConnect.eConnectStatus = CONNECTED;
								stConnect.iConnectType = 1;

								WXString strName;
								strName.Format("%s", CNetworkServices::Get().m_mapName[m_uniqueid].c_str());
								wcscpy(stConnect.wszDevName, strName.str());

								uint64_t uniqueid = 0;
								memcpy(&uniqueid, m_strRemoteIp.c_str(), 4);
								if (CNetworkServices::Get().m_stAirplay.m_CallBackConnectInfo != NULL)
								{
									//not allow connect
									if (CNetworkServices::Get().m_stAirplay.m_CallBackConnectInfo(stConnect, uniqueid) == 0)
									{
										//
										set_mirror_status(m_uniqueid, 1);
										CNetworkServices::Get().DisconnectMirrorNew(uniqueid);
										WXLogWriteNew("connect pool is full");
										continue;
									}
								}

								HWND hwnd = (HWND)CNetworkServices::Get().m_stAirplay.m_CallBackGetParentWindow(uniqueid);

								WXLogWriteNew("dxva init end");

								int Sw = 0;
								int Sh = 0;
								H264GetSize(pExtraData, iExtraSize, &Sw, &Sh);

								if (CNetworkServices::Get().m_stAirplay.m_CallBackWindowStatus != NULL)
								{
									WindowShowStruct stWindowShow;
									stWindowShow.eWindowStatus = Sw > Sh ? LANDSCAPE : PORTRAIT;
									stWindowShow.iScreenW = Sw;
									stWindowShow.iScreenH = Sh;
									std::string strDevice = CNetworkServices::Get().m_mapModel[uniqueid];

									WXString wxstrDevice;
									wxstrDevice.Format("%s", strDevice.c_str());
									wcscpy(stWindowShow.m_wszType, wxstrDevice.str());
									stWindowShow.iIPad = AirplayGetIpadType(strDevice);

									std::string strOsVersion = CNetworkServices::Get().m_mapOsVersion[uniqueid];
									stWindowShow.iOsVersion = AirplayGetOsVersionType(strOsVersion);//记得加上iphone系统信息 多次回调信息少了 会影响上层调用东西
									CNetworkServices::Get().m_stAirplay.m_CallBackWindowStatus(stWindowShow, uniqueid);
								}
								//WXH264DecSetHw(FALSE);
								m_pDecoder = WXH264DecCreate(pExtraData, iExtraSize);
								if (m_pDecoder == NULL) {
									return;
								}

							}
						}
					}
					else {
						DrawBlack();
					}
				}
				//heartbeat
				else if (type == 2)
				{
					printf("heart\n");
				}
				else if (type == 5) {
					plist_t resplist = NULL;
					const char *tmpbuf = data + 128;
					printf("\n");
				}
				//memset(parcNeedData.m_pBuf, 0, size + 128);
				//memcpy(parcNeedData.m_pBuf, parcNeedData.m_pBuf + size + 128, iBufferLen - size - 128);
				//memset(parcNeedData.m_pBuf + iBufferLen - 128 - size, 0, 128 + size);
				//iNeedSize -= size + 128;
				delete obj;
			}
			else {
				//m_nTime2 = WXGetTimeMs();
				//if (m_nTime2 - m_nTime1 >= 15 * 1000)//30s
				//{
				//	set_mirror_status(m_uniqueid, 1);
				//	CNetworkServices::Get().DisconnectMirrorNew(m_uniqueid);//通知底层中断
				//	WXLogWriteNew("network error, we can't get more mirror data, so disconnect the connection");
				//}
				WXSleepMs(1);//空队列
				continue;
			}
		}


		WXLogWriteNew("MirrorDataFunc Stop!!!!");

		//线程退出的清理
		if (m_pDecoder) {
			WXH264DecDestroy(m_pDecoder);
			m_pDecoder = NULL;
		}

		WXAirplayVideoRenderDestroy(m_uniqueid);
		set_mirror_status(m_uniqueid, 0);
	}

public:
	int m_iWidth = 0;
	int m_iHeight = 0;
	uint64_t m_uniqueid = 0;
	std::string m_strParam1;
	std::string m_strParam2;
	AES_KEY m_key;

	std::string m_strRemoteIp;
};

static void Handle(HandleMirroringStream *pHandler)
{
	CAirPlayServer::SetMirroring(1);
	uint64_t uniqueid = pHandler->m_uniqueid;
	pHandler->MirrorDataFunc();
	delete pHandler;
	s_MapMirror[uniqueid] = NULL;
	CAirPlayServer::SetMirroring(0);
}

extern airplay_t *g_airplay;

void StartMirror(const char *strParam1, const char *strParam2, const char* remoteip)
{
#if JUSTIN_DEBUG
	InitLog();
#endif
	uint64_t uniqueid = 0;
	memcpy(&uniqueid, remoteip, 4);
	set_mirror_status(uniqueid, 0);
	HandleMirroringStream *pHandle = new HandleMirroringStream(strParam1, strParam2, remoteip);
	pHandle->m_uniqueid = uniqueid;
	s_MapMirror[uniqueid] = pHandle;

	std::thread new_thread(&Handle, pHandle);
	new_thread.detach();
	httpd_set_mirror_streaming(g_airplay->mirror_server, uniqueid);
}

static void * conn_init(void *opaque, uint8_t *local, int locallen, uint8_t *remote, int remotelen, char* arcRemoteIp)
{
	airplay_conn_t *conn = (airplay_conn_t *)calloc(1, sizeof(airplay_conn_t));
	if (!conn) {
		return NULL;
	}
	conn->airplay = (airplay_t *)opaque;

	if (locallen == 4) {
		WXLogWriteNew(
			"Local: %d.%d.%d.%d",
			local[0], local[1], local[2], local[3]);
	}
	else if (locallen == 16) {
		WXLogWriteNew(
			"Local: %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
			local[0], local[1], local[2], local[3], local[4], local[5], local[6], local[7],
			local[8], local[9], local[10], local[11], local[12], local[13], local[14], local[15]);
	}
	if (remotelen == 4) {

		WXLogWriteNew(
			"Remote: %d.%d.%d.%d",
			remote[0], remote[1], remote[2], remote[3]);
	}
	else if (remotelen == 16) {
		WXLogWriteNew(
			"Remote: %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
			remote[0], remote[1], remote[2], remote[3], remote[4], remote[5], remote[6], remote[7],
			remote[8], remote[9], remote[10], remote[11], remote[12], remote[13], remote[14], remote[15]);
	}

	conn->local = (uint8_t *)malloc(locallen);
	assert(conn->local);
	memcpy(conn->local, local, locallen);

	conn->remote = (uint8_t *)malloc(remotelen);
	assert(conn->remote);
	memcpy(conn->remote, remote, remotelen);

	conn->locallen = locallen;
	conn->remotelen = remotelen;

	digest_generate_nonce(conn->nonce, sizeof(conn->nonce));

	std::string strRemoteIp;
	strRemoteIp.assign((char*)conn->remote, 4);

	if (strRemoteIp != "") {
		uint64_t uniqueid = 0;
		memcpy(&uniqueid, conn->remote, 4);
	}

	return conn;
}

static void conn_request(void *ptr, http_request_t *request, http_response_t **response)
{
	const char realm[] = "airplay";
	int headerLength = 0;
	int status = AIRPLAY_STATUS_OK;
	int needAuth = 0;
	airplay_conn_t *conn = (airplay_conn_t*)ptr;
	airplay_t *airplay = conn->airplay;
	char * statusMsg = (char*)"OK";

	const char *method;
	//const char *challenge;
	int require_auth = 0;
	char responseHeader[4096] = { 0 };
	char responseBody[4096] = { 0 };
	int  responseLength = 0;

	const char * uri = http_request_get_url(request);
	method = http_request_get_method(request);

	if (!method) {
		return;
	}

	WXLogWriteNew("%s uri=%s\n", method, uri);

	{
		const char *data;
		int len;
		data = http_request_get_data(request, &len);
		WXLogWriteNew("data len %d:%s\n", len, data);
	}

	// This is the socket which will be used for reverse HTTP
	// negotiate reverse HTTP via upgrade
	if (strcmp(uri, "/reverse") == 0)
	{
		status = AIRPLAY_STATUS_SWITCHING_PROTOCOLS;
		sprintf(responseHeader, "Upgrade: PTTH/1.0\r\nConnection: Upgrade\r\n");
	}

	// The rate command is used to play/pause media.
	// A value argument should be supplied which indicates media should be played or paused.
	// 0.000000 => pause
	// 1.000000 => play
	else if (strcmp(uri, "/rate") == 0)
	{

	}

	// The volume command is used to change playback volume.
	// A value argument should be supplied which indicates how loud we should get.
	// 0.000000 => silent
	// 1.000000 => loud
	else if (strcmp(uri, "/volume") == 0)
	{

	}


	// Contains a header like format in the request body which should contain a
	// Content-Location and optionally a Start-Position
	else if (strcmp(uri, "/play") == 0)
	{

	}

	// Used to perform seeking (POST request) and to retrieve current player position (GET request).
	// GET scrub seems to also set rate 1 - strange but true
	else if (strcmp(uri, "/scrub") == 0)
	{

	}

	// Sent when media playback should be stopped
	else if (strcmp(uri, "/stop") == 0)
	{

	}

	// RAW JPEG data is contained in the request body
	else if (strcmp(uri, "/photo") == 0)
	{

	}

	else if (strcmp(uri, "/playback-info") == 0)
	{

	}

	else if (strcmp(uri, "/stream.xml") == 0)
	{
		WXLogWriteNew("AIRPLAY: got request %s", uri);
		sprintf(responseBody, "%s", STREAM_INFO);
		sprintf(responseHeader, "Content-Type: text/x-apple-plist+xml\r\n");

	}
	else if (strcmp(uri, "/stream") == 0)
	{
		const char *plist_bin = NULL;
		char *xml = NULL;
		int size = 0;
		plist_t root = NULL;

		plist_bin = http_request_get_data(request, &size);
		plist_from_bin(plist_bin, size, &root);
		if (root)
		{
			plist_to_xml(root, &xml, (uint32_t*)&size);
			/* TODO: in this plist, we will get param1&param2, which is the
			encoded aeskey & aesiv */
			if (xml) fprintf(stderr, "%s\n", xml);
			std::string strXml = "";
			strXml.assign(xml, size);
			if (plist_dict_get_size(root))
			{
				plist_t tmpNode = plist_dict_get_item(root, "param1");
				uint64_t len = 0;
				char *val = NULL;
				std::string strParam1 = "";
				std::string strParam2 = "";
				if (tmpNode)
				{
					plist_get_data_val(tmpNode, &val, &len);
					strParam1.assign(val, (size_t)len);
				}

				tmpNode = plist_dict_get_item(root, "param2");
				if (tmpNode)
				{
					plist_get_data_val(tmpNode, &val, &len);
					strParam2.assign(val, (size_t)len);
				}
				plist_free(root);

				//send encrypted param1 key
				int sock_fd = get_fairplay_socket();
				if (sock_fd == 0)
				{
					return;
				}
				char recvbuf[1024] = { 0 };
				char sendbuf[1024] = { 0 };

				uint8_t *buf;
				memcpy(sendbuf, strParam1.c_str(), 72);

				int sendlen = 72;
				int retlen = send(sock_fd, sendbuf, sendlen, 0);
				if (retlen < 0) {
					close_fairplay_socket();
					return;
				}

				retlen = recv(sock_fd, recvbuf, 1024, 0);
				if (retlen <= 0) {
					close_fairplay_socket();
					return;
				}
				buf = (uint8_t*)malloc(retlen);
				memcpy(buf, recvbuf, retlen);
				close_fairplay_socket();
			}

			/* after /stream, this connection will no longer a http session */
			//httpd_set_mirror_streaming(conn->airplay->mirror_server);
			return;
		}
		else
		{
			WXLogWriteNew("AIRPLAY: Invalid bplist");
			status = AIRPLAY_STATUS_NOT_FOUND;
		}
	}
	else if (strcmp(uri, "/server-info") == 0)
	{
		WXLogWriteNew("AIRPLAY: got request %s", uri);
		sprintf(responseBody, "%s\r\n", conn->airplay->hwaddr);
		sprintf(responseHeader, "Content-Type: text/x-apple-plist+xml\r\n");
	}

	else if (strcmp(uri, "/slideshow-features") == 0)
	{
		// Ignore for now.
	}

	else if (strcmp(uri, "/authorize") == 0)
	{
		// DRM, ignore for now.
	}

	else if (strcmp(uri, "/setProperty") == 0)
	{
		status = AIRPLAY_STATUS_NOT_FOUND;
	}

	else if (strcmp(uri, "/getProperty") == 0)
	{
		status = AIRPLAY_STATUS_NOT_FOUND;
	}

	else if (strcmp(uri, "/fp-setup") == 0)
	{
		ConnectStatusStruct stConnect;
		stConnect.eConnectStatus = STARTING;
		stConnect.iConnectType = 1;
		if (CNetworkServices::Get().m_stAirplay.m_CallBackConnectInfo != NULL)
		{
			//CNetworkServices::Get().m_stAirplay.m_CallBackConnectInfo(stConnect);
		}

		const uint8_t *data;
		int datalen;

		data = (const uint8_t *)http_request_get_data(request, &datalen);

		if (data[6] == 0x01)
		{
			//start airplay connect
			ConnectStatusStruct stConnect;
			stConnect.eConnectStatus = CONNECTING;
			stConnect.iConnectType = 1;
			if (CNetworkServices::Get().m_stAirplay.m_CallBackConnectInfo != NULL)
			{
				//CNetworkServices::Get().m_stAirplay.m_CallBackConnectInfo(stConnect);
			}

			int size1 = 142;

			int sock_fd = get_fairplay_socket();
			if (sock_fd == 0)
			{
				return;
			}
			char recvbuf[1024] = { 0 };
			char sendbuf[1024] = { 0 };
			int sendlen = 16;
			int retlen;
			//uint8_t *buf;
			memcpy(sendbuf, data, sendlen);
			retlen = send(sock_fd, sendbuf, sendlen, 0);
			if (retlen < 0) {
				close_fairplay_socket();
				return;
			}

			retlen = recv(sock_fd, recvbuf, 1024, 0);
			if (retlen <= 0) {
				close_fairplay_socket();
				return;
			}
			s_pBuf31 = (uint8_t*)malloc(retlen);
			memcpy(s_pBuf31, recvbuf, retlen);

			if (s_pBuf31) {
				memcpy(responseBody, s_pBuf31, size1);
				responseLength = size1;
				free(s_pBuf31);
				sprintf(responseHeader, "Content-Type: application/octet-stream\r\n");
			}
		}
		else
		{
			ConnectStatusStruct stConnect;
			stConnect.eConnectStatus = CONNECTING;
			stConnect.iConnectType = 1;
			if (CNetworkServices::Get().m_stAirplay.m_CallBackConnectInfo != NULL)
			{
				//CNetworkServices::Get().m_stAirplay.m_CallBackConnectInfo(stConnect);
			}

			int size2 = 32;
			char buf2[32] = {
				0x46, 0x50, 0x4c, 0x59, 0x03, 0x01, 0x04, 0x00, 0x00, 0x00, 0x00, 0x14, 0,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0
			};
			for (int i = 0; i < 20; i++)
			{
				buf2[i + 12] = data[164 - 20 + i];
			}

			int sock_fd = get_fairplay_socket();
			if (sock_fd == 0)
			{
				return;
			}
			char recvbuf[1024] = { 0 };
			char sendbuf[1024] = { 0 };
			int sendlen = 164;
			int retlen;
			//uint8_t *data = (uint8_t*)param;
			memcpy(sendbuf, data, sendlen);
			retlen = send(sock_fd, sendbuf, sendlen, 0);
			if (retlen < 0) {
				close_fairplay_socket();
				return;
			}

			////for test
			//{
			//	FILE *fp = fopen("2.txt", "wb");
			//	fwrite(sendbuf, sendlen, 1, fp);
			//	fclose(fp);
			//}

			retlen = recv(sock_fd, recvbuf, 1024, 0);
			if (retlen <= 0) {
				close_fairplay_socket();
				return;
			}
			s_pBuf32 = (uint8_t*)malloc(retlen);
			memcpy(s_pBuf32, recvbuf, retlen);

			if (s_pBuf32) {
				memcpy(responseBody, s_pBuf32, size2);
				free(s_pBuf32);
				responseLength = size2;
				sprintf(responseHeader, "Content-Type: application/octet-stream\r\n");
			}
		}

	}

	else if (strcmp(uri, "200") == 0) //response OK from the event reverse message
	{
		status = AIRPLAY_STATUS_NO_RESPONSE_NEEDED;
	}
	else
	{
		WXLogWriteNew("AIRPLAY Server: unhandled request [%s]\n", uri);
		status = AIRPLAY_STATUS_NOT_IMPLEMENTED;
	}

	if (status == AIRPLAY_STATUS_NEED_AUTH)
	{
		//ComposeAuthRequestAnswer(responseHeader, responseBody);
	}



	switch (status)
	{
	case AIRPLAY_STATUS_NOT_IMPLEMENTED:
		statusMsg = (char*)"Not Implemented";
		break;
	case AIRPLAY_STATUS_SWITCHING_PROTOCOLS:
		statusMsg = (char*)"Switching Protocols";
		//reverseSockets[sessionId] = m_socket;//save this socket as reverse http socket for this sessionid
		break;
	case AIRPLAY_STATUS_NEED_AUTH:
		statusMsg = (char*)"Unauthorized";
		break;
	case AIRPLAY_STATUS_NOT_FOUND:
		statusMsg = (char*)"Not Found";
		break;
	case AIRPLAY_STATUS_METHOD_NOT_ALLOWED:
		statusMsg = (char*)"Method Not Allowed";
		break;
	case AIRPLAY_STATUS_PRECONDITION_FAILED:
		statusMsg = (char*)"Precondition Failed";
		break;
	}
	{
		// Prepare the response
		char resbuf[4096];
		http_response_t *res;
		//const time_t ltime = time(NULL);
		//char *date = asctime(gmtime(&ltime)); //Fri, 17 Dec 2010 11:18:01 GMT;
		//date[strlen(date) - 1] = '\0'; // remove \n
		//sprintf(resbuf, "HTTP/1.1 %d %s\nDate: %s\r\n", status, statusMsg, date);
		sprintf(resbuf, "HTTP/1.1 %d %s\r\n", status, statusMsg);
		if (responseHeader[0] != '\0')
		{
			strcat(resbuf, responseHeader);
		}

		if (responseLength == 0) responseLength = strlen(responseBody);

		sprintf(resbuf, "%sContent-Length: %d\r\n\r\n", resbuf, responseLength);

		headerLength = strlen(resbuf);

		if (responseLength)
		{
			memcpy(resbuf + strlen(resbuf), responseBody, responseLength);
			resbuf[headerLength + responseLength] = 0;
		}


		res = http_response_init1(resbuf, headerLength + responseLength);

		WXLogWriteNew("AIRPLAY Handled request %s with response %s", method, http_response_get_data(res, &responseLength));
		*response = res;
	}
}

static void conn_destroy(void *ptr)
{
	airplay_conn_t *conn = (airplay_conn_t *)ptr;

	//if (conn->airplay_rtp) {
	//	/* This is done in case TEARDOWN was not called */
	//	raop_rtp_destroy(conn->airplay_rtp);
	//}
	free(conn->local);
	free(conn->remote);
	free(conn);
}

static void conn_mirror_destroy(const char* remoteip)
{
	uint64_t uniqueid = 0;
	memcpy(&uniqueid, remoteip, 4);
	set_mirror_status(uniqueid, 1);
}
#include <chrono>
static void conn_datafeed(void *ptr, uint8_t *data, int len, uint64_t uniqueid, uint64_t pts) {
	if (len <= 0) {
		return;
	}

	//pts = WXGetTimeMs(); 
	if (s_MapMirror[uniqueid]) {
		s_MapMirror[uniqueid]->MirrorDataPush(data, len, pts);
	}
}

int  get_mirror_stream(uint64_t uniqueid) {
	WXAutoLock lock(s_lockHttp);
	return s_HttpStreamMap[uniqueid];
}

void set_mirror_stream(uint64_t uniqueid, int disconnect)
{
	WXAutoLock lock(s_lockHttp);
	s_HttpStreamMap[uniqueid] = disconnect;
}

airplay_t * airplay_init(int max_clients, airplay_callbacks_t *callbacks, const char *pemkey, int *error)
{
	airplay_t *airplay;
	httpd_t *httpd;
	httpd_t *mirror_server;
	rsakey_t *rsakey;
	httpd_callbacks_t httpd_cbs;

	assert(callbacks);
	assert(max_clients > 0);
	assert(max_clients < 100);
	assert(pemkey);

	/* Initialize the network */
	if (netutils_init() < 0) {
		return NULL;
	}

	// 	/* Validate the callbacks structure */
	// 	if (!callbacks->audio_init ||
	// 	    !callbacks->audio_process ||
	// 	    !callbacks->audio_destroy) {
	// 		return NULL;
	// 	}

	/* Allocate the airplay_t structure */
	airplay = (airplay_t*)calloc(1, sizeof(airplay_t));
	if (!airplay) {
		return NULL;
	}

	/* Set HTTP callbacks to our handlers */
	memset(&httpd_cbs, 0, sizeof(httpd_cbs));
	httpd_cbs.opaque = airplay;
	httpd_cbs.conn_init = &conn_init;
	httpd_cbs.conn_request = &conn_request;
	httpd_cbs.conn_destroy = &conn_destroy;
	httpd_cbs.conn_datafeed = &conn_datafeed;
	httpd_cbs.conn_mirror_destroy = &conn_mirror_destroy;
	httpd_cbs.set_mirror_stream = &set_mirror_stream;
	httpd_cbs.get_mirror_stream = &get_mirror_stream;

	/* Initialize the http daemon */
	httpd = httpd_init(&httpd_cbs, max_clients);
	if (!httpd) {
		free(airplay);
		return NULL;
	}

	/* Initialize the mirror server daemon */
	mirror_server = httpd_init(&httpd_cbs, max_clients);
	if (!mirror_server) {
		free(httpd);
		free(airplay);
		return NULL;
	}

	/* Copy callbacks structure */
	memcpy(&airplay->callbacks, callbacks, sizeof(airplay_callbacks_t));

	/* Initialize RSA key handler */
	rsakey = rsakey_init_pem(pemkey);
	if (!rsakey) {
		free(httpd);
		free(mirror_server);
		free(airplay);
		return NULL;
	}

	airplay->httpd = httpd;
	airplay->rsakey = rsakey;

	airplay->mirror_server = mirror_server;

	return airplay;
}

airplay_t * airplay_init_from_key(int max_clients, airplay_callbacks_t *callbacks)
{
	airplay_t *airplay;
	airplay = airplay_init(max_clients, callbacks, RSA_KEY, NULL);
	return airplay;
}

void airplay_destroy(airplay_t *airplay)
{
	if (airplay) {
		airplay_stop(airplay);

		httpd_destroy(airplay->httpd);
		httpd_destroy(airplay->mirror_server);
		rsakey_destroy(airplay->rsakey);
		free(airplay);

		/* Cleanup the network */
		netutils_cleanup();
	}
}

int airplay_is_running(airplay_t *airplay)
{
	assert(airplay);

	return httpd_is_running(airplay->httpd);
}

int airplay_start(airplay_t *airplay, unsigned short *port, unsigned short *mirror_port, const char *hwaddr, int hwaddrlen, const char *password)
{
	WXLogWriteNew("airplay_start");
	int ret;
	//unsigned short mirror_port;

	assert(airplay);
	assert(port);
	assert(mirror_port);
	assert(hwaddr);

	/* Validate hardware address */
	if (hwaddrlen > MAX_HWADDR_LEN) {
		return -1;
	}

	memset(airplay->password, 0, sizeof(airplay->password));
	if (password) {
		/* Validate password */
		if (strlen(password) > MAX_PASSWORD_LEN) {
			return -1;
		}

		/* Copy password to the airplay structure */
		strncpy(airplay->password, password, MAX_PASSWORD_LEN);
	}

	/* Copy hwaddr to the airplay structure */
	memcpy(airplay->hwaddr, hwaddr, hwaddrlen);
	airplay->hwaddrlen = hwaddrlen;
	ret = httpd_start(airplay->httpd, port, 0);
	int iCount = 0;
	while (ret < 0)
	{
		if (iCount++ > 3)
		{
			WXLogWriteNew("airplay_start fail for 3 times");
			break;
		}

		if (ret == -3)
		{
			*port += 2;
			ret = httpd_start(airplay->httpd, port, 0);
		}
		else
		{
			break;
		}
	}
	if (ret != 1) return ret;
	ret = httpd_start(airplay->mirror_server, mirror_port, 1);
	iCount = 0;
	while (ret < 0)
	{
		if (iCount++ > 3)
		{
			WXLogWriteNew("airplay_start mirror fail for 3 times");
			break;
		}

		if (ret == -3)
		{
			*mirror_port += 2;
			ret = httpd_start(airplay->mirror_server, mirror_port, 1);
		}
		else
		{
			break;
		}
	}

	return ret;
}

void airplay_stop(airplay_t *airplay)
{
	WXLogWriteNew("airplay_stop");
	if (airplay != NULL)
	{
		httpd_stop(airplay->mirror_server);
	}
}

void airplay_disconnect(airplay_t *airplay, uint64_t uniqueid)
{
	if (airplay != NULL)
	{
		httpd_disconnect(airplay->mirror_server, uniqueid);
	}
}

void mirror_disconnect(airplay_t *airplay, uint64_t uniqueid)
{
	if (airplay != NULL)
	{
		httpd_remove_connection_new(airplay->mirror_server, uniqueid);
	}
}

void mirror_exit()
{
	WXLogWriteNew("mirror_exit");
	set_mirror_status(NULL, 2);
}

AIRPLAY_API int  GetRecordWidth(uint64_t uniqueid) {
	return   s_MapWidth.count(uniqueid) ? s_MapWidth[uniqueid] : 0;
}

AIRPLAY_API int  GetRecordHeight(uint64_t uniqueid) {
	return   s_MapHeight.count(uniqueid) ? s_MapHeight[uniqueid] : 0;
}

AIRPLAY_API void  SetDisplayMode(int mode) {
	int new_mode = RENDER_TYPE_D3DX;

	if (mode == RENDER_TYPE_D3D)
		new_mode = RENDER_TYPE_D3D;
	else if (mode == RENDER_TYPE_D3DX)
		new_mode = RENDER_TYPE_D3DX;
	else if (mode == RENDER_TYPE_OPENGL)
		new_mode = RENDER_TYPE_D3DX;
	else if (mode == RENDER_TYPE_GDI)
		new_mode = RENDER_TYPE_GDI;

	if (new_mode != g_nModeVideoRender) { //模式改变
		g_nModeVideoRender = new_mode;
	}
}


//在当前状态下进行翻转和旋转
AIRPLAY_API void WXAirplaySetDisplayRotateFlip(uint64_t uniqueid, int type) {
	int nRotateFile = RENDER_ROTATE_NONE;//原始值
	if (s_MapVideoRotateFilp.count(uniqueid) != 0) {
		nRotateFile = s_MapVideoRotateFilp[uniqueid];//存在
	}
	int newRotateFile = WXGetRotateFilp(nRotateFile, type);//新的显示角度
	s_MapVideoRotateFilp[uniqueid] = newRotateFile;//替换
}


AIRPLAY_API void WXAirplayShotPicture(uint64_t uniqueid, const wchar_t* strName) {
	WXAutoLock al(s_lockPicture);
	s_mapShotPicture[uniqueid] = 1;
	s_mapStrPicture[uniqueid] = strName;
}

//改变窗体大小，强制刷新视频画面
AIRPLAY_API void  ChangeSize() {

}

AIRPLAY_API void  SetDisplayRotate(int rotate) {
	for (auto obj : s_MapVideoRotateFilp) {
		uint64_t uid = obj.first;
		int new_rotate = rotate;
		if (rotate < 0)new_rotate = RENDER_ROTATE_NONE;
		if (rotate > 3)new_rotate = RENDER_ROTATE_NONE;
		if (new_rotate != obj.second) { //角度改变
			s_MapVideoRotateFilp[uid] = new_rotate;

			/** 强制刷新
			 *  投照片时，后续没有新的数据过来，不会触发旋转效果，需要强制刷新一下当前帧
			 *  modify by morgan
			 *  2022/03/16
			 */
			WXVideoRenderDisplay(g_MapVideoRender[uid], NULL, g_bFixed, s_MapVideoRotateFilp[uid]);
		}
	}
}

AIRPLAY_API void  WXAirplaySetLogDrop(int bLog) {

}

AIRPLAY_API void SetDisplayHighFPS(int high) {

}


AIRPLAY_API void WXAirplaySetShowJ420(int bJ420) {
	s_bShowJ420 = !!bJ420;
	//WXMediaSetLut(s_bShowJ420);
}

AIRPLAY_API void SetH264DecodeHwMode(int mode) {
	WXH264DecSetHw(mode);
}

AIRPLAY_API void WXAirplaySetVideoRenderFixed(int fixed) {
	g_bFixed = fixed;
}





#include <windows.h>

#include "../mdns/dns_sd.h"

extern "C" {
	extern int _stdcall DNSServiceStart();
}


static int s_init = 0;
AIRPLAY_API void AirplayMDNSInit() {
	if (s_init == 0) {
		s_init = 1;
		DNSServiceStart();
	}
}

EXTERN_C BOOL WINAPI plist_init(HINSTANCE hModule, DWORD dwReason, LPVOID lpReserved);
BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	
	plist_init((HINSTANCE)hModule, ul_reason_for_call, lpReserved);
	
	std::wstring strPath = L"";

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:

		InitDeviceList();
		WXH264DecSetHw(FALSE);
		break;

	case DLL_THREAD_ATTACH:
		//pthread_win32_thread_attach_np();
		break;

	case DLL_THREAD_DETACH:
		//pthread_win32_thread_detach_np();
		break;

	case DLL_PROCESS_DETACH:
		//pthread_win32_thread_detach_np();
		//pthread_win32_process_detach_np();
		break;
	}
	return (TRUE);
}


