#include "LibInst.hpp"
#include "WXLog.h"

//日志类
class WXLogInstance {
	WXLocker m_mutex;
	WXString m_strFileName;
	std::ofstream m_fout;
public:
	 WXLocker s_lockLog;//日志锁
public:
	int s_bInitLog = 0;
	bool s_bCreateFile = false;
	//限制程序一次运行的最大日志数量，避免日志过多
	int64_t s_nMaxLog = 20000;//程序运行中最多的日志数量
	int64_t s_nCountLog = 0;//log 数量
public:
	static WXLogInstance& GetInst() {
		static WXLogInstance s_inst;
		return s_inst;
	}
	WXLogInstance() {}

	virtual ~WXLogInstance() {
		Close();
		exit(-1);
	}

	void Close() {
		WXAutoLock al(m_mutex);
		if (m_fout.is_open()) {
			m_fout.close();
		}
	}

	bool Open(const wchar_t* strFileName) {
		WXAutoLock al(m_mutex);
		Close();

		m_strFileName = strFileName;
		int64_t filesize = WXBase::Filesize(m_strFileName.c_str());
		if (filesize > 1 * 1000 * 1000) {
			int rename2 = 1;
			while (1) {
				WXString wxstr;
				wxstr.Format(L"%ws.%d", strFileName, rename2);

				bool bExist = WXBase::Exists(wxstr.c_str());
				if (bExist) {
					rename2++;
					continue;
				}
				else {
					bExist = FALSE;
					RENAME(m_strFileName.c_str(), wxstr.c_str()); //
					break;
				}
			}
		}

		if (WXBase::Exists(m_strFileName.str())) {
			s_bCreateFile = false;
		}
		else {
			s_bCreateFile = true;//创建文件
		}

		m_fout.open(m_strFileName.str(), std::ios::app | std::ios::out | std::ios::binary);
		if (m_fout.is_open()) {
			if (s_bCreateFile) {
				uint8_t headText[2] = { 0xff, 0xfe };
				m_fout.write((const char*)headText, 2);
			}
			return true;
		}
		return m_fout.is_open();
	}

	void Write(const wchar_t* wszMsg) {
		WXAutoLock al(m_mutex);
		if (m_fout.is_open()) {
			m_fout.write((const char*)wszMsg, sizeof(wchar_t) * wcslen(wszMsg));
			m_fout.flush();
		}
	}

	void Write(const char* szMsg) {
		WXAutoLock al(m_mutex);
		if (m_fout.is_open()) {
			m_fout.write(szMsg, sizeof(char) * strlen(szMsg));
			m_fout.flush();
		}
	}

public:
	void WXLogV(const char* format, va_list args) {
		WXAutoLock al(s_lockLog);
		if (s_bInitLog) {
			s_nCountLog++;
			if (s_nCountLog >= s_nMaxLog) {
				s_bInitLog = false;//禁止写日志
				return;
			}
			//写入日志	
			char    szMsg[4096];
			memset(szMsg, 0, 4096);
			vsprintf(szMsg, format, args);
			WXString wxstr;
			auto now = std::chrono::system_clock::now();
			//通过不同精度获取相差的毫秒数
			uint64_t dis_millseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()
				- std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count() * 1000;
			time_t tt = std::chrono::system_clock::to_time_t(now);
			auto time_tm = localtime(&tt);
			wxstr.Format("%04d-%02d-%02d %02d:%02d:%02d %03d - %s\r\n",
				time_tm->tm_year + 1900,
				time_tm->tm_mon + 1, time_tm->tm_mday, time_tm->tm_hour,
				time_tm->tm_min, time_tm->tm_sec, (int)dis_millseconds, szMsg);
			Write(wxstr.str());
		}
	}

	void WXLogV(const wchar_t* format, va_list args) {
		WXAutoLock al(s_lockLog);
		if (s_bInitLog) {
			s_nCountLog++;
			if (s_nCountLog >= s_nMaxLog) {
				s_bInitLog = false;//禁止写日志
				return;
			}
			//写入日志
			wchar_t wszMsg[4096];
			memset(wszMsg, 0, 4096 * 2);
			vswprintf(wszMsg,
#ifndef _WIN32
				4096,
#endif
				format, args);
			WXString wxstr;
			auto now = std::chrono::system_clock::now();
			//通过不同精度获取相差的毫秒数
			uint64_t dis_millseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()
				- std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count() * 1000;
			time_t tt = std::chrono::system_clock::to_time_t(now);
			auto time_tm = localtime(&tt);

			wxstr.Format(L"%04d-%02d-%02d %02d:%02d:%02d %03d - %ws\r\n",
				time_tm->tm_year + 1900,
				time_tm->tm_mon + 1, time_tm->tm_mday, time_tm->tm_hour,
				time_tm->tm_min, time_tm->tm_sec, (int)dis_millseconds, wszMsg);
			Write(wxstr.w_str());
		}
	}

	void  WXLogVOnce(const char* format, va_list args) {
		WXAutoLock al(s_lockLog);
		if (s_bInitLog && s_bCreateFile) {
			s_nCountLog++;
			if (s_nCountLog >= s_nMaxLog) {
				s_bInitLog = false;//禁止写日志
				return;
			}
			//写入日志	
			char    szMsg[4096];
			memset(szMsg, 0, 4096);
			vsprintf(szMsg, format, args);
			WXString wxstr;
			auto now = std::chrono::system_clock::now();
			//通过不同精度获取相差的毫秒数
			uint64_t dis_millseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()
				- std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count() * 1000;
			time_t tt = std::chrono::system_clock::to_time_t(now);
			auto time_tm = localtime(&tt);
			wxstr.Format("%04d-%02d-%02d %02d:%02d:%02d %03d - %s\r\n",
				time_tm->tm_year + 1900,
				time_tm->tm_mon + 1, time_tm->tm_mday, time_tm->tm_hour,
				time_tm->tm_min, time_tm->tm_sec, (int)dis_millseconds, szMsg);
			Write(wxstr.str());
		}
	}

	void  WXLogVOnce(const wchar_t* format, va_list args) {
		WXAutoLock al(s_lockLog);
		if (s_bInitLog && s_bCreateFile) {
			s_nCountLog++;
			if (s_nCountLog >= s_nMaxLog) {
				s_bInitLog = false;//禁止写日志
				return;
			}
			//写入日志
			wchar_t wszMsg[4096];
			memset(wszMsg, 0, 4096 * 2);
			vswprintf(wszMsg,
#ifndef _WIN32
				4096,
#endif
				format, args);
			WXString wxstr;
			auto now = std::chrono::system_clock::now();
			//通过不同精度获取相差的毫秒数
			uint64_t dis_millseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()
				- std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count() * 1000;
			time_t tt = std::chrono::system_clock::to_time_t(now);
			auto time_tm = localtime(&tt);

			wxstr.Format(L"%04d-%02d-%02d %02d:%02d:%02d %03d - %ws\r\n",
				time_tm->tm_year + 1900,
				time_tm->tm_mon + 1, time_tm->tm_mday, time_tm->tm_hour,
				time_tm->tm_min, time_tm->tm_sec, (int)dis_millseconds, wszMsg);
			Write(wxstr.w_str());
		}
	}
public:

	int Init(const wchar_t* strFileName) {
		WXAutoLock al(s_lockLog);
		int bRet = Open(strFileName);
		s_bInitLog = bRet;
		return bRet;
	}
};

//设置日志文件
EXTERN_C int   WXLogInit(const wchar_t* strFileName) {
	return WXLogInstance::GetInst().Init(strFileName);
}

EXTERN_C void  WXLogAOnce(const char* format, ...) {
		va_list args
#ifdef _WIN32
			= nullptr
#endif
			;
		va_start(args, format);
		WXLogInstance::GetInst().WXLogVOnce(format, args);    // 调用内部处理函数
		va_end(args);
}

EXTERN_C void  WXLogWOnce(const wchar_t* format, ...) {
	va_list args
#ifdef _WIN32
		= nullptr
#endif
		;
	va_start(args, format);
	WXLogInstance::GetInst().WXLogVOnce(format, args);    // 调用内部处理函数
	va_end(args);
}

EXTERN_C void  WXLogA(const char* format, ...) {
	va_list args
#ifdef _WIN32
		= nullptr
#endif
		;
	va_start(args, format);
	WXLogInstance::GetInst().WXLogV(format, args);    // 调用内部处理函数
	va_end(args);
}

EXTERN_C void  WXLogW(const wchar_t* format, ...) {
	va_list args
#ifdef _WIN32
		= nullptr
#endif
		;
	va_start(args, format);
	WXLogInstance::GetInst().WXLogV(format, args);    // 调用内部处理函数
	va_end(args);
}


//WinRT API for WGC
EXTERN_C int32_t WINAPI WINRT_IMPL_RoOriginateLanguageException(int32_t error, void* message, void* languageException) noexcept
{
	if (LibInst::GetInst().m_RoOriginateLanguageException == nullptr)
		return TRUE;
	return LibInst::GetInst().m_RoOriginateLanguageException(error, message, languageException);
}

EXTERN_C int32_t WINAPI WINRT_IMPL_RoGetActivationFactory(void* classId, winrt::guid const& iid, void** factory) noexcept
{
	if (LibInst::GetInst().m_RoGetActivationFactory == nullptr) {
		*factory = nullptr;
		return winrt::impl::error_class_not_available;
	}
	return LibInst::GetInst().m_RoGetActivationFactory(classId, iid, factory);
}
