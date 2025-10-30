/*压缩宝、视频压缩*/
#ifdef _WIN32
#include "..\WXFfmpeg\FfmpegExe.h"
#include <WXMediaCpp.h>

//计算压缩后视频的大小

class WXMediaCompress {
public:
	WXLocker m_mutex;
	WXString m_strInput;//输入

	//文件解析
	int64_t m_nFileTime = 0;
	int64_t m_nFileSize = 0;

	int m_hasVideo = 0;//是否有视频
	int m_iWidth = 0;
	int m_iHeight = 0;
	int64_t m_nVideoBitrate = 0;

	int m_hasAudio = 0; //是否有音频
	int64_t m_nAudioBitrate = 0;
	double m_Fps = 0.0;//文件帧率
	double m_AvgFps = 0.0;//源文件的平均帧率

	FfmpegExe m_Exe;
	FfmpegExe m_Exe2;

	BOOL  m_bH264 = FALSE;//强制H264+AAC输出！！

	int m_nTime = 0;//限制编码长度
public:
	void ParserInput(WXCTSTR wszInput) { //解析文件，获得时间长度和编码器类型
		WXAutoLock al(m_mutex);
		m_strInput = wszInput;
		int error = 0;
		void *info = WXMediaInfoCreateFast(m_strInput.str(), &error);
		if (info) {
			m_nFileSize = WXMediaInfoGetFileSize(info);
			m_nFileTime = WXMediaInfoGetFileDuration(info) / 1000;//视频长度，单位为秒
			if (WXMediaInfoHasVideo(info)) {
				m_iWidth  = WXMediaInfoGetVideoWidth(info);//原视频宽度
				m_iHeight = WXMediaInfoGetVideoHeight(info);//视频高度
				m_AvgFps  = WXMediaInfoGetVideoAvgFps(info);//视频帧率
				m_hasVideo = 1;
			}
			if (WXMediaInfoHasAudio(info)) {
				m_hasAudio = 1;
			}
			WXMediaInfoDestroy(info);
			if (!m_hasAudio && !m_hasVideo) {
				m_nFileTime = 0;
				m_nFileSize = 0;
			}
			else {
				int length = m_strInput.length();
				int pos = 0;
				for (int i = length - 1; i >= 0; i--) {
					if (wszInput[i] == L'.') {
						pos = i;
						break;
					}
				}
				WXString strExt = m_strInput.Left(length - pos - 1); //后缀名
				if (wcsicmp(strExt.str(), L"ts") == 0 ||
					wcsicmp(strExt.str(), L"flv") == 0 ||
					wcsicmp(strExt.str(), L"mp4") == 0
					) {
					m_bH264 = TRUE;
				}
			}
		}
	}

	int64_t GetTargetSize(int mode, int fps) {

		WXAutoLock al(m_mutex);
		if (m_nFileTime <= 0) {
			WXLogW(L"m_Duration=%lld", m_nFileTime);
			return 0;
		}

		m_Fps = fps; 
		m_nAudioBitrate = 0;
		m_nVideoBitrate = 0;

		int64_t OldnBitrate = m_nFileSize * 8 / m_nFileTime / 1000; //预估的整体码率， 单位kbps

		int64_t TargetSize = 0;

		if (mode == MODE_FAST) {
			TargetSize = m_nFileSize * 4 / 10;
		}else if (mode == MODE_NORMAL) {
			TargetSize = m_nFileSize * 6 / 10;
		}else if (mode == MODE_BEST) {
			TargetSize = m_nFileSize * 8 / 10;
		}
		int64_t nBitrate = TargetSize * 8 / m_nFileTime / 1000; //预估的整体码率， 单位kbps

		if (m_hasVideo) {
			if (nBitrate < 500 && m_hasAudio) {
				m_nAudioBitrate = 64;
			}else if (nBitrate < 1000 && m_hasAudio) {
				m_nAudioBitrate = 96;
			}else {
				m_nAudioBitrate = 128;
			}
			m_nVideoBitrate = nBitrate - m_nAudioBitrate;
		}else {
			m_nAudioBitrate = nBitrate;
		}
		return TargetSize;
	}

	//-----------  输入原来视频的编码参数 ---------------------------
	int64_t GetTargetSize2(int oldMode, int fps, int oldBitrate, int newMode) {
		return  GetTargetSize(newMode, fps);
	}

	int Process2(WXCTSTR wszOutput, int oldMode, int fps, int oldBitrate, int newMode) { //转换过程
		return Process(wszOutput, newMode, fps);
	}

	void SetTime(int second) {
		m_nTime = second;
	}


	int Processpass_normal(WXCTSTR wszOutput, int mode, int fps) {
		GetTargetSize(mode, fps);
#define MAX_ARGC 30

		WXString argw[MAX_ARGC];
		int argc = 0;

		argw[argc++].Format("ffmpeg");
		argw[argc++].Format("-i");
		argw[argc++].Format(L"%ws", m_strInput.w_str());

		if (m_nTime) {
			argw[argc++].Format("-t");
			argw[argc++].Format("%d", m_nTime);
		}

		if (m_hasVideo) { //视频编码
			
			argw[argc++].Format("-b:v");
			argw[argc++].Format("%dk", (int)m_nVideoBitrate);

			argw[argc++].Format("-r");
			if (fps == 0)
				argw[argc++].Format("%d", 24);
			else
				argw[argc++].Format("%d", fps);
		}
		
			argw[argc++].Format("-b:a");
			argw[argc++].Format("128k");

			WXString strOutput;
			strOutput.Format(wszOutput);
			argw[argc++].Format(L"%ws", strOutput.w_str());
			argw[argc++].Format("-y");
		


		char* argv[MAX_ARGC] = { nullptr };
		for (int i = 0; i < argc; i++) {
			argv[i] = strdup(argw[i].c_str());
			WXLogA("--- Argw[%d] = %s", i, argv[i]);
		}
		int ret = m_Exe.Process(argc, argv);
		for (int i = 0; i < argc; i++) {
			free(argv[i]);
		}
		return ret;
	}

	int Processpass1(WXCTSTR wszOutput, int mode, int fps) { 
		GetTargetSize(mode, fps);
#define MAX_ARGC 30

		WXString argw[MAX_ARGC];
		int argc = 0;

		argw[argc++].Format("ffmpeg");
		argw[argc++].Format("-i");
		argw[argc++].Format(L"%ws", m_strInput.w_str());

		if (m_nTime) {
			argw[argc++].Format("-t");
			argw[argc++].Format("%d", m_nTime);
		}

		if (m_hasVideo) { //视频编码
			//if (m_bH264) {
				argw[argc++].Format("-c:v");
				argw[argc++].Format("libx264");

				argw[argc++].Format("-tune");
				argw[argc++].Format("zerolatency");
			//}
			argw[argc++].Format("-b:v");
			argw[argc++].Format("%dk", (int)m_nVideoBitrate);

			argw[argc++].Format("-r");
			if(fps == 0)
				argw[argc++].Format("%d", 24);
			else
				argw[argc++].Format("%d", fps);
		}
		argw[argc++].Format("-pass");
		argw[argc++].Format("1");
		argw[argc++].Format("-an");
		argw[argc++].Format("-f");
		argw[argc++].Format("mp4");
		argw[argc++].Format("-y");
		argw[argc++].Format("NUL");
		
		
		char* argv[MAX_ARGC] = { nullptr };
		for (int i = 0; i < argc; i++) {
			argv[i] = strdup(argw[i].c_str());
			WXLogA("--- Argw[%d] = %s", i, argv[i]);
		}
		int ret = m_Exe.Process(argc, argv);
		for (int i = 0; i < argc; i++) {
			free(argv[i]);
		}
		return ret;
	}
	int Processpass2(WXCTSTR wszOutput, int mode, int fps) { 
		GetTargetSize(mode, fps);
#define MAX_ARGC 30

		WXString argw[MAX_ARGC];
		int argc = 0;

		argw[argc++].Format("ffmpeg");
		argw[argc++].Format("-i");
		argw[argc++].Format(L"%ws", m_strInput.w_str());

		if (m_nTime) {
			argw[argc++].Format("-t");
			argw[argc++].Format("%d", m_nTime);
		}

		argw[argc++].Format("-threads");
		argw[argc++].Format("0");
		if (m_hasVideo) { //视频编码
			/*if (m_bH264) {*/
				argw[argc++].Format("-c:v");
				argw[argc++].Format("libx264");

				argw[argc++].Format("-tune");
				argw[argc++].Format("zerolatency");
			//}
			argw[argc++].Format("-b:v");
			argw[argc++].Format("%dk", (int)m_nVideoBitrate);

			argw[argc++].Format("-r");
			if (fps == 0)
				argw[argc++].Format("%d", 24);
			else
				argw[argc++].Format("%d", fps);
		}
		argw[argc++].Format("-pass");
		argw[argc++].Format("2");
		argw[argc++].Format("-c:a");
		argw[argc++].Format("aac");
		argw[argc++].Format("-b:a");
		argw[argc++].Format("128k");
		WXString strOutput;
		strOutput.Format(wszOutput);
		argw[argc++].Format(L"%ws", strOutput.w_str());
		argw[argc++].Format("-y");

		char* argv[MAX_ARGC] = { nullptr };
		for (int i = 0; i < argc; i++) {
			argv[i] = strdup(argw[i].c_str());
			WXLogA("--- Argw[%d] = %s", i, argv[i]);
		}
		int ret = m_Exe2.Process(argc, argv);
		for (int i = 0; i < argc; i++) {
			free(argv[i]);
		}
		return ret;
	}

	int Process(WXCTSTR wszOutput, int mode, int fps) { //转换过程
		/*if (true||m_bH264)
		{*/
			Processpass1(wszOutput, mode, fps);
			return Processpass2(wszOutput, mode, fps);
		/*}
		else
		{
			return Processpass_normal(wszOutput, mode, fps);
		}*/
	}

	int GetProcess() {
		int64_t pts1 = (m_Exe.GetCurrTime()+ m_Exe2.GetCurrTime())/2;
		return pts1 * 100 / (m_nFileTime);
		return 0;
	}
};

WXMEDIA_API void* WXMediaConvertCreate(WXCTSTR wszFileName) {
	WXLogW(L"视频压缩 %ws Input FileName=[%ws]",__FUNCTIONW__,wszFileName);
	WXMediaCompress *conv = new WXMediaCompress;
	conv->ParserInput(wszFileName);
	return conv;
}

//设置视频压缩限制时间，0表示无限制
WXMEDIA_API void WXMediaConvertSetTime(void* p, int second) {
	if (p) {
		WXMediaCompress *conv = (WXMediaCompress*)p;
		return conv->SetTime(second);
	}
}

WXMEDIA_API int64_t WXMediaConvertGetTargetSize(void* p, int mode, int fps) {
	if (p) {
		WXMediaCompress *conv = (WXMediaCompress*)p;
		return conv->GetTargetSize(mode, fps);
	}
	return 0;
}


//转换函数
WXMEDIA_API int WXMediaConvertProcess(void* p, WXCTSTR wszOutFileName, int mode, int fps) {
	if (p) {
		
		WXLogW(L"视频压缩 %ws Output FileName=[%ws] threadID[%d]", __FUNCTIONW__, wszOutFileName, GetCurrentThreadId());
		WXMediaCompress *conv = (WXMediaCompress*)p;
		/*if (!conv->m_mutex.try_lock()) {
			WXLogW(L"视频压缩 %ws Output FileName=[%ws] threadID[%d] WXMediaCompress被占用，退出转换", __FUNCTIONW__, wszOutFileName, GetCurrentThreadId());
			return -1;
		}*/
		conv->GetTargetSize(mode, fps);
		int ret = conv->Process(wszOutFileName, mode, fps);
		return ret;
	}
	return 0;
}


WXMEDIA_API int64_t WXMediaConvertGetTargetSize2(void* p, int oldMode, int fps, int oldBitrate, int newMode) {
	if (p) {
		WXMediaCompress *conv = (WXMediaCompress*)p;
		return conv->GetTargetSize2(oldMode, fps, oldBitrate, newMode);
	}
	return 0;
}

//转换函数
WXMEDIA_API int WXMediaConvertProcess2(void* p, WXCTSTR wszOutFileName, int oldMode, int fps, int oldBitrate, int newMode) {
	if (p) {
		WXLogW(L"视频压缩 %ws Output FileName=[%ws]", __FUNCTIONW__, wszOutFileName);
		WXMediaCompress *conv = (WXMediaCompress*)p;
		conv->GetTargetSize(newMode, fps);
		int ret = conv->Process2(wszOutFileName, oldMode, fps, oldBitrate, newMode);
		return ret;
	}
	return 0;
}

//获取转换进度，应该和WXMediaConvertProcess在不同线程
WXMEDIA_API int WXMediaConvertGetProcess(void* p) {
	if (p) {
		WXMediaCompress *conv = (WXMediaCompress*)p;
		int ret = conv->GetProcess();
		return ret;
	}
	return 0;
}

WXMEDIA_API void WXMediaConvertProcessBreak(void* p) {
	if (p) {
		WXMediaCompress *conv = (WXMediaCompress*)p;
		conv->m_Exe.Exit();//中断退出
		conv->m_Exe2.Exit();//中断退出
	}
}

WXMEDIA_API void WXMediaConvertDestroy(void* p) {
	if (p) {
		WXMediaCompress *conv = (WXMediaCompress*)p;
		delete conv;
		p = nullptr;
	}
}

#endif