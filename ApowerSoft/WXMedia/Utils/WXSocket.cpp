/*
用于TCP/UDP发送接收的简单封装
*/

#include <WXMediaCpp.h>



//TCP 发送
class  WXTcpSender {
	WXLocker m_locker;
	bool m_bInit = false;
	SOCKET m_sock = 0;//创建套接字
	WXString m_strIP = L"127.0.0.1";
	int m_nPort = 3914;

	int InitImpl() {
		m_sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (m_sock == INVALID_SOCKET) {
			return 0;
		}

		//建立连接
		SOCKADDR_IN server_addr;
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(m_nPort);
		server_addr.sin_addr.S_un.S_addr = inet_addr(m_strIP.c_str());
		int ret = ::connect(m_sock, (sockaddr *)&server_addr, sizeof server_addr);
		if (ret == SOCKET_ERROR) {
			Deinit();
			return 0;
		}

		int enable = TRUE;
		::setsockopt(m_sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&enable, sizeof(enable));
		m_bInit = true;
		return m_bInit;
	}
public:
	int  Init(const char *strIP, int nPort) {
		m_strIP.Format("%s", strIP);
		m_nPort = nPort;
		return InitImpl();
	}

	int  Send(const uint8_t *buf, int buf_size) {
		WXAutoLock al(m_locker);
		if (m_bInit) {
			int ret = ::send(m_sock, (const char*)buf, buf_size, 0);
			if (ret == SOCKET_ERROR) {
				Deinit();
				return 0;
			}
			return ret;
		}
		return SOCKET_ERROR;
	}

	void Deinit() { //第四步关闭socket
		WXAutoLock al(m_locker);
		m_bInit = false;
		if (m_sock) {
			::closesocket(m_sock);
			m_sock = 0;
		}
	}
};

//-------------------------------------------
WXMEDIA_API void *WXTcpSenderCreate(WXCTSTR wszIP, int nPort) {
	WXTcpSender* sender = new WXTcpSender;
	WXString strIP = wszIP;
	if (sender->Init(strIP.c_str(), nPort)) {
		return (void*)sender;
	}

	delete sender;
	return nullptr;
}

WXMEDIA_API int   WXTcpSenderSend(void *ptr, uint8_t *buf, int buf_size) {
	if (ptr) {
		WXTcpSender* sender = (WXTcpSender*)ptr;
		return sender->Send(buf, buf_size);
	}
	return 0;
}

WXMEDIA_API void  WXTcpSenderDestroy(void *ptr) {
	if (ptr) {
		WXTcpSender* sender = (WXTcpSender*)ptr;
		sender->Deinit();
		delete sender;
	}
}

//-------------------------------------------
//UDP 发送
class  WXUdpSender {
	WXLocker m_locker;
	int m_nUdpPort = 0;
	SOCKET m_sockListener = 0;//创建套接字
	SOCKADDR_IN m_saUdpServ;//指向通信对象的结构体指针     
public:
	int  Init(const char *strIP, int nPort) {
		//第二步建立一个数据报类型的UDP套接字
		WXAutoLock al(m_locker);
		m_sockListener = socket(PF_INET, SOCK_DGRAM, 0);
		if (m_sockListener <= 0) {
			printf("Can't socket(PF_INET, SOCK_DGRAM, 0) \n");
			return 0;
		}
		BOOL fBroadcast = TRUE;                             //用于setsockopt(),表示允许
															//       setsockopt函数用于设置套接口选项
															//       采用广播形式须将第三个参数设置为SO_BROADCAST
		int ret = setsockopt(m_sockListener, SOL_SOCKET, SO_BROADCAST, (CHAR *)&fBroadcast, sizeof(BOOL));
		if (ret < 0) {
			printf("Can't setsockopt SO_BROADCAST\n");
			return 0;
		}

		//  参数设置，注意要将IP地址设为INADDR_BROADCAST，表示发送广播UDP数据报
		m_saUdpServ.sin_family = AF_INET;
		m_saUdpServ.sin_addr.s_addr = inet_addr(strIP);
		m_saUdpServ.sin_port = htons(nPort);//发送用的端口，可以根据需要更改
		return 1;
	}

	int  Send(const uint8_t *buf, int buf_size) {
		WXAutoLock al(m_locker);
		if (m_sockListener) {
			return sendto(m_sockListener, (const char*)buf, buf_size, 0, (SOCKADDR *)&m_saUdpServ, sizeof(SOCKADDR_IN));
		}
		return -1;
	}

	void Deinit() { //第四步关闭socket
		WXAutoLock al(m_locker);
		if (m_sockListener) {
			closesocket(m_sockListener);         //关闭监听socket
			m_sockListener = 0;
		}
	}
};

WXMEDIA_API void *WXUdpSenderCreate(WXCTSTR wszIP, int nPort) {
	WXUdpSender* sender = new WXUdpSender;
	WXString strIP = wszIP;
	if (sender->Init(strIP.c_str(), nPort)) {
		return (void*)sender;
	}
	delete sender;
	return nullptr;
}

WXMEDIA_API int   WXUdpSenderSend(void *ptr, uint8_t *buf, int buf_size) {
	if (ptr) {
		WXUdpSender* sender = (WXUdpSender*)ptr;
		return sender->Send(buf, buf_size);
	}
	return 0;
}

WXMEDIA_API void  WXUdpSenderDestroy(void *ptr) {
	if (ptr) {
		WXUdpSender* sender = (WXUdpSender*)ptr;
		sender->Deinit();
		delete sender;
	}
}

//-------------------------------------------
class  WXTcpRecv {

	SOCKET m_fdListen = 0;//监听socket
	int m_nPort = 0;

	std::map<SOCKET, uint64_t>m_mapFdClinet;//接收服务端

	void *m_pSink = nullptr;
	OnTcpData m_cb = nullptr;
public:
	int  Start(int nPort, void *sink, OnTcpData cb) {
		m_nPort = nPort;
		m_pSink = sink;
		m_cb = cb;
		m_fdListen = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (m_fdListen == INVALID_SOCKET) {
			m_fdListen = 0;
			WXLogWriteNew("WXTcpRecv %ws ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) ",__FUNCTIONW__);
			return 0;
		}

		//TCP 服务 快速启动
		const int on = 1;
		::setsockopt(m_fdListen, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));

		//绑定
		SOCKADDR_IN server_addr;
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.S_un.S_addr = INADDR_ANY;
		server_addr.sin_port = htons(m_nPort);
		int ret = ::bind(m_fdListen, (sockaddr *)&server_addr, sizeof(server_addr));
		if (ret == SOCKET_ERROR) {
			WXLogWriteNew("WXTcpRecv %ws ::bind(m_sock, (sockaddr *)&server_addr, sizeof server_addr) ret=%d ", __FUNCTIONW__,ret);
			return 0;
		}

		ret = ::listen(m_fdListen, 1);
		if (ret == SOCKET_ERROR) {
			WXLogWriteNew("WXTcpRecv %ws Error ::listen(m_sock, 1) ret=%d ", __FUNCTIONW__, ret);
			return 0;
		}

		int enable = TRUE;
		ret = ::setsockopt(m_fdListen, IPPROTO_TCP, TCP_NODELAY, (const char*)&enable, sizeof(enable));
		if (ret == SOCKET_ERROR) {
			WXLogWriteNew("WXTcpRecv %ws Error ::setsockopt(m_sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&enable, sizeof(enable)) ret=%d ", __FUNCTIONW__, ret);
			return 0;
		}
		std::thread threadListen([this] {
			WXLogWriteNewW(L"Start TCP Listen Thread");
			while (true) {
				SOCKADDR_IN client_addr;
				int cAddrLen = sizeof(client_addr);
				SOCKET fd = ::accept(m_fdListen, (sockaddr *)&client_addr, &cAddrLen);//新的客户端
				if (fd == INVALID_SOCKET || fd == SOCKET_ERROR) {  //外部关闭监听线程m_fdListen
					break;
				}else { //新连接创建
					WXLogWriteNewW(L"TCP Listen Thread accept new IP=%d.%d.%d.%d ",
						(int)client_addr.sin_addr.S_un.S_un_b.s_b1,
						(int)client_addr.sin_addr.S_un.S_un_b.s_b2,
						(int)client_addr.sin_addr.S_un.S_un_b.s_b3,
						(int)client_addr.sin_addr.S_un.S_un_b.s_b4
					);
					uint64_t id = client_addr.sin_addr.S_un.S_addr;//区别符号
					m_mapFdClinet[fd] = id;
					//创建监听线程

					std::thread threadRecv([this, fd] {
						WXLogWriteNewW(L"Start TCP Recv [%08x] Thread",fd);
						while (true) {
							uint8_t buf[2048];
							int ret = ::recv(fd, (char*)buf, 2048, 0);
							if (ret == SOCKET_ERROR) {  //对方异常退出
								WXLogWriteNew("threadRecv will be Stop!");

								break;
							}else if (ret > 0) { //OK
								if (m_cb) {
									m_cb(m_pSink, m_mapFdClinet[fd], buf, ret);//回调数据处理
								}
							}else if (ret == 0) {
								SLEEPMS(1);
							}
						}
						//::shutdown(fd, 1);
						::closesocket(fd);//关闭远端Socket
						m_mapFdClinet[fd] = 0;
						m_mapFdClinet.erase(fd);
						WXLogWriteNewW(L"Stop TCP Recv [%08x] Thread", fd);
					});
					threadRecv.detach();
				}
			}
			WXLogWriteNewW(L"Start TCP Listen Thread");
			m_fdListen = 0;
		});
		threadListen.detach();
		return 1;
	}

	void Stop() {
		for (auto obj : m_mapFdClinet) {
			if (obj.first != 0) {
				//::shutdown(obj.first, 1);
				::closesocket(obj.first);//关闭远端Socket
				obj.second = 0;
				m_mapFdClinet.erase(obj.first);
			}
		}

		if (m_fdListen) {
			//::shutdown(m_fdListen, 1);
			::closesocket(m_fdListen);
			WXLogWriteNewW(L"::closesocket(m_sock)");
		}
	}

	virtual ~WXTcpRecv() {
		Stop();
	}
};


WXMEDIA_API void *WXTcpRecvCreate(int nPort, void *sink, OnTcpData cb) {
	WXTcpRecv *recv = new WXTcpRecv;
	if (recv->Start(nPort, sink, cb)) {
		return (void*)recv;
	}
	delete recv;
	return nullptr;
}

WXMEDIA_API void  WXTcpRecvDestroy(void *ptr) {
	if (ptr) {
		WXTcpRecv *recv = (WXTcpRecv*)ptr;
		recv->Stop();
		delete recv;
	}
}

//-------------------------------------------
//Udp接收
class  WXUdpRecv :public WXThread {
	void* m_pSink = nullptr;
	OnData m_cb = nullptr;
	int m_nPort = 0;
	SOCKET m_socket = 0;
public:
	int Start(int nPort, void *sink, OnData cb) {
		m_pSink = sink;
		m_cb = cb;
		m_nPort = nPort;

		m_socket = socket(AF_INET, SOCK_DGRAM, 0);
		if (m_socket == INVALID_SOCKET) {
			m_socket = 0;
			return 0;
		}

		SOCKADDR_IN addr_in;
		addr_in.sin_family = AF_INET;
		addr_in.sin_port = htons(m_nPort);
		addr_in.sin_addr.S_un.S_addr = htonl(INADDR_ANY);//本机
		int ret = ::bind(m_socket, (sockaddr*)&addr_in, sizeof(addr_in));
		if (ret == SOCKET_ERROR) {
			return 0;
		}
		ThreadSetName(L"WXUdpRecv");
		ThreadStart();
		return 1;
	}

	void Stop() {
		if (m_socket) {
			closesocket(m_socket);
			m_socket = 0;
		}
		ThreadStop();
	}

	virtual void ThreadPrepare() {

	}
	virtual void ThreadProcess() {
		SOCKADDR_IN client_addr;
		int len = sizeof(SOCKADDR_IN);
		char recvBuf[2000];
		int ret = recvfrom(m_socket, recvBuf, sizeof(recvBuf), 0, (sockaddr*)&client_addr, &len);
		if (ret > 0) { //不要做复杂处理，避免堵塞
			uint8_t* buf = (uint8_t*)(recvBuf);
			if (m_cb) {
				m_cb(m_pSink, buf, ret);
			}
		}
		else if (ret == SOCKET_ERROR) {
			this->ThreadWillStop();
		}
	}
	virtual void ThreadPost() {

	}
	virtual ~WXUdpRecv() {
		Stop();
	}
};

WXMEDIA_API void *WXUdpRecvCreate(int nPort, void *sink, OnData cb) {
	WXUdpRecv *recv = new WXUdpRecv;
	if (recv->Start(nPort, sink, cb)) {
		return (void*)recv;
	}
	delete recv;
	return nullptr;
}

WXMEDIA_API void  WXUdpRecvDestroy(void *ptr) {
	if (ptr) {
		WXUdpRecv *recv = (WXUdpRecv*)ptr;
		recv->Stop();
		delete recv;
	}
}