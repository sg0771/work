/*
WXBase 基础库，基于C++11
*/
#ifndef _WX_BASE_H_
#define _WX_BASE_H_

#include  <stdint.h>
#include  <stdarg.h>
#include  <thread>
#include  <mutex>
#include  <queue>
#include  <fstream>
#include  <codecvt>
#include  <functional>
#include  <filesystem>

#ifdef _WIN32
#include <Windows.h>
#define  RENAME MoveFileA
#include <tchar.h>
#else
#define  RENAME rename
#define _T(x)       x
#define _TEXT(x)    x
using namespace std;
#endif

//WXMedia 内部log
EXTERN_C void  WXLogA(const char* format, ...);
EXTERN_C void  WXLogW(const wchar_t* format, ...);

//递归锁
#define WXLocker    std::recursive_mutex

//自动锁
#define WXAutoLock   std::lock_guard<WXLocker>
#define WXTask       std::function<void()>
#define WXMutex      std::mutex
#define WXLockMutex  std::unique_lock<WXMutex> 
#define SLEEP(sec)   std::this_thread::sleep_for(std::chrono::seconds(sec));
#define SLEEPMS(ms)  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
#define WXCond       std::condition_variable

namespace WXBase {

	static bool MakeSureDirectoryPathExistsW(const std::wstring& path) {
		size_t pos = 0;
		std::wstring subdir;

		// 循环遍历每个目录层级
		while ((pos = path.find_first_of(L"/\\", pos + 1)) != std::wstring::npos) {
			subdir = path.substr(0, pos);
			size_t len = subdir.length();
			const wchar_t* tt = subdir.c_str();
			if (tt[len - 1] == L':') {
				continue;
			}
			if (!CreateDirectoryW(subdir.c_str(), nullptr)) {
				DWORD error = GetLastError();
				if (error != ERROR_ALREADY_EXISTS) {
					//std::wcerr << L"无法创建目录：" << subdir << L"，错误代码：" << error << std::endl;
					return false;
				}
			}
		}

		// 确保路径的最后一部分被创建
		if (!::CreateDirectoryW(path.c_str(), nullptr)) {
			DWORD error = GetLastError();
			if (error != ERROR_ALREADY_EXISTS) {
				//std::wcerr << L"无法创建目录：" << path << L"，错误代码：" << error << std::endl;
				return false;
			}
		}

		return true;
	}


	//分割字符串
	typedef std::vector<std::string>StringList;
	static StringList string_splitstr(const char* str, const char* pattern)
	{
		StringList  li;
		std::string subStr;
		std::string tPattern;
		size_t      patternLen = strlen(pattern);
		size_t      strLen = strlen(str);

		for (size_t i = 0; i < strLen; i++) {
			if (pattern[0] == str[i] && ((strLen - i) >= patternLen)) {
				if (memcmp(&pattern[0], &str[i], patternLen) == 0)
				{
					i += patternLen - 1;
					if (!subStr.empty())
					{
						li.push_back(subStr);
						subStr.clear();
					}
				}
				else {
					subStr.push_back(str[i]);
				}
			}
			else {
				subStr.push_back(str[i]);
			}
		}

		if (!subStr.empty()) {
			li.push_back(subStr);
		}

		return li;
	}

	//分割字符串
	typedef std::vector<std::wstring>WstringList;
	static WstringList string_splitstr(const wchar_t* str, const wchar_t* pattern)
	{
		WstringList  li;
		std::wstring subStr;
		std::wstring tPattern;
		size_t      patternLen = wcslen(pattern);
		size_t      strLen = wcslen(str);

		for (size_t i = 0; i < strLen; i++) {
			if (pattern[0] == str[i] && ((strLen - i) >= patternLen)) {
				if (memcmp(&pattern[0], &str[i], patternLen * sizeof(wchar_t)) == 0)
				{
					i += patternLen - 1;
					if (!subStr.empty())
					{
						li.push_back(subStr);
						subStr.clear();
					}
				}
				else {
					subStr.push_back(str[i]);
				}
			}
			else {
				subStr.push_back(str[i]);
			}
		}

		if (!subStr.empty()) {
			li.push_back(subStr);
		}

		return li;
	}


	//环境变量
	static std::wstring getEnvVarW(const std::wstring& name) {
		// 定义一个初始缓冲区长度
		DWORD bufferLen = 256;
		std::wstring buffer(bufferLen, L'\0');

		// 尝试获取环境变量
		DWORD resultLen = GetEnvironmentVariableW(name.c_str(), &buffer[0], bufferLen);

		// 如果获得的数据长度超过缓冲区，则重新分配缓冲区
		if (resultLen > bufferLen) {
			buffer.resize(resultLen);
			resultLen = GetEnvironmentVariableW(name.c_str(), &buffer[0], resultLen);
		}

		// 如果调用成功且结果不为空，则返回结果
		if (resultLen > 0 && resultLen <= buffer.size()) {
			return buffer.substr(0, resultLen);
		}

		// 如果找不到环境变量或者出现错误，返回空字符串
		return L"";
	}


	static void WXCond_Wait(WXCond* cond, int* bFlag) {
		if (cond) {
			WXMutex mutex;
			WXLockMutex lock(mutex);//定义独占锁
			cond->wait(lock, [bFlag] {
				if (bFlag) {
					return *bFlag == 1;//false 表示被堵塞, true表示不被堵塞
				}
				return false;//被堵塞
				});//阻塞到cond执行notify
		}
	}

	static void WXCond_Notify(WXCond* cond, int* bFlag, int bAll) {
		if (cond) {
			if (bFlag) {
				*bFlag = 1;
			}
			bAll ? cond->notify_all() : cond->notify_one();
		}
	}

	////当前系统string(Windows 默认是CP_ACP，其它平台默认是utf8) 转
	static std::wstring GBToUTF16(std::string strASNI) {
		int len = ::MultiByteToWideChar(CP_ACP, 0, strASNI.c_str(), -1, NULL, 0);
		if (len <= 0)
			return L"";
		std::wstring wstr(len, 0);
		::MultiByteToWideChar(CP_ACP, 0, strASNI.c_str(), -1, &wstr[0], len);
		return wstr;
	}

	static std::string UTF16ToGB(std::wstring strUTF16) {
		int len = ::WideCharToMultiByte(CP_ACP, 0, strUTF16.c_str(), -1, NULL, 0, NULL, NULL);
		if (len <= 0)
			return "";
		std::string str(len, 0);
		::WideCharToMultiByte(CP_ACP, 0, strUTF16.c_str(), -1, &str[0], len, NULL, NULL);
		return str;
	}

	static std::wstring UTF8ToUTF16(std::string strUTF8) {
		try
		{
			std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
			std::wstring strUTF16 = converter.from_bytes(strUTF8);
			return strUTF16;
		}
		catch (...)
		{
			return GBToUTF16(strUTF8);
		}

	}

	static std::string  UTF16ToUTF8(std::wstring strUTF16) {
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		std::string strUTF8 = converter.to_bytes(strUTF16);
		return strUTF8;
	}

	static std::string  UTF8ToGB(std::string strUTF8)
	{
		std::wstring wstrUTF16 = UTF8ToUTF16(strUTF8);
		std::string  result = UTF16ToGB(wstrUTF16);
		return result;
	}

	static std::string  GBToUTF8(std::string strASNI)
	{
		std::wstring wstrUTF16 = GBToUTF16(strASNI);
		std::string  result = UTF16ToUTF8(wstrUTF16);
		return result;
	}

	//当前运行目录
	static std::wstring GetRunPath() {
		std::wstring result = std::filesystem::current_path().wstring();
		result += L"\\";
		return result;
	}

	//当前EXE全路径
	static std::wstring GetFullExeName() {
		std::wstring fullPath = L"";
		if (__argv) {
			//__argv 不是UTF8
			fullPath = WXBase::GBToUTF16(__argv[0]);
		}
		else {
			fullPath = __wargv[0];
		}
		return fullPath; 
	}

	//获得EXE所在目录
	static std::wstring GetExePath() {
		std::wstring s_FullExeName = WXBase::GetFullExeName(); //当前EXE全路径
		std::filesystem::path path(s_FullExeName);
		std::wstring s_ExePath = path.parent_path().wstring() + L"\\";//EXE目录
		return s_ExePath;
	}

	//获得EXE名字
	static std::wstring GetExeName() {
		std::wstring s_FullExeName = WXBase::GetFullExeName(); //当前EXE全路径
		std::filesystem::path path(s_FullExeName);
		std::wstring s_ExeName = path.filename().wstring();//EXE名字
		return s_ExeName;
	}

	static HMODULE wxGetSelfModuleHandle()
	{
		MEMORY_BASIC_INFORMATION mbi;
		return ((::VirtualQuery(wxGetSelfModuleHandle, &mbi, sizeof(mbi)) != 0) ? (HMODULE)mbi.AllocationBase : NULL);
	}

	//Medialib.dll 所在目录
	static std::wstring GetDllPath() {
		wchar_t dllPath[MAX_PATH];
		GetModuleFileNameW(wxGetSelfModuleHandle()/*NULL*/, dllPath, MAX_PATH);
		std::filesystem::path path1(dllPath);
		std::wstring result = path1.parent_path().wstring();
		result += L"\\";
		return result;
	}

	//判断文件是否存在
	static bool Exists(const std::string& path) {
		try {
			std::filesystem::path p = std::filesystem::u8path(path);
			return std::filesystem::exists(p.c_str());
		}
		catch (...) {
			try
			{
				return std::filesystem::exists(path);;
			}
			catch (const std::exception&)
			{
				return false;
			}
		}
		return false;
	}

	static bool Exists(const std::wstring& path) {
		try {
			return std::filesystem::exists(path);
		}
		catch (...) {
			return false;
		}
		return false;
	}

	//获取文件长度
	static uint64_t Filesize(const std::string& path) {

		try {
			std::filesystem::path p = std::filesystem::u8path(path);
			// 获取文件大小
			return std::filesystem::file_size(p.c_str());
		}
		catch (...) {
			try
			{
				return std::filesystem::file_size(path);
			}
			catch (const std::exception&)
			{
				return 0;
			}
		}

		return 0;
	}
	
	static uint64_t Filesize(const std::wstring& path) {
		try
		{
			return std::filesystem::file_size(path);
		}
		catch (const std::exception&)
		{
			return 0;
		}
		return 0;
	}
	

};

//内存数据管理
class WXDataBuffer {
	std::shared_ptr<uint8_t> m_pBuf;
public:
	int     m_iBufSize = 0;
	int     m_iPos = 0;
	int64_t m_pts = 0;
	int64_t extra1 = 0;
	int64_t extra2 = 0;
public:
	uint8_t* GetBuffer() {
		return m_pBuf.get();
	}
	void Init(uint8_t* buf, int size, INT64 pts = 0) {
		if (size <= 0)
			return;
		m_pBuf.reset();
		m_pBuf = std::shared_ptr<uint8_t>((uint8_t*)malloc(size),
				[](uint8_t* p) { if (p) {
					free(p);
					p = nullptr;
				}
			}
		);

		if (buf != nullptr) {
			memcpy(GetBuffer(), buf, size);
		}
		else {
			memset(GetBuffer(), 0, size);
		}
		m_pts = pts;
		m_iBufSize = size;
	}

	WXDataBuffer() {}

	WXDataBuffer(uint8_t* buf, int size) {
		Init(buf, size);
	}

	virtual ~WXDataBuffer() {
		if (m_pBuf) {
			m_pBuf.reset();
			m_pBuf = nullptr;
			m_iBufSize = 0;
		}
	}
};

class WXString {
	std::string  m_strUTF8 = "";//ffmpeg 需要
	std::wstring m_strUTF16 = L"";

	//Acsi
	void Init(const char* sz) {
#ifdef _WIN32
		std::string strAsni = sz;
		m_strUTF16 = WXBase::GBToUTF16(strAsni);
		m_strUTF8 = WXBase::UTF16ToUTF8(m_strUTF16);
#else
		m_strUTF8 = sz;
		m_strUTF16 = WXBase::UTF8ToUTF16(sz);
#endif
	}

	//Unicode
	void Init(const wchar_t* wsz) {
		m_strUTF16 = wsz;
		m_strUTF8 = WXBase::UTF16ToUTF8(m_strUTF16);
	}

public:
	void Format(const char* format, ...) {
		char    szMsg[4096];
		memset(szMsg, 0, 4096);
		va_list marker
#ifdef  _WIN32
			= nullptr
#endif
			;
		va_start(marker, format);
		vsprintf(szMsg, format, marker);
		va_end(marker);
		Init(szMsg);
	}
	void Format(const wchar_t* format, ...) {
		wchar_t wszMsg[4096];
		memset(wszMsg, 0, 4096 * 2);
		va_list marker
#ifdef  _WIN32
			= nullptr
#endif
			;
		va_start(marker, format);
		vswprintf(wszMsg,
#ifndef _WIN32
			4096,
#endif
			format, marker);
		va_end(marker);
		Init(wszMsg);
	}
public:
	WXString() {}

	WXString(const WXString& wxstr) {
		Init(wxstr.w_str());
	}

	WXString(const wchar_t* wsz) {
		Init(wsz);
	}

	const int  length() const {
		return (int)m_strUTF16.length();
	}

#ifdef _WIN32
	const wchar_t* str()const {
		return w_str();
	}
	const wchar_t* Left(int n) {
		return m_strUTF16.c_str() + (length() - n);
	}
#else
	const char* str()const {
		return c_str();
	}
	const char* Left(int n) {
		return m_strUTF8.c_str() + (m_strUTF8.length() - n);
	}
#endif

	const wchar_t* w_str()const {
		return m_strUTF16.c_str();
	}

	const char* c_str() const {
		return m_strUTF8.c_str();
	}


public://WXString

	WXString& operator+=(const WXString& wxstr) {
		std::wstring wstr = m_strUTF16;
		wstr += wxstr.w_str();
		Init(wstr.c_str());
		return *this;
	}
	bool operator==(const WXString& wxstr) const {
		const wchar_t* wsz1 = this->w_str();
		const wchar_t* wsz2 = wxstr.w_str();
		return (wcscmp(wsz1, wsz2) == 0);
	}
	bool operator!=(const WXString& wxstr) const {
		const wchar_t* wsz1 = this->w_str();
		const wchar_t* wsz2 = wxstr.w_str();
		return (wcscmp(wsz1, wsz2) != 0);
	}

public://const wchar_t*
	WXString& operator+=(const wchar_t* wsz) {
		std::wstring wstr = m_strUTF16;
		wstr += wsz;
		Init(wstr.c_str());
		return *this;
	}
	bool operator==(const wchar_t* wsz) const {
		const wchar_t* wsz1 = this->w_str();
		return (wcscmp(wsz1, wsz) == 0);
	}
	bool operator!=(const wchar_t* wsz) const {
		const wchar_t* wsz1 = this->w_str();
		return (wcscmp(wsz1, wsz) != 0);
	}
	void Cat(WXString wxstr, const wchar_t* wszSlipt) {
		//添加一个间隔符来拼接字符串
		if (wxstr.length() != 0) {
			std::wstring wstr = m_strUTF16;
			if (length() != 0)
				wstr += wszSlipt;
			wstr += wxstr.w_str();
			Init(wstr.c_str());
		}
	}
};

namespace WXBase {
	//获取文件名后缀
	static WXString GetFileNameSuffix(const wchar_t* wszFileName) {
		WXString strFileName = wszFileName;
		int  length = strFileName.length();
		int pos = 0;
		for (int i = length - 1; i >= 0; i--) {
			if (strFileName.str()[i] == L'.') {
				pos = i;
				break;
			}
		}
		WXString strDst = L"";
		if (pos) {
			strDst = strFileName.Left(length - pos - 1);
		}
		return strDst;
	}
};

#include <chrono>

class AutoLog {
	WXString m_strName;
	std::chrono::high_resolution_clock::time_point m_start;

public:
	AutoLog(const char* name) {
		m_strName.Format("%s", name);
		// 记录开始时间
		m_start = std::chrono::high_resolution_clock::now();
		//WXLogW(L"  ############### %ws Begin ############### ", m_strName.str());
	}
	virtual ~AutoLog() {


		// 记录结束时间
		auto end = std::chrono::high_resolution_clock::now();

		// 计算耗时
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - m_start).count();

		WXLogW(L"  function %ws   [%d]ms ", m_strName.str(), (int)duration);

	}
};
#define BEGIN_LOG_FUNC  AutoLog CAutoLog(__FUNCTION__);

//成员应该为指针类型！
template <class PtrT>
class ThreadSafeQueue {
protected:
	WXLocker m_lock;
	std::queue<PtrT> m_queue;
public:
	bool Empty() {
		WXAutoLock al(m_lock);
		return m_queue.empty();
	}
	PtrT Pop() {
		PtrT obj = nullptr;
		{
			WXAutoLock al(m_lock);
			if (!m_queue.empty()) {
				obj = m_queue.front();
				m_queue.pop();
			}
		}
		return obj;
	}
	int Size()
	{
		WXAutoLock al(m_lock);
		return m_queue.size();
	}
	void Push(PtrT obj) {
		WXAutoLock al(m_lock);
		m_queue.push(obj);
	}
	void Flush() {
		WXAutoLock al(m_lock);
		while (!m_queue.empty()) {
			PtrT obj = m_queue.front();
			m_queue.pop();
			delete obj;
		}
	}
};

//线程类
class WXTrace {
public:
	int m_nMaxError = 5;//最多错误次数，超过之后内部log不执行
	int m_nError = 0;
	void  LogW(const wchar_t* format, ...) {
		if (m_nError < m_nMaxError) {
			wchar_t wszMsg[4096];
			memset(wszMsg, 0, 4096 * 2);
			va_list marker = nullptr;
			va_start(marker, format);
			vswprintf(wszMsg, format, marker);
			va_end(marker);
			WXLogW(wszMsg);
		}
	}
	void  LogA(const char* format, ...) {
		if (m_nError < m_nMaxError) {
			char szMsg[4096];
			memset(szMsg, 0, 4096);
			va_list marker = nullptr;
			va_start(marker, format);
			vsprintf(szMsg, format, marker);
			va_end(marker);
			WXLogA(szMsg);
		}
	}
};

class WXThread :public WXTrace {

	WXString m_strThreadName = L"WXThread";

	bool m_bUseTask = false;//异步任务
	ThreadSafeQueue<WXTask>m_queueTask;

	std::thread::id m_id;

	volatile bool m_bThreadStop = true;
	std::shared_ptr<std::thread> m_thread = nullptr;

	WXCond* m_condWait = nullptr;//等待启动
	int* m_bWaitFlag = nullptr;

	void WaitQueueEmpty() {
		WXMutex mutex;
		WXLockMutex lock(mutex);
		WXCond m_cond;//队列空消息
		m_cond.wait(lock, [this] {
			return m_queueTask.Empty(); //
			});
		return;
	}

	void TaskRunning() {
		if (m_bUseTask) { //异步任务
			while (!Empty()) {
				WXTask task = m_queueTask.Pop();
				if (Empty()) {
					task();//执行队列里面的任务
					break;
				}
				else {
					task(); //执行队列里面的任务
				}
			}
		}
	}

	bool Empty() {
		return m_queueTask.Empty();
	}

public:
	//线程循环前的初始化操作，运行这个后向 ThreadStart 发送启动成功消息
	virtual  void ThreadPrepare() {
		//this->LogA("WXThread [%ws] ThreadPrepare", m_strThreadName.str());
	}


	virtual  void ThreadWait() {
		//this->LogA("WXThread [%ws] ThreadWait", m_strThreadName.str());
	}

	//线程循环函数,必须实现
	virtual  void ThreadProcess() = 0;

	//线程循环结束后的退出处理
	virtual  void ThreadPost() {
		//this->LogW(L"WXThread [%ws] ThreadPost", m_strThreadName.str());
	}
public:
	bool IsRunning() {
		return m_thread != nullptr;
	}
	//功能: 在执行线程中执行任务
	//task: 任务
	//bSync: 是否等待到执行完毕
	void RunTask(WXTask task, int bSync = FALSE) {
		if (!m_bThreadStop && m_bUseTask) {
			if (std::this_thread::get_id() == m_id) {
				task();//直接运行
			}
			else {
				if (bSync)
					WaitQueueEmpty();
				m_queueTask.Push(task);
				if (bSync)
					WaitQueueEmpty();
			}
		}
	}


	void ThreadWillStop() {
		m_bThreadStop = true;
	}

	void ThreadSetCond(WXCond* condWait, int* bFlag) {
		m_condWait = condWait;//等待启动
		m_bWaitFlag = bFlag;
	}
	WXCond* GetCond() {
		return m_condWait;
	}
	int* GetFlag() {
		return m_bWaitFlag;
	}
	void ThreadSetName(const wchar_t* wszName) {
		m_strThreadName = wszName;
	}
	bool ThreadStart(bool bUseTask = false) {
		if (m_thread)
			return true;//已经启动

		m_bThreadStop = false;
		m_bUseTask = bUseTask;

		WXCond condStart;//启动消息
		int bStartFlag = 0;
		m_thread = std::shared_ptr<std::thread>(new std::thread([this, &condStart, &bStartFlag] {
			m_id = std::this_thread::get_id();//线程ID

			ThreadPrepare(); //线程资源初始化操作
			WXBase::WXCond_Notify(&condStart, &bStartFlag, FALSE);//通知ThreadStart线程已经启动

			if (nullptr != m_condWait) {//如果已经注册了外部信号量，堵塞等待
				WXBase::WXCond_Wait(m_condWait, m_bWaitFlag);
			}
			ThreadWait();//线程激活

			while (!m_bThreadStop) { //线程循环
				TaskRunning(); //执行队列上的任务
				ThreadProcess(); //自定义线程函数
			}

			ThreadPost(); //线程退出操作

		}), [](std::thread* thread) {
				//删除线程的处理
				if (thread) {
					if (thread->joinable()) {
						thread->join();
					}
					delete thread;
				}
				thread = nullptr;
		});

		//等待线程函数执行完ThreadPrepare
		//如果已经注册外部信号量
		//使用WXCond_Notify 来激活线程函数
		WXBase::WXCond_Wait(&condStart, &bStartFlag);
		return true;
	}

	void ThreadStop() {
		m_bThreadStop = true;//结束线程循环
		if (m_condWait) {
			WXBase::WXCond_Notify(m_condWait, m_bWaitFlag, TRUE);//避免线程堵塞，激活一下
		}
		m_thread = nullptr;//结束线程
	}
};

//单向队列
class WXFifo {
public:
	volatile int m_bEnable = 1;//是否可写
	WXDataBuffer m_dataBuffer;
	int64_t m_nTotalSize = 384000;
	int64_t m_nPosRead = 0;//读位置
	int64_t m_nPosWrite = 0;//写位置
	uint8_t m_last = 0;
	WXLocker m_mutex;
public:
	inline int64_t Size() {
		WXAutoLock al(m_mutex);
		return m_nPosWrite - m_nPosRead;
	}
public:
	WXFifo(int totolsize = 192000) {
		WXAutoLock al(m_mutex);
		m_dataBuffer.Init(nullptr, totolsize);
		m_nTotalSize = totolsize;
	}

	void Init(int totolsize = 192000) {
		WXAutoLock al(m_mutex);
		m_dataBuffer.Init(nullptr, totolsize);
		m_nTotalSize = totolsize;
	}

	void Reset() {
		WXAutoLock al(m_mutex);
		m_nPosRead = 0;
		m_nPosWrite = 0;
		memset(m_dataBuffer.GetBuffer(), 0, m_dataBuffer.m_iBufSize);
	}

	virtual ~WXFifo() {
		WXAutoLock al(m_mutex);
		m_nPosRead = 0;
		m_nPosWrite = 0;
	}

	void Write(uint8_t* pBuf, int nSize) { //写数据
		WXAutoLock al(m_mutex);
		if (m_bEnable) {
			int64_t newSize = m_nPosWrite - m_nPosRead + nSize;//可写区域
			if (newSize < m_nTotalSize) { //数据区可写
				int64_t posWirte = m_nPosWrite % m_nTotalSize;//实际写入位置
				int64_t posLeft = m_nTotalSize - posWirte;//在实际位置上buffer还剩多少可以写入的空间
				if (posLeft >= nSize) { //数据区足够
					memcpy(m_dataBuffer.GetBuffer() + posWirte, pBuf, nSize);
				}
				else {  //有部分数据需要写到RingBuffer的头部
					memcpy(m_dataBuffer.GetBuffer() + posWirte, pBuf, posLeft);//写到RingBuffer尾部
					memcpy(m_dataBuffer.GetBuffer(), pBuf + posLeft, nSize - posLeft);//写到RingBuffer头部
				}
				m_nPosWrite += nSize;//更新写位置
			}
		}
	}

	int Read(uint8_t* pBuf, int nSize) {//读数据
		WXAutoLock al(m_mutex);
		int64_t nowSize = m_nPosWrite - m_nPosRead;//可读区域
		if (nSize && nowSize >= nSize) { //数据足够读
			if (pBuf) {
				int64_t posRead = m_nPosRead % m_nTotalSize;//实际读取位置
				int64_t posLeft = m_nTotalSize - posRead;//在实际位置上buffer还剩多少可以写入的空间
				if (posLeft >= nSize) { //数据区足够
					memcpy(pBuf, m_dataBuffer.GetBuffer() + posRead, (size_t)nSize);
				}
				else {  //有部分数据需要写到RingBuffer的头部
					memcpy(pBuf, m_dataBuffer.GetBuffer() + posRead, (size_t)posLeft);//从尾部拷贝数据
					memcpy(pBuf + posLeft, m_dataBuffer.GetBuffer(), (size_t)(nSize - posLeft));//从头部拷贝数据
				}
			}
			m_nPosRead += nSize;
			m_last = pBuf[nSize - 1];
			return nSize;
		}
		return 0;
	}

	int Read2(uint8_t* buf, int size) {//buf=nullptr Skip
		WXAutoLock al(m_mutex);
		memset(buf, m_last, size);
		int64_t nowSize = m_nPosWrite - m_nPosRead;//有效数据区
		if (nowSize >= size) {
			return Read(buf, size);
		}
		else if (nowSize > 0) {
			int ret = Read(buf, nowSize);
			return ret;
		}
		return 0;
	}

	int Read3(uint8_t* buf, int size) {//buf=nullptr Skip
		WXAutoLock al(m_mutex);
		memset(buf, 0, size);
		int64_t nowSize = m_nPosWrite - m_nPosRead;//有效数据区
		if (nowSize >= size) {
			return Read(buf, size);
		}
		else if (nowSize > 0) {
			int ret = Read(buf, nowSize);
			return ret;
		}
		return 0;
	}
};

#endif
