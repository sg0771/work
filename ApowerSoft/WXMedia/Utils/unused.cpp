#include <WXMediaCpp.h>

//��Ļ �������֡���С����ɫ����
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

//����: ������������ʱ�л�����ģʽ
//����:
//nMode: ���պ�SOUND_MODE_*

//�Ƿ�ǿ��ʹ��LAV������
/*extern */int g_nForceLav = 0;

//��Ƶ˫����ģʽ
/*extern */int g_nSoundMode = SOUND_MODE_NONE;

//ȫ�ֲ�������
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

WXMEDIA_API void     WXSetGlobalValue(WXCTSTR strType, int nValue) {

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

WXMEDIA_API int      WXGetGlobalValue(WXCTSTR strType) {
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


WXMEDIA_API void     WXSetGlobalString(WXCTSTR strType, WXCTSTR strValue) {
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


WXMEDIA_API WXCTSTR  WXGetGlobalString(WXCTSTR strType) {
	std::wstring str = strType;
	if (WXMediaGlobalValue::GetString().count(str) == 0) {
		WXMediaGlobalValue::GetString()[str] = L"";
	}
	return WXMediaGlobalValue::GetString()[str].c_str();
}


//Patch For Airplay
WXMEDIA_API void WXSetRegionCallBack(void* ctx, WXRegionResetCallBack cb1, WXRegionDrawCallBack cb2) {}

//�����ӿ�
WXMEDIA_API void WXCaptureSetRegion(int left, int top, int width, int height) {}

WXMEDIA_API void WXSetSoundRenderJitterBuffer(int delay) {}

WXMEDIA_API int  WXGetSoundRenderJitterBuffer() {
	return 0;
}

WXMEDIA_API void WXLogWriteNew(const char* format, ...) {}
WXMEDIA_API void WXLogWriteNewW2(const wchar_t* format, ...) {}
WXMEDIA_API void WXLogWriteNewW(const wchar_t* format, ...) {}

//����VAD�ɼ����°��������
WXMEDIA_API  void WXCaptureSetVAD(int b) {}

WXMEDIA_API void WXMediaSetTemp(int b) {}

WXMEDIA_API int WXMediaGetTemp() {
	return 0;
}

WXMEDIA_API void WXMediaSaveParam(int bSave) {}

WXMEDIA_API void WXSetSystemSoundType(int i) {}

WXMEDIA_API void WXSetMicSoundType(int i) {}


//����һ֡��Ƶ�����ʱ����
WXMEDIA_API int64_t WXCaptureGetVideoTimeOut() {
	return 0;
}

WXMEDIA_API int64_t WXCaptureGetDuration() {
	return 0;
}

//static int s_iLevel = LEVEL_BETTER;
WXMEDIA_API void WXSetMachineLevel(int level) {

	return;//��Ի��������Զ�ѡ���ʺϵı��뻭��
	//������������ܣ�ʹ��Ĭ������
	int nLevel = LEVEL_BETTER;
	if (level < 0 || level > 3) { //Ĭ������
		int memory = WXGetMemory();//��ȡ�ڴ��С����λG
		int cpu_num = WXGetCpuNum();//��ȡCPU������˫�ˣ�4�ˣ�6�ˣ�
		int cpu_speed = WXGetCpuSpeed();//��ȡcpu�ٶȣ�MHz
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
		level = MIN(3, MAX(level, 1));//ǿ������
		WXLogW(L"%ws SetLevel [%d] To [%d]", level, nLevel);
	}
	WXSetGlobalValue(L"MachLevel", nLevel);//��������
}