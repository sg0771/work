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

#ifndef THREADS_H
#define THREADS_H

#if defined(WIN32)
#include <windows.h>
#include "unistd.h"

#define sleepms(x) Sleep(x)

typedef HANDLE thread_handle_t;

#define THREAD_RETVAL DWORD WINAPI
#define THREAD_CREATE(handle, func, arg) \
	handle = CreateThread(NULL, 0, func, arg, 0, NULL)
#define THREAD_JOIN(handle) do { WaitForSingleObject(handle, INFINITE); CloseHandle(handle); } while(0)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>

typedef CRITICAL_SECTION pthread_mutex_t;
typedef CONDITION_VARIABLE pthread_cond_t;

typedef pthread_mutex_t mutex_handle_t;
typedef pthread_cond_t cond_handle_t;

#define MUTEX_CREATE(handle) pthread_mutex_init(&(handle), NULL)
#define MUTEX_LOCK(handle) pthread_mutex_lock(&(handle))
#define MUTEX_UNLOCK(handle) pthread_mutex_unlock(&(handle))
#define MUTEX_DESTROY(handle) pthread_mutex_destroy(&(handle))

static __inline int pthread_mutex_init(pthread_mutex_t *m, void* attr)
{
	InitializeCriticalSection(m);
	return 0;
}
static __inline int pthread_mutex_destroy(pthread_mutex_t *m)
{
	DeleteCriticalSection(m);
	return 0;
}
static __inline int pthread_mutex_lock(pthread_mutex_t *m)
{
	EnterCriticalSection(m);
	return 0;
}
static __inline int pthread_mutex_unlock(pthread_mutex_t *m)
{
	LeaveCriticalSection(m);
	return 0;
}

#if _WIN32_WINNT >= 0x0600
static __inline int pthread_cond_init_new(pthread_cond_t *cond, const void *unused_attr)
{
	InitializeConditionVariable(cond);
	return 0;
}

/* native condition variables do not destroy */
static __inline void pthread_cond_destroy_new(pthread_cond_t *cond)
{
	return;
}

static __inline void pthread_cond_broadcast(pthread_cond_t *cond)
{
	WakeAllConditionVariable(cond);
}

static __inline int pthread_cond_wait_new(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
	SleepConditionVariableCS(cond, mutex, INFINITE);
	return 0;
}

static __inline void pthread_cond_signal_new(pthread_cond_t *cond)
{
	WakeConditionVariable(cond);
}
#endif

#define COND_CREATE(handle) pthread_cond_init_new(&(handle), NULL)
#define COND_WAIT(handle, mutex) pthread_cond_wait_new(&(handle), &(mutex))
#define COND_SIGNAL(handle) pthread_cond_signal_new(&(handle))
#define COND_DESTROY(handle) pthread_cond_destroy_new(&(handle))

#else /* Use pthread library */

#include <pthread.h>
#include "unistd.h"

#define sleepms(x) usleep((x)*1000)

typedef pthread_t thread_handle_t;

#define THREAD_RETVAL void *
#define THREAD_CREATE(handle, func, arg) \
if (pthread_create(&(handle), NULL, func, arg)) handle = 0
#define THREAD_JOIN(handle) pthread_join(handle, NULL)
#define THREAD_CANCEL(handle) pthread_cancel(handle)

#define MUTEX_CREATE(handle) pthread_mutex_init(&(handle), NULL)
#define MUTEX_LOCK(handle) pthread_mutex_lock(&(handle))
#define MUTEX_UNLOCK(handle) pthread_mutex_unlock(&(handle))
#define MUTEX_DESTROY(handle) pthread_mutex_destroy(&(handle))

#define COND_CREATE(handle) pthread_cond_init(&(handle), NULL)
#define COND_WAIT(handle, mutex) pthread_cond_wait(&(handle), &(mutex))
#define COND_SIGNAL(handle) pthread_cond_signal(&(handle))
#define COND_DESTROY(handle) pthread_cond_destroy(&(handle))

#endif

#endif /* THREADS_H */
