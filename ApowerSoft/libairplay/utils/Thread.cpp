
#include "Thread.h"



#define __STDC_FORMAT_MACROS
#include <inttypes.h>
static ThreadLocal<AThread> currentThread;
#include <process.h>
#include "../utils/WIN32Utils.h"

void AThread::SpawnThread(unsigned stacksize)
{
    // Create in the suspended state, so that no matter the thread priorities and scheduled order, the handle will be assigned
    // before the new thread exits.
    unsigned threadId;
    m_ThreadOpaque.handle = (HANDLE)_beginthreadex(NULL, stacksize, &staticThread, this, CREATE_SUSPENDED, &threadId);
    if (m_ThreadOpaque.handle == NULL)
    {
        //if (logger) logger->Log(LOGERROR, "%s - fatal error %d creating thread", __FUNCTION__, GetLastError());
        return;
    }
    m_ThreadId = threadId;

    ResumeThread(m_ThreadOpaque.handle);

}

void AThread::TermHandler()
{
    CloseHandle(m_ThreadOpaque.handle);
    m_ThreadOpaque.handle = NULL;
}

void AThread::SetThreadInfo()
{
    const unsigned int MS_VC_EXCEPTION = 0x406d1388;

#pragma pack(push,8)
    struct THREADNAME_INFO
    {
        DWORD dwType; // must be 0x1000
        LPCSTR szName; // pointer to name (in same addr space)
        DWORD dwThreadID; // thread ID (-1 caller thread)
        DWORD dwFlags; // reserved for future use, most be zero
    } info;
#pragma pack(pop)

    info.dwType = 0x1000;
    info.szName = m_ThreadName.c_str();
    info.dwThreadID = m_ThreadId;
    info.dwFlags = 0;

    __try
    {
        RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

ThreadIdentifier AThread::GetCurrentThreadId()
{
    return ::GetCurrentThreadId();
}

bool AThread::IsCurrentThread(const ThreadIdentifier tid)
{
    return (::GetCurrentThreadId() == tid);
}

int AThread::GetMinPriority(void)
{
    return(THREAD_PRIORITY_IDLE);
}

int AThread::GetMaxPriority(void)
{
    return(THREAD_PRIORITY_HIGHEST);
}

int AThread::GetNormalPriority(void)
{
    return(THREAD_PRIORITY_NORMAL);
}

int AThread::GetSchedRRPriority(void)
{
    return GetNormalPriority();
}

bool AThread::SetPriority(const int iPriority)
{
    bool bReturn = false;

    CSingleLock lock(m_CriticalSection);
    if (m_ThreadOpaque.handle)
    {
        bReturn = SetThreadPriority(m_ThreadOpaque.handle, iPriority) == TRUE;
    }

    return bReturn;
}

int AThread::GetPriority()
{
    CSingleLock lock(m_CriticalSection);

    int iReturn = THREAD_PRIORITY_NORMAL;
    if (m_ThreadOpaque.handle)
    {
        iReturn = GetThreadPriority(m_ThreadOpaque.handle);
    }
    return iReturn;
}

bool AThread::WaitForThreadExit(unsigned int milliseconds)
{
    bool bReturn = true;

    CSingleLock lock(m_CriticalSection);
    if (m_ThreadId && m_ThreadOpaque.handle != NULL)
    {
        // boost priority of thread we are waiting on to same as caller
        int callee = GetThreadPriority(m_ThreadOpaque.handle);
        int caller = GetThreadPriority(::GetCurrentThread());
        if (caller != THREAD_PRIORITY_ERROR_RETURN && caller > callee)
            SetThreadPriority(m_ThreadOpaque.handle, caller);

        lock.Leave();
        bReturn = m_TermEvent.WaitMSec(milliseconds);
        lock.Enter();

        // restore thread priority if thread hasn't exited
        if (callee != THREAD_PRIORITY_ERROR_RETURN && caller > callee && m_ThreadOpaque.handle)
            SetThreadPriority(m_ThreadOpaque.handle, callee);
    }
    return bReturn;
}

int64_t AThread::GetAbsoluteUsage()
{
    CSingleLock lock(m_CriticalSection);

    if (!m_ThreadOpaque.handle)
        return 0;

    uint64_t time = 0;
    FILETIME CreationTime, ExitTime, UserTime, KernelTime;
    if (GetThreadTimes(m_ThreadOpaque.handle, &CreationTime, &ExitTime, &KernelTime, &UserTime))
    {
        time = (((uint64_t)UserTime.dwHighDateTime) << 32) + ((uint64_t)UserTime.dwLowDateTime);
        time += (((uint64_t)KernelTime.dwHighDateTime) << 32) + ((uint64_t)KernelTime.dwLowDateTime);
    }
    return time;
}

float AThread::GetRelativeUsage()
{
    unsigned int iTime = SystemClockMillis();
    iTime *= 10000; // convert into 100ns tics

    // only update every 1 second
    if (iTime < m_iLastTime + 1000 * 10000) return m_fLastUsage;

    int64_t iUsage = GetAbsoluteUsage();

    if (m_iLastUsage > 0 && m_iLastTime > 0)
        m_fLastUsage = (float)(iUsage - m_iLastUsage) / (float)(iTime - m_iLastTime);

    m_iLastUsage = iUsage;
    m_iLastTime = iTime;

    return m_fLastUsage;
}

void AThread::SetSignalHandlers()
{
    // install win32 exception translator
    //win32_exception::install_handler();
}


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//#define LOG if(logger) logger->Log

AThread::AThread(const char* ThreadName)
: m_StopEvent(true,true), m_TermEvent(true), m_StartEvent(true)
{
  m_bStop = false;

  m_bAutoDelete = false;
  m_ThreadId = 0;
  m_iLastTime = 0;
  m_iLastUsage = 0;
  m_fLastUsage = 0.0f;

  m_pRunnable=NULL;

  if (ThreadName)
    m_ThreadName = ThreadName;
}

AThread::AThread(IRunnable* pRunnable, const char* ThreadName)
: m_StopEvent(true,true), m_TermEvent(true), m_StartEvent(true)
{
  m_bStop = false;

  m_bAutoDelete = false;
  m_ThreadId = 0;
  m_iLastTime = 0;
  m_iLastUsage = 0;
  m_fLastUsage = 0.0f;

  m_pRunnable=pRunnable;

  if (ThreadName)
    m_ThreadName = ThreadName;
}

AThread::~AThread()
{
  StopThread();
}

void AThread::Create(bool bAutoDelete, unsigned stacksize)
{
  if (m_ThreadId != 0)
  {
    ;//LOG(LOGERROR, "%s - fatal error creating thread- old thread id not null", __FUNCTION__);
    exit(1);
  }
  m_iLastTime = SystemClockMillis() * 10000;
  m_iLastUsage = 0;
  m_fLastUsage = 0.0f;
  m_bAutoDelete = bAutoDelete;
  m_bStop = false;
  m_StopEvent.Reset();
  //m_TermEvent.Reset();
  //m_StartEvent.Reset();

  SpawnThread(stacksize);
}

bool AThread::IsRunning() const
{
  return m_ThreadId ? true : false;
}

THREADFUNC AThread::staticThread(void* data)
{
    AThread* pThread = (AThread*)(data);
  std::string name;
  ThreadIdentifier id;
  bool autodelete;

  if (!pThread) {
    ;//LOG(LOGERROR,"%s, sanity failed. thread is NULL.",__FUNCTION__);
    return 1;
  }

  name = pThread->m_ThreadName;
  id = pThread->m_ThreadId;
  autodelete = pThread->m_bAutoDelete;

  pThread->SetThreadInfo();

  ;//LOG(LOGNOTICE,"Thread %s start, auto delete: %s", name.c_str(), (autodelete ? "true" : "false"));

  currentThread.set(pThread);
  //pThread->m_StartEvent.Set();

  pThread->Action();

  // lock during termination
  CSingleLock lock(pThread->m_CriticalSection);

  pThread->m_ThreadId = 0;
  //pThread->m_TermEvent.Set();
  pThread->TermHandler();

  lock.Leave();

  if (autodelete)
  {
    ;//LOG(LOGDEBUG,"Thread %s %" PRIu64" terminating (autodelete)", name.c_str(), (uint64_t)id);
    delete pThread;
    pThread = NULL;
  }
  else
    ;//LOG(LOGDEBUG,"Thread %s %" PRIu64" terminating", name.c_str(), (uint64_t)id);

  return 0;
}

bool AThread::IsAutoDelete() const
{
  return m_bAutoDelete;
}

void AThread::StopThread(bool bWait /*= true*/)
{
  m_bStop = true;
  m_StopEvent.Set();
  CSingleLock lock(m_CriticalSection);
  if (m_ThreadId && bWait)
  {
    lock.Leave();
    WaitForThreadExit(0xFFFFFFFF);
  }
}

ThreadIdentifier AThread::ThreadId() const
{
  return m_ThreadId;
}

void AThread::Process()
{
  if(m_pRunnable)
    m_pRunnable->Run();
}

bool AThread::IsCurrentThread() const
{
  return IsCurrentThread(ThreadId());
}

AThread* AThread::GetCurrentThread()
{
  return currentThread.get();
}

void AThread::Sleep(unsigned int milliseconds)
{
  if(milliseconds > 10 && IsCurrentThread())
    m_StopEvent.WaitMSec(milliseconds);
  else
    ThreadSleep(milliseconds);
}

void AThread::Action()
{
  try
  {
    OnStartup();
  }
  catch (...)
  {
    ;//LOG(LOGERROR, "%s - thread %s, Unhandled exception caught in thread startup, aborting. auto delete: %d", __FUNCTION__, m_ThreadName.c_str(), IsAutoDelete());
    if (IsAutoDelete())
      return;
  }

  try
  {
    Process();
  }
  catch (...)
  {
    ;//LOG(LOGERROR, "%s - thread %s, Unhandled exception caught in thread process, aborting. auto delete: %d", __FUNCTION__, m_ThreadName.c_str(), IsAutoDelete());
  }

  try
  {
    OnExit();
  }
  catch (...)
  {
    ;//LOG(LOGERROR, "%s - thread %s, Unhandled exception caught in thread OnExit, aborting. auto delete: %d", __FUNCTION__, m_ThreadName.c_str(), IsAutoDelete());
  }
}



#include <stdint.h>

#if   defined(TARGET_DARWIN)
#include <mach/mach_time.h>
#include <CoreVideo/CVHostTime.h>
#elif defined(_WIN32)
#include <windows.h>
#include <Mmsystem.h>
#else
#include <time.h>
#endif

unsigned int SystemClockMillis()
{
    uint64_t now_time;
    static uint64_t start_time = 0;
    static bool start_time_set = false;
#if defined(TARGET_DARWIN)
    now_time = CVGetCurrentHostTime() * 1000 / CVGetHostClockFrequency();
#elif defined(_WIN32)
    now_time = (uint64_t)timeGetTime();
#else
    struct timespec ts = {};
#ifdef CLOCK_MONOTONIC_RAW
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
#else
    clock_gettime(CLOCK_MONOTONIC, &ts);
#endif // CLOCK_MONOTONIC_RAW
    now_time = (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
#endif
    if (!start_time_set)
    {
        start_time = now_time;
        start_time_set = true;
    }
    return (unsigned int)(now_time - start_time);
}
const unsigned int EndTime::InfiniteValue = std::numeric_limits<unsigned int>::max();



#include <stdarg.h>
#include <limits>


void CEvent::addGroup(CEventGroup* group)
{
    CSingleLock lock(groupListMutex);
    if (groups == NULL)
        groups = new std::vector<CEventGroup*>();

    groups->push_back(group);
}

void CEvent::removeGroup(CEventGroup* group)
{
    CSingleLock lock(groupListMutex);
    if (groups)
    {
        for (std::vector<CEventGroup*>::iterator iter = groups->begin(); iter != groups->end(); ++iter)
        {
            if ((*iter) == group)
            {
                groups->erase(iter);
                break;
            }
        }

        if (groups->size() <= 0)
        {
            delete groups;
            groups = NULL;
        }
    }
}

// locking is ALWAYS done in this order:
//  CEvent::groupListMutex -> CEventGroup::mutex -> CEvent::mutex
void CEvent::Set()
{
    // Originally I had this without locking. Thanks to FernetMenta who
    // pointed out that this creates a race condition between setting
    // checking the signal and calling wait() on the Wait call in the
    // CEvent class. This now perfectly matches the boost example here:
    // http://www.boost.org/doc/libs/1_41_0/doc/html/thread/synchronization.html#thread.synchronization.condvar_ref
    {
        CSingleLock slock(mutex);
        signaled = true;
    }

    condVar.notifyAll();

    CSingleLock l(groupListMutex);
    if (groups)
    {
        for (std::vector<CEventGroup*>::iterator iter = groups->begin();
            iter != groups->end(); ++iter)
            (*iter)->Set(this);
    }
}

/**
 * This will block until any one of the CEvents in the group are
 * signaled at which point a pointer to that CEvents will be
 * returned.
 */
CEvent* CEventGroup::wait()
{
    return wait(std::numeric_limits<unsigned int>::max());
}

/**
 * This will block until any one of the CEvents in the group are
 * signaled or the timeout is reachec. If an event is signaled then
 * it will return a pointer to that CEvent, otherwise it will return
 * NULL.
 */
 // locking is ALWAYS done in this order:
 //  CEvent::groupListMutex -> CEventGroup::mutex -> CEvent::mutex
 //
 // Notice that this method doesn't grab the CEvent::groupListMutex at all. This
 // is fine. It just grabs the CEventGroup::mutex and THEN the individual 
 // CEvent::mutex's
CEvent* CEventGroup::wait(unsigned int milliseconds)
{
    CSingleLock lock(mutex); // grab CEventGroup::mutex
    numWaits++;

    // ==================================================
    // This block checks to see if any child events are 
    // signaled and sets 'signaled' to the first one it
    // finds.
    // ==================================================
    signaled = NULL;
    for (std::vector<CEvent*>::iterator iter = events.begin();
        signaled == NULL && iter != events.end(); ++iter)
    {
        CEvent* cur = *iter;
        if (cur->signaled)
            signaled = cur;
    }
    // ==================================================

    if (!signaled)
    {
        // both of these release the CEventGroup::mutex
        if (milliseconds == std::numeric_limits<unsigned int>::max())
            condVar.wait(mutex);
        else
            condVar.wait(mutex, milliseconds);
    } // at this point the CEventGroup::mutex is reacquired
    numWaits--;

    // signaled should have been set by a call to CEventGroup::Set
    CEvent* ret = signaled;
    if (numWaits == 0)
    {
        if (signaled)
            // This acquires and releases the CEvent::mutex. This is fine since the
            //  CEventGroup::mutex is already being held
            signaled->WaitMSec(0); // reset the event if needed
        signaled = NULL;  // clear the signaled if all the waiters are gone
    }
    return ret;
}

CEventGroup::CEventGroup(int num, CEvent* v1, ...) : signaled(NULL), condVar(actualCv, signaled), numWaits(0)
{
    va_list ap;

    va_start(ap, v1);
    if (v1)
        events.push_back(v1);
    num--; // account for v1
    for (; num > 0; num--)
    {
        CEvent* const cur = va_arg(ap, CEvent*);
        if (cur)
            events.push_back(cur);
    }
    va_end(ap);

    // we preping for a wait, so we need to set the group value on
    // all of the CEvents. 
    for (std::vector<CEvent*>::iterator iter = events.begin();
        iter != events.end(); ++iter)
        (*iter)->addGroup(this);
}

CEventGroup::CEventGroup(CEvent* v1, ...) : signaled(NULL), condVar(actualCv, signaled), numWaits(0)
{
    va_list ap;

    va_start(ap, v1);
    if (v1)
        events.push_back(v1);
    bool done = false;
    while (!done)
    {
        CEvent* cur = va_arg(ap, CEvent*);
        if (cur)
            events.push_back(cur);
        else
            done = true;
    }
    va_end(ap);

    // we preping for a wait, so we need to set the group value on
    // all of the CEvents. 
    for (std::vector<CEvent*>::iterator iter = events.begin();
        iter != events.end(); ++iter)
        (*iter)->addGroup(this);
}

CEventGroup::~CEventGroup()
{
    for (std::vector<CEvent*>::iterator iter = events.begin();
        iter != events.end(); ++iter)
        (*iter)->removeGroup(this);
}
