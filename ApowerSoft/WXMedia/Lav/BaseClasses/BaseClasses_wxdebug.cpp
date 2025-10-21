//------------------------------------------------------------------------------
// File: WXDebug.cpp
//
// Desc: DirectShow base classes - implements ActiveX system debugging
//       facilities.
//
// Copyright (c) 1992-2001 Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

#include "streams.h"
#include <stdarg.h>
#include <stdio.h>
#include <dvdmedia.h>
#include <algorithm>

#include <tchar.h>
#include <strsafe.h>

#ifdef _OBJBASE_H_

    /*  Stuff for printing out our GUID names */

    GUID_STRING_ENTRY g_GuidNames[] = {
    #define OUR_GUID_ENTRY(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    { #name, { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } } },
        #include <uuids.h>
    };

    CGuidNameList GuidNames;
    int g_cGuidNames = sizeof(g_GuidNames) / sizeof(g_GuidNames[0]);

    char *CGuidNameList::operator [] (const GUID &guid)
    {
        for (int i = 0; i < g_cGuidNames; i++) {
            if (g_GuidNames[i].guid == guid) {
                return g_GuidNames[i].szName;
            }
        }
        if (guid == GUID_NULL) {
            return "GUID_NULL";
        }

	// !!! add something to print FOURCC guids?
	
	// shouldn't this print the hex CLSID?
        return "Unknown GUID Name";
    }

#endif /* _OBJBASE_H_ */

/*  CDisp class - display our data types */

// clashes with REFERENCE_TIME
CDisp::CDisp(LONGLONG ll, int Format)
{
    // note: this could be combined with CDisp(LONGLONG) by
    // introducing a default format of CDISP_REFTIME
    LARGE_INTEGER li;
    li.QuadPart = ll;
    switch (Format) {
	case CDISP_DEC:
	{
	    TCHAR  temp[20];
	    int pos=20;
	    temp[--pos] = 0;
	    int digit;
	    // always output at least one digit
	    do {
		// Get the rightmost digit - we only need the low word
	        digit = li.LowPart % 10;
		li.QuadPart /= 10;
		temp[--pos] = (TCHAR) digit+L'0';
	    } while (li.QuadPart);
	    (void)StringCchCopy(m_String, NUMELMS(m_String), temp+pos);
	    break;
	}
	case CDISP_HEX:
	default:
	    (void)StringCchPrintf(m_String, NUMELMS(m_String), TEXT("0x%X%8.8X"), li.HighPart, li.LowPart);
    }
};

CDisp::CDisp(REFCLSID clsid)
{
#ifdef UNICODE 
    (void)StringFromGUID2(clsid, m_String, NUMELMS(m_String));
#else
    WCHAR wszTemp[50];
    (void)StringFromGUID2(clsid, wszTemp, NUMELMS(wszTemp));
    (void)StringCchPrintf(m_String, NUMELMS(m_String), TEXT("%S"), wszTemp);
#endif
};

#ifdef __STREAMS__
/*  Display stuff */
CDisp::CDisp(CRefTime llTime)
{
    LONGLONG llDiv;
    if (llTime < 0) {
        llTime = -llTime;
        (void)StringCchCopy(m_String, NUMELMS(m_String), TEXT("-"));
    }
    llDiv = (LONGLONG)24 * 3600 * 10000000;
    if (llTime >= llDiv) {
        (void)StringCchPrintf(m_String + lstrlen(m_String), NUMELMS(m_String) - lstrlen(m_String), TEXT("%d days "), (LONG)(llTime / llDiv));
        llTime = llTime % llDiv;
    }
    llDiv = (LONGLONG)3600 * 10000000;
    if (llTime >= llDiv) {
        (void)StringCchPrintf(m_String + lstrlen(m_String), NUMELMS(m_String) - lstrlen(m_String), TEXT("%d hrs "), (LONG)(llTime / llDiv));
        llTime = llTime % llDiv;
    }
    llDiv = (LONGLONG)60 * 10000000;
    if (llTime >= llDiv) {
        (void)StringCchPrintf(m_String + lstrlen(m_String), NUMELMS(m_String) - lstrlen(m_String), TEXT("%d mins "), (LONG)(llTime / llDiv));
        llTime = llTime % llDiv;
    }
    (void)StringCchPrintf(m_String + lstrlen(m_String), NUMELMS(m_String) - lstrlen(m_String), TEXT("%d.%3.3d sec"),
             (LONG)llTime / 10000000,
             (LONG)((llTime % 10000000) / 10000));
};

#endif // __STREAMS__


/*  Display pin */
CDisp::CDisp(IPin *pPin)
{
    PIN_INFO pi;
    TCHAR str[MAX_PIN_NAME];
    CLSID clsid;

    if (pPin) {
       pPin->QueryPinInfo(&pi);
       pi.pFilter->GetClassID(&clsid);
       QueryPinInfoReleaseFilter(pi);
      #ifndef UNICODE
       WideCharToMultiByte(GetACP(), 0, pi.achName, lstrlenW(pi.achName) + 1,
                           str, MAX_PIN_NAME, NULL, NULL);
      #else
       (void)StringCchCopy(str, NUMELMS(str), pi.achName);
      #endif
    } else {
       (void)StringCchCopy(str, NUMELMS(str), TEXT("NULL IPin"));
    }

    m_pString = (PTCHAR)new TCHAR[lstrlen(str)+64];
    if (!m_pString) {
	return;
    }

    (void)StringCchPrintf(m_pString, lstrlen(str) + 64, TEXT("%hs(%s)"), GuidNames[clsid], str);
}

/*  Display filter or pin */
CDisp::CDisp(IUnknown *pUnk)
{
    IBaseFilter *pf;
    HRESULT hr = pUnk->QueryInterface(IID_IBaseFilter, (void **)&pf);
    if(SUCCEEDED(hr))
    {
        FILTER_INFO fi;
        hr = pf->QueryFilterInfo(&fi);
        if(SUCCEEDED(hr))
        {
            QueryFilterInfoReleaseGraph(fi);

            size_t len = lstrlenW(fi.achName)  + 1;

            m_pString = new TCHAR[len];
            if(m_pString)
            {
#ifdef UNICODE
                (void)StringCchCopy(m_pString, len, fi.achName);
#else
                (void)StringCchPrintf(m_pString, len, "%S", fi.achName);
#endif
            }
        }

        pf->Release();

        return;
    }

    IPin *pp;
    hr = pUnk->QueryInterface(IID_IPin, (void **)&pp);
    if(SUCCEEDED(hr))
    {
        CDisp::CDisp(pp);
        pp->Release();
        return;
    }
}


CDisp::~CDisp()
{
}

CDispBasic::~CDispBasic()
{
    if (m_pString != m_String) {
	delete [] m_pString;
    }
}

CDisp::CDisp(double d)
{
    (void)StringCchPrintf(m_String, NUMELMS(m_String), TEXT("%d.%03d"), (int) d, (int) ((d - (int) d) * 1000));
}






