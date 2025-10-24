/*
WXMedia 游戏录制类
*/

#include "WXGameCapture.h"
#include "WXMediaCpp.h"

std::vector<WXString>s_arrGameBlackList;

#define BLACK_LIST 71
static const TCHAR game_blacklisted_exes[BLACK_LIST][60] = {
	L"Video Editor Pro",
	L"VLC",
	L"Video Edit Pro",
	L"Apowersoft Video Editor Pro",
	L"Bee Cut",
	L"BeeCut",
	L"APOWERMIRROR",
	L"APOWEREDIT",
	L"APOWERREC",
	L"DEVENV",
	L"QQ",
	L"WangxuTechOA",
	L"explorer",
	L"steam",
	L"wmplay",
	L"winstore.app",
	L"GameLoader",
	L"TPHelper",
	L"TP2Helper",
	L"TP3Helper",
	L"Helper32",
	L"Helper64",
	L"tgp_daemon",
	L"tgp_launcher",
	L"tgp_reporter",
	L"tgp_render",
	L"tgp_browser",
	L"tgp_minibrowser",
	L"bugreport",
	L"BsSndRpt",
	L"screencapture",
	L"APOWER",
	L"DAEMON",
	L"REPORT",
	L"360AP",
	L"QQMusic",
	L"360se",
	L"pallas",
	L"360DesktopLite64",
	L"ComputerZTray",
	L"YY",
	L"yyexternal",
	L"wegame",
	L"seafile-applet",
	L"dwm",
	L"ScriptedSandbox64",
	L"steam",
	L"battle.net",
	L"galaxyclient",
	L"skype",
	L"uplay",
	L"origin",
	L"taskmgr",
	L"systemsettings",
	L"applicationframehost",
	L"cmd",
	L"dbgview",
	L"GameLoader",
	L"Helper32",
	L"Helper64",
	L"shellexperiencehost",
	L"searchui",
	L"firefox",
	L"DingTalk",
	L"notepad",
	L"notepad2",
	L"notepad++",
	L"QQEIM",
	L"chrome",
	L"DesktopMgr64",
	L"ntcls_daemon" //穿越火线
};

WXMEDIA_API BOOL WXBlackListedExe(WXCTSTR wszExe, int bGame) {
	if (wcsnicmp(L"Apowersoft", wszExe, 10) == 0)
		return true;

	for (int i = 0; i < s_arrGameBlackList.size(); i++) {
		if (wcsicmp(s_arrGameBlackList[i].str(), wszExe) == 0)
			return true;
	}
	return false;
}

//获取PID对应的EXE名字
WXMEDIA_API int WXGetProcessName(unsigned long processID, wchar_t * strName) {
	HANDLE handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, processID);
	if (handle) {
		TCHAR szPathName[MAX_PATH] = { 0 };
		DWORD dwLen = MAX_PATH;
		QueryFullProcessImageName(handle, 0, szPathName, &dwLen);
		CloseHandle(handle);

		TCHAR szFileName[MAX_PATH] = { 0 };
		_tsplitpath_s(szPathName,
			NULL, 0,
			NULL, 0,
			szFileName, _MAX_FNAME,
			NULL, 0);

		int length = (int)wcslen(szFileName);
		if (length) {
			wcscpy(strName, szFileName);
			return length;
		}
	}
	return 0;
}

static int  s_nTypeFps = 0;
static int  s_nModeFps = 0;

static bool  s_bWXGameInit = false;
static struct graphics_offsets offsets32 = { 0 };
static struct graphics_offsets offsets64 = { 0 };
static WXString s_strHelper64;
static WXString s_strHelper32;
static WXString s_strHook64 = L"help64.dll";
static WXString s_strHook32 = L"help32.dll";
static int64_t s_ptsLast = 0;

void WXGameInit() {

	if (s_bWXGameInit)return;

	int bWriteGameBlackList = WXGetGlobalValue(L"GameBlackList",  -1);
	if (bWriteGameBlackList) {
		WXSetGlobalValue(L"GameBlackList", 1);
		WXSetGlobalString(L"GameBlackList-Notify", L"\" Add BalckList As No=exeName \"");
		for (int i = 0; i < BLACK_LIST; i++) {
			WXString strNum;
			strNum.Format(L"GameBlackList-%d", i);
			WXString strValue;
			strValue.Format(L"%ws", game_blacklisted_exes[i]);
			WXSetGlobalString(strNum.str(), strValue.str());
		}
	}

	s_arrGameBlackList.clear();
	{
		int index = 0;
		while (1) {
			WXCHAR strValue[MAX_PATH];
			WXString wxstr;
			wxstr.Format(L"GameBlackList-%d", index);
			WXGetGlobalString(wxstr.str(), strValue,L"");
			if (wcslen(strValue) == 0) {
				break;
			}
			WXString strExe = strValue;
			s_arrGameBlackList.push_back(strExe);
			index++;
		}
	}

	HANDLE hShareMem = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, MEMSIZE, SHARED_OFFSETS);
	if (hShareMem == INVALID_HANDLE_VALUE || hShareMem == NULL) {
		hShareMem = OpenFileMapping(FILE_MAP_READ | FILE_MAP_WRITE, false, SHARED_OFFSETS);
	}

	uint8_t *pShareMem = nullptr;
	if ((hShareMem != INVALID_HANDLE_VALUE) && (hShareMem != NULL)) {
		pShareMem = (uint8_t *)MapViewOfFile(hShareMem, FILE_MAP_ALL_ACCESS, 0, 0, MEMSIZE);
		if (pShareMem) {
			memset(pShareMem, 0, MEMSIZE);
		}
	}
	if ((hShareMem != INVALID_HANDLE_VALUE) && (hShareMem != NULL)) {
		if (pShareMem) {
			TCHAR szFilePath[MAX_PATH + 1] = { 0 };
			GetModuleFileName(NULL, szFilePath, MAX_PATH);
			(_tcsrchr(szFilePath, L'\\'))[1] = 0;

			int size = sizeof(struct graphics_offsets);
			s_strHelper32 = szFilePath;
			s_strHelper32 += L"helper32.exe";
			WXGameExcute(s_strHelper32.c_str());
			memcpy(&offsets32, pShareMem, size);

			s_strHelper64 = szFilePath;
			s_strHelper64 += L"helper64.exe";
			WXGameExcute(s_strHelper64.c_str());
			memcpy(&offsets64, pShareMem + size, size);

			s_bWXGameInit = true;
			UnmapViewOfFile(pShareMem);
		}
		CloseHandle(hShareMem);
	}
}

class  DrawInfo  {
public:

	DrawInfo() {

	}
	WXLocker m_mutex;

    WXString m_strMsg;
    int  m_bChangeText = 0;//文字修改
    WXString m_strFont = L"宋体";
    int      m_iFontSize = 30;
    int m_colorText = 0x00FFFFFF;
    int m_colorBk = 0x00000000;
    int m_posxText = 0;
    int m_posyText = 0;
    void SetDrawColor(int colorText, int colorBk) {
        WXAutoLock al(m_mutex);
        m_colorText = colorText;
        m_colorBk = colorBk;
    }

    void SetDrawFont(WXCTSTR strFont, int fontSize) {
        WXAutoLock al(m_mutex);
        m_strFont = strFont;
        m_iFontSize = fontSize;
    }

	//type=0不显示文字信息
	//type=1在左上角显示文字信息
	//type=2在右上角显示文字信息
	//type=3在左下角显示文字信息
	//type=4在右下角显示文字信息
    void DrawString(WXCTSTR str, int x, int y, int type = 1) {
        WXAutoLock al(m_mutex);
        if (wcscmp(str, m_strMsg.str()) != 0 || m_posxImage != x || m_posyImage != y) {
            m_bChangeText = type;
            m_strMsg = str;
            m_posxText = x;
            m_posyText = y;
        }
    }

    WXString m_strImage;// = L"C:\\Users\\tam\\Desktop\\logo.png";
    int  m_bChangeImage = 0;//文字修改
    int m_posxImage = 0;
    int m_posyImage = 0;
    void DrawImage(WXCTSTR str, int x, int y) {
        WXAutoLock al(m_mutex);
        if (wcscmp(str, m_strImage.str()) != 0 || m_posxImage != x || m_posyImage != y) {
            m_bChangeImage = 1;
            m_strImage = str;
            m_posxImage = x;
            m_posyImage = y;
        }
    }


	int  m_bChangeCamera = 0;//摄像头画面修改
	int m_nPosXCamera = 0;
	int m_nPosYCamera = 0;

	//绘制摄像头图像
	BOOL m_bDrawCamera = FALSE;
	WXVideoFrame m_cameraYuvData;
	WXVideoFrame m_cameraRgbData;

	int m_nWidthCamera = 0;
	int m_nHeightCamera = 0;

	void DrawCamrea(AVFrame *frame, int x, int y, int w, int h) {
		WXAutoLock al(m_mutex);

		if (w != m_nWidthCamera || h != m_nHeightCamera) {
			m_cameraYuvData.Init(AV_PIX_FMT_YUV420P, w, h);
			m_cameraRgbData.Init(AV_PIX_FMT_RGB32, w, h);
			m_nWidthCamera = w;
			m_nHeightCamera = h;
		}

		if (frame && !m_bChangeCamera) {
			m_bDrawCamera = TRUE;
			m_nPosXCamera = x;
			m_nPosYCamera = y;
			libyuv::I420Scale(frame->data[0], frame->linesize[0],
				frame->data[1], frame->linesize[1],
				frame->data[2], frame->linesize[2],
				frame->width, frame->height,
				m_cameraYuvData.GetFrame()->data[0], m_cameraYuvData.GetFrame()->linesize[0],
				m_cameraYuvData.GetFrame()->data[1], m_cameraYuvData.GetFrame()->linesize[1],
				m_cameraYuvData.GetFrame()->data[2], m_cameraYuvData.GetFrame()->linesize[2],
				m_cameraYuvData.GetFrame()->width,   m_cameraYuvData.GetFrame()->height,
				libyuv::FilterMode::kFilterBilinear
			);
			libyuv::I420ToARGB(
				m_cameraYuvData.GetFrame()->data[0], m_cameraYuvData.GetFrame()->linesize[0],
				m_cameraYuvData.GetFrame()->data[1], m_cameraYuvData.GetFrame()->linesize[1],
				m_cameraYuvData.GetFrame()->data[2], m_cameraYuvData.GetFrame()->linesize[2],
				m_cameraRgbData.GetFrame()->data[0], m_cameraRgbData.GetFrame()->linesize[0],
				m_cameraYuvData.GetFrame()->width,   m_cameraYuvData.GetFrame()->height);
		}
		if (frame == NULL) {
			m_bDrawCamera = FALSE;
		}
	}
};
static DrawInfo s_info;

class WXGameCapture :public WXThread{
	WXLocker m_mutex;
    int m_nTime = 100;//采样间隔
    void GetDrawInfo() {
        if (m_global_hook_info) {

			//ApowerRec原来的逻辑
			//录屏的时候不写字,降低资源开销
			if (m_bOutput)
				m_global_hook_info->m_bCapture = true;
			else
				m_global_hook_info->m_bCapture = false;

			if (s_nTypeFps) { //设置了显示FPS强制显示
				m_global_hook_info->m_bCapture = false;
				m_global_hook_info->m_modeFPS = s_nModeFps;
			}

            m_global_hook_info->m_colorText = s_info.m_colorText;//文字颜色
            m_global_hook_info->m_colorBk = s_info.m_colorBk;//背景色
            if (s_info.m_strFont.length() > 0) { //字体处理
                memset(m_global_hook_info->m_strFont, 0, MAX_PATH * sizeof(wchar_t));
                wcscpy(m_global_hook_info->m_strFont, s_info.m_strFont.str());
                m_global_hook_info->m_iFontSize = s_info.m_iFontSize;//红色
            }else {
                memset(m_global_hook_info->m_strFont, 0, MAX_PATH * sizeof(wchar_t));
            }

            if (s_info.m_strMsg.length() > 0) { //输出文本
				m_global_hook_info->m_bDrawText = s_nTypeFps;// ? s_nTypeFps : (m_bOutput ? 0 : 1);
                m_global_hook_info->m_bChangeText = 1;//通知底层修改字符串
                m_global_hook_info->m_posxText = s_info.m_posxText;
                m_global_hook_info->m_posyText = s_info.m_posyText;
                wcscpy(m_global_hook_info->HookMsg, s_info.m_strMsg.str());
            }else {
                m_global_hook_info->m_bDrawText = 0;
                memset(m_global_hook_info->HookMsg, 0, MAX_PATH * sizeof(wchar_t));
            }

            if (s_info.m_strImage.length() > 0) { //图像和位置
                m_global_hook_info->m_bDrawImage = 1;
                m_global_hook_info->m_bChangeImage = 1;//通知底层修改字符串
                m_global_hook_info->m_posxImage = s_info.m_posxImage;
                m_global_hook_info->m_posyImage = s_info.m_posyImage;
                wcscpy(m_global_hook_info->m_HookImage, s_info.m_strImage.str());
            }else {
                m_global_hook_info->m_bDrawImage = 0;
                memset(m_global_hook_info->m_HookImage, 0, MAX_PATH * sizeof(wchar_t));
            }

			if (s_info.m_bDrawCamera) {
				m_global_hook_info->m_bDrawCamera = 1;
				m_global_hook_info->m_bChangeCamera = 1;
				m_global_hook_info->m_nPosXCamera = s_info.m_nPosXCamera;
				m_global_hook_info->m_nPosYCamera = s_info.m_nPosYCamera;
				m_global_hook_info->m_nWidthCamera = s_info.m_nWidthCamera;
				m_global_hook_info->m_nHeightCamera = s_info.m_nHeightCamera;
				memcpy(m_global_hook_info->m_pCamera, s_info.m_cameraRgbData.GetFrame()->data[0],
					s_info.m_cameraRgbData.GetFrame()->height * s_info.m_cameraRgbData.GetFrame()->linesize[0]);
			}else {
				m_global_hook_info->m_bDrawCamera = 0;
				m_global_hook_info->m_bChangeCamera = 0;
			}
        }
    }
public:
    std::atomic<int64_t> m_ptsLast = -1;//最后一帧时间戳

    int  GetHookType() {
        return m_iHookType;
    }

	int m_iHookType = -1;//Hook 类型

    int      m_cx = 0;  //width
    int      m_cy = 0;  //height
    uint32_t m_pitch = 0; //Pitch

	enum DXGI_FORMAT  m_dxgiFormat = DXGI_FORMAT_B8G8R8A8_UNORM;//默认RGB

    DWORD    m_idProcess = 0;  //对应exe的id
    DWORD    m_idThread = 0;  //对应exe的id
	DWORD    m_idCurr = 0;  //对应exe的id

    int      m_iMode = 1;//为1 时强制顶级窗口模式
	HWND     m_window = nullptr;//全屏模式 process_id thread_id 都是0

    bool     m_bInject = false;//注入成功
    bool     m_bHooked = false;//获取hook参数成功，可以开始抓取图像

    bool     process_is_64bit = false;
    bool     convert_16bit = false;
    bool     m_bApp = false;

    hook_info  *m_global_hook_info = nullptr;//和底层交互的数据

    HANDLE  keepalive_mutex = nullptr;
    HANDLE  hook_init = nullptr;
    HANDLE  hook_restart = nullptr;
    HANDLE  hook_stop = nullptr;
    HANDLE  hook_ready = nullptr;
    HANDLE  hook_exit = nullptr;
    HANDLE  hook_data_map = nullptr;
    HANDLE  global_hook_info_map = nullptr;
    HANDLE  target_process = nullptr;
	HANDLE  texture_mutexes[2] = {nullptr,nullptr};
	WXString m_strAppSid = L"";

    union {
        struct {
            struct shmem_data *shmem_data;
            uint8_t *texture_buffers[2];
        };
        uint8_t *m_mapViewData = nullptr;
    };

    enum gs_color_format m_videoFormat = GS_UNKNOWN;//回调数据格式

	int m_iWidth = 0;//4 的倍数
	int m_iHeight = 0;
	HWND m_hwndDisplay = 0;//图像预览
	BOOL m_bOutput = FALSE;//是否输出图像

	WXVideoFrame m_tempFrame;
	WXVideoFrame m_videoFrame; //本地缓存的RGB32 数据

    WXCursorCapture  m_cursor;//鼠标

    void *m_pSink = 0;//回调对象
    wxCallBack m_cbFunc = 0;//回调函数
    WXString m_strFileName;//对应EXE名字
    WXString m_strPathName;//对应EXE名字

    void DrawMouseImpl() {

        if (m_window) {
            m_cursor.Capture();
            if (!m_cursor.m_visible)return;

            int m_iMousePosX = m_cursor.cursor_pos.x;  //鼠标位置
            int m_iMousePosY = m_cursor.cursor_pos.y;

            RECT rc;
            GetWindowRect(m_window, &rc);

            m_iMousePosX -= (double)rc.left;
            m_iMousePosY -= (double)rc.top; //减去窗体的起始位置

            int sw = rc.right - rc.left;
            int sh = rc.bottom - rc.top;
            if (sw <= 0 || sh <= 0)return;
            double fWidth = (double)m_cx / (double)sw;
            double fHeight = (double)m_cy / (double)sh;
            m_iMousePosX *= fWidth;
            m_iMousePosY *= fHeight;//鼠标在窗口上的实际位置

            //绘制鼠标到录屏图像
            m_cursor.Draw(m_videoFrame.GetFrame(), m_iMousePosX, m_iMousePosY);
        }
    }

    AVFrame* GrabVideoData(int64_t pts) {
        if (!shmem_data)return  nullptr;
        int cur_texture = shmem_data->last_tex;
        if (cur_texture < 0 || cur_texture > 1)
            return  nullptr;
		{
			HANDLE mutex = NULL;
			int next_texture = cur_texture == 1 ? 0 : 1;
			if (object_signalled(texture_mutexes[cur_texture])) { //跨进程信号量
				mutex = texture_mutexes[cur_texture];
			}
			else if (object_signalled(texture_mutexes[next_texture])) {
				mutex = texture_mutexes[next_texture];
				cur_texture = next_texture;
			}
			else {
				return nullptr;
			}
			int K = convert_16bit ? 2 : 4;
			uint8_t *pSrc = texture_buffers[cur_texture];

			if (m_global_hook_info->flip) {
				for (int i = 0; i < m_cy; i++) {
					memcpy(m_tempFrame.GetFrame()->data[0] + i * m_cx * K,
						pSrc + (m_iHeight - 1 - i) * m_pitch,
						m_cx * K);
				}
			}else {
				for (int i = 0; i < m_cy; i++) {
					memcpy(m_tempFrame.GetFrame()->data[0] + i * m_cx * K,
						pSrc + i * m_pitch,
						m_cx * K);
				}
			}
			ReleaseMutex(mutex);//快速拷贝，编码底层堵塞
		}

		if (convert_16bit) { //RGB16 Format， 早期的D3D8格式
			if (m_global_hook_info->format == DXGI_FORMAT_B5G5R5A1_UNORM) {//RGB555
				libyuv::ARGB1555ToARGB(m_tempFrame.GetFrame()->data[0], m_cx * 2,
					m_videoFrame.GetFrame()->data[0], m_videoFrame.GetFrame()->linesize[0],
					m_iWidth, m_iHeight);
			}
			else if (m_global_hook_info->format == DXGI_FORMAT_B5G6R5_UNORM) {//RGB565
				libyuv::RGB565ToARGB(m_tempFrame.GetFrame()->data[0], m_cx * 2,
					m_videoFrame.GetFrame()->data[0], m_videoFrame.GetFrame()->linesize[0],
					m_iWidth, m_iHeight);
			}
		}else { //RGB32 Format 
			if (m_videoFormat == GS_BGRX || m_videoFormat == GS_BGRA) {//标准的RGB32图像数据
				libyuv::ARGBCopy(m_tempFrame.GetFrame()->data[0], m_tempFrame.GetFrame()->linesize[0],
					m_videoFrame.GetFrame()->data[0], m_videoFrame.GetFrame()->linesize[0],
					m_iWidth, m_iHeight);
			}else if (m_videoFormat == GS_RGBA) {  // 守望先锋， 和RGB32 颜色相反
				for (int i = 0; i < m_iHeight; i++) {
					for (int j = 0; j < m_iWidth; j++) {
						int posSrc = j * 4 + i * m_cx * 4;
						int posDst = j * 4 + i * m_iWidth * 4;
						m_videoFrame.GetFrame()->data[0][posDst]     = m_tempFrame.GetFrame()->data[0][posSrc + 2];
						m_videoFrame.GetFrame()->data[0][posDst + 1] = m_tempFrame.GetFrame()->data[0][posSrc + 1];
						m_videoFrame.GetFrame()->data[0][posDst + 2] = m_tempFrame.GetFrame()->data[0][posSrc];
						m_videoFrame.GetFrame()->data[0][posDst + 3] = m_tempFrame.GetFrame()->data[0][posSrc + 3];
					}
				}
			}else if (m_videoFormat == GS_R10G10B10A2) {  // 堡垒之夜！！！
				R10G10B10A2_To_RGB32(m_tempFrame.GetFrame()->data[0], m_tempFrame.GetFrame()->linesize[0],
					m_videoFrame.GetFrame()->data[0], m_videoFrame.GetFrame()->linesize[0],
					m_iWidth, m_iHeight);
			}
		}

		/*
		过滤黑色区域很多的画面，比如DNF的聊天窗口
		*/


		DrawMouseImpl(); //绘制鼠标


		m_ptsLast = timeGetTime();//视频时间戳

		if (!m_bOutput && m_hwndDisplay) {  //hook数据直接预览
			HDC hdc = ::GetDC(m_hwndDisplay);
			SetStretchBltMode(hdc, HALFTONE);//硬件缩放
			RECT rc;
			::GetClientRect(m_hwndDisplay, &rc);
			::StretchDIBits(hdc, 0, 0, rc.right, rc.bottom,
				0, 0, m_iWidth, m_iHeight, m_videoFrame.GetFrame()->data[0],
				(BITMAPINFO*)m_videoFrame.GetBIH(), DIB_RGB_COLORS, SRCCOPY);//画出倒的图片
			::ReleaseDC(m_hwndDisplay, hdc);
		}

		if (m_bOutput) { //输出到录制
			m_videoFrame.GetFrame()->pts = pts;
			return m_videoFrame.GetFrame();
		}
		return nullptr;
    }

	//hook线程
	void HookData() {

		GetDrawInfo();//获取绘图参数

		WXAutoLock al(m_mutex);

		//动态切换Hook窗口
		if (m_iMode == 1) { //没有输出的时候切换顶级窗口
			HWND hwnd = GetForegroundWindow(); //顶级窗口或者包含在顶级窗口的子窗口(狂野飙车)
			if (is_uwp_window(hwnd))
				hwnd = get_uwp_actual_window(hwnd);//嵌入其它进程的进程

			if (m_window != hwnd) { //顶级窗口已经切换

				//判断新的EXE 是否 黑名单
				DWORD idProcess = 0;
				DWORD idThread = GetWindowThreadProcessId(hwnd, &idProcess);

				if (idProcess != m_idCurr) { //不是录屏主程序
					HANDLE handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, idProcess);
					if (handle) { //获取名字，黑名单过滤
						TCHAR szPathName[MAX_PATH] = { 0 };
						DWORD dwLen = MAX_PATH;
						QueryFullProcessImageName(handle, 0, szPathName, &dwLen);//exe 全路径
						CloseHandle(handle);
						TCHAR szFileName[MAX_PATH] = { 0 };//EXE名字
						_tsplitpath_s(szPathName,
							NULL, 0,
							NULL, 0,
							szFileName, _MAX_FNAME,
							NULL, 0);
						if (!WXBlackListedExe(szFileName, TRUE)) { //不在录制黑名单里面
							m_strPathName = szPathName;//全路径
							m_strFileName = szFileName;//exe名字

							wxlog("Change Window  Hooking Exe is %s", m_strFileName.c_str());

							stop_capture();
							m_window = hwnd;  //切换Hook窗口
							m_idThread = idThread;
							m_idProcess = idProcess;
						}
					}
				}
			}
		}

		//指定窗口模式，目前用于测试
		if (m_idProcess == 0 && m_window != nullptr && m_iMode == 0) {
			m_idThread = GetWindowThreadProcessId(m_window, &m_idProcess);
		}

		if (m_idProcess == 0) {
			//wxlog("IdProcess is 0, Error");
			return;
		}

		if (!m_bInject && m_iMode != 0 && m_idProcess != m_idCurr) { //通过ProcessID查找名字匹配黑名单过滤

			HANDLE handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, m_idProcess);
			if (handle) { //获取名字，黑名单过滤
				TCHAR szPathName[MAX_PATH] = { 0 };
				DWORD dwLen = MAX_PATH;
				QueryFullProcessImageName(handle, 0, szPathName, &dwLen);//exe 全路径
				CloseHandle(handle);
				TCHAR szFileName[MAX_PATH] = { 0 };//EXE名字
				_tsplitpath_s(szPathName,
					NULL, 0,
					NULL, 0,
					szFileName, _MAX_FNAME,
					NULL, 0);
				if (!WXBlackListedExe(szFileName, TRUE)) {
					m_strPathName = szPathName;//全路径
					m_strFileName = szFileName;//exe名字
					wxlog("Hooking Exe is %s", m_strFileName.c_str());
				}
				else {
					return;
				}
			}
		}

		if (!m_bInject) {
			try_hook(); //注入hook.dll
			if (!m_bInject) { //输入失败
				return;
			}
		}

		//注入成功
		if (hook_stop && object_signalled(hook_stop)) {
			wxlog("%s hook stop", __FUNCTION__);
			stop_capture();
		}

		if (!hook_ready && m_idProcess) {
			hook_ready = open_event_gc(EVENT_HOOK_READY);
			if (hook_ready) {
				wxlog("%s hook ready!!!!", __FUNCTION__);
			}
		}

		if (hook_ready && !m_bHooked && m_global_hook_info && m_global_hook_info->cx) {
			enum capture_result result = init_capture_data();
			if (result == CAPTURE_SUCCESS) {
				m_bHooked = start_capture();
				wxlog("%s start_capture!!!!", __FUNCTION__);
			}

			if (result != CAPTURE_RETRY && !m_bHooked) {
				stop_capture();
				wxlog("%s stop_capture!!!!", __FUNCTION__);
			}
		}

		if (m_bHooked) {
			if (!capture_valid()) {
				stop_capture();
			}else {
				GrabVideoData(0);
			}
		}
	}

    AVFrame* GrabFrame() {

		int64_t pts = WXGetTimeMs();
		GetDrawInfo();//获取绘图参数

		WXAutoLock al(m_mutex);

		//动态切换Hook窗口
        if (!m_bOutput && m_iMode == 1) { //没有输出的时候切换顶级窗口
			HWND hwnd = GetForegroundWindow(); //顶级窗口或者包含在顶级窗口的子窗口(狂野飙车)
			if (is_uwp_window(hwnd))
				hwnd = get_uwp_actual_window(hwnd);//嵌入其它进程的进程

			if (m_window != hwnd) { //顶级窗口已经切换

				//判断新的EXE 是否 黑名单
				DWORD idProcess = 0;
				DWORD idThread = GetWindowThreadProcessId(hwnd, &idProcess);
				
				if (idProcess != m_idCurr) { //不是录屏主程序
					HANDLE handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, idProcess);
					if (handle) { //获取名字，黑名单过滤
						TCHAR szPathName[MAX_PATH] = { 0 };
						DWORD dwLen = MAX_PATH;
						QueryFullProcessImageName(handle, 0, szPathName, &dwLen);//exe 全路径
						CloseHandle(handle);
						TCHAR szFileName[MAX_PATH] = { 0 };//EXE名字
						_tsplitpath_s(szPathName,
							NULL, 0,
							NULL, 0,
							szFileName, _MAX_FNAME,
							NULL, 0);
						if (!WXBlackListedExe(szFileName, TRUE)) { //不在录制黑名单里面
							m_strPathName = szPathName;//全路径
							m_strFileName = szFileName;//exe名字
							wxlog("Change Window  Hooking Exe is %s", m_strFileName.c_str());

							stop_capture();
							m_window = hwnd;  //切换Hook窗口
							m_idThread = idThread;
							m_idProcess = idProcess;
						}
					}
				}
			}
        }

		//指定窗口模式，目前用于测试
		if (m_idProcess == 0 && m_window != nullptr && m_iMode == 0) { 
			m_idThread = GetWindowThreadProcessId(m_window, &m_idProcess);
		}

		if (m_idProcess == 0) {
			return nullptr;
		}

		if (!m_bOutput && !m_bInject && m_iMode != 0 && m_idProcess != m_idCurr) { //通过ProcessID查找名字匹配黑名单过滤

			HANDLE handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, m_idProcess);
			if (handle) { //获取名字，黑名单过滤
				TCHAR szPathName[MAX_PATH] = { 0 };
				DWORD dwLen = MAX_PATH;
				QueryFullProcessImageName(handle, 0, szPathName, &dwLen);//exe 全路径
				CloseHandle(handle);
				TCHAR szFileName[MAX_PATH] = { 0 };//EXE名字
				_tsplitpath_s(szPathName,
					NULL, 0,
					NULL, 0,
					szFileName, _MAX_FNAME,
					NULL, 0);
				if (!WXBlackListedExe(szFileName, TRUE)){
					m_strPathName = szPathName;//全路径
					m_strFileName = szFileName;//exe名字
					wxlog("  Hooking Exe is %s", m_strFileName.c_str());
				}else {
					return   nullptr;
				}
			}
		}

		if (!m_bInject) {
			try_hook(); //注入hook.dll
			if (!m_bInject) { //输入失败
				return  nullptr;
			}
		}

		//注入成功
        if (hook_stop && object_signalled(hook_stop)) {
			wxlog("%s hook stop", __FUNCTION__);
            stop_capture();
        }

        if (!hook_ready && m_idProcess) {
            hook_ready = open_event_gc(EVENT_HOOK_READY);
			if (hook_ready) {
				wxlog("%s hook ready!!!!", __FUNCTION__);
			}
        }

        if (hook_ready && !m_bHooked && m_global_hook_info->cx) {
            enum capture_result result = init_capture_data();
			if (result == CAPTURE_SUCCESS) {
				m_bHooked = start_capture();
				wxlog("%s start_capture!!!!", __FUNCTION__);
			}

            if (result != CAPTURE_RETRY && !m_bHooked) {
                stop_capture();
				wxlog("%s stop_capture!!!!", __FUNCTION__);
            }
        }

       if(m_bHooked) {
            if (!capture_valid()) {
                stop_capture();
            }else {
				return GrabVideoData(pts);
            }
        }
	   return nullptr;
    }

public:
    inline HANDLE open_mutex_plus_id(const wchar_t *name, DWORD id) {
        wchar_t new_name[64];
        _snwprintf(new_name, 64, L"%s%lu", name, id);
        return m_bApp
            ? open_app_mutex(m_strAppSid.str(), new_name)
            : open_mutex(new_name);
    }

    inline HANDLE open_mutex_gc(const wchar_t *name) {
        return open_mutex_plus_id(name, m_idProcess);
    }

    inline HANDLE open_event_plus_id(const wchar_t *name, DWORD id) {
        wchar_t new_name[64];
        _snwprintf(new_name, 64, L"%s%lu", name, id);
        return m_bApp
            ? open_app_event(m_strAppSid.str(), new_name)
            : open_event(new_name);
    }

    inline HANDLE open_event_gc(const wchar_t *name) {
        return open_event_plus_id(name, m_idProcess);
    }

    inline HANDLE open_map_plus_id(const wchar_t *name, DWORD id) {
        wchar_t new_name[64];
        _snwprintf(new_name, 64, L"%s%lu", name, id);
        return m_bApp
            ? open_app_map(m_strAppSid.str(), new_name)
            : OpenFileMappingW(FILE_MAP_READ | FILE_MAP_WRITE, false, new_name);
    }

    inline HANDLE open_hook_info() {
        return open_map_plus_id(SHMEM_HOOK_INFO, m_idProcess);
    }

    void stop_capture() {
		wxlog(" %s" ,__FUNCTION__);
        if (hook_stop) {
            SetEvent(hook_stop);
            hook_stop = NULL;
        }

        if (m_global_hook_info) { //
            m_global_hook_info->m_bDrawText = 0;
            m_global_hook_info->m_bDrawImage = 0;
            UnmapViewOfFile(m_global_hook_info);
            m_global_hook_info = NULL;
        }

        if (m_mapViewData) {
            UnmapViewOfFile(m_mapViewData);
			m_mapViewData = NULL;
        }

        if (keepalive_mutex) {
            close_handle(&keepalive_mutex);
            keepalive_mutex = NULL;
        }

        if (hook_restart) {
            close_handle(&hook_restart);
            hook_restart = NULL;
        }


        if (hook_stop) {
            close_handle(&hook_stop);
            hook_stop = NULL;
        }

        if (hook_ready) {
            close_handle(&hook_ready);
            hook_ready = NULL;
        }

        if (hook_exit) {
            close_handle(&hook_exit);
            hook_exit = NULL;
        }

        if (hook_init) {
            close_handle(&hook_init);
            hook_init = NULL;
        }

        if (hook_data_map) {
            close_handle(&hook_data_map);
            hook_data_map = NULL;
        }

        if (global_hook_info_map) {
            close_handle(&global_hook_info_map);
            global_hook_info_map = NULL;
        }

        if (target_process) {
            close_handle(&target_process);
            target_process = NULL;
        }

        if (texture_mutexes[0]) {
            close_handle(&texture_mutexes[0]);
            texture_mutexes[0] = NULL;
        }

        if (texture_mutexes[1]) {
            close_handle(&texture_mutexes[1]);
            texture_mutexes[1] = NULL;
        }

		m_cx = 0;
		m_cy = 0;
		m_iWidth = 0;
		m_iHeight = 0;
		m_mapViewData = NULL;
		shmem_data = NULL;
		texture_buffers[0] = NULL;
		texture_buffers[1] = NULL;
		m_bInject = false;
		m_bHooked = false;
    }

	void get_app_sid(HANDLE process){
		m_strAppSid = L"";
		HANDLE token = nullptr;
		if (OpenProcessToken(process, TOKEN_QUERY, &token)) {
			DWORD info_len = GetSidLengthRequired(12) + sizeof(TOKEN_APPCONTAINER_INFORMATION);
			PTOKEN_APPCONTAINER_INFORMATION info = (PTOKEN_APPCONTAINER_INFORMATION)malloc(info_len);
			DWORD size_ret = 0;
			BOOL success = GetTokenInformation(token, TokenAppContainerSid,
				info, info_len, &size_ret);
			if (success) {
				wchar_t *wszSid = NULL;
				ConvertSidToStringSidW(info->TokenAppContainer, &wszSid);
				free(info);
				CloseHandle(token);
				m_strAppSid = wszSid;
				wxlog("App SID is %s", m_strAppSid.c_str());
				LocalFree(wszSid);
			}
		}
	}

    inline bool open_target_process() {
        target_process = OpenProcess(PROCESS_QUERY_INFORMATION | SYNCHRONIZE, false, m_idProcess);
        if (!target_process) {
            return false;
        }
        process_is_64bit = is_64bit_process(target_process);
        m_bApp = is_app(target_process);
        if (m_bApp) {
            get_app_sid(target_process);
        }
        return true;
    }

    inline bool init_texture_mutexes() {
        texture_mutexes[0] = open_mutex_gc(MUTEX_TEXTURE1);
        texture_mutexes[1] = open_mutex_gc(MUTEX_TEXTURE2);
        if (!texture_mutexes[0] || !texture_mutexes[1]) {
            return false;
        }
        return true;
    }

    /* if there's already a hook in the process, then signal and start */
    inline bool attempt_existing_hook()
    {
        hook_restart = open_event_gc(EVENT_CAPTURE_RESTART);
        if (hook_restart) {
            SetEvent(hook_restart);
            return true;
        }
        return false;
    }

    inline bool init_hook_info() {
        global_hook_info_map = open_hook_info();
        if (!global_hook_info_map) {
            WXLogW(L"init_hook_info: get_hook_info failed: %lu", GetLastError());
            return false;
        }
        m_global_hook_info = (hook_info*)MapViewOfFile(global_hook_info_map,
            FILE_MAP_ALL_ACCESS, 0, 0,
            sizeof(hook_info) + MAX_CAMERA_WIDTH * MAX_CAMERA_HEIGHT * 4); //和底层通信的数据区域
        if (!m_global_hook_info) {
            WXLogW(L"init_hook_info: failed to map data view: %lu", GetLastError());
            return false;
        }

		if (process_is_64bit) {
			wxlog("copy offsets64");
			memcpy(&m_global_hook_info->offsets, &offsets64, sizeof(struct graphics_offsets));
		}else {
			wxlog("copy offsets32");
			memcpy(&m_global_hook_info->offsets, &offsets32, sizeof(struct graphics_offsets));
		}
        return true;
    }

    inline BOOL inject_hook(){
		bool anti_cheat = true;

		PROCESS_INFORMATION pi = { 0 };
		STARTUPINFO si = { 0 };
		si.cb = sizeof(si);
		wchar_t command_line_w[4096] = { 0 };
		swprintf(command_line_w, 4096, L"\"%s\" \"%s\" %lu %lu",
			process_is_64bit ? s_strHelper64.str() : s_strHelper32.str(),
			process_is_64bit ? s_strHook64.str()   : s_strHook32.str(),
			(unsigned long)anti_cheat,
			anti_cheat ? m_idThread : m_idProcess);

		BOOL success = !!CreateProcessW(process_is_64bit ? s_strHelper64.str() : s_strHelper32.str(), 
			command_line_w, NULL, NULL,
			false, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);

		if (success) {
			DWORD exit_code = 0;
			GetExitCodeProcess(pi.hProcess, &exit_code);
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			if (exit_code == 0) {
				stop_capture();
				WXLogW(L"CreateProcessW: %lu", GetLastError());
				return FALSE;
			}
		}else {
			WXLogW(L"Failed to create inject helper process: %lu", GetLastError());
		}
		return success;
    }

    inline bool init_keepalive() {
        wchar_t new_name[64];
        _snwprintf(new_name, 64, L"%s%lu", WINDOW_HOOK_KEEPALIVE,
            m_idProcess);

        keepalive_mutex = CreateMutexW(NULL, false, new_name);
        if (!keepalive_mutex) {
            WXLogW(L"Failed to create keepalive mutex: %lu", GetLastError());
            return false;
        }
        return true;
    }

    bool init_hook() {

        if (!open_target_process()) {
			wxlog("init_hook open_target_process error");
            return false;
        }

        if (!init_keepalive()) {
			wxlog("init_hook init_keepalive error");
            return false;
        }

        if (!attempt_existing_hook()) {
            if (!inject_hook()) {
				wxlog("init_hook inject_hook error");
                return false;
            }
        }

        if (!init_texture_mutexes()) {
			wxlog("init_hook init_texture_mutexes error");
            return false;
        }

        if (!init_hook_info()) {
			wxlog("init_hook init_hook_info error");
            return false;
        }

        if (!init_events()) {
			wxlog("init_hook init_events error");
            return false;
        }

        SetEvent(hook_init);
		m_ptsLast = timeGetTime();//视频时间戳
		m_bInject = true;//Hook 成功！！！

        WXAutoLock al(m_mutex);
        if (m_cbFunc) {
            m_cbFunc(m_pSink, WX_EVENT_HOOK_START, (void*)(m_strPathName.str()));//HOOK 成功消息　
        }
        return true;
    }

    void try_hook(){
		wxlog("%s ",__FUNCTION__);
        HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION, false, m_idProcess);
        if (process) {
            m_bApp = is_app(process);
            if (m_bApp) {
                get_app_sid(process);
            }
            CloseHandle(process);
        }

        HANDLE hook_restart = open_event_gc(EVENT_CAPTURE_RESTART);
        if (hook_restart) {
            CloseHandle(hook_restart);
        }

        if (!init_hook()) {
            stop_capture();//HOOK FAILED
        }
    }

    inline bool init_events(){
        if (!hook_restart) {
            hook_restart = open_event_gc(EVENT_CAPTURE_RESTART);
            if (!hook_restart) {
                WXLogW(L"init_events: failed to get hook_restart "
                    "event: %lu", GetLastError());
                return false;
            }
        }

        if (!hook_stop) {
            hook_stop = open_event_gc(EVENT_CAPTURE_STOP);
            if (!hook_stop) {
                WXLogW(L"init_events: failed to get hook_stop event: %lu",
                    GetLastError());
                return false;
            }
        }

        if (!hook_init) {
            hook_init = open_event_gc(EVENT_HOOK_INIT);
            if (!hook_init) {
                WXLogW(L"init_events: failed to get hook_init event: %lu",
                    GetLastError());
                return false;
            }
        }

        if (!hook_ready) {
            hook_ready = open_event_gc(EVENT_HOOK_READY);
            if (!hook_ready) {
				WXLogW(L"init_events: failed to get hook_ready event: %lu",
                    GetLastError());
                return false;
            }
        }

        if (!hook_exit) {
            hook_exit = open_event_gc(EVENT_HOOK_EXIT);
            if (!hook_exit) {
				WXLogW(L"init_events: failed to get hook_exit event: %lu",
                    GetLastError());
                return false;
            }
        }

        return true;
    }

    inline enum capture_result init_capture_data()
    {
        m_cx = m_global_hook_info->cx;
        m_cy = m_global_hook_info->cy;
        m_pitch = m_global_hook_info->pitch;

        if (m_mapViewData) {
            UnmapViewOfFile(m_mapViewData);
			m_mapViewData = NULL;
        }

        CloseHandle(hook_data_map);

        hook_data_map = open_map_plus_id(SHMEM_TEXTURE, m_global_hook_info->map_id);
        if (!hook_data_map) {
            DWORD error = GetLastError();
            if (error == 2) {
                return CAPTURE_RETRY;
            }else {
				WXLogW(L"init_capture_data: failed to open file "
                    "mapping: %lu", error);
            }
            return CAPTURE_FAIL;
        }
        //数据区域
		m_mapViewData = (uint8_t*)MapViewOfFile(hook_data_map, FILE_MAP_ALL_ACCESS, 0, 0, m_global_hook_info->map_size);
        if (!m_mapViewData) {
			WXLogW(L"init_capture_data: failed to map data view: %lu", GetLastError());
            return CAPTURE_FAIL;
        }

		m_iWidth  = m_cx / 4 * 4;
		m_iHeight = m_cy / 4 * 4;
        m_tempFrame.Init(AV_PIX_FMT_RGB32, m_cx ,m_cy);
		m_videoFrame.Init(AV_PIX_FMT_RGB32, m_iWidth, m_iHeight);

        return CAPTURE_SUCCESS;
    }

    inline bool init_shmem_capture(){
        texture_buffers[0] = (uint8_t*)m_mapViewData + shmem_data->tex1_offset;
        texture_buffers[1] = (uint8_t*)m_mapViewData + shmem_data->tex2_offset;
        convert_16bit = is_16bit_format(m_global_hook_info->format);
		m_dxgiFormat = (enum DXGI_FORMAT)m_global_hook_info->format;
        m_videoFormat = convert_16bit ? GS_BGRA : convert_format(m_global_hook_info->format);//RGBX

        m_iHookType = m_global_hook_info->m_iType;//类型
        if (m_window) {
            m_cursor.Capture();
        }

        WXCTSTR szHookType = WXGetHookType(m_iHookType);
        WXString msg;
        msg.Format(L"[%ws]\r\n App=[%ws], VFilp=%d, HookType=[%d][%ws]\r\n HookData ImageSize=[%dx%d,%d] format=%d\r\n convert_16bit=%d \r\n Out_Size=[%dx%d]",
            __FUNCTIONW__,
            m_strFileName.str(),
            (int)m_global_hook_info->flip,
            m_iHookType, szHookType,
            m_cx, m_cy, m_pitch, m_videoFormat, convert_16bit, m_iWidth, m_iHeight);
		wxlog(msg.c_str());
        WXLogW(msg.str());

        WXAutoLock al(m_mutex);
        if (m_cbFunc) {
			m_cbFunc(m_pSink, WX_EVENT_HOOK_WIDTH, &m_iWidth);
			m_cbFunc(m_pSink, WX_EVENT_HOOK_HEIGHT, &m_iHeight);
        }
        return true;
    }

    bool start_capture(){
        if (!init_shmem_capture()) {
            return false;
        }
		return true;
	}

	bool capture_valid() {
		return !object_signalled(target_process);
	}

	int Hooked() {
		return  m_bHooked;
	}

	int GetWidth() {
		return m_bHooked ?  m_iWidth : 0;
	}

	int GetHeight() {
		return m_bHooked ? m_iHeight: 0;
	}

	void SetPreview(HWND hwnd) {
		WXAutoLock al(m_mutex);
		m_hwndDisplay = hwnd;
	}

	void SetGameHwnd(HWND hwndCapture) {
		WXAutoLock al(m_mutex);
		m_window = hwndCapture;
		if (is_uwp_window(hwndCapture))
			m_window = get_uwp_actual_window(hwndCapture);
		m_idThread = ::GetWindowThreadProcessId(m_window, &m_idProcess);
		m_iMode = 0;//外部传入模式
	}

	void SetSink(void *_sink) {
		WXAutoLock al(m_mutex);
		m_pSink = _sink;
	}

	void SetCb(wxCallBack cb) {
		WXAutoLock al(m_mutex);
		m_cbFunc = cb;
	}

	//开始Hook线程
	int StartHook() {
		WXAutoLock al(m_mutex);

		ThreadSetName(L"WXGameCapture");
		ThreadStart(false);
		return WX_ERROR_SUCCESS;
	}

	void StartRecord(int nFps) {
		if (m_bHooked) {
			m_bOutput = TRUE;
			if (m_global_hook_info) {
				m_global_hook_info->m_bCapture = true;
			}
			m_nTime = int(1000.0 / nFps + 0.5);
		}
	}

	void StopRecord() {
		if (m_bOutput) {
			m_bOutput = FALSE;
			m_nTime = 100;
			if (m_global_hook_info) {
				m_global_hook_info->m_bCapture = false;
			}
		}
	}

	WXGameCapture() {
		//WXGameInit();
		m_iMode = 1;//顶级窗口模式
		m_mapViewData = NULL;
		shmem_data = NULL;
		texture_buffers[0] = NULL;
		texture_buffers[1] = NULL;
		texture_mutexes[0] = NULL;
		texture_mutexes[1] = NULL;
		m_idCurr = GetCurrentProcessId();
	}

	virtual ~WXGameCapture() {
		m_bOutput = FALSE;
		ThreadStop();
		stop_capture(); //完全销毁
		m_idProcess = 0;
		m_idThread = 0;
		m_idCurr = 0;
		m_window = nullptr;
	}

	//Hook 线程
	virtual  void ThreadProcess() {

		int64_t pts1 = WXGetTimeMs();

		if(m_global_hook_info){
			if (s_nTypeFps) {
				int fps = m_global_hook_info->m_nFps;
				WXString str;
				str.Format("%d", fps);
				if (!m_bOutput) {
					s_info.SetDrawColor(0x0000FF00, 0x00000000);
				}
				else {
					s_info.SetDrawColor(0x000000FF, 0x00000000);
				}
				s_info.DrawString(str.str(), 0, 0);
			}
			else {
				m_global_hook_info->m_bDrawText = 0;//不写字了
			}
		}

		if (m_bOutput) {
			AVFrame *frame = GrabFrame();
			WXRgbaPush(frame);
		}else {
			HookData();
		}

		int64_t pts2 = WXGetTimeMs() - pts1;
		SLEEPMS(m_nTime - pts2); //延时
		//可以通过改变nTime来改变采集速度
    }
};

//static WXLocker  s_lockGlobal;//游戏录制锁
static WXGameCapture * s_GameCapture = NULL;

void WXGameDrawCamera(AVFrame *frame, int x, int y, int w, int h) {
	//WXAutoLock al(s_lockGlobal);
	return s_info.DrawCamrea(frame,x,y, w, h);
}

WXMEDIA_API int64_t WXGameGetDuration() {
    if (s_GameCapture && s_GameCapture->Hooked()) {
        return timeGetTime() - s_GameCapture->m_ptsLast;
    }
    return -1;//没有开始录制
}

WXMEDIA_API int WXGameHookType() {
    WXAutoLock al(s_lockGlobal);
    if (s_GameCapture) {
        return s_GameCapture->GetHookType();
    }
    return -1;
}

WXMEDIA_API void WXGameDrawString(WXCTSTR str, int x, int y) {
    s_info.DrawString(str, x, y);
}

WXMEDIA_API void WXGameSetDrawColor(int colorText, int colorBk) {
    s_info.SetDrawColor(colorText, colorBk);
}

WXMEDIA_API void WXGameSetDrawFont(WXCTSTR strFont, int fontSize) {
    s_info.SetDrawFont(strFont, fontSize);
}

WXMEDIA_API void WXGameDrawImage(WXCTSTR str, int x, int y) {
    s_info.DrawImage(str, x, y);
}

WXMEDIA_API int WXGameHooked() {
    if (s_GameCapture) {
        return s_GameCapture->Hooked();
    }
    return 0;
}




WXMEDIA_API void WXGameShowFps(int b){
	s_nTypeFps = b;
}

//刷新FPS的模式
//MODE=0 表示按秒更新
//MODE=1 表示每一帧更新
WXMEDIA_API void  WXGameShowFpsMode(int mode) {
	s_nModeFps = mode;
}


//全屏Hook
WXMEDIA_API void  WXGameCreate(void *sink, wxCallBack cb) {
	WXAutoLock al(s_lockGlobal);
	SAFE_DELETE(s_GameCapture);
	if (s_GameCapture == NULL) {
		s_GameCapture = new  WXGameCapture;
		s_GameCapture->SetSink(sink);
		s_GameCapture->SetCb(cb);
	}
}

WXMEDIA_API void  WXGameCreateByWindow(HWND hwnd, void *sink, wxCallBack cb) {
    WXAutoLock al(s_lockGlobal);
	SAFE_DELETE(s_GameCapture);
    if (s_GameCapture == NULL) {
        s_GameCapture = new  WXGameCapture;
        s_GameCapture->SetGameHwnd(hwnd);
        s_GameCapture->SetSink(sink);
        s_GameCapture->SetCb(cb);
    }
}

WXMEDIA_API void  WXGameStart() {
    WXAutoLock al(s_lockGlobal);
    if (s_GameCapture) {
        s_GameCapture->StartHook();
    }
}

WXMEDIA_API void  WXGameSetPreview(HWND hwnd) {
    WXAutoLock al(s_lockGlobal);
    if (s_GameCapture) {
        s_GameCapture->SetPreview(hwnd);
    }
}

WXMEDIA_API void  WXGameStop() {
    WXAutoLock al(s_lockGlobal);
	SAFE_DELETE(s_GameCapture);
}

WXMEDIA_API void  WXGameDestory() {
    WXAutoLock al(s_lockGlobal);
	SAFE_DELETE(s_GameCapture);
}

WXMEDIA_API int  WXGameGetWidth() {
    WXAutoLock al(s_lockGlobal);
    if (s_GameCapture) {
        return s_GameCapture->GetWidth();
    }
    return NULL;
}

WXMEDIA_API int  WXGameGetHeight() {
    WXAutoLock al(s_lockGlobal);
    if (s_GameCapture) {
        return s_GameCapture->GetHeight();
    }
    return NULL;
}


void WXGameStartRecord(int nFps) {
	WXAutoLock al(s_lockGlobal);
	if (s_GameCapture) {
		 s_GameCapture->StartRecord(nFps);
	}
}
void WXGameStopRecord() {
	WXAutoLock al(s_lockGlobal);
	if (s_GameCapture) {
		s_GameCapture->StopRecord();
	}
}

