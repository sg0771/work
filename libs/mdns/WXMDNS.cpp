#include "../mdns/dns_sd.h"
#include "WXMDNS.h"
#include "WXBase.h"

//窗口类
class  WindowClass
{
	HINSTANCE				m_instance;
	WNDCLASSEX				m_wcex;
public:
	WindowClass(LPCTSTR name) {
		int ret = DNSServiceStart();
		m_instance = GetModuleHandle(NULL);
		memset(&m_wcex,0, sizeof(WNDCLASSEX));
		m_wcex.cbSize = sizeof(m_wcex);
		m_wcex.lpfnWndProc = DefWindowProc;// (WNDPROC)WndProc;
		m_wcex.hInstance = m_instance;
		m_wcex.lpszClassName = name;
		RegisterClassEx(&m_wcex);
	}

    HWND WindowCreate() {
		return CreateWindowW(m_wcex.lpszClassName,
			m_wcex.lpszClassName, 
			0,CW_USEDEFAULT, 0, CW_USEDEFAULT,
			0, NULL, NULL, m_instance, NULL);
	}

	void WindowDestroy(HWND hwnd) {
		::DestroyWindow(hwnd);
	}
};
static WindowClass s_inst(TEXT("mDNS"));


#include <map>
class  WXMDNSBrowser {
	struct Packet
	{
		std::string m_strAppName; //实例名字
		std::string m_strType;    //服务类型
		std::string m_strIP;      //IP地址
		std::string m_strHost;    //服务主机名
		int m_nPort;//服务端口
		WXDataBuffer m_bufTxt;//附加信息
		WXMDNSBrowser* ctx = nullptr;
	};
	std::map<std::string, Packet*> m_mapPacket;
public:
#define BONJOUR_EVENT		( WM_USER + 0x100 )	//
	// Prototypes
	void *m_pSink = nullptr;
	mdnsCallBack m_cbFunc = nullptr;

	WXLocker m_mutex;
	DNSServiceRef m_ServiceRef = NULL;
	HWND m_hWnd = nullptr;

	static void DNSSD_API GetAddrInfoCallback(DNSServiceRef sdRef,
		DNSServiceFlags                  flags,
		uint32_t                         interfaceIndex,
		DNSServiceErrorType              errorCode,
		const char                       *hostname, // tam-2.local. 主机名字
		const struct sockaddr            *address, //IP地址
		uint32_t                         ttl, //120
		void                             *context
	) {
		if (kDNSServiceErr_NoError == errorCode) {
			Packet* pkt = (Packet*)context;
			pkt->m_strIP = inet_ntoa(((const struct sockaddr_in*)address)->sin_addr);//IP地址
			pkt->m_strHost = hostname;//主机名字
			WXString strAppName = pkt->m_strAppName.c_str();
			WXString strHostName = pkt->m_strHost.c_str();
			WXString strIP = pkt->m_strIP.c_str();

			if (pkt->ctx->m_cbFunc)
				pkt->ctx->m_cbFunc(pkt->ctx->m_pSink,
					strAppName.str(),
					strHostName.str(),
					strIP.str(), 
					pkt->m_nPort, 0);
		}
	}

	static void DNSSD_API ResolveCallback(DNSServiceRef sdRef,
		DNSServiceFlags flags,
		uint32_t interfaceIndex,
		DNSServiceErrorType errorCode,
		const char  *fullname, //_wxcast._tcp.local. 服务全名
		const char  *hosttarget,//tam-2.local. 主机名字
		uint16_t port, //port //ntohs //
		uint16_t txtLen, const unsigned char *txtRecord, //txtRecord[txtLen] TXT 描述信息
		void *context
	) {
		if (kDNSServiceErr_NoError == errorCode) {
			Packet* pkt = (Packet*)context;
			pkt->m_nPort = ntohs(port);//服务端口
			pkt->m_bufTxt.Init((uint8_t*)txtRecord, txtLen);
			//查询IP
			DNSServiceErrorType err = DNSServiceGetAddrInfo(&sdRef, 0, 0, kDNSServiceProtocol_IPv4,
				hosttarget,
				GetAddrInfoCallback,//查询地址回调
				context);
			if (kDNSServiceErr_NoError == err) {
				err = DNSServiceProcessResult(sdRef);
			}
		}
	}

	static void DNSSD_API BrowserCallBack(DNSServiceRef	sdRef,
		DNSServiceFlags flags, 	//Add Flag=2 ifi=10 
		uint32_t ifi, 
		DNSServiceErrorType	inError,
		const char* inName, 	//inName AppName  实例名字
		const char* inType, 		//inType _wxcast._tcp. 服务类型
		const char* inDomain,  // local.
		void* inContext) {

		if (inError == kDNSServiceErr_NoError){
			WXMDNSBrowser* obj = (WXMDNSBrowser*)inContext;

			if (kDNSServiceFlagsAdd == flags) { 
				if (!obj->m_mapPacket.count(inName)) {
					//如果对象不存在
					Packet* pkt = new Packet;
					pkt->ctx = obj;
					pkt->m_strAppName = inName;
					pkt->m_strType = inType;
					DNSServiceErrorType err = DNSServiceResolve(&sdRef, 0, 0,
						inName,
						inType,
						inDomain,
						ResolveCallback,
						pkt);
					if (kDNSServiceErr_NoError == err) {
						obj->m_mapPacket[inName] = pkt;
						DNSServiceProcessResult(sdRef);//查询服务端口
					}
				};
			}
			else if (0 == flags) { //Remove!!!
				if (obj->m_mapPacket.count(inName)) {
					Packet* pkt = obj->m_mapPacket[inName];
					WXString strAppName = pkt->m_strAppName.c_str();
					WXString strHostName = pkt->m_strHost.c_str();
					WXString strIP = pkt->m_strIP.c_str();
					if (pkt->ctx->m_cbFunc)
						pkt->ctx->m_cbFunc(pkt->ctx->m_pSink,
							strAppName.str(),
							strHostName.str(),
							strIP.str(),
							pkt->m_nPort, 1);
					obj->m_mapPacket.erase(inName);
				}
			}
		}
	}

	static LRESULT CALLBACK NewWndProc(HWND hwnd, UINT inMsg,
		WPARAM inWParam, 
		LPARAM inLParam) {
		LRESULT					result;
		DNSServiceErrorType		err;
		WXMDNSBrowser*obj = (WXMDNSBrowser*)::GetWindowLongPtr(hwnd, GWLP_USERDATA);
		switch (inMsg) {
		case BONJOUR_EVENT://socket 消息
			err = DNSServiceProcessResult(obj->m_ServiceRef);
			if (err != kDNSServiceErr_NoError) {
				obj->Stop();
			}
			result = 0;
			break;
		default:
			result = DefWindowProc(hwnd, inMsg, inWParam, inLParam);
			break;
		}
		return(result);
	}

	void Stop() {
		WXAutoLock al(m_mutex);
		if (m_ServiceRef) {
			if (m_hWnd) {
				WSAAsyncSelect((SOCKET)DNSServiceRefSockFD(m_ServiceRef), m_hWnd, BONJOUR_EVENT, 0);
			}
			DNSServiceRefDeallocate(m_ServiceRef);
			m_ServiceRef = NULL;
		}
		if (m_hWnd) {
			s_inst.WindowDestroy(m_hWnd);
			m_hWnd = nullptr;
		}
	}

	int Start(const wchar_t * wszType, void *pSink, mdnsCallBack cb) {
		WXAutoLock al(m_mutex);
		if (m_hWnd != nullptr)
			return -1;

		int ret = -1;
		m_hWnd = s_inst.WindowCreate();
		if (m_hWnd) {
			WXString strType = wszType;
			m_pSink = pSink;
			m_cbFunc = cb;
			DNSServiceErrorType		err = DNSServiceBrowse(&m_ServiceRef, 0, 0, strType.c_str(), NULL, BrowserCallBack, this);
			if (err == kDNSServiceErr_NoError) {
				::SetWindowLongPtr(m_hWnd, GWLP_WNDPROC, (LONG_PTR)NewWndProc);
				::SetWindowLongPtr(m_hWnd, GWLP_USERDATA, (LONG_PTR)this);
				//将socket的 select操作的 FD_READ | FD_CLOSE 绑定 到 窗口消息上
				//也可以改写select操作
				err = WSAAsyncSelect((SOCKET)DNSServiceRefSockFD(m_ServiceRef), 
					m_hWnd, BONJOUR_EVENT, FD_READ | FD_CLOSE);
				if (err == 0) {
					return 0;
				}
				else {
					ret = -1;//WSAAsyncSelect 失败
					Stop();
				}
			}
			else {
				ret = -2;//DNSServiceBrowse 失败
				Stop();
			}
		}
		else {
			ret = -1;//CreateWindow 失败
		}
		Stop();
		return ret;
	}
};

WXMDNS_API void* WXMDNSBrowserStart(const wchar_t * wszType,
	void *pSink, mdnsCallBack cb) {
	WXMDNSBrowser*obj = new WXMDNSBrowser;
	int ret = obj->Start(wszType, pSink, cb);
	if (ret == 0)
		return(void*)obj;
	delete obj;
	return nullptr;
}

WXMDNS_API void WXMDNSBrowserStop(void* ptr) {
	if (ptr) {
		WXMDNSBrowser*obj = (WXMDNSBrowser*)ptr;
		obj->Stop();
		delete obj;
	}
}


//------------------------------------------------------------------
class Register :public WXThread {
	DNSServiceRef mService = nullptr;
	TXTRecordRef mTxtRecord;
	WXString m_strAppName;
	WXString m_strProtocal;
	int m_nPort = 0;
	WXLocker m_mutex;
public:
	virtual  void ThreadProcess() {
		clock_t time1 = clock();

		DNSServiceErrorType errortype = DNSServiceRegister(&mService, 0, 0,
			m_strAppName.c_str(), //设备名字
			m_strProtocal.c_str(),//注册类型
			NULL, NULL,
			htons(m_nPort),//注册端口
			TXTRecordGetLength(&mTxtRecord),
			TXTRecordGetBytesPtr(&mTxtRecord),
			NULL, NULL);
		if (errortype < 0) {
			this->ThreadWillStop();
		}
		clock_t time2 = clock() - time1;
		::Sleep(10 - time2);//TTL 默认120s
	}

	Register() {
		TXTRecordCreate(&mTxtRecord, 0, NULL);
	}

	virtual ~Register() {
		Stop();
	}

	void AddDesc(const wchar_t* wszKey, const wchar_t* wszValue) {
		WXAutoLock al(m_mutex);
		WXString strKey = wszKey;
		WXString strValue = wszValue;
		TXTRecordSetValue(&mTxtRecord, strKey.c_str(), strValue.length(), strValue.c_str());
	}

	//生命周期2555
	int  Start(const wchar_t* wszAppName, const wchar_t* wszProtocol, int nPort) {
		WXAutoLock al(m_mutex);
		m_strAppName = wszAppName;
		m_strProtocal = wszProtocol;
		m_nPort = nPort;

		AddDesc(L"Data",L"2022.12.20");
		AddDesc(L"Time",L"11:17");
		AddDesc(L"Name", L"Tam");
		AddDesc(L"Host", L"Tam.loacl");
		AddDesc(L"VCodec", L"H264");
		AddDesc(L"ACodec", L"AAC");

		DNSServiceErrorType errortype = DNSServiceRegister(&mService, 0, 0,
			m_strAppName.c_str(), //设备名字
			m_strProtocal.c_str(),//注册类型
			NULL, NULL,
			htons(m_nPort),//注册端口
			TXTRecordGetLength(&mTxtRecord),
			TXTRecordGetBytesPtr(&mTxtRecord),
			NULL, NULL);
		if (errortype == 0) {
			ThreadStart();
			return TRUE;
		}
		else {
			return FALSE;
		}
	}

	void Stop() {
		WXAutoLock al(m_mutex);
		ThreadStop();
		if (mService) { //反注册
			DNSServiceRefDeallocate(mService);
			TXTRecordDeallocate(&mTxtRecord);
			mService = NULL;
		}
	}
};

WXMDNS_API void* WXMDNSRegisterStart(const wchar_t* wszAppName, const wchar_t* wszProtocol, int nPort) {
	Register* dnssd = new Register;
	int ret = dnssd->Start(wszAppName, wszProtocol, nPort);
	if (ret == TRUE)
		return (void*)dnssd;
	delete dnssd;
	return nullptr;
}

WXMDNS_API void WXMDNSRegisterStop(void* ptr) {
	if (ptr) {
		Register* dnssd = (Register*)ptr;
		delete dnssd;
	}
}