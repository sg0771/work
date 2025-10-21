/*
����TCP/UDP���ͽ��յļ򵥷�װ
*/

#include <WXMediaCpp.h>



//TCP ����
class  WXTcpSender {
	WXLocker m_locker;
	bool m_bInit = false;
	SOCKET m_sock = 0;//�����׽���
	WXString m_strIP = L"127.0.0.1";
	int m_nPort = 3914;

	int InitImpl() {
		m_sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (m_sock == INVALID_SOCKET) {
			return 0;
		}

		//��������
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

	void Deinit() { //���Ĳ��ر�socket
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
//UDP ����
class  WXUdpSender {
	WXLocker m_locker;
	int m_nUdpPort = 0;
	SOCKET m_sockListener = 0;//�����׽���
	SOCKADDR_IN m_saUdpServ;//ָ��ͨ�Ŷ���Ľṹ��ָ��     
public:
	int  Init(const char *strIP, int nPort) {
		//�ڶ�������һ�����ݱ����͵�UDP�׽���
		WXAutoLock al(m_locker);
		m_sockListener = socket(PF_INET, SOCK_DGRAM, 0);
		if (m_sockListener <= 0) {
			printf("Can't socket(PF_INET, SOCK_DGRAM, 0) \n");
			return 0;
		}
		BOOL fBroadcast = TRUE;                             //����setsockopt(),��ʾ����
															//       setsockopt�������������׽ӿ�ѡ��
															//       ���ù㲥��ʽ�뽫��������������ΪSO_BROADCAST
		int ret = setsockopt(m_sockListener, SOL_SOCKET, SO_BROADCAST, (CHAR *)&fBroadcast, sizeof(BOOL));
		if (ret < 0) {
			printf("Can't setsockopt SO_BROADCAST\n");
			return 0;
		}

		//  �������ã�ע��Ҫ��IP��ַ��ΪINADDR_BROADCAST����ʾ���͹㲥UDP���ݱ�
		m_saUdpServ.sin_family = AF_INET;
		m_saUdpServ.sin_addr.s_addr = inet_addr(strIP);
		m_saUdpServ.sin_port = htons(nPort);//�����õĶ˿ڣ����Ը�����Ҫ����
		return 1;
	}

	int  Send(const uint8_t *buf, int buf_size) {
		WXAutoLock al(m_locker);
		if (m_sockListener) {
			return sendto(m_sockListener, (const char*)buf, buf_size, 0, (SOCKADDR *)&m_saUdpServ, sizeof(SOCKADDR_IN));
		}
		return -1;
	}

	void Deinit() { //���Ĳ��ر�socket
		WXAutoLock al(m_locker);
		if (m_sockListener) {
			closesocket(m_sockListener);         //�رռ���socket
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

	SOCKET m_fdListen = 0;//����socket
	int m_nPort = 0;

	std::map<SOCKET, uint64_t>m_mapFdClinet;//���շ����

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

		//TCP ���� ��������
		const int on = 1;
		::setsockopt(m_fdListen, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));

		//��
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
				SOCKET fd = ::accept(m_fdListen, (sockaddr *)&client_addr, &cAddrLen);//�µĿͻ���
				if (fd == INVALID_SOCKET || fd == SOCKET_ERROR) {  //�ⲿ�رռ����߳�m_fdListen
					break;
				}else { //�����Ӵ���
					WXLogWriteNewW(L"TCP Listen Thread accept new IP=%d.%d.%d.%d ",
						(int)client_addr.sin_addr.S_un.S_un_b.s_b1,
						(int)client_addr.sin_addr.S_un.S_un_b.s_b2,
						(int)client_addr.sin_addr.S_un.S_un_b.s_b3,
						(int)client_addr.sin_addr.S_un.S_un_b.s_b4
					);
					uint64_t id = client_addr.sin_addr.S_un.S_addr;//�������
					m_mapFdClinet[fd] = id;
					//���������߳�

					std::thread threadRecv([this, fd] {
						WXLogWriteNewW(L"Start TCP Recv [%08x] Thread",fd);
						while (true) {
							uint8_t buf[2048];
							int ret = ::recv(fd, (char*)buf, 2048, 0);
							if (ret == SOCKET_ERROR) {  //�Է��쳣�˳�
								WXLogWriteNew("threadRecv will be Stop!");

								break;
							}else if (ret > 0) { //OK
								if (m_cb) {
									m_cb(m_pSink, m_mapFdClinet[fd], buf, ret);//�ص����ݴ���
								}
							}else if (ret == 0) {
								SLEEPMS(1);
							}
						}
						//::shutdown(fd, 1);
						::closesocket(fd);//�ر�Զ��Socket
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
				::closesocket(obj.first);//�ر�Զ��Socket
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
//Udp����
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
		addr_in.sin_addr.S_un.S_addr = htonl(INADDR_ANY);//����
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
		if (ret > 0) { //��Ҫ�����Ӵ����������
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