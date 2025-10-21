//------------------------------------------------------------------------------
// File: WXDebug.h
//
// Desc: DirectShow base classes - provides debugging facilities.
//
// Copyright (c) 1992-2001 Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------


#ifndef __WXDEBUG__
#define __WXDEBUG__

// This library provides fairly straight forward debugging functionality, this
// is split into two main sections. The first is assertion handling, there are
// three types of assertions provided here. The most commonly used one is the
// ASSERT(condition) macro which will pop up a message box including the file
// and line number if the condition evaluates to FALSE. Then there is the
// EXECUTE_ASSERT macro which is the same as ASSERT except the condition will
// still be executed in NON debug builds. The final type of assertion is the
// KASSERT macro which is more suitable for pure (perhaps kernel) filters as
// the condition is printed onto the debugger rather than in a message box.
//
// The other part of the debug module facilties is general purpose logging.
// This is accessed by calling DbgLog(). The function takes a type and level
// field which define the type of informational string you are presenting and
// it's relative importance. The type field can be a combination (one or more)
// of LOG_TIMING, LOG_TRACE, LOG_MEMORY, LOG_LOCKING and LOG_ERROR. The level
// is a DWORD value where zero defines highest important. Use of zero as the
// debug logging level is to be encouraged ONLY for major errors or events as
// they will ALWAYS be displayed on the debugger. Other debug output has it's
// level matched against the current debug output level stored in the registry
// for this module and if less than the current setting it will be displayed.
//
// Each module or executable has it's own debug output level for each of the
// five types. These are read in when the DbgInitialise function is called
// for DLLs linking to STRMBASE.LIB this is done automatically when the DLL
// is loaded, executables must call it explicitely with the module instance
// handle given to them through the WINMAIN entry point. An executable must
// also call DbgTerminate when they have finished to clean up the resources
// the debug library uses, once again this is done automatically for DLLs

// These are the five different categories of logging information

enum {  LOG_TIMING = 0x01,    // Timing and performance measurements
        LOG_TRACE = 0x02,     // General step point call tracing
        LOG_MEMORY =  0x04,   // Memory and object allocation/destruction
        LOG_LOCKING = 0x08,   // Locking/unlocking of critical sections
        LOG_ERROR = 0x10,     // Debug error notification
        LOG_CUSTOM1 = 0x20,
        LOG_CUSTOM2 = 0x40,
        LOG_CUSTOM3 = 0x80,
        LOG_CUSTOM4 = 0x100,
        LOG_CUSTOM5 = 0x200,
};

#define LOG_FORCIBLY_SET 0x80000000

enum {  CDISP_HEX = 0x01,
        CDISP_DEC = 0x02};

// For each object created derived from CBaseObject (in debug builds) we
// create a descriptor that holds it's name (statically allocated memory)
// and a cookie we assign it. We keep a list of all the active objects
// we have registered so that we can dump a list of remaining objects

typedef struct tag_ObjectDesc {
    LPCSTR m_szName;
    LPCWSTR m_wszName;
    DWORD m_dwCookie;
    tag_ObjectDesc *m_pNext;
} ObjectDesc;

#define DLLIMPORT __declspec(dllimport)
#define DLLEXPORT __declspec(dllexport)

    #define NAME(_x_) ((LPTSTR) NULL)

    #define DbgInitialise(hInst)
    #define DbgTerminate()
    #define DbgLog(_x_) 0
    #define DbgOutString(psz)
    #define DbgAssertAligned( _ptr_, _alignment_ ) 0

    #define DbgRegisterObjectCreation(pObjectName)
    #define DbgRegisterObjectDestruction(dwCookie)
    #define DbgDumpObjectRegister()

    #define DbgCheckModuleLevel(Type,Level)
    #define DbgSetModuleLevel(Type,Level)
    #define DbgSetAutoRefreshLevels(fAuto)

    #define DbgWaitForSingleObject(h)  WaitForSingleObject(h, INFINITE)
    #define DbgWaitForMultipleObjects(nCount, lpHandles, bWaitAll)     \
               WaitForMultipleObjects(nCount, lpHandles, bWaitAll, INFINITE)
    #define DbgSetWaitTimeout(dwTimeout)

    #define KDbgBreak(_x_)
    #define DbgBreak(_x_)

    #define KASSERT(_x_) ((void)0)
    #ifndef ASSERT
	#define ASSERT(_x_) ((void)0)
    #endif
    #define EXECUTE_ASSERT(_x_) ((void)(_x_))

    // MFC style trace macros

    #define NOTE(_x_) ((void)0)
    #define NOTE1(_x_,a) ((void)0)
    #define NOTE2(_x_,a,b) ((void)0)
    #define NOTE3(_x_,a,b,c) ((void)0)
    #define NOTE4(_x_,a,b,c,d) ((void)0)
    #define NOTE5(_x_,a,b,c,d,e) ((void)0)

    //#define DisplayType(label, pmtIn) ((void)0)
    #define DumpGraph(pGraph, label) ((void)0)



// Checks a pointer which should be non NULL - can be used as follows.

#define CheckPointer(p,ret) {if((p)==NULL) return (ret);}

//   HRESULT Foo(VOID *pBar)
//   {
//       CheckPointer(pBar,E_INVALIDARG)
//   }
//
//   Or if the function returns a boolean
//
//   BOOL Foo(VOID *pBar)
//   {
//       CheckPointer(pBar,FALSE)
//   }

#define ValidateReadPtr(p,cb) 0
#define ValidateWritePtr(p,cb) 0
#define ValidateReadWritePtr(p,cb) 0
#define ValidateStringPtr(p) 0
#define ValidateStringPtrA(p) 0
#define ValidateStringPtrW(p) 0


#ifdef _OBJBASE_H_

    //  Outputting GUID names.  If you want to include the name
    //  associated with a GUID (eg CLSID_...) then
    //
    //      GuidNames[yourGUID]
    //
    //  Returns the name defined in uuids.h as a string

    typedef struct {
        CHAR   *szName;
        GUID    guid;
    } GUID_STRING_ENTRY;

    class CGuidNameList {
    public:
        CHAR *operator [] (const GUID& guid);
    };

    extern CGuidNameList GuidNames;

#endif

#ifndef REMIND
    //  REMIND macro - generates warning as reminder to complete coding
    //  (eg) usage:
    //
    //  #pragma message (REMIND("Add automation support"))


    #define QUOTE(x) #x
    #define QQUOTE(y) QUOTE(y)
    #define REMIND(str) __FILE__ "(" QQUOTE(__LINE__) ") :  " str
#endif

//  Method to display objects in a useful format
//
//  eg If you want to display a LONGLONG ll in a debug string do (eg)
//
//  DbgLog((LOG_TRACE, n, TEXT("Value is %s"), (LPCTSTR)CDisp(ll, CDISP_HEX)));


class CDispBasic
{
public:
    CDispBasic() { m_pString = m_String; };
    ~CDispBasic();
protected:
    PTCHAR m_pString;  // normally points to m_String... unless too much data
    TCHAR m_String[50];
};
class CDisp : public CDispBasic
{
public:
    CDisp(LONGLONG ll, int Format = CDISP_HEX); // Display a LONGLONG in CDISP_HEX or CDISP_DEC form
    CDisp(REFCLSID clsid);      // Display a GUID
    CDisp(double d);            // Display a floating point number
#ifdef __strmif_h__
#ifdef __STREAMS__
    CDisp(CRefTime t);          // Display a Reference Time
#endif
    CDisp(IPin *pPin);          // Display a pin as {filter clsid}(pin name)
    CDisp(IUnknown *pUnk);      // Display a filter or pin
#endif // __strmif_h__
    ~CDisp();

    //  Implement cast to (LPCTSTR) as parameter to logger
    operator LPCTSTR()
    {
        return (LPCTSTR)m_pString;
    };
};


#if defined(DEBUG)
class CAutoTrace
{
private:
    LPCTSTR  _szBlkName;
    const int _level;
    static const TCHAR _szEntering[];
    static const TCHAR _szLeaving[];
public:
    CAutoTrace(LPCTSTR szBlkName, const int level = 15)
        : _szBlkName(szBlkName), _level(level)
    {DbgLog((LOG_TRACE, _level, _szEntering, _szBlkName));}

    ~CAutoTrace()
    {DbgLog((LOG_TRACE, _level, _szLeaving, _szBlkName));}
};

#if defined (__FUNCTION__)

#define AMTRACEFN()  CAutoTrace __trace(TEXT(__FUNCTION__))
#define AMTRACE(_x_) CAutoTrace __trace(TEXT(__FUNCTION__))

#else

#define AMTRACE(_x_) CAutoTrace __trace _x_
#define AMTRACEFN()

#endif

#else

#define AMTRACE(_x_)
#define AMTRACEFN()

#endif

#endif // __WXDEBUG__


