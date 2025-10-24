/*
ffmpeg 视频转换功能封装
首先要 执行初始化函数 WXMediaFfmpeg_Init()
如果对ffmpeg命令行属性，可以直接调用
WXMEDIA_API int64_t WXMediaFfmpeg_Convert(int argc, char **argv);
相当于 ffmpeg(argc,argv);
*/

#include <WXMediaCpp.h>

extern "C" {
	typedef struct WXCtx WXCtx;
	// in ffmpeg.c
	extern WXCtx* avffmpeg_create();
	extern void    avffmpeg_setEventOwner(WXCtx* octx, void* owner);
	extern void    avffmpeg_setRotate(WXCtx* octx, int rotate);//设置旋转角度
	extern void    avffmpeg_setEvent(WXCtx* octx, WXFfmpegOnEvent cb);

	extern void    avffmpeg_setVideoCb(WXCtx* octx, onFfmpegVideoData cb);

	extern void    avffmpeg_setEventID(WXCtx* octx, WXCTSTR szID);
	extern void    avffmpeg_set_video_encode_mode(WXCtx* octx, int mode);
	extern void    avffmpeg_destroy(WXCtx* octx);
	extern int     avffmpeg_convert(WXCtx* octx, int argc, char** argv);
	extern void    avffmpeg_interrupt(WXCtx* octx);
	extern int64_t avffmpeg_getCurrTime(WXCtx* octx);
	extern int64_t avffmpeg_getTotalTime(WXCtx* octx);
	extern int     avffmpeg_getState(WXCtx* octx);
}
class WXConvert {
	WXLocker m_mutex;

	enum { MAX_FFMPEG_ARGC = 50 };
	int  m_argc = 0;
	char* m_argv[MAX_FFMPEG_ARGC];

	WXString m_arrStrArgv[MAX_FFMPEG_ARGC];

	WXCtx* m_ctx = nullptr;


	//Convert
	int64_t   m_ptsStart = 0;  //-ss
	int64_t   m_ptsDuration = 0;  //-t
	int64_t   m_ptsCurr = 0;//

	int  m_iVodeoCodecMode = 1;// 0 Faset 1 Normal 2 Best
	int  m_iVolume = 256;//音量  0-1000 //默认音量256
	int  m_iRotate = 0;//旋转角度

	std::vector<WXString> m_arrInput;//inputs

	WXString  m_strVideoFmt = L"noset";// -pix_fmt yuv420p
	WXString  m_strVideoCodec = L"noset";// -vcodec libx264
	WXString  m_strAudioCodec = L"noset"; // -acodec aac

	double    m_fFps = 0.0;//
	int       m_iVideoBitrate = 0;  //-b:v
	int       m_iAudioBitrate = 0; // -b:a
	int       m_iAudioSampleRate = 0; // -ar 8000
	int       m_iAudioChannel = 0;  // -ac 1

	int       m_iSpeed = 100;//速度 0.5--2.0 //默认速度 1.0   100

	//亮度，对比度，饱和度
	int m_iBrightness = 0;
	int m_iContrast = 50;
	int m_iSaturation = 100;

	int m_iSrcWidth = 0;
	int m_iSrcHeight = 0;

	int m_iWidth = 0;
	int m_iHeight = 0;

	int m_iDARWidth = 0;
	int m_iDARHeight = 0;

	int m_iCropX = 0;
	int m_iCropY = 0;
	int m_iCropW = 0;
	int m_iCropH = 0;
	int m_bVFilp = 0;
	int m_bHFilp = 0;

	WXString m_strSubtitle = L"";
	WXString  m_strSubtitleFontName = L"";
	int       m_iSubtitleFontSize = 20;
	int       m_iSubtitleFontColor = 0xFFFFFF;
	int       m_iSubtitleFontAlpha = 0;
	int       m_iSubtitlePostion = 0;//0--270
	int       m_iAlignment = 2;//2 Bottom 10 center 6 Top

	struct WM_Data {
		WXString m_strImage = L""; //WaterMark Image
		int m_iWMPosX = 0;
		int m_iWMPosY = 0;
		int m_iWMWidth = 0;
		int m_iWMHeight = 0;
	};
	std::vector<WM_Data>m_arrWatermark;

	//水印filter
	void HandleWaterMark() {
		if (m_arrWatermark.size() == 0)return;

		WXString strScale = L"";
		WXString strWM = L"";
		int size = (int)m_arrWatermark.size();

		for (int i = 0; i < size; i++) {
			{
				WXString wxstr;
				wxstr.Format("[%d:v]scale=%d:%d[img%d]",
					i + 1, m_arrWatermark[i].m_iWMWidth, m_arrWatermark[i].m_iWMHeight, i + 1);
				strScale.Cat(wxstr, L";");
			}

			{
				WXString wxstr;
				if (i == 0 && size == 1) {
					wxstr.Format("[0:v][img1]overlay=%d:%d",
						m_arrWatermark[i].m_iWMPosX, m_arrWatermark[i].m_iWMPosY); //只有一个水印！！
				}
				else if (i == 0 && size != 1) {
					wxstr.Format("[0:v][img%d]overlay=%d:%d[bkg%d]",
						i + 1, m_arrWatermark[i].m_iWMPosX, m_arrWatermark[i].m_iWMPosY, i + 1); //多个水印中的第一个
				}
				else if (i != 0 && i != size - 1) {
					wxstr.Format("[bkg%d][img%d]overlay=%d:%d[bkg%d]",
						i, i + 1, m_arrWatermark[i].m_iWMPosX, m_arrWatermark[i].m_iWMPosY, i + 1); //多个水印，但不是最后一个水印
				}
				else if (i != 0 && i == size - 1) {
					wxstr.Format("[bkg%d][img%d]overlay=%d:%d",
						i, i + 1, m_arrWatermark[i].m_iWMPosX, m_arrWatermark[i].m_iWMPosY); //多个水印中最后一个水印
				}
				strWM.Cat(wxstr, L";");
			}
		}
		strScale.Cat(strWM, L";");
		m_strVideoFilter.Cat(strScale, L";");
	}

	//字幕filter
	void HandleSubtitle() {
		if (m_strSubtitle.length() != 0) {
			WXString wxstr;
			wxstr.Format("subtitles=%s", m_strSubtitle.c_str()); //需要UTF8
			WXString wxstrForce_Style;
			uint32_t color = (m_iSubtitleFontAlpha << 24) | m_iSubtitleFontColor;
			if (m_strSubtitleFontName.length() > 0) {
				wxstrForce_Style.Format(":force_style=\'FontName=%s,FontSize=%d,PrimaryColour=&H%08x&,MarginV=%d,Alignment=%d\'",
					m_strSubtitleFontName.c_str(), m_iSubtitleFontSize, color, m_iSubtitlePostion, m_iAlignment);
			}
			else if (m_strSubtitleFontName.length() == 0) {
				wxstrForce_Style.Format(":force_style=\'FontSize=%d,PrimaryColour=&H%08x&,MarginV=%d,Alignment=%d\'",
					m_iSubtitleFontSize, color, m_iSubtitlePostion, m_iAlignment);
			}
			wxstr += wxstrForce_Style;
			m_strVideoFilter.Cat(wxstr, L";");
		}
	}

	void HandleFilters() {
		HandleWaterMark();//水印处理

		if (m_iCropW > 0 && m_iCropH > 0) {
			WXString wxstr;
			wxstr.Format("crop=%d:%d:%d:%d", m_iCropW, m_iCropH, m_iCropX, m_iCropY);
			m_strVideoFilter.Cat(wxstr, L";");
			m_iWidth = m_iCropW;
			m_iHeight = m_iCropH;
			if (m_iRotate == 90 || m_iRotate == 270) {
				m_iWidth = m_iCropH;
				m_iHeight = m_iCropW;
			}
		}

		if (m_bVFilp) {
			WXString wxstr = L"vflip";//垂直旋转
			m_strVideoFilter.Cat(wxstr, L";");
		}

		if (m_bHFilp) {
			WXString wxstr = L"hflip";//垂直旋转
			m_strVideoFilter.Cat(wxstr, L";");
		}

		if (m_iRotate != 0) {
			if (m_iRotate == 90) {
				WXString wxstr = L"transpose=clock";
				m_strVideoFilter.Cat(wxstr, L",");
			}
			else if (m_iRotate == 270) {
				WXString wxstr = L"transpose=cclock";
				m_strVideoFilter.Cat(wxstr, L",");
			}
			else {
				WXString wxstr;
				wxstr.Format("rotate=%d*PI/180", m_iRotate);
				m_strVideoFilter.Cat(wxstr, L",");
			}
		}

		if (m_iSpeed != 100) { //播放速度
			m_strAudioFilter.Format("atempo=%0.3f", m_iSpeed / 100.0);
			WXString wxstr;
			wxstr.Format("setpts=%0.3f*PTS", 100.0 / m_iSpeed);
			m_strVideoFilter.Cat(wxstr, L",");
		}

		if (m_iBrightness != 0 || m_iContrast != 50 || m_iSaturation != 100) { //亮度、对比度、饱和度
			WXString wxstr;
			wxstr.Format("eq=brightness=%0.2f:contrast=%0.2f:saturation=%0.2f",
				m_iBrightness / 100.0f, m_iContrast / 50.0, m_iSaturation / 100.0f);
			m_strVideoFilter.Cat(wxstr, L",");
		}

		HandleSubtitle();
	}

public:

	WXConvert() {
		WXAutoLock al(m_mutex);
		m_ctx = avffmpeg_create();
	}

	~WXConvert() {
		WXAutoLock al(m_mutex);
		if (m_ctx) {
			if (m_thread) {
				m_thread->join();
				delete m_thread;
				m_thread = nullptr;
			}
			avffmpeg_destroy(m_ctx);
			m_ctx = nullptr;
		}
	}

	int LogRet(int ret) {
		WXLogW(L"Convert Param");
		for (int i = 0; i < m_argc; i++) {
			WXLogW(L"argv[%d] = %ws", i, m_arrStrArgv[i].str());
		}
		WXString wxstr;
		wxstr.Format(WXFfmpegGetError(ret));
		WXLogW(L"WXConvert Result = %ws", wxstr.str());
		return ret;
	}

	int WX_GCD(int m, int n) {
		int r;
		while (n != 0)
		{
			r = (m >= n) ? (m - n) : m;
			m = n;
			n = r;
		}
		return m;
	}

public:
	std::thread* m_thread = nullptr;
	int Function(int async) {
		for (int i = 0; i < m_argc; i++)
			m_argv[i] = (char*)m_arrStrArgv[i].c_str();
		if (async) {
			m_thread = new std::thread(&WXConvert::ThreadFunction, this);//异步执行
		}
		else {
			return ThreadFunction();
		}
		return 0;
	}
	int ThreadFunction() {
		int ret = avffmpeg_convert(m_ctx, m_argc, m_argv);
		LogRet(ret);
		return ret;
	}
public:
	//H264+AAC--->MP4
	int MixerAV(WXCTSTR strVideo, WXCTSTR strAudio, WXCTSTR strMixer, int async) {
		WXAutoLock al(m_mutex);

		m_argc = 0;
		m_arrStrArgv[m_argc++].Format("ffmpeg");
		m_arrStrArgv[m_argc++].Format("-i");
		m_arrStrArgv[m_argc++].Format(strVideo);
		m_arrStrArgv[m_argc++].Format("-i");
		m_arrStrArgv[m_argc++].Format(strAudio);
		m_arrStrArgv[m_argc++].Format("-c");
		m_arrStrArgv[m_argc++].Format("copy");
		m_arrStrArgv[m_argc++].Format(strMixer);
		return Function(async);
	}

	int ReplaceAudio(WXCTSTR strVideo, WXCTSTR strAudio, WXCTSTR strMixer, int copy, int async) {
		WXAutoLock al(m_mutex);
		m_argc = 0;
		m_arrStrArgv[m_argc++].Format("ffmpeg");
		m_arrStrArgv[m_argc++].Format("-i");
		m_arrStrArgv[m_argc++].Format(strVideo);
		m_arrStrArgv[m_argc++].Format("-i");
		m_arrStrArgv[m_argc++].Format(strAudio);
		m_arrStrArgv[m_argc++].Format("-map");
		m_arrStrArgv[m_argc++].Format("0:v");
		m_arrStrArgv[m_argc++].Format("-map");
		m_arrStrArgv[m_argc++].Format("1:a");


		if (copy) {
			m_arrStrArgv[m_argc++].Format("-c");
			m_arrStrArgv[m_argc++].Format("copy");
		}
		m_arrStrArgv[m_argc++].Format(strMixer);
		return Function(async);
	}

	//文件裁剪
	int CutFile2(WXCTSTR strInput, WXCTSTR strOutput,
		int64_t ptsStart, int64_t ptsDuration, int width, int height) {
		WXAutoLock al(m_mutex);
		m_argc = 0;
		m_arrStrArgv[m_argc++].Format("ffmpeg");
		m_arrStrArgv[m_argc++].Format("-i");
		m_arrStrArgv[m_argc++].Format(strInput);

		if (ptsStart != 0 || ptsDuration != 0) {
			m_arrStrArgv[m_argc++].Format("-ss");
			m_arrStrArgv[m_argc++].Format("%0.2f", ptsStart / 1000.0f);
			m_arrStrArgv[m_argc++].Format("-t");
			m_arrStrArgv[m_argc++].Format("%0.2f", ptsDuration / 1000.0f);
		}

		if (width && height) {
			m_arrStrArgv[m_argc++].Format("-s");
			m_arrStrArgv[m_argc++].Format("%dx%d", width, height);
		}
		m_arrStrArgv[m_argc++].Format(strOutput);
		return Function(0);
	}

	//文件裁剪
	int CutFile(WXCTSTR strInput, WXCTSTR strOutput,
		int64_t ptsStart, int64_t ptsDuration, int Fast, int async) {
		WXAutoLock al(m_mutex);
		m_argc = 0;
		m_arrStrArgv[m_argc++].Format("ffmpeg");
		m_arrStrArgv[m_argc++].Format("-i");
		m_arrStrArgv[m_argc++].Format(strInput);

		if (ptsStart != 0 || ptsDuration != 0) {
			m_arrStrArgv[m_argc++].Format("-ss");
			m_arrStrArgv[m_argc++].Format("%0.2f", ptsStart / 1000.0f);
			m_arrStrArgv[m_argc++].Format("-t");
			m_arrStrArgv[m_argc++].Format("%0.2f", ptsDuration / 1000.0f);
		}

		if (Fast) {
			m_arrStrArgv[m_argc++].Format("-c");
			m_arrStrArgv[m_argc++].Format("copy");
		}
		m_arrStrArgv[m_argc++].Format(strOutput);
		return Function(async);
	}

	//视频格式转换
	int ConvertVideo(WXCTSTR strInput, WXCTSTR strOutput, int async) {
		WXAutoLock al(m_mutex);
		HandleFilters();

		WXString wsxtrInput;
		wsxtrInput.Format(strInput);
		WXString wxstrOutput;
		wxstrOutput.Format(strOutput);

		//后缀名判断
		WXString strExt = WXBase::GetFileNameSuffix(strOutput);

		if (WXStrcmp(strExt.str(), _T("3gp")) == 0) {//3gp 视频格式	H263+AMR_NB(8000 MOMO)
			bool bFindSize = false;
			if ((m_iWidth == 1408 && m_iHeight == 1152) ||
				(m_iWidth == 704 && m_iHeight == 576) ||
				(m_iWidth == 352 && m_iHeight == 288) ||
				(m_iWidth == 176 && m_iHeight == 144) ||
				(m_iWidth == 120 && m_iHeight == 96)) {
				bFindSize = true;//配置正确的分辨率
			}
			if (!bFindSize) {
				m_iWidth = 704;
				m_iHeight = 576;
			}
			m_strVideoCodec = L"h263";
			m_strAudioCodec = L"amr_nb";
			m_iAudioSampleRate = 8000;
			m_iAudioChannel = 1;
			//m_iVideoBitrate = 0;
			m_iAudioBitrate = 0;
		}
		m_argc = 0;
		m_arrStrArgv[m_argc++].Format("ffmpeg");

		if (m_ptsStart > 0 || m_ptsDuration > 0) { //-ss tsStart -t tsDuration
			m_arrStrArgv[m_argc++].Format("-ss");
			m_arrStrArgv[m_argc++].Format("%0.2f", m_ptsStart / 1000.0f);
			m_arrStrArgv[m_argc++].Format("-t");
			m_arrStrArgv[m_argc++].Format("%0.2f", m_ptsDuration / 1000.0f);
		}

		//输入文件
		m_arrStrArgv[m_argc++].Format("-i");
		m_arrStrArgv[m_argc++].Format(wsxtrInput.str());

		//水印图片
		if (m_arrWatermark.size() > 0) {
			for (int i = 0; i < m_arrWatermark.size(); i++) {
				if (m_arrWatermark[i].m_strImage.length() > 0) { //水印图片
					m_arrStrArgv[m_argc++].Format("-i");
					m_arrStrArgv[m_argc++].Format(m_arrWatermark[i].m_strImage.str());
				}
			}
		}

		if (WXStrcmp(strExt.str(), _T("dv")) == 0) { //dvvideo DV视频格式
			m_arrStrArgv[m_argc++].Format("-target");
			m_strVideoCodec == L"ntscdv" ? m_arrStrArgv[m_argc++].Format("ntsc-dv") : m_arrStrArgv[m_argc++].Format("pal-dv");
			m_arrStrArgv[m_argc++].Format("-aspect");
			m_arrStrArgv[m_argc++].Format("4:3");
			m_arrStrArgv[m_argc++].Format(strOutput);
		}
		else if (WXStrcmp(strExt.str(), _T("vob")) == 0) { //vob 视频格式
			m_arrStrArgv[m_argc++].Format("-target");
			m_arrStrArgv[m_argc++].Format("pal-dv");
			m_arrStrArgv[m_argc++].Format(wxstrOutput.str());
			if (m_iWidth == 720 && m_iHeight == 576) {
				m_arrStrArgv[m_argc++].Format("pal-dvd");
			}
			else  if (m_iWidth == 720 && m_iHeight == 480) {
				m_arrStrArgv[m_argc++].Format("ntsc-dvd");
			}
			else if (m_iWidth == 480 && m_iHeight == 576) {
				m_arrStrArgv[m_argc++].Format("pal-svcd");
			}
			else  if (m_iWidth == 480 && m_iHeight == 480) {
				m_arrStrArgv[m_argc++].Format("ntsc-svcd");
			}
			else if (m_iWidth == 352 && m_iHeight == 288) {
				m_arrStrArgv[m_argc++].Format("pal-vcd");
			}
			else  if (m_iWidth == 352 && m_iHeight == 240) {
				m_arrStrArgv[m_argc++].Format("ntsc-vcd");
			}
		}
		else {
			bool setSize = 0;
			if (m_iWidth != 0 && m_iHeight != 0) { //-s 352x288  分辨率设置
				setSize = 1;
				//设置DAR
				{
					WXString wxstr;
					wxstr.Format("setsar=sar=1/1");//设置输出SAR为1：1
					m_strVideoFilter.Cat(wxstr, L",");

					m_arrStrArgv[m_argc++].Format("-aspect");//设置输出比例为PAR，使得在MAC分辨率正常
					int GCD = WX_GCD(m_iWidth, m_iHeight);
					m_arrStrArgv[m_argc++].Format("%d:%d", m_iWidth / GCD, m_iHeight / GCD);

					if (m_iDARWidth && m_iDARHeight) { //设置DAR输出，裁剪输出窗口，填充黑边，使
						int deltaW = 0;
						int deltaH = 0;
						if (m_iDARWidth * m_iHeight < m_iDARHeight * m_iWidth) {
							deltaW = (m_iWidth - m_iDARWidth * m_iHeight / m_iDARHeight) / 2;
						}
						else {
							deltaH = (m_iHeight - m_iDARHeight * m_iWidth / m_iDARWidth) / 2;
						}
						WXString wxstr2;
						wxstr2.Format("scale=%d:%d,pad=%d:%d:%d:%d", m_iWidth - 2 * deltaW, m_iHeight - 2 * deltaH,
							m_iWidth, m_iHeight, deltaW, deltaH);//设置输出SAR为1：1
						m_strVideoFilter.Cat(wxstr2, L",");
					}
				}

				m_arrStrArgv[m_argc++].Format("-s");
				m_arrStrArgv[m_argc++].Format("%dx%d", m_iWidth, m_iHeight);
			}

			if (m_strVideoCodec != L"noset") { //-vcodec libx264
				m_arrStrArgv[m_argc++].Format("-vcodec");
				m_arrStrArgv[m_argc++].Format(m_strVideoCodec.str());
			}


			if (m_strVideoFmt != L"noset") { //-pix_fmt yuv420p
				m_arrStrArgv[m_argc++].Format("-pix_fmt");
				m_arrStrArgv[m_argc++].Format(m_strVideoFmt.str());

				if (setSize == 0) { //没有手动设置分辨率
					//保持了分辨率，但是奇数分辨率使用 -pix_fmt yuv420p 会报错！！
					int error_v = 0;
					void* info = WXMediaInfoCreate(strInput, &error_v);
					if (info) {
						m_iWidth = WXMediaInfoGetVideoWidth(info) / 2 * 2;
						m_iHeight = WXMediaInfoGetVideoHeight(info) / 2 * 2;
						WXMediaInfoDestroy(info);
					}
					m_iWidth = m_iWidth / 2 * 2;
					m_iHeight = m_iHeight / 2 * 2;
					m_arrStrArgv[m_argc++].Format("-s");
					m_arrStrArgv[m_argc++].Format("%dx%d", m_iWidth, m_iHeight);
				}
			}

			if (fabs(m_fFps - 0.0) > 1.0) { //-r 25
				m_arrStrArgv[m_argc++].Format("-r");
				m_arrStrArgv[m_argc++].Format("%02f", m_fFps);
			}

			if (m_iVideoBitrate != 0) { //b:v
				m_arrStrArgv[m_argc++].Format("-b:v");
				m_arrStrArgv[m_argc++].Format("%d", m_iVideoBitrate);
			}


			if (m_strAudioCodec != L"noset") { //-acodec aac
				m_arrStrArgv[m_argc++].Format("-acodec");
				m_arrStrArgv[m_argc++].Format(m_strAudioCodec.str());
			}

			if (m_iAudioSampleRate != 0) { //-ar 48000
				m_arrStrArgv[m_argc++].Format("-ar");
				m_arrStrArgv[m_argc++].Format("%d", m_iAudioSampleRate);
			}

			if (m_iAudioChannel != 0) { //-ac 2
				m_arrStrArgv[m_argc++].Format("-ac");
				m_arrStrArgv[m_argc++].Format("%d", m_iAudioChannel);
			}

			if (m_iAudioBitrate != 0) { //-ab 128k
				m_arrStrArgv[m_argc++].Format("-b:a");
				m_arrStrArgv[m_argc++].Format("%d", m_iAudioBitrate);
			}

			if (m_iVolume != 256) { //-vol 1000 默认音量256  或者用 -af volume=10db ?
				m_arrStrArgv[m_argc++].Format("-vol");
				m_arrStrArgv[m_argc++].Format("%d", m_iVolume);
			}

			if (m_strVideoFilter.length() != 0) {
				m_arrStrArgv[m_argc++].Format("-filter_complex");
				m_arrStrArgv[m_argc++].Format(m_strVideoFilter.str());
			}

			if (m_strAudioFilter.length() > 0) { // "-af"
				m_arrStrArgv[m_argc++].Format("-af");
				m_arrStrArgv[m_argc++].Format(m_strAudioFilter.str());
			}
			m_arrStrArgv[m_argc++].Format(wxstrOutput.str());
		}
		return Function(async);
	}


	int ConvertVideoFast(WXCTSTR strInput, WXCTSTR strOutput) {
		WXAutoLock al(m_mutex);
		m_argc = 0;
		m_arrStrArgv[m_argc++].Format("ffmpeg");
		m_arrStrArgv[m_argc++].Format("-i");
		m_arrStrArgv[m_argc++].Format(strInput);
		m_arrStrArgv[m_argc++].Format("-c");
		m_arrStrArgv[m_argc++].Format("copy");
		m_arrStrArgv[m_argc++].Format(strOutput);
		return Function(0);
	}

	//音频格式转换
	int ConvertAudio(WXCTSTR strInput, WXCTSTR strOutput, int async) {
		WXAutoLock al(m_mutex);

		//WXLogA("%s strInput=%s strOutput=%s", __FUNCTION__, strInput, strOutput);

		WXString wxstrOutput;
		wxstrOutput.Format(strOutput);

		//后缀名判断
		WXString strExt = WXBase::GetFileNameSuffix(strOutput);

		if (WXStrcmp(strExt.str(), _T("aac")) == 0 || WXStrcmp(strExt.str(), _T("mp3")) == 0) {
			m_iAudioBitrate = 48000;
			m_iAudioChannel = 2;
			if (m_iAudioBitrate == 0)
				m_iAudioBitrate = 128000;
		}


		if (WXStrcmp(strExt.str(), _T("3gp")) == 0) {//3gp 视频格式
			m_iAudioBitrate = 0;
			m_strAudioCodec = L"amr_nb";
			m_iAudioSampleRate = 8000;
			m_iAudioChannel = 1;
		}



		m_argc = 0;
		m_arrStrArgv[m_argc++].Format("ffmpeg");

		if (m_ptsStart > 0 || m_ptsDuration > 0) { //-ss tsStart -t tsDuration
			m_arrStrArgv[m_argc++].Format("-ss");
			m_arrStrArgv[m_argc++].Format("%0.2f", m_ptsStart / 1000.0f);
			m_arrStrArgv[m_argc++].Format("-t");
			m_arrStrArgv[m_argc++].Format("%0.2f", m_ptsDuration / 1000.0f);
		}

		//输入文件
		m_arrStrArgv[m_argc++].Format("-i");
		m_arrStrArgv[m_argc++].Format(strInput);

		m_arrStrArgv[m_argc++].Format("-vn");

		if (m_iSpeed != 100) { //播放速度
			m_strAudioFilter.Format("atempo=%0.3f", m_iSpeed / 100.0);
			m_arrStrArgv[m_argc++].Format("-af");
			m_arrStrArgv[m_argc++].Format(m_strAudioFilter.str());
		}

		if (WXStrcmp(strExt.str(), _T("dts")) == 0) {//DTS 音频格式
			m_arrStrArgv[m_argc++].Format("-strict");
			m_arrStrArgv[m_argc++].Format("-2");
			if (m_iAudioBitrate == 0) {
				m_arrStrArgv[m_argc++].Format("-b:a");
				m_arrStrArgv[m_argc++].Format("%d", m_iAudioBitrate);
			}
		}
		else {//某些纯音频格式
			if (m_iAudioSampleRate != 0) { //-ar 48000
				m_arrStrArgv[m_argc++].Format("-ar");
				m_arrStrArgv[m_argc++].Format("%d", m_iAudioSampleRate);
			}
			if (m_iAudioChannel != 0) { //-ac 2
				m_arrStrArgv[m_argc++].Format("-ac");
				m_arrStrArgv[m_argc++].Format("%d", m_iAudioChannel);
			}
			if (WXStrcmp(strExt.str(), _T("flac")) &&
				WXStrcmp(strExt.str(), _T("wav")) &&
				WXStrcmp(strExt.str(), _T("aiff"))) {
				if (m_iAudioBitrate != 0) {
					m_arrStrArgv[m_argc++].Format("-b:a");
					m_arrStrArgv[m_argc++].Format("%d", m_iAudioBitrate);
				}
			}
		}
		m_arrStrArgv[m_argc++].Format(wxstrOutput.str());
		return Function(async);
	}

	// 多个文件依次合并
	int MergerFile(WXCTSTR strOutput, WXCTSTR strTemp, int fast, int async) {  //合并文件，多输入，设置临时文件，输出文件
		WXAutoLock al(m_mutex);
		WXString wxstrTemp;
		wxstrTemp.Format(strTemp);
		FILE* fout = _wfopen(strTemp, _T("wb"));//临时文件
		if (fout == nullptr) {
			wxstrTemp.Format("filelist.txt");
			fout = fopen(wxstrTemp.c_str(), "wb");
		}
		if (fout) {
			int num = (int)m_arrInput.size();
			for (int i = 0; i < num; i++) {
				char sz[1024];
#ifdef _WIN32
				sprintf(sz, "file '%ws'\n", m_arrInput[i].str());
#else
				sprintf(sz, "file '%s'\n", m_arrInput[i].c_str());
#endif
				fwrite(sz, strlen(sz), 1, fout);
			}
			fclose(fout);
			m_argc = 0;
			m_arrStrArgv[m_argc++].Format("ffmpeg");
			m_arrStrArgv[m_argc++].Format("-f");
			m_arrStrArgv[m_argc++].Format("concat");
			m_arrStrArgv[m_argc++].Format("-safe");
			m_arrStrArgv[m_argc++].Format("0");
			m_arrStrArgv[m_argc++].Format("-i");
			m_arrStrArgv[m_argc++].Format(wxstrTemp.str());

			if (fast) {
				m_arrStrArgv[m_argc++].Format("-c");
				m_arrStrArgv[m_argc++].Format("copy");
			}

			m_arrStrArgv[m_argc++].Format(strOutput);
			return Function(async);
		}
		else {
			WXLogW(L"Create Temp File Error %ws", wxstrTemp.str());
			return -1;
		}
		return 0;
	}

	// 截图操作
	int ShotPicture(WXCTSTR strInput, int64_t ts, WXCTSTR strOutput) {
		WXAutoLock al(m_mutex);

		WXString wxstrOutput;
		wxstrOutput.Format(strOutput);
		m_argc = 0;
		m_arrStrArgv[m_argc++].Format("ffmpeg");
		m_arrStrArgv[m_argc++].Format("-ss");
		m_arrStrArgv[m_argc++].Format("%f", (double)(ts) / 1000.0);//传入毫秒
		m_arrStrArgv[m_argc++].Format("-i");
		m_arrStrArgv[m_argc++].Format(strInput);//输入文件
		m_arrStrArgv[m_argc++].Format(wxstrOutput.str());//输出文件
		m_arrStrArgv[m_argc++].Format("-r");
		m_arrStrArgv[m_argc++].Format("1");
		m_arrStrArgv[m_argc++].Format("-vframes");
		m_arrStrArgv[m_argc++].Format("1");
		m_arrStrArgv[m_argc++].Format("-an");
		m_arrStrArgv[m_argc++].Format("-f");
		m_arrStrArgv[m_argc++].Format("mjpeg");

		for (int i = 0; i < m_argc; i++)
			m_argv[i] = (char*)m_arrStrArgv[i].c_str();
		int ret = avffmpeg_convert(m_ctx, m_argc, m_argv);
		LogRet(ret);
		if (ret == FFMPEG_ERROR_OK) {
			struct _stat64 statFile;
			int ret_stat = _wstat64(strOutput, &statFile);
			if (ret_stat == 0) {
				if (statFile.st_size <= 0) {
					WXLogW(L"Convert Result = %ws", WXFfmpegGetError(FFMPEG_ERROR_EMPTYFILE));
					return FFMPEG_ERROR_EMPTYFILE;
				}
				else {
					return FFMPEG_ERROR_OK;//OK
				}
			}
			else {
				WXLogW(L"ShotPicture Result = %ws", WXFfmpegGetError(FFMPEG_ERROR_NOFILE));
				return FFMPEG_ERROR_NOFILE;//No output file
			}
		}
		return ret;
	}

public: //Set Param
	//设置裁剪区域
	void SetCrop(int x, int y, int w, int h) {
		WXAutoLock al(m_mutex);
		//设置裁剪尺寸， 首先设置尺寸
		if (x < 0 || y < 0 || w <= 0 || h <= 0)
			return;
		m_iCropX = x;
		m_iCropY = y;
		m_iCropW = w;
		m_iCropH = h;
	}

	//垂直翻转
	void SetVFlip(int b) {
		WXAutoLock al(m_mutex);
		m_bVFilp = b;
	}

	//水平翻转
	void SetHFlip(int b) {
		WXAutoLock al(m_mutex);
		m_bHFilp = b;
	}

	//速度
	void SetSpeed(float speed) {
		WXAutoLock al(m_mutex);
		m_iSpeed = av_clip(speed, 50, 200);
	}


	void SetPictureQuality(int brightness, int contrast, int saturation) { //亮度,对比度,饱和度 0 50 100
		WXAutoLock al(m_mutex);
		m_iBrightness = av_clip(brightness, -100, 100);//0
		m_iContrast = av_clip(contrast, -100, 100);  // 50
		m_iSaturation = av_clip(saturation, 0, 300);    // 100
	}

	WXString m_strVideoFilter = L""; //水印
	WXString m_strAudioFilter = L""; //-af

	void AddWMImage(WXCTSTR szImage, int x = 0, int y = 0, int w = 0, int h = 0) {
		WXAutoLock al(m_mutex);
		WM_Data data;
		data.m_strImage.Format(szImage);
		data.m_iWMPosX = x;
		data.m_iWMPosY = y;
		data.m_iWMWidth = w;
		data.m_iWMHeight = h;
		m_arrWatermark.push_back(data);
	}

	void SetSubtitle(WXCTSTR wsz) {
		WXAutoLock al(m_mutex);
		if (WXStrlen(wsz) > 0) {
#ifdef _WIN32
			std::wstring temp = L"";
			for (int i = 0; i < WXStrlen(wsz); i++) {
				if (wsz[i] == L':')
					temp += L"\\\\:";
				else if (wsz[i] == '\\') {
					temp += L"\\\\\\\\";
				}
				else {
					temp += wsz[i];
				}
			}
			m_strSubtitle = temp.c_str();
#else
			m_strSubtitle.Format(wsz);
#endif
		}
		else {
			m_strSubtitle = L"";
		}
	}

	void SetSubtitleFont(WXCTSTR wsz, int FontSize, int FontColor) {
		WXAutoLock al(m_mutex);
		m_strSubtitleFontName = L"";
#ifdef _WIN32
		if (wsz != nullptr && WXStrlen(wsz) > 0)
			m_strSubtitleFontName = wsz;
#endif            
		m_iSubtitleFontSize = FontSize;
		m_iSubtitleFontColor = FontColor;
	}

	void SetSubtitleAlpha(int alpha) {
		WXAutoLock al(m_mutex);
		m_iSubtitleFontAlpha = av_clip_c(alpha, 0, 255);
	}

	void SetSubtitlePostion(int postion) {
		WXAutoLock al(m_mutex);
		m_iSubtitlePostion = postion;
	}

	void SetSubtitleAlignment(int alignment) {
		int Align = av_clip(alignment, 0, 2);
		if (Align == 0)
			m_iAlignment = 2;
		else if (Align == 1)
			m_iAlignment = 10;
		else
			m_iAlignment = 6;
	}

	void SetEventOwner(void* ownerEvent) {
		WXAutoLock al(m_mutex);
		avffmpeg_setEventOwner(m_ctx, ownerEvent);
	}

	void SetEventCb(WXFfmpegOnEvent cbEvent) {
		WXAutoLock al(m_mutex);
		avffmpeg_setEvent(m_ctx, cbEvent);
	}

	void SetEventID(WXCTSTR szEvent) {
		WXAutoLock al(m_mutex);
		avffmpeg_setEventID(m_ctx, szEvent);
	}

	void SetVolume(int volume) {
		WXAutoLock al(m_mutex);
		m_iVolume = av_clip(volume, 0, 1000);
	}

	void SetRoate(int rotate) {
		WXAutoLock al(m_mutex);
		m_iRotate = (rotate % 360 + 360) % 360;
	}

	void AddInput(WXCTSTR wszInput) {
		WXAutoLock al(m_mutex);
		WXString wxstr;
		wxstr.Format(wszInput);
		m_arrInput.push_back(wxstr);
	}

	void SetConvertTime(int64_t ptsStart, int64_t ptsDuration) {
		WXAutoLock al(m_mutex);
		m_ptsStart = ptsStart;
		m_ptsDuration = ptsDuration;
	}

	void SetVideoCB(onFfmpegVideoData cb) {
		WXAutoLock al(m_mutex);
		avffmpeg_setVideoCb(m_ctx, cb);//数据回调
	}

	void SetVideoFmtStr(WXCTSTR wsz) {
		WXAutoLock al(m_mutex);
		m_strVideoFmt.Format(wsz);
	}
	void SetVideoCodecStr(WXCTSTR wsz) {
		WXAutoLock al(m_mutex);
		m_strVideoCodec.Format(wsz);
		if (m_strVideoCodec == L"xvid")m_strVideoCodec = L"libxvid";
		if (m_strVideoCodec == L"ogv")m_strVideoCodec = L"libtheora";
		if (m_strVideoCodec == L"ogg")m_strVideoCodec = L"libtheora";
		if (m_strVideoCodec == L"vp8")m_strVideoCodec = L"libvpx";
		if (m_strVideoCodec == L"vp9")m_strVideoCodec = L"libvpx-vp9";
	}

	void SetVideoCodecMode(int mode) {
		WXAutoLock al(m_mutex);
		m_iVodeoCodecMode = av_clip(mode, 0, 2);
		avffmpeg_set_video_encode_mode(m_ctx, m_iVodeoCodecMode);
	}

	void SetVideoFps(double fps) {
		WXAutoLock al(m_mutex);
		m_fFps = fps;
	}

	void SetVideoSize(int width, int height) {
		WXAutoLock al(m_mutex);
		m_iWidth = width;
		m_iHeight = height;
	}

	void SetVideoDar(int dar_width, int dar_height) {
		WXAutoLock al(m_mutex);
		m_iDARWidth = dar_width;
		m_iDARHeight = dar_height;
	}

	void SetVideoBitrate(int bitrate) {
		WXAutoLock al(m_mutex);
		m_iVideoBitrate = bitrate;
		if (m_iVideoBitrate < 1000)
			m_iVideoBitrate *= 1000;
	}

	void SetAudioCodecStr(WXCTSTR wsz) {
		WXAutoLock al(m_mutex);
		m_strAudioCodec.Format(wsz);
	}

	void SetAudioBitrate(int bitrate) {
		WXAutoLock al(m_mutex);
		m_iAudioBitrate = bitrate;
		if (m_iAudioBitrate < 1000)
			m_iAudioBitrate *= 1000;
	}

	void SetAudioSampleRate(int sample_rate) {
		WXAutoLock al(m_mutex);
		m_iAudioSampleRate = sample_rate;
	}

	void SetAudioChannel(int channel) {
		WXAutoLock al(m_mutex);
		m_iAudioChannel = channel;
	}

	int64_t GetCurrTime() {
		return m_ptsCurr ? m_ptsCurr : avffmpeg_getCurrTime(m_ctx) * m_iSpeed / 100;
	}

	int64_t GetTotalTime() {
		return  m_ptsDuration ? m_ptsDuration : avffmpeg_getTotalTime(m_ctx);//设置了处理长度
	}

	int GetState() {
		return avffmpeg_getState(m_ctx);
	}

	void Interrupt() {
		avffmpeg_interrupt(m_ctx);
	}
};

WXMEDIA_API void WXFfmpegParamSetRotate(void* p, int rotate) {
	WXConvert* obj = (WXConvert*)p;
	if (obj) obj->SetRoate(rotate);
}

WXMEDIA_API void WXFfmpegParamSetVFlip(void* p, int b) {
	WXConvert* obj = (WXConvert*)p;
	if (obj) obj->SetVFlip(b);
}

WXMEDIA_API void WXFfmpegParamSetHFlip(void* p, int b) {
	WXConvert* obj = (WXConvert*)p;
	if (obj) obj->SetHFlip(b);
}

WXMEDIA_API void WXFfmpegParamSetCrop(void* p, int x, int y, int w, int h) {
	WXConvert* obj = (WXConvert*)p;
	if (obj) obj->SetCrop(x, y, w, h);
}

WXMEDIA_API void WXFfmpegParamSetSpeed(void* p, int speed) {
	WXConvert* obj = (WXConvert*)p;
	if (obj)obj->SetSpeed(speed);
}

WXMEDIA_API void WXFfmpegParamSetVolume(void* p, int volume) {
	WXConvert* obj = (WXConvert*)p;
	if (obj) obj->SetVolume(volume);
}

WXMEDIA_API void WXFfmpegParamSetPictureQuality(void* p, int brightness, int contrast, int saturation) { //亮度
	WXConvert* obj = (WXConvert*)p;
	if (obj)obj->SetPictureQuality(brightness, contrast, saturation);
}

WXMEDIA_API void* WXFfmpegParamCreate() {
	WXConvert* obj = new WXConvert;
	return (void*)obj;
}


WXMEDIA_API void WXFfmpegParamDestroy(void* p) {
	WXConvert* obj = (WXConvert*)p;
	if (obj)delete obj;
}

WXMEDIA_API void WXFfmpegParamAddWMImage(void* p, WXCTSTR wszImage, int x, int y, int w, int h) {
	WXConvert* obj = (WXConvert*)p;
	if (obj)obj->AddWMImage(wszImage, x, y, w, h);
}

WXMEDIA_API void WXFfmpegParamSetSubtitle(void* p, WXCTSTR sz) {
	WXConvert* obj = (WXConvert*)p;
	if (obj)obj->SetSubtitle(sz);
}

WXMEDIA_API void WXFfmpegParamSetSubtitleFont(void* p, WXCTSTR sz, int FontSize, int FontColor) {
	WXConvert* obj = (WXConvert*)p;
	if (obj)obj->SetSubtitleFont(sz, FontSize, FontColor);
}

WXMEDIA_API void WXFfmpegParamSetSubtitleAlpha(void* p, int alpha) {
	WXConvert* obj = (WXConvert*)p;
	if (obj)obj->SetSubtitleAlpha(alpha);
}

WXMEDIA_API void WXFfmpegParamSetSubtitlePostion(void* p, int postion) {
	WXConvert* obj = (WXConvert*)p;
	if (obj)obj->SetSubtitlePostion(postion);
}

WXMEDIA_API void WXFfmpegParamSetSubtitleAlignment(void* p, int alignment) {
	WXConvert* obj = (WXConvert*)p;
	if (obj)obj->SetSubtitleAlignment(alignment);
}

WXMEDIA_API void WXFfmpegParamSetEventOwner(void* p, void* ownerEvent) {
	WXConvert* obj = (WXConvert*)p;
	if (obj)obj->SetEventOwner(ownerEvent);
}

WXMEDIA_API void WXFfmpegParamSetEventCb(void* p, WXFfmpegOnEvent cbEvent) {
	WXConvert* obj = (WXConvert*)p;
	if (obj)obj->SetEventCb(cbEvent);
}

WXMEDIA_API void WXFfmpegParamSetEventID(void* p, WXCTSTR  szEvent) {
	WXConvert* obj = (WXConvert*)p;
	if (obj)obj->SetEventID(szEvent);
}

//if Merger File, Maybey has some SetInput By order
WXMEDIA_API void WXFfmpegParamAddInput(void* p, WXCTSTR szInput) {
	WXConvert* obj = (WXConvert*)p;
	if (obj)obj->AddInput(szInput);
}

WXMEDIA_API void WXFfmpegParamSetConvertTime(void* p, int64_t ptsStart, int64_t ptsDuration) {
	WXConvert* obj = (WXConvert*)p;
	if (obj) obj->SetConvertTime(ptsStart, ptsDuration);
}


//设置转换过程中编码前的回调函数
WXMEDIA_API void     WXFfmpegParamSetVideoCB(void* p, onFfmpegVideoData cb) {

	WXConvert* obj = (WXConvert*)p;
	if (obj) obj->SetVideoCB(cb);
}
//设置转换后的视频格式 yuv420p 。。
WXMEDIA_API void     WXFfmpegParamSetVideoFmtStr(void* p, WXCTSTR sz) {
	WXConvert* obj = (WXConvert*)p;
	if (obj) obj->SetVideoFmtStr(sz);
}

WXMEDIA_API void WXFfmpegParamSetVideoCodecStr(void* p, WXCTSTR sz) {
	WXConvert* obj = (WXConvert*)p;
	if (obj) obj->SetVideoCodecStr(sz);
}

WXMEDIA_API void  WXFfmpegParamSetVideoMode(void* p, int mode) {
	WXConvert* obj = (WXConvert*)p;
	if (obj) obj->SetVideoCodecMode(mode);
}

WXMEDIA_API void WXFfmpegParamSetVideoFps(void* p, double fps) {
	WXConvert* obj = (WXConvert*)p;
	if (obj) obj->SetVideoFps(fps);
}

WXMEDIA_API void WXFfmpegParamSetVideoSize(void* p, int width, int height) {
	WXConvert* obj = (WXConvert*)p;
	if (obj) obj->SetVideoSize(width, height);
}

WXMEDIA_API void WXFfmpegParamSetVideoDar(void* p, int dar_width, int dar_height) {
	WXConvert* obj = (WXConvert*)p;
	if (obj) obj->SetVideoDar(dar_width, dar_height);
}

WXMEDIA_API void WXFfmpegParamSetVideoBitrate(void* p, int bitrate) {
	WXConvert* obj = (WXConvert*)p;
	if (obj) obj->SetVideoBitrate(bitrate);
}

WXMEDIA_API void WXFfmpegParamSetAudioCodecStr(void* p, WXCTSTR sz) {
	WXConvert* obj = (WXConvert*)p;
	if (obj) obj->SetAudioCodecStr(sz);
}

WXMEDIA_API void WXFfmpegParamSetAudioBitrate(void* p, int bitrate) {
	WXConvert* obj = (WXConvert*)p;
	if (obj) obj->SetAudioBitrate(bitrate);
}

WXMEDIA_API void WXFfmpegParamSetAudioSampleRate(void* p, int sample_rate) {
	WXConvert* obj = (WXConvert*)p;
	if (obj) obj->SetAudioSampleRate(sample_rate);
}

WXMEDIA_API void WXFfmpegParamSetAudioChannel(void* p, int channel) {
	WXConvert* obj = (WXConvert*)p;
	if (obj) obj->SetAudioChannel(channel);
}


WXMEDIA_API int  WXFfmpegCutFile(void* p, WXCTSTR strInput, WXCTSTR strOutput,
	int64_t ptsStart, int64_t ptsDuration, int Fast, int async) {
	WXConvert* obj = (WXConvert*)p;
	return  obj ? obj->CutFile(strInput, strOutput, ptsStart, ptsDuration, Fast, async) : -1;
}

WXMEDIA_API int  WXFfmpegCutFile2(void* p, WXCTSTR strInput, WXCTSTR strOutput,
	int64_t ptsStart, int64_t ptsDuration, int width, int height) {
	WXConvert* obj = (WXConvert*)p;
	return  obj ? obj->CutFile2(strInput, strOutput, ptsStart, ptsDuration, width, height) : -1;
}


WXMEDIA_API int  WXFfmpegConvertVideo(void* p, WXCTSTR strInput, WXCTSTR strOutput, int async) {
	WXConvert* obj = (WXConvert*)p;
	return  obj ? obj->ConvertVideo(strInput, strOutput, async) : -1;
}

WXMEDIA_API int  WXFfmpegConvertAudio(void* p, WXCTSTR strInput, WXCTSTR strOutput, int async) {
	WXConvert* obj = (WXConvert*)p;
	return  obj ? obj->ConvertAudio(strInput, strOutput, async) : -1;
}

WXMEDIA_API int WXFfmpegMergerFile(void* p, WXCTSTR strOutput, WXCTSTR strTemp, int fast, int async) {
	WXConvert* obj = (WXConvert*)p;
	return obj ? obj->MergerFile(strOutput, strTemp, fast, async) : -1;
}

WXMEDIA_API int WXFfmpegMixerAV(void* p, WXCTSTR strAudio, WXCTSTR strVideo, WXCTSTR strMixer, int async) {
	WXConvert* obj = (WXConvert*)p;
	return obj ? obj->MixerAV(strVideo, strAudio, strMixer, async) : -1;
}

WXMEDIA_API int WXFfmpegReplaceAudio(void* p, WXCTSTR strVideo, WXCTSTR strAudio, WXCTSTR strOutput, int copy, int async) {
	WXConvert* obj = (WXConvert*)p;
	return obj ? obj->ReplaceAudio(strVideo, strAudio, strOutput, copy, async) : -1;
}

WXMEDIA_API int WXFfmpegShotPicture(void* p, WXCTSTR strInput, int64_t ts, WXCTSTR strOutput) {
	WXConvert* obj = (WXConvert*)p;
	return obj ? obj->ShotPicture(strInput, ts, strOutput) : -1;
}

WXMEDIA_API int64_t WXFfmpegGetCurrTime(void* p) {
	WXConvert* obj = (WXConvert*)p;
	return obj ? obj->GetCurrTime() : 0;
}

WXMEDIA_API int64_t WXFfmpegGetTotalTime(void* p) {
	WXConvert* obj = (WXConvert*)p;
	return obj ? obj->GetTotalTime() : 0;
}

WXMEDIA_API int WXFfmpegGetState(void* p) {
	WXConvert* obj = (WXConvert*)p;
	return obj ? obj->GetState() : -1;
}

WXMEDIA_API void WXFfmpegInterrupt(void* p) {
	WXConvert* obj = (WXConvert*)p;
	if (obj)obj->Interrupt();
}

WXMEDIA_API int  WXFfmpegConvertVideoFast(WXCTSTR strInput, WXCTSTR strOutput)
{
	WXConvert conv;
	return  conv.ConvertVideoFast(strInput, strOutput);
}
