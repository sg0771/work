#include <WXMediaCpp.h>

//字幕 字体名字、大小、颜色设置
std::wstring  WXGetSubtitleFontName() {
	wchar_t wszFontName[MAX_PATH];
	WXGetGlobalString(L"FontName", wszFontName, L"Arail");
	std::wstring strFontName = wszFontName;
	return strFontName;
}

EXTERN_C int WXGetSubtitleFontPos() {
	return WXGetGlobalValue(L"FontPos", 0);
}

EXTERN_C int WXGetSubtitleFontSize() {
	return WXGetGlobalValue(L"FontSize", 0);
}

EXTERN_C int WXGetSubtitleFontColor() {
	return WXGetGlobalValue(L"FontColor", 0);
}

EXTERN_C int WXGetSubtitleFontBold() {
	return WXGetGlobalValue(L"FontBold", 0);
}
EXTERN_C int WXGetSubtitleFontItalic() {
	return WXGetGlobalValue(L"FontItalic", 0);
}
EXTERN_C int WXGetSubtitleFontUnderLine() {
	return WXGetGlobalValue(L"FontUnderLine", 0);
}
EXTERN_C int WXGetSubtitleFontStrikeOut() {
	return WXGetGlobalValue(L"FontStrikeOut",0);
}





//Patch For Airplay
WXMEDIA_API void WXSetRegionCallBack(void* ctx, WXRegionResetCallBack cb1, WXRegionDrawCallBack cb2) {}

//废弃接口
WXMEDIA_API void WXCaptureSetRegion(int left, int top, int width, int height) {}

WXMEDIA_API void WXSetSoundRenderJitterBuffer(int delay) {}

WXMEDIA_API int  WXGetSoundRenderJitterBuffer() {
	return 0;
}

WXMEDIA_API void WXLogWriteNew(const char* format, ...) {}
WXMEDIA_API void WXLogWriteNewW2(const wchar_t* format, ...) {}
WXMEDIA_API void WXLogWriteNewW(const wchar_t* format, ...) {}

//设置VAD采集，新版可以弃用
WXMEDIA_API  void WXCaptureSetVAD(int b) {}

WXMEDIA_API void WXMediaSetTemp(int b) {}

WXMEDIA_API int WXMediaGetTemp() {
	return 0;
}

WXMEDIA_API void WXMediaSaveParam(int bSave) {}

WXMEDIA_API void WXSetSystemSoundType(int i) {}

WXMEDIA_API void WXSetMicSoundType(int i) {}


//和上一帧视频编码的时间间隔
WXMEDIA_API int64_t WXCaptureGetVideoTimeOut() {
	return 0;
}

WXMEDIA_API int64_t WXCaptureGetDuration() {
	return 0;
}

//static int s_iLevel = LEVEL_BETTER;
WXMEDIA_API void WXSetMachineLevel(int level) {

	return;//针对机器性能自动选择适合的编码画质
	////先屏蔽这个功能，使用默认配置
	//int nLevel = LEVEL_BETTER;
	//if (level < 0 || level > 3) { //默认配置
	//	int memory = WXGetMemory();//获取内存大小，单位G
	//	int cpu_num = WXGetCpuNum();//获取CPU数量，双核，4核，6核，
	//	int cpu_speed = WXGetCpuSpeed();//获取cpu速度，MHz
	//	if (cpu_num <= 4) {
	//		nLevel = LEVEL_GOOD;
	//	}
	//	else if (cpu_num < 6) {
	//		nLevel = LEVEL_BETTER;
	//	}
	//	else {
	//		nLevel = LEVEL_BEST;
	//	}
	//	WXLogW(L"%ws SetLevel [%d] To [%d]", level, nLevel);
	//}
	//else {
	//	level = MIN(3, MAX(level, 1));//强制设置
	//	WXLogW(L"%ws SetLevel [%d] To [%d]", level, nLevel);
	//}
	//WXSetGlobalValue(L"MachLevel", nLevel);//机器性能
}