#ifndef _ZLOG_H
#define _ZLOG_H

#include <Windows.h>

EXTERN_C int   WXLogInit(const wchar_t* strFileName);//�ɹ�����1��ʧ�ܷ���0
EXTERN_C void  WXLogA(const char* format, ...);
EXTERN_C void  WXLogW(const wchar_t* format, ...);
EXTERN_C void  WXLogAOnce(const char* format, ...);//ֻдһ��log
EXTERN_C void  WXLogWOnce(const wchar_t* format, ...);//ֻдһ��log

#endif