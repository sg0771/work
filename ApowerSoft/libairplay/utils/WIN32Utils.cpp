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
#include <Windows.h>
#include <shellapi.h>
#include <winioctl.h>
#include "WIN32Utils.h"
#include "../utils/URIUtils.h"
#include "PowrProf.h"
#include <shlobj.h>
#include "../utils/SpecialProtocol.h"


 //** Defines taken from ntddscsi.h in MS Windows DDK CD
#define SCSI_IOCTL_DATA_OUT             0 //Give data to SCSI device (e.g. for writing)
#define SCSI_IOCTL_DATA_IN              1 //Get data from SCSI device (e.g. for reading)
#define SCSI_IOCTL_DATA_UNSPECIFIED     2 //No data (e.g. for ejecting)

#define MAX_SENSE_LEN 18 //Sense data max length

#define IOCTL_SCSI_PASS_THROUGH         0x4D004
typedef struct _SCSI_PASS_THROUGH {
    USHORT Length;
    UCHAR ScsiStatus;
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    UCHAR CdbLength;
    UCHAR SenseInfoLength;
    UCHAR DataIn;
    ULONG DataTransferLength;
    ULONG TimeOutValue;
    ULONG_PTR DataBufferOffset;
    ULONG SenseInfoOffset;
    UCHAR Cdb[16];
}SCSI_PASS_THROUGH, * PSCSI_PASS_THROUGH;

#define IOCTL_SCSI_PASS_THROUGH_DIRECT  0x4D014
typedef struct _SCSI_PASS_THROUGH_DIRECT {
    USHORT Length;
    UCHAR ScsiStatus;
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    UCHAR CdbLength;
    UCHAR SenseInfoLength;
    UCHAR DataIn;
    ULONG DataTransferLength;
    ULONG TimeOutValue;
    PVOID DataBuffer;
    ULONG SenseInfoOffset;
    UCHAR Cdb[16];
}SCSI_PASS_THROUGH_DIRECT, * PSCSI_PASS_THROUGH_DIRECT;
//** End of defines taken from ntddscsi.h from MS Windows DDK CD

typedef struct _SCSI_PASS_THROUGH_AND_BUFFERS {
    SCSI_PASS_THROUGH spt;
    BYTE DataBuffer[64 * 1024];
}T_SPT_BUFS;

typedef struct _SCSI_PASS_THROUGH_DIRECT_AND_SENSE_BUFFER {
    SCSI_PASS_THROUGH_DIRECT sptd;
    UCHAR SenseBuf[MAX_SENSE_LEN];
}T_SPDT_SBUF;


#include "Setupapi.h"
#include "../utils/StringUtils.h"
#include <cassert>

#define DLL_ENV_PATH "special://xbmc/system/;" \
                     "special://xbmc/system/players/dvdplayer/;" \
                     "special://xbmc/system/players/paplayer/;" \
                     "special://xbmc/system/cdrip/;" \
                     "special://xbmc/system/python/;" \
                     "special://xbmc/system/webserver/;" \
                     "special://xbmc/"

#include <locale.h>

extern HWND g_hWnd;

using namespace std;

std::string CWIN32Util::ConvertPathToWin32Form(const std::string& pathUtf8)
{
  std::string result;
  if (pathUtf8.empty())
    return result;

  if (pathUtf8.compare(0, 2, "\\\\", 2) != 0) // pathUtf8 don't start from "\\"
  { // assume local file path in form 'C:\Folder\File.ext'
    std::string formedPath("\\\\?\\"); // insert "\\?\" prefix
    formedPath += URIUtils::CanonicalizePath(URIUtils::FixSlashesAndDups(pathUtf8, '\\'), '\\'); // fix duplicated and forward slashes, resolve relative path
    //convertResult = g_charsetConverter.utf8ToW(formedPath, result, false, false, true);
  }
  else if (pathUtf8.compare(0, 8, "\\\\?\\UNC\\", 8) == 0) // pathUtf8 starts from "\\?\UNC\"
  {
    std::string formedPath("\\\\?\\UNC"); // start from "\\?\UNC" prefix
    formedPath += URIUtils::CanonicalizePath(URIUtils::FixSlashesAndDups(pathUtf8.substr(7), '\\'), '\\'); // fix duplicated and forward slashes, resolve relative path, don't touch "\\?\UNC" prefix,
    //convertResult = g_charsetConverter.utf8ToW(formedPath, result, false, false, true); 
  }
  else if (pathUtf8.compare(0, 4, "\\\\?\\", 4) == 0) // pathUtf8 starts from "\\?\", but it's not UNC path
  {
    std::string formedPath("\\\\?"); // start from "\\?" prefix
    formedPath += URIUtils::CanonicalizePath(URIUtils::FixSlashesAndDups(pathUtf8.substr(3), '\\'), '\\'); // fix duplicated and forward slashes, resolve relative path, don't touch "\\?" prefix,
    //convertResult = g_charsetConverter.utf8ToW(formedPath, result, false, false, true);
  }
  else // pathUtf8 starts from "\\", but not from "\\?\UNC\"
  { // assume UNC path in form '\\server\share\folder\file.ext'
    std::string formedPath("\\\\?\\UNC"); // append "\\?\UNC" prefix
    formedPath += URIUtils::CanonicalizePath(URIUtils::FixSlashesAndDups(pathUtf8), '\\'); // fix duplicated and forward slashes, resolve relative path, transform "\\" prefix to single "\"
    //convertResult = g_charsetConverter.utf8ToW(formedPath, result, false, false, true);
  }



  return result;
}

std::string CWIN32Util::ConvertPathToWin32Form(const CURL& url)
{
  assert(url.GetProtocol().empty() || url.IsProtocol("smb"));

  if (url.GetFileName().empty())
    return std::string(); // empty string

  if (url.GetProtocol().empty())
  {
	  std::string result;
	  result = "\\\\?\\" + URIUtils::CanonicalizePath(URIUtils::FixSlashesAndDups(url.GetFileName(), '\\'), '\\');
	  return result;
  }
  //else if (url.IsProtocol("smb"))
  //{
  //  if (url.GetHostName().empty())
  //    return std::wstring(); // empty string
  //  
  //  std::wstring result;
  //  if (g_charsetConverter.utf8ToW("\\\\?\\UNC\\" +
  //        URIUtils::CanonicalizePath(URIUtils::FixSlashesAndDups(url.GetHostName() + '\\' + url.GetFileName(), '\\'), '\\'),
  //        result, false, false, true))
  //    return result;
  //}
  else
    return std::string(); // unsupported protocol, return empty string

  ;// CLog::Log(LOGERROR, "%s: Error converting path \"%s\" to Win32 form", __FUNCTION__, url.Get().c_str());
  return std::string(); // empty string
}


extern "C"
{
  FILE *fopen_utf8(const char *_Filename, const char *_Mode)
  {
    std::string modetmp = _Mode;
    std::string wfilename, wmode(modetmp.begin(), modetmp.end());
    //g_charsetConverter.utf8ToW(_Filename, wfilename, false);
    return fopen(wfilename.c_str(), wmode.c_str());
  }
}

extern "C" {
  /*
   * Ported from NetBSD to Windows by Ron Koenderink, 2007
   */

  /*  $NetBSD: strptime.c,v 1.25 2005/11/29 03:12:00 christos Exp $  */

  /*-
   * Copyright (c) 1997, 1998, 2005 The NetBSD Foundation, Inc.
   * All rights reserved.
   *
   * This code was contributed to The NetBSD Foundation by Klaus Klein.
   * Heavily optimised by David Laight
   *
   * Redistribution and use in source and binary forms, with or without
   * modification, are permitted provided that the following conditions
   * are met:
   * 1. Redistributions of source code must retain the above copyright
   *    notice, this list of conditions and the following disclaimer.
   * 2. Redistributions in binary form must reproduce the above copyright
   *    notice, this list of conditions and the following disclaimer in the
   *    documentation and/or other materials provided with the distribution.
   * 3. All advertising materials mentioning features or use of this software
   *    must display the following acknowledgement:
   *        This product includes software developed by the NetBSD
   *        Foundation, Inc. and its contributors.
   * 4. Neither the name of The NetBSD Foundation nor the names of its
   *    contributors may be used to endorse or promote products derived
   *    from this software without specific prior written permission.
   *
   * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
   * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
   * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
   * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   * POSSIBILITY OF SUCH DAMAGE.
   */

  #if !defined(_WIN32)
  #include <sys/cdefs.h>
  #endif

  #if defined(LIBC_SCCS) && !defined(lint)
  __RCSID("$NetBSD: strptime.c,v 1.25 2005/11/29 03:12:00 christos Exp $");
  #endif

  #if !defined(_WIN32)
  #include "namespace.h"
  #include <sys/localedef.h>
  #else
  typedef unsigned char u_char;
  typedef unsigned int uint;
  #endif
  #include <ctype.h>
  #include <locale.h>
  #include <string.h>
  #include <time.h>
  #if !defined(_WIN32)
  #include <tzfile.h>
  #endif

  #ifdef __weak_alias
  __weak_alias(strptime,_strptime)
  #endif

  #if !defined(_WIN32)
  #define  _ctloc(x)    (_CurrentTimeLocale->x)
  #else
  #define _ctloc(x)   (x)
  const char *abday[] = {
    "Sun", "Mon", "Tue", "Wed",
    "Thu", "Fri", "Sat"
  };
  const char *day[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday",
    "Thursday", "Friday", "Saturday"
  };
  const char *abmon[] =  {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };
  const char *mon[] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
  };
  const char *am_pm[] = {
    "AM", "PM"
  };
  char *d_t_fmt = (char*)"%a %Ef %T %Y";
  char *t_fmt_ampm = (char*)"%I:%M:%S %p";
  char *t_fmt = (char*)"%H:%M:%S";
  char *d_fmt = (char*)"%m/%d/%y";
  #define TM_YEAR_BASE 1900
  #define __UNCONST(x) ((char *)(((const char *)(x) - (const char *)0) + (char *)0))

  #endif
  /*
   * We do not implement alternate representations. However, we always
   * check whether a given modifier is allowed for a certain conversion.
   */
  #define ALT_E      0x01
  #define ALT_O      0x02
  #define  LEGAL_ALT(x)    { if (alt_format & ~(x)) return NULL; }


  static const u_char *conv_num(const unsigned char *, int *, uint, uint);
  static const u_char *find_string(const u_char *, int *, const char * const *,
    const char * const *, int);


  char *
  strptime(const char *buf, const char *fmt, struct tm *tm)
  {
    unsigned char c;
    const unsigned char *bp;
    int alt_format, i, split_year = 0;
    const char *new_fmt;

    bp = (const u_char *)buf;

    while (bp != NULL && (c = *fmt++) != '\0') {
      /* Clear `alternate' modifier prior to new conversion. */
      alt_format = 0;
      i = 0;

      /* Eat up white-space. */
      if (isspace(c)) {
        while (isspace(*bp))
          bp++;
        continue;
      }

      if (c != '%')
        goto literal;


  again:    switch (c = *fmt++) {
      case '%':  /* "%%" is converted to "%". */
  literal:
        if (c != *bp++)
          return NULL;
        LEGAL_ALT(0);
        continue;

      /*
       * "Alternative" modifiers. Just set the appropriate flag
       * and start over again.
       */
      case 'E':  /* "%E?" alternative conversion modifier. */
        LEGAL_ALT(0);
        alt_format |= ALT_E;
        goto again;

      case 'O':  /* "%O?" alternative conversion modifier. */
        LEGAL_ALT(0);
        alt_format |= ALT_O;
        goto again;

      /*
       * "Complex" conversion rules, implemented through recursion.
       */
      case 'c':  /* Date and time, using the locale's format. */
        new_fmt = _ctloc(d_t_fmt);
        goto recurse;

      case 'D':  /* The date as "%m/%d/%y". */
        new_fmt = "%m/%d/%y";
        LEGAL_ALT(0);
        goto recurse;

      case 'R':  /* The time as "%H:%M". */
        new_fmt = "%H:%M";
        LEGAL_ALT(0);
        goto recurse;

      case 'r':  /* The time in 12-hour clock representation. */
        new_fmt =_ctloc(t_fmt_ampm);
        LEGAL_ALT(0);
        goto recurse;

      case 'T':  /* The time as "%H:%M:%S". */
        new_fmt = "%H:%M:%S";
        LEGAL_ALT(0);
        goto recurse;

      case 'X':  /* The time, using the locale's format. */
        new_fmt =_ctloc(t_fmt);
        goto recurse;

      case 'x':  /* The date, using the locale's format. */
        new_fmt =_ctloc(d_fmt);
          recurse:
        bp = (const u_char *)strptime((const char *)bp,
                    new_fmt, tm);
        LEGAL_ALT(ALT_E);
        continue;

      /*
       * "Elementary" conversion rules.
       */
      case 'A':  /* The day of week, using the locale's form. */
      case 'a':
        bp = find_string(bp, &tm->tm_wday, _ctloc(day),
            _ctloc(abday), 7);
        LEGAL_ALT(0);
        continue;

      case 'B':  /* The month, using the locale's form. */
      case 'b':
      case 'h':
        bp = find_string(bp, &tm->tm_mon, _ctloc(mon),
            _ctloc(abmon), 12);
        LEGAL_ALT(0);
        continue;

      case 'C':  /* The century number. */
        i = 20;
        bp = conv_num(bp, &i, 0, 99);

        i = i * 100 - TM_YEAR_BASE;
        if (split_year)
          i += tm->tm_year % 100;
        split_year = 1;
        tm->tm_year = i;
        LEGAL_ALT(ALT_E);
        continue;

      case 'd':  /* The day of month. */
      case 'e':
        bp = conv_num(bp, &tm->tm_mday, 1, 31);
        LEGAL_ALT(ALT_O);
        continue;

      case 'k':  /* The hour (24-hour clock representation). */
        LEGAL_ALT(0);
        /* FALLTHROUGH */
      case 'H':
        bp = conv_num(bp, &tm->tm_hour, 0, 23);
        LEGAL_ALT(ALT_O);
        continue;

      case 'l':  /* The hour (12-hour clock representation). */
        LEGAL_ALT(0);
        /* FALLTHROUGH */
      case 'I':
        bp = conv_num(bp, &tm->tm_hour, 1, 12);
        if (tm->tm_hour == 12)
          tm->tm_hour = 0;
        LEGAL_ALT(ALT_O);
        continue;

      case 'j':  /* The day of year. */
        i = 1;
        bp = conv_num(bp, &i, 1, 366);
        tm->tm_yday = i - 1;
        LEGAL_ALT(0);
        continue;

      case 'M':  /* The minute. */
        bp = conv_num(bp, &tm->tm_min, 0, 59);
        LEGAL_ALT(ALT_O);
        continue;

      case 'm':  /* The month. */
        i = 1;
        bp = conv_num(bp, &i, 1, 12);
        tm->tm_mon = i - 1;
        LEGAL_ALT(ALT_O);
        continue;

      case 'p':  /* The locale's equivalent of AM/PM. */
        bp = find_string(bp, &i, _ctloc(am_pm), NULL, 2);
        if (tm->tm_hour > 11)
          return NULL;
        tm->tm_hour += i * 12;
        LEGAL_ALT(0);
        continue;

      case 'S':  /* The seconds. */
        bp = conv_num(bp, &tm->tm_sec, 0, 61);
        LEGAL_ALT(ALT_O);
        continue;

      case 'U':  /* The week of year, beginning on sunday. */
      case 'W':  /* The week of year, beginning on monday. */
        /*
         * XXX This is bogus, as we can not assume any valid
         * information present in the tm structure at this
         * point to calculate a real value, so just check the
         * range for now.
         */
         bp = conv_num(bp, &i, 0, 53);
         LEGAL_ALT(ALT_O);
         continue;

      case 'w':  /* The day of week, beginning on sunday. */
        bp = conv_num(bp, &tm->tm_wday, 0, 6);
        LEGAL_ALT(ALT_O);
        continue;

      case 'Y':  /* The year. */
        i = TM_YEAR_BASE;  /* just for data sanity... */
        bp = conv_num(bp, &i, 0, 9999);
        tm->tm_year = i - TM_YEAR_BASE;
        LEGAL_ALT(ALT_E);
        continue;

      case 'y':  /* The year within 100 years of the epoch. */
        /* LEGAL_ALT(ALT_E | ALT_O); */
        bp = conv_num(bp, &i, 0, 99);

        if (split_year)
          /* preserve century */
          i += (tm->tm_year / 100) * 100;
        else {
          split_year = 1;
          if (i <= 68)
            i = i + 2000 - TM_YEAR_BASE;
          else
            i = i + 1900 - TM_YEAR_BASE;
        }
        tm->tm_year = i;
        continue;

      /*
       * Miscellaneous conversions.
       */
      case 'n':  /* Any kind of white-space. */
      case 't':
        while (isspace(*bp))
          bp++;
        LEGAL_ALT(0);
        continue;


      default:  /* Unknown/unsupported conversion. */
        return NULL;
      }
    }

    return __UNCONST(bp);
  }


  static const u_char *
  conv_num(const unsigned char *buf, int *dest, uint llim, uint ulim)
  {
    uint result = 0;
    unsigned char ch;

    /* The limit also determines the number of valid digits. */
    uint rulim = ulim;

    ch = *buf;
    if (ch < '0' || ch > '9')
      return NULL;

    do {
      result *= 10;
      result += ch - '0';
      rulim /= 10;
      ch = *++buf;
    } while ((result * 10 <= ulim) && rulim && ch >= '0' && ch <= '9');

    if (result < llim || result > ulim)
      return NULL;

    *dest = result;
    return buf;
  }

  static const u_char *
  find_string(const u_char *bp, int *tgt, const char * const *n1,
      const char * const *n2, int c)
  {
    int i;
    unsigned int len;

    /* check full name - then abbreviated ones */
    for (; n1 != NULL; n1 = n2, n2 = NULL) {
      for (i = 0; i < c; i++, n1++) {
        len = strlen(*n1);
        if (strnicmp(*n1, (const char *)bp, len) == 0) {
          *tgt = i;
          return bp + len;
        }
      }
    }

    /* Nothing matched */
    return NULL;
  }
}

extern "C"
{
  /* case-independent string matching, similar to strstr but
  * matching */
  char * strcasestr(const char* haystack, const char* needle)
  {
    int i;
    int nlength = (int) strlen (needle);
    int hlength = (int) strlen (haystack);

    if (nlength > hlength) return NULL;
    if (hlength <= 0) return NULL;
    if (nlength <= 0) return (char *)haystack;
    /* hlength and nlength > 0, nlength <= hlength */
    for (i = 0; i <= (hlength - nlength); i++)
    {
      if (strnicmp (haystack + i, needle, nlength) == 0)
      {
        return (char *)haystack + i;
      }
    }
    /* substring not found */
    return NULL;
  }
}




#include "XBDateTime.h"
#include "../utils/thread.h"

#if   defined(TARGET_DARWIN)
#include <mach/mach_time.h>
#include <CoreVideo/CVHostTime.h>
#elif defined(_WIN32)
#include <windows.h>
#else
#include <time.h>
#endif

int64_t CurrentHostCounter(void)
{
#if   defined(TARGET_DARWIN)
    return((int64_t)CVGetCurrentHostTime());
#elif defined(_WIN32)
    LARGE_INTEGER PerformanceCount;
    QueryPerformanceCounter(&PerformanceCount);
    return((int64_t)PerformanceCount.QuadPart);
#else
    struct timespec now;
#ifdef CLOCK_MONOTONIC_RAW
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
#else
    clock_gettime(CLOCK_MONOTONIC, &now);
#endif // CLOCK_MONOTONIC_RAW
    return(((int64_t)now.tv_sec * 1000000000L) + now.tv_nsec);
#endif
}

int64_t CurrentHostFrequency(void)
{
#if defined(TARGET_DARWIN)
    return((int64_t)CVGetHostClockFrequency());
#elif defined(_WIN32)
    LARGE_INTEGER Frequency;
    QueryPerformanceFrequency(&Frequency);
    return((int64_t)Frequency.QuadPart);
#else
    return((int64_t)1000000000L);
#endif
}


#include <algorithm>
#include "../utils/utils.h"

#include "../utils/WIN32Utils.h"
#include "../NetWork/Network.h"
#include "../utils/thread.h"

#if defined(TARGET_DARWIN)
#include <sys/param.h>
#include <mach-o/dyld.h>
#endif

#if defined(TARGET_FREEBSD)
#include <sys/param.h>
#include <sys/sysctl.h>
#endif

#ifdef TARGET_POSIX
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/wait.h>
#endif
#if defined(TARGET_ANDROID)
#include "android/bionic_supplement/bionic_supplement.h"
#endif
#include <stdlib.h>

//#include "Application.h"


#include "../utils/StackDirectory.h"
#include "../utils/SpecialProtocol.h"
#ifdef HAS_FILESYSTEM_RAR
#include "filesystem/RarManager.h"
#endif
#ifdef HAS_UPNP
#include "filesystem/UPnPDirectory.h"
#endif
#if defined(TARGET_DARWIN)
#include "CompileInfo.h"
#include "osx/DarwinUtils.h"
#endif
#include "../utils/File.h"
#include "../utils/StringUtils.h"
#include "../utils/md5.h"
#include "../utils/Win32Utils.h"
#include "../utils/URIUtils.h"
#include "URL.h"

using namespace std;
using namespace XFILE;




std::string CUtil::ValidatePath(const std::string& path, bool bFixDoubleSlashes /* = false */)
{
    std::string result = path;

    // Don't do any stuff on URLs containing %-characters or protocols that embed
    // filenames. NOTE: Don't use IsInZip or IsInRar here since it will infinitely
    // recurse and crash XBMC
    if (URIUtils::IsURL(path) &&
        (path.find('%') != std::string::npos ||
            StringUtils::StartsWithNoCase(path, "apk:") ||
            StringUtils::StartsWithNoCase(path, "zip:") ||
            StringUtils::StartsWithNoCase(path, "rar:") ||
            StringUtils::StartsWithNoCase(path, "stack:") ||
            StringUtils::StartsWithNoCase(path, "bluray:") ||
            StringUtils::StartsWithNoCase(path, "multipath:")))
        return result;

    // check the path for incorrect slashes
#ifdef _WIN32
    if (URIUtils::IsDOSPath(path))
    {
        StringUtils::Replace(result, '/', '\\');
        /* The double slash correction should only be used when *absolutely*
           necessary! This applies to certain DLLs or use from Python DLLs/scripts
           that incorrectly generate double (back) slashes.
        */
        if (bFixDoubleSlashes && !result.empty())
        {
            // Fixup for double back slashes (but ignore the \\ of unc-paths)
            for (size_t x = 1; x < result.size() - 1; x++)
            {
                if (result[x] == '\\' && result[x + 1] == '\\')
                    result.erase(x);
            }
        }
    }
    else if (path.find("://") != std::string::npos || path.find(":\\\\") != std::string::npos)
#endif
    {
        StringUtils::Replace(result, '\\', '/');
        /* The double slash correction should only be used when *absolutely*
           necessary! This applies to certain DLLs or use from Python DLLs/scripts
           that incorrectly generate double (back) slashes.
        */
        if (bFixDoubleSlashes && !result.empty())
        {
            // Fixup for double forward slashes(/) but don't touch the :// of URLs
            for (size_t x = 2; x < result.size() - 1; x++)
            {
                if (result[x] == '/' && result[x + 1] == '/' && !(result[x - 1] == ':' || (result[x - 1] == '/' && result[x - 2] == ':')))
                    result.erase(x);
            }
        }
    }
    return result;
}


#include <string.h>

static int similar_text(const char* str1, const char* str2, int len1, int len2)
{
    int sum;
    int pos1 = 0, pos2 = 0;
    int max = 0;

    char* p, * q;
    char* end1 = (char*)str1 + len1;
    char* end2 = (char*)str2 + len2;
    int l;

    for (p = (char*)str1; p < end1; p++)
    {
        for (q = (char*)str2; q < end2; q++)
        {
            for (l = 0; (p + l < end1) && (q + l < end2) && (p[l] == q[l]); l++)
                ;
            if (l > max)
            {
                max = l;
                pos1 = p - str1;
                pos2 = q - str2;
            }
        }
    }
    if ((sum = max))
    {
        if (pos1 && pos2)
            sum += similar_text(str1, str2, pos1, pos2);

        if ((pos1 + max < len1) && (pos2 + max < len2))
            sum += similar_text(str1 + pos1 + max, str2 + pos2 + max,
                len1 - pos1 - max, len2 - pos2 - max);
    }

    return sum;
}


double  fstrcmp(const char* string1, const char* string2, double minimum)
{
    int len1, len2, score;

    len1 = (int)strlen(string1);
    len2 = (int)strlen(string2);

    /* short-circuit obvious comparisons */
    if (len1 == 0 && len2 == 0)
        return 1.0;
    if (len1 == 0 || len2 == 0)
        return 0.0;

    score = similar_text(string1, string2, len1, len2);
    /* The result is
    ((number of chars in common) / (average length of the strings)).
       This is admittedly biased towards finding that the strings are
       similar, however it does produce meaningful results.  */
    return ((double)score * 2.0 / (len1 + len2));
}


