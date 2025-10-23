#ifndef _WXLOG_H
#define _WXLOG_H

#include <Windows.h>

EXTERN_C int   WXLogInit(const wchar_t* strFileName);//成功返回1，失败返回0
EXTERN_C void  WXLogA(const char* format, ...);
EXTERN_C void  WXLogW(const wchar_t* format, ...);
EXTERN_C void  WXLogAOnce(const char* format, ...);//只写一次log
EXTERN_C void  WXLogWOnce(const wchar_t* format, ...);//只写一次log


//设置ini路径，只能设置一次
EXTERN_C void WXSetGlobalPath(const wchar_t* wszPath);
//配置ini的数值
EXTERN_C void WXSetGlobalValue(const wchar_t* wszKey, int nValue);
//配置ini的字符串
EXTERN_C void WXSetGlobalString(const wchar_t* wszKey, const wchar_t* strValue);

//从ini获取数值,不存在则返回默认值
EXTERN_C int WXGetGlobalValue(const wchar_t* wszKey, int nDefValue);
//从ini获取字符串,不存在则返回默认值
//strRet 返回值, 默认长度MAX_PATH
//strDef 默认值
EXTERN_C void WXGetGlobalString(const wchar_t* wszKey, wchar_t* strRet, wchar_t* strDef);


#endif