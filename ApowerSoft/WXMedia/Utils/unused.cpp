#include <WXMediaCpp.h>

//字幕 字体名字、大小、颜色设置
EXTERN_C WXCTSTR WXGetSubtitleFontName() {
	return WXGetGlobalString(L"FontName");
}

EXTERN_C int WXGetSubtitleFontPos() {
	return WXGetGlobalValue(L"FontPos");
}

EXTERN_C int WXGetSubtitleFontSize() {
	return WXGetGlobalValue(L"FontSize");
}

EXTERN_C int WXGetSubtitleFontColor() {
	return WXGetGlobalValue(L"FontColor");
}

EXTERN_C int WXGetSubtitleFontBold() {
	return WXGetGlobalValue(L"FontBold");
}
EXTERN_C int WXGetSubtitleFontItalic() {
	return WXGetGlobalValue(L"FontItalic");
}
EXTERN_C int WXGetSubtitleFontUnderLine() {
	return WXGetGlobalValue(L"FontUnderLine");
}
EXTERN_C int WXGetSubtitleFontStrikeOut() {
	return WXGetGlobalValue(L"FontStrikeOut");
}

//功能: 声道声音播放时切换声道模式
//参数:
//nMode: 参照宏SOUND_MODE_*

//是否强制使用LAV播放器
/*extern */int g_nForceLav = 0;

//音频双声道模式
/*extern */int g_nSoundMode = SOUND_MODE_NONE;

//全局参数设置
class WXMediaGlobalValue {
public:
	static std::map<std::wstring, int>& GetInt() {
		static std::map<std::wstring, int>s_mapGlobalValue;
		return s_mapGlobalValue;
	}
	static std::map<std::wstring, std::wstring>& GetString() {
		static std::map<std::wstring, std::wstring>s_mapGlobalString;
		return s_mapGlobalString;
	}
};

EXTERN_C void     WXSetGlobalValue(WXCTSTR strType, int nValue) {

	//WXLogW(L"WXSetGlobalValue[%ws]=%d", strType, nValue);

	if (wcsicmp(strType, L"MediaPlayer") == 0) {
		if (nValue == MEDIAPLAYER_LAV) {
			g_nForceLav = TRUE;
			WXLogW(L"MediaPlayer Use LavFilter");
		}
		else {
			g_nForceLav = FALSE;
			WXLogW(L"MediaPlayer Use Ffplay");
		}
		return;
	}

	if (wcsicmp(strType, L"SoundMode") == 0) {
		if (nValue == 0) {
			WXLogW(L"Sound Mode Use Normal");
			g_nSoundMode = SOUND_MODE_NONE;
		}
		else if (nValue == 1) {
			WXLogW(L"Sound Mode Use Max");
			g_nSoundMode = SOUND_MODE_MAX;
		}
		else if (nValue == 2) {
			WXLogW(L"Sound Mode Use Avg");
			g_nSoundMode = SOUND_MODE_AVG;
		}
		else if (nValue == 3) {
			WXLogW(L"Sound Mode Use Left");
			g_nSoundMode = SOUND_MODE_LEFT;
		}
		else if (nValue == 4) {
			WXLogW(L"Sound Mode Use Right");
			g_nSoundMode = SOUND_MODE_RIGHT;
		}
		else {
			WXLogW(L"Sound Mode Use Normal");
			g_nSoundMode = SOUND_MODE_NONE;
		}
		return;
	}

	std::wstring str = strType;
	WXMediaGlobalValue::GetInt()[str] = nValue;
}

EXTERN_C int      WXGetGlobalValue(WXCTSTR strType) {
	std::wstring str = strType;
	if (WXMediaGlobalValue::GetInt().count(str) == 0) {
		WXMediaGlobalValue::GetInt()[str] = 0;
	}

	if (wcsicmp(strType, L"CheckDir") == 0) {
		WXCTSTR wszDir = WXGetGlobalString(L"Dir");
		if (wcslen(wszDir) == 0) {
			return 0;
		}
		else {
			WXString strDir = wszDir;
			strDir += L"\\test.dir.temp";
			std::ofstream of(strDir.str()/*strFile.str()*/);
			if (of.is_open()) {
				of.close();
				//REMOVE(strDir.str()/*strFile.str()*/);
				WXLogW(L"%ws %ws can create file !!", __FUNCTIONW__, strDir.str());
				return 1;
			}
			WXLogW(L"%ws %ws can not create file !!", __FUNCTIONW__, strDir.str());
			return 0;
		}
	}

	return WXMediaGlobalValue::GetInt()[str];
}


EXTERN_C void     WXSetGlobalString(WXCTSTR strType, WXCTSTR strValue) {
	//WXLogW(L"WXSetGlobalValue[%ws]=%ws", strType, strValue);
	if (wcsicmp(strType, L"MediaPlayer") == 0) {
		if (wcsicmp(strValue, L"Lav") == 0) {
			g_nForceLav = 1;
		}
		else if (wcsicmp(strValue, L"Ffplay") == 0) {
			g_nForceLav = 0;
		}
		return;
	}

	if (wcsicmp(strType, L"SoundMode") == 0) {
		if (wcsicmp(strValue, L"None") == 0) {
			g_nSoundMode = SOUND_MODE_NONE;
		}
		else 	if (wcsicmp(strValue, L"Max") == 0) {
			g_nSoundMode = SOUND_MODE_MAX;
		}
		else	if (wcsicmp(strValue, L"Avg") == 0) {
			g_nSoundMode = SOUND_MODE_AVG;
		}
		else	if (wcsicmp(strValue, L"Left") == 0) {
			g_nSoundMode = SOUND_MODE_LEFT;
		}
		else	if (wcsicmp(strValue, L"Right") == 0) {
			g_nSoundMode = SOUND_MODE_RIGHT;
		}
		return;
	}

	std::wstring str = strType;
	if (strValue) {
		WXMediaGlobalValue::GetString()[str] = strValue;
	}
	else {
		WXMediaGlobalValue::GetString()[str] = L"";
	}
}


EXTERN_C WXCTSTR  WXGetGlobalString(WXCTSTR strType) {
	std::wstring str = strType;
	if (WXMediaGlobalValue::GetString().count(str) == 0) {
		WXMediaGlobalValue::GetString()[str] = L"";
	}
	return WXMediaGlobalValue::GetString()[str].c_str();
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
	//先屏蔽这个功能，使用默认配置
	int nLevel = LEVEL_BETTER;
	if (level < 0 || level > 3) { //默认配置
		int memory = WXGetMemory();//获取内存大小，单位G
		int cpu_num = WXGetCpuNum();//获取CPU数量，双核，4核，6核，
		int cpu_speed = WXGetCpuSpeed();//获取cpu速度，MHz
		if (cpu_num <= 4) {
			nLevel = LEVEL_GOOD;
		}
		else if (cpu_num < 6) {
			nLevel = LEVEL_BETTER;
		}
		else {
			nLevel = LEVEL_BEST;
		}
		WXLogW(L"%ws SetLevel [%d] To [%d]", level, nLevel);
	}
	else {
		level = MIN(3, MAX(level, 1));//强制设置
		WXLogW(L"%ws SetLevel [%d] To [%d]", level, nLevel);
	}
	WXSetGlobalValue(L"MachLevel", nLevel);//机器性能
}