#ifndef __MIRACASTEXPORT_H
#define __MIRACASTEXPORT_H

typedef void(*WXStartMirror)(unsigned long long llUniqueid);

typedef void(*WXStopMirror)(unsigned long long llUniqueid);

typedef void(*WXGetAudioData)(unsigned long long llUniqueid, const char* pszAudioData, int iSize);

typedef void(*WXGetVideoData)(unsigned long long llUniqueid, const char* pszVideoData, int iSize, int iWidth, int iHeight);

typedef struct TagWXCastStruct
{
	const wchar_t* m_pszName;
	const wchar_t* m_pszLog;
	WXStartMirror m_callBackStart;
	WXStopMirror m_callBackStop;
	WXGetAudioData m_callBackAudio;
	WXGetVideoData m_callBackVideo;
	int m_iLogLevel;
	int m_iNoAVData;
}WXCastStruct;

int WXInitMiracast(const WXCastStruct stMiracast);

void WXUninitMiracast();

void WXSetLogLevel(int iLevel);

void WXStop(unsigned long long llUniqueid);
void WXStopMiraCast();

#endif
