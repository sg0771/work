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

#include "SpecialProtocol.h"
#include "../utils/URL.h"
#include "../utils/Win32Utils.h"
//#include "settings/Settings.h"
#include "../utils/URIUtils.h"
#include <cassert>
#ifdef TARGET_POSIX
#include <dirent.h>
#include "../utils/StringUtils.h"
#endif

////using namespace std;

std::map<std::string, std::string> CSpecialProtocol::m_pathMap;

void CSpecialProtocol::SetProfilePath(const std::string &dir)
{
  SetPath("profile", dir);
  ;// CLog::Log(LOGNOTICE, "special://profile/ is mapped to: %s", GetPath("profile").c_str());
}

void CSpecialProtocol::SetXBMCPath(const std::string &dir)
{
  SetPath("xbmc", dir);
}

void CSpecialProtocol::SetXBMCBinPath(const std::string &dir)
{
  SetPath("xbmcbin", dir);
}

void CSpecialProtocol::SetXBMCFrameworksPath(const std::string &dir)
{
  SetPath("frameworks", dir);
}

void CSpecialProtocol::SetHomePath(const std::string &dir)
{
  SetPath("home", dir);
}

void CSpecialProtocol::SetUserHomePath(const std::string &dir)
{
  SetPath("userhome", dir);
}

void CSpecialProtocol::SetMasterProfilePath(const std::string &dir)
{
  SetPath("masterprofile", dir);
}

void CSpecialProtocol::SetTempPath(const std::string &dir)
{
  SetPath("temp", dir);
}

bool CSpecialProtocol::ComparePath(const std::string &path1, const std::string &path2)
{
  return TranslatePath(path1) == TranslatePath(path2);
}

std::string CSpecialProtocol::TranslatePath(const std::string &path)
{
  CURL url(path);
  // check for special-protocol, if not, return
  if (!url.IsProtocol("special"))
  {
    return path;
  }
  return TranslatePath(url);
}

std::string CSpecialProtocol::TranslatePath(const CURL &url)
{
  // check for special-protocol, if not, return
  if (!url.IsProtocol("special"))
  {
#if defined(TARGET_POSIX) && defined(_DEBUG)
    std::string path(url.Get());
    if (path.length() >= 2 && path[1] == ':')
    {
      ;// CLog::Log(LOGWARNING, "Trying to access old style dir: %s\n", path.c_str());
     // printf("Trying to access old style dir: %s\n", path.c_str());
    }
#endif

    return url.Get();
  }

  std::string FullFileName = url.GetFileName();

  std::string translatedPath;
  std::string FileName;
  std::string RootDir;

  // Split up into the special://root and the rest of the filename
  size_t pos = FullFileName.find('/');
  if (pos != std::string::npos && pos > 1)
  {
    RootDir = FullFileName.substr(0, pos);

    if (pos < FullFileName.size())
      FileName = FullFileName.substr(pos + 1);
  }
  else
    RootDir = FullFileName;

  // from here on, we have our "real" special paths
  if (RootDir == "xbmc" ||
           RootDir == "xbmcbin" ||
           RootDir == "home" ||
           RootDir == "userhome" ||
           RootDir == "temp" ||
           RootDir == "profile" ||
           RootDir == "masterprofile" ||
           RootDir == "frameworks")
  {
    std::string basePath = GetPath(RootDir);
    if (!basePath.empty())
      translatedPath = URIUtils::AddFileToFolder(basePath, FileName);
    else
      translatedPath.clear();
  }

  // check if we need to recurse in
  if (URIUtils::IsSpecial(translatedPath))
  { // we need to recurse in, as there may be multiple translations required
    return TranslatePath(translatedPath);
  }

  // Validate the final path, just in case
  return CUtil::ValidatePath(translatedPath);
}

// private routines, to ensure we only set/get an appropriate path
void CSpecialProtocol::SetPath(const std::string &key, const std::string &path)
{
  m_pathMap[key] = path;
}

std::string CSpecialProtocol::GetPath(const std::string &key)
{
    std::map<std::string, std::string>::iterator it = m_pathMap.find(key);
  if (it != m_pathMap.end())
    return it->second;
  assert(false);
  return "";
}
