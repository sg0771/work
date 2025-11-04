#pragma once

#include <Windows.h>
#include <stdint.h>
#include <process.h>
#include <inttypes.h>


#define HAS_FILESYSTEM
#define HAVE_LIBPLIST
#define HAS_AIRPLAY
#define HAVE_LIBSHAIRPLAY
#define HAS_AIRTUNES

#if defined(_WIN32)
#define HAS_WIN32_NETWORK
#define HAS_IRSERVERSUITE
#define HAS_AUDIO
#define HAS_WEB_SERVER
#define HAS_WEB_INTERFACE
#define HAS_LIBRTMP
#define HAVE_LIBBLURAY
#define HAS_ASAP_CODEC
#define HAS_FILESYSTEM_SMB
#define HAS_FILESYSTEM_NFS
#define HAS_ZEROCONF
#define HAS_MDNS
#define HAS_AIRPLAY
#define HAS_AIRTUNES
#define HAVE_LIBSHAIRPLAY
#define HAVE_LIBCEC
#define HAVE_LIBMP3LAME
#define HAVE_LIBVORBISENC
#define DECLARE_UNUSED(a,b) a b;
#endif

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)       do { delete (p);     (p)=NULL; } while (0)
#endif

#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) do { delete[] (p);   (p)=NULL; } while (0)
#endif

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      do { if(p) { (p)->Release(); (p)=NULL; } } while (0)
#endif


#define sleepms(x) Sleep(x)

typedef HANDLE thread_handle_t;

#define THREAD_RETVAL DWORD WINAPI
#define THREAD_CREATE(handle, func, arg) \
	handle = CreateThread(NULL, 0, func, arg, 0, NULL)
#define THREAD_JOIN(handle) do { WaitForSingleObject(handle, INFINITE); CloseHandle(handle); } while(0)

#define sleepms(x) Sleep(x)

typedef CRITICAL_SECTION pthread_mutex_t;
typedef CONDITION_VARIABLE pthread_cond_t;

typedef pthread_mutex_t mutex_handle_t;
typedef pthread_cond_t cond_handle_t;

#define MUTEX_CREATE(handle) pthread_mutex_init(&(handle), NULL)
#define MUTEX_LOCK(handle) pthread_mutex_lock(&(handle))
#define MUTEX_UNLOCK(handle) pthread_mutex_unlock(&(handle))
#define MUTEX_DESTROY(handle) pthread_mutex_destroy(&(handle))

#define COND_CREATE(handle) pthread_cond_init_new(&(handle), NULL)
#define COND_WAIT(handle, mutex) pthread_cond_wait_new(&(handle), &(mutex))
#define COND_SIGNAL(handle) pthread_cond_signal_new(&(handle))
#define COND_DESTROY(handle) pthread_cond_destroy_new(&(handle))

static __inline int pthread_mutex_init(pthread_mutex_t * m, void* attr)
{
	InitializeCriticalSection(m);
	return 0;
}
static __inline int pthread_mutex_destroy(pthread_mutex_t * m)
{
	DeleteCriticalSection(m);
	return 0;
}
static __inline int pthread_mutex_lock(pthread_mutex_t * m)
{
	EnterCriticalSection(m);
	return 0;
}
static __inline int pthread_mutex_unlock(pthread_mutex_t * m)
{
	LeaveCriticalSection(m);
	return 0;
}

#if _WIN32_WINNT >= 0x0600
static __inline int pthread_cond_init_new(pthread_cond_t * cond, const void* unused_attr)
{
	InitializeConditionVariable(cond);
	return 0;
}

/* native condition variables do not destroy */
static __inline void pthread_cond_destroy_new(pthread_cond_t * cond)
{
	return;
}

static __inline void pthread_cond_broadcast(pthread_cond_t * cond)
{
	WakeAllConditionVariable(cond);
}

static __inline int pthread_cond_wait_new(pthread_cond_t * cond, pthread_mutex_t * mutex)
{
	SleepConditionVariableCS(cond, mutex, INFINITE);
	return 0;
}

static __inline void pthread_cond_signal_new(pthread_cond_t * cond)
{
	WakeConditionVariable(cond);
}
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

#define KB  (1024)          // 1 KiloByte (1KB)   1024 Byte (2^10 Byte)
#define MB  (1024*KB)       // 1 MegaByte (1MB)   1024 KB (2^10 KB)
#define GB  (1024*MB)       // 1 GigaByte (1GB)   1024 MB (2^10 MB)
#define TB  (1024*GB)       // 1 TerraByte (1TB)  1024 GB (2^10 GB)

#define MAX_KNOWN_ATTRIBUTES  46


#define _ARRAY_SIZE(X)         (sizeof(X)/sizeof((X)[0]))



