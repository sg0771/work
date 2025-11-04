/**
 *  Copyright (C) 2011-2012  Juho Vähä-Herttua
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 */

/* These defines allow us to compile on iOS */
#ifndef __has_feature
# define __has_feature(x) 0
#endif
#ifndef __has_extension
# define __has_extension __has_feature
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "../mdns/dns_sd.h"

#ifndef WIN32
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef uint32_t in_addr_t;
//extern uint16_t htons(uint16_t __hostshort);
#endif

#include "dnssd.h"

#define RAOP_TXTVERS "1"
#define RAOP_CH "2"             /* Audio channels: 2 */
//#define RAOP_CN "0,1"           /* Audio codec: PCM, ALAC */
#define RAOP_CN "0,1,2,3"           /* Audio codec: PCM, ALAC */
//#define RAOP_CN "0,1,3"           /* Audio codec: PCM, ALAC */
//#define RAOP_ET "0,1"           /* Encryption type: none, RSA */
#define RAOP_ET "0,3,5"           /* Encryption type: none, fairplay */
#define RAOP_SV "false"
#define RAOP_DA "true"
#define RAOP_SR "44100"
#define RAOP_SS "16"            /* Sample size: 16 */
#define RAOP_VN "3"
//#define RAOP_VN "65537"
//#define RAOP_TP "TCP,UDP"
#define RAOP_TP "UDP"
#define RAOP_MD "0,1,2"         /* Metadata: text, artwork, progress */
#define RAOP_SM "false"
#define RAOP_EK "1"
#define RAOP_SF "0x4"
//#define RAOP_VS "150.33"
//#define RAOP_AM "AppleTV3,1"
#define RAOP_VS "130.14"
#define RAOP_AM "Shairport,1"

#define GLOBAL_FEATURES 0x7
#define GLOBAL_FEATURES_AIRPLAY 0x29ff

#define MAX_HWADDR_LEN 6

#if defined(_WIN32)
#include <ws2tcpip.h>
#include <windows.h>
#ifndef snprintf
#define snprintf _snprintf
#endif
#else
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#endif




#if defined(_WIN32)

#define SYSTEM_GET_PAGESIZE(ret) do {\
	SYSTEM_INFO si;\
	GetSystemInfo(&si);\
	ret = si.dwPageSize;\
} while(0)
#define SYSTEM_GET_TIME(ret) ret = timeGetTime()

#define ALIGNED_MALLOC(memptr, alignment, size) do {\
	char *ptr = malloc(sizeof(void*) + (size) + (alignment)-1);\
	memptr = NULL;\
	if (ptr) {\
		size_t ptrval = (size_t)ptr + sizeof(void*) + (alignment)-1;\
		ptrval = ptrval / (alignment) * (alignment);\
		memptr = (void *)ptrval;\
		*(((void **)memptr)-1) = ptr;\
	}\
} while(0)
#define ALIGNED_FREE(memptr) free(*(((void **)memptr)-1))

#else

#define SYSTEM_GET_PAGESIZE(ret) ret = sysconf(_SC_PAGESIZE)
#define SYSTEM_GET_TIME(ret) do {\
	struct timeval tv;\
	gettimeofday(&tv, NULL);\
	ret = (unsigned int)(tv.tv_sec*1000 + tv.tv_usec/1000);\
} while(0)

#define ALIGNED_MALLOC(memptr, alignment, size) if (posix_memalign((void **)&memptr, alignment, size)) memptr = NULL
#define ALIGNED_FREE(memptr) free(memptr)

#endif



#if defined(_WIN32)
typedef int socklen_t;

#ifndef SHUT_RD
#  define SHUT_RD SD_RECEIVE
#endif
#ifndef SHUT_WR
#  define SHUT_WR SD_SEND
#endif
#ifndef SHUT_RDWR
#  define SHUT_RDWR SD_BOTH
#endif

#define SOCKET_GET_ERROR()      WSAGetLastError()
#define SOCKET_SET_ERROR(value) WSASetLastError(value)
#define SOCKET_ERRORNAME(name)  WSA##name

#define WSAEAGAIN WSAEWOULDBLOCK
#define WSAENOMEM WSA_NOT_ENOUGH_MEMORY

#else

#define closesocket close
#define ioctlsocket ioctl

#define SOCKET_GET_ERROR()      (errno)
#define SOCKET_SET_ERROR(value) (errno = (value))
#define SOCKET_ERRORNAME(name)  name

#endif



char* utils_strsep(char** stringp, const char* delim);
int utils_hwaddr_raop(char* str, int strlen, const char* hwaddr, int hwaddrlen);
int utils_hwaddr_airplay(char* str, int strlen, const char* hwaddr, int hwaddrlen);

#define MAX_DEVICEID 18
#define MAX_SERVNAME 256

#include "../mdns/dns_sd.h"
# define DNSSD_STDCALL

struct dnssd_s {
	DNSServiceRef raopService;
	TXTRecordRef raoptxtRecord;

	DNSServiceRef airplayService;
	TXTRecordRef airplaytxtRecord;
};

dnssd_t *dnssd_init(int *error)
{
	dnssd_t *dnssd;

	if (error) *error = DNSSD_ERROR_NOERROR;

	dnssd = (dnssd_t *)calloc(1, sizeof(dnssd_t));
	if (!dnssd) {
		if (error) *error = DNSSD_ERROR_OUTOFMEM;
		return NULL;
	}
	return dnssd;
}

DNSSD_API void dnssd_update(dnssd_t* dnssd, bool boddnum) 
{
	if (!dnssd || !dnssd->raopService || !dnssd->airplayService) {
		return;
	}

	if (boddnum) 
	{
		TXTRecordSetValue(&dnssd->airplaytxtRecord, "xbmcdummy", strlen("evendummy"), "evendummy");
		TXTRecordSetValue(&dnssd->raoptxtRecord, "xbmcdummy", strlen("evendummy"), "evendummy");
	}
	else 
	{
		TXTRecordSetValue(&dnssd->airplaytxtRecord, "xbmcdummy", strlen("odddummy"), "odddummy");
		TXTRecordSetValue(&dnssd->raoptxtRecord, "xbmcdummy", strlen("odddummy"), "odddummy");
	}
	DNSServiceUpdateRecord(dnssd->airplayService,
		NULL, 0,
		TXTRecordGetLength(&dnssd->airplaytxtRecord),
		TXTRecordGetBytesPtr(&dnssd->airplaytxtRecord), 0);
	DNSServiceUpdateRecord(dnssd->raopService,
		NULL, 0,
		TXTRecordGetLength(&dnssd->raoptxtRecord),
		TXTRecordGetBytesPtr(&dnssd->raoptxtRecord), 0);
}

DNSSD_API int dnssd_register(dnssd_t * dnssd, const char * servname, const char * tcpname, unsigned short port, std::vector<std::pair<std::string, std::string> > txt)
{
	//TXTRecordRef txtRecord;
	int ret;

	assert(dnssd);
	assert(servname);

	/* Register the service */

	if (strcmp(tcpname, "_raop._tcp") == 0)
	{
		TXTRecordCreate(&dnssd->raoptxtRecord, 0, NULL);
		for (size_t i = 0; i < txt.size(); i++)
		{
			std::pair<std::string, std::string> t = txt[i];
			TXTRecordSetValue(&dnssd->raoptxtRecord, t.first.c_str(), t.second.length(), t.second.c_str());
		}

		DNSServiceErrorType errortype = DNSServiceRegister(&dnssd->raopService, 0, 0,
			servname, tcpname,
			NULL, NULL,
#ifdef _WIN32
			htons(port),
#else
			htons2(port),
#endif
			TXTRecordGetLength(&dnssd->raoptxtRecord),
			TXTRecordGetBytesPtr(&dnssd->raoptxtRecord),
			NULL, NULL);
		printf("errortype : %d\n", errortype);
	}
	else 
	{
		TXTRecordCreate(&dnssd->airplaytxtRecord, 0, NULL);
		for (size_t i = 0; i < txt.size(); i++)
		{
			std::pair<std::string, std::string> t = txt[i];
			TXTRecordSetValue(&dnssd->airplaytxtRecord, t.first.c_str(), t.second.length(), t.second.c_str());
		}

		DNSServiceErrorType errortype = DNSServiceRegister(&dnssd->airplayService, 0, 0,
			servname, tcpname,
			NULL, NULL,
#ifdef _WIN32
			htons(port),
#else
			htons2(port),
#endif
			TXTRecordGetLength(&dnssd->airplaytxtRecord),
			TXTRecordGetBytesPtr(&dnssd->airplaytxtRecord),
			NULL, NULL);
		printf("errortype : %d\n", errortype);
	}

	/* Deallocate TXT record */
	//dnssd->TXTRecordDeallocate(&txtRecord);
	return 1;
}

DNSSD_API void dnssd_destroy(dnssd_t *dnssd){
	if (dnssd) {
		free(dnssd);
	}
}

DNSSD_API void dnssd_unregister(dnssd_t *dnssd)
{
	assert(dnssd);

	if (dnssd->airplayService) 
	{
		DNSServiceRefDeallocate(dnssd->airplayService);
		TXTRecordDeallocate(&dnssd->airplaytxtRecord);
		dnssd->airplayService = NULL;
	}

	if (dnssd->raopService)
	{
		DNSServiceRefDeallocate(dnssd->raopService);
		TXTRecordDeallocate(&dnssd->raoptxtRecord);
		dnssd->raopService = NULL;
	}
}
