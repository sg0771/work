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

#if (defined HAVE_CONFIG_H) && (!defined _WIN32)
  #include "config.h"
#endif
#include "../NetWork/Network.h"
#include "../utils/utils.h"

#include "FileFactory.h"
#ifdef TARGET_POSIX
#include "posix/PosixFile.h"
#elif defined(_WIN32)
#include "Win32File.h"
#endif // _WIN32
#include "CurlFile.h"
#include "HTTPFile.h"



#ifdef HAS_FILESYSTEM_RAR
#include "RarFile.h"
#endif
#ifdef HAS_FILESYSTEM_SFTP
#include "SFTPFile.h"
#endif

#if defined(TARGET_ANDROID)
#include "AndroidAppFile.h"
#endif
#ifdef HAS_UPNP
#include "UPnPFile.h"
#endif


#include "SpecialProtocolFile.h"

//#include "Application.h"
#include "../utils/URL.h"
//#include "../utils/log.h"
//#include "network/WakeOnAccess.h"

using namespace XFILE;

CFileFactory::CFileFactory()
{
}

CFileFactory::~CFileFactory()
{
}

IFile* CFileFactory::CreateLoader(const std::string& strFileName)
{
  CURL url(strFileName);
  return CreateLoader(url);
}

IFile* CFileFactory::CreateLoader(const CURL& url)
{
if (url.IsProtocol("special")) return new CSpecialProtocolFile();
if (url.IsProtocol("file") || url.GetProtocol().empty()) return new CWin32File();
  bool networkAvailable = true;//g_application.getNetwork().IsAvailable();
  if (networkAvailable)
  {
    if (url.IsProtocol("ftp")
    ||  url.IsProtocol("ftps")
    ||  url.IsProtocol("rss")) return new CCurlFile();
    else if (url.IsProtocol("http") ||  url.IsProtocol("https")) return new CHTTPFile();
  }
  return NULL;
}
