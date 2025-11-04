/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

// Thread.h: interface for the CThread class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <string>
#include <stdint.h>
#include <windows.h>
#include <vector>
#include <limits>

unsigned int SystemClockMillis();

class EndTime
{
    unsigned int startTime;
    unsigned int totalWaitTime;
public:
    static const unsigned int InfiniteValue;
    inline EndTime() : startTime(0), totalWaitTime(0) {}
    inline EndTime(unsigned int millisecondsIntoTheFuture) : startTime(SystemClockMillis()), totalWaitTime(millisecondsIntoTheFuture) {}

    inline void Set(unsigned int millisecondsIntoTheFuture) { startTime = SystemClockMillis(); totalWaitTime = millisecondsIntoTheFuture; }

    inline bool IsTimePast() const { return totalWaitTime == InfiniteValue ? false : (totalWaitTime == 0 ? true : (SystemClockMillis() - startTime) >= totalWaitTime); }

    inline unsigned int MillisLeft() const
    {
        if (totalWaitTime == InfiniteValue)
            return InfiniteValue;
        if (totalWaitTime == 0)
            return 0;
        unsigned int timeWaitedAlready = (SystemClockMillis() - startTime);
        return (timeWaitedAlready >= totalWaitTime) ? 0 : (totalWaitTime - timeWaitedAlready);
    }

    inline void SetExpired() { totalWaitTime = 0; }
    inline void SetInfinite() { totalWaitTime = InfiniteValue; }
    inline bool IsInfinite(void) const { return (totalWaitTime == InfiniteValue); }
    inline unsigned int GetInitialTimeoutValue(void) const { return totalWaitTime; }
    inline unsigned int GetStartTime(void) const { return startTime; }
};

/**
 * Any class that inherits from NonCopyable will ... not be copyable (Duh!)
 */
class NonCopyable
{
    inline NonCopyable(const NonCopyable&) {}
    inline NonCopyable& operator=(const NonCopyable&) { return *this; }
public:
    inline NonCopyable() {}
};

/**
 * This will create a new predicate from an old predicate P with
 *  inverse truth value. This predicate is safe to use in a
 *  TightConditionVariable<P>
 */
template <class P> class InversePredicate
{
    P predicate;

public:
    inline InversePredicate(P predicate_) : predicate(predicate_) {}
    inline InversePredicate(const InversePredicate<P>& other) : predicate(other.predicate) {}
    inline InversePredicate<P>& operator=(InversePredicate<P>& other) { predicate = other.predicate; }

    inline bool operator!() const { return !(!predicate); }
};

// forward declare in preparation for the friend declaration
class ConditionVariable;

namespace windows
{
    class RecursiveMutex
    {
        CRITICAL_SECTION mutex;

        // needs acces to 'mutex'
        friend class ConditionVariable;
    public:
        inline RecursiveMutex()
        {
            //pthread_mutexattr_t attr;
            //pthread_mutexattr_init(&attr);
            //pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
            //pthread_mutex_init(&mutex, &attr);
            InitializeCriticalSection(&mutex);
        }

        inline ~RecursiveMutex()
        {
            //pthread_mutex_destroy(&mutex);
            DeleteCriticalSection(&mutex);
        }

        inline void lock()
        {
            //pthread_mutex_lock(&mutex);
            EnterCriticalSection(&mutex);
        }

        inline void unlock()
        {
            // pthread_mutex_unlock(&mutex);
            LeaveCriticalSection(&mutex);
        }

        inline bool try_lock()
        {
            //  return pthread_mutex_trylock(&mutex) ? false : true;
            return TryEnterCriticalSection(&mutex) ? true : false;
        }
    };
}

template<class L> class CountingLockable : public NonCopyable
{
    friend class ConditionVariable;
protected:
    L mutex;
    unsigned int count;

public:
    inline CountingLockable() : count(0) {}
    inline void lock() { mutex.lock(); count++; }
    inline bool try_lock() { return mutex.try_lock() ? count++, true : false; }
    inline void unlock() { count--; mutex.unlock(); }

    /**
     * This implements the "exitable" behavior mentioned above.
     *
     * This can be used to ALMOST exit, but not quite, by passing
     *  the number of locks to leave. This is used in the windows
     *  ConditionVariable which requires that the lock be entered
     *  only once, and so it backs out ALMOST all the way, but
     *  leaves one still there.
     */
    inline unsigned int exit(unsigned int leave = 0)
    {
        // it's possibe we don't actually own the lock
        // so we will try it.
        unsigned int ret = 0;
        if (try_lock())
        {
            if (leave < (count - 1))
            {
                ret = count - 1 - leave;  // The -1 is because we don't want 
                //  to count the try_lock increment.
// We must NOT compare "count" in this loop since 
// as soon as the last unlock is called another thread
// can modify it.
                for (unsigned int i = 0; i < ret; i++)
                    unlock();
            }
            unlock(); // undo the try_lock before returning
        }

        return ret;
    }

    /**
     * Restore a previous exit to the provided level.
     */
    inline void restore(unsigned int restoreCount)
    {
        for (unsigned int i = 0; i < restoreCount; i++)
            lock();
    }

    /**
     * Some implementations (see pthreads) require access to the underlying
     *  CCriticalSection, which is also implementation specific. This
     *  provides access to it through the same method on the guard classes
     *  UniqueLock, and SharedLock.
     *
     * There really should be no need for the users of the threading library
     *  to call this method.
     */
    inline L& get_underlying() { return mutex; }
};

/**
 * This template can be used to define the base implementation for any UniqueLock
 * (such as CSingleLock) that uses a Lockable as its mutex/critical section.
 */
template<typename L> class UniqueLock : public NonCopyable
{
protected:
    L& mutex;
    bool owns;
    inline UniqueLock(L& lockable) : mutex(lockable), owns(true) { mutex.lock(); }
    inline UniqueLock(L& lockable, bool try_to_lock_discrim) : mutex(lockable) { owns = mutex.try_lock(); }
    inline ~UniqueLock() { if (owns) mutex.unlock(); }

public:

    inline bool owns_lock() const { return owns; }

    //This also implements lockable
    inline void lock() { mutex.lock(); owns = true; }
    inline bool try_lock() { return (owns = mutex.try_lock()); }
    inline void unlock() { if (owns) { mutex.unlock(); owns = false; } }

    /**
     * See the note on the same method on CountingLockable
     */
    inline L& get_underlying() { return mutex; }
};

/**
 * This template can be used to define the base implementation for any SharedLock
 * (such as CSharedLock) that uses a Shared Lockable as its mutex/critical section.
 *
 * Something that implements the "Shared Lockable" concept has all of the methods
 * required by the Lockable concept and also:
 *
 * void lock_shared();
 * bool try_lock_shared();
 * void unlock_shared();
 */
template<typename L> class SharedLock : public NonCopyable
{
protected:
    L& mutex;
    bool owns;
    inline SharedLock(L& lockable) : mutex(lockable), owns(true) { mutex.lock_shared(); }
    inline ~SharedLock() { if (owns) mutex.unlock_shared(); }

    inline bool owns_lock() const { return owns; }
    inline void lock() { mutex.lock_shared(); owns = true; }
    inline bool try_lock() { return (owns = mutex.try_lock_shared()); }
    inline void unlock() { if (owns) mutex.unlock_shared(); owns = false; }

    /**
     * See the note on the same method on CountingLockable
     */
    inline L& get_underlying() { return mutex; }
};

class CCriticalSection : public CountingLockable<windows::RecursiveMutex> {};

template <typename P> class TightConditionVariable
{
    ConditionVariable& cond;
    P predicate;

public:
    inline TightConditionVariable(ConditionVariable& cv, P predicate_) : cond(cv), predicate(predicate_) {}

    template <typename L> inline void wait(L& lock) { while (!predicate) cond.wait(lock); }
    template <typename L> inline bool wait(L& lock, unsigned long milliseconds)
    {
        bool ret = true;
        if (!predicate)
        {
            if (!milliseconds)
            {
                cond.wait(lock, milliseconds /* zero */);
                return !(!predicate); // eh? I only require the ! operation on P
            }
            else
            {
                EndTime endTime((unsigned int)milliseconds);
                for (bool notdone = true; notdone && ret == true;
                    ret = (notdone = (!predicate)) ? ((milliseconds = endTime.MillisLeft()) != 0) : true)
                {
                    cond.wait(lock, milliseconds);
                    Sleep(1);
                }
            }
        }
        return ret;
    }

    inline void notifyAll() { cond.notifyAll(); }
    inline void notify() { cond.notify(); }
};

class CSingleLock : public UniqueLock<CCriticalSection>
{
public:
    inline CSingleLock(CCriticalSection& cs) : UniqueLock<CCriticalSection>(cs) {}
    inline CSingleLock(const CCriticalSection& cs) : UniqueLock<CCriticalSection>((CCriticalSection&)cs) {}

    inline void Leave() { unlock(); }
    inline void Enter() { lock(); }
protected:
    inline CSingleLock(CCriticalSection& cs, bool dicrim) : UniqueLock<CCriticalSection>(cs, true) {}
};

class CSingleExit
{
    CCriticalSection& sec;
    unsigned int count;
public:
    inline CSingleExit(CCriticalSection& cs) : sec(cs), count(cs.exit()) { }
    inline ~CSingleExit() { sec.restore(count); }
};

class ConditionVariable : public NonCopyable
{
    //friend class ConditionVariable;
    CONDITION_VARIABLE cond;
    CCriticalSection lock;
    int waiting;
    int signals;

public:

    class Semaphore
    {
        friend class ConditionVariable;
        //friend class ConditionVariableXp;
        HANDLE sem;
        volatile LONG count;

        inline Semaphore() : count(0L), sem(NULL) {  }
        inline ~Semaphore() { if (sem) CloseHandle(sem); }

        inline void Init() { sem = CreateSemaphore(NULL, 0, 32 * 1024, NULL); }

        inline bool wait(DWORD dwMilliseconds)
        {
            return (WAIT_OBJECT_0 == WaitForSingleObject(sem, dwMilliseconds)) ?
                (InterlockedDecrement(&count), true) : false;
        }

        inline bool post()
        {
            /* Increase the counter in the first place, because
             * after a successful release the semaphore may
             * immediately get destroyed by another thread which
             * is waiting for this semaphore.
             */
            InterlockedIncrement(&count);
            return ReleaseSemaphore(sem, 1, NULL) ? true : (InterlockedDecrement(&count), false);
        }
    };

    Semaphore wait_sem;
    Semaphore wait_done;

    inline ConditionVariable() : waiting(0), signals(0) { }
    inline ~ConditionVariable() {}

    inline void Init() { wait_sem.Init(); wait_done.Init(); }

    inline void wait(CCriticalSection& mutex)
    {
        wait(mutex, (unsigned long)-1L);
    }

    inline bool wait(CCriticalSection& mutex, unsigned long milliseconds)
    {
        bool success = false;
        DWORD ms = ((unsigned long)-1L) == milliseconds ? INFINITE : (DWORD)milliseconds;

        {
            CSingleLock l(lock);
            waiting++;
        }

        {
            CSingleExit ex(mutex);
            success = wait_sem.wait(ms);

            {
                CSingleLock l(lock);
                if (signals > 0)
                {
                    if (!success)
                        wait_sem.wait(INFINITE);
                    wait_done.post();
                    --signals;
                }
                --waiting;
            }
        }

        return success;
    }


    inline void wait(CSingleLock& lock) { wait(lock.get_underlying()); }
    inline bool wait(CSingleLock& lock, unsigned long milliseconds) { return wait(lock.get_underlying(), milliseconds); }

    inline void notifyAll()
    {
        /* If there are waiting threads not already signalled, then
           signal the condition and wait for the thread to respond.
        */
        CSingleLock l(lock);
        if (waiting > signals)
        {
            int i, num_waiting;

            num_waiting = (waiting - signals);
            signals = waiting;
            for (i = 0; i < num_waiting; ++i)
                wait_sem.post();

            /* Now all released threads are blocked here, waiting for us.
               Collect them all (and win fabulous prizes!) :-)
            */
            l.Leave();
            for (i = 0; i < num_waiting; ++i)
                wait_done.wait(INFINITE);
        }
    }

    inline void notify()
    {
        /* If there are waiting threads not already signalled, then
           signal the condition and wait for the thread to respond.
        */
        CSingleLock l(lock);
        if (waiting > signals)
        {
            ++signals;
            wait_sem.post();
            l.Leave();
            wait_done.wait(INFINITE);
        }
    }
};

class CSingleTryLock : public CSingleLock
{
public:
    inline CSingleTryLock(CCriticalSection& cs) : CSingleLock(cs, true) {}

    inline bool IsOwner() const { return owns_lock(); }
};



class CSharedSection
{
    CCriticalSection sec;
    ConditionVariable actualCv;
    TightConditionVariable<InversePredicate<unsigned int&> > cond;

    unsigned int sharedCount;

public:
    inline CSharedSection() : cond(actualCv, InversePredicate<unsigned int&>(sharedCount)), sharedCount(0) {}

    inline void lock() { CSingleLock l(sec); while (sharedCount) cond.wait(l); sec.lock(); }
    inline bool try_lock() { return (sec.try_lock() ? ((sharedCount == 0) ? true : (sec.unlock(), false)) : false); }
    inline void unlock() { sec.unlock(); }

    inline void lock_shared() { CSingleLock l(sec); sharedCount++; }
    inline bool try_lock_shared() { return (sec.try_lock() ? sharedCount++, sec.unlock(), true : false); }
    inline void unlock_shared() { CSingleLock l(sec); sharedCount--; if (!sharedCount) { cond.notifyAll(); } }
};

class CSharedLock : public SharedLock<CSharedSection>
{
public:
    inline CSharedLock(CSharedSection& cs) : SharedLock<CSharedSection>(cs) {}
    inline CSharedLock(const CSharedSection& cs) : SharedLock<CSharedSection>((CSharedSection&)cs) {}

    inline bool IsOwner() const { return owns_lock(); }
    inline void Enter() { lock(); }
    inline void Leave() { unlock(); }
};

class CExclusiveLock : public UniqueLock<CSharedSection>
{
public:
    inline CExclusiveLock(CSharedSection& cs) : UniqueLock<CSharedSection>(cs) {}
    inline CExclusiveLock(const CSharedSection& cs) : UniqueLock<CSharedSection>((CSharedSection&)cs) {}

    inline bool IsOwner() const { return owns_lock(); }
    inline void Leave() { unlock(); }
    inline void Enter() { lock(); }
};

class CEventGroup;

class CEvent : public NonCopyable
{
    bool manualReset;
    volatile bool signaled;
    unsigned int numWaits;

    CCriticalSection groupListMutex; // lock for the groups list
    std::vector<CEventGroup*>* groups;

    /**
     * To satisfy the TightConditionVariable requirements and allow the
     *  predicate being monitored to include both the signaled and interrupted
     *  states.
     */
    ConditionVariable actualCv;
    TightConditionVariable<volatile bool&> condVar;
    CCriticalSection mutex;

    friend class CEventGroup;

    void addGroup(CEventGroup* group);
    void removeGroup(CEventGroup* group);

    // helper for the two wait methods
    inline bool prepReturn() { bool ret = signaled; if (!manualReset && numWaits == 0) signaled = false; return ret; }

public:
    inline CEvent(bool manual = false, bool signaled_ = false) :
        manualReset(manual), signaled(signaled_), numWaits(0), groups(NULL), condVar(actualCv, signaled) {}

    inline void Reset() { CSingleLock lock(mutex); signaled = false; }
    void Set();

    /** Returns true if Event has been triggered and not reset, false otherwise. */
    inline bool Signaled() { CSingleLock lock(mutex); return signaled; }

    /**
     * This will wait up to 'milliSeconds' milliseconds for the Event
     *  to be triggered. The method will return 'true' if the Event
     *  was triggered. Otherwise it will return false.
     */
    inline bool WaitMSec(unsigned int milliSeconds)
    {
        CSingleLock lock(mutex); numWaits++; condVar.wait(mutex, milliSeconds); numWaits--; return prepReturn();
    }

    /**
     * This will wait for the Event to be triggered. The method will return
     * 'true' if the Event was triggered. If it was either interrupted
     * it will return false. Otherwise it will return false.
     */
    inline bool Wait()
    {
        CSingleLock lock(mutex); numWaits++; condVar.wait(mutex); numWaits--; return prepReturn();
    }

    /**
     * This is mostly for testing. It allows a thread to make sure there are
     *  the right amount of other threads waiting.
     */
    inline int getNumWaits() { CSingleLock lock(mutex); return numWaits; }

};

class CEventGroup : public NonCopyable
{
    std::vector<CEvent*> events;
    CEvent* signaled;
    ConditionVariable actualCv;
    TightConditionVariable<CEvent*&> condVar;
    CCriticalSection mutex;

    unsigned int numWaits;

    // This is ONLY called from CEvent::Set.
    inline void Set(CEvent* child) { CSingleLock l(mutex); signaled = child; condVar.notifyAll(); }

    friend class ::CEvent;

public:

    /**
     * Create a CEventGroup from a number of CEvents. num is the number
     *  of Events that follow. E.g.:
     *
     *  CEventGroup g(3, event1, event2, event3);
     */
    CEventGroup(int num, CEvent* v1, ...);

    /**
     * Create a CEventGroup from a number of CEvents. The parameters
     *  should form a NULL terminated list of CEvent*'s
     *
     *  CEventGroup g(event1, event2, event3, NULL);
     */
    CEventGroup(CEvent* v1, ...);
    ~CEventGroup();

    /**
     * This will block until any one of the CEvents in the group are
     * signaled at which point a pointer to that CEvents will be
     * returned.
     */
    CEvent* wait();

    /**
     * This will block until any one of the CEvents in the group are
     * signaled or the timeout is reachec. If an event is signaled then
     * it will return a pointer to that CEvent, otherwise it will return
     * NULL.
     */
    CEvent* wait(unsigned int milliseconds);

    /**
     * This is mostly for testing. It allows a thread to make sure there are
     *  the right amount of other threads waiting.
     */
    inline int getNumWaits() { CSingleLock lock(mutex); return numWaits; }

};

struct threadOpaque
{
    HANDLE handle;
};

typedef DWORD ThreadIdentifier;
typedef threadOpaque ThreadOpaque;
#define THREADFUNC unsigned __stdcall

inline static void ThreadSleep(unsigned int millis) { Sleep(millis); }

template <typename T> class ThreadLocal
{
    DWORD key;
public:
    inline ThreadLocal()
    {
        key = TlsAlloc();
    }

    inline ~ThreadLocal()
    {
       TlsFree(key);
    }

    inline void set(T* val)
    {
        TlsSetValue(key, (LPVOID)val);
    }

    inline T* get() { return (T*)TlsGetValue(key); }
};

#ifdef TARGET_DARWIN
#include <mach/mach.h>
#endif

class IRunnable
{
public:
  virtual void Run()=0;
  virtual ~IRunnable() {}
};

// minimum as mandated by XTL
#define THREAD_MINSTACKSIZE 0x10000

class ThreadSettings;

class AThread
{
protected:
    AThread(const char* ThreadName);

public:
    AThread(IRunnable* pRunnable, const char* ThreadName);
  virtual ~AThread();
  void Create(bool bAutoDelete = false, unsigned stacksize = 0);
  void Sleep(unsigned int milliseconds);
  int GetSchedRRPriority(void);
  bool SetPrioritySched_RR(int iPriority);
  bool IsAutoDelete() const;
  virtual void StopThread(bool bWait = true);
  bool IsRunning() const;

  // -----------------------------------------------------------------------------------
  // These are platform specific and can be found in ./platform/[platform]/ThreadImpl.cpp
  // -----------------------------------------------------------------------------------
  bool IsCurrentThread() const;
  int GetMinPriority(void);
  int GetMaxPriority(void);
  int GetNormalPriority(void);
  int GetPriority(void);
  bool SetPriority(const int iPriority);
  bool WaitForThreadExit(unsigned int milliseconds);
  float GetRelativeUsage();  // returns the relative cpu usage of this thread since last call
  int64_t GetAbsoluteUsage();
  // -----------------------------------------------------------------------------------

  static bool IsCurrentThread(const ThreadIdentifier tid);
  static ThreadIdentifier GetCurrentThreadId();
  static AThread* GetCurrentThread();

  virtual void OnException(){} // signal termination handler
protected:
  virtual void OnStartup(){};
  virtual void OnExit(){};
  virtual void Process();

  volatile bool m_bStop;

  enum WaitResponse { WAIT_INTERRUPTED = -1, WAIT_SIGNALED = 0, WAIT_TIMEDOUT = 1 };

  /**
   * This call will wait on a CEvent in an interruptible way such that if
   *  stop is called on the thread the wait will return with a response
   *  indicating what happened.
   */
  inline WaitResponse AbortableWait(CEvent& event, int timeoutMillis = -1 /* indicates wait forever*/)
  {
    CEventGroup group(&event, &m_StopEvent, NULL);
    CEvent* result = timeoutMillis < 0 ? group.wait() : group.wait(timeoutMillis);
    return  result == &event ? WAIT_SIGNALED :
      (result == NULL ? WAIT_TIMEDOUT : WAIT_INTERRUPTED);
  }

private:
  static THREADFUNC staticThread(void *data);
  void Action();

  // -----------------------------------------------------------------------------------
  // These are platform specific and can be found in ./platform/[platform]/ThreadImpl.cpp
  // -----------------------------------------------------------------------------------
  ThreadIdentifier ThreadId() const;
  void SetThreadInfo();
  void TermHandler();
  void SetSignalHandlers();
  void SpawnThread(unsigned stacksize);
  // -----------------------------------------------------------------------------------

  ThreadIdentifier m_ThreadId;
  ThreadOpaque m_ThreadOpaque;
  bool m_bAutoDelete;
  CEvent m_StopEvent;
  CEvent m_TermEvent;
  CEvent m_StartEvent;
  CCriticalSection m_CriticalSection;
  IRunnable* m_pRunnable;
  uint64_t m_iLastUsage;
  uint64_t m_iLastTime;
  float m_fLastUsage;

  std::string m_ThreadName;
};
