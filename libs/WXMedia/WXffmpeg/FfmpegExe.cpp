/*
基于ffmpeg.exe  视频压缩
*/
#include "FfmpegExe.h"
#include <WXMediaCpp.h>
#include "parseCmd.hpp"


#define MAX_ARGC  100

void  ffmpeg_log(const char* msg) {
	WXLogA(msg);
}

//ffmpeg.exe
WXMEDIA_API int FfmpegExeProcess2(int argv, WXCTSTR *argw) {
    FfmpegExe obj;
    WXString arrStr[MAX_ARGC];
    char *argc[MAX_ARGC];
    for (int i = 0; i < argv; i++) {
        arrStr[i].Format(argw[i]);
        argc[i] = (char*)arrStr[i].c_str();
    }
    return obj.Process(argv, argc);
}

WXMEDIA_API void* FfmpegExeCreate() {
	FfmpegExe *obj = new FfmpegExe;
	return (void*)obj;
}

WXMEDIA_API int FfmpegExeProcess(void *exe, int argv, const wchar_t**argw) {
	FfmpegExe *obj = (FfmpegExe *)exe;
	if (obj) {
		WXString arrStr[MAX_ARGC];
		char *argc[MAX_ARGC];
		for (int i = 0; i < argv; i++) {
			arrStr[i] = argw[i];
			argc[i] = (char*)arrStr[i].c_str();
		}
		return obj->Process(argv, argc);
	}
	return 0;
}

WXMEDIA_API void FfmpegExeDestroy(void *exe) {
	FfmpegExe *obj = (FfmpegExe *)exe;
	if (obj) {
		delete obj;
	}
}


WXMEDIA_API int64_t FfmpegExeGetCurrTime(void *exe) {
	FfmpegExe *obj = (FfmpegExe *)exe;
	if (obj)
		return obj->GetCurrTime();
	return 0;
}

WXMEDIA_API int64_t FfmpegExeGetTotalTime(void *exe) {
	FfmpegExe *obj = (FfmpegExe *)exe;
	if (obj)
		return obj->GetTotalTime();
	return 0;
}

WXMEDIA_API int FfmpegExeGetState(void *exe) {
	FfmpegExe *obj = (FfmpegExe *)exe;
	if (obj)
		return obj->GetState();
	return -1;
}

WXMEDIA_API int FfmpegExeProcess2Wrap(const wchar_t* cmd) {
	std::vector<std::wstring> elems;


	std::wstring token{};
	std::size_t pos{};
	while (ParseToken(cmd, pos, token)) {

		elems.emplace_back(token);
	}


	wchar_t** ppcmds = new wchar_t* [elems.size()];
	for (int i = 0; i < elems.size(); i++) {

		auto len = elems[i].length();
		ppcmds[i] = new wchar_t[len + 1];
		memset(ppcmds[i], 0x00, sizeof(wchar_t) * (len + 1));

	}
	for (int i = 0; i < elems.size(); i++) {

		auto start = elems[i].find_first_of(L"\"");
		auto end = elems[i].find_last_of(L"\"");

		if (start != std::wstring::npos && end != std::wstring::npos) {
			elems[i] = elems[i].substr(start + 1, end - 1);
		}

		wcscpy(ppcmds[i], elems[i].c_str());
	}


	wchar_t** p = (wchar_t**)ppcmds;
	int ret = FfmpegExeProcess2(elems.size(), (const wchar_t**)ppcmds);

	for (int i = 0; i < elems.size(); i++) {

		delete[] ppcmds[i];
	}
	delete[] ppcmds;

	return ret;
}


