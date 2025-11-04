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

#include "NetWork/Network.h"
#include "URIUtils.h"
#include "filesystem/SpecialProtocol.h"
#include "filesystem/StackDirectory.h"
#include "URL.h"
#include "StringUtils.h"

#include <cassert>



#include <Winsock2.h>
#include <WS2tcpip.h>
#include <ws2bth.h>

#ifndef SHUT_RDWR
#define SHUT_RDWR SD_BOTH
#endif

#ifndef SHUT_RD
#define SHUT_RD SD_RECEIVE
#endif

#ifndef SHUT_WR
#define SHUT_WR SD_SEND
#endif


#ifndef AF_BTH
#define AF_BTH          32
#endif

#ifndef BTHPROTO_RFCOMM
#define BTHPROTO_RFCOMM 3
#endif

#ifndef AF_BLUETOOTH
#define AF_BLUETOOTH AF_BTH
#endif

#ifndef BTPROTO_RFCOMM
#define BTPROTO_RFCOMM BTHPROTO_RFCOMM
#endif

typedef int socklen_t;



#include <algorithm>

using namespace std;



const std::string URIUtils::GetFileName(const CURL& url)
{
  return GetFileName(url.GetFileName());
}

/* returns a filename given an url */
/* handles both / and \, and options in urls*/
const std::string URIUtils::GetFileName(const std::string& strFileNameAndPath)
{
  if(IsURL(strFileNameAndPath))
  {
    CURL url(strFileNameAndPath);
    return GetFileName(url.GetFileName());
  }

  /* find the last slash */
  const size_t slash = strFileNameAndPath.find_last_of("/\\");
  return strFileNameAndPath.substr(slash+1);
}

void URIUtils::Split(const std::string& strFileNameAndPath,
                     std::string& strPath, std::string& strFileName)
{
  //Splits a full filename in path and file.
  //ex. smb://computer/share/directory/filename.ext -> strPath:smb://computer/share/directory/ and strFileName:filename.ext
  //Trailing slash will be preserved
  strFileName = "";
  strPath = "";
  int i = strFileNameAndPath.size() - 1;
  while (i > 0)
  {
    char ch = strFileNameAndPath[i];
    // Only break on ':' if it's a drive separator for DOS (ie d:foo)
    if (ch == '/' || ch == '\\' || (ch == ':' && i == 1)) break;
    else i--;
  }
  if (i == 0)
    i--;

  // take left including the directory separator
  strPath = strFileNameAndPath.substr(0, i+1);
  // everything to the right of the directory separator
  strFileName = strFileNameAndPath.substr(i+1);
}


bool URIUtils::HasEncodedFilename(const CURL& url)
{
  const std::string prot2 = url.GetTranslatedProtocol();

  // For now assume only (quasi) http internet streams use URL encoding
  return CURL::IsProtocolEqual(prot2, "http")  ||
         CURL::IsProtocolEqual(prot2, "https");
}


std::string URLEncodePath(const std::string& strPath)
{
  vector<string> segments = StringUtils::Split(strPath, "/");
  for (vector<string>::iterator i = segments.begin(); i != segments.end(); ++i)
    *i = CURL::Encode(*i);

  return StringUtils::Join(segments, "/");
}

std::string URLDecodePath(const std::string& strPath)
{
  vector<string> segments = StringUtils::Split(strPath, "/");
  for (vector<string>::iterator i = segments.begin(); i != segments.end(); ++i)
    *i = CURL::Decode(*i);

  return StringUtils::Join(segments, "/");
}


bool URIUtils::IsProtocol(const std::string& url, const std::string &type)
{
  return StringUtils::StartsWithNoCase(url, type + "://");
}

static bool addr_match(uint32_t addr, const char* target, const char* submask)
{
  uint32_t addr2 = ntohl(inet_addr(target));
  uint32_t mask = ntohl(inet_addr(submask));
  return (addr & mask) == (addr2 & mask);
}

bool URIUtils::IsSpecial(const std::string& strFile)
{
  std::string strFile2(strFile);

  if (IsStack(strFile))
    strFile2 = XFILE::CStackDirectory::GetFirstStackedFile(strFile);

  return IsProtocol(strFile2, "special");
}

bool URIUtils::IsURL(const std::string& strFile)
{
  return strFile.find("://") != std::string::npos;
}

bool URIUtils::IsDOSPath(const std::string &path)
{
  if (path.size() > 1 && path[1] == ':' && isalpha(path[0]))
    return true;

  // windows network drives
  if (path.size() > 1 && path[0] == '\\' && path[1] == '\\')
    return true;

  return false;
}

void URIUtils::AddSlashAtEnd(std::string& strFolder)
{
  if (IsURL(strFolder))
  {
    CURL url(strFolder);
    std::string file = url.GetFileName();
    if(!file.empty() && file != strFolder)
    {
      AddSlashAtEnd(file);
      url.SetFileName(file);
      strFolder = url.Get();
    }
    return;
  }

  if (!HasSlashAtEnd(strFolder))
  {
    if (IsDOSPath(strFolder))
      strFolder += '\\';
    else
      strFolder += '/';
  }
}

bool URIUtils::HasSlashAtEnd(const std::string& strFile, bool checkURL /* = false */)
{
  if (strFile.empty()) return false;
  if (checkURL && IsURL(strFile))
  {
    CURL url(strFile);
    std::string file = url.GetFileName();
    return file.empty() || HasSlashAtEnd(file, false);
  }
  char kar = strFile.c_str()[strFile.size() - 1];

  if (kar == '/' || kar == '\\')
    return true;

  return false;
}

void URIUtils::RemoveSlashAtEnd(std::string& strFolder)
{
  if (IsURL(strFolder))
  {
    CURL url(strFolder);
    std::string file = url.GetFileName();
    if (!file.empty() && file != strFolder)
    {
      RemoveSlashAtEnd(file);
      url.SetFileName(file);
      strFolder = url.Get();
      return;
    }
    if(url.GetHostName().empty())
      return;
  }

  while (HasSlashAtEnd(strFolder))
    strFolder.erase(strFolder.size()-1, 1);
}


std::string URIUtils::FixSlashesAndDups(const std::string& path, const char slashCharacter /* = '/' */, const size_t startFrom /*= 0*/)
{
  const size_t len = path.length();
  if (startFrom >= len)
    return path;

  std::string result(path, 0, startFrom);
  result.reserve(len);

  const char* const str = path.c_str();
  size_t pos = startFrom;
  do
  {
    if (str[pos] == '\\' || str[pos] == '/')
    {
      result.push_back(slashCharacter);  // append one slash
      pos++;
      // skip any following slashes
      while (str[pos] == '\\' || str[pos] == '/') // str is null-terminated, no need to check for buffer overrun
        pos++;
    }
    else
      result.push_back(str[pos++]);   // append current char and advance pos to next char

  } while (pos < len);

  return result;
}

std::string URIUtils::CanonicalizePath(const std::string& path, const char slashCharacter /*= '\\'*/)
{
  assert(slashCharacter == '\\' || slashCharacter == '/');

  if (path.empty())
    return path;

  const std::string slashStr(1, slashCharacter);
  vector<std::string> pathVec, resultVec;
  StringUtils::Tokenize(path, pathVec, slashStr);

  for (vector<std::string>::const_iterator it = pathVec.begin(); it != pathVec.end(); ++it)
  {
    if (*it == ".")
    { /* skip - do nothing */ }
    else if (*it == ".." && !resultVec.empty() && resultVec.back() != "..")
      resultVec.pop_back();
    else
      resultVec.push_back(*it);
  }

  std::string result;
  if (path[0] == slashCharacter)
    result.push_back(slashCharacter); // add slash at the begin

  result += StringUtils::Join(resultVec, slashStr);

  if (path[path.length() - 1] == slashCharacter  && !result.empty() && result[result.length() - 1] != slashCharacter)
    result.push_back(slashCharacter); // add slash at the end if result isn't empty and result isn't "/"

  return result;
}

std::string URIUtils::AddFileToFolder(const std::string& strFolder, 
                                const std::string& strFile)
{
  if (IsURL(strFolder))
  {
    CURL url(strFolder);
    if (url.GetFileName() != strFolder)
    {
      url.SetFileName(AddFileToFolder(url.GetFileName(), strFile));
      return url.Get();
    }
  }

  std::string strResult = strFolder;
  if (!strResult.empty())
    AddSlashAtEnd(strResult);

  // Remove any slash at the start of the file
  if (strFile.size() && (strFile[0] == '/' || strFile[0] == '\\'))
    strResult += strFile.substr(1);
  else
    strResult += strFile;

  // correct any slash directions
  if (!IsDOSPath(strFolder))
    StringUtils::Replace(strResult, '\\', '/');
  else
    StringUtils::Replace(strResult, '/', '\\');

  return strResult;
}

std::string URIUtils::GetDirectory(const std::string &strFilePath)
{
  // Will from a full filename return the directory the file resides in.
  // Keeps the final slash at end and possible |option=foo options.

  size_t iPosSlash = strFilePath.find_last_of("/\\");
  if (iPosSlash == string::npos)
    return ""; // No slash, so no path (ignore any options)

  size_t iPosBar = strFilePath.rfind('|');
  if (iPosBar == string::npos)
    return strFilePath.substr(0, iPosSlash + 1); // Only path

  return strFilePath.substr(0, iPosSlash + 1) + strFilePath.substr(iPosBar); // Path + options
}





