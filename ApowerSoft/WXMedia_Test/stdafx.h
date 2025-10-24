// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__846BA7EA_847B_11D6_B650_0020780D200B__INCLUDED_)
#define AFX_STDAFX_H__846BA7EA_847B_11D6_B650_0020780D200B__INCLUDED_

// Change these values to use different versions
// Change these values to use different versions
#define WINVER		0x0601
#define _WIN32_WINNT	0x0601
#define _WIN32_IE	0x0700
#define _RICHEDIT_VER	0x0500

#include <atlbase.h>
#include <atlapp.h>

extern CAppModule _Module;

#include <atlstr.h>
#include <atlcom.h>
#include <atlhost.h>
#include <atlwin.h>
#include <atlctl.h>
#include <atlframe.h>
#include <atlctrls.h>
#include <atldlgs.h>
#include <atlddx.h>
#include <atlctrlw.h>

#include "resource.h"

#ifndef __ATLMISC_H__
#include <atlmisc.h>
#endif

#ifndef __ATLCTRLS_H__
#include <atlctrls.h>
#endif

#ifndef __ATLCRACK_H__
#include <atlcrack.h>
#endif


#include <fstream>
#include <gdiplus.h>
#include <stdint.h>
#include <memory>
#include <vector>
#include <string>
#include <mutex>

#pragma comment(lib,"gdiplus.lib")

#pragma warning(disable: 4996)
#pragma warning(disable: 4244)
#pragma warning(disable: 4838)


#include <WXMedia/WXMedia.h>
//#include <WXMediaCpp.h>
#include <WXBase.h>
#include <FfmpegIncludes.h>
#include <WXLog.h>

enum { TIMER_ID = 1001 };

#ifndef CB_SETDROPPEDHEIGHT
#define CB_SETDROPPEDHEIGHT 0x0153
#endif

#pragma comment(lib, "WXMedia.lib")
#pragma comment(lib, "libffmpeg.lib")

#endif // !defined(AFX_STDAFX_H__846BA7EA_847B_11D6_B650_0020780D200B__INCLUDED_)
