// wfdtest.cpp : 定义控制台应用程序的入口点。
//
#include <stdlib.h>
#include <stdio.h>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <iphlpapi.h>
#include <string>
#include <vector>

extern "C"{
#include "httpd.h"
#include "netutils.h"
#include "utils.h"
}

#include "MiracastExport.h"

#pragma pack(1)

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib,"Ws2_32.lib")

typedef DWORD (*MyWFDDisplaySinkCloseSession)(HANDLE hSessionHandle);

typedef DWORD (*MyWFDStopDisplaySink)(void);

typedef bool (*MyIsMiracastSupportedByWlan)(void);

typedef int(*MyWFDDisplaySinkInit)(__int64, __int64(*)(__int64, struct in_addr *, unsigned __int8 *), int *, __int64 *);

typedef void(*MyWFDDisplaySinkDeInit)(void);

NewQueue recvqueue;
UdpParam udpParam;

WXCastStruct g_stMiracast;

int read_data(void *opaque, uint8_t *buf, int buf_size) {
	//	UdpParam udphead;
	int size = buf_size;
	int ret;
	//	printf("read data %d\n", buf_size);
	do {
		ret = get_queue(&recvqueue, buf, buf_size);
	} while (ret);

	//	printf("read data Ok %d\n", buf_size);
	return size;
}

int caclseq(char* buffer)
{
	std::string strTmp = buffer;
	int pos = -1;
	if ((pos = strTmp.find("CSeq")) != -1)
	{
		strTmp = strTmp.substr(pos);
		pos = strTmp.find("\r\n");
		strTmp = strTmp.substr(6, pos - 6);
	}
	return atoi(strTmp.c_str());
}

#define BUF_SIZE	4096*500

//typedef __int64(*MyWFDStartDisplaySink)(unsigned __int64, unsigned __int64, unsigned __int64, unsigned __int64, int *, int *, int *);


#define _BYTE unsigned char
#define _QWORD unsigned __int64
#define _WORD short
#define _DWORD int
#define LODWORD(x)  (*((_DWORD*)&(x)))
#define LOBYTE(x)   (*((_BYTE*)&(x)))   // low byte
#define LOWORD(x)   (*((_WORD*)&(x)))   // low word
#define HIDWORD(x)  (*((_DWORD*)&(x)+1))
#define WORDn(x, n)   (*((_WORD*)&(x)+n))
#define WORD1(x)   WORDn(x,  1)
#define BYTEn(x, n)   (*((_BYTE*)&(x)+n))
#define BYTE4(x)   BYTEn(x,  4)
#define SHIDWORD(x)  (*((int*)&(x)+1))
#define BYTE1(x)   BYTEn(x,  1)         // byte 1 (counting from 0)
#define BYTE2(x)   BYTEn(x,  2)

typedef __int64(*MyWFDStartDisplaySink)(_QWORD, int *, _QWORD, int *, int *, __int64 *, int *);

void sub_7FFB89B75D00(_QWORD *a1);
void sub_7FFB898A4440(__int64 a1, struct _RTL_CRITICAL_SECTION *a2);
void sub_7FFB89B77E80(void *Memory, int a2, char a3);
__int64 sub_7FFB87BA3C00(__int64 a1);
bool sub_7FFB87BB0B20(__int16 *a1, __int64 a2);
bool sub_7FFB87BB0B20(__int16 *a1, __int64 a2);
__int64 sub_7FFB87BD1090(__int64 a1);
void sub_7FFB87BD0C90(void *Memory);
__int64 sub_7FFB8794E300(__int64 a1);
char * sub_7FFB89B70D40(__int64 *a1);
char sub_7FFB89B73210(LPVOID lpParameter, char *Src, __int16 a3, u_short a4, int a5, char a6);
char sub_7FFB89B713F0(LPCRITICAL_SECTION lpCriticalSection, int a2);
__int64 sub_7FFB87BA4420(LPVOID lpParameter);
int sub_7FFB87BA25E0(char *Format, signed __int64 a2, __int64 a3, char *Formata, ...);

typedef struct RTP_FIXED_HEADER {
	/* byte 0 */
	unsigned char csrc_len : 4;       /* expect 0 */
	unsigned char extension : 1;      /* expect 1 */
	unsigned char padding : 1;        /* expect 0 */
	unsigned char version : 2;        /* expect 2 */
									  /* byte 1 */
	unsigned char payload : 7;
	unsigned char marker : 1;        /* expect 1 */
									 /* bytes 2, 3 */
	unsigned short seq_no;
	/* bytes 4-7 */
	unsigned  long timestamp;
	/* bytes 8-11 */
	unsigned long ssrc;            /* stream number is used here. */
} RTP_FIXED_HEADER;

typedef struct MPEGTS_FIXED_HEADER {
	unsigned sync_byte : 8;
	unsigned transport_error_indicator : 1;
	unsigned payload_unit_start_indicator : 1;
	unsigned transport_priority : 1;
	unsigned PID : 13;
	unsigned scrambling_control : 2;
	unsigned adaptation_field_exist : 2;
	unsigned continuity_counter : 4;
} MPEGTS_FIXED_HEADER;

signed __int64 sub_7FFB8794D520(const char *a1, const char *a2)
{
	int v2; // ecx
	signed __int64 result; // rax

	v2 = _stricmp(a1, a2);
	if (!v2)
		return 1i64;
	result = 2i64;
	if (v2 < 0)
		result = 0i64;
	return result;
}

void __fastcall sub_7FFB87948710(__int64 a1) 
{

}

_DWORD * sub_7FFB87948000(int a1)
{
	int v1; // ebx
	_DWORD *result; // rax

	v1 = a1;
	result = (_DWORD*)malloc(0x30ui64);
	result[1] = 1;
	*result = v1;
	result[2] = 0;
	return result;
}

_DWORD * sub_7FFB8794F080(void *Src, size_t Size)
{
	unsigned int v2; // ebp
	void *v3; // rsi
	signed __int64 v4; // rbx
	_DWORD *v5; // rax
	_DWORD *v6; // rdi
	void *v7; // rax

	v2 = Size;
	v3 = Src;
	LODWORD(v4) = Size;
	if (!(_DWORD)Size)
	{
		v4 = -1i64;
		do
			++v4;
		while (*((_BYTE *)Src + v4));
		v2 = v4;
	}
	v5 = sub_7FFB87948000(2);
	if (v2 < (unsigned int)v4)
		LODWORD(v4) = v2;
	v6 = v5;
	*((_BYTE *)v5 + 20) = 0;
	v5[3] = v4;
	v5[4] = v4;
	v7 = malloc((unsigned int)(v4 + 1));
	*((_QWORD *)v6 + 3) = (_QWORD)v7;
	memcpy(v7, v3, (unsigned int)v4);
	*(_BYTE *)((unsigned int)v6[3] + *((_QWORD *)v6 + 3)) = 0;
	return v6;
}

__int64 sub_7FFB87948730(__int64 a1)
{
	__int64 result; // rax

	if (!a1)
		return 0i64;
	result = a1;
	//if (*(_DWORD *)(a1 + 8) != 1)
	//	_InterlockedIncrement((volatile signed __int32 *)(a1 + 4));
	return result;
}

__int64 sub_1803DF1F0(__int64 a1, struct in_addr *a2, unsigned __int8 *a3)
{
	__int64 v3; // rbp
	__int64 v4; // rsi
	unsigned __int8 *v5; // rbx
	struct in_addr *v6; // r14
	ULONG v7; // ecx
	int v8; // ecx
	int v9; // ecx
	void(__fastcall *v10)(__int64, _QWORD, _QWORD); // rax
	__int64 v11; // rbx
	int v12; // eax
	struct in_addr *v13; // r15
	int v14; // ebx
	char *v15; // rax
	char *v16; // rax
	int v17; // eax
	int v18; // ebx
	char *v19; // rax
	char *v20; // rax
	int v21; // eax
	int v22; // ebx
	char *v23; // rax
	char *v24; // rax
	_DWORD *v25; // rax
	__int64 v26; // rcx
	__int64 v27; // rbx
	__int64 v28; // r8
	char *v29; // rax
	__int64 v30; // ST28_8
	__int64 v31; // ST50_8
	__int64 v32; // ST48_8
	__int64 v33; // ST40_8
	__int64 v34; // ST38_8
	__int64 v35; // ST30_8
	__int64 v36; // ST28_8
	__int64 v38; // [rsp+28h] [rbp-110h]
	__int64 v39; // [rsp+28h] [rbp-110h]
	__int64 v40; // [rsp+30h] [rbp-108h]
	__int64 v41; // [rsp+30h] [rbp-108h]
	__int64 v42; // [rsp+30h] [rbp-108h]
	__int64 v43; // [rsp+30h] [rbp-108h]
	__int64 v44; // [rsp+38h] [rbp-100h]
	__int64 v45; // [rsp+38h] [rbp-100h]
	__int64 v46; // [rsp+40h] [rbp-F8h]
	__int64 v47; // [rsp+40h] [rbp-F8h]
	__int64 v48; // [rsp+48h] [rbp-F0h]
	__int64 v49; // [rsp+50h] [rbp-E8h]
	size_t PtNumOfCharConverted; // [rsp+60h] [rbp-D8h]
	char Dst[0x80]; // [rsp+70h] [rbp-C8h]

	printf("listen message begin\n");
	v3 = *(_QWORD *)(a1 + 40);
	v4 = a1;
	v5 = a3;
	v6 = a2;
	memset(&Dst, 0, 0x80ui64);
	wcstombs_s(&PtNumOfCharConverted, Dst, 0x80ui64, (const wchar_t*)&v6[2].S_un.S_un_w.s_w1, 0x7Fui64);
	LODWORD(v46) = v6->S_un.S_un_w.s_w2;
	LODWORD(v44) = v6->S_un.S_un_b.s_b2;
	LODWORD(v40) = v6->S_un.S_un_b.s_b1;
	//LODWORD(v38) = v6[1];
	printf("Received Miracast Sink notification of type %i, (Header %i, %i, size %i), from %s\n",
		0,
		v40,
		v44,
		v46,
		&Dst);
	v7 = v6[1].S_un.S_addr;
	if (!v7)
	{
		printf("Miracast Provisioning Request Notification\n");
		//LODWORD(v30) = v6[22];
		//sub_7FFB87BA25E0(
		//	(char *)0x12C,
		//	(signed __int64)"Supported Provisioning Methods: 0x%08x",
		//	(__int64)"MIRACAST",
		//	"Supported Provisioning Methods: 0x%08x",
		//	v30);
		if (v5)
		{
			LODWORD(v49) = *((_DWORD *)v5 + 3);
			LODWORD(v48) = *((_DWORD *)v5 + 2);
			LODWORD(v47) = *((_DWORD *)v5 + 1);
			LODWORD(v45) = *((unsigned __int16 *)v5 + 1);
			LODWORD(v41) = v5[1];
			LODWORD(v39) = *v5;
			printf("Default Result: Header(%i,%i,%i), %i, 0x%08x, %i\n",
				v39,
				v41,
				v45,
				v47,
				v48,
				v49);
			LODWORD(v31) = *((_DWORD *)v5 + 3);
			LODWORD(v32) = 128;
			LODWORD(v33) = 0;
			LODWORD(v34) = 24;
			LODWORD(v35) = 1;
			LODWORD(v36) = 1;
			*(_QWORD *)v5 = 1573121i64;
			*((_DWORD *)v5 + 2) = 128;
			printf("Output Result: Header(%i,%i,%i), %i, 0x%08x, %i\n",
				v36,
				v35,
				v34,
				v33,
				v32,
				v31);
			return 0i64;
		}
		printf("NotificationResult was null\n");
	LABEL_19:
		return 0i64;
	}
	v8 = v7 - 1;
	if (!v8)
	{
		printf("Miracast Reconnect Request Notification\n");
		goto LABEL_19;
	}
	v9 = v8 - 1;
	if (v9)
	{
		if (v9 == 1)
		{
			printf("Miracast Disconnect Notification\n");
			v10 = *(void(__fastcall **)(__int64, _QWORD, _QWORD))(v4 + 32);
			if (v10)
			{
				v11 = *(_QWORD *)(v3 + 56);
				if (v11)
				{
					v10(v4, *(_QWORD *)(v3 + 56), *(_QWORD *)(v4 + 8));
					*(_QWORD *)(v3 + 56) = 0i64;
					//sub_7FFB87948710(v11);
				}
			}
		}
	}
	else
	{
		printf("Miracast Connect Notification\n");
		v12 = v6[108].S_un.S_un_w.s_w1;
		v13 = v6 + 77;
		*(_WORD *)(v3 + 64) = v12;
		v14 = v12;
		v15 =  inet_ntoa(v6[77]);
		LODWORD(v41) = v14;
		printf("Remote Address %s, port %i\n", v15, v41);
		v16 = inet_ntoa(v6[77]);
		if ((unsigned int)sub_7FFB8794D520(v16, "0.0.0.0") == 1)
		{
			printf("Invalid address.  Try again\n");
			v17 = v6[108].S_un.S_un_w.s_w1;
			v13 = v6 + 77;
			*(_WORD *)(v3 + 64) = v17;
			v18 = v17;
			v19 = inet_ntoa(v6[77]);
			LODWORD(v42) = v18;
			printf("Remote Address %s, port %i\n", v19, v42);
			v20 = inet_ntoa(v6[77]);
			if ((unsigned int)sub_7FFB8794D520(v20, "0.0.0.0") == 1)
			{
				v21 = v6[104].S_un.S_un_w.s_w1;
				v13 = v6 + 73;
				*(_WORD *)(v3 + 64) = v21;
				v22 = v21;
				v23 = inet_ntoa(v6[73]);
				LODWORD(v43) = v22;
				printf("Remote Address %s, port %i\n", v23, v43);
			}
		}
		if (*(_QWORD *)(v4 + 24))
		{
			v24 = inet_ntoa(*v13);
			v25 = sub_7FFB8794F080(v24, 0i64);
			v26 = *(_QWORD *)(v3 + 56);
			v27 = (__int64)v25;
			if (v26)
			{
				*(_QWORD *)(v3 + 56) = 0i64;
				sub_7FFB87948710(v26);
			}
			sub_7FFB87948730(v27);
			v28 = *(unsigned __int16 *)(v3 + 64);
			*(_QWORD *)(v3 + 56) = v27;
			(*(void(__fastcall **)(__int64, __int64, __int64, char *, _QWORD))(v4 + 24))(
				v4,
				v27,
				v28,
				Dst,
				*(_QWORD *)(v4 + 8));
			sub_7FFB87948710(v27);
		}
	}
	printf("listen message end\n");
	return 0i64;
}

const char * sub_7FFB87BE3210(int a1)
{
	const char *result; // rax

	switch (a1)
	{
	case 0:
		result = "RFServerMiracastStateIdle";
		break;
	case 1:
		result = "RFServerMiracastStateProvisioning";
		break;
	case 2:
		result = "RFServerMiracastStateGONegotiation";
		break;
	case 3:
		result = "RFServerMiracastStateGroupFormed";
		break;
	case 4:
		result = "RFServerMiracastStateP2PConnected";
		break;
	case 5:
		result = "RFServerMiracastStateIPAddressObtained";
		break;
	case 6:
		result = "RFServerMiracastStateSinkConnectingToSource";
		break;
	case 7:
		result = "RFServerMiracastStateSinkNegotiatingCapabilities";
		break;
	case 8:
		result = "RFServerMiracastStateSinkConnectionEstablished";
		break;
	case 9:
		result = "RFServerMiracastStateFramesReceived";
		break;
	case 10:
		result = "RFServerMiracastStateDisconnected";
		break;
	case 11:
		result = "RFServerMiracastStateError";
		break;
	default:
		result = "Unknown";
		break;
	}
	return result;
}

signed __int64 sub_7FFB87BD3A90(int a1)
{
	signed __int64 result; // rax

	result = a1;
	switch (a1)
	{
	case 0:
		result = 0i64;
		break;
	case 1:
		result = 1i64;
		break;
	case 2:
		result = 2i64;
		break;
	case 3:
		result = 3i64;
		break;
	case 4:
		result = 4i64;
		break;
	case 5:
		result = 5i64;
		break;
	case 6:
		result = 6i64;
		break;
	case 7:
		result = 7i64;
		break;
	case 8:
		result = 8i64;
		break;
	case 9:
		result = 9i64;
		break;
	case 10:
		result = 10i64;
		break;
	case 11:
		result = 11i64;
		break;
	default:
		return result;
	}
	return result;
}

__int64(__fastcall *__fastcall sub_7FFC056AD240(__int64 a1, __int64 a2, unsigned int a3, __int64 a4))(__int64, _QWORD, _QWORD, _QWORD)
{
	__int64 v4; // rsi
	int v5; // eax
	__int64 v6; // r9
	int v7; // edi
	unsigned int v8; // ebp
	const char *v9; // rbx
	const char *v10; // rax
	__int64(__fastcall *result)(__int64, _QWORD, _QWORD, _QWORD); // rax

	printf("sub_7FFC056AD240 callback begin\n");

	v4 = a4;
	v5 = sub_7FFB87BD3A90(a3);
	//v7 = *(_DWORD *)(v6 + 1408);
	v8 = v5;
	//*(_DWORD *)(v6 + 1408) = v5;
	//v9 = sub_7FFB87BE3210(v5);
	//v10 = sub_7FFB87BE3210(v7);
	//sub_7FFB87BA25E0(
	//	(char *)0xC8,
	//	(signed __int64)"Miracast State Changed from %s to %s",
	//	(__int64)"RFSERVER",
	//	"Miracast State Changed from %s to %s",
	//	v10,
	//	v9);
	result = *(__int64(__fastcall **)(__int64, _QWORD, _QWORD, _QWORD))(v4 + 872);
	if (result)
		result = (__int64(__fastcall *)(__int64, _QWORD, _QWORD, _QWORD))result(
			v4,
			*(unsigned int *)(v4 + 1408),
			v8,
			*(_QWORD *)(v4 + 1416));
	return result;
}

__int64 sub_7FFB87BDAE80(LPCRITICAL_SECTION lpCriticalSection, __int64 *a2, unsigned int a3)
{
	return 0;
}

void sub_7FFB87BD0C90(void *Memory) 
{

}

__int64 sub_7FFB87BB65E0(unsigned int *a1)
{
	return *a1;
}

void sub_7FFB87BD1C70(__int64 a1, char a2)
{
	*(_BYTE *)(a1 + 557) = a2;
	*(_BYTE *)(a1 + 558) = a2;
}

void sub_7FFB87BD1DF0(__int64 a1, int a2)
{
	*(_DWORD *)(a1 + 1024) = a2;
}

void sub_7FFB87BCAD20(__int64 a1, signed int a2, unsigned int a3, __int64 a4) 
{
	__int64 v4; // rsi
	SOCKET v5; // r14
	signed int v6; // ebx
	__int64 v7; // rdi
	__int64 v8; // rdx
	__int64 v9; // r15
	__int64 v10; // rbx
	__int64 v11; // rdi
	__int64 v12; // r14
	__int64 v13; // rax
	__int64 v14; // rax
	__int64 v15; // rbx
	__int64 v16; // r12
	__int64 v17; // rsi
	__int64 v18; // rax
	__int64 v19; // rdx
	int v20; // ecx
	__int64 v21; // rax
	void *v22; // rbx
	char *v23; // rax
	int v24; // edx
	char *v25; // r12
	char *v26; // rsi
	char *v27; // r14
	char *v28; // r15
	char *v29; // rax
	__int64 v30; // rcx
	unsigned int v31; // esi
	__int64 v32; // rcx
	void *v33; // rcx
	__int16 v34; // r12
	__int64 v35; // rax
	__int64 v36; // r9
	__int64 v37; // rcx
	SOCKET v38; // rax
	char *v39; // rax
	char *v40; // rax
	char *v41; // rax
	char *v42; // rax
	char *v43; // rax
	char *v44; // rax
	char *v45; // rax
	__int64 v46; // rax
	__int64 v47; // [rsp+28h] [rbp-19h]
	__int64 v48; // [rsp+28h] [rbp-19h]
	char v49[8]; // [rsp+60h] [rbp+1Fh]
	int v50[2]; // [rsp+68h] [rbp+27h]
	int v51[2]; // [rsp+70h] [rbp+2Fh]
	char v52; // [rsp+78h] [rbp+37h]
	int v53; // [rsp+80h] [rbp+3Fh]
	SOCKET s; // [rsp+A0h] [rbp+5Fh]
	__int64 v55; // [rsp+C0h] [rbp+7Fh]

	v4 = a4;
	v5 = (signed int)a3;
	v6 = a2;
	v7 = a1;
	//*(_DWORD *)(a1 + 592) = a2;

	//miracast
	if (a2 == 512) 
	{
		v22 = 0i64;
		if (*(_QWORD *)a4)
		{
			if (!*(_WORD *)(a4 + 8))
				sub_7FFB87BA25E0(
				(char *)0x190,
					(signed __int64)"Warning: Connecting to Miracast source with an unspecified port, using the default",
					(__int64)"RFSERVER",
					"Warning: Connecting to Miracast source with an unspecified port, using the default",
					0i64);
		}

		//
		//sub_7FFB87BA4420(NULL);
	}
}

__int64(__fastcall *__fastcall sub_7FFB87BCEF50(__int64 a1, unsigned int a2))(__int64, _QWORD, _QWORD, _QWORD)
{
	__int64(__fastcall *result)(__int64, _QWORD, _QWORD, _QWORD); // rax
	unsigned int v3; // er10

	result = *(__int64(__fastcall **)(__int64, _QWORD, _QWORD, _QWORD))(a1 + 1152);
	v3 = *(_DWORD *)(a1 + 20);
	*(_DWORD *)(a1 + 20) = a2;
	if (result)
		result = (__int64(__fastcall *)(__int64, _QWORD, _QWORD, _QWORD))result(a1, v3, a2, *(_QWORD *)(a1 + 1104));
	return result;
}

void get_fairplay_socket(const char* ip, int port, int* fairplay_sock_fd)
{
	struct sockaddr_in ser_addr;

	if (*fairplay_sock_fd > 0) return;

	memset(&ser_addr, 0, sizeof(ser_addr));
	ser_addr.sin_family = AF_INET;
	if (inet_pton(AF_INET, ip, &ser_addr.sin_addr) <= 0)
	{
		printf("inet_pton error for %s\n", ip);
		return;
	}
	ser_addr.sin_port = htons(port);

	*fairplay_sock_fd = socket(AF_INET, SOCK_STREAM, 0);

	do
	{
		unsigned long flags;
#ifdef _WIN32
		int timeout = 5 * 1000;
		struct timeval select_timeout = { 0 };
		select_timeout.tv_sec = 5;
#else
		struct timeval timeout;
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
#endif

		if (*fairplay_sock_fd <= 0)
		{
			fprintf(stderr, "%s:%d, create socket failed", __FILE__, __LINE__);
			break;
		}
		//
		if (setsockopt(*fairplay_sock_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) != 0)
		{
			break;
		}
		//
		if (setsockopt(*fairplay_sock_fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) != 0)
		{
			break;
		}

#ifdef _WIN32
		flags = 1;
		if (ioctlsocket(*fairplay_sock_fd, FIONBIO, &flags) != 0)
#else
		flags = fcntl(*fairplay_sock_fd, F_GETFL, 0);
		flags |= O_NONBLOCK;
		if (fcntl(*fairplay_sock_fd, F_SETFL, flags) != 0)
#endif
		{
			break;
		}
		//
		if (connect(*fairplay_sock_fd, (struct sockaddr *)&ser_addr, sizeof(ser_addr)) < 0)
		{
			fd_set fs;
			int ret = 0;
			//
			FD_ZERO(&fs);
			FD_SET(*fairplay_sock_fd, &fs);
#ifdef _WIN32
			ret = select(*fairplay_sock_fd + 1, NULL, &fs, NULL, &select_timeout);
#else
			while (true) {
				int err = errno;
				ret = select(*fairplay_sock_fd + 1, NULL, &fs, NULL, &timeout);
				err = errno;
				if (errno != 4 || ret >0) {
					break;
				}
			}

#endif

			if (ret == 0)
			{
				fprintf(stderr, "%s:%d, create socket failed: %d", __FILE__, __LINE__, GetLastError());
				char tmpbuffer[200] = { 0 };
				sprintf(tmpbuffer, "%s:%d, create socket failed: %d", __FILE__, __LINE__, errno);
				break;
			}
			else if (ret < 0)
			{

				break;
			}
		}
#ifdef _WIN32
		flags = 0;
		if (ioctlsocket(*fairplay_sock_fd, FIONBIO, &flags) != 0)
#else
		flags = fcntl(*fairplay_sock_fd, F_GETFL, 0);
		flags &= ~O_NONBLOCK;
		if (fcntl(*fairplay_sock_fd, F_SETFL, flags) != 0)
#endif
		{
			break;
		}
		return;
	} while (0);
	*fairplay_sock_fd = 0;
	return;
}

void close_fairplay_socket(int* fairplay_sock_fd)
{
	if (*fairplay_sock_fd > 0) closesocket(*fairplay_sock_fd);/*close(fairplay_sock_fd);*/
	*fairplay_sock_fd = 0;
}

void* http_init(int max_clients, int *error)
{
	httpd_t *httpd = NULL;
	httpd_callbacks_t httpd_cbs;

	/* Initialize the network */
	if (netutils_init() < 0) {
		return NULL;
	}

	/* Set HTTP callbacks to our handlers */
	memset(&httpd_cbs, 0, sizeof(httpd_cbs));
	httpd_cbs.opaque = NULL;

	/* Initialize the http daemon */
	httpd = httpd_init(NULL, &httpd_cbs, max_clients);
	if (!httpd) {
		return NULL;
	}

	return httpd;
}

signed __int64 sub_7FFD4E49C290(__int64 a1)
{
	__int64 v1; // rbx
	char v2; // cl
	unsigned __int64 v3; // rdx
	int v4; // er12
	__int64 v5; // r14
	signed __int64 v6; // rsi
	unsigned __int64 v7; // rdi
	u_short v8; // cx
	u_short v9; // cx
	__int64 v10; // rcx
	unsigned __int64 v11; // rsi
	unsigned __int64 v12; // rcx
	__int64 v14; // rdi
	unsigned __int64 v15; // rbp
	unsigned __int64 v16; // r15
	char v17; // al
	u_short v18; // cx
	u_short v19; // ax
	u_long v20; // ecx
	u_long v21; // eax
	u_long v22; // ecx
	u_long v23; // eax
	void *v24; // rcx
	u_long v25; // eax

	v1 = a1;
	v2 = *(_BYTE *)(a1 + 80);
	if ((v2 & 0xC0) != 0x80)
		return 0i64;
	v3 = *(_QWORD *)(v1 + 1584);
	if (v3 - 13 > 0x5CF)
		return 0i64;
	v4 = v2 & 0x20;
	LODWORD(v5) = v2 & 0xF;
	if (!(v2 & 0x10))
		goto LABEL_24;
	v6 = 4i64 * (v2 & 0xF) + 12;
	v7 = 4i64 * (v2 & 0xF) + 16;
	if (v7 > v3)
		return 1i64;
	v8 = *(_WORD *)(v6 + v1 + 80);
	*(_WORD *)(v1 + 40) = v8;
	*(_WORD *)(v1 + 40) = ntohs(v8);
	v9 = *(_WORD *)(v6 + v1 + 82);
	*(_WORD *)(v1 + 48) = v9;
	v10 = ntohs(v9);
	*(_QWORD *)(v1 + 48) = v10;
	*(_QWORD *)(v1 + 32) = v7 + v1 + 80;
	if (4i64 * (signed int)v5 != -16)
		v11 = v10 + v7;
	else
		LABEL_24:
	v11 = 4 * (signed int)v5 + 12i64;
	v12 = *(_QWORD *)(v1 + 1584);
	if (v11 > v12)
		return 1i64;
	v14 = 0i64;
	v15 = v12 - v11;
	v16 = 0i64;
	if (v4)
	{
		v16 = *(unsigned __int8 *)(v1 + 80) + v12 - 1;
		if (v16 > v15)
			return 0i64;
		v15 -= v16;
	}
	v17 = *(_BYTE *)(v1 + 81);
	*(_DWORD *)(v1 + 4) = (unsigned int)*(unsigned __int8 *)(v1 + 81) >> 7;
	v18 = *(_WORD *)(v1 + 82);
	*(_WORD *)(v1 + 8) = v18;
	*(_BYTE *)v1 = v17 & 0x7F;
	v19 = ntohs(v18);
	v20 = *(_DWORD *)(v1 + 84);
	*(_WORD *)(v1 + 8) = v19;
	*(_DWORD *)(v1 + 12) = v20;
	v21 = ntohl(v20);
	v22 = *(_DWORD *)(v1 + 88);
	*(_DWORD *)(v1 + 16) = v22;
	*(_DWORD *)(v1 + 12) = v21;
	v23 = ntohl(v22);
	v24 = *(void **)(v1 + 24);
	*(_DWORD *)(v1 + 16) = v23;
	if (v24)
		free(v24);
	if ((_DWORD)v5)
	{
		v5 = (signed int)v5;
		*(_QWORD *)(v1 + 24) = *(_QWORD*)malloc(4i64 * (signed int)v5);
		if ((signed int)v5 > 0i64)
		{
			do
			{
				v25 = ntohl(*(_DWORD *)(v1 + 4 * v14++ + 92));
				*(_DWORD *)(*(_QWORD *)(v1 + 24) + 4 * v14 - 4) = v25;
			} while (v14 < v5);
		}
	}
	*(_QWORD *)(v1 + 64) = v15;
	*(_QWORD *)(v1 + 72) = v16;
	*(_QWORD *)(v1 + 56) = v11 + v1 + 80;
	return 1i64;
}

_QWORD * sub_7FFD4E49C4C0(void *Src, size_t Size)
{
	size_t v2; // rdi
	void *v3; // rsi
	_QWORD *v5; // rbx
	void *v6; // rcx

	v2 = Size;
	v3 = Src;
	if (Size > 0x5DC)
		return 0i64;
	v5 = (_QWORD*)malloc(0x648ui64);
	memset(v5, 0, 0x648ui64);
	v5[4] = 0i64;
	v5[3] = 0i64;
	memcpy(v5 + 10, v3, v2);
	v5[198] = v2;
	if ((unsigned int)sub_7FFD4E49C290((__int64)v5))
		return v5;
	v6 = (void *)v5[3];
	if (v6)
		free(v6);
	free(v5);
	return 0i64;
}

void server(void *param) 
{
	int sock = *(int*)param;
	//1、建立接收缓存字节数组
	char recvbuf[32768] = { 0 };
	//2、定义socket
	struct sockaddr_in peeraddr;
	socklen_t peerlen;
	int n = 0;
	while (1) 
	{
		//
		peerlen = sizeof(peeraddr);
		//3、清空接收缓存数组
		memset(recvbuf, 0, sizeof(recvbuf));
		//4、开始接收数据
		n = recvfrom(sock, recvbuf, sizeof(recvbuf), 0, (struct sockaddr *)&peeraddr, &peerlen);
		//5、判断是否接收完整数据
		if (n == -1) 
		{
			perror("recvfrom");
			break;
		}
		else if (n > 0) 
		{
			//
			g_stMiracast.m_callBackVideo(1, recvbuf, n, 0, 0);


			//if (1)
			//{
			//	FILE* fp = fopen("d:/test.ts", "ab");
			//	fwrite(recvbuf + 12, n - 12, 1, fp);
			//	fclose(fp);
			//	continue;
			//}

			//put_queue(&recvqueue, (unsigned char*)recvbuf, n);

			continue;

			//how decode
			LPDWORD v8; // ST28_8
			__int64 v9; // ST38_8
			__int64 v10; // ST30_8
			LPDWORD v11; // ST28_8
			__int64 v44; // [rsp+30h] [rbp-C8h]
			__int64 v45; // [rsp+30h] [rbp-C8h]
			__int64 v46; // [rsp+38h] [rbp-C0h]
			__int64 v47; // [rsp+40h] [rbp-B8h]
			_QWORD* v7 = sub_7FFD4E49C4C0(recvbuf, n);
			LODWORD(v47) = *((_DWORD *)v7 + 3);//timestamp
			LODWORD(v46) = *((unsigned __int16 *)v7 + 4);//sequencenumber
			LODWORD(v44) = *((_DWORD *)v7 + 1);//marked
			LODWORD(v8) = *(unsigned __int8 *)v7;//payload type
			LODWORD(v9) = *((unsigned __int16 *)v7 + 20);//extensionheaderid
			LODWORD(v10) = *((_DWORD *)v7 + 5);//cscrcount
			LODWORD(v11) = *((_DWORD *)v7 + 4);//ssrc
			_QWORD extensionHeaderLength = v7[6];//extensionHeaderLength
			_QWORD PayloadLength = v7[8];//PayloadLength
			_QWORD paddingLength = v7[9];//paddingLength
			printf("Payload Type: %u, Marked: %u, SequenceNumber: %u, Timestamp: %u, SSRC: %lx, CSCRCount: %u, extensionHeaderID: %i, extensionHeaderLength: %i, PayloadLength: %i, paddingLength: %i, n = %d\n",
				v8, v44, v46, v47, v11, v10, v9, v7[6], v7[8], v7[9], n);
		}
	}
	//关闭socket
	closesocket(sock);
}

int sock = -1;

int http_start(httpd_t* httpd, unsigned short *port)
{
	//创建服务器socket
	if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0) 
	{
		perror("socket error");
		return -1;
	}
	int opt = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(int));
	//设置服务器socket参数
	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(*port);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	//将socket绑定地址
	//if (bind(sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
	//	perror("bind error");
	//	return -1;
	//}

	init_queue(&recvqueue, 1024 * 1024 * 10);
	udpParam.queue = &recvqueue;
	udpParam.sock = sock;

	////start receive thread
	//HANDLE hThread2 = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)processdata, &recvqueue, 0, NULL);
	//CloseHandle(hThread2);

	HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)server, &sock, 0, NULL);
	CloseHandle(hThread);

	return 0;
}

int sock2 = -1;
int g_sock = -1;
int g_cseq = -1;
void server2(void *param)
{
	int sock = *(int*)param;
	//1、建立接收缓存字节数组
	char recvbuf[32768] = { 0 };
	//2、定义socket
	struct sockaddr_in peeraddr;
	socklen_t peerlen;
	int n = 0;
	char sendbuf[1024] = { 0 };
	while (1)
	{
		//
		peerlen = sizeof(peeraddr);
		//3、清空接收缓存数组
		memset(recvbuf, 0, sizeof(recvbuf));
		//4、开始接收数据
		//n = recvfrom(sock2, recvbuf, sizeof(recvbuf), 0, (struct sockaddr *)&peeraddr, &peerlen);
		n = recv(g_sock, recvbuf, sizeof(recvbuf), 0);
		//5、判断是否接收完整数据
		if (n == -1)
		{
			perror("recvfrom");
			////////////////////////////////////////////////
			//need IDR
			memset(sendbuf, 0, 1024);
			sprintf(sendbuf, "SET_PARAMETER rtsp://255.255.255.255/wfd1.0/streamid=0 RTSP/1.0\r\n"
				"CSeq: %d\r\n"
				"Content-Type: text/parameters\r\n"
				"Content-Length: 17\r\n\r\nwfd_idr_request\r\n", ++g_cseq);
			std::string header = sendbuf;
			n = send(g_sock, header.c_str(), header.size(), 0);
			/////////////////////////////////////////////////
			continue;
		}
		else if (n > 0)
		{
		//	GET_PARAMETER rtsp ://localhost/wfd1.0 RTSP/1.0
		//CSeq: 4
		//	Session : 00000013
			printf("recvbuf----:%s\n", recvbuf);
			std::string strTmp = recvbuf;
			if (strncmp(recvbuf, "GET_PARAMETER", strlen("GET_PARAMETER")) == 0) 
			{
				memset(sendbuf, 0, 1024);
				int cseq = caclseq(recvbuf);
				sprintf(sendbuf, "RTSP/1.0 200 OK\r\n"
					"CSeq: %d\r\n"
					"Content-Length: 0\r\n\r\n", cseq);
				std::string header = sendbuf;
				//sink -> src set_parameter response
				n = send(g_sock, header.c_str(), header.size(), 0);
				if (n == header.size()) 
				{
					continue;
				}
			}
		}
	}
}

int http_start2(httpd_t* httpd, unsigned short *port)
{
	////创建服务器socket
	//if ((sock2 = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
	//{
	//	perror("socket error");
	//	return -1;
	//}
	//int opt = 1;
	//setsockopt(sock2, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(int));
	////设置服务器socket参数
	//struct sockaddr_in servaddr;
	//memset(&servaddr, 0, sizeof(servaddr));
	//servaddr.sin_family = AF_INET;
	//servaddr.sin_port = htons(*port);
	//servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	////将socket绑定地址
	//if (bind(sock2, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
	//	perror("bind error");
	//	return -1;
	//}

	//start receive thread
	HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)server2, &sock2, 0, NULL);
	CloseHandle(hThread);

	return 0;
}

void sub_7FFB87BD4CF0(LPCRITICAL_SECTION lpCriticalSection, char *Src, unsigned __int16 a3, char *a4)
{
	int v4; // er14
	LPCRITICAL_SECTION v5; // rdi
	char *v6; // rbp
	char *v7; // rsi
	char *v8; // rax
	__int64 v9; // rbx
	__int64 v10; // rbx
	__int64 v11; // rbx
	char *v12; // rax
	unsigned int *v14; // rcx
	__int64 v15; // rdx
	__int64 v16; // [rsp+28h] [rbp-80h]
	__int64 v17; // [rsp+30h] [rbp-78h]
	__int64 v18; // [rsp+30h] [rbp-78h]
	char *v19; // [rsp+40h] [rbp-68h]
	__int16 v20; // [rsp+48h] [rbp-60h]
	char *v21; // [rsp+50h] [rbp-58h]
	__int16 v22; // [rsp+58h] [rbp-50h]
	char pAddrBuf[20] = {0}; // [rsp+5Ch] [rbp-4Ch]

	v4 = a3;
	//v5 = lpCriticalSection;
	v6 = a4;
	v7 = Src;
	v22 = 2;
	inet_pton(2, Src, pAddrBuf);
	//v8 = sub_7FFB87BA2870(v7);
	LODWORD(v17) = v4;
	//sub_7FFB87BA25E0(
	//	(char *)0x12C,
	//	(signed __int64)"Checking for an existing Miracast connection to %s:%i",
	//	(__int64)"RFSERVER",
	//	"Checking for an existing Miracast connection to %s:%i",
	//	v8,
	//	v17);
	//EnterCriticalSection(v5);
	//v9 = (__int64)v5[1].DebugInfo;
	//if (v9)
	//{
	//	while (!sub_7FFB87BB0B20(&v22, v9 + 692) || *(_DWORD *)(v9 + 4) != 2)
	//	{
	//		v9 = *(_QWORD *)(v9 + 8);
	//		if (!v9)
	//			goto LABEL_5;
	//	}
	//	v10 = sub_7FFB87BD1090(v9);
	//}
	//else
	{
	LABEL_5:
		v10 = 0i64;
	}
	//LeaveCriticalSection(v5);
	if (v10)
	{
		//sub_7FFB87BA25E0(
		//	(char *)0x190,
		//	(signed __int64)"Miracast connection already exists!",
		//	(__int64)"RFSERVER",
		//	"Miracast connection already exists!",
		//	0i64);
	}
	else
	{
		//add new connection
		printf("remote ip : %s, remote port : %d\n", Src, a3);

		//0 start udp listen thread, port = 62853
		//need start udp receiver thread
		int error = 0;
		httpd_t *httpd = (httpd_t*)http_init(1, &error);
		unsigned short port = 62853;
		int ret = 0;
		if (g_stMiracast.m_iNoAVData == 0)
		{
			ret = http_start(httpd, &port);
		}
		port = 62854;
		/*ret = http_start2(httpd, &port);*/

		//1 connect to the remote ip:port

		Sleep(2000);

		int sock = -1;
		get_fairplay_socket(Src, a3, &sock);

		char buffer[1024] = { 0 };
		//src -> sink options
		ret = recv(sock, buffer, 1024, 0);
		printf("---------------------------------------------\n");
		printf("buffer : %s\n", buffer);
		printf("---------------------------------------------\n");

		//find cseq
		int cseq = caclseq(buffer);

		char sendbuf[1024] = { 0 };
		sprintf(sendbuf, "RTSP/1.0 200 OK\r\n"
			"CSeq: %d\r\n"
			"Public: org.wfa.wfd1.0, SETUP, TEARDOWN, PLAY, PAUSE, GET_PARAMETER, SET_PARAMETER\r\n\r\n", cseq);
		std::string header(sendbuf);
		//sink -> src options response
		ret = send(sock, header.c_str(), header.size(), 0);
		if (ret == header.size()) 
		{
			header = "OPTIONS * RTSP/1.0\r\n"
				"CSeq: 2\r\n"
				"Require: org.wfa.wfd1.0\r\n\r\n";
			//sink -> src options request
			ret = send(sock, (char*)header.c_str(), header.size(), 0);
			if (ret == header.size()) 
			{
				memset(buffer, 0, 1024);
				//src -> sink options request response
				Sleep(1000);
				ret = recv(sock, buffer, 1024, 0);
				printf("---------------------------------------------\n");
				printf("buffer options recevice : %s\n", buffer);
				printf("---------------------------------------------\n");
				cseq = caclseq(buffer);
				std::string payload_buffer;
				std::string strTmp = buffer;
				memset(sendbuf, 0, 1024);
				//if (strTmp.find("wfd_hwe_version") != -1) 
				//{
				//	//send get_parameter response
				//	sprintf(sendbuf, "RTSP/1.0 200 OK\r\n"
				//		"CSeq: %d\r\n"
				//		"Content-Type: text/parameters\r\n"
				//		"Content-Length: 464\r\n\r\n", cseq);
				//	header = sendbuf;
				//	payload_buffer = "wfd_audio_codecs: LPCM 00000003 00, AAC 00000001 00\r\n"
				//		"wfd_content_protection: none\r\n"
				//		"wfd_video_formats: 40 01 01 10 0001FFFF 0FFFFFFF 00000FFF 0A 0000 0000 00 none none, 02 10 0001FFFF 0FFFFFFF 00000FFF 0A 0000 0000 00 none none\r\n"
				//		"wfd_client_rtp_ports: RTP/AVP/UDP;unicast 62853 0 mode=play\r\n"
				//		"wfd_uibc_capability: none\r\n"
				//		"wfd_hwe_version: none\r\n"
				//		"wfd_hwe_vendor_id: none\r\n"
				//		"wfd_hwe_product_id: none\r\n"
				//		"wfd_hwe_vtp: none\r\n"
				//		"wfd_hwe_video_60fps: none\r\n"
				//		"wfd_hwe_hevc_formats: none\r\n";
				//}
				//else 
				{
					//send get_parameter response
					sprintf(sendbuf, "RTSP/1.0 200 OK\r\n"
						"CSeq: %d\r\n"
						"Content-Type: text/parameters\r\n"
						"Content-Length: 358\r\n\r\n", cseq);
					header = sendbuf;
					payload_buffer = 
						"wfd_client_rtp_ports: RTP\/AVP\/UDP;unicast 62853 0 mode=play\r\n"
						"wfd_audio_codecs: LPCM 00000003 00, AAC 00000001 00\r\n"
						"wfd_video_formats: 40 01 01 10 0001FFFF 0FFFFFFF 00000FFF 0A 0000 0000 00 none none, 02 10 0001FFFF 0FFFFFFF 00000FFF 0A 0000 0000 00 none none\r\n"
						"wfd_uibc_capability: none\r\n"
						"wfd_standby_resume_capability: supported\r\n"
						"wfd_content_protection: none\r\n";
				}

				std::string totalbuf = header + payload_buffer;
				//sink -> src get_parameter response
				ret = send(sock, totalbuf.c_str(), totalbuf.size(), 0);
				if (ret == totalbuf.size()) 
				{
					memset(buffer, 0, 1024);
					//src -> sink set_parameter request
					ret = recv(sock, buffer, 1024, 0);
					printf("buffer set_parameter receive:%s\n", buffer);
					memset(sendbuf, 0, 1024);
					cseq = caclseq(buffer);
					sprintf(sendbuf, "RTSP/1.0 200 OK\r\n"
						"CSeq: %d\r\n"
						"Content-Length: 0\r\n\r\n", cseq);
					header = sendbuf;
					printf("buffer set_parameter response:%s\n", sendbuf);
					//sink -> src set_parameter response
					ret = send(sock, header.c_str(), header.size(), 0);
					if (ret == header.size())
					{
						//
						memset(buffer, 0, 1024);
						//src -> sink set_parameter setup request
						Sleep(2000);
						ret = recv(sock, buffer, 1024, 0);
						if (ret > 0)
						{
							memset(sendbuf, 0, 1024);
							cseq = caclseq(buffer);
						}
						printf("buffer set_parameter setup receive:%s\n", buffer);
						sprintf(sendbuf, "RTSP/1.0 200 OK\r\n"
							"CSeq: %d\r\n"
							"Content-Length: 0\r\n\r\n", cseq);
						header = sendbuf;

						ret = send(sock, header.c_str(), header.size(), 0);

						memset(sendbuf, 0, 1024);
						sprintf(sendbuf, "SETUP rtsp://255.255.255.255/wfd1.0/streamid=0 RTSP/1.0\r\n"
							"CSeq: %d\r\n"
							"Transport: RTP/AVP/UDP;unicast;client_port=62853\r\n"
							"Content-Length: 0\r\n\r\n", cseq);
						payload_buffer = sendbuf;

						totalbuf = header + payload_buffer;
						//sink -> src set_parameter setup response
						ret = send(sock, payload_buffer.c_str(), payload_buffer.size(), 0);
						if (ret == payload_buffer.size())
						{
							memset(buffer, 0, 1024);
							//src -> sink setup request
							Sleep(1000);
							ret = recv(sock, buffer, 1024, 0);
							if (ret > 0)
							{
								memset(sendbuf, 0, 1024);
								cseq = caclseq(buffer);
							}
							printf("buffer setup receive:%s\n", buffer);

							memset(sendbuf, 0, 1024);
							cseq = caclseq(buffer);
							sprintf(sendbuf, "PLAY rtsp://255.255.255.255/wfd1.0/streamid=0 RTSP/1.0\r\n"
								"CSeq: %d\r\n"
								"Content-Length: 0\r\n\r\n", cseq);
							header = sendbuf;
							//sink -> src play request
							ret = send(sock, header.c_str(), header.size(), 0);
							if (ret == header.size())
							{
								memset(buffer, 0, 1024);
								//src -> sink play response
								ret = recv(sock, buffer, 1024, 0);
								printf("buffer play receive:%s\n", buffer);
								if (ret > 0)
								{
									g_sock = sock;
									memset(sendbuf, 0, 1024);
									cseq = caclseq(buffer);
									g_cseq = cseq;
									//////////////////////////////////////////////////
									////need IDR
									//memset(sendbuf, 0, 1024);
									//cseq = caclseq(buffer);
									//sprintf(sendbuf, "SET_PARAMETER rtsp://255.255.255.255/wfd1.0/streamid=0 RTSP/1.0\r\n"
									//	"CSeq: %d\r\n"
									//	"Content-Type: text/parameters\r\n"
									//	"Content-Length: 17\r\n\r\nwfd_idr_request\r\n", cseq + 1);
									//header = sendbuf;
									//ret = send(sock, header.c_str(), header.size(), 0);
									//if (ret == header.size())
									//{
									//	memset(buffer, 0, 1024);
									//	//src -> sink play response
									//	ret = recv(sock, buffer, 1024, 0);
									//	printf("buffer SET_PARAMETER receive:%s\n", buffer);
									//	if (ret > 0) 
									//	{
									//		printf("buffer SET_PARAMETER receive:%s\n", buffer);
									//	}
									//}
									///////////////////////////////////////////////////

									ret = http_start2(httpd, &port);
									printf("**********************it's successful*********************\n");
								}
							}
						}
					}

				}
				printf("\n");
			}

		}

		//v11 = sub_7FFB87BA27F0(v6);
		//v12 = sub_7FFB87BA2870(v7);
		//sub_7FFB87BA25E0(
		//	(char *)0x12C,
		//	(signed __int64)"Adding new Miracast connection to %s/%s",
		//	(__int64)"RFSERVER",
		//	"Adding new Miracast connection to %s/%s",
		//	v12,
		//	v11);
		//v10 = sub_7FFB87BDAE80(v5, (__int64 *)&v21, 2u);
		//if (!v10)
		//{
		//	//LODWORD(v18) = HIDWORD(v5[30].LockSemaphore);
		//	//LODWORD(v16) = HIDWORD(v5[30].LockSemaphore);
		//	//return /*sub_7FFB87BA25E0(
		//	//	(char *)0x190,
		//	//	(signed __int64)"Miracast connection ignored because %i/%i connections are already active",
		//	//	(__int64)"RFSERVER",
		//	//	"Miracast connection ignored because %i/%i connections are already active",
		//	//	v16,
		//	//	v18)*/;
		//}
		//v14 = (unsigned int *)&v5[25].DebugInfo->Type;
		//v19 = v7;
		//v20 = v4;
		//v21 = v6;
		//if (v14)
		//{
		//	//if ((unsigned int)sub_7FFB87BB65E0(v14) == 2
		//	//	|| (unsigned int)sub_7FFB87BB65E0((unsigned int *)&v5[25].DebugInfo->Type) == 3)
		//	//{
		//	//	//sub_7FFB87BA25E0(
		//	//	//	(char *)0xC8,
		//	//	//	(signed __int64)"Finalizing a recording - cannot add a miracast client to the recording now",
		//	//	//	(__int64)"RFSERVER",
		//	//	//	"Finalizing a recording - cannot add a miracast client to the recording now",
		//	//	//	0i64);
		//	//}
		//	//else
		//	//{
		//	//	LOBYTE(v15) = 1;
		//	//	sub_7FFB87BD1C70(v10, v15);
		//	//}
		//}
		//sub_7FFB87BD1DF0(v10, (int)v5[32].OwningThread);
		//*(_DWORD *)(v10 + 1088) = v5[29].SpinCount;
		//*(_DWORD *)(v10 + 1028) = v5[30].SpinCount;
		//*(_QWORD *)(v10 + 1032) = (_QWORD)v5[31].DebugInfo;
		//*(_QWORD *)(v10 + 1040) = *(_QWORD *)&v5[31].LockCount;
		//*(_QWORD *)(v10 + 1048) = (_QWORD)v5[31].OwningThread;
		//*(_QWORD *)(v10 + 1056) = (_QWORD)v5[31].LockSemaphore;
		//*(_QWORD *)(v10 + 1064) = (_QWORD)v5[31].SpinCount;
		//*(_QWORD *)(v10 + 1072) = (_QWORD)v5[32].DebugInfo;
		//*(_DWORD *)(v10 + 1080) = (_QWORD)v5[32].LockCount;
		//*(_BYTE *)(v10 + 1084) = v5[32].RecursionCount;
		//*(_QWORD *)(v10 + 528) = (_QWORD)v5[22].DebugInfo;
		//*(_QWORD *)(v10 + 536) = *(_QWORD *)&v5[22].LockCount;
		//*(_BYTE *)(v10 + 544) = (_QWORD)v5[22].OwningThread;
		//*(_DWORD *)(v10 + 548) = HIDWORD(v5[22].OwningThread);
		//*(_DWORD *)(v10 + 552) = (_QWORD)v5[22].LockSemaphore;
		//*(_BYTE *)(v10 + 556) = BYTE4(v5[22].LockSemaphore);
		//*(_DWORD *)(v10 + 24) = HIDWORD(v5[33].DebugInfo);
		//*(_QWORD *)(v10 + 712) = (_QWORD)sub_7FFB8794F080(v7, 0i64);
		//sub_7FFB87BCAD20(v10, 512, 0, (__int64)&v19);

		//sub_7FFB89B73210(0, Src, 0, a3, 0, 0);

	}
	//*(_BYTE *)(v10 + 804) = 0;
	//if (!*(_DWORD *)(v10 + 20))
	//	sub_7FFB87BCEF50(v10, 4i64);
	//sub_7FFB87BD0C90((void *)v10);
}

void sub_7FFB87BDD2F0(LPCRITICAL_SECTION lpCriticalSection, __int64 a2, __int64 a3, __int64 a4, struct _RTL_CRITICAL_SECTION *lpCriticalSectiona)
{
	char *v5; // rbx
	unsigned __int16 v6; // di
	__int64 v7; // rsi
	char *v8; // rax

	printf("sub_7FFB87BDD2F0 callback begin\n");

	v5 = (char *)a4;
	v6 = a3;
	v7 = a2;
	sub_7FFB87948730(a2);
	v8 = (char *)sub_7FFB8794E300(v7);
	sub_7FFB87BD4CF0(lpCriticalSectiona, v8, v6, v5);
	sub_7FFB87948710(v7);
}

//int sub_7FFFC6D5D2F0(LPCRITICAL_SECTION lpCriticalSection, __int64 a2, __int64 a3, __int64 a4, struct _RTL_CRITICAL_SECTION *lpCriticalSectiona)
//{
//	return 0;
//}

__int64 sub_7FFB8794E300(__int64 a1)
{
	__int64 result; // rax

	if (a1)
		result = *(_QWORD *)(a1 + 24);
	else
		result = 0i64;
	return result;
}

bool sub_7FFB87BB0B20(__int16 *a1, __int64 a2)
{
	__int16 v2; // ax
	__int64 v4; // rax

	v2 = *a1;
	if (*a1 != *(_WORD *)a2)
		return 0;
	if (v2 == 2)
		return *((_DWORD *)a1 + 1) == *(_DWORD *)(a2 + 4);
	if (v2 != 23)
		return 0;
	v4 = *((_QWORD *)a1 + 1) - *(_QWORD *)(a2 + 8);
	if (*((_QWORD *)a1 + 1) == *(_QWORD *)(a2 + 8))
		v4 = *((_QWORD *)a1 + 2) - *(_QWORD *)(a2 + 16);
	return v4 == 0;
}

__int64 sub_7FFB87BD1090(__int64 a1)
{
	//_InterlockedIncrement((volatile signed __int32 *)(a1 + 16));
	return a1;
}

__int64 sub_7FFB87BCFCD0(__int64 a1)
{
	return *(unsigned __int8 *)(a1 + 560);
}

__int64 sub_7FFB87BCFCE0(__int64 a1)
{
	return *(unsigned __int8 *)(a1 + 557);
}

void sub_7FFB87BD1C20(__int64 a1, char a2)
{
	*(_BYTE *)(a1 + 560) = a2;
}

__int64 sub_7FFB87BBFBD0(__int64 a1)
{
	__int64 result; // rax

	for (result = 0i64; a1; ++result)
		a1 = *(_QWORD *)(a1 + 352);
	return result;
}

__int64 sub_7FFB87BBA6F0(__int64 a1)
{
	return sub_7FFB87BBFBD0(*(_QWORD *)(a1 + 56));
}

__int64 sub_7FFB87BC04E0(__int64 a1)
{
	return *(_QWORD *)(a1 + 352);
}

__int64 sub_7FFB87BBA570(__int64 a1, int a2)
{
	__int64 result; // rax
	int v3; // ebx
	int v4; // edi

	result = *(_QWORD *)(a1 + 56);
	v3 = 0;
	v4 = a2;
	if (a2)
	{
		do
		{
			if (!result)
				break;
			++v3;
			result = sub_7FFB87BC04E0(result);
		} while (v3 != v4);
	}
	return result;
}

__int64 sub_7FFB87BC0BC0(__int64 a1)
{
	return *(_QWORD *)(a1 + 112);
}

void sub_7FFB87BB7A10(__int64 a1, int a2)
{
	__int64 v2; // rsi
	__int64 v3; // rdi
	__int64 v4; // rdi
	__int64 v5; // rbx

	v2 = a1;
	v3 = a2;
	EnterCriticalSection((LPCRITICAL_SECTION)(a1 + 24));
	v4 = *(_QWORD *)(v2 + 8 * v3 + 120);
	LeaveCriticalSection((LPCRITICAL_SECTION)(v2 + 24));
	if (v4)
	{
		if (sub_7FFB87BBA6F0(v4) == 1)
		{
			v5 = sub_7FFB87BBA570(v4, 0i64);
			if (!sub_7FFB87BC0BC0(v5))
			{
				//sub_7FFB87BA25E0(
				//	(char *)0x1F4,
				//	(signed __int64)"Encountered an empty track. Exploding...",
				//	(__int64)"RECORDING",
				//	"Encountered an empty track. Exploding...",
				//	0i64);
				//sub_7FFB87BC00E0(v5);
			}
		}
		//sub_7FFB87BBAB90(v4);
	}
}

void sub_7FFB87BDF450(struct _RTL_CRITICAL_SECTION *a1, signed int *a2)
{
	signed int *v2; // rbx
	struct _RTL_CRITICAL_SECTION *v3; // rdi
	__int64 v4; // rsi
	signed int **v5; // rdx
	signed int **v6; // rcx
	__int64 v7; // [rsp+28h] [rbp-10h]

	v2 = a2;
	v3 = a1;
	EnterCriticalSection(a1);
	LODWORD(v7) = *v2;
	//sub_7FFB87BA25E0(
	//	(char *)0x12C,
	//	(signed __int64)"Removing client %i from the list",
	//	(__int64)"RFSERVER",
	//	"Removing client %i from the list",
	//	v7);
	v4 = *v2;
	v5 = (signed int **)&v3[1];
	if (v3[1].DebugInfo)
	{
		while (1)
		{
			v6 = (signed int **)*v5;
			if (*v5 == v2)
				break;
			v5 = v6 + 1;
			if (!v6[1])
				goto LABEL_6;
		}
		*v5 = v6[1];
		//sub_7FFB87BD0C90(v6);
		--v3[1].LockCount;
	}
LABEL_6:
	*(&v3[1].RecursionCount + v4) = 0;
	LeaveCriticalSection(v3);
}

LARGE_INTEGER sub_7FFB8794FAC0()
{
	LARGE_INTEGER PerformanceCount; // [rsp+30h] [rbp+8h]

	QueryPerformanceCounter(&PerformanceCount);
	return PerformanceCount;
}

char __fastcall sub_7FFB87C00A70(LPCRITICAL_SECTION lpCriticalSection) 
{
	return 0;
}

void __fastcall sub_7FFB87933E00(__int64 a1, _BYTE *a2) 
{
	while (1) 
	{
		Sleep(2000);//?????
	}
}

__int64 sub_7FFB87BAA4E0(LPVOID lpThreadParameter)
{
	//start sink run loop

	__int64 *v1; // rbx
	__int64 v2; // rcx

	v1 = (__int64 *)lpThreadParameter;
	//sub_7FFB87BA25E0((char *)0xC8, (signed __int64)"Started sink run loop", (__int64)"MIRASINK", "Started sink run loop");
	v2 = *v1;
	*((_BYTE *)v1 + 8) = 0;
	sub_7FFB87933E00(v2, (_BYTE *)v1 + 8);
	//sub_7FFB87BA25E0(
	//	(char *)0xC8,
	//	(signed __int64)"Sink run loop completed",
	//	(__int64)"MIRASINK",
	//	"Sink run loop completed");
	*((_BYTE *)v1 + 24) = 0;
	return 0i64;
}

void sub_7FFB87C053F0(signed __int64 a1, unsigned int a2, __int64 a3, __int64 a4, void *a5) 
{

}

__int64 sub_7FFB87C00410(_QWORD *a1, __int64 a2, __int64 a3, __int64 a4, __int64 a5, __int64 a6, __int64 a7, __int64 a8, __int64 a9, __int64 a10)
{
	__int64 result = 0; // rax

	a1[8314] = a2;
	a1[8313] = a3;
	a1[8315] = (_QWORD)sub_7FFB87C053F0;
	a1[8311] = a4;
	a1[8316] = a6;
	a1[8310] = a10;
	a1[8319] = a5;
	a1[8320] = a7;
	a1[8321] = a8;
	result = a9;
	a1[8322] = a9;
	return result;
}

void sub_7FFB87C020D0(__int64 a1, _BYTE *a2)
{
	__int64 v2; // rsi
	void *v3; // rcx
	_BYTE *v4; // rbp
	size_t v5; // rbx
	signed __int64 v6; // rax
	void *v7; // rax

	if (a1)
	{
		v2 = a1;
		v3 = *(void **)(a1 + 66472);
		v4 = a2;
		if (v3)
		{
			free(v3);
			*(_QWORD *)(v2 + 66472) = 0i64;
		}
		if (v4)
		{
			v5 = -1i64;
			v6 = -1i64;
			do
				++v6;
			while (v4[v6]);
			if (v6)
			{
				do
					++v5;
				while (v4[v5]);
				v7 = malloc(v5 + 1);
				*(_QWORD *)(v2 + 66472) = (_QWORD)v7;
				memset(v7, 0, v5 + 1);
				memcpy(*(void **)(v2 + 66472), v4, v5);
			}
		}
		*(_BYTE *)(v2 + 176) = 1;
		*(_BYTE *)(v2 + 832) = 1;
		*(_BYTE *)(v2 + 968) = 1;
		*(_BYTE *)(v2 + 1680) = 1;
	}
}

void sub_7FFB87C01E90(__int64 a1, int a2)
{
	*(_DWORD *)(a1 + 66468) = a2;
}

char sub_7FFB87BFF1F0(void *a1, char *a2, __int16 a3, u_short a4, int a5, char a6)
{
	return sub_7FFB89B73210(a1, a2, a3, a4, a5, a6);
}

int sub_7FFB87C021A0(__int64 a1, char a2, int a3, int a4, int a5)
{
	int result; // eax
	__int64 v6; // rdi
	int v7; // esi
	__int64 v8; // rbx
	SOCKET v9; // rcx
	int *v10; // rax
	signed __int64 v11; // rax
	SOCKET v12; // rcx
	signed __int64 v13; // rax
	signed __int64 v14; // rax
	SOCKET v15; // rcx
	signed __int64 v16; // rax
	__int64 cbOutBuffer; // [rsp+28h] [rbp-490h]
	char optval[4]; // [rsp+50h] [rbp-468h]
	DWORD lpcbBytesReturned; // [rsp+54h] [rbp-464h]
	DWORD cbBytesReturned; // [rsp+58h] [rbp-460h]
	int vInBuffer; // [rsp+60h] [rbp-458h]
	int v22; // [rsp+64h] [rbp-454h]
	int v23; // [rsp+68h] [rbp-450h]
	int v24; // [rsp+70h] [rbp-448h]
	char Buf; // [rsp+80h] [rbp-438h]

	result = a5;
	v6 = a1;
	*(_BYTE *)(a1 + 66453) = a2;
	*(_DWORD *)(a1 + 66456) = a3;
	*(_DWORD *)(a1 + 66460) = a4;
	*(_DWORD *)(a1 + 66464) = a5;
	v7 = 0;
	if (*(_DWORD *)(a1 + 2316) >= 0)
	{
		v8 = a1 + 2704;
		do
		{
			if (*(_BYTE *)(v8 + 117))
			{
				result = __WSAFDIsSet(*(_QWORD *)v8, (fd_set *)(v6 + 184));
				if (!result)
				{
					v9 = *(_QWORD *)v8;
					if (*(_BYTE *)(v6 + 66453))
					{
						*(_DWORD *)optval = 1;
						if (setsockopt(v9, 0xFFFF, 8, optval, 4))
						{
							//v10 = errno();
							//strerror_s(&Buf, 0x400ui64, *v10);
							//v11 = (unsigned int)*errno();
							//LODWORD(cbOutBuffer) = v11;
							//sub_7FFB87BA25E0(
							//	(char *)0x1F4,
							//	v11,
							//	(__int64)"SOCKETSET",
							//	"Failed to set SO_KEEPALIVE on socket: (%i) %s",
							//	cbOutBuffer,
							//	&Buf);
						}
						v12 = *(_QWORD *)v8;
						v22 = 1000 * *(_DWORD *)(v6 + 66456);
						v23 = 1000 * *(_DWORD *)(v6 + 66464);
						vInBuffer = 1;
						result = WSAIoctl(v12, 0x98000004, &vInBuffer, 0xCu, 0i64, 0, &cbBytesReturned, 0i64, 0i64);
						if (result == -1)
						{
							LODWORD(v13) = WSAGetLastError();
							LODWORD(cbOutBuffer) = v13;
							//result = sub_7FFB87BA25E0(
							//	(char *)0x1F4,
							//	v13,
							//	(__int64)"SOCKETSET",
							//	"Failed to enable Win32 Keep Alive settings: %i",
							//	cbOutBuffer);
						}
					}
					else
					{
						*(_DWORD *)optval = 0;
						LODWORD(v14) = setsockopt(v9, 0xFFFF, 8, optval, 4);
						if ((_DWORD)v14)
							;// sub_7FFB87BA25E0((char *)0x1F4, v14, (__int64)"SOCKETSET", "Failed to unset SO_KEEPALIVE on socket", 0i64);
						v15 = *(_QWORD *)v8;
						v24 = 0;
						result = WSAIoctl(v15, 0x98000004, &v24, 0xCu, 0i64, 0, &lpcbBytesReturned, 0i64, 0i64);
						if (result == -1)
						{
							LODWORD(v16) = WSAGetLastError();
							LODWORD(cbOutBuffer) = v16;
							//result = sub_7FFB87BA25E0(
							//	(char *)0x1F4,
							//	v16,
							//	(__int64)"SOCKETSET",
							//	"Failed to disable Win32 Keep Alive settings: %i",
							//	cbOutBuffer);
						}
					}
				}
			}
			++v7;
			v8 += 664i64;
		} while (v7 <= *(_DWORD *)(v6 + 2316));
	}
	return result;
}

__int64(__fastcall *__fastcall sub_7FFB87BAB4E0(LPCRITICAL_SECTION lpCriticalSection, int a2, __int64 a3))(__int64, _QWORD)
{
	int v3; // er14
	__int64 v4; // rdi
	struct _RTL_CRITICAL_SECTION *v5; // rsi
	signed int v6; // eax
	char *v7; // rbp
	char *v8; // rax
	__int64 v9; // rsi
	__int64 v10; // rbx
	__int64 v11; // ST28_8
	__int64(__fastcall *result)(__int64, _QWORD); // rax
	void(__fastcall *v13)(__int64, _QWORD); // rax
	char *v14; // rbp
	char *v15; // rax
	__int64 v16; // rsi
	__int64 v17; // rbx
	__int64 v18; // ST28_8
	void(__fastcall *v19)(__int64, signed __int64, _QWORD); // rax
	__int64 v20; // [rsp+28h] [rbp-20h]
	__int64 v21; // [rsp+28h] [rbp-20h]
	__int64 v22; // [rsp+28h] [rbp-20h]
	__int64 v23; // [rsp+30h] [rbp-18h]

	LODWORD(v20) = a2;
	v3 = a2;
	v4 = a3;
	v5 = lpCriticalSection;
	//sub_7FFB87BA25E0(
	//	(char *)0x12C,
	//	(signed __int64)"SQMiracastSink - Disconnect %i",
	//	(__int64)"MIRASINK",
	//	"SQMiracastSink - Disconnect %i",
	//	v20);
	if (v3 != 12293 || (v6 = *(_DWORD *)(v4 + 4168), v6 >= 10) || *(_BYTE *)(v4 + 94))
	{
		v13 = *(void(__fastcall **)(__int64, _QWORD))(v4 + 4000);
		if (v13)
			v13(v4, *(_QWORD *)(v4 + 3936));
		if (*(_DWORD *)(v4 + 84) != 6)
			*(_DWORD *)(v4 + 84) = 6;
		if (*(_DWORD *)(v4 + 88))
			*(_DWORD *)(v4 + 88) = 0;
		*(LARGE_INTEGER *)(v4 + 3928) = sub_7FFB8794FAC0();
		//v14 = sub_7FFB8794F8E0((__int64)"MiracastStateDisconnected");
		//v15 = sub_7FFB8794F8E0((__int64)"MiracastSinkStateNew");
		//v16 = (__int64)v15;
		//if (v14 && v15)
		//{
		//	v17 = sub_7FFB8794E300((__int64)v15);
		//	v18 = sub_7FFB8794E300((__int64)v14);
		//	//sub_7FFB87BA25E0(
		//	//	(char *)0x12C,
		//	//	(signed __int64)"Miracast state now %s, %s",
		//	//	(__int64)"MIRASINK",
		//	//	"Miracast state now %s, %s",
		//	//	v18,
		//	//	v17);
		//}
		//else
		//{
		//	LODWORD(v23) = 0;
		//	LODWORD(v21) = 6;
		//	//sub_7FFB87BA25E0(
		//	//	(char *)0x12C,
		//	//	(signed __int64)"Miracast state now %i, %i",
		//	//	(__int64)"MIRASINK",
		//	//	"Miracast state now %i, %i",
		//	//	v21,
		//	//	v23);
		//}
		//if (v16)
		//	sub_7FFB87948710(v16);
		//if (v14)
		//	sub_7FFB87948710((__int64)v14);
		v19 = *(void(__fastcall **)(__int64, signed __int64, _QWORD))(v4 + 4008);
		if (v19)
		{
			v19(v4, 4i64, *(_QWORD *)(v4 + 3936));
			if (!*(_BYTE *)(v4 + 93))
			{
				if (v3)
					(*(void(__fastcall **)(__int64, signed __int64, _QWORD))(v4 + 4008))(v4, 5i64, *(_QWORD *)(v4 + 3936));
			}
		}
		result = *(__int64(__fastcall **)(__int64, _QWORD))(v4 + 4016);
		if (result)
			result = (__int64(__fastcall *)(__int64, _QWORD))result(v4, *(_QWORD *)(v4 + 3936));
	}
	else
	{
		LODWORD(v21) = v6 + 1;
		//sub_7FFB87BA25E0(
		//	(char *)0x1F4,
		//	(signed __int64)"MiracastSink Connection Failed, Retrying ( %i / 10 )",
		//	(__int64)"MIRASINK",
		//	"MiracastSink Connection Failed, Retrying ( %i / 10 )",
		//	v21);
		*(_QWORD *)(v4 + 32) = 0i64;
		sub_7FFB87C00A70(v5);
		++*(_DWORD *)(v4 + 4168);
		Sleep(0xFAu);
		if (*(_DWORD *)(v4 + 84) != 1)
			*(_DWORD *)(v4 + 84) = 1;
		if (*(_DWORD *)(v4 + 88))
			*(_DWORD *)(v4 + 88) = 0;
		*(LARGE_INTEGER *)(v4 + 3928) = sub_7FFB8794FAC0();
		/*v7 = sub_7FFB8794F8E0((__int64)"MiracastStateNew");
		v8 = sub_7FFB8794F8E0((__int64)"MiracastSinkStateNew");
		v9 = (__int64)v8;
		if (v7 && v8)
		{
			v10 = sub_7FFB8794E300((__int64)v8);
			v11 = sub_7FFB8794E300((__int64)v7);
			sub_7FFB87BA25E0(
				(char *)0x12C,
				(signed __int64)"Miracast state now %s, %s",
				(__int64)"MIRASINK",
				"Miracast state now %s, %s",
				v11,
				v10);
		}
		else
		{
			LODWORD(v23) = 0;
			LODWORD(v22) = 1;
			sub_7FFB87BA25E0(
				(char *)0x12C,
				(signed __int64)"Miracast state now %i, %i",
				(__int64)"MIRASINK",
				"Miracast state now %i, %i",
				v22,
				v23);
		}
		if (v9)
			sub_7FFB87948710(v9);
		if (v7)
			sub_7FFB87948710((__int64)v7);*/
		result = (__int64(__fastcall *)(__int64, _QWORD))sub_7FFB87BA4420((LPVOID)v4);
	}
	return result;
}

LARGE_INTEGER __fastcall sub_7FFB87BAB360(__int64 a1, __int64 a2, LARGE_INTEGER *a3)
{
	bool v3; // zf
	LARGE_INTEGER *v4; // rdi
	char *v5; // rbp
	char *v6; // rax
	__int64 v7; // rsi
	__int64 v8; // rbx
	__int64 v9; // rax
	void(__fastcall *v10)(LARGE_INTEGER *, signed __int64, LARGE_INTEGER); // rax
	LARGE_INTEGER result = { 0 }; // rax
	__int64 v12; // [rsp+28h] [rbp-20h]
	__int64 v13; // [rsp+30h] [rbp-18h]

	v3 = a3[10].HighPart == 3;
	v4 = a3;
	a3[521].LowPart = 0;
	if (!v3)
		a3[10].HighPart = 3;
	if (a3[11].LowPart != 2)
		a3[11].LowPart = 2;
	a3[491] = sub_7FFB8794FAC0();
	/*v5 = sub_7FFB8794F8E0((__int64)"MiracastStateConnected");
	v6 = sub_7FFB8794F8E0((__int64)"MiracastSinkStateConnected");
	v7 = (__int64)v6;
	if (v5 && v6)
	{
		v8 = sub_7FFB8794E300((__int64)v6);
		v9 = sub_7FFB8794E300((__int64)v5);
		sub_7FFB87BA25E0(
			(char *)0x12C,
			(signed __int64)"Miracast state now %s, %s",
			(__int64)"MIRASINK",
			"Miracast state now %s, %s",
			v9,
			v8);
	}
	else
	{
		LODWORD(v13) = 2;
		LODWORD(v12) = 3;
		sub_7FFB87BA25E0(
			(char *)0x12C,
			(signed __int64)"Miracast state now %i, %i",
			(__int64)"MIRASINK",
			"Miracast state now %i, %i",
			v12,
			v13);
	}
	if (v7)
		sub_7FFB87948710(v7);
	if (v5)
		sub_7FFB87948710((__int64)v5);*/
	//v10 = (void(__fastcall *)(LARGE_INTEGER *, signed __int64, LARGE_INTEGER))v4[501].QuadPart;
	//if (v10)
	//	((void(__fastcall *)(_QWORD, _QWORD, _QWORD))v10)(v4, 1i64, (LARGE_INTEGER)v4[492].QuadPart);
	//result.QuadPart = sub_7FFB87934070(v4->QuadPart, (unsigned __int64)sub_7FFB87BACDE0, (__int64)v4, 0);
	//v4[489] = result;
	return result;
}

signed __int64 sub_7FFB87C00F20(__int64 a1, int a2)
{
	__int64 v2; // r10
	int v3; // er9
	__int64 v4; // r8
	__int64 v5; // rax
	signed __int64 result; // rax
	signed __int64 *v7; // rax

	v2 = *(signed int *)(a1 + 2316);
	v3 = 0;
	v4 = 0i64;
	if (v2 < 0)
		goto LABEL_6;
	v5 = a1 + 2712;
	while (!*(_BYTE *)(v5 + 109) || *(_DWORD *)v5 != a2)
	{
		++v4;
		++v3;
		v5 += 664i64;
		if (v4 > v2)
			goto LABEL_6;
	}
	v7 = (signed __int64 *)(664i64 * v3 + a1 + 2704);
	if (v7)
		result = *v7;
	else
		LABEL_6:
	result = -1i64;
	return result;
}

int sub_7FFB87BAB190(__int64 a1, int a2, unsigned int a3, __int64 a4, __int64 a5)
{
	unsigned int v5; // esi
	__int64 v7; // rbx
	signed __int64 v8; // rax
	int v9; // edi
	signed __int64 v10; // r8
	signed __int64 v11; // rax
	int v12; // edi
	__int64 v13; // [rsp+28h] [rbp-10h]

	v5 = a3;
	if (a2 == 52)
	{
		v7 = a5;
		v11 = sub_7FFB87C00F20(*(_QWORD *)(a5 + 32), 80);
		v12 = v11;
		if (v11 != -1)
		{
			sub_7FFB89B713F0(*(LPCRITICAL_SECTION *)(a5 + 32), 80);
			closesocket(v12);
		}
		v10 = 80i64;
	}
	else
	{
		if (a2 != 53)
		{
			LODWORD(v13) = a2;
			return 0;/*sub_7FFB87BA25E0(
				(char *)0x1F4,
				(signed __int64)"Asked to accept inbound TCP connection from an unknown parent: 0x%02",
				(__int64)"MIRASINK",
				"Asked to accept inbound TCP connection from an unknown parent: 0x%02",
				v13);*/
		}
		v7 = a5;
		v8 = sub_7FFB87C00F20(*(_QWORD *)(a5 + 32), 81);
		v9 = v8;
		if (v8 != -1)
		{
			sub_7FFB89B713F0(*(LPCRITICAL_SECTION *)(a5 + 32), 81);
			closesocket(v9);
		}
		v10 = 81i64;
	}
	return 0;// sub_7FFB87C00810(*(_QWORD *)(v7 + 32), v5, v10, 0i64);
}

char *sub_7FFB87BF89D0()
{
	char *v0; // rbx

	v0 = (char *)malloc(0xA2E8ui64);
	memset(v0, 0, 0xA2E8ui64);
	InitializeCriticalSection((LPCRITICAL_SECTION)(v0 + 41648));
	*((_QWORD *)v0 + 5211) = 20000i64;
	*((_WORD *)v0 + 1606) = 0;
	*((_QWORD *)v0 + 5212) = 20000i64;
	return v0;
}

void sub_7FFB87BF9640(__int64 a1, __int64 a2, __int64 a3)
{
	*(_QWORD *)(a1 + 41688) = a2;
	*(_QWORD *)(a1 + 41696) = a3;
}

void sub_7FFB87BF9670(__int64 a1, char a2)
{
	*(_BYTE *)(a1 + 128) = a2;
}

__int64 sub_7FFB87BF9650(_QWORD *a1, __int64 a2, __int64 a3, __int64 a4, __int64 a5)
{
	__int64 result; // rax

	result = a5;
	*a1 = a2;
	a1[1] = a3;
	a1[3] = a5;
	a1[2] = a4;
	return result;
}

signed __int64 sub_7FFB87C04CE0(__int64 a1, int a2)
{
	__int64 v2; // r11
	int v3; // er10
	__int64 v4; // r9
	__int64 v5; // r8

	v2 = *(signed int *)(a1 + 2316);
	v3 = 0;
	v4 = 0i64;
	if (v2 < 0)
		return 0xFFFFFFFFi64;
	v5 = a1 + 2704;
	while (!*(_BYTE *)(v5 + 117) || *(_QWORD *)v5 != a2)
	{
		++v4;
		++v3;
		v5 += 664i64;
		if (v4 > v2)
			return 0xFFFFFFFFi64;
	}
	return *(unsigned int *)(664i64 * v3 + a1 + 2712);
}

void sub_7FFB87C06BC0(__int64 a1, int a2, unsigned int a3, __int64 a4)
{
	void(__fastcall *v4)(__int64, _QWORD, _QWORD, __int64, _QWORD); // rdi
	__int64 v5; // rsi
	unsigned int v6; // ebp
	__int64 v7; // rbx
	unsigned int v8; // eax

	v4 = *(void(__fastcall **)(__int64, _QWORD, _QWORD, __int64, _QWORD))(a1 + 66528);
	v5 = a4;
	v6 = a3;
	v7 = a1;
	if (v4)
	{
		v8 = sub_7FFB87C04CE0(a1, a2);
		v4(v7, v8, v6, v5, *(_QWORD *)(v7 + 66480));
	}
}

__int64 sub_7FFB87C085C0(LPCRITICAL_SECTION lpCriticalSection, __int64 a2, __int64 a3, signed __int64 a4)
{
	signed __int64 v4; // rsi
	__int64 v5; // r15
	__int64 v6; // rbp
	struct _RTL_CRITICAL_SECTION *v7; // r14
	signed __int64 v8; // rdi
	int v9; // ebx
	__int64 result; // rax
	__int64 v11; // rax
	int v12; // eax
	signed __int64 v13 = 0; // rax
	__int64 v14; // [rsp+28h] [rbp-20h]

	v4 = a4;
	v5 = a3;
	v6 = a2;
	v7 = lpCriticalSection;
	EnterCriticalSection(lpCriticalSection);
	v8 = -1i64;
	v9 = 0;
	if (!v4)
		;//sub_7FFB87BA25E0(
		//(char *)0xC8,
		//	(signed __int64)"SSL Write Length 0",
		//	(__int64)"SOCKETSET",
		//	"SSL Write Length 0",
		//	0i64);
	if (v6)
	{
		if (v4 <= 0)
		{
		LABEL_9:
			LeaveCriticalSection(v7);
			result = v8;
		}
		else
		{
			v11 = 0i64;
			//while (1)
			//{
			//	v12 = sub_7FFB87E36A60(v6, v11 + v5, (unsigned int)v4);
			//	v8 = v12;
			//	if (v12 <= 0)
			//		break;
			//	v9 += v12;
			//	v11 = v9;
			//	if (v9 >= v4)
			//		goto LABEL_9;
			//}
			//v13 = sub_7FFB87E34C90(v6);
			LODWORD(v14) = v13;
			//sub_7FFB87BA25E0((char *)0x1F4, v13, (__int64)"SOCKETSET", "SSL Write Failed on: %i", v14);
			LeaveCriticalSection(v7);
			result = -1i64;
		}
	}
	else
	{
		//sub_7FFB87BA25E0(
		//	(char *)0xC8,
		//	(signed __int64)"SSL Write without a context",
		//	(__int64)"SOCKETSET",
		//	"SSL Write without a context");
		LeaveCriticalSection(v7);
		result = 0i64;
	}
	return result;
}

signed __int64 sub_7FFB87C04C80(__int64 a1, int a2)
{
	__int64 v2; // r10
	int v3; // er9
	__int64 v4; // r8
	signed __int64 v5; // rax

	v2 = *(signed int *)(a1 + 2316);
	v3 = 0;
	v4 = 0i64;
	if (v2 < 0)
		return 0i64;
	v5 = a1 + 2712;
	while (!*(_BYTE *)(v5 + 109) || *(_DWORD *)v5 != a2)
	{
		++v4;
		++v3;
		v5 += 664i64;
		if (v4 > v2)
			return 0i64;
	}
	return 664i64 * v3 + a1 + 2704;
}

void sub_7FFB879343F0(__int64 a1)
{
	__int64 v1; // rbx

	v1 = a1;
	EnterCriticalSection((LPCRITICAL_SECTION)(a1 + 8));
	if (*(_DWORD *)v1 > *(_DWORD *)(v1 + 48))
	{
		SetEvent(*(HANDLE *)(v1 + 56));
		++*(_DWORD *)(v1 + 48);
		++*(_DWORD *)(v1 + 52);
	}
	LeaveCriticalSection((LPCRITICAL_SECTION)(v1 + 8));
}

void __fastcall sub_7FFB87C07E80(void *Memory, int a2, char a3)
{

}

void __fastcall sub_7FFB87C04B50(LPVOID lpParameter)
{

}

char sub_7FFB87C01750(LPVOID lpParameter, void *Src, unsigned __int64 a3, unsigned int a4)
{
	bool v4; // zf
	unsigned int v5; // esi
	unsigned __int64 v6; // r14
	const char *v7; // r12
	char *v8; // rbp
	char result; // al
	int v10; // edx
	__int64 v11; // r15
	int v12; // ecx
	signed __int64 v13; // rax
	char *v14; // rbx
	unsigned int v15; // ecx
	int v16; // eax
	unsigned __int64 v17; // rdi
	__int64 v18; // rdx
	void *v19; // rcx
	DWORD v20; // ebx
	struct _RTL_CRITICAL_SECTION *v21; // r13
	unsigned __int64 v22; // rdx
	unsigned __int64 v23; // rax
	unsigned __int64 v24; // rax
	unsigned int v25; // er8
	unsigned int v26; // ecx
	struct _RTL_CRITICAL_SECTION *v27; // rsi
	void *v28; // rcx
	__int64 v29; // rax
	unsigned __int64 v30; // r13
	char *v31; // rax
	__int64 v32; // rdx
	size_t v33; // rcx
	size_t v34; // rdi
	char *v35; // rsi
	void *v36; // rax
	__int64 v37; // rdx
	char v38; // si
	size_t v39; // rdi
	__int64 v40; // rcx
	size_t v41; // r14
	__int64 v42; // rax
	__int64 tolen; // [rsp+28h] [rbp-40h]
	__int64 v44; // [rsp+38h] [rbp-30h]
	char *v45; // [rsp+70h] [rbp+8h]

	v4 = *((_BYTE *)lpParameter + 2305) == 0;
	v5 = a4;
	v6 = a3;
	v7 = (const char *)Src;
	v8 = (char *)lpParameter;
	*((_BYTE *)lpParameter + 2307) = 1;
	if (v4)
	{
		*((_BYTE *)lpParameter + 2307) = 0;
		return 0;
	}
	v10 = *((_DWORD *)lpParameter + 579);
	v11 = 0i64;
	v12 = 0;
	if (v10 < 0)
		goto LABEL_18;
	v13 = (signed __int64)(v8 + 2712);
	while (!*(_BYTE *)(v13 + 109) || *(_DWORD *)v13 != a4)
	{
		++v12;
		v13 += 664i64;
		if (v12 > v10)
		{
			v8[2307] = 0;
			return 0;
		}
	}
	v14 = &v8[664 * v12 + 2704];
	if (!v14 || !v14[117] || !v14[116])
		goto LABEL_18;
	v15 = *((_DWORD *)v14 + 15);
	if (v15 == 5)
	{
		//sub_7FFB87BA25E0(
		//	(char *)0x1F4,
		//	(signed __int64)"Invalid operation attempting to send data on a kSocketTypeMulticastReceiver receiver, ignoring",
		//	(__int64)"SOCKETSET",
		//	"Invalid operation attempting to send data on a kSocketTypeMulticastReceiver receiver, ignoring",
		//	0i64);
		return 0;
	}
	if ((v15 - 3) & 0xFFFFFFFC || v15 == 5)
	{
		if (v14[132])
		{
			v17 = 0i64;
			if (a3)
			{
				while (1)
				{
					if (v14[324])
					{
						if (!v6)
							goto LABEL_28;
						v18 = *((_QWORD *)v14 + 74);
						v11 = sub_7FFB87C085C0((LPCRITICAL_SECTION)(v14 + 600), *(_QWORD *)(v14 + 592), (__int64)&v7[v17], v6 - v17);
					}
					else
					{
						v11 = send(*(_QWORD *)v14, &v7[v17], v6 - v17, 0);
					}
					if (v11 < 0)
						break;
					v17 += v11;
					if (v17 >= v6)
						goto LABEL_28;
				}
				LODWORD(tolen) = v11;
				//sub_7FFB87BA25E0((char *)0x1F4, (signed __int64)"Send Error: %i", (__int64)"SOCKETSET", "Send Error: %i", tolen);
				v19 = (void *)*((_QWORD *)v8 + 107);
				v8[2307] = 0;
				v20 = GetThreadId(v19);
				if (GetCurrentThreadId() != v20)
					sub_7FFB87C07E80(v8, 12294, 1);
				result = 0;
			}
			else
			{
			LABEL_28:
				v8[2307] = 0;
				sub_7FFB87C06BC0((__int64)v8, *(_DWORD *)v14, v5, v11);
				if (v8[56])
				{
					//sub_7FFB87BA25E0(
					//	(char *)0x64,
					//	(signed __int64)"Socket set close internal from send data",
					//	(__int64)"SOCKETSET",
					//	"Socket set close internal from send data");
					sub_7FFB87C07E80(v8, 0, 0);
					result = 1;
				}
				else
				{
					if (v14[160])
					{
						LODWORD(tolen) = *((_DWORD *)v14 + 2);
						//sub_7FFB87BA25E0(
						//	(char *)0xC8,
						//	(signed __int64)"Disconnecting %i after direct send",
						//	(__int64)"SOCKETSET",
						//	"Disconnecting %i after direct send",
						//	tolen);
						shutdown(*(_QWORD *)v14, 0);
					}
					result = 1;
				}
			}
			return result;
		}
		v8[2307] = 0;
		sub_7FFB87C04B50(v8);
		v21 = (struct _RTL_CRITICAL_SECTION *)(v8 + 1536);
		EnterCriticalSection((LPCRITICAL_SECTION)(v8 + 1536));
		v22 = *(signed int *)sub_7FFB87C04C80((__int64)v8, v5);
		v23 = *((_QWORD *)v8 + 282);
		if (v22 > v23 || !v23)
			*((_QWORD *)v8 + 282) = v22;
		v24 = *((_QWORD *)v8 + 281);
		if (v22 < v24 || !v24)
			*((_QWORD *)v8 + 281) = v22;
		v25 = *((_DWORD *)v8 + 432);
		v26 = 0;
		if (v25)
		{
			do
			{
				if (*(_QWORD *)&v8[8 * v26 + 1736] == v22)
					break;
				++v26;
			} while (v26 < v25);
		}
		if (v26 == v25 && v25 < 0x40)
		{
			*(_QWORD *)&v8[8 * v26 + 1736] = v22;
			++*((_DWORD *)v8 + 432);
		}
		if (*((_QWORD *)v14 + 19) - *((_QWORD *)v14 + 17) >= v6)
		{
		LABEL_65:
			v37 = *((_QWORD *)v14 + 15);
			v38 = 1;
			if (v37 && *((_QWORD *)v14 + 19) - *((_QWORD *)v14 + 17) >= v6)
			{
				v39 = v6;
				if (*((_QWORD *)v14 + 19) - *((_QWORD *)v14 + 18) < v6)
					v39 = *((_QWORD *)v14 + 19) - *((_QWORD *)v14 + 18);
				memcpy((void *)(v37 + *((_QWORD *)v14 + 18)), v7, v39);
				*((_QWORD *)v14 + 18) += v39;
				v40 = *((_QWORD *)v14 + 18);
				if (*((_QWORD *)v14 + 18) == *((_QWORD *)v14 + 19))
					v40 = 0i64;
				*((_QWORD *)v14 + 17) += v39;
				*((_QWORD *)v14 + 18) = v40;
				v41 = v6 - v39;
				if (v41)
				{
					memcpy((void *)(*((_QWORD *)v14 + 18) + *((_QWORD *)v14 + 15)), &v7[v39], v41);
					*((_QWORD *)v14 + 18) += v41;
					v42 = *((_QWORD *)v14 + 18);
					*((_QWORD *)v14 + 17) += v41;
					if (v42 == *((_QWORD *)v14 + 19))
						*((_QWORD *)v14 + 18) = 0i64;
				}
			}
			else
			{
				v38 = 0;
			}
			send(*((_QWORD *)v8 + 190), "A", 1, 0);
			sub_7FFB879343F0((__int64)(v8 + 1616));
			LeaveCriticalSection(v21);
			return v38;
		}
		v27 = (struct _RTL_CRITICAL_SECTION *)(v8 + 1576);
		EnterCriticalSection((LPCRITICAL_SECTION)(v8 + 1576));
		v28 = (void *)*((_QWORD *)v14 + 15);
		if (v28)
		{
			v29 = *((_QWORD *)v14 + 17);
			v30 = v29 + v6 + 0x8000;
			if (v30 >= 0x2800000)
			{
				LODWORD(v44) = 41943040;
				LODWORD(tolen) = *((_DWORD *)v14 + 38);
				//sub_7FFB87BA25E0(
				//	(char *)0x1F4,
				//	(signed __int64)"Failed to expand existing buffer of size %lu to at least size %lu (Would exceed hardmax of %i)",
				//	(__int64)"SOCKETSET",
				//	"Failed to expand existing buffer of size %lu to at least size %lu (Would exceed hardmax of %i)",
				//	tolen,
				//	v6 + (unsigned int)v29,
				//	v44);
				v21 = (struct _RTL_CRITICAL_SECTION *)(v8 + 1536);
			}
			else
			{
				v31 = (char *)realloc(v28, v29 + v6 + 0x8000);
				v32 = *((_QWORD *)v14 + 18);
				v33 = *((_QWORD *)v14 + 17);
				v45 = v31;
				*((_QWORD *)v14 + 15) = *(_QWORD*)v31;
				if ((signed __int64)(v32 - v33) < 0)
				{
					v34 = v33 - v32;
					v35 = (char *)malloc(v33);
					memcpy(v35, &v45[*((_QWORD *)v14 + 19) - v34], v34);
					memcpy(&v35[v34], *((const void **)v14 + 15), *((_QWORD *)v14 + 17) - v34);
					memcpy(*((void **)v14 + 15), v35, *((_QWORD *)v14 + 17));
					*((_QWORD *)v14 + 18) = *((_QWORD *)v14 + 17);
					free(v35);
					v27 = (struct _RTL_CRITICAL_SECTION *)(v8 + 1576);
				}
				*((_QWORD *)v14 + 19) = v30;
				v21 = (struct _RTL_CRITICAL_SECTION *)(v8 + 1536);
			}
			goto LABEL_64;
		}
		if (v14[133] && v6 <= 0x100000)
		{
			v36 = malloc(0x100000ui64);
			*((_QWORD *)v14 + 19) = 0x100000i64;
		}
		else if (v6 > 0x8000)
		{
			if (v6 > 0x2800000)
			{
				LODWORD(v44) = 41943040;
				//sub_7FFB87BA25E0(
				//	(char *)0x1F4,
				//	(signed __int64)"Failed to expand existing buffer of size %lu to at least size %lu (Would exceed hardmax of %i)",
				//	(__int64)"SOCKETSET",
				//	"Failed to expand existing buffer of size %lu to at least size %lu (Would exceed hardmax of %i)",
				//	0i64,
				//	v6,
				//	v44);
			LABEL_64:
				LeaveCriticalSection(v27);
				goto LABEL_65;
			}
			v36 = malloc(v6 + 0x8000);
			*((_QWORD *)v14 + 19) = v6 + 0x8000;
		}
		else
		{
			v36 = malloc(0x8000ui64);
			*((_QWORD *)v14 + 19) = 0x8000i64;
		}
		*((_QWORD *)v14 + 15) = *(_QWORD*)v36;
		goto LABEL_64;
	}
	v16 = sendto(*(_QWORD *)v14, v7, a3, 0, (const struct sockaddr *)v14 + 5, 16);
	if (v16 < 0)
	{
		//sub_7FFB87BA25E0((char *)0x1F4, (signed __int64)"Sendto fail: %ld", (__int64)"SOCKETSET", "Sendto fail: %ld", v16);
	LABEL_18:
		v8[2307] = 0;
		return 0;
	}
	v8[2307] = 0;
	sub_7FFB87C06BC0((__int64)v8, *(unsigned int *)v14, v5, v6);
	return 1;
}

char sub_7FFB87BA6380(__int64 a1, __int64 a2, void *a3, __int64 a4)
{
	__int64 v4; // rdi
	__int64 v5; // rbx
	void *v6; // rsi
	__int16 v7; // ax
	char *v8; // rax
	int v9; // ecx
	int v10; // ecx
	unsigned int v11; // er9
	unsigned int v13 = 0; // [rsp+48h] [rbp+10h]

	v4 = a4;
	v5 = a2;
	v6 = a3;
	//LOBYTE(v7) = sub_7FFB87BA25E0(
	//	(char *)0x64,
	//	(signed __int64)"Sending %i RTCP feedback bytes",
	//	(__int64)"MIRASINK",
	//	"Sending %i RTCP feedback bytes",
	//	a4);
	if (!*(_QWORD *)(v5 + 32))
	{
		v8 = "Attempted to send feedback without a socket set";
	LABEL_12:
		LOBYTE(v7) = 0;// sub_7FFB87BA25E0((char *)0x1F4, (signed __int64)v8, (__int64)"MIRASINK", v8);
		return v7;
	}
	v9 = *(_DWORD *)(v5 + 60);
	if (!v9)
	{
		v8 = "Attempted to send feedback with an unknown RTP type";
		goto LABEL_12;
	}
	v10 = v9 - 1;
	if (v10)
	{
		if (v10 == 1)
		{
			v7 = *(_WORD *)(v5 + 78);
			v11 = (*(_WORD *)(v5 + 76) != v7) + 54;
		}
		else
		{
			v11 = v13;
		}
	}
	else
	{
		v7 = *(_WORD *)(v5 + 74);
		v11 = (*(_WORD *)(v5 + 72) != v7) + 50;
	}
	if (v11 != -1)
		LOBYTE(v7) = sub_7FFB87C01750(*(LPVOID *)(v5 + 32), v6, v4, v11);
	return v7;
}

__int64 sub_7FFB87942BF0(__int64 a1)
{
	return *(unsigned int *)(a1 + 12);
}

__int64 sub_7FFB87942BE0(__int64 a1)
{
	return *(_QWORD *)(a1 + 24);
}

__int64 sub_7FFB87BA4FA0(__int64 a1, __int64 a2)
{
	__int64 v2; // rsi
	__int64 v3; // rdi
	void(__fastcall *v4)(__int64, _QWORD, _QWORD, _QWORD, _QWORD); // r10
	__int64 result; // rax
	__int64 v6; // rbp
	unsigned int v7; // ebx
	__int64 v8; // rax
	__int64 v9; // [rsp+28h] [rbp-20h]
	__int64 v10; // [rsp+30h] [rbp-18h]
	__int64 v11; // [rsp+38h] [rbp-10h]

	v2 = a2;
	v3 = a1;
	if (!*(_BYTE *)(a1 + 3796))
	{
		LODWORD(v11) = 1024;
		*(_BYTE *)(a1 + 3796) = 1;
		*(_DWORD *)(a1 + 3804) = 48000;
		*(_DWORD *)(a1 + 3808) = 2;
		*(_DWORD *)(a1 + 3800) = 1024;
		LODWORD(v10) = 2;
		LODWORD(v9) = 48000;
		//sub_7FFB87BA25E0(
		//	(char *)0x12C,
		//	(signed __int64)"Miracast Audio Started AAC With Sample Rate: %i, Channels: %i, FramesPerPacket: %i",
		//	(__int64)"MIRASINK",
		//	"Miracast Audio Started AAC With Sample Rate: %i, Channels: %i, FramesPerPacket: %i",
		//	v9,
		//	v10,
		//	v11);
		v4 = *(void(__fastcall **)(__int64, _QWORD, _QWORD, _QWORD, _QWORD))(v3 + 3960);
		if (v4)
			v4(
				v3,
				*(unsigned int *)(v3 + 3804),
				*(unsigned int *)(v3 + 3808),
				*(unsigned int *)(v3 + 3800),
				*(_QWORD *)(v3 + 3936));
	}
	result = *(signed int *)(v2 + 48);
	v6 = result + 1000000i64 * *(signed int *)(v2 + 44);
	if (*(_QWORD *)(v3 + 3968))
	{
		v7 = (unsigned __int64)sub_7FFB87942BF0(*(_QWORD *)(v2 + 32)) - 4;
		v8 = sub_7FFB87942BE0(*(_QWORD *)(v2 + 32));
		result = (*(__int64(__fastcall **)(__int64, __int64, _QWORD, __int64, _QWORD))(v3 + 3968))(
			v3,
			v8 + 4,
			v7,
			v6,
			*(_QWORD *)(v3 + 3936));
	}
	return result;
}

void sub_7FFB87B7DD40(__int64 a1, unsigned __int64 a2)
{
	unsigned __int64 v2; // rbx
	__int64 v3; // r8
	unsigned __int64 v4; // r10
	unsigned int v5; // er11
	__int64 v6; // rax
	unsigned int v7; // edx
	unsigned int v8; // er9
	unsigned __int64 v9; // rcx
	__int64 v10; // rax
	int v11; // eax
	_BYTE *v12; // rcx

	v2 = a2;
	v3 = a1;
	if (a1 && a2 >= 4)
	{
		v4 = a2 - 4;
		v5 = 0;
		if (a2 == 4)
		{
		LABEL_10:
			v5 = v2;
		}
		else
		{
			v6 = 0i64;
			v7 = 2;
			while (*(_BYTE *)(v6 + a1) || *(_BYTE *)(v7 - 1 + a1) || *(_BYTE *)(v7 + a1) || *(_BYTE *)(v7 + 1 + a1) != 1)
			{
				++v5;
				++v7;
				v6 = v5;
				if (v5 >= v4)
					goto LABEL_10;
			}
		}
		if (v5 != v2)
		{
			v8 = v5 + 1;
			v9 = v5 + 1;
			if (v9 < v2)
			{
				do
				{
					if (v9 >= v4)
					{
					LABEL_20:
						v8 = v2;
					}
					else
					{
						v10 = v8 + 2;
						while (*(_BYTE *)(v9 + v3)
							|| *(_BYTE *)((unsigned int)(v10 - 1) + v3)
							|| *(_BYTE *)(v10 + v3)
							|| *(_BYTE *)((unsigned int)(v10 + 1) + v3) != 1)
						{
							++v8;
							v10 = (unsigned int)(v10 + 1);
							v9 = v8;
							if (v8 >= v4)
								goto LABEL_20;
						}
					}
					v11 = v8 - v5;
					v12 = (_BYTE *)(v3 + v5);
					v5 = v8;
					v11 -= 4;
					++v8;
					*v12 = HIBYTE(v11);
					v12[3] = v11;
					v12[1] = BYTE2(v11);
					v12[2] = BYTE1(v11);
					v9 = v8;
				} while (v8 < v2);
			}
		}
	}
}

__int64 sub_7FFB87B7E0F0(__int64 a1, __int64 a2, unsigned int a3)
{
	unsigned __int64 v3; // r9
	__int64 v4; // r8
	__int64 v5; // rcx
	__int64 v6; // rax

	v3 = a3;
	v4 = a1;
	v5 = (unsigned int)v3;
	if (v3 >= a2 - 4)
		return (unsigned int)a2;
	v6 = (unsigned int)(v3 + 2);
	while (*(_BYTE *)(v5 + v4)
		|| *(_BYTE *)((unsigned int)(v6 - 1) + v4)
		|| *(_BYTE *)(v6 + v4)
		|| *(_BYTE *)((unsigned int)(v6 + 1) + v4) != 1)
	{
		LODWORD(v3) = v3 + 1;
		v6 = (unsigned int)(v6 + 1);
		v5 = (unsigned int)v3;
		if ((unsigned int)v3 >= (unsigned __int64)(a2 - 4))
			return (unsigned int)a2;
	}
	return (unsigned int)v3;
}

void sub_7FFB87B7D990(void *Src, size_t Size, _QWORD *a3, size_t *a4)
{
	char *v4; // r15
	size_t v5; // rbp
	signed __int64 v6; // r12
	size_t v7; // r14
	size_t *v8; // r13
	unsigned __int64 v9; // rdi
	char *v10; // rsi
	int v11; // ebx
	unsigned int v12; // edx
	int v13; // eax
	int v14; // er8
	int v15; // edx
	int v16; // eax
	size_t v17; // r8
	char *v18; // rax
	char *v19; // rbx
	char v20; // cl
	char *v21; // rcx
	size_t v22; // r8
	const void *v23; // rdx
	char *v24; // rax
	_QWORD *v25; // [rsp+18h] [rbp+18h]

	if (Src)
	{
		v25 = a3;
		v4 = 0i64;
		v5 = 0i64;
		v6 = 0i64;
		v7 = 0i64;
		v8 = a4;
		v9 = Size;
		v10 = (char *)Src;
		if (Size >= 4)
		{
			if (*(_WORD *)Src || *((_BYTE *)Src + 2) || *((_BYTE *)Src + 3) != 1)
			{
				v24 = (char *)malloc(Size);
				v22 = v9;
				v23 = v10;
				v19 = v24;
				v21 = v24;
			}
			else
			{
				v11 = sub_7FFB87B7E0F0((__int64)Src, Size, 0);
				if (v11 == v9)
					return;
				v12 = v11 + 1;
				if ((unsigned int)(v11 + 1) >= v9)
					goto LABEL_21;
				do
				{
					v13 = sub_7FFB87B7E0F0((__int64)v10, v9, v12);
					v14 = v13;
					v15 = v13;
					v16 = v10[v11 + 4] & 0x1F;
					v17 = (unsigned int)(v14 - v11 - 4);
					if (v16 == 7)
					{
						v5 = (unsigned int)v17;
						v4 = &v10[v11 + 4];
					}
					else if (v16 == 8)
					{
						v7 = v17;
						v6 = (signed __int64)&v10[v11 + 4];
					}
					v11 = v15;
					v12 = v15 + 1;
				} while (v12 < v9);
				if (!v5 || !v7)
				{
				LABEL_21:
					*v8 = 0i64;
					*v25 = 0i64;
					return;
				}
				v9 = v7 + v5 + 11;
				v18 = (char *)malloc(v7 + v5 + 11);
				*v18 = 1;
				v19 = v18;
				v18[1] = v4[1];
				v18[2] = v4[2];
				v20 = v4[3];
				*((_WORD *)v18 + 2) = -7681;
				v18[3] = v20;
				v18[7] = v5;
				v18[6] = BYTE1(v5);
				memcpy(v18 + 8, v4, v5);
				v19[v5 + 8] = 1;
				v19[v5 + 10] = v7;
				v21 = &v19[v5 + 11];
				v22 = v7;
				v23 = (const void *)v6;
				v19[v5 + 9] = BYTE1(v7);
			}
			memcpy(v21, v23, v22);
			*v25 = *(_QWORD*)v19;
			*v8 = v9;
		}
	}
}

__int64 sub_7FFB87B7BE80(void *Memory) 
{
	return 0;
}

int sub_7FFB87BA25E0(char *Format, signed __int64 a2, __int64 a3, char *Formata, ...) 
{
	return 0;
}

signed __int64 sub_7FFB87B7E610(_DWORD *a1, __int64 a2, __int64 a3) 
{
	return 0;
}

__int64 __fastcall sub_7FFB87B7FC10(__int64 a1, __int64 a2, unsigned __int64 a3)
{
	_DWORD* v3; // r9
	__int64 result; // rax
	unsigned __int64 v5; // rcx

	v3 = (_DWORD*)a1;
	if (a3 >= 9)
	{
		v5 = *(unsigned __int8 *)(a2 + 7) + (signed __int64)(*(unsigned __int8 *)(a2 + 6) << 8);
		if (v5 <= a3 + 8)
		{
			result = sub_7FFB87B7E610(
				v3,
				a2 + 8,
				*(unsigned __int8 *)(a2 + 7) + (signed __int64)(*(unsigned __int8 *)(a2 + 6) << 8));
		}
		else
		{
			sub_7FFB87BA25E0(
				(char *)0x64,
				(signed __int64)"SQH264SPSParseFromAvcC: SPS Length (%u) > AvcC Length w/ Offset (%u)",
				(__int64)"H264",
				"SQH264SPSParseFromAvcC: SPS Length (%u) > AvcC Length w/ Offset (%u)",
				v5,
				a3);
			result = 0i64;
		}
	}
	else
	{
		//sub_7FFB87BA25E0(
		//	(char *)0x64,
		//	(signed __int64)"SQH264SPSParseFromAvcC: SPS cannot be found in Avcc w/ length %u",
		//	(__int64)"H264",
		//	"SQH264SPSParseFromAvcC: SPS cannot be found in Avcc w/ length %u",
		//	a3);
		result = 0i64;
	}
	return result;
}

__int64 sub_7FFB87B7FB60(_DWORD *a1, _DWORD *a2, _DWORD *a3, _DWORD *a4, _DWORD *a5, _DWORD *a6, _DWORD *a7)
{
	__int64 result; // rax

	if (a4)
	{
		result = (unsigned int)a1[766];
		*a4 = result;
	}
	if (a5)
	{
		result = (unsigned int)a1[767];
		*a5 = result;
	}
	if (a6)
	{
		result = (unsigned int)a1[768];
		*a6 = result;
	}
	if (a7)
	{
		result = (unsigned int)a1[769];
		*a7 = result;
	}
	if (a2)
	{
		result = (unsigned int)(2 * (8 * a1[760] + 8 - a1[767] - a1[766]));
		*a2 = result;
	}
	if (a3)
	{
		result = (unsigned int)(a1[761] + 1);
		*a3 = 2 * (8 * result * (2 - a1[762]) - a1[769] - a1[768]);
	}
	return result;
}

struct _RTL_CRITICAL_SECTION *__fastcall sub_7FFB87B82950(unsigned __int64 a1, unsigned int a2, unsigned int a3, unsigned int a4, _DWORD *a5) 
{
	return NULL;
}

struct _RTL_CRITICAL_SECTION * sub_7FFB87B827E0(__int64 a1, int a2, int a3, unsigned int a4)
{
	int v4; // eax
	__int64 v6; // [rsp+30h] [rbp-28h]
	__int64 v7; // [rsp+38h] [rbp-20h]

	v4 = a3 + 64;
	v6 = 0i64;
	v7 = 0i64;
	switch (a2)
	{
	case 32:
	case 1380401729:
	case 1111970369:
	case 1380401752:
		goto LABEL_11;
	case 2033463856:
	case 2037788978:
		LODWORD(v6) = a3 + 64;
		HIDWORD(v6) = v4 >> 1;
		LODWORD(v7) = v4 >> 1;
		return sub_7FFB87B82950(a1, a2, a3, a4, (int*)&v6);
	case 1853239602:
	case 1853239857:
		HIDWORD(v6) = a3 + 64;
		goto LABEL_11;
	case 1498765618:
		v4 = 2 * a3;
	LABEL_11:
		LODWORD(v6) = v4;
		break;
	}
	return sub_7FFB87B82950(a1, a2, a3, a4, (int*)&v6);
}

__int64 sub_7FFB87B7BF80(int a1, unsigned int a2, unsigned int a3, _BYTE *a4, size_t Size, unsigned int a6, __int64 a7, __int64 a8, _QWORD *a9)
{
	int v9; // er15
	_BYTE *v10; // rbx
	unsigned int v11; // er12
	unsigned int v12; // er13
	_DWORD *v13; // rdi
	void *v14; // rax
	unsigned int v15; // er14
	_DWORD *v16; // rbp
	char *v17; // rax
	_QWORD *v18 = NULL; // rbx
	_DWORD *v19; // rsi
	__int64(__fastcall *v20)(_DWORD *); // rax

	v9 = a1;
	v10 = a4;
	v11 = a3;
	v12 = a2;
	v13 = (_DWORD*)malloc(0x90ui64);
	memset(v13, 0, 0x90ui64);
	v13[4] = v12;
	v13[5] = v11;
	v13[6] = v12;
	v13[7] = v11;
	v13[8] = a6;
	v14 = malloc(Size);
	*((_QWORD *)v13 + 7) = *(_QWORD *)v14;
	memcpy(v14, v10, Size);
	*((_QWORD *)v13 + 8) = Size;
	*((_QWORD *)v13 + 10) = a8;
	v15 = 1;
	*((_QWORD *)v13 + 11) = a7;
	if (Size < 3 || *v10 || v10[1] || v10[2] != 1)
	{
		if (Size < 4 || *v10 || v10[1] || v10[2] || v10[3] != 1)
		{
			v16 = 0i64;
			v17 = "H.264 format is NALU";
			v13[12] = 0;
		}
		else
		{
			v13[12] = 1;
			v16 = 0i64;
			v17 = "H.264 format is AnnexB of length 4";
		}
	}
	else
	{
		v13[12] = 1;
		v16 = 0i64;
		v17 = "H.264 format is AnnexB of length 3";
	}
	sub_7FFB87BA25E0((char *)0xC8, (signed __int64)v17, (__int64)"H264", v17, 0i64);
	//v18 = qword_7FFB88EBB380;
	//if (qword_7FFB88EBB380)
	if (1)
	{
		while (1)
		{
			v19 = (_DWORD *)*v18;
			if ((v9 & *(_DWORD *)*v18) == *(_DWORD *)*v18)
			{
				sub_7FFB87BA25E0(
					(char *)0xC8,
					*((_QWORD *)v19 + 1),
					(__int64)"H264",
					"Attempting the %s Decoder",
					*((_QWORD *)v19 + 1));
				*v13 = *v19;
				*((_QWORD *)v13 + 1) = *((_QWORD *)v19 + 1);
				*((_QWORD *)v13 + 14) = *((_QWORD *)v19 + 3);
				*((_QWORD *)v13 + 15) = *((_QWORD *)v19 + 4);
				*((_QWORD *)v13 + 16) = *((_QWORD *)v19 + 5);
				v20 = (__int64(__fastcall *)(_DWORD *))*((_QWORD *)v19 + 6);
				*((_QWORD *)v13 + 17) = *(_QWORD *)v20;
				v15 = v20(v13);
				if (!v15)
					break;
			}
			v18 = (_QWORD *)v18[2];
			if (!v18)
				goto LABEL_20;
		}
		sub_7FFB87BA25E0(
			(char *)0xC8,
			(signed __int64)"Using the %s Decoder",
			(__int64)"H264",
			"Using the %s Decoder",
			*((_QWORD *)v13 + 1));
		*((_BYTE *)v13 + 104) = 1;
		if (!*((_BYTE *)v19 + 16))
			*((_QWORD *)v13 + 5) = *(_QWORD *)sub_7FFB87B827E0(10i64, a6, v12, v11);
	}
LABEL_20:
	if (*((_BYTE *)v13 + 104))
		v16 = v13;
	*a9 = (_QWORD)v16;
	return v15;
}

__int64(__fastcall *__fastcall sub_7FFB87BA47B0(__int64 a1, __int64 a2))(__int64, __int64, _QWORD)
{
	return 0;
}

int sub_7FFB87BA4720(__int64 a1, int a2, __int64 a3, __int64 a4, __int64 a5, __int64 a6, __int64 a7, __int64 a8, __int64 a9)
{
	int result; // eax

	if (a2 || !a3)
		result = sub_7FFB87BA25E0(
		(char *)0x1F4,
			(signed __int64)"Failed decoding a miracast h264 frame",
			(__int64)"MIRASINK",
			"Failed decoding a miracast h264 frame",
			a6,
			a7,
			a8,
			a9);
	else
		result = (unsigned __int64)sub_7FFB87BA47B0(a5, a3);
	return result;
}

__int64 sub_7FFB87B7BF70(__int64 a1)
{
	return *(unsigned __int8 *)(a1 + 105);
}

__int64 sub_7FFB87B7BEE0(__int64 a1, __int64 a2, __int64 a3)
{
	__int64(__cdecl *v3)(__int64, __int64, __int64); // r10
	__int64 v4; // rbx
	__int64 result; // rax

	v3 = *(__int64(__cdecl **)(__int64, __int64, __int64))(a1 + 120);
	v4 = a1;
	if (!v3)
		return 3i64;
	result = v3(a1, a2, a3);
	++*(_QWORD *)(v4 + 96);
	return result;
}

void __fastcall sub_7FFB87BA5320(__int64 a1, __int64 a2)
{
	__int64 v2; // rbx
	__int64 v3; // r14
	signed int v4; // esi
	__int64 v5; // rcx
	size_t v6; // rdi
	__int64 v7; // rax
	__int64 v8; // rax
	__int64 v9; // r12
	void *v10; // rdi
	char v11; // r15
	void *v12; // rcx
	void *v13; // rcx
	unsigned __int64 v14; // r8
	__int64 v15; // rdx
	int v16; // edx
	int v17; // er8
	int v18; // eax
	__int64 v19; // ST30_8
	__int64 v20; // ST28_8
	const void *v21; // rcx
	int v22; // eax
	__int64 v23; // rcx
	void(__fastcall *v24)(__int64, _QWORD, _QWORD, _QWORD, __int64, __int64, _QWORD, __int64, _QWORD); // r10
	unsigned int v25; // edi
	__int64 v26; // rax
	unsigned int v27; // edi
	__int64 v28; // rax
	__int64 v29; // [rsp+20h] [rbp-60h]
	__int64 v30; // [rsp+28h] [rbp-58h]
	__int64 v31; // [rsp+28h] [rbp-58h]
	__int64 v32; // [rsp+28h] [rbp-58h]
	__int64 v33; // [rsp+30h] [rbp-50h]
	__int64 v34; // [rsp+38h] [rbp-48h]
	__int64 v35; // [rsp+40h] [rbp-40h]
	__int64 v36; // [rsp+48h] [rbp-38h]
	__int64 v37; // [rsp+50h] [rbp-30h]
	int v38; // [rsp+60h] [rbp-20h]
	int v39; // [rsp+64h] [rbp-1Ch]
	int v40; // [rsp+68h] [rbp-18h]
	__int64 Buf2; // [rsp+70h] [rbp-10h]
	int v42; // [rsp+C0h] [rbp+40h]
	int v43; // [rsp+C8h] [rbp+48h]
	int v44; // [rsp+D8h] [rbp+58h]

	v2 = a1;
	v3 = a2;
	*(_DWORD *)(a1 + 3772) = 0;
	v4 = 0;
	if ((unsigned int)sub_7FFB87942BF0(*(_QWORD *)(a2 + 32)) != 4)
	{
		do
		{
			if (*(_DWORD *)(sub_7FFB87942BE0(*(_QWORD *)(v3 + 32)) + v4) == 1000000)
				break;
			++v4;
		} while (v4 < (unsigned int)sub_7FFB87942BF0(*(_QWORD *)(v3 + 32)) - 4);
		if (v4)
		{
			LODWORD(v33) = (unsigned __int64)sub_7FFB87942BF0(*(_QWORD *)(v3 + 32)) - v4;
			LODWORD(v30) = v4;
			//sub_7FFB87BA25E0(
			//	(char *)0x64,
			//	(signed __int64)"Miracast has an unusual offset of %i, leaving %i",
			//	(__int64)"MIRASINK",
			//	"Miracast has an unusual offset of %i, leaving %i",
			//	v30,
			//	v33);
		}
	}
	v5 = *(_QWORD *)(v3 + 32);
	Buf2 = 0ui64;
	v6 = (unsigned int)sub_7FFB87942BF0(v5) - (signed __int64)v4;
	v7 = sub_7FFB87942BE0(*(_QWORD *)(v3 + 32));
	sub_7FFB87B7D990((void *)(v7 + v4), v6, (unsigned long long*)&Buf2, (size_t *)&Buf2 + 1);
	LODWORD(v6) = (unsigned __int64)sub_7FFB87942BF0(*(_QWORD *)(v3 + 32)) - v4;
	v8 = sub_7FFB87942BE0(*(_QWORD *)(v3 + 32));
	sub_7FFB87B7DD40(v8 + v4, (unsigned int)v6);
	v9 = *((_QWORD *)&Buf2 + 1);
	v10 = (void *)Buf2;
	v11 = 1;
	if (*((_QWORD *)&Buf2 + 1) == *(_DWORD *)(v2 + 192))
	{
		v21 = *(const void **)(v2 + 184);
		if (v21)
		{
			if ((_QWORD)Buf2)
			{
				v11 = 1;
				if (memcmp(v21, (const void *)Buf2, *((size_t *)&Buf2 + 1)))
					v11 = 0;
			}
		}
	}
	else
	{
		v11 = 0;
	}
	if (v9 && !v11)
	{
		v12 = *(void **)(v2 + 184);
		if (v12)
		{
			free(v12);
			LODWORD(v9) = DWORD(Buf2);
			v10 = (void *)Buf2;
			*(_QWORD *)(v2 + 184) = 0i64;
		}
		*(_QWORD *)&Buf2 = 0i64;
		*(_DWORD *)(v2 + 192) = v9;
		*(_QWORD *)(v2 + 184) = *(_QWORD*)v10;
		//sub_7FFB87BA25E0(
		//	(char *)0xC8,
		//	(signed __int64)"Miracast RTP Found AnnexB SPS/PPS, setting up H264 decoder..",
		//	(__int64)"MIRASINK",
		//	"Miracast RTP Found AnnexB SPS/PPS, setting up H264 decoder..",
		//	0i64);
		v13 = *(void **)(v2 + 200);
		if (v13)
			sub_7FFB87B7BE80(v13);
		v14 = *(unsigned int *)(v2 + 192);
		v15 = *(_QWORD *)(v2 + 184);
		*(_DWORD *)(v2 + 216) = 1920;
		*(_DWORD *)(v2 + 220) = 1080;
		if ((unsigned int)sub_7FFB87B7FC10(v2 + 224, v15, v14))
		{
			v42 = *(_DWORD *)(v2 + 216);
			v43 = *(_DWORD *)(v2 + 220);
			sub_7FFB87B7FB60((_DWORD *)(v2 + 224), &v42, &v43, &v40, &v39, &v38, &v44);
			v16 = v43;
			v17 = v42;
			LODWORD(v37) = v44;
			v18 = v38;
			*(_DWORD *)(v2 + 220) = v43;
			*(_DWORD *)(v2 + 216) = v17;
			LODWORD(v36) = v18;
			LODWORD(v35) = v39;
			LODWORD(v34) = v40;
			LODWORD(v19) = v16;
			LODWORD(v20) = v17;
			//sub_7FFB87BA25E0(
			//	(char *)0x64,
			//	(signed __int64)"Miracast H264 SPS: w: %d, h: %d, left: %d, right: %d, top: %d, bottom: %d",
			//	(__int64)"MIRASINK",
			//	"Miracast H264 SPS: w: %d, h: %d, left: %d, right: %d, top: %d, bottom: %d",
			//	v20,
			//	v19,
			//	v34,
			//	v35,
			//	v36,
			//	v37);
		}
		else
		{
			LODWORD(v31) = *(_DWORD *)(v2 + 192);
			//sub_7FFB87BA25E0(
			//	(char *)0x64,
			//	(signed __int64)"Miracast H264 SPS: Could not decode SPS (AvcC %u bytes)",
			//	(__int64)"MIRASINK",
			//	"Miracast H264 SPS: Could not decode SPS (AvcC %u bytes)",
			//	v31);
		}
		LODWORD(v32) = 2033463856;
		if ((unsigned int)sub_7FFB87B7BF80(
			0xFFFFFFFFi64,
			*(unsigned int *)(v2 + 216),
			*(unsigned int *)(v2 + 220),
			*(_BYTE **)(v2 + 184),
			*(unsigned int *)(v2 + 192),
			v32,
			(__int64)sub_7FFB87BA4720,
			v2,
			(_QWORD *)(v2 + 200)))
		{
			//sub_7FFB87BA25E0(
			//	(char *)0x1F4,
			//	(signed __int64)"H264Decoder failed to init.",
			//	(__int64)"MIRASINK",
			//	"H264Decoder failed to init.",
			//	0i64);
		}
		else
		{
			//sub_7FFB87BA25E0(
			//	(char *)0xC8,
			//	(signed __int64)"Decoder opened successfully",
			//	(__int64)"MIRASINK",
			//	"Decoder opened successfully",
			//	0i64);
			*(_DWORD *)(v2 + 3768) = 0;
		}
		v22 = *(_DWORD *)(v2 + 216);
		v23 = *(_QWORD *)(v2 + 200);
		*(_QWORD *)(v2 + 3784) = 0i64;
		*(_DWORD *)(v2 + 3776) = v22;
		*(_DWORD *)(v2 + 3780) = *(_DWORD *)(v2 + 220);
		if ((unsigned __int8)sub_7FFB87B7BF70(v23))
		{
			v24 = *(void(__fastcall **)(__int64, _QWORD, _QWORD, _QWORD, __int64, __int64, _QWORD, __int64, _QWORD))(v2 + 3992);
			if (v24)
			{
				LODWORD(v34) = *(_DWORD *)(v2 + 192);
				LODWORD(v30) = *(_DWORD *)(v2 + 3788);
				LODWORD(v29) = *(_DWORD *)(v2 + 3784);
				v24(
					v2,
					0i64,
					*(unsigned int *)(v2 + 3776),
					*(unsigned int *)(v2 + 3780),
					v29,
					v30,
					*(_QWORD *)(v2 + 184),
					v34,
					*(_QWORD *)(v2 + 3936));
			}
			v10 = (void *)Buf2;
			*(_DWORD *)(v2 + 3768) = 1;
		}
		else
		{
			v10 = (void *)Buf2;
			*(_DWORD *)(v2 + 3768) = 0;
		}
	}
	if (v10)
	{
		free(v10);
		*(_QWORD *)&Buf2 = 0i64;
	}
	if (*(_QWORD *)(v2 + 200))
	{
		v25 = (unsigned __int64)sub_7FFB87942BF0(*(_QWORD *)(v3 + 32)) - v4;
		v26 = sub_7FFB87942BE0(*(_QWORD *)(v3 + 32));
		HIDWORD(v33) = HIDWORD(v2);
		HIDWORD(v30) = 0;
		if ((unsigned int)sub_7FFB87B7BEE0(*(_QWORD *)(v2 + 200), v26 + v4, v25))
		{
			//sub_7FFB87BA25E0(
			//	(char *)0x1F4,
			//	(signed __int64)"Failed decoding Miracast H.264 frame, total %i!",
			//	(__int64)"MIRASINK",
			//	"Failed decoding Miracast H.264 frame, total %i!",
			//	(unsigned int)++*(_DWORD *)(v2 + 3792),
			//	v2);
			if (*(_DWORD *)(v2 + 3792) >= 5)
				/*sub_7FFB87BA25E0(
				(char *)0x1F4,
					(signed __int64)"Hit the failure limit for H.264 frames",
					(__int64)"MIRASINK",
					"Hit the failure limit for H.264 frames",
					0i64)*/;
		}
		else
		{
			*(_DWORD *)(v2 + 3792) = 0;
		}
	}
	if (*(_QWORD *)(v2 + 3976))
	{
		if (*(_DWORD *)(v2 + 3768))
		{
			v27 = (unsigned __int64)sub_7FFB87942BF0(*(_QWORD *)(v3 + 32)) - v4;
			v28 = sub_7FFB87942BE0(*(_QWORD *)(v3 + 32));
			LODWORD(v34) = *(_DWORD *)(v2 + 220);
			LODWORD(v33) = *(_DWORD *)(v2 + 216);
			LODWORD(v30) = *(_DWORD *)(v2 + 192);
			(*(void(__fastcall **)(__int64, _QWORD, __int64, _QWORD, _QWORD, __int64, __int64, __int64, _QWORD, _QWORD))(v2 + 3976))(
				v2,
				0i64,
				v28 + v4,
				v27,
				*(_QWORD *)(v2 + 184),
				v30,
				v33,
				v34,
				*(_QWORD *)(v3 + 44),
				*(_QWORD *)(v2 + 3936));
		}
	}
}

int sub_7FFB87BA58D0(__int64 a1, __int64 a2)
{
	return 0;
}

__int16 sub_7FFB87BA60B0(__int64 a1, __int64 a2) 
{
	return 0;
}

LARGE_INTEGER sub_7FFB87BA50D0(__int64 a1, __int64 a2, __int64 a3, unsigned int a4)
{
	__int64 v4; // r14
	__int64 v5; // rbp
	char v6; // r12
	char v7; // r13
	__int64 v8; // rdi
	void(__fastcall *v9)(__int64, signed __int64, _QWORD); // rax
	LARGE_INTEGER result; // rax
	__int64 v11; // rbx
	signed __int64 v12; // rcx
	signed __int64 v13; // rcx
	__int64 v14; // [rsp+28h] [rbp-40h]
	__int64 v15; // [rsp+28h] [rbp-40h]
	__int64 v16; // [rsp+30h] [rbp-38h]
	__int64 v17; // [rsp+38h] [rbp-30h]

	v4 = a3;
	v5 = a2;
	v6 = 0;
	v7 = 0;
	v8 = sub_7FFB8794FAC0().QuadPart;
	if (!*(_BYTE *)(v5 + 92))
	{
		v9 = *(void(__fastcall **)(__int64, signed __int64, _QWORD))(v5 + 4008);
		if (v9)
			v9(v5, 3i64, *(_QWORD *)(v5 + 3936));
		*(_BYTE *)(v5 + 92) = 1;
	}
	LODWORD(v17) = *(unsigned __int8 *)(v4 + 40);
	LODWORD(v16) = *(unsigned __int16 *)(v4 + 42);
	//LODWORD(v14) = sub_7FFB87942BF0(*(_QWORD *)(v4 + 32));
	//sub_7FFB87BA25E0(
	//	(char *)0x32,
	//	(signed __int64)"Received Miracast PES packet of %i bytes on PID %i type %i",
	//	(__int64)"MIRASINK",
	//	"Received Miracast PES packet of %i bytes on PID %i type %i",
	//	v14,
	//	v16,
	//	v17);
	switch (*(_BYTE *)(v4 + 40))
	{
	case 0xF:
		sub_7FFB87BA4FA0(v5, v4);
		break;
	case 0x1B:
		sub_7FFB87BA5320(v5, v4);
		v6 = 1;
		goto LABEL_15;
	case 0x24:
		sub_7FFB87BA58D0(v5, v4);
		v6 = 1;
		goto LABEL_15;
	default:
		if (*(unsigned __int8 *)(v4 + 40) != 131)
		{
			//LODWORD(v15) = *(unsigned __int8 *)(v4 + 40);
			//sub_7FFB87BA25E0(
			//	(char *)0x1F4,
			//	(signed __int64)"Unhandled stream type: %i",
			//	(__int64)"MIRASINK",
			//	"Unhandled stream type: %i",
			//	v15);
			goto LABEL_15;
		}
		sub_7FFB87BA60B0(v5, v4);
		break;
	}
	v7 = 1;
LABEL_15:
	result = sub_7FFB8794FAC0();
	v11 = result.QuadPart;
	//if (v7)
	//{
	//	*(_QWORD *)(v5 + 4080) = sub_7FFB8794FA80(*(_QWORD *)(v4 + 8), *(_QWORD *)v4);
	//	*(_QWORD *)(v5 + 4096) = sub_7FFB8794FA80(*(_QWORD *)(v4 + 16), *(_QWORD *)(v4 + 8));
	//	*(_QWORD *)(v5 + 4112) = sub_7FFB8794FA80(*(_QWORD *)(v4 + 24), *(_QWORD *)(v4 + 16));
	//	*(_QWORD *)(v5 + 4128) = sub_7FFB8794FA80(v11, v8);
	//	v12 = 1000000i64 * *(signed int *)(v4 + 44);
	//	result.QuadPart = *(signed int *)(v4 + 48);
	//	++*(_QWORD *)(v5 + 4160);
	//	*(_QWORD *)(v5 + 4144) = result.QuadPart + v12;
	//}
	//else if (v6)
	//{
	//	*(_QWORD *)(v5 + 4072) = sub_7FFB8794FA80(*(_QWORD *)(v4 + 8), *(_QWORD *)v4);
	//	*(_QWORD *)(v5 + 4088) = sub_7FFB8794FA80(*(_QWORD *)(v4 + 24), *(_QWORD *)(v4 + 8));
	//	*(_QWORD *)(v5 + 4104) = sub_7FFB8794FA80(*(_QWORD *)(v4 + 24), *(_QWORD *)(v4 + 16));
	//	*(_QWORD *)(v5 + 4120) = sub_7FFB8794FA80(v11, v8);
	//	v13 = 1000000i64 * *(signed int *)(v4 + 44);
	//	result.QuadPart = *(signed int *)(v4 + 48);
	//	++*(_QWORD *)(v5 + 4152);
	//	*(_QWORD *)(v5 + 4136) = result.QuadPart + v13;
	//}
	return result;
}

void sub_7FFB87BAA560(LARGE_INTEGER *a1)
{
	LARGE_INTEGER *v1; // rdi
	__int64 v2; // rcx
	char *v3; // rbp
	char *v4; // rax
	__int64 v5; // rsi
	__int64 v6; // rbx
	__int64 v7; // rax
	__int64 v8; // rcx
	const char *v9; // rax
	__int64 v10; // [rsp+28h] [rbp-90h]
	__int64 v11; // [rsp+30h] [rbp-88h]
	const char **v12; // [rsp+40h] [rbp-78h]
	const char **v13; // [rsp+48h] [rbp-70h]
	int v14; // [rsp+50h] [rbp-68h]
	const char *v15; // [rsp+58h] [rbp-60h]
	const char *v16; // [rsp+60h] [rbp-58h]
	const char *v17; // [rsp+80h] [rbp-38h]
	__int64 v18; // [rsp+88h] [rbp-30h]

	v1 = a1;
	v2 = a1[13].QuadPart;
	v12 = &v15;
	v13 = &v17;
	v15 = "Content-Type";
	v14 = 1;
	v17 = "text/parameters";
	if (v2)
	{
		v16 = "Session";
		v14 = 2;
		v18 = sub_7FFB8794E300(v2);
	}
	if (v1[10].HighPart != 3)
		v1[10].HighPart = 3;
	if (v1[11].LowPart != 38)
		v1[11].LowPart = 38;
	v1[491] = sub_7FFB8794FAC0();
	//v3 = sub_7FFB8794F8E0((__int64)"MiracastStateConnected");
	//v4 = sub_7FFB8794F8E0((__int64)"MiracastSinkStateM13Sent");
	//v5 = (__int64)v4;
	//if (v3 && v4)
	//{
	//	v6 = sub_7FFB8794E300((__int64)v4);
	//	v7 = sub_7FFB8794E300((__int64)v3);
	//	sub_7FFB87BA25E0(
	//		(char *)0x12C,
	//		(signed __int64)"Miracast state now %s, %s",
	//		(__int64)"MIRASINK",
	//		"Miracast state now %s, %s",
	//		v7,
	//		v6);
	//}
	//else
	//{
	//	LODWORD(v11) = 38;
	//	LODWORD(v10) = 3;
	//	sub_7FFB87BA25E0(
	//		(char *)0x12C,
	//		(signed __int64)"Miracast state now %i, %i",
	//		(__int64)"MIRASINK",
	//		"Miracast state now %i, %i",
	//		v10,
	//		v11);
	//}
	//if (v5)
	//	sub_7FFB87948710(v5);
	//if (v3)
	//	sub_7FFB87948710((__int64)v3);
	if (v1[4].QuadPart)
	{
		v8 = v1[12].QuadPart;
		if (v8)
			v9 = (const char *)sub_7FFB8794E300(v8);
		else
			v9 = "/wfd1.0";
		//((void(__fastcall *)(_QWORD, _QWORD, _QWORD, _QWORD, _QWORD, _QWORD, _QWORD, _DWORD, _QWORD, _QWORD, _QWORD, _QWORD))sub_7FFB87BFFA30)(
		//	(LARGE_INTEGER)v1[4].QuadPart,
		//	"SET_PARAMETER",
		//	v9,
		//	"RTSP/1.0",
		//	&v12,
		//	"wfd_idr_request\r\n",
		//	17i64,
		//	32,
		//	v12,
		//	v13,
		//	*(_QWORD *)&v14,
		//	v15);
	}
}

void sub_7FFB87BA4F40(__int64 a1, __int64 a2, int a3)
{
	__int64 v3; // rbx

	if (a3 == 1)
	{
		v3 = a2;
		sub_7FFB87BA25E0(
			(char *)0x12C,
			(signed __int64)"RTP Stream Handler requested IDR",
			(__int64)"MIRASINK",
			"RTP Stream Handler requested IDR",
			0i64);
		*(_BYTE *)(v3 + 3897) = 1;
		sub_7FFB87BAA560((LARGE_INTEGER *)v3);
	}
}

size_t sub_7FFB87BF8B00(__int64 a1, void *a2, size_t a3)
{
	void *v3; // rdi
	size_t v4; // rsi
	__int64 v5; // rbx
	signed int v6; // ebp
	LARGE_INTEGER *v7; // rdi
	LPDWORD v8; // ST28_8
	__int64 v9; // ST38_8
	__int64 v10; // ST30_8
	LPDWORD v11; // ST28_8
	DWORD v12; // eax
	__int64 v13; // rdx
	unsigned __int16 v14; // ax
	__int64 v15; // rcx
	unsigned __int64 v16; // rsi
	unsigned __int64 v17; // rdi
	unsigned __int64 v18; // rdi
	__int64 v19; // r8
	unsigned __int64 v20; // rdi
	unsigned __int64 v21; // r9
	int v22; // ecx
	__int64 v23; // rdx
	LARGE_INTEGER *v24; // rsi
	int v26; // ecx
	__int64 v27; // rdx
	__int64 v28; // rdi
	__int64 v29; // rcx
	__int64 v30; // rdx
	int v31; // ecx
	__int64 v32; // rcx
	__int64 v33; // rax
	__int16 v34; // ax
	__int64 ST20_8_58; // ST20_8
	unsigned __int64 v36; // rdi
	__int16 v37; // ax
	LONG v38; // eax
	__int64 v39; // rbx
	signed __int64 v40; // rax
	LPDWORD lpThreadId; // [rsp+28h] [rbp-D0h]
	LPDWORD lpThreadIda; // [rsp+28h] [rbp-D0h]
	LPDWORD lpThreadIdb; // [rsp+28h] [rbp-D0h]
	__int64 v44; // [rsp+30h] [rbp-C8h]
	__int64 v45; // [rsp+30h] [rbp-C8h]
	__int64 v46; // [rsp+38h] [rbp-C0h]
	__int64 v47; // [rsp+40h] [rbp-B8h]
	char v48; // [rsp+60h] [rbp-98h]
	unsigned __int8 v49; // [rsp+61h] [rbp-97h]
	LARGE_INTEGER *v50; // [rsp+68h] [rbp-90h]
	size_t Size; // [rsp+70h] [rbp-88h]
	unsigned __int64 v52; // [rsp+78h] [rbp-80h]
	char v53; // [rsp+80h] [rbp-78h]
	unsigned __int64 v54; // [rsp+88h] [rbp-70h]
	unsigned __int64 v55; // [rsp+90h] [rbp-68h]
	void *Src; // [rsp+98h] [rbp-60h]
	char v57[48]; // [rsp+A0h] [rbp-58h]

	*(_QWORD *)(a1 + 112) += a3;
	++*(_QWORD *)(a1 + 96);
	v3 = a2;
	Src = a2;
	v4 = a3;
	Size = a3;
	v5 = a1;
	sub_7FFB87BA25E0(
		(char *)0x32,
		(signed __int64)"Processing %i RTP Bytes",
		(__int64)"CASTRTP",
		"Processing %i RTP Bytes");
	v6 = 0;
	/*v7 = (LARGE_INTEGER *)sub_7FFB87BFC4C0(v3, v4);
	v50 = v7;
	if (!v7)
	{
	LABEL_31:
		sub_7FFB87BA25E0(
			(char *)0x1F4,
			(signed __int64)"Failed to decode RTP Packet",
			(__int64)"CASTRTP",
			"Failed to decode RTP Packet",
			0i64);
		return Size;
	}
	v48 = 0;
	while (1)
	{
		v7[200] = sub_7FFB8794FAC0();
		sub_7FFB87BA25E0(
			(char *)0x32,
			(signed __int64)"Received RTP Packet: ",
			(__int64)"CASTRTP",
			"Received RTP Packet: ",
			0i64);
		LODWORD(v47) = v7[1].HighPart;
		LODWORD(v46) = LOWORD(v7[1].LowPart);
		LODWORD(v44) = HIDWORD(v7->QuadPart);
		LODWORD(v8) = LOBYTE(v7->QuadPart);
		sub_7FFB87BA25E0(
			(char *)0x32,
			(signed __int64)"Payload Type: %u, Marked: %u, SequenceNumber: %u, Timestamp: %u",
			(__int64)"CASTRTP",
			"Payload Type: %u, Marked: %u, SequenceNumber: %u, Timestamp: %u",
			v8,
			v44,
			v46,
			v47);
		LODWORD(v9) = LOWORD(v7[5].LowPart);
		LODWORD(v10) = v7[2].HighPart;
		LODWORD(v11) = v7[2].LowPart;
		sub_7FFB87BA25E0(
			(char *)0x32,
			(signed __int64)"SSRC: %lx, CSCRCount: %u, extensionHeaderID: %i, extensionHeaderLength: %i, PayloadLength: %i, paddingLength: %i",
			(__int64)"CASTRTP",
			"SSRC: %lx, CSCRCount: %u, extensionHeaderID: %i, extensionHeaderLength: %i, PayloadLength: %i, paddingLength: %i",
			v11,
			v10,
			v9,
			v7[6].QuadPart,
			v7[8].QuadPart,
			v7[9].QuadPart);
		if (!*(_BYTE *)(v5 + 33))
		{
			if (LOBYTE(v7->QuadPart) != 33)
				goto LABEL_7;
			v12 = v7[2].LowPart;
			*(_BYTE *)(v5 + 33) = 1;
			*(_DWORD *)(v5 + 36) = v12;
		}
		if (v7[2].LowPart != *(_DWORD *)(v5 + 36))
		{
			sub_7FFB87BA25E0(
				(char *)0x190,
				(signed __int64)"Received packet on different SSRC %i",
				(__int64)"CASTRTP",
				"Received packet on different SSRC %i",
				0i64);
			v39 = 0i64;
			do
			{
				if ((unsigned __int64)v6 >= v7[8].QuadPart)
					break;
				v40 = v7[7].QuadPart;
				LODWORD(v45) = *(unsigned __int8 *)(v40 + v39);
				LODWORD(lpThreadIdb) = v6;
				sub_7FFB87BA25E0((char *)0x12C, v40, (__int64)"CASTRTP", "Payload byte %i: 0x%02x", lpThreadIdb, v45);
				++v6;
				++v39;
			} while (v6 < 16);
			sub_7FFB87BFC460(v7);
			return Size;
		}
	LABEL_7:
		if (!*(_BYTE *)(v5 + 32))
		{
			sub_7FFB87BA25E0(
				(char *)0x12C,
				(signed __int64)"Received first RTP packet",
				(__int64)"CASTRTP",
				"Received first RTP packet",
				0i64);
			*(_BYTE *)(v5 + 32) = 1;
			*(_WORD *)(v5 + 48) = v7[1].LowPart;
			*(_QWORD *)(v5 + 56) = LOWORD(v7[1].LowPart);
			*(_QWORD *)(v5 + 64) = LOWORD(v7[1].LowPart);
			*(_DWORD *)(v5 + 72) = LOWORD(v7[1].LowPart);
			v38 = v7[1].HighPart;
			*(_QWORD *)(v5 + 80) = 1i64;
			*(_DWORD *)(v5 + 52) = v38;
			goto LABEL_67;
		}
		++*(_QWORD *)(v5 + 80);
		v13 = *(_QWORD *)(v5 + 56);
		v14 = *(_WORD *)(v5 + 48);
		LOWORD(v15) = v7[1].LowPart;
		v16 = v13 + 1;
		if ((unsigned __int16)v15 <= v14)
		{
			if (v14 < 0xF447u || (unsigned __int16)v15 >= 0xBB8u)
			{
				sub_7FFB87BA25E0(
					(char *)0x64,
					(signed __int64)"Packet seq number is behind head",
					(__int64)"CASTRTP",
					"Packet seq number is behind head",
					0i64);
				v15 = LOWORD(v7[1].LowPart);
				v13 = *(_QWORD *)(v5 + 56);
				v17 = v15 | *(_QWORD *)(v5 + 56) & 0xFFFFFFFFFFFF0000ui64;
			}
			else
			{
				sub_7FFB87BA25E0(
					(char *)0xC8,
					(signed __int64)"Packet rolled over RTP Sequence counter...",
					(__int64)"CASTRTP",
					"Packet rolled over RTP Sequence counter...",
					0i64);
				v15 = LOWORD(v7[1].LowPart);
				v13 = *(_QWORD *)(v5 + 56);
				v17 = (v15 | *(_QWORD *)(v5 + 56) & 0xFFFFFFFFFFFF0000ui64) + 0x10000;
			}
		}
		else
		{
			v17 = (unsigned __int16)v15 | v13 & 0xFFFFFFFFFFFF0000ui64;
		}
		LODWORD(v45) = *(unsigned __int16 *)(v5 + 48);
		LODWORD(lpThreadId) = (unsigned __int16)v15;
		v52 = v17;
		sub_7FFB87BA25E0(
			(char *)0x32,
			(signed __int64)"Computed packet seq %i last was %i - extended (%llu was %llu)",
			(__int64)"CASTRTP",
			"Computed packet seq %i last was %i - extended (%llu was %llu)",
			lpThreadId,
			v45,
			v17,
			v13);
		if (v17 > *(unsigned int *)(v5 + 72))
			*(_DWORD *)(v5 + 72) = v17;
		if (v17 <= v16)
			break;
		v18 = v17 - v16;
		sub_7FFB87BA25E0(
			(char *)0x64,
			(signed __int64)"Gap of ~%i packets since last in-order packet!",
			(__int64)"CASTRTP",
			"Gap of ~%i packets since last in-order packet!",
			v18 - 1);
		if (v18 > 0x3E8)
		{
			v36 = v52;
			sub_7FFB87BA25E0(
				(char *)0x190,
				(signed __int64)"Processing an extremely large packet gap, resetting sequence from %llu to %llu",
				(__int64)"CASTRTP",
				"Processing an extremely large packet gap, resetting sequence from %llu to %llu",
				*(_QWORD *)(v5 + 56),
				v52);
			goto LABEL_62;
		}
		if (*(_BYTE *)(v5 + 128) && v18 > 0x14)
		{
			v36 = v52;
		LABEL_62:
			v48 = 1;
			v37 = v50[1].QuadPart;
			*(_QWORD *)(v5 + 56) = v36;
			*(_WORD *)(v5 + 48) = v37;
			sub_7FFB87BFA090(v5);
			v7 = v50;
			v4 = Size;
			goto LABEL_67;
		}
		v20 = v52;
		LOBYTE(v19) = 0;
		v21 = v16;
		v54 = v16;
		v55 = v16;
		v53 = 0;
		v49 = 0;
		if (v16 >= v52)
			goto LABEL_28;
		while (v16 - v21 < 0x180)
		{
			v22 = 0;
			while (1)
			{
				v23 = *(_QWORD *)(v5 + 8i64 * v22 + 136);
				if (v23)
				{
					if (*(_WORD *)(v23 + 8) == (_WORD)v16)
						break;
				}
				if (++v22 >= 384)
					goto LABEL_26;
			}
			if (*(_QWORD *)(v5 + 8i64 * v22 + 136))
			{
				if ((_BYTE)v19)
					goto LABEL_45;
				v26 = 0;
				while (1)
				{
					v27 = *(_QWORD *)(v5 + 8i64 * v26 + 136);
					if (v27)
					{
						if (*(_WORD *)(v27 + 8) == (_WORD)v16)
							break;
					}
					if (++v26 >= 384)
					{
						v28 = 0i64;
						goto LABEL_41;
					}
				}
				v28 = *(_QWORD *)(v5 + 8i64 * v26 + 136);
				*(_QWORD *)(v5 + 8i64 * v26 + 136) = 0i64;
				*(_WORD *)(v5 + 130) = v26;
			LABEL_41:
				--*(_QWORD *)(v5 + 80);
				sub_7FFB87BF8B00(v5, v28 + 80, *(_QWORD *)(v28 + 1584));
				sub_7FFB87BFC460(v28);
				v20 = v52;
				v21 = v54;
				goto LABEL_44;
			}
		LABEL_26:
			if ((_BYTE)v19)
			{
				v55 = v16;
				v53 = v16 - v21;
				*(_QWORD *)&v57[8 * (((signed int)v16 - (signed int)v21 - 1) / 64)] |= 1i64 << (char)(v16 - v21 - 1) % -64;
			LABEL_44:
				v19 = v49;
				goto LABEL_45;
			}
			LOBYTE(v19) = 1;
			v21 = v16;
			v54 = v16;
			v49 = 1;
			if (*(_BYTE *)(v5 + 128))
				goto LABEL_28;
		LABEL_45:
			if (++v16 >= v20)
				break;
		}
		if (!(_BYTE)v19)
			goto LABEL_28;
		if (*(_BYTE *)(v5 + 128))
			goto LABEL_28;
		v29 = *(_QWORD *)(v5 + 6328);
		if (v29 == v21 && *(_DWORD *)(v5 + 6336) <= 0x14u)
		{
			LODWORD(v44) = *(_DWORD *)(v5 + 6336);
			sub_7FFB87BA25E0(
				(char *)0x64,
				(signed __int64)"Waiting for re-request for %llu - waited %i packets so far",
				(__int64)"CASTRTP",
				"Waiting for re-request for %llu - waited %i packets so far",
				v29,
				v44);
			++*(_DWORD *)(v5 + 6336);
		LABEL_28:
			v24 = v50;
			goto LABEL_29;
		}
		v30 = *(_QWORD *)(v5 + 6344);
		*(_QWORD *)(v5 + 6328) = v21;
		*(_DWORD *)(v5 + 6336) = 0;
		if (v30 == v21)
		{
			v31 = *(_DWORD *)(v5 + 6352);
			*(_DWORD *)(v5 + 6352) = v31 - 1;
			if (!v31)
			{
				sub_7FFB87BA25E0(
					(char *)0x64,
					(signed __int64)"Completely abandoning hope for %llu",
					(__int64)"CASTRTP",
					"Completely abandoning hope for %llu",
					v30);
				v32 = *(_QWORD *)(v5 + 6344);
				if ((unsigned __int64)*(unsigned int *)(v5 + 72) - v32 <= 0x180)
				{
					*(_QWORD *)(v5 + 56) = v55;
					sub_7FFB87BA25E0(
						(char *)0x64,
						(signed __int64)"Abandoning %llu-%llu, now looking for %llu",
						(__int64)"CASTRTP",
						"Abandoning %llu-%llu, now looking for %llu",
						v32,
						v55,
						v55 + 1);
				}
				else
				{
					v33 = (unsigned int)(*(_DWORD *)(v5 + 72) - 384);
					*(_QWORD *)(v5 + 56) = v33;
					sub_7FFB87BA25E0(
						(char *)0x64,
						(signed __int64)"Abandoning %llu, now looking for %llu",
						(__int64)"CASTRTP",
						"Abandoning %llu, now looking for %llu",
						v32,
						v33 + 1);
				}
				v34 = *(_WORD *)(v5 + 56);
				v21 = v54;
				*(_QWORD *)(v5 + 6344) = 0i64;
				*(_WORD *)(v5 + 48) = v34;
				*(_DWORD *)(v5 + 6352) = 0;
				*(_QWORD *)(v5 + 6328) = 0i64;
			}
		}
		else
		{
			*(_QWORD *)(v5 + 6344) = v21;
			*(_DWORD *)(v5 + 6352) = 1;
		}
		LODWORD(v44) = 2;
		LODWORD(lpThreadIda) = 2 - *(_DWORD *)(v5 + 6352);
		sub_7FFB87BA25E0(
			(char *)0x64,
			(signed __int64)"Processing re-request (%i of %i) for starting seq %llu",
			(__int64)"CASTRTP",
			"Processing re-request (%i of %i) for starting seq %llu",
			lpThreadIda,
			v44,
			v21);
		v24 = v50;
		LOBYTE(ST20_8_58) = v53;
		sub_7FFB87BFB2B0(v5, v50[2].LowPart, (unsigned __int16)v54, v57, ST20_8_58);
	LABEL_29:
		if (*(_QWORD *)(v5 + 56) != v20 - 1)
		{
			sub_7FFB87BFB590(v5, v24, v19, v21);
			return Size;
		}
		sub_7FFB87BFC460(v24);
		--*(_QWORD *)(v5 + 80);
		v4 = Size;
		*(_QWORD *)(v5 + 112) += Size;
		++*(_QWORD *)(v5 + 96);
		sub_7FFB87BA25E0(
			(char *)0x32,
			(signed __int64)"Processing %i RTP Bytes",
			(__int64)"CASTRTP",
			"Processing %i RTP Bytes",
			Size);
		v7 = (LARGE_INTEGER *)sub_7FFB87BFC4C0(Src, Size);
		v50 = v7;
		if (!v7)
			goto LABEL_31;
	}
	if (v17 < v16)
	{
		sub_7FFB87BA25E0(
			(char *)0x64,
			(signed __int64)"Ignoring duplicate packet!",
			(__int64)"CASTRTP",
			"Ignoring duplicate packet!",
			0i64);
		sub_7FFB87BFC460(v50);
		return Size;
	}
	*(_QWORD *)(v5 + 56) = v17;
	*(_WORD *)(v5 + 48) = v50[1].LowPart;
	sub_7FFB87BA25E0(
		(char *)0x32,
		(signed __int64)"Processing in-order packet ext# %llu",
		(__int64)"CASTRTP",
		"Processing in-order packet ext# %llu",
		v17);
	v7 = v50;
	v4 = Size;
LABEL_67:
	LOBYTE(v7[199].LowPart) = v48;
	if (!*(_BYTE *)(v5 + 41608) && !*(_BYTE *)(v5 + 41609))
	{
		sub_7FFB87BA25E0(
			(char *)0x12C,
			(signed __int64)"Starting Miracast RTP Scheduler Thread",
			(__int64)"CASTRTP",
			"Starting Miracast RTP Scheduler Thread",
			0i64);
		*(_BYTE *)(v5 + 41608) = 1;
		*(_QWORD *)(v5 + 41600) = CreateThread(0i64, 0i64, sub_7FFB87BF9C50, (LPVOID)v5, 0, 0i64);
	}
	sub_7FFB87BFB4B0(v5, v7);*/
	return v4;
}

__int64 sub_7FFB87BABFA0(__int64 a1, __int64 a2, __int64 a3)
{
	__int64 v3; // rdi
	void* v4; // rsi
	__int64 v5; // rbx
	char* v6; // rax
	__int64 v7; // r8
	__int64 v8; // rdx
	__int64 v9; // rdx

	v3 = a3;
	v4 = (void *)a2;
	v5 = a1;
	if (!*(_QWORD *)(a1 + 112))
	{
		v6 = sub_7FFB87BF89D0();
		v7 = *(_QWORD *)(v5 + 3880);
		v8 = *(_QWORD *)(v5 + 3888);
		*(_QWORD *)(v5 + 112) = *(_QWORD*)v6;
		sub_7FFB87BF9640((__int64)v6, v8, v7);
		if (*(_BYTE *)(v5 + 80))
		{
			//sub_7FFB87BA25E0(
			//	(char *)0x190,
			//	(signed __int64)"SQMiracastSink: Disabling NACKs on Microsoft connection, using IDRs instead",
			//	(__int64)"MIRASINK",
			//	"SQMiracastSink: Disabling NACKs on Microsoft connection, using IDRs instead",
			//	0i64);
			LOBYTE(v9) = 1;
			sub_7FFB87BF9670(*(_QWORD *)(v5 + 112), v9);
		}
		sub_7FFB87BF9650(
			*(_QWORD **)(v5 + 112),
			(__int64)sub_7FFB87BA6380,
			(__int64)sub_7FFB87BA50D0,
			(__int64)sub_7FFB87BA4F40,
			v5);
	}
	return sub_7FFB87BF8B00(*(_QWORD *)(v5 + 112), v4, v3);
}

void __fastcall sub_7FFB87BAB990(__int64 a1, unsigned __int8 *a2)
{

}

int sub_7FFB87BABE30(__int64 a1, unsigned __int8 *a2, unsigned __int64 a3) 
{
	return 0;
}

int sub_7FFB87BAB7D0(__int64 a1, int a2, __int64 a3, __int64 a4, __int64 a5, __int64 a6, __int64 a7, __int64 a8, __int64 a9)
{
	int result; // eax
	char *v10; // rax
	__int64 v11; // [rsp+28h] [rbp-10h]

	switch (a2)
	{
	case 32:
		return 0/*sub_7FFB87BA25E0(
			(char *)0x1F4,
			(signed __int64)"Received %i raw bytes from the RTSP stream",
			(__int64)"MIRASINK",
			"Received %i raw bytes from the RTSP stream",
			a4)*/;
	case 48:
		return sub_7FFB87BABFA0(a5, a3, a4);
	case 49:
		v10 = "Unhandled data on the RTPC UDP client stream";
		goto LABEL_5;
	case 50:
		v10 = "Unhandled data on the RTPC UDP server stream";
		goto LABEL_5;
	case 51:
		v10 = "Unhandled data on the RTPC UDP server stream";
		goto LABEL_5;
	case 52:
		v10 = "Unhandled data on the RTP TCP client stream";
		goto LABEL_5;
	case 53:
		v10 = "Unhandled data on the RTPC TCP client stream";
		goto LABEL_5;
	case 54:
		v10 = "Unhandled data on the RTP TCP server stream";
		goto LABEL_5;
	case 55:
		v10 = "Unhandled data on the RTPC TCP server stream";
		goto LABEL_5;
	case 64:
		sub_7FFB87BAB990(a5, (unsigned __int8 *)a3);
		break;
	case 80:
		sub_7FFB87BABE30(a5, (unsigned __int8 *)a3, a4);
		break;
	case 81:
		v10 = "Unhandled data on the RTPC TCP connection stream";
	LABEL_5:
		result = 0;// sub_7FFB87BA25E0((char *)0x190, (signed __int64)v10, (__int64)"MIRASINK", v10, a6, a7, a8, a9);
		break;
	default:
		LODWORD(v11) = a2;
		result = 0;/*sub_7FFB87BA25E0(
			(char *)0x1F4,
			(signed __int64)"Received Mirascast Sink data on an unknown tag: 0x%02x",
			(__int64)"MIRASINK",
			"Received Mirascast Sink data on an unknown tag: 0x%02x",
			v11);*/
		break;
	}
	return result;
}

__int64 sub_7FFB87BAC730(__int64 a1, __int64 a2, __int64 a3, __int64 a4, __int64 a5)
{
	__int64 result; // rax

	result = a5;
	++*(_QWORD *)(result + 4040);
	*(_QWORD *)(result + 4056) += a4;
	return result;
}

__int64 sub_7FFB87C01E40(_QWORD *a1, __int64 a2, __int64 a3, __int64 a4, __int64 a5, __int64 a6, __int64 a7)
{
	__int64 result; // rax

	a1[8314] = a2;
	a1[8313] = a3;
	a1[8315] = a5;
	a1[8311] = a4;
	a1[8316] = a6;
	result = a7;
	a1[8310] = a7;
	return result;
}

void sub_7FFB87934190(__int64 a1)
{
	__int64 v1; // rbx

	v1 = a1;
	EnterCriticalSection((LPCRITICAL_SECTION)(a1 + 16));
	sub_7FFB879343F0(v1 + 56);
	LeaveCriticalSection((LPCRITICAL_SECTION)(v1 + 16));
}

void sub_7FFB87BACC60(LARGE_INTEGER *a1)
{
	LARGE_INTEGER *v1; // rdi
	char *v2; // rbp
	char *v3; // rax
	__int64 v4; // rsi
	__int64 v5; // rbx
	__int64 v6; // ST28_8
	struct _RTL_CRITICAL_SECTION *v7; // rbx
	void(__fastcall *v8)(LARGE_INTEGER *, signed __int64, LARGE_INTEGER); // rax
	__int64 v9; // rcx
	void *v10; // rcx
	__int64 v11; // [rsp+28h] [rbp-20h]
	__int64 v12; // [rsp+30h] [rbp-18h]

	v1 = a1;
	if (a1[11].LowPart != 112)
	{
		if (a1[10].HighPart != 7)
			a1[10].HighPart = 7;
		a1[11].LowPart = 112;
		a1[491] = sub_7FFB8794FAC0();
		//v2 = sub_7FFB8794F8E0((__int64)"MiracastStateStopped");
		//v3 = sub_7FFB8794F8E0((__int64)"MiracastSinkStateStopped");
		//v4 = (__int64)v3;
		//if (v2 && v3)
		//{
		//	v5 = sub_7FFB8794E300((__int64)v3);
		//	v6 = sub_7FFB8794E300((__int64)v2);
		//	sub_7FFB87BA25E0(
		//		(char *)0x12C,
		//		(signed __int64)"Miracast state now %s, %s",
		//		(__int64)"MIRASINK",
		//		"Miracast state now %s, %s",
		//		v6,
		//		v5);
		//}
		//else
		//{
		//	LODWORD(v11) = 112;
		//	LODWORD(v10) = 7;
		//	sub_7FFB87BA25E0(
		//		(char *)0x12C,
		//		(signed __int64)"Miracast state now %i, %i",
		//		(__int64)"MIRASINK",
		//		"Miracast state now %i, %i",
		//		v10,
		//		v11);
		//}
		//if (v4)
		//	sub_7FFB87948710(v4);
		//if (v2)
		//	sub_7FFB87948710((__int64)v2);
		if (*(_DWORD *)(v1 + 84) > 0)
		{
			v7 = *(struct _RTL_CRITICAL_SECTION **)(v1 + 32);
			*(_QWORD *)(v1 + 32) = 0i64;
			if (v7)
			{
				sub_7FFB87C01E40((unsigned long long*)v7, 0i64, 0i64, 0i64, 0i64, 0i64, (__int64)v1);
				sub_7FFB87C00A70(v7);
				v8 = (void(__fastcall *)(LARGE_INTEGER *, signed __int64, LARGE_INTEGER))v1[501].QuadPart;
				//if (v8)
				//	((void(__fastcall *)(_QWORD, _QWORD, _QWORD))v8)(v1, 4i64, (LARGE_INTEGER)v1[492].QuadPart);
			}
		}
		v9 = v1->QuadPart;
		if (v1->QuadPart)
		{
			LOBYTE(v1[1].LowPart) = 1;
			sub_7FFB87934190(v9);
			v10 = (void *)v1[2].QuadPart;
			if (v10)
			{
				WaitForSingleObject(v10, 0xFFFFFFFF);
				v1[2].QuadPart = 0i64;
			}
		}
	}
}

_DWORD * sub_7FFB87BFE2A0(__int64 a1)
{
	__int64 v1; // rsi
	_DWORD *v2; // rbx
	const char *v3; // rcx
	const char *v4; // rcx
	const char *v5; // rcx
	size_t v6; // rcx
	void *v7; // rax
	_QWORD *v8; // rax
	bool v9; // zf
	int v10; // ebp
	__int64 v11; // rax
	__int64 v12; // rdi
	char *v13; // rax

	v1 = a1;
	v2 = (_DWORD*)calloc(1ui64, 0x40ui64);
	*(_BYTE *)v2 = 1;
	v2[1] = 1;
	v3 = *(const char **)(v1 + 8);
	if (v3)
		*((_QWORD *)v2 + 1) = *(_QWORD*)_strdup(v3);
	v4 = *(const char **)(v1 + 16);
	if (v4)
		*((_QWORD *)v2 + 2) = *(_QWORD*)_strdup(v4);
	v5 = *(const char **)(v1 + 24);
	if (v5)
		*((_QWORD *)v2 + 3) = *(_QWORD*)_strdup(v5);
	if (*(_QWORD *)(v1 + 40))
	{
		v6 = *(_QWORD *)(v1 + 48);
		if (v6)
		{
			v7 = malloc(v6);
			*((_QWORD *)v2 + 5) = *(_QWORD*)v7;
			memcpy(v7, *(const void **)(v1 + 40), *(_QWORD *)(v1 + 48));
			*((_QWORD *)v2 + 6) = *(_QWORD *)(v1 + 48);
		}
	}
	v8 = (_QWORD *)malloc(0x18ui64);
	v9 = *(_QWORD *)(v1 + 32) == 0i64;
	*((_QWORD *)v2 + 4) = *(_QWORD *)v8;
	if (v9)
	{
		*v8 = *(_QWORD *)malloc(8ui64);
		*(_QWORD *)(*((_QWORD *)v2 + 4) + 8i64) = *(_QWORD *)malloc(8ui64);
		*(_DWORD *)(*((_QWORD *)v2 + 4) + 16i64) = 0;
	}
	else
	{
		*v8 = *(_QWORD*)malloc(8i64 * (*(_DWORD *)(*(_QWORD *)(v1 + 32) + 16i64) + 1));
		v10 = 0;
		*(_QWORD *)(*((_QWORD *)v2 + 4) + 8i64) = *(_QWORD*)malloc(8i64 * (*(_DWORD *)(*(_QWORD *)(v1 + 32) + 16i64) + 1));
		*(_DWORD *)(*((_QWORD *)v2 + 4) + 16i64) = *(_DWORD *)(*(_QWORD *)(v1 + 32) + 16i64);
		v11 = *(_QWORD *)(v1 + 32);
		if (*(_DWORD *)(v11 + 16) > 0)
		{
			v12 = 0i64;
			do
			{
				*(_QWORD *)(v12 + **((_QWORD **)v2 + 4)) = *(_QWORD*)_strdup(*(const char **)(v12 + *(_QWORD *)v11));
				v13 = _strdup(*(const char **)(*(_QWORD *)(*(_QWORD *)(v1 + 32) + 8i64) + v12));
				++v10;
				v12 += 8i64;
				*(_QWORD *)(v12 + *(_QWORD *)(*((_QWORD *)v2 + 4) + 8i64) - 8) = *(_QWORD*)v13;
				v11 = *(_QWORD *)(v1 + 32);
			} while (v10 < *(_DWORD *)(v11 + 16));
		}
	}
	return v2;
}

void sub_7FFB87BAC750(__int64 a1, __int64 a2, __int64 a3, __int64 a4, __int64 a5, __int64 a6)
{
	sub_7FFB87BA25E0(
		(char *)0x1F4,
		(signed __int64)"SQMiracastSink - unexpected password request, aborting",
		(__int64)"MIRASINK",
		"SQMiracastSink - unexpected password request, aborting",
		0i64);
	sub_7FFB87BACC60((LARGE_INTEGER *)a6);
}

_QWORD * sub_7FFB8794FF80(_QWORD *a1, __int64 a2, __int64 a3)
{
	_QWORD *v3; // rsi
	__int64 v4; // rdi
	__int64 v5; // rbx
	_QWORD *result; // rax
	_QWORD *i; // rcx

	v3 = a1;
	v4 = a3;
	v5 = a2;
	result = (_QWORD*)malloc(0x20ui64);
	result[2] = 0i64;
	result[3] = 0i64;
	*result = v5;
	result[1] = v4;
	if (v3)
	{
		for (i = v3; i[2]; i = (_QWORD *)i[2])
			;
		i[2] = (_QWORD)result;
		result[3] = (_QWORD)i;
		result = v3;
	}
	return result;
}

void sub_7FFB87933BD0(__int64 a1, __int64 a2, const void *a3, size_t a4, __int64 a5)
{
	__int64 v5; // rsi
	size_t v6; // r14
	const void *v7; // rbp
	__int64 v8; // rbx
	_QWORD *v9; // rdi
	void *v10; // rbx
	__int64 v11; // rax

	v5 = a1;
	v6 = a4;
	v7 = a3;
	v8 = a2;
	v9 = (_QWORD*)malloc(0x50ui64);
	v9[3] = 0i64;
	*((_BYTE *)v9 + 32) = 0;
	v9[2] = 0i64;
	*v9 = v8;
	if (v6)
	{
		v10 = malloc(v6);
		memcpy(v10, v7, v6);
		v11 = *(_QWORD *)free;
		v9[1] = (_QWORD)v10;
	}
	else
	{
		v11 = a5;
		v9[1] = (_QWORD)v7;
	}
	v9[3] = v11;
	EnterCriticalSection((LPCRITICAL_SECTION)(v5 + 16));
	*(_QWORD *)(v5 + 120) = *(_QWORD *)sub_7FFB8794FF80(*(_QWORD **)(v5 + 120), (__int64)v9, 80i64);
	sub_7FFB879343F0(v5 + 56);
	LeaveCriticalSection((LPCRITICAL_SECTION)(v5 + 16));
}

__int64 sub_7FFB87BFE0A0(__int64 a1)
{
	__int64 v1; // r10
	__int64 v3; // r9
	_QWORD *v4; // r8
	__int64 v5; // r11
	signed __int64 v6; // rcx
	signed __int64 v8; // rax

	v1 = 0i64;
	if (!a1)
		return 0i64;
	v3 = *(signed int *)(a1 + 16);
	if (v3 > 0)
	{
		v4 = *(_QWORD **)(a1 + 8);
		v5 = *(_QWORD *)a1 - (_QWORD)v4;
		do
		{
			v6 = -1i64;
			while (*(_BYTE *)(*(_QWORD *)((char *)v4 + v5) + v6++ + 1) != 0)
				;
			v8 = -1i64;
			do
				++v8;
			while (*(_BYTE *)(*v4 + v8));
			++v4;
			v1 += v8 + v6 + 4;
			--v3;
		} while (v3);
	}
	return v1;
}

void sub_7FFB87BAC260(__int64 *a1)
{
	__int64 v1; // rdi
	__int64 v2; // rbx
	__int64 v3; // rax
	signed __int64 v4; // rcx
	signed __int64 v5; // rdx
	signed __int64 v6; // r9
	signed __int64 v7; // rax
	signed __int64 v8; // r9
	void *v9; // rsi
	const char *v10; // rbp
	__int64 v11; // rdx
	char v12; // al
	size_t v13; // rbp

	v1 = *a1;
	v2 = a1[1];
	++*(_QWORD *)(v1 + 4048);
	v3 = sub_7FFB87BFE0A0(*(_QWORD *)(v2 + 32));
	v4 = -1i64;
	v5 = -1i64;
	do
		++v5;
	while (*(_BYTE *)(*(_QWORD *)(v2 + 24) + v5));
	v6 = v5 + v3;
	v7 = -1i64;
	do
		++v7;
	while (*(_BYTE *)(*(_QWORD *)(v2 + 16) + v7));
	v8 = v7 + v6;
	do
		++v4;
	while (*(_BYTE *)(*(_QWORD *)(v2 + 8) + v4));
	v9 = 0i64;
	*(_QWORD *)(v1 + 4064) += v4 + v8 + *(_QWORD *)(v2 + 48) + 6i64;
	sub_7FFB87BA25E0(
		(char *)0x12C,
		(signed __int64)"SQMiracastSink Request: ",
		(__int64)"MIRASINK",
		"SQMiracastSink Request: ",
		0i64);
	/*sub_7FFB87BFE690(v2);
	v10 = *(const char **)(v2 + 8);
	v11 = 0i64;
	while (1)
	{
		v12 = aOptions[v11++];
		if (v12 != v10[v11 - 1])
			break;
		if (v11 == 8)
		{
			sub_7FFB87BA6480(v1);
			return;
		}
	}
	if (!strcmp("GET_PARAMETER", *(const char **)(v2 + 8)))
	{
		if (*(_QWORD *)(v2 + 40) && *(_QWORD *)(v2 + 48))
			sub_7FFB87BA6820(v1, v2);
		else
			sub_7FFB87BA6700(v1, v2);
		return;
	}
	if (strcmp("SET_PARAMETER", v10) || (unsigned int)(*(_DWORD *)(v1 + 84) - 3) > 2)
		return;
	if (!*(_QWORD *)(v2 + 40))
		goto LABEL_32;
	v13 = *(_QWORD *)(v2 + 48);
	if (!v13)
		goto LABEL_32;
	v9 = calloc(v13 + 1, 1ui64);
	memcpy(v9, *(const void **)(v2 + 40), v13);
	if (!v9)
		goto LABEL_32;
	if (strstr((const char *)v9, "wfd2_transport_switch"))
	{
		sub_7FFB87BA9150(v1, v2);
		goto LABEL_28;
	}
	if (strstr((const char *)v9, "wfd_av_format_change_timing"))
	{
		sub_7FFB87BA9060(v1, v2);
		goto LABEL_28;
	}
	if (strstr((const char *)v9, "wfd_trigger_method"))
		sub_7FFB87BA9670(v1, v2);
	else
		LABEL_32:
	sub_7FFB87BA7BA0(v1, v2);*/
LABEL_28:
	if (v9)
		free(v9);
}

void sub_7FFB87BAC490(_QWORD *a1)
{
	_QWORD *v1; // rbx
	void *v2; // rcx

	v1 = a1;
	v2 = (void *)a1[1];
	if (v2)
	{
		v1[1] = 0i64;
		//sub_7FFB87BFE790(v2);
	}
	free(v1);
}

void sub_7FFB87BAC1F0(__int64 a1, __int64 a2, __int64 a3, _QWORD *a4)
{
	__int64 *v4; // rsi
	__int64 v5; // rbx
	_QWORD *v6; // rax
	const void *v7; // rdi

	v4 = (__int64*)a4;
	v5 = a3;
	v6 = (_QWORD*)malloc(0x10ui64);
	*v6 = (_QWORD)v4;
	v7 = v6;
	v6[1] = (_QWORD)sub_7FFB87BFE2A0(v5);
	sub_7FFB87933BD0(*v4, (__int64)sub_7FFB87BAC260, v7, 0i64, (__int64)sub_7FFB87BAC490);
}

_DWORD * sub_7FFB87BFE960(__int64 a1)
{
	__int64 v1; // rsi
	_DWORD *v2; // rbx
	const char *v3; // rcx
	const char *v4; // rcx
	size_t v5; // rcx
	void *v6; // rax
	_QWORD *v7; // rax
	int v8; // ebp
	__int64 v9; // rax
	__int64 v10; // rdi
	char *v11; // rax

	v1 = a1;
	v2 = (_DWORD *)calloc(1ui64, 0x40ui64);
	*(_BYTE *)v2 = 1;
	v2[1] = 1;
	v3 = *(const char **)(v1 + 8);
	if (v3)
		*((_QWORD *)v2 + 1) = *(_QWORD *)_strdup(v3);
	v2[14] = *(_DWORD *)(v1 + 56);
	v4 = *(const char **)(v1 + 24);
	if (v4)
		*((_QWORD *)v2 + 3) = *(_QWORD *)_strdup(v4);
	if (*(_QWORD *)(v1 + 40))
	{
		v5 = *(_QWORD *)(v1 + 48);
		if (v5)
		{
			v6 = malloc(v5);
			*((_QWORD *)v2 + 5) = *(_QWORD *)v6;
			memcpy(v6, *(const void **)(v1 + 40), *(_QWORD *)(v1 + 48));
			*((_QWORD *)v2 + 6) = *(_QWORD *)(v1 + 48);
		}
	}
	if (!*(_QWORD *)(v1 + 32))
		return v2;
	v7 = (_QWORD *)malloc(0x18ui64);
	*((_QWORD *)v2 + 4) = *(_QWORD *)v7;
	*v7 = *(_QWORD *)malloc(8i64 * *(signed int *)(*(_QWORD *)(v1 + 32) + 16i64));
	v8 = 0;
	*(_QWORD *)(*((_QWORD *)v2 + 4) + 8i64) = *(_QWORD *)malloc(8i64 * *(signed int *)(*(_QWORD *)(v1 + 32) + 16i64));
	*(_DWORD *)(*((_QWORD *)v2 + 4) + 16i64) = *(_DWORD *)(*(_QWORD *)(v1 + 32) + 16i64);
	v9 = *(_QWORD *)(v1 + 32);
	if (*(_DWORD *)(v9 + 16) > 0)
	{
		v10 = 0i64;
		do
		{
			*(_QWORD *)(v10 + **((_QWORD **)v2 + 4)) = *(_QWORD *)_strdup(*(const char **)(*(_QWORD *)v9 + v10));
			v11 = _strdup(*(const char **)(*(_QWORD *)(*(_QWORD *)(v1 + 32) + 8i64) + v10));
			++v8;
			v10 += 8i64;
			*(_QWORD *)(v10 + *(_QWORD *)(*((_QWORD *)v2 + 4) + 8i64) - 8) = *(_QWORD *)v11;
			v9 = *(_QWORD *)(v1 + 32);
		} while (v8 < *(_DWORD *)(v9 + 16));
	}
	return v2;
}

void  sub_7FFB87BAC700(_QWORD *a1)
{
	_QWORD *v1; // rbx
	void *v2; // rcx

	v1 = a1;
	v2 = (void *)a1[1];
	if (v2)
	{
		v1[1] = 0i64;
		//sub_7FFB87BFED70(v2);
	}
	free(v1);
}

__int64 sub_7FFB87BAC530(__int64 *a1)
{
	__int64 v1; // rbx
	_QWORD *v2; // rdi
	__int64 v3; // rax
	signed __int64 v4; // rcx
	signed __int64 v5; // rdx
	__int64 result = 0; // rax
	__int64 v7; // [rsp+28h] [rbp-10h]

	v1 = *a1;
	v2 = (_QWORD *)a1[1];
	++*(_QWORD *)(v1 + 4048);
	v3 = sub_7FFB87BFE0A0(v2[4]);
	v4 = -1i64;
	v5 = -1i64;
	do
		++v5;
	while (*(_BYTE *)(v2[3] + v5));
	do
		++v4;
	while (*(_BYTE *)(v2[1] + v4));
	*(_QWORD *)(v1 + 4064) += v4 + v5 + v3 + v2[6] + 9i64;
	sub_7FFB87BA25E0(
		(char *)0x12C,
		(signed __int64)"SQMiracastSink Response: ",
		(__int64)"MIRASINK",
		"SQMiracastSink Response: ",
		0i64);
	/*sub_7FFB87BFEC80(v2);
	switch (*(_DWORD *)(v1 + 88))
	{
	case 0x12:
		result = sub_7FFB87BA9CF0(v1, v2);
		break;
	case 0x18:
		result = sub_7FFB87BA9E20(v1, v2);
		break;
	case 0x1A:
	case 0x26:
		result = sub_7FFB87BAB130(v1, 4i64, 80i64);
		break;
	case 0x1C:
		result = sub_7FFB87BAB130(v1, 3i64, 2i64);
		break;
	case 0x1E:
		result = sub_7FFB87BAB130(v1, 5i64, 96i64);
		break;
	default:
		LODWORD(v7) = *(_DWORD *)(v1 + 88);
		sub_7FFB87BA25E0(
			(char *)0x1F4,
			(signed __int64)"Got response during invalid Miracast Sink State: %i",
			(__int64)"MIRASINK",
			"Got response during invalid Miracast Sink State: %i",
			v7);
		result = sub_7FFB87BFEC80(v2);
		break;
	}*/
	return result;
}

void sub_7FFB87BAC4C0(__int64 a1, __int64 a2, __int64 a3, __int64 *a4)
{
	__int64 *v4; // rsi
	__int64 v5; // rbx
	_QWORD *v6; // rax
	const void *v7; // rdi

	v4 = a4;
	v5 = a3;
	v6 = (_QWORD*)malloc(0x10ui64);
	*v6 = (_QWORD)v4;
	v7 = v6;
	v6[1] = (_QWORD)sub_7FFB87BFE960(v5);
	sub_7FFB87933BD0(*v4, (__int64)sub_7FFB87BAC530, v7, 0i64, (__int64)sub_7FFB87BAC700);
}

int __fastcall sub_7FFB87BACA60(__int64 a1)
{
	//Started RTP UDP listeners
	return 0;
}

int __fastcall sub_7FFB87BAC860(__int64 a1)
{
	//Started RTP TCP listeners

	return 0;
}

int __fastcall sub_7FFB87BAC7A0(__int64 a1)
{
	//Started hardware cursor listener
	return 0;
}

__int64 sub_7FFB87BA4420(LPVOID lpParameter)
{
	LPVOID v1; // rdi
	char *v2; // rax
	char *v3; // rbp
	char *v4; // rax
	__int64 v5; // rsi
	__int64 v6; // rbx
	LPDWORD v7; // ST28_8
	void(__fastcall *v8)(LPVOID, _QWORD, _QWORD); // rax
	LPDWORD lpThreadId; // [rsp+28h] [rbp-30h]
	__int64 v11; // [rsp+30h] [rbp-28h]

	v1 = lpParameter;
	if (*(_QWORD *)((char *)lpParameter + 84) != 1i64)
		return 1i64;
	*((_BYTE *)lpParameter + 24) = 1;
	*((_QWORD *)lpParameter + 2) = (_QWORD)CreateThread(
		0i64,
		0i64,
		(LPTHREAD_START_ROUTINE)sub_7FFB87BAA4E0,
		lpParameter,
		0,
		0i64);
	sub_7FFB89B70D40((__int64 *)v1 + 4);
	sub_7FFB87C00410(
		*((_QWORD **)v1 + 4),
		(__int64)sub_7FFB87BAB4E0,
		(__int64)sub_7FFB87BAB360,
		(__int64)sub_7FFB87BAB190,
		(__int64)sub_7FFB87BAB7D0,
		(__int64)sub_7FFB87BAC730,
		(__int64)sub_7FFB87BAC1F0,
		(__int64)sub_7FFB87BAC4C0,
		(__int64)sub_7FFB87BAC750,
		(__int64)v1);
	sub_7FFB87C020D0(*((_QWORD *)v1 + 4), (unsigned char*)"Miracast Sink");
	sub_7FFB87C01E90(*((_QWORD *)v1 + 4), 500);
	v2 = (char *)sub_7FFB8794E300(*((_QWORD *)v1 + 6));
	sub_7FFB87BFF1F0(*((void **)v1 + 4), v2, 0, *((_WORD *)v1 + 28), 2, 32);
	sub_7FFB87C021A0(*((_QWORD *)v1 + 4), 1, 5, 10, 1);
	if (*((_DWORD *)v1 + 21) != 2)
		*((_DWORD *)v1 + 21) = 2;
	if (*((_DWORD *)v1 + 22) != 1)
		*((_DWORD *)v1 + 22) = 1;
	*((LARGE_INTEGER *)v1 + 491) = sub_7FFB8794FAC0();
	//v3 = sub_7FFB8794F8E0((__int64)"MiracastStateConnecting");
	//v4 = sub_7FFB8794F8E0((__int64)"MiracastSinkStateConnecting");
	//v5 = (__int64)v4;
	//if (v3 && v4)
	//{
	//	v6 = sub_7FFB8794E300((__int64)v4);
	//	v7 = (LPDWORD)sub_7FFB8794E300((__int64)v3);
	//	sub_7FFB87BA25E0(
	//		(char *)0x12C,
	//		(signed __int64)"Miracast state now %s, %s",
	//		(__int64)"MIRASINK",
	//		"Miracast state now %s, %s",
	//		v7,
	//		v6);
	//}
	//else
	//{
	//	LODWORD(v11) = 1;
	//	LODWORD(lpThreadId) = 2;
	//	sub_7FFB87BA25E0(
	//		(char *)0x12C,
	//		(signed __int64)"Miracast state now %i, %i",
	//		(__int64)"MIRASINK",
	//		"Miracast state now %i, %i",
	//		lpThreadId,
	//		v11);
	//}
	//if (v5)
	//	sub_7FFB87948710(v5);
	//if (v3)
	//	sub_7FFB87948710((__int64)v3);
	sub_7FFB87BACA60((__int64)v1);
	sub_7FFB87BAC860((__int64)v1);
	if (*((_BYTE *)v1 + 120))
		sub_7FFB87BAC7A0((__int64)v1);
	v8 = (void(__fastcall *)(LPVOID, _QWORD, _QWORD))*((_QWORD *)v1 + 501);
	if (v8)
		v8(v1, 0i64, *((_QWORD *)v1 + 492));
	return 0i64;
}

__int64 sub_7FFB87BA3C00(__int64 a1)
{
	sub_7FFB87BAB4E0(*(LPCRITICAL_SECTION *)(a1 + 32), 0, a1);
	return 0i64;
}

void __fastcall sub_7FFB87BDFA50(LPCRITICAL_SECTION lpCriticalSection, const CHAR *a2)
{
	LPCRITICAL_SECTION v2; // rdi
	__int64 v3; // rbx
	__int64 v4; // rbx
	__int64 v5; // rcx
	__int16 v6; // [rsp+30h] [rbp-28h]
	char pAddrBuf; // [rsp+34h] [rbp-24h]

	//v2 = lpCriticalSection;
	//v6 = 2;
	//inet_pton(2, a2, &pAddrBuf);
	//EnterCriticalSection(v2);
	//v3 = (__int64)v2[1].DebugInfo;
	//if (v3)
	//{
	//	while (!sub_7FFB87BB0B20(&v6, v3 + 692) || *(_DWORD *)(v3 + 4) != 2)
	//	{
	//		v3 = *(_QWORD *)(v3 + 8);
	//		if (!v3)
	//			goto LABEL_5;
	//	}
	//	v4 = sub_7FFB87BD1090(v3);
	//}
	//else
	//{
	//LABEL_5:
	//	v4 = 0i64;
	//}
	//LeaveCriticalSection(v2);
	//if (v4)
	//{
	//	//sub_7FFB87BA25E0(
	//	//	(char *)0x12C,
	//	//	(signed __int64)"Removing Miracast connection",
	//	//	(__int64)"RFSERVER",
	//	//	"Removing Miracast connection",
	//	//	0i64,
	//	//	*(_QWORD *)&v6);
	//	v5 = *(_QWORD *)(v4 + 512);
	//	if (v5)
	//		sub_7FFB87BA3C00(v5);
	//	Sleep(0xFAu);
	//	*(_BYTE *)(v4 + 804) = 0;
	//	sub_7FFB87BDF450(v2, (signed int *)v4);
	//	if (v2[25].DebugInfo && (unsigned __int8)sub_7FFB87BCFCE0(v4))
	//	{
	//		if ((unsigned __int8)sub_7FFB87BCFCD0(v4))
	//		{
	//			sub_7FFB87BB7A10((__int64)v2[25].DebugInfo, *(_DWORD *)v4);
	//			sub_7FFB87BD1C20(v4, 0);
	//		}
	//	}
	//	//sub_7FFB87BD0C90((void *)v4);
	//}
	//else
	//{
	//	//sub_7FFB87BA25E0(
	//	//	(char *)0x190,
	//	//	(signed __int64)"Miracast connection doesn't exist!",
	//	//	(__int64)"RFSERVER",
	//	//	"Miracast connection doesn't exist!",
	//	//	0i64,
	//	//	*(_QWORD *)&v6);
	//}
}

void __fastcall sub_7FFFC6D5D350(__int64 a1, __int64 a2, struct _RTL_CRITICAL_SECTION *a3)
{
	struct _RTL_CRITICAL_SECTION *v3; // rbx
	__int64 v4; // rdi
	const CHAR *v5; // rax

	printf("sub_7FFFC6D5D350 callback begin\n");

	v3 = a3;
	v4 = a2;
	sub_7FFB87948730(a2);
	v5 = (const CHAR *)sub_7FFB8794E300(v4);
	sub_7FFB87BDFA50(v3, v5);
	sub_7FFB87948710(v4);
}

char sub_7FFB89B13500(__int64 a1, __int64 a2, __int64 a3, __int64 a4, __int64 a5) 
{
	return 0;
}

int sub_7FFB89B13530(__int64 a1, unsigned int a2, __int64 a3, unsigned __int64 a4, __int64 a5) 
{
	return 0;
}

HANDLE  sub_7FFB898A4390(__int64 a1)
{
	__int64 v1; // rbx
	__int64 v2; // rcx
	HANDLE result; // rax

	v1 = a1;
	v2 = a1 + 8;
	*(_QWORD *)(v2 - 8) = 0i64;
	*(_QWORD *)v2 = 0i64;
	*(_QWORD *)(v2 + 8) = 0i64;
	*(_QWORD *)(v2 + 16) = 0i64;
	*(_QWORD *)(v2 + 24) = 0i64;
	*(_QWORD *)(v2 + 32) = 0i64;
	*(_QWORD *)(v2 + 48) = 0i64;
	*(_DWORD *)(v2 - 8) = 0;
	*(_QWORD *)(v2 + 40) = 0i64;
	InitializeCriticalSection((LPCRITICAL_SECTION)v2);
	result = CreateEventW(0i64, 1, 0, 0i64);
	*(_QWORD *)(v1 + 56) = (_QWORD)result;
	return result;
}

signed __int64 sub_7FFB898A2C70(SOCKET *a1) 
{
	SOCKET sock = socket(2, 1, 0);
	SOCKET sockarr[2];
	struct sockaddr name;
	int namelen = 0;
	name.sa_family = 2;
	*(_QWORD *)&name.sa_data[6] = 0i64;
	*(_WORD *)name.sa_data = htons(0);
	namelen = 16;
	*(_DWORD *)&name.sa_data[2] = htonl(0x7F000001u);

	int ret = bind(sock, (struct sockaddr*)&name, 16);
	if (ret == -1)
	{
		closesocket(sock);
		return -1;
	}
	ret = listen(sock, 1);
	if (ret == -1)
	{
		closesocket(sock);
		return -1;
	}
	if (getsockname(sock, (struct sockaddr*)&name, &namelen) == -1)
	{
		return -1;
	}
	sockarr[1] = socket(2, 1, 0);
	ret = connect(sockarr[1], (struct sockaddr*)&name, namelen);
	if (ret == -1)
	{
		closesocket(sock);
		return -1;
	}
	SOCKET sock3 = accept(sock, (struct sockaddr*)&name, &namelen);
	sockarr[0] = sock3;
	if (sock3 == -1)
	{
		closesocket(sockarr[1]);
		sockarr[1] = -1;
		closesocket(sock);
		return -1;
	}
	closesocket(sock);
	return 0;
}

char *sub_7FFB89B78700()
{
	char *v0; // rsi
	_QWORD *v1; // rbx
	struct _RTL_CRITICAL_SECTION *v2; // rdi
	signed __int64 v3; // rbp

	//sub_7FFB89B7A1A0();
	v0 = (char *)malloc(0x106A0ui64);
	memset(v0, 0, 0x106A0ui64);
	*(_DWORD *)(v0 + 2305) = 257;
	v1 = (_QWORD*)((char*)v0 + 2704);
	v2 = (struct _RTL_CRITICAL_SECTION *)(v0 + 2720);
	v3 = 96i64;
	do
	{
		*v1 = -1i64;
		InitializeCriticalSection(v2);
		v2 = (struct _RTL_CRITICAL_SECTION *)((char *)v2 + 664);
		v1 += 83;
		--v3;
	} while (v3);
	InitializeCriticalSection((LPCRITICAL_SECTION)(v0 + 1576));
	InitializeCriticalSection((LPCRITICAL_SECTION)(v0 + 2264));
	InitializeCriticalSection((LPCRITICAL_SECTION)v0);
	InitializeCriticalSection((LPCRITICAL_SECTION)(v0 + 1688));
	InitializeCriticalSection((LPCRITICAL_SECTION)(v0 + 72));
	sub_7FFB898A4390((__int64)(v0 + 112));
	InitializeCriticalSection((LPCRITICAL_SECTION)(v0 + 864));
	sub_7FFB898A4390((__int64)(v0 + 904));
	InitializeCriticalSection((LPCRITICAL_SECTION)(v0 + 728));
	sub_7FFB898A4390((__int64)(v0 + 768));
	InitializeCriticalSection((LPCRITICAL_SECTION)(v0 + 1536));
	sub_7FFB898A4390((__int64)(v0 + 1616));
	if ((signed int)sub_7FFB898A2C70((SOCKET *)v0 + 189) < 0)
		;//sub_7FFB89B125E0(
		//(char *)0x1F4,
		//	(signed __int64)"Send thread self-pipe creation failed",
		//	(__int64)"SOCKETSET",
		//	"Send thread self-pipe creation failed",
		//	0i64);
	return v0;
}

char * sub_7FFB89B70D40(__int64 *a1)
{
	__int64 *v1; // rbx
	char *result; // rax

	v1 = a1;
	result = sub_7FFB89B78700();
	*v1 = (__int64)result;
	return result;
}

__int64 sub_7FFB89B71E40(_QWORD *a1, __int64 a2, __int64 a3, __int64 a4, __int64 a5, __int64 a6, __int64 a7)
{
	__int64 result; // rax

	a1[8314] = a2;
	a1[8313] = a3;
	a1[8315] = a5;
	a1[8311] = a4;
	a1[8316] = a6;
	result = a7;
	a1[8310] = a7;
	return result;
}

char *__fastcall sub_7FFB89B12870(char *Src) 
{
	return NULL;
}

void sub_7FFB898B8710(__int64 a1) 
{

}

void sub_7FFB898A43F0(__int64 a1)
{
	__int64 v1; // rbx

	v1 = a1;
	EnterCriticalSection((LPCRITICAL_SECTION)(a1 + 8));
	if (*(_DWORD *)v1 > *(_DWORD *)(v1 + 48))
	{
		SetEvent(*(HANDLE *)(v1 + 56));
		++*(_DWORD *)(v1 + 48);
		++*(_DWORD *)(v1 + 52);
	}
	LeaveCriticalSection((LPCRITICAL_SECTION)(v1 + 8));
}

__int64 sub_7FFB89B77050(LPVOID lpThreadParameter, signed __int64 a2)
{
	return 0;
}

void sub_7FFB89B74030(LPVOID lpParameter, int a2)
{
	char *v2; // rbx
	unsigned __int64 v3; // r14
	unsigned int v4; // edi
	unsigned int v5; // edx
	__int64 v6; // rcx
	__int64 v7; // rax
	unsigned __int64 v8; // rax
	unsigned __int64 v9; // rax
	unsigned int v10; // ecx

	v2 = (char *)lpParameter;
	v3 = a2;
	EnterCriticalSection((LPCRITICAL_SECTION)((char *)lpParameter + 2264));
	v4 = 0;
	if (!*((_QWORD *)v2 + 107))
	{
		if ((signed int)sub_7FFB898A2C70((SOCKET *)v2 + 105) < 0)
		{
			//sub_7FFB89B125E0(
			//	(char *)0x1F4,
			//	(signed __int64)"Recv thread self-pipe creation failed",
			//	(__int64)"SOCKETSET",
			//	"Recv thread self-pipe creation failed",
			//	0i64);
			goto LABEL_24;
		}
		v5 = *((_DWORD *)v2 + 244);
		v6 = 0i64;
		if (v5)
		{
			do
			{
				if (*(_QWORD *)&v2[8 * v6 + 984] == *((_QWORD *)v2 + 105))
					break;
				v6 = (unsigned int)(v6 + 1);
			} while ((unsigned int)v6 < v5);
		}
		if ((_DWORD)v6 == v5 && v5 < 0x40)
		{
			*(_QWORD *)&v2[8 * v6 + 984] = *((_QWORD *)v2 + 105);
			++*((_DWORD *)v2 + 244);
		}
		*((_QWORD *)v2 + 188) = *((_QWORD *)v2 + 105);
		v7 = *((_QWORD *)v2 + 105);
		v2[968] = 1;
		*((_QWORD *)v2 + 187) = v7;
		*((_QWORD *)v2 + 107) = (_QWORD)CreateThread(0i64, 0i64, (LPTHREAD_START_ROUTINE)sub_7FFB89B77050, v2, 0, 0i64);
	}
	EnterCriticalSection((LPCRITICAL_SECTION)(v2 + 864));
	v8 = *((_QWORD *)v2 + 188);
	*((_DWORD *)v2 + 16612) = 0;
	if (v3 > v8 || !v8)
		*((_QWORD *)v2 + 188) = v3;
	v9 = *((_QWORD *)v2 + 187);
	if (v3 < v9 || !v9)
		*((_QWORD *)v2 + 187) = v3;
	v10 = *((_DWORD *)v2 + 244);
	if (v10)
	{
		do
		{
			if (*(_QWORD *)&v2[8 * v4 + 984] == v3)
				break;
			++v4;
		} while (v4 < v10);
	}
	if (v4 == v10 && v10 < 0x40)
	{
		*(_QWORD *)&v2[8 * v4 + 984] = v3;
		++*((_DWORD *)v2 + 244);
	}
	send(*((_QWORD *)v2 + 106), "B", 1, 0);
	sub_7FFB898A43F0((__int64)(v2 + 904));
	LeaveCriticalSection((LPCRITICAL_SECTION)(v2 + 864));
LABEL_24:
	LeaveCriticalSection((LPCRITICAL_SECTION)(v2 + 2264));
}

_DWORD * sub_7FFB898B8000(int a1)
{
	int v1; // ebx
	_DWORD *result; // rax

	v1 = a1;
	result = (_DWORD*)malloc(0x30ui64);
	result[1] = 1;
	*result = v1;
	result[2] = 0;
	return result;
}

_QWORD * sub_7FFB898BFF80(_QWORD *a1, __int64 a2, __int64 a3)
{
	_QWORD *v3; // rsi
	__int64 v4; // rdi
	__int64 v5; // rbx
	_QWORD *result; // rax
	_QWORD *i; // rcx

	v3 = a1;
	v4 = a3;
	v5 = a2;
	result = (_QWORD*)malloc(0x20ui64);
	result[2] = 0i64;
	result[3] = 0i64;
	*result = v5;
	result[1] = v4;
	if (v3)
	{
		for (i = v3; i[2]; i = (_QWORD *)i[2])
			;
		i[2] = *result;//???
		result[3] = *i;//???
		result = v3;
	}
	return result;
}

_DWORD * sub_7FFB898B2F40(size_t Size)
{
	unsigned int v1; // ebx
	_DWORD *v2; // rax
	_DWORD *v3; // rdi

	v1 = Size;
	v2 = sub_7FFB898B8000(3);
	v2[3] = 0;
	v2[4] = v1;
	v3 = v2;
	*((_QWORD *)v2 + 3) = *((_QWORD*)malloc(v1));
	return v3;
}

signed __int64 sub_7FFB89B74CE0(__int64 a1, int a2)
{
	__int64 v2; // r11
	int v3; // er10
	__int64 v4; // r9
	signed __int64 v5; // r8

	v2 = *(signed int *)(a1 + 2316);
	v3 = 0;
	v4 = 0i64;
	if (v2 < 0)
		return 0xFFFFFFFFi64;
	v5 = a1 + 2704;
	while (!*(_BYTE *)(v5 + 117) || *(_QWORD *)v5 != a2)
	{
		++v4;
		++v3;
		v5 += 664i64;
		if (v4 > v2)
			return 0xFFFFFFFFi64;
	}
	return *(unsigned int *)(664i64 * v3 + a1 + 2712);
}

char sub_7FFB89B713F0(LPCRITICAL_SECTION lpCriticalSection, int a2)
{
	int v2; // edi
	LPCRITICAL_SECTION v3; // rbx
	int v4; // er8
	unsigned int v5; // esi
	int v6; // ecx
	signed __int64 v7; // rax
	char result; // al
	signed __int64 v9; // r14
	SOCKET v10; // rcx
	SOCKET v11; // rbp
	void *v12; // rcx
	__int64 v13; // rcx
	_QWORD *v14; // rcx
	unsigned int v15; // edx
	__int64 v16; // rcx
	__int64 v17; // rdx
	unsigned int v18; // edx
	__int64 v19; // rcx
	__int64 v20; // rdx
	unsigned int v21; // ecx
	__int64 v22; // rcx

	v2 = a2;
	v3 = lpCriticalSection;
	EnterCriticalSection(lpCriticalSection);
	v4 = HIDWORD(v3[57].SpinCount);
	v5 = 0;
	v6 = 0;
	if (v4 < 0)
		goto LABEL_6;
	v7 = (signed __int64)&v3[67].SpinCount;
	while (!*(_BYTE *)(v7 + 109) || *(_DWORD *)v7 != v2)
	{
		++v6;
		v7 += 664i64;
		if (v6 > v4)
			goto LABEL_6;
	}
	v9 = (signed __int64)(&v3[67].LockSemaphore + 83 * v6);
	if (v9)
	{
		v10 = *(_QWORD *)v9;
		*(_BYTE *)(v9 + 117) = 0;
		shutdown(v10, 2);
		closesocket(*(_QWORD *)v9);
		v11 = *(_QWORD *)v9;
		*(_QWORD *)v9 = -1i64;
		*(_BYTE *)(v9 + 116) = 0;
		EnterCriticalSection((LPCRITICAL_SECTION)((char *)v3 + 1536));
		EnterCriticalSection((LPCRITICAL_SECTION)((char *)v3 + 1576));
		v12 = *(void **)(v9 + 120);
		if (v12)
		{
			*(_QWORD *)(v9 + 120) = 0i64;
			*(_QWORD *)(v9 + 136) = 0i64;
			free(v12);
		}
		LeaveCriticalSection((LPCRITICAL_SECTION)((char *)v3 + 1576));
		LeaveCriticalSection((LPCRITICAL_SECTION)((char *)v3 + 1536));
		EnterCriticalSection((LPCRITICAL_SECTION)((char *)v3 + 1688));
		v13 = *(_QWORD *)(v9 + 168);
		if (v13)
		{
			*(_QWORD *)(v9 + 168) = 0i64;
			sub_7FFB898B8710(v13);
		}
		LeaveCriticalSection((LPCRITICAL_SECTION)((char *)v3 + 1688));
		v14 = *(_QWORD **)(v9 + 312);
		if (v14)
		{
			*(_QWORD *)(v9 + 312) = 0i64;
			sub_7FFB89B75D00(v14);
		}
		DeleteCriticalSection((LPCRITICAL_SECTION)(v9 + 16));
		memset((void *)v9, 0, 0x298ui64);
		*(_QWORD *)v9 = -1i64;
		InitializeCriticalSection((LPCRITICAL_SECTION)(v9 + 16));
		EnterCriticalSection((LPCRITICAL_SECTION)((char *)v3 + 72));
		if (v3[1].LockSemaphore)
		{
			v15 = (unsigned int)v3[4].LockSemaphore;
			v16 = 0i64;
			if (v15)
			{
				while (*(&v3[4].SpinCount + v16) != v11)
				{
					v16 = (unsigned int)(v16 + 1);
					if ((unsigned int)v16 >= v15)
						goto LABEL_22;
				}
				if ((unsigned int)v16 < v15 - 1)
				{
					do
					{
						v17 = (unsigned int)(v16 + 1);
						*(&v3[4].SpinCount + v16) = *(&v3[4].SpinCount + v17);
						v16 = (unsigned int)(v16 + 1);
					} while ((unsigned int)v17 < LODWORD(v3[4].LockSemaphore) - 1);
				}
				--LODWORD(v3[4].LockSemaphore);
			}
		LABEL_22:
			send(*(_QWORD *)&v3[1].LockCount, "X", 1, 0);
			sub_7FFB898A43F0((__int64)&v3[2].SpinCount);
		}
		LeaveCriticalSection((LPCRITICAL_SECTION)((char *)v3 + 72));
		EnterCriticalSection((LPCRITICAL_SECTION)((char *)v3 + 864));
		if (v3[21].OwningThread)
		{
			v18 = (unsigned int)v3[24].OwningThread;
			v19 = 0i64;
			if (v18)
			{
				while (*((_QWORD *)&v3[24].LockSemaphore + v19) != v11)
				{
					v19 = (unsigned int)(v19 + 1);
					if ((unsigned int)v19 >= v18)
						goto LABEL_31;
				}
				if ((unsigned int)v19 < v18 - 1)
				{
					do
					{
						v20 = (unsigned int)(v19 + 1);
						*((_QWORD *)&v3[24].LockSemaphore + v19) = *((_QWORD *)&v3[24].LockSemaphore + v20);
						v19 = (unsigned int)(v19 + 1);
					} while ((unsigned int)v20 < LODWORD(v3[24].OwningThread) - 1);
				}
				--LODWORD(v3[24].OwningThread);
			}
		LABEL_31:
			send(*(_QWORD *)&v3[21].LockCount, "B", 1, 0);
			sub_7FFB898A43F0((__int64)&v3[22].LockSemaphore);
		}
		LeaveCriticalSection((LPCRITICAL_SECTION)((char *)v3 + 864));
		EnterCriticalSection((LPCRITICAL_SECTION)((char *)v3 + 1536));
		if (*(_QWORD *)&v3[38].LockCount)
		{
			v21 = v3[43].LockCount;
			if (v21)
			{
				while (*((_QWORD *)&v3[43].OwningThread + v5) != v11)
				{
					if (++v5 >= v21)
						goto LABEL_40;
				}
				if (v5 < v21 - 1)
				{
					do
					{
						v22 = v5++;
						*((_QWORD *)&v3[43].OwningThread + v22) = *((_QWORD *)&v3[43].OwningThread + v5);
					} while (v5 < v3[43].LockCount - 1);
				}
				--v3[43].LockCount;
			}
		LABEL_40:
			send((SOCKET)v3[38].DebugInfo, "A", 1, 0);
			sub_7FFB898A43F0((__int64)&v3[40].OwningThread);
		}
		LeaveCriticalSection((LPCRITICAL_SECTION)((char *)v3 + 1536));
		LeaveCriticalSection(v3);
		result = 1;
	}
	else
	{
	LABEL_6:
		LeaveCriticalSection(v3);
		result = 0;
	}
	return result;
}

char sub_7FFB89B76A10(LPCRITICAL_SECTION lpCriticalSection, __int64 a2, unsigned int a3, unsigned int a4)
{
	unsigned int v4; // er15
	int v5; // ebp
	__int64 v6; // rsi
	LPCRITICAL_SECTION v7; // rdi
	char v8; // r14
	int v9; // eax
	unsigned int v10; // esi
	__int64 v11; // rcx
	int v12; // ebx
	signed __int64 v13; // rax
	signed __int64 v14; // r8
	void(__fastcall *v15)(_QWORD, _QWORD, _QWORD, _QWORD); // rax
	__int64 v17; // [rsp+28h] [rbp-30h]
	__int64 v18; // [rsp+28h] [rbp-30h]
	__int64 v19; // [rsp+30h] [rbp-28h]

	v4 = a4;
	v5 = a3;
	v6 = a2;
	v7 = lpCriticalSection;
	if (!*(_BYTE *)(a2 + 12))
	{
		LODWORD(v19) = a3;
		LODWORD(v17) = *(_DWORD *)(a2 + 8);
		//sub_7FFB89B125E0(
		//	(char *)0x12C,
		//	(signed __int64)"Non optional sock tag: %i disconnect. on %i",
		//	(__int64)"SOCKETSET",
		//	"Non optional sock tag: %i disconnect. on %i",
		//	v17,
		//	v19);
		return 0;
	}
	EnterCriticalSection((LPCRITICAL_SECTION)(a2 + 16));
	v8 = *(_BYTE *)(v6 + 13);
	*(_BYTE *)(v6 + 13) = 1;
	LeaveCriticalSection((LPCRITICAL_SECTION)(v6 + 16));
	v9 = sub_7FFB89B74CE0((__int64)v7, v5);
	v10 = v9;
	if (v9 <= -1)
	{
		LODWORD(v17) = v5;
		//sub_7FFB89B125E0(
		//	(char *)0x1F4,
		//	(signed __int64)" RECV failed without removing socket: %i",
		//	(__int64)"SOCKETSET",
		//	" RECV failed without removing socket: %i",
		//	v17);
	}
	else
	{
		sub_7FFB89B713F0(v7, v9);
		LODWORD(v17) = v10;
		//sub_7FFB89B125E0(
		//	(char *)0x1F4,
		//	(signed __int64)"RECV failed. Removing socket with tag: %i",
		//	(__int64)"SOCKETSET",
		//	"RECV failed. Removing socket with tag: %i",
		//	v17);
	}
	EnterCriticalSection(v7);
	v11 = SHIDWORD(v7[57].SpinCount);
	v12 = 0;
	if ((signed int)v11 >= 0)
	{
		v13 = (signed __int64)&v7[67].LockSemaphore;
		v14 = v11 + 1;
		do
		{
			if (*(_QWORD *)v13 != -1i64 && *(_BYTE *)(v13 + 117))
				++v12;
			v13 += 664i64;
			--v14;
		} while (v14);
	}
	LODWORD(v19) = HIDWORD(v7[57].SpinCount);
	LODWORD(v18) = v12;
	//sub_7FFB89B125E0(
	//	(char *)0x1F4,
	//	(signed __int64)"Socket Err Recv.. %i remaining fd's of %i",
	//	(__int64)"SOCKETSET",
	//	"Socket Err Recv.. %i remaining fd's of %i",
	//	v18,
	//	v19);
	LeaveCriticalSection(v7);
	if (!BYTE4(v7[57].LockSemaphore) && v12 <= 0)
		return 0;
	if (!v8)
	{
		v15 = (void(__fastcall *)(_QWORD, _QWORD, _QWORD, _QWORD))v7[1663].LockSemaphore;
		if (v15)
			v15((_QWORD)v7, v10, v4, (_QWORD)v7[1662].DebugInfo);
	}
	return 1;
}

__int64 sub_7FFB89B743E0(LPCRITICAL_SECTION lpCriticalSection, unsigned int a2, __int64 a3, __int64 a4, __int64 a5, __int64 *a6, __int64 a7, char a8) 
{
	return 0;
}

__int64 sub_7FFB89B74760(LPVOID lpThreadParameter)
{
	char *v1; // rdi
	const char *v2; // rdx
	int v3; // edx
	int v4; // ecx
	_BYTE *v5; // rax
	int v6; // ebp
	signed __int64 v7; // rbx
	signed __int64 v8; // rsi
	int v9; // eax
	char v10; // al
	int v11; // edx
	__int64 v12; // r9
	__int64 v13; // rsi
	void(__fastcall *v14)(char *, signed __int64, _QWORD); // r10
	int v15; // er9
	int v16; // edx
	__int64 v17; // r8
	signed __int64 v18; // rax
	signed __int64 v19; // rdx
	char v20; // r8
	char Dest[8]; // [rsp+40h] [rbp-68h]
	__int64 v23; // [rsp+48h] [rbp-60h]
	__int64 v24; // [rsp+50h] [rbp-58h]
	__int64 v25; // [rsp+58h] [rbp-50h]
	__int64 v26; // [rsp+60h] [rbp-48h]
	__int64 v27; // [rsp+68h] [rbp-40h]
	__int64 v28; // [rsp+70h] [rbp-38h]
	__int64 v29; // [rsp+78h] [rbp-30h]

	v1 = (char *)lpThreadParameter;
	do
	{
		while (1)
		{
		LABEL_2:
			if (v1[832])
			{
				*(_QWORD *)Dest = 0i64;
				v23 = 0i64;
				v24 = 0i64;
				v25 = 0i64;
				v26 = 0i64;
				v27 = 0i64;
				v28 = 0i64;
				v29 = 0i64;
				if (*((_QWORD *)v1 + 8309))
				{
					strncat(Dest, "CN ", 0x40ui64);
					v2 = (const char *)*((_QWORD *)v1 + 8309);
				}
				else
				{
					v2 = "Connect Socket";
				}
				strncat(Dest, v2, 0x40ui64);
				//nullsub_19(Dest);
				v1[832] = 0;
			}
			EnterCriticalSection((LPCRITICAL_SECTION)(v1 + 728));
			v3 = *((_DWORD *)v1 + 579);
			v4 = 0;
			if (v3 >= 0)
				break;
		LABEL_13:
			sub_7FFB898A4440((__int64)(v1 + 768), (struct _RTL_CRITICAL_SECTION *)(v1 + 728));
			LeaveCriticalSection((LPCRITICAL_SECTION)(v1 + 728));
			if (!v1[2305])
				return 0i64;
		}
		v5 = (unsigned char*)(v1 + 2821);
		while (*(v5 - 1) || *v5 != 1 || *(_DWORD *)(v5 - 57) != 2)
		{
			++v4;
			v5 += 664;
			if (v4 > v3)
				goto LABEL_13;
		}
		LeaveCriticalSection((LPCRITICAL_SECTION)(v1 + 728));
		v6 = 0;
	} while (*((_DWORD *)v1 + 579) < 0);
	v7 = (signed __int64)(v1 + 2764);
	while (1)
	{
		if (*(_BYTE *)(v7 + 56) || *(_BYTE *)(v7 + 57) != 1 || *(_DWORD *)v7 != 2)
			goto LABEL_39;
		v8 = (signed __int64)&v1[664 * v6];
		v9 = connect(*(_QWORD *)(v7 - 60), (const struct sockaddr *)(v8 + 2784), 16);
		if (v9 >= 0)
			break;
		v10 = sub_7FFB89B76A10((LPCRITICAL_SECTION)v1, v8 + 2704, *(_DWORD *)(v7 - 60), v9);
		if (v1[56])
		{
			//sub_7FFB89B125E0(
			//	(char *)0x64,
			//	(signed __int64)"Socket set close internal from connect thread",
			//	(__int64)"SOCKETSET",
			//	"Socket set close internal from connect thread");
			v20 = 0;
			v11 = 0;
			goto LABEL_45;
		}
		if (!v10)
		{
			v11 = 12293;
			goto LABEL_44;
		}
	LABEL_39:
		++v6;
		v7 += 664i64;
		if (v6 > *((_DWORD *)v1 + 579))
			goto LABEL_2;
	}
	if (!*(_BYTE *)(v7 + 264))
	{
	LABEL_30:
		v13 = *(signed int *)(v7 - 60);
		*(_BYTE *)(v7 + 56) = 1;
		v14 = (void(__fastcall *)(char *, signed __int64, _QWORD))*((_QWORD *)v1 + 8313);
		if (v14)
		{
			v15 = *((_DWORD *)v1 + 579);
			v16 = 0;
			v17 = 0i64;
			if (v15 < 0)
			{
			LABEL_36:
				v19 = 0xFFFFFFFFi64;
			}
			else
			{
				v18 = (signed __int64)(v1 + 2704);
				while (!*(_BYTE *)(v18 + 117) || *(_QWORD *)v18 != v13)
				{
					++v16;
					++v17;
					v18 += 664i64;
					if (v16 > v15)
						goto LABEL_36;
				}
				v19 = *(unsigned int *)&v1[664 * v17 + 2712];
			}
			v14(v1, v19, *((_QWORD *)v1 + 8310));
		}
		sub_7FFB89B74030(v1, v13);
		goto LABEL_39;
	}
	if (*(_BYTE *)(v7 + 266))
		v12 = v8 + 3030;
	else
		v12 = 0i64;
	if (!(unsigned int)sub_7FFB89B743E0(
		(LPCRITICAL_SECTION)(v8 + 3304),
		*(_DWORD *)(v7 - 60),
		0i64,
		0i64,
		v8 + 3288,
		(__int64 *)(v8 + 3296),
		v12,
		*(_DWORD *)(v7 + 68)))
	{
		*(_BYTE *)(v7 + 265) = 1;
		goto LABEL_30;
	}
	v11 = 12295;
LABEL_44:
	v20 = 1;
LABEL_45:
	sub_7FFB89B77E80(v1, v11, v20);
	return 0i64;
}

void sub_7FFB89B74AD0(LPVOID lpParameter)
{
	char *v1; // rdi

	v1 = (char *)lpParameter;
	if (!*((_QWORD *)lpParameter + 90))
	{
		*((_BYTE *)lpParameter + 832) = 1;
		*((_QWORD *)lpParameter + 90) = (_QWORD)CreateThread(
			0i64,
			0i64,
			(LPTHREAD_START_ROUTINE)sub_7FFB89B74760,
			lpParameter,
			0,
			0i64);
	}
	EnterCriticalSection((LPCRITICAL_SECTION)(v1 + 728));
	sub_7FFB898A43F0((__int64)(v1 + 768));
	LeaveCriticalSection((LPCRITICAL_SECTION)(v1 + 728));
}

void sub_7FFB898A4440(__int64 a1, struct _RTL_CRITICAL_SECTION *a2)
{
	__int64 v2; // rbx
	struct _RTL_CRITICAL_SECTION *v3; // r15
	int v4; // er14
	int v5; // ebp
	signed int v6; // edi
	bool v7; // zf
	int v8; // eax
	int v9; // eax

	v2 = a1;
	v3 = a2;
	EnterCriticalSection((LPCRITICAL_SECTION)(a1 + 8));
	++*(_DWORD *)v2;
	v4 = *(_DWORD *)(v2 + 52);
	LeaveCriticalSection((LPCRITICAL_SECTION)(v2 + 8));
	LeaveCriticalSection(v3);
	v5 = 0;
	while (WaitForSingleObject(*(HANDLE *)(v2 + 56), 0xFFFFFFFF) != 258)
	{
		EnterCriticalSection((LPCRITICAL_SECTION)(v2 + 8));
		if (*(_DWORD *)(v2 + 48) <= 0 || (v6 = 1, *(_DWORD *)(v2 + 52) == v4))
			v6 = 0;
		LeaveCriticalSection((LPCRITICAL_SECTION)(v2 + 8));
		if (v6)
		{
			EnterCriticalSection(v3);
			EnterCriticalSection((LPCRITICAL_SECTION)(v2 + 8));
			--*(_DWORD *)v2;
			v7 = (*(_DWORD *)(v2 + 48))-- == 1;
		LABEL_11:
			LOBYTE(v5) = v7;
			goto LABEL_12;
		}
	}
	EnterCriticalSection(v3);
	EnterCriticalSection((LPCRITICAL_SECTION)(v2 + 8));
	v8 = *(_DWORD *)(v2 + 48);
	--*(_DWORD *)v2;
	if (v8 && *(_DWORD *)(v2 + 52) != v4)
	{
		v9 = v8 - 1;
		v7 = v9 == 0;
		*(_DWORD *)(v2 + 48) = v9;
		goto LABEL_11;
	}
LABEL_12:
	LeaveCriticalSection((LPCRITICAL_SECTION)(v2 + 8));
	if (v5)
		ResetEvent(*(HANDLE *)(v2 + 56));
}

void sub_7FFB898C0090(void *Memory, void(__fastcall *a2)(_QWORD, _QWORD))
{
	void(__fastcall *v2)(_QWORD, _QWORD); // rsi
	_QWORD *v3; // rbx
	_QWORD *v4; // rdi

	if (Memory)
	{
		v2 = a2;
		v3 = (_QWORD*)Memory;
		do
		{
			v4 = (_QWORD *)v3[2];
			if (v2)
				v2(*v3, v3[1]);
			*v3 = 0i64;
			v3[1] = 0i64;
			v3[3] = 0i64;
			v3[2] = 0i64;
			free(v3);
			v3 = v4;
		} while (v4);
	}
}

int sub_7FFB898A2DD0(SOCKET a1, char *a2, int a3)
{
	int result; // eax
	int v4; // ebx

	result = recv(a1, a2, a3, 0);
	v4 = result;
	if (result < 0)
	{
		if (WSAGetLastError() == 10054)
			v4 = 0;
		result = v4;
	}
	return result;
}

void sub_7FFB89B75D00(_QWORD *a1)
{
	_QWORD *v1; // rbx
	void *v2; // rcx
	int v3; // edi
	void *v4; // rcx
	void *v5; // rcx
	void *v6; // rcx
	__int64 v7; // rsi
	__int64 v8; // rbp

	v1 = a1;
	v2 = (void *)a1[5];
	v3 = 0;
	if (v2)
	{
		v1[5] = 0i64;
		free(v2);
	}
	v4 = (void *)v1[2];
	if (v4)
	{
		v1[2] = 0i64;
		free(v4);
	}
	v5 = (void *)v1[3];
	if (v5)
	{
		v1[3] = 0i64;
		free(v5);
	}
	v6 = (void *)v1[1];
	if (v6)
	{
		v1[1] = 0i64;
		free(v6);
	}
	v7 = v1[4];
	if (v7)
	{
		v1[4] = 0i64;
		if (*(_DWORD *)(v7 + 16) > 0)
		{
			v8 = 0i64;
			do
			{
				free(*(void **)(*(_QWORD *)v7 + v8));
				free(*(void **)(*(_QWORD *)(v7 + 8) + v8));
				++v3;
				v8 += 8i64;
			} while (v3 < *(_DWORD *)(v7 + 16));
		}
		free(*(void **)v7);
		free(*(void **)(v7 + 8));
		free((void *)v7);
	}
	free(v1);
}

BOOL sub_7FFB898A4360(__int64 a1)
{
	__int64 v1; // rbx

	v1 = a1;
	DeleteCriticalSection((LPCRITICAL_SECTION)(a1 + 8));
	return CloseHandle(*(HANDLE *)(v1 + 56));
}

void sub_7FFB89B77E80(void *Memory, int a2, char a3)
{
	char v3; // r13
	int v4; // er15
	char *v5; // rbx
	char v6; // di
	void *v7; // rcx
	struct _RTL_CRITICAL_SECTION *v8; // r12
	signed __int64 v9; // rax
	int v10; // er14
	signed __int64 v11; // rdi
	void *v12; // rcx
	_QWORD *v13; // rsi
	__int64 v14; // rbp
	__int64 v15; // rax
	__int64 v16; // rax
	SOCKET v17; // rsi
	SOCKET v18; // rcx
	void *v19; // rcx
	__int64 v20; // rcx
	_QWORD *v21; // rcx
	void *v22; // rcx
	DWORD v23; // edi
	void *v24; // rcx
	DWORD v25; // edi
	void *v26; // rcx
	DWORD v27; // edi
	void *v28; // rcx
	DWORD v29; // edi
	SOCKET v30; // rax
	SOCKET v31; // rdi
	SOCKET v32; // rsi
	SOCKET v33; // rax
	SOCKET v34; // rdi
	SOCKET v35; // rsi
	SOCKET v36; // rax
	SOCKET v37; // rdi
	SOCKET v38; // rsi
	signed int v39; // edi
	int v40; // esi
	__int64 *v41; // rdi
	bool v42; // zf
	struct _RTL_CRITICAL_SECTION *v43; // rdi
	signed __int64 v44; // rsi
	void(__fastcall *v45)(char *, _QWORD, _QWORD); // rax
	void *v46; // rcx

	v3 = a3;
	v4 = a2;
	v5 = (char *)Memory;
	EnterCriticalSection((LPCRITICAL_SECTION)Memory);
	if (v5[2305])
	{
		*((_WORD *)v5 + 1152) = 1;
		*((_DWORD *)v5 + 16646) = v4;
		v6 = 1;
	}
	else
	{
		v6 = 0;
	}
	LeaveCriticalSection((LPCRITICAL_SECTION)v5);
	if (v6)
	{
		v7 = (void *)*((_QWORD *)v5 + 8324);
		if (v7)
		{
			sub_7FFB898C0090(v7, (void(__fastcall *)(_QWORD, _QWORD))free);
			*((_QWORD *)v5 + 8324) = 0i64;
		}
		if (*((_QWORD *)v5 + 90))
		{
			EnterCriticalSection((LPCRITICAL_SECTION)(v5 + 728));
			sub_7FFB898A43F0((__int64)(v5 + 768));
			LeaveCriticalSection((LPCRITICAL_SECTION)(v5 + 728));
		}
		if (*((_QWORD *)v5 + 8))
		{
			send(*((_QWORD *)v5 + 6), "X", 1, 0);
			EnterCriticalSection((LPCRITICAL_SECTION)(v5 + 72));
			sub_7FFB898A43F0((__int64)(v5 + 112));
			LeaveCriticalSection((LPCRITICAL_SECTION)(v5 + 72));
		}
		if (*((_QWORD *)v5 + 107))
		{
			send(*((_QWORD *)v5 + 106), "B", 1, 0);
			EnterCriticalSection((LPCRITICAL_SECTION)(v5 + 864));
			sub_7FFB898A43F0((__int64)(v5 + 904));
			LeaveCriticalSection((LPCRITICAL_SECTION)(v5 + 864));
			send(*((_QWORD *)v5 + 106), "B", 1, 0);
		}
		if (*((_QWORD *)v5 + 191))
		{
			send(*((_QWORD *)v5 + 190), "A", 1, 0);
			EnterCriticalSection((LPCRITICAL_SECTION)(v5 + 1536));
			sub_7FFB898A43F0((__int64)(v5 + 1616));
			LeaveCriticalSection((LPCRITICAL_SECTION)(v5 + 1536));
		}
		v8 = (struct _RTL_CRITICAL_SECTION *)(v5 + 1576);
		EnterCriticalSection((LPCRITICAL_SECTION)(v5 + 1576));
		v10 = 0;
		if (*((_DWORD *)v5 + 579) >= 0)
		{
			v11 = (signed __int64)(v5 + 3296);
			do
			{
				v12 = *(void **)(v11 - 288);
				if (v12)
				{
					*(_QWORD *)(v11 - 288) = 0i64;
					sub_7FFB898C0090(v12, (void(__fastcall *)(_QWORD, _QWORD))free);
				}
				if (v4 == 12298)
					;// LODWORD(v9) = sub_7FFB89B125E0((char *)0x1F4, v9, (__int64)"SOCKETSET", "ssl disconnect error.", 0i64);
				if (*(_BYTE *)(v11 - 268))
				{
					v13 = *(_QWORD **)v11;
					if (*(_QWORD *)v11)
					{
						v14 = *(_QWORD *)(v11 - 8);
						*(_QWORD *)v11 = 0i64;
						*(_QWORD *)(v11 - 8) = 0i64;
						if (*(_BYTE *)(v11 - 267))
						{
							//v15 = sub_7FFB89DA6810(0i64, (__int64)v13);
							//v16 = sub_7FFB89DA35A0(v15, (__int64)v13);
							//sub_7FFB89DA34D0(v16, (__int64)v13);
							//sub_7FFB89DA4270(v13);
							//sub_7FFB89DA2500(v14);
						}
						else if (!v4)
						{
							//sub_7FFB89DA6810(0i64, (__int64)v13);
						}
						DeleteCriticalSection((LPCRITICAL_SECTION)(v11 + 8));
						*(_WORD *)(v11 - 268) = 0;
					}
				}
				v17 = *(_QWORD *)(v11 - 592);
				if (v17 != -1i64)
				{
					v18 = *(_QWORD *)(v11 - 592);
					*(_BYTE *)(v11 - 476) = 0;
					*(_QWORD *)(v11 - 592) = -1i64;
					shutdown(v18, 2);
					closesocket(v17);
					v19 = *(void **)(v11 - 472);
					if (v19)
					{
						*(_QWORD *)(v11 - 472) = 0i64;
						free(v19);
					}
					EnterCriticalSection((LPCRITICAL_SECTION)(v5 + 1688));
					v20 = *(_QWORD *)(v11 - 424);
					if (v20)
					{
						*(_QWORD *)(v11 - 424) = 0i64;
						sub_7FFB898B8710(v20);
					}
					LeaveCriticalSection((LPCRITICAL_SECTION)(v5 + 1688));
					v21 = *(_QWORD **)(v11 - 280);
					if (v21)
					{
						*(_QWORD *)(v11 - 280) = 0i64;
						sub_7FFB89B75D00(v21);
					}
				}
				++v10;
				v11 += 664i64;
			} while (v10 <= *((_DWORD *)v5 + 579));
			v8 = (struct _RTL_CRITICAL_SECTION *)(v5 + 1576);
		}
		LeaveCriticalSection(v8);
		v22 = (void *)*((_QWORD *)v5 + 90);
		if (v22)
		{
			v23 = GetThreadId(v22);
			if (GetCurrentThreadId() != v23)
				WaitForSingleObject(*((HANDLE *)v5 + 90), 0xFFFFFFFF);
			*((_QWORD *)v5 + 90) = 0i64;
		}
		v24 = (void *)*((_QWORD *)v5 + 8);
		if (v24)
		{
			v25 = GetThreadId(v24);
			if (GetCurrentThreadId() != v25)
				WaitForSingleObject(*((HANDLE *)v5 + 8), 0xFFFFFFFF);
			*((_QWORD *)v5 + 8) = 0i64;
		}
		v26 = (void *)*((_QWORD *)v5 + 107);
		if (v26)
		{
			v27 = GetThreadId(v26);
			if (GetCurrentThreadId() != v27)
				WaitForSingleObject(*((HANDLE *)v5 + 107), 0xFFFFFFFF);
			*((_QWORD *)v5 + 107) = 0i64;
		}
		v28 = (void *)*((_QWORD *)v5 + 191);
		if (v28)
		{
			v29 = GetThreadId(v28);
			if (GetCurrentThreadId() != v29)
				WaitForSingleObject(*((HANDLE *)v5 + 191), 0xFFFFFFFF);
			*((_QWORD *)v5 + 191) = 0i64;
		}
		if (*((_QWORD *)v5 + 105))
		{
			v30 = *((signed int *)v5 + 210);
			v31 = *((signed int *)v5 + 212);
			*((_QWORD *)v5 + 105) = -1i64;
			*((_QWORD *)v5 + 106) = -1i64;
			v32 = v30;
			shutdown(v30, 2);
			shutdown(v31, 2);
			closesocket(v32);
			closesocket(v31);
		}
		if (*((_QWORD *)v5 + 5))
		{
			v33 = *((signed int *)v5 + 10);
			v34 = *((signed int *)v5 + 12);
			*((_QWORD *)v5 + 5) = -1i64;
			*((_QWORD *)v5 + 6) = -1i64;
			v35 = v33;
			shutdown(v33, 2);
			shutdown(v34, 2);
			closesocket(v35);
			closesocket(v34);
		}
		if (*((_QWORD *)v5 + 189))
		{
			v36 = *((signed int *)v5 + 378);
			v37 = *((signed int *)v5 + 380);
			*((_QWORD *)v5 + 189) = -1i64;
			*((_QWORD *)v5 + 190) = -1i64;
			v38 = v36;
			shutdown(v36, 2);
			shutdown(v37, 2);
			closesocket(v38);
			closesocket(v37);
		}
		sub_7FFB898A4360((__int64)(v5 + 768));
		DeleteCriticalSection((LPCRITICAL_SECTION)(v5 + 728));
		sub_7FFB898A4360((__int64)(v5 + 112));
		DeleteCriticalSection((LPCRITICAL_SECTION)(v5 + 72));
		sub_7FFB898A4360((__int64)(v5 + 904));
		DeleteCriticalSection((LPCRITICAL_SECTION)(v5 + 864));
		sub_7FFB898A4360((__int64)(v5 + 1616));
		DeleteCriticalSection((LPCRITICAL_SECTION)(v5 + 1536));
		DeleteCriticalSection(v8);
		DeleteCriticalSection((LPCRITICAL_SECTION)(v5 + 1688));
		DeleteCriticalSection((LPCRITICAL_SECTION)(v5 + 2264));
		v39 = 0;
		if (v5[2307])
		{
			//sub_7FFB89B125E0(
			//	(char *)0xC8,
			//	(signed __int64)"Busy wait for direct-send to finish",
			//	(__int64)"SOCKETSET",
			//	"Busy wait for direct-send to finish",
			//	0i64);
			for (; v5[2307]; ++v39)
			{
				if (v39 >= 1000)
					break;
				Sleep(1u);
			}
		}
		v40 = 0;
		if (*((_DWORD *)v5 + 579) >= 0)
		{
			v41 = (__int64 *)(v5 + 3352);
			do
			{
				if (*((_BYTE *)v41 - 8))
				{
					v42 = *((_BYTE *)v41 - 7) == 0;
					*((_BYTE *)v41 - 8) = 0;
					if (!v42)
					{
						//sub_7FFB898C26E0(*v41);
						//sub_7FFB898C26E0(v41[1]);
						//free((void *)*v41);
						//free((void *)v41[1]);
					}
					*v41 = 0i64;
					v41[1] = 0i64;
				}
				++v40;
				v41 += 83;
			} while (v40 <= *((_DWORD *)v5 + 579));
		}
		if (v5[66600])
		{
			v5[66600] = 0;
			//sub_7FFB898C26E0((__int64)(v5 + 66608));
			//sub_7FFB898C26E0((__int64)(v5 + 66920));
		}
		v43 = (struct _RTL_CRITICAL_SECTION *)(v5 + 2720);
		v44 = 96i64;
		do
		{
			DeleteCriticalSection(v43);
			v43 = (struct _RTL_CRITICAL_SECTION *)((char *)v43 + 664);
			--v44;
		} while (v44);
		v5[2304] = 0;
		if (v3)
		{
			v45 = (void(__fastcall *)(char *, _QWORD, _QWORD))*((_QWORD *)v5 + 8314);
			if (v45)
				v45(v5, *((unsigned int *)v5 + 16646), *((_QWORD *)v5 + 8310));
		}
		DeleteCriticalSection((LPCRITICAL_SECTION)v5);
		v46 = (void *)*((_QWORD *)v5 + 8309);
		if (v46)
		{
			*((_QWORD *)v5 + 8309) = 0i64;
			free(v46);
		}
		free(v5);
	}
}

#define __int128 __int64
#define _OWORD __int64

__int64 __fastcall sub_7FFB89B76C20(LPVOID lpThreadParameter)
{
	char *v1; // rbx
	const char *v2; // rdx
	__int128 *v3; // rax
	fd_set *v4; // rcx
	signed __int64 v5; // rdx
	__int128 v6; // xmm0
	__int128 v7; // xmm1
	__int128 v8; // xmm0
	__int128 v9; // xmm1
	__int128 v10; // xmm0
	__int128 v11; // xmm1
	__int128 v12; // xmm0
	__int128 v13; // xmm1
	int v14; // esi
	int v15; // ebp
	__int64 v16; // rcx
	int v17; // eax
	unsigned int v18; // er10
	int v19; // edx
	void(__fastcall *v20)(char *, signed __int64, _QWORD, struct sockaddr *, _QWORD); // r11
	int v21; // er8
	int v22; // ecx
	__int64 v23; // rdx
	signed __int64 v24; // rax
	signed __int64 v25; // rdx
	char v26; // r8
	int addrlen; // [rsp+30h] [rbp-6A8h]
	struct sockaddr addr; // [rsp+38h] [rbp-6A0h]
	fd_set readfds; // [rsp+50h] [rbp-688h]
	char Dest[8*6]; // [rsp+260h] [rbp-478h]
	//__int64 v32; // [rsp+268h] [rbp-470h]
	//__int64 v33; // [rsp+270h] [rbp-468h]
	//__int64 v34; // [rsp+278h] [rbp-460h]
	//__int64 v35; // [rsp+280h] [rbp-458h]
	//__int64 v36; // [rsp+288h] [rbp-450h]
	//__int64 v37; // [rsp+290h] [rbp-448h]
	__int64 v38; // [rsp+298h] [rbp-440h]
	char v39; // [rsp+2A0h] [rbp-438h]

	v1 = (char *)lpThreadParameter;
	readfds.fd_count = 0;
	while (1)
	{
		while (1)
		{
			if (v1[176])
			{
				*(_QWORD *)Dest = 0i64;
				//v32 = 0i64;
				//v33 = 0i64;
				//v34 = 0i64;
				//v35 = 0i64;
				//v36 = 0i64;
				//v37 = 0i64;
				v38 = 0i64;
				if (*((_QWORD *)v1 + 8309))
				{
					strncat(Dest, "LN ", 0x40ui64);
					v2 = (const char *)*((_QWORD *)v1 + 8309);
				}
				else
				{
					v2 = "Listen Socket";
				}
				strncat(Dest, v2, 0x40ui64);
				//nullsub_19(Dest);
				v1[176] = 0;
			}
			EnterCriticalSection((LPCRITICAL_SECTION)(v1 + 72));
			v3 = (__int128 *)(v1 + 184);
			v4 = &readfds;
			v5 = 4i64;
			do
			{
				v6 = *v3;
				v7 = v3[1];
				v4 = (fd_set *)((char *)v4 + 128);
				v3 += 8;
				*(_OWORD *)&v4[-1].fd_array[48] = v6;
				v8 = *(v3 - 6);
				*(_OWORD *)&v4[-1].fd_array[50] = v7;
				v9 = *(v3 - 5);
				*(_OWORD *)&v4[-1].fd_array[52] = v8;
				v10 = *(v3 - 4);
				*(_OWORD *)&v4[-1].fd_array[54] = v9;
				v11 = *(v3 - 3);
				*(_OWORD *)&v4[-1].fd_array[56] = v10;
				v12 = *(v3 - 2);
				*(_OWORD *)&v4[-1].fd_array[58] = v11;
				v13 = *(v3 - 1);
				*(_OWORD *)&v4[-1].fd_array[60] = v12;
				*(_OWORD *)&v4[-1].fd_array[62] = v13;
				--v5;
			} while (v5);
			v14 = *((_DWORD *)v1 + 176);
			v15 = *((_DWORD *)v1 + 178);
			*(_QWORD *)&v4->fd_count = *(_QWORD *)v3;
			if (v14 || v15)
				break;
			sub_7FFB898A4440((__int64)(v1 + 112), (struct _RTL_CRITICAL_SECTION *)(v1 + 72));
			LeaveCriticalSection((LPCRITICAL_SECTION)(v1 + 72));
			if (!v1[2305])
				return 0i64;
		}
		LeaveCriticalSection((LPCRITICAL_SECTION)(v1 + 72));
		if (select(v15 + 1, &readfds, 0i64, 0i64, 0i64) < 0)
			break;
		if (!v1[2305])
			return 0i64;
		for (; v14 <= v15; ++v14)
		{
			if (__WSAFDIsSet(v14, &readfds))
			{
				v16 = *((_QWORD *)v1 + 5);
				if (v14 == v16)
				{
					sub_7FFB898A2DD0(v16, &v39, 1024i64);
				}
				else
				{
					addrlen = 16;
					v17 = accept(v14, &addr, &addrlen);
					v18 = v17;
					if (v17 >= 0)
					{
						v20 = (void(__fastcall *)(char *, signed __int64, _QWORD, struct sockaddr *, _QWORD))*((_QWORD *)v1 + 8311);
						if (v20)
						{
							v21 = *((_DWORD *)v1 + 579);
							v22 = 0;
							v23 = 0i64;
							if (v21 < 0)
							{
							LABEL_28:
								v25 = 0xFFFFFFFFi64;
							}
							else
							{
								v24 = (signed __int64)(v1 + 2704);
								while (!*(_BYTE *)(v24 + 117) || *(_QWORD *)v24 != v14)
								{
									++v22;
									++v23;
									v24 += 664i64;
									if (v22 > v21)
										goto LABEL_28;
								}
								v25 = *(unsigned int *)&v1[664 * v23 + 2712];
							}
							v20(v1, v25, v18, &addr, *((_QWORD *)v1 + 8310));
						}
						else
						{
							closesocket(v17);
						}
					}
					else if (!v1[2306])
					{
						v19 = 12290;
						goto LABEL_36;
					}
				}
			}
		}
		if (v1[56])
		{
			//sub_7FFB89B125E0(
			//	(char *)0x64,
			//	(signed __int64)"Socket set close internal from listen thread",
			//	(__int64)"SOCKETSET",
			//	"Socket set close internal from listen thread");
			v26 = 0;
			v19 = 0;
			goto LABEL_37;
		}
	}
	v19 = 12289;
LABEL_36:
	v26 = 1;
LABEL_37:
	sub_7FFB89B77E80(v1, v19, v26);
	return 0i64;
}

void sub_7FFB89B73E70(LPVOID lpParameter, int a2)
{
	char *v2; // rbx
	unsigned __int64 v3; // r14
	unsigned int v4; // edi
	unsigned int v5; // edx
	__int64 v6; // rcx
	__int64 v7; // rax
	unsigned __int64 v8; // rax
	unsigned __int64 v9; // rax
	unsigned int v10; // ecx

	v2 = (char *)lpParameter;
	v3 = a2;
	EnterCriticalSection((LPCRITICAL_SECTION)((char *)lpParameter + 2264));
	v4 = 0;
	if (!*((_QWORD *)v2 + 8))
	{
		if ((signed int)sub_7FFB898A2C70((SOCKET *)v2 + 5) < 0)
		{
			//sub_7FFB89B125E0(
			//	(char *)0x1F4,
			//	(signed __int64)"Listen thread self-pipe creation failed",
			//	(__int64)"SOCKETSET",
			//	"Listen thread self-pipe creation failed",
			//	0i64);
			goto LABEL_24;
		}
		v5 = *((_DWORD *)v2 + 46);
		v6 = 0i64;
		if (v5)
		{
			do
			{
				if (*(_QWORD *)&v2[8 * v6 + 192] == *((_QWORD *)v2 + 5))
					break;
				v6 = (unsigned int)(v6 + 1);
			} while ((unsigned int)v6 < v5);
		}
		if ((_DWORD)v6 == v5 && v5 < 0x40)
		{
			*(_QWORD *)&v2[8 * v6 + 192] = *((_QWORD *)v2 + 5);
			++*((_DWORD *)v2 + 46);
		}
		*((_QWORD *)v2 + 89) = *((_QWORD *)v2 + 5);
		v7 = *((_QWORD *)v2 + 5);
		v2[176] = 1;
		*((_QWORD *)v2 + 88) = v7;
		*((_QWORD *)v2 + 8) = (_QWORD)CreateThread(0i64, 0i64, (LPTHREAD_START_ROUTINE)sub_7FFB89B76C20, v2, 0, 0i64);
	}
	EnterCriticalSection((LPCRITICAL_SECTION)(v2 + 72));
	v8 = *((_QWORD *)v2 + 89);
	if (v3 > v8 || !v8)
		*((_QWORD *)v2 + 89) = v3;
	v9 = *((_QWORD *)v2 + 88);
	if (v3 < v9 || !v9)
		*((_QWORD *)v2 + 88) = v3;
	v10 = *((_DWORD *)v2 + 46);
	if (v10)
	{
		do
		{
			if (*(_QWORD *)&v2[8 * v4 + 192] == v3)
				break;
			++v4;
		} while (v4 < v10);
	}
	if (v4 == v10 && v10 < 0x40)
	{
		*(_QWORD *)&v2[8 * v4 + 192] = v3;
		++*((_DWORD *)v2 + 46);
	}
	send(*((_QWORD *)v2 + 6), "X", 1, 0);
	sub_7FFB898A43F0((__int64)(v2 + 112));
	LeaveCriticalSection((LPCRITICAL_SECTION)(v2 + 72));
LABEL_24:
	LeaveCriticalSection((LPCRITICAL_SECTION)(v2 + 2264));
}

char sub_7FFB89B73210(LPVOID lpParameter, char *Src, __int16 a3, u_short a4, int a5, char a6)
{
	char *v6; // r15
	int v7; // er14
	u_short v8; // r13
	_BYTE *v9; // rax
	char *v10; // rbx
	signed __int64 v11; // rcx
	signed __int64 v13; // rsi
	char *v14; // rax
	signed __int64 v15; // r8
	int v16; // ecx
	int v17; // edx
	struct hostent *v18; // rax
	struct in_addr **v19; // rdi
	unsigned __int64 v20; // rax
	struct hostent *v21; // rax
	struct in_addr *v22; // rax
	signed __int64 v23; // rdi
	unsigned __int32 v24; // eax
	u_short v25; // ax
	__int16 v26; // ax
	__int64 v27; // r12
	__int64 v28; // rbx
	char *v29; // rdi
	_DWORD *v30; // rax
	char *v31; // rax
	int v32; // ebx
	int *v33; // rbx
	int *v34; // rax
	char *v35; // rax
	__int64 v36; // rcx
	SOCKET v37; // r12
	bool v38; // zf
	signed __int64 v39; // rax
	int v40; // eax
	int v41; // ebx
	int v42; // edi
	char *v43; // rax
	__int64 v44; // rcx
	int *v45; // rbx
	int *v46; // rax
	char *v47; // rax
	__int64 v48; // rcx
	u_short v49; // ax
	int *v50; // rax
	int *v51; // rax
	int *v52; // rax
	int v53; // ecx
	int *v54; // rbx
	int *v55; // rax
	char *v56; // rax
	int v57; // ecx
	__int64 v58; // rax
	int namelen[2]; // [rsp+18h] [rbp-41h]
	int optlen[2]; // [rsp+20h] [rbp-39h]
	char *pAddrBuf; // [rsp+28h] [rbp-31h]
	__int64 v62; // [rsp+30h] [rbp-29h]
	__int64 v63; // [rsp+38h] [rbp-21h]
	void *retaddr = NULL; // [rsp+B8h] [rbp+5Fh]
	int v65 = 0; // [rsp+C0h] [rbp+67h]
	char v66 = 0; // [rsp+C8h] [rbp+6Fh]
	char v67 = 0; // [rsp+D0h] [rbp+77h]
	char *Source = NULL; // [rsp+D8h] [rbp+7Fh]

	v6 = (char *)lpParameter;
	v7 = 0;
	v8 = a4;
	LOWORD(namelen[0]) = a4;
	LOWORD(optlen[0]) = a3;
	//v9 = (_BYTE *)((char *)lpParameter + 2821);
	v10 = Src;
	v11 = 0i64;
	//while (*v9)
	//{
	//	++v11;
	//	++v7;
	//	v9 += 664;
	//	if (v11 >= 96)
	//		return 0;
	//}
	//if (v7 == -1)
	//	return 0;
	//v13 = (signed __int64)&v6[664 * v7 + 2704];
	if (Src)
	{
		inet_pton(2, Src, (char *)&pAddrBuf + 4);
		v14 = inet_ntoa(*(struct in_addr *)((char *)&pAddrBuf + 4));
		//v15 = v10 - v14;
		//do
		//{
		//	v16 = (unsigned __int8)v14[v15];
		//	v17 = (unsigned __int8)*v14 - v16;
		//	if ((unsigned __int8)*v14 != v16)
		//		break;
		//	++v14;
		//} while (v16);
		//if (v17)
		{
			pAddrBuf = (char *)sub_7FFB89B12870(v10);
			//sub_7FFB89B125E0(
			//	(char *)0x64,
			//	(signed __int64)"Resolving %s to final IP...",
			//	(__int64)"SOCKETSET",
			//	"Resolving %s to final IP...",
			//	pAddrBuf);
			v18 = gethostbyname(v10);
			if (v18)
			{
				v19 = (struct in_addr **)v18->h_addr_list;
				if (*v19)
				{
					v10 = inet_ntoa(**v19);
					pAddrBuf = (char *)sub_7FFB89B12870(v10);
					//sub_7FFB89B125E0(
					//	(char *)0x64,
					//	(signed __int64)"Resolved to %s..",
					//	(__int64)"SOCKETSET",
					//	"Resolved to %s..",
					//	pAddrBuf);
					v20 = -1i64;
					do
						++v20;
					while (v10[v20]);
					if (v20 > 3 && *(_WORD *)v10 == 13873 && v10[2] == 57)
					{
						if (v19[1])
						{
							Sleep(0x3E8u);
							v21 = gethostbyname(v10);
							if (v21)
								v19 = (struct in_addr **)v21->h_addr_list;
						}
						v22 = v19[1];
						if (v22)
						{
							v10 = inet_ntoa(*v22);
							pAddrBuf = (char *)sub_7FFB89B12870(v10);
							//sub_7FFB89B125E0(
							//	(char *)0x64,
							//	(signed __int64)"Skipping 169 address, trying %s...",
							//	(__int64)"SOCKETSET",
							//	"Skipping 169 address, trying %s...",
							//	pAddrBuf);
						}
					}
				}
			}
		}
	}
	//v23 = v13 + 80;
	/**(_QWORD *)v23 = 0i64;
	*(_QWORD *)(v23 + 8) = 0i64;
	*(_WORD *)v23 = 2;
	if (v10)
		v24 = inet_addr(v10);
	else
		v24 = htonl(0);
	*(_DWORD *)(v13 + 84) = v24;
	*(_WORD *)(v13 + 82) = htons(v8);
	*(_QWORD *)(v13 + 64) = 0i64;
	*(_QWORD *)(v13 + 72) = 0i64;
	*(_WORD *)(v13 + 64) = 2;
	*(_DWORD *)(v13 + 68) = htonl(0);
	if (!(_DWORD)retaddr && v10)
		*(_DWORD *)(v13 + 68) = inet_addr(v10);
	v25 = htons(optlen[0]);
	*(_WORD *)(v13 + 112) = optlen[0];
	*(_WORD *)(v13 + 66) = v25;
	v26 = namelen[0];
	*(_WORD *)(v13 + 117) = 0;
	*(_WORD *)(v13 + 114) = v26;
	*(_BYTE *)(v13 + 116) = 0;
	*(_DWORD *)(v13 + 8) = v65;
	*(_DWORD *)(v13 + 60) = (_DWORD)retaddr;
	*(_DWORD *)(v13 + 128) = a5;
	*(_QWORD *)(v13 + 120) = 0i64;
	*(_BYTE *)(v13 + 12) = a6;
	*(_BYTE *)(v13 + 13) = 0;
	*(_BYTE *)(v13 + 161) = v66;
	*(_BYTE *)(v13 + 160) = 0;
	*(_DWORD *)(v13 + 320) = 1;
	*(_BYTE *)(v13 + 324) = v67;
	if (v67)
		InitializeCriticalSection((LPCRITICAL_SECTION)(v13 + 600));
	*(_BYTE *)(v13 + 325) = 0;
	if (Source)
		strncpy((char *)(v13 + 326), Source, 0x100ui64);
	else
		*(_BYTE *)(v13 + 326) = 0;
	*(_BYTE *)(v13 + 640) = 0;
	*(_QWORD *)(v13 + 656) = 0i64;
	*(_QWORD *)(v13 + 648) = 0i64;
	if (v6[66600])
	{
		*(_BYTE *)(v13 + 640) = 1;
		*(_QWORD *)(v13 + 656) = (_QWORD)(v6 + 66920);
		*(_QWORD *)(v13 + 648) = (_QWORD)(v6 + 66608);
	}
	*(_QWORD *)(v13 + 312) = 0i64;
	v27 = *((_QWORD *)v6 + 8324);
	if (v27)
	{
		do
		{
			v28 = *(_QWORD *)(v27 + 8);
			v29 = (char *)malloc(v28 + 1);
			strncpy(v29, *(const char **)v27, v28 + 1);
			*(_QWORD *)(v13 + 304) = (_QWORD)sub_7FFB898BFF80(*(_QWORD **)(v13 + 304), (__int64)v29, *(_QWORD *)(v27 + 8));
			v27 = *(_QWORD *)(v27 + 16);
		} while (v27);
		v23 = v13 + 80;
	}
	if (v66)
	{
		v30 = sub_7FFB898B2F40(0x400ui64);
		*(_WORD *)(v13 + 162) = 0;
		*(_BYTE *)(v13 + 164) = 0;
		*(_QWORD *)(v13 + 168) = (_QWORD)v30;
	}
	else
	{
		*(_QWORD *)(v13 + 168) = 0i64;
	}*/
	v31 = (char *)socket(2, (unsigned int)((unsigned int)((_DWORD)retaddr - 3) <= 3) + 1, 0);
	v32 = (signed int)v31;
	pAddrBuf = v31;
	if (!(_DWORD)v31)
	{
		//v33 = errno();
		//v34 = errno();
		//v35 = strerror(*v34);
		//LODWORD(v62) = *v33;
		//pAddrBuf = v35;
		//sub_7FFB89B125E0(
		//	(char *)0x1F4,
		//	(signed __int64)"Error creating socket: %s (%i)",
		//	(__int64)"SOCKETSET",
		//	"Error creating socket: %s (%i)",
		//	v35,
		//	v62);
		//v36 = *(_QWORD *)(v13 + 168);
		//if (v36)
		//{
		//	*(_QWORD *)(v13 + 168) = 0i64;
		//	sub_7FFB898B8710(v36);
		//	return 0;
		//}
		return 0;
	}
	v37 = (signed int)v31;
	//*(_QWORD *)v13 = (signed int)v31;
	v38 = v6[66452] == 0;
	optlen[1] = 1;
	if (v38 && setsockopt((signed int)v31, 0xFFFF, 4, (const char *)&optlen[1], 4))
	{
		pAddrBuf = 0i64;
		//sub_7FFB89B125E0(
		//	(char *)0x1F4,
		//	(signed __int64)"Failed to set SO_REUSEADDR on socket",
		//	(__int64)"SOCKETSET",
		//	"Failed to set SO_REUSEADDR on socket");
	}
	if (setsockopt(v37, 6, 1, (const char *)&optlen[1], 4))
	{
		pAddrBuf = 0i64;
		//sub_7FFB89B125E0(
		//	(char *)0x1F4,
		//	(signed __int64)"Failed to set TCP_NODELAY on socket",
		//	(__int64)"SOCKETSET",
		//	"Failed to set TCP_NODELAY on socket");
	}
	//switch ((_DWORD)retaddr)
	//{
	//case 2:
	//	goto LABEL_111;
	//case 4:
	//	goto LABEL_80;
	//case 6:
	//LABEL_111:
	//	if ((_DWORD)retaddr != 4)
	//	{
	//		if ((_DWORD)retaddr == 6)
	//		{
	//			LOBYTE(optlen[0]) = 3;
	//			pAddrBuf = 0i64;
	//			v62 = 0i64;
	//			LOWORD(pAddrBuf) = 2;
	//			WORD1(pAddrBuf) = htons(0);
	//			HIDWORD(pAddrBuf) = htonl(0);
	//			if (bind(v37, (const struct sockaddr *)&pAddrBuf, 16) < 0)
	//			{
	//				perror("Error binding multicast socket to interface");
	//				exit(0);
	//				__debugbreak();
	//			}
	//			setsockopt(v37, 0, 9, (const char *)namelen, 4);
	//			setsockopt(v37, 0, 10, (const char *)optlen, 1);
	//			setsockopt(v37, 0, 11, (const char *)&optlen[1], 1);
	//			//*(_BYTE *)(v13 + 116) = 1;
	//			goto LABEL_87;
	//		}
	//		v57 = v7;
	//		if (*((_DWORD *)v6 + 579) > v7)
	//			v57 = *((_DWORD *)v6 + 579);
	//		*((_DWORD *)v6 + 579) = v57;
	//		//*(_BYTE *)(v13 + 117) = 1;
	//		sub_7FFB89B74AD0(v6);
	//	LABEL_99:
	//		if ((_DWORD)retaddr == 5)
	//		{
	//			*(_QWORD *)namelen = 0i64;
	//			//v58 = *(unsigned int *)(v13 + 84);
	//			if (setsockopt(v37, 0, 12, (const char *)namelen, 8) < 0)
	//			{
	//				perror("Error joining the multicast group!");
	//				goto LABEL_91;
	//			}
	//		}
	//	LABEL_87:
	//		//*(_BYTE *)(v13 + 117) = 1;
	//		if (((_DWORD)retaddr - 3) & 0xFFFFFFFD)
	//		{
	//			if ((unsigned int)retaddr <= 1)
	//			{
	//				if (listen(v37, 32) < 0)
	//				{
	//					//v54 = errno();
	//					//v55 = errno();
	//					//v56 = strerror(*v55);
	//					//LODWORD(v62) = *v54;
	//					//pAddrBuf = v56;
	//					//sub_7FFB89B125E0(
	//					//	(char *)0x1F4,
	//					//	(signed __int64)"Error listening socket: %s (%i)",
	//					//	(__int64)"SOCKETSET",
	//					//	"Error listening socket: %s (%i)");
	//				LABEL_91:
	//					closesocket(v37);
	//					EnterCriticalSection((LPCRITICAL_SECTION)(v6 + 1688));
	//					v44 = *(_QWORD *)(v13 + 168);
	//					if (v44)
	//					{
	//						*(_QWORD *)(v13 + 168) = 0i64;
	//						goto LABEL_93;
	//					}
	//				LABEL_94:
	//					LeaveCriticalSection((LPCRITICAL_SECTION)(v6 + 1688));
	//					return 0;
	//				}
	//				if (*((_DWORD *)v6 + 579) > v7)
	//					v7 = *((_DWORD *)v6 + 579);
	//				*((_DWORD *)v6 + 579) = v7;
	//				sub_7FFB89B73E70(v6, v32);
	//			}
	//		}
	//		else
	//		{
	//			if (*((_DWORD *)v6 + 579) > v7)
	//				v7 = *((_DWORD *)v6 + 579);
	//			*((_DWORD *)v6 + 579) = v7;
	//			sub_7FFB89B74030(v6, v32);
	//		}
	//		return 1;
	//	}
	//LABEL_80:
	//	connect(v37, (const struct sockaddr *)v23, 16);
	//	v53 = v7;
	//	if (*((_DWORD *)v6 + 579) > v7)
	//		v53 = *((_DWORD *)v6 + 579);
	//	namelen[0] = 8;
	//	*((_DWORD *)v6 + 579) = v53;
	//	SendARP(*(_DWORD *)(v13 + 84), 0, &pAddrBuf, (PULONG)namelen);
	//	*(_BYTE *)(v13 + 116) = 1;
	//	goto LABEL_99;
	//}
	//while (bind(v37, (const struct sockaddr *)(v13 + 64), 16))
	//{
	//	if (!v6[66452] || !*(_WORD *)(v13 + 112) || WSAGetLastError() != 10048)
	//	{
	//		v40 = WSAGetLastError();
	//		v41 = *(unsigned __int16 *)(v13 + 112);
	//		v42 = v40;
	//		v43 = strerror(v40);
	//		LODWORD(v63) = v41;
	//		LODWORD(v62) = v42;
	//		pAddrBuf = v43;
	//		//sub_7FFB89B125E0(
	//		//	(char *)0x1F4,
	//		//	(signed __int64)"Error binding socket: %s (%i) port: %i",
	//		//	(__int64)"SOCKETSET",
	//		//	"Error binding socket: %s (%i) port: %i",
	//		//	v43,
	//		//	v62,
	//		//	v63);
	//		closesocket(v37);
	//		EnterCriticalSection((LPCRITICAL_SECTION)(v6 + 1688));
	//		v44 = *(_QWORD *)(v13 + 168);
	//		if (!v44)
	//			goto LABEL_94;
	//		*(_QWORD *)(v13 + 168) = 0i64;
	//	LABEL_93:
	//		sub_7FFB898B8710(v44);
	//		goto LABEL_94;
	//	}
	//	//*(_WORD *)(v13 + 66) = htons(++*(_WORD *)(v13 + 112));
	//	//v39 = *(unsigned __int16 *)(v13 + 112);
	//	//LODWORD(pAddrBuf) = *(unsigned __int16 *)(v13 + 112);
	//	//sub_7FFB89B125E0((char *)0x64, v39, (__int64)"SOCKETSET", "Port in use, incrementing to %i");
	//}
	//if (*(_WORD *)(v13 + 112))
	//{
	//LABEL_67:
	//	if ((_DWORD)retaddr == 3 && a5 & 2)
	//	{
	//		//*(_QWORD *)optlen = optlen;//?????
	//		optlen[0] = 4;
	//		if (getsockopt(v37, 0xFFFF, 4098, (char *)namelen, *(int **)optlen) == -1)
	//		{
	//			//v50 = errno();
	//			//pAddrBuf = strerror(*v50);
	//			//sub_7FFB89B125E0(
	//			//	(char *)0x1F4,
	//			//	(signed __int64)pAddrBuf,
	//			//	(__int64)"SOCKETSET",
	//			//	"Error getting SO_RCVBUF: %s",
	//			//	pAddrBuf);
	//		}
	//		LODWORD(pAddrBuf) = namelen[0];
	//		//sub_7FFB89B125E0(
	//		//	(char *)0x12C,
	//		//	(signed __int64)"Initial SO_RCVBUF was %i",
	//		//	(__int64)"SOCKETSET",
	//		//	"Initial SO_RCVBUF was %i",
	//		//	pAddrBuf);
	//		if (namelen[0] < 0x40000)
	//		{
	//			if (setsockopt(v37, 0xFFFF, 4098, (const char *)namelen, 4) == -1)
	//			{
	//				//v51 = errno();
	//				//pAddrBuf = strerror(*v51);
	//				//sub_7FFB89B125E0(
	//				//	(char *)0x1F4,
	//				//	(signed __int64)"Error setting SO_RCVBUF: %s",
	//				//	(__int64)"SOCKETSET",
	//				//	"Error setting SO_RCVBUF: %s",
	//				//	pAddrBuf);
	//			}
	//			if (getsockopt(v37, 0xFFFF, 4098, (char *)namelen, optlen) == -1)
	//			{
	//				//v52 = errno();
	//				//pAddrBuf = strerror(*v52);
	//				//sub_7FFB89B125E0(
	//				//	(char *)0x1F4,
	//				//	(signed __int64)pAddrBuf,
	//				//	(__int64)"SOCKETSET",
	//				//	"Error getting SO_RCVBUF: %s",
	//				//	pAddrBuf);
	//			}
	//			LODWORD(v62) = 0x40000;
	//			LODWORD(pAddrBuf) = namelen[0];
	//			//sub_7FFB89B125E0(
	//			//	(char *)0x12C,
	//			//	(signed __int64)"New SO_RCVBUF was %i (Attempted %i)",
	//			//	(__int64)"SOCKETSET",
	//			//	"New SO_RCVBUF was %i (Attempted %i)");
	//			v32 = (signed int)pAddrBuf;
	//			*(_BYTE *)(v13 + 116) = 1;
	//			goto LABEL_99;
	//		}
	//		pAddrBuf = 0i64;
	//		//sub_7FFB89B125E0(
	//		//	(char *)0x12C,
	//		//	(signed __int64)"We are happy with that SO_RCVBUF, no changes made",
	//		//	(__int64)"SOCKETSET",
	//		//	"We are happy with that SO_RCVBUF, no changes made");
	//	}
	//	v32 = (signed int)pAddrBuf;
	//	*(_BYTE *)(v13 + 116) = 1;
	//	goto LABEL_99;
	//}
	//if (getsockname(v37, (struct sockaddr *)(v13 + 64), namelen) >= 0)
	//{
	//	v49 = ntohs(*(_WORD *)(v13 + 66));
	//	LODWORD(pAddrBuf) = v49;
	//	*(_WORD *)(v13 + 112) = v49;
	//	//sub_7FFB89B125E0(
	//	//	(char *)0x64,
	//	//	(signed __int64)"Bound to open port %i",
	//	//	(__int64)"SOCKETSET",
	//	//	"Bound to open port %i");
	//	goto LABEL_67;
	//}
	//v45 = errno();
	//v46 = errno();
	//v47 = strerror(*v46);
	//LODWORD(v62) = *v45;
	//pAddrBuf = v47;
	//sub_7FFB89B125E0(
	//	(char *)0x1F4,
	//	(signed __int64)"Error getting name socket: %s (%i)",
	//	(__int64)"SOCKETSET",
	//	"Error getting name socket: %s (%i)",
	//	v47,
	//	v62);
	//EnterCriticalSection((LPCRITICAL_SECTION)(v6 + 1688));
	//v48 = *(_QWORD *)(v13 + 168);
	//if (v48)
	//{
	//	*(_QWORD *)(v13 + 168) = 0i64;
	//	sub_7FFB898B8710(v48);
	//}
	//LeaveCriticalSection((LPCRITICAL_SECTION)(v6 + 1688));
	closesocket(v37);
	return 0;
}

char sub_7FFB89B70970(void *a1, char *a2, int a3, int a4, int a5, char a6)
{
	return sub_7FFB89B73210(a1, a2, a3, a4, a5, a6);
}

int StartMiracastServer(const WXCastStruct stMiracast)
{
	if (netutils_init() < 0) 
	{
		return -1;
	}

	if (stMiracast.m_callBackStart == NULL) 
	{
		return -2;
	}

	if (stMiracast.m_callBackStop == NULL)
	{
		return -3;
	}

	if (stMiracast.m_callBackVideo == NULL)
	{
		return -4;
	}

	memcpy(&g_stMiracast, &stMiracast, sizeof(WXCastStruct));

	struct _RTL_CRITICAL_SECTION CriticalSection;

	CRITICAL_SECTION critical_sec;
	InitializeCriticalSection((LPCRITICAL_SECTION)&critical_sec);

	LPCRITICAL_SECTION lpcritical_sec = &critical_sec;

	int v15 = 0;
	__int64 v16 = 0;
	void* v0 = (void*)malloc(0x38ui64);
	memset((void *)v0, 0, 0x38);
	__int64 v1 = (__int64)v0;
	*(_QWORD *)v0 = 0i64;//0
	*(_QWORD *)((__int64)v0 + 8) = (_QWORD)lpcritical_sec;

	*(_QWORD *)((__int64)v0 + 16) = (__int64)sub_7FFC056AD240;//fun1
	*(_QWORD *)((__int64)v0 + 24) = (__int64)sub_7FFB87BDD2F0;//fun2
	*(_QWORD *)((__int64)v0 + 32) = (__int64)sub_7FFFC6D5D350;//fun3

	*(_QWORD *)((__int64)v0 + 40) = 0i64;//use for wifidisplay function list
	*(_QWORD *)((__int64)v0 + 48) = 0i64;//0

	void *v20 = malloc(0x48ui64);
	*(_QWORD *)v20 = 0i64;
	*((_QWORD *)v20 + 1) = 0i64;
	*((_QWORD *)v20 + 2) = 0i64;
	*((_QWORD *)v20 + 3) = 0i64;
	*((_QWORD *)v20 + 4) = 0i64;
	*((_QWORD *)v20 + 5) = 0i64;
	*((_QWORD *)v20 + 6) = 0i64;
	*((_QWORD *)v20 + 7) = 0i64;
	*((_QWORD *)v20 + 8) = 0i64;

	WCHAR* v3 = (WCHAR*)malloc(0x1FFui64);
	GetSystemDirectoryW(v3, 0xFFu);

	CLSID clsid;
	HRESULT res = CLSIDFromString(L"{07997038-5CA8-4D98-A62A-BC3C789D5024}", &clsid);

	HMODULE handle = ::LoadLibraryA("WiFiDisplay.dll");
	MyWFDDisplaySinkInit sinkinit = (MyWFDDisplaySinkInit)GetProcAddress(handle, "WFDDisplaySinkInit");
	MyIsMiracastSupportedByWlan issupportwifi = (MyIsMiracastSupportedByWlan)GetProcAddress(handle, "IsMiracastSupportedByWlan");
	MyWFDStartDisplaySink startsink = (MyWFDStartDisplaySink)GetProcAddress(handle, "WFDDisplaySinkStartEx");
	MyWFDStopDisplaySink stopsink = (MyWFDStopDisplaySink)GetProcAddress(handle, "WFDDisplaySinkStop");
	MyWFDDisplaySinkDeInit sinkdeinit = (MyWFDDisplaySinkDeInit)GetProcAddress(handle, "WFDDisplaySinkDeInit");
	//MyWFDDisplaySinkSetPersistedGroupIDList setgpidlist = (MyWFDDisplaySinkSetPersistedGroupIDList)GetProcAddress(handle, "WFDDisplaySinkSetPersistedGroupIDList");
	//sinkinit = (PWFDDisplaySinkInit)GetProcAddress(handle, "WFDDisplaySinkQueryCapabilities");
	MyWFDDisplaySinkCloseSession closesess = (MyWFDDisplaySinkCloseSession)GetProcAddress(handle, "WFDDisplaySinkCloseSession");

	printf("handle:%x, sinkinit:%x, issupportwifi:%x, startsink:%x, stopsink:%x, sinkdeinit:%x\n", 
		handle, sinkinit, issupportwifi, startsink, stopsink, sinkdeinit);

	*((_BYTE *)v20 + 66) = 0;
	*(_QWORD *)v20 = 0i64;
	*(_QWORD *)v20 = *(_QWORD*)&handle;

	*((_QWORD *)v20 + 1) = *(_QWORD*)&stopsink;
	*((_QWORD *)v20 + 2) = *(_QWORD*)&sinkinit;
	*((_QWORD *)v20 + 3) = *(_QWORD*)&sinkdeinit;
	*((_QWORD *)v20 + 4) = *(_QWORD*)&startsink;

	*(_QWORD *)(v1 + 40) = *(_QWORD*)&v20;//wfddisplay export function list

	bool bSup = false;
	bSup = issupportwifi();
	if (!bSup) 
	{
		printf("unsupport wifi mode\n");
		return -1;
	}

	if (sinkinit) 
	{
		//1 sink init
		printf("sinkinit begin\n");
		int init = sinkinit((__int64)v0, sub_1803DF1F0, &v15, &v16);


		if (init != 0)
		{
			return -1;
		}
		int Flags = *(_DWORD *)(v16 + 8);
		int Period = *(_DWORD *)(v16 + 4);
		if (!Period)
		{
			//error
		}
	}

	_BYTE* v9 = (_BYTE *)calloc(0x60ui64, 0x60ui64);

	*(_BYTE *)v9 = 1;
	*(_BYTE *)(v9 + 1) = 1;
	*(_BYTE *)(v9 + 2i64) = 96;
	*(_BYTE *)(v9 + 3i64) = 0;

	*(_BYTE *)(v9 + 4i64) = 2;
	*(_BYTE *)(v9 + 5i64) = 0;
	*(_BYTE *)(v9 + 6i64) = 0;
	*(_BYTE *)(v9 + 7i64) = 0;

	*(_BYTE *)(v9 + 8i64) = 15;
	*(_BYTE *)(v9 + 9i64) = 0;
	*(_BYTE *)(v9 + 10i64) = 4;
	*(_BYTE *)(v9 + 11i64) = 0;

	*(_BYTE *)(v9 + 12i64) = 0;
	*(_BYTE *)(v9 + 13i64) = 0;
	*(_BYTE *)(v9 + 14i64) = 0;
	*(_BYTE *)(v9 + 15i64) = 0;

	*(_BYTE *)(v9 + 16i64) = 7;
	*(_BYTE *)(v9 + 17i64) = 0;
	*(_BYTE *)(v9 + 18i64) = 0;
	*(_BYTE *)(v9 + 19i64) = 0;

	_QWORD tmp = *(_QWORD*)&v9;
	_QWORD tmptype = 96;

	__int64 v2 = 0;
	int param1[2] = { 0 };
	int param2[2] = { 0 };

	//2 sink start
	printf("wfdstartex begin\n");

	std::wstring strName = L"wxmiracast";

	//stopsink();

	if (g_stMiracast.m_pszName != NULL)
	{
		strName = g_stMiracast.m_pszName;
		char v17[0xC6 + 4 + 1] = { 0 };
		int tmpv = 0xF00101;
		memcpy(v17, &tmpv, 4);

		char newV10[0xC6 + 4 + 1] = { 0 };
		memcpy(newV10, &tmpv, 4);
		wcscpy((wchar_t*)(newV10 + 4), strName.c_str());
		////////////////////

		printf("wfdstartex begin 2\n");
		v2 = startsink(*(_BYTE *)(newV10 + 2i64), (int*)newV10, tmptype, (int*)v9, (int*)v17, (__int64*)&param2[1], &param2[0]);
		printf("wfdstartex begin done\n");
		bool bSuccess = false;
		if (v2 == 2023 || v2 == 1206)
		{
			printf("wfdstartex error\n");
			//v2 = startsink(*(_BYTE *)(v10 + 2i64), tmp2, tmptype, tmp, param1, &param2[1], &param2[0]);
			v2 = startsink(*(_BYTE *)(newV10 + 2i64), (int*)newV10, tmptype, (int*)v9, (int*)v17, (__int64*)&param2[1], &param2[0]);
		}
		if (v2 == 13)
		{
			*(_BYTE *)(newV10 + 2i64) = 0xE8;
			v2 = startsink(*(_BYTE *)(newV10 + 2i64), (int*)newV10, tmptype, (int*)v9, (int*)v17, (__int64*)&param2[1], &param2[0]);
			if (v2 == 2023 || v2 == 1206)
			{
				printf("wfdstartex error\n");
				v2 = startsink(*(_BYTE *)(newV10 + 2i64), (int*)newV10, tmptype, (int*)v9, (int*)v17, (__int64*)&param2[1], &param2[0]);
			}
		}
		if (v2 == 0)
		{
			printf("wfdstartex success\n");
			bSuccess = true;
		}
		free(v9);
		v9 = NULL;

		if (bSuccess)
		{
			//3 create sockets for send thread self-pipe
			char result = 0;
			sub_7FFB89B70D40((__int64 *)(v1 + 48));
			sub_7FFB89B71E40(*(_QWORD **)(v1 + 48), 0i64, 0i64, (__int64)sub_7FFB89B13500, (__int64)sub_7FFB89B13530, 0i64, v1);
			result = sub_7FFB89B70970(*(void **)(v1 + 48), 0i64, 7250, 0, 0, 16);
		}
		if (v2 == 13)
		{
			v9 = (_BYTE *)calloc(0x54ui64, 0x54ui64);
			*(_BYTE *)v9 = 1;
			*(_BYTE *)(v9 + 1i64) = 1;
			*(_BYTE *)(v9 + 2i64) = 84;
			*(_BYTE *)(v9 + 3i64) = 0;

			*(_BYTE *)(v9 + 4i64) = 2;
			*(_BYTE *)(v9 + 5i64) = 0;
			*(_BYTE *)(v9 + 6i64) = 0;
			*(_BYTE *)(v9 + 7i64) = 0;

			*(_BYTE *)(v9 + 8i64) = 15;
			*(_BYTE *)(v9 + 9i64) = 0;
			*(_BYTE *)(v9 + 10i64) = 4;
			*(_BYTE *)(v9 + 11i64) = 0;

			*(_BYTE *)(v9 + 12i64) = 7;
			*(_BYTE *)(v9 + 13i64) = 0;
			*(_BYTE *)(v9 + 14i64) = 0;
			*(_BYTE *)(v9 + 15i64) = 0;

			*(_BYTE *)(v9 + 16i64) = 0;
			*(_BYTE *)(v9 + 17i64) = 0;
			*(_BYTE *)(v9 + 18i64) = 0;
			*(_BYTE *)(v9 + 19i64) = 0;

			tmp = *(_QWORD*)&v9;
			tmptype = 84;

			printf("startsink start\n");
			int param1[2] = { 0 };
			int param2[2] = { 0 };

			v2 = startsink(*(_BYTE *)(newV10 + 2i64), (int*)newV10, tmptype, (int*)v9, (int*)v17, (__int64*)&param2[1], &param2[0]);
			if (v2)
			{
				if (v2 == 13)
				{
					//error
					printf("wfdstartex error again\n");
				}
			}
			else
			{
				printf("wfdstartex success\n");
				//3 create sockets for send thread self-pipe
				char result = 0;
				sub_7FFB89B70D40((__int64 *)(v1 + 48));
				sub_7FFB89B71E40(*(_QWORD **)(v1 + 48), 0i64, 0i64, (__int64)sub_7FFB89B13500, (__int64)sub_7FFB89B13530, 0i64, v1);
				result = sub_7FFB89B70970(*(void **)(v1 + 48), 0i64, 7250, 0, 0, 16);
			}
		}
	}
	else 
	{
		char v17[0xC6 + 4 + 1] = { 0 };
		int tmpv = 0xF00101;
		memcpy(v17, &tmpv, 4);

		char newV10[0xC6 + 4 + 1] = { 0 };
		memcpy(newV10, &tmpv, 4);
		wcscpy((wchar_t*)(newV10 + 4), strName.c_str());

		printf("wfdstartex begin 2\n");
		v2 = startsink(0, 0, tmptype, (int*)v9, (int*)v17, (__int64*)&param2[1], &param2[0]);
		printf("wfdstartex begin done\n");
		bool bSuccess = false;
		if (v2 == 0)
		{
			printf("wfdstartex success\n");
			bSuccess = true;
		}
		free(v9);
		v9 = NULL;

		if (bSuccess)
		{
			//3 create sockets for send thread self-pipe
			char result = 0;
			sub_7FFB89B70D40((__int64 *)(v1 + 48));
			sub_7FFB89B71E40(*(_QWORD **)(v1 + 48), 0i64, 0i64, (__int64)sub_7FFB89B13500, (__int64)sub_7FFB89B13530, 0i64, v1);
			result = sub_7FFB89B70970(*(void **)(v1 + 48), 0i64, 7250, 0, 0, 16);
		}
		if (v2 == 13)
		{
			v9 = (_BYTE *)calloc(0x54ui64, 0x54ui64);
			*(_BYTE *)v9 = 1;
			*(_BYTE *)(v9 + 1i64) = 1;
			*(_BYTE *)(v9 + 2i64) = 84;
			*(_BYTE *)(v9 + 3i64) = 0;
			*(_BYTE *)(v9 + 4i64) = 1;
			*(_BYTE *)(v9 + 5i64) = 0;
			*(_BYTE *)(v9 + 6i64) = 1;
			*(_BYTE *)(v9 + 7i64) = 0;
			*(_BYTE *)(v9 + 8i64) = 7;
			*(_BYTE *)(v9 + 9i64) = 0;
			*(_BYTE *)(v9 + 10i64) = 4;
			*(_BYTE *)(v9 + 11i64) = 0;

			tmp = *(_QWORD*)&v9;
			tmptype = 84;

			printf("startsink start\n");
			int param1[2] = { 0 };
			int param2[2] = { 0 };

			v2 = startsink(0, 0, tmptype, (int*)v9, (int*)v17, (__int64*)&param2[1], &param2[0]);
			if (v2)
			{
				if (v2 == 13)
				{
					//error
					printf("wfdstartex error again\n");
				}
			}
			else
			{
				printf("wfdstartex success\n");
				//3 create sockets for send thread self-pipe
				char result = 0;
				sub_7FFB89B70D40((__int64 *)(v1 + 48));
				sub_7FFB89B71E40(*(_QWORD **)(v1 + 48), 0i64, 0i64, (__int64)sub_7FFB89B13500, (__int64)sub_7FFB89B13530, 0i64, v1);
				result = sub_7FFB89B70970(*(void **)(v1 + 48), 0i64, 7250, 0, 0, 16);
			}
		}
	}


    return 0;
}

///////////////////////////////////////////////////////////////
int WXInitMiracast(const WXCastStruct stMiracast) 
{
	return StartMiracastServer(stMiracast);
}

void WXUninitMiracast() 
{

}

void WXSetLogLevel(int iLevel) 
{

}

void WXStop(unsigned long long llUniqueid) 
{

}
void WXStopMiraCast()
{

}

