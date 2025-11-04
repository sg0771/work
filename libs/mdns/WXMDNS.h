/*
从 dnssd 程序封装 的 mdns注册和查找功能
*/

#ifndef _WX_MDNS_H_
#define _WX_MDNS_H_

#ifdef DNSSD_EXPORTS
# define WXMDNS_API __declspec(dllexport)
#else
# define WXMDNS_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

//注册简单MDNS服务
WXMDNS_API void*  WXMDNSRegisterStart(const wchar_t * wszAppName, const wchar_t * wszProtocol, int nPort);

//关闭服务	
WXMDNS_API void   WXMDNSRegisterStop(void *ptr);

//------------------ MDNS  发现设备
//数据回调
//type = 0 Add
//type = 1 Remove
typedef void (WINAPI *mdnsCallBack)(void *ctx,
	const wchar_t *wszAppName,
	const wchar_t* wszHostName,
	const wchar_t *wszIP, 
	int nPort, 
	int type);

//返回值为非NULL 表示执行成功
//wszType 表示查找类型
//pSink 表示回调对象
//cb 表示回调函数
WXMDNS_API void* WXMDNSBrowserStart(const wchar_t * wszType, void *pSink, mdnsCallBack cb);
WXMDNS_API void  WXMDNSBrowserStop(void*);//返回

#ifdef __cplusplus
}
#endif

#endif
