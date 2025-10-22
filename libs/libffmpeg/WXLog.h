#ifndef _WXLOG_H
#define _WXLOG_H

#include <Windows.h>

EXTERN_C int   WXLogInit(const wchar_t* strFileName);//成功返回1，失败返回0
EXTERN_C void  WXLogA(const char* format, ...);
EXTERN_C void  WXLogW(const wchar_t* format, ...);
EXTERN_C void  WXLogAOnce(const char* format, ...);//只写一次log
EXTERN_C void  WXLogWOnce(const wchar_t* format, ...);//只写一次log

#endif