/*
*      Copyright (C) 2014 Team XBMC
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
#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif // WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <sys/stat.h>

#include <intsafe.h>
#include <wchar.h>
#include <cassert>
#include <stdlib.h>
#include "Win32File.h"
#include "../utils/Win32Utils.h"
#include "auto_buffer.h"

using namespace XFILE;


CWin32File::CWin32File() : m_smbFile(false)
{
  m_hFile = INVALID_HANDLE_VALUE;
  m_filePos = -1;
  m_allowWrite = false;
  m_lastSMBFileErr = ERROR_SUCCESS;
}

CWin32File::CWin32File(bool asSmbFile) : m_smbFile(asSmbFile)
{
  m_hFile = INVALID_HANDLE_VALUE;
  m_filePos = -1;
  m_allowWrite = false;
  m_lastSMBFileErr = ERROR_SUCCESS;
}


CWin32File::~CWin32File()
{
  if (m_hFile != INVALID_HANDLE_VALUE)
    CloseHandle(m_hFile);
}

bool CWin32File::Open(const CURL& url)
{
  assert((!m_smbFile && url.GetProtocol().empty()) || (m_smbFile && url.IsProtocol("smb"))); // function suitable only for local or SMB files
  if (m_smbFile)
    m_lastSMBFileErr = ERROR_INVALID_DATA; // used to indicate internal errors, cleared by successful file operation

  if (m_hFile != INVALID_HANDLE_VALUE)
  {
    //CLog::LogF(LOGERROR, "Attempt to open file without closing opened file object first");
    return false;
  }

  std::string pathnameW(CWIN32Util::ConvertPathToWin32Form(url));
  if (pathnameW.length() <= 6) // 6 is length of "\\?\x:"
    return false; // pathnameW is empty or points to device ("\\?\x:")

  assert((pathnameW.compare(4, 4, "UNC\\", 4) == 0 && m_smbFile) || !m_smbFile);

  m_filepathnameW = pathnameW;
  m_hFile = CreateFileA(pathnameW.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (m_smbFile)
    m_lastSMBFileErr = GetLastError(); // set real error state

  return m_hFile != INVALID_HANDLE_VALUE;
}

bool CWin32File::OpenForWrite(const CURL& url, bool bOverWrite /*= false*/)
{
  assert((!m_smbFile && url.GetProtocol().empty()) || (m_smbFile && url.IsProtocol("smb"))); // function suitable only for local or SMB files
  if (m_smbFile)
    m_lastSMBFileErr = ERROR_INVALID_DATA; // used to indicate internal errors, cleared by successful file operation

  if (m_hFile != INVALID_HANDLE_VALUE)
    return false;

  std::string pathnameW(CWIN32Util::ConvertPathToWin32Form(url));
  if (pathnameW.length() <= 6) // 6 is length of "\\?\x:"
    return false; // pathnameW is empty or points to device ("\\?\x:")

  assert((pathnameW.compare(4, 4, "UNC\\", 4) == 0 && m_smbFile) || !m_smbFile);

  m_hFile = CreateFileA(pathnameW.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
                        NULL, bOverWrite ? CREATE_ALWAYS : OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

  if (m_smbFile)
    m_lastSMBFileErr = GetLastError(); // set real error state

  if (m_hFile == INVALID_HANDLE_VALUE)
    return false;
    
  const bool newlyCreated = (GetLastError() != ERROR_ALREADY_EXISTS);
  m_filepathnameW = pathnameW;

  if (!newlyCreated)
  {
    if (Seek(0, SEEK_SET) != 0)
    {
     // CLog::LogF(LOGERROR, "Can't move i/o pointer");
      Close();
      if (m_smbFile)
        m_lastSMBFileErr = ERROR_INVALID_DATA; // indicate internal errors
      return false;
    }
  }
  else
  { // newly created file
    /* set "hidden" attribute if filename starts with a period */
    size_t lastSlash = pathnameW.rfind(L'\\');
    if (lastSlash < pathnameW.length() - 1 // is two checks in one: lastSlash != std::string::npos && lastSlash + 1 < pathnameW.length()
        && pathnameW[lastSlash + 1] == L'.')
    {
      //FILE_BASIC_INFO basicInfo;
      bool hiddenSet = false;

	  BY_HANDLE_FILE_INFORMATION basicInfo;
	  if (GetFileInformationByHandle(m_hFile, &basicInfo) != 0)
      //if (GetFileInformationByHandleEx(m_hFile, FileBasicInfo, &basicInfo, sizeof(basicInfo)) != 0)
      {
		  if ((basicInfo.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0)
          hiddenSet = true;
        else
        {
			basicInfo.dwFileAttributes |= FILE_ATTRIBUTE_HIDDEN;
          //hiddenSet = SetFileInformationByHandle(m_hFile, FileBasicInfo, &basicInfo, sizeof(basicInfo)) != 0;
        }
      }
    }
  }

  m_allowWrite = true;

  return true;
}

void CWin32File::Close()
{
  if (m_hFile != INVALID_HANDLE_VALUE)
    CloseHandle(m_hFile);
  
  m_hFile = INVALID_HANDLE_VALUE;
  m_filePos = -1;
  m_allowWrite = false;
  m_lastSMBFileErr = ERROR_SUCCESS;
  m_filepathnameW.clear();
}

size_t CWin32File::Read(void* lpBuf, size_t uiBufSize)
{


  size_t read = 0;
  return read;
}

size_t CWin32File::Write(const void* lpBuf, size_t uiBufSize)
{
  if (m_hFile == INVALID_HANDLE_VALUE)
    return -1;

  assert(lpBuf != NULL || uiBufSize == 0);
  if (lpBuf == NULL && uiBufSize != 0)
    return -1;

  if (!m_allowWrite)
  {
    //CLog::LogF(LOGERROR, "Attempt to write file opened for reading");
    return -1;
  }

  if (uiBufSize == 0)
  { // allow "test" write with zero size
    auto_buffer dummyBuf(255);
    dummyBuf.get()[0] = 0;
    DWORD bytesWritten = 0;
    if (!WriteFile(m_hFile, dummyBuf.get(), 0, &bytesWritten, NULL))
      return -1;

    assert(bytesWritten != 0);
    return 0;
  }

  if (uiBufSize > SSIZE_MAX)
    uiBufSize = SSIZE_MAX;

  size_t written = 0;
  while (uiBufSize > 0)
  {
    DWORD lastWritten = 0;
    const DWORD toWrite = uiBufSize > DWORD_MAX ? DWORD_MAX : (DWORD)uiBufSize;
    if (!WriteFile(m_hFile, ((const BYTE*)lpBuf) + written, toWrite, &lastWritten, NULL))
    {
      m_filePos = -1;
      if (written > 0)
        return written; // return number of successfully written bytes
      else
        return -1;
    }
    written += lastWritten;
    uiBufSize -= lastWritten;
    // if m_filePos is set - update it
    if (m_filePos >= 0)
      m_filePos += lastWritten;

    if (lastWritten != toWrite)
      break;
  }
  return written;
}

int64_t CWin32File::Seek(int64_t iFilePosition, int iWhence /*= SEEK_SET*/)
{
  if (m_hFile == INVALID_HANDLE_VALUE)
    return -1;

  LARGE_INTEGER distance, newPos = {};
  distance.QuadPart = iFilePosition;
  DWORD moveMethod;
  if (iWhence == SEEK_SET)
    moveMethod = FILE_BEGIN;
  else if (iWhence == SEEK_CUR)
    moveMethod = FILE_CURRENT;
  else if (iWhence == SEEK_END)
    moveMethod = FILE_END;
  else
    return -1;

  if (!SetFilePointerEx(m_hFile, distance, &newPos, moveMethod))
    m_filePos = -1; // Seek failed, invalidate position
  else
    m_filePos = newPos.QuadPart;

  return m_filePos;
}

int CWin32File::Truncate(int64_t toSize)
{
  //if (m_hFile == INVALID_HANDLE_VALUE)
  //  return -1;

  //int64_t prevPos = GetPosition();
  //if (prevPos < 0)
  //  return -1;
  //
  //if (Seek(toSize) != toSize)
  //{
  //  Seek(prevPos);
  //  return -1;
  //}
  //const int ret = (SetEndOfFile(m_hFile) != 0) ? 0 : -1;
  //Seek(prevPos);

	return 0;//ret;
}

int64_t CWin32File::GetPosition()
{

  return m_filePos;
}

int64_t CWin32File::GetLength()
{
  if (m_hFile == INVALID_HANDLE_VALUE)
    return -1;

  LARGE_INTEGER fileSize = {};
  // always request current size from filesystem, as file can be changed externally
  if (!GetFileSizeEx(m_hFile, &fileSize))
    return -1;
  else
    return fileSize.QuadPart;
}

void CWin32File::Flush()
{
  if (m_hFile == INVALID_HANDLE_VALUE)
    return;

  if (!m_allowWrite)
  {
    //CLog::LogF(LOGERROR, "Attempt to flush file opened for reading");
    return;
  }

  FlushFileBuffers(m_hFile);
}

bool CWin32File::Delete(const CURL& url)
{
  assert((!m_smbFile && url.GetProtocol().empty()) || (m_smbFile && url.IsProtocol("smb"))); // function suitable only for local or SMB files
  if (m_smbFile)
    m_lastSMBFileErr = ERROR_INVALID_DATA; // used to indicate internal errors, cleared by successful file operation

  std::string pathnameW(CWIN32Util::ConvertPathToWin32Form(url));
  if (pathnameW.empty())
    return false;

  const bool result = (DeleteFileA(pathnameW.c_str()) != 0);
  if (m_smbFile)
    m_lastSMBFileErr = GetLastError(); // set real error state

  return result;
}

bool CWin32File::Rename(const CURL& urlCurrentName, const CURL& urlNewName)
{
  assert((!m_smbFile && urlCurrentName.GetProtocol().empty()) || (m_smbFile && urlCurrentName.IsProtocol("smb"))); // function suitable only for local or SMB files
  assert((!m_smbFile && urlNewName.GetProtocol().empty()) || (m_smbFile && urlNewName.IsProtocol("smb"))); // function suitable only for local or SMB files
  if (m_smbFile)
    m_lastSMBFileErr = ERROR_INVALID_DATA; // used to indicate internal errors, cleared by successful file operation

  // TODO: check whether it's file or directory
  std::string curNameW(CWIN32Util::ConvertPathToWin32Form(urlCurrentName));
  if (curNameW.empty())
    return false;

  std::string newNameW(CWIN32Util::ConvertPathToWin32Form(urlNewName));
  if (newNameW.empty())
    return false;

  const bool result = (MoveFileExA(curNameW.c_str(), newNameW.c_str(), MOVEFILE_COPY_ALLOWED) != 0);
  if (m_smbFile)
    m_lastSMBFileErr = GetLastError(); // set real error state

  return result;
}

bool CWin32File::SetHidden(const CURL& url, bool hidden)
{
  assert((!m_smbFile && url.GetProtocol().empty()) || (m_smbFile && url.IsProtocol("smb"))); // function suitable only for local or SMB files
  if (m_smbFile)
    m_lastSMBFileErr = ERROR_INVALID_DATA; // used to indicate internal errors, cleared by successful file operation

  std::string pathnameW(CWIN32Util::ConvertPathToWin32Form(url));
  if (pathnameW.empty())
    return false;

  const DWORD attrs = GetFileAttributesA(pathnameW.c_str());
  if (attrs == INVALID_FILE_ATTRIBUTES || (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0)
    return false;
  
  // check whether attribute is already set/cleared
  if (((attrs & FILE_ATTRIBUTE_HIDDEN) != 0) == hidden)
    return true;

  bool result;
  if (hidden)
    result = (SetFileAttributesA(pathnameW.c_str(), attrs | FILE_ATTRIBUTE_HIDDEN) != 0);
  else
    result = SetFileAttributesA(pathnameW.c_str(), attrs & ~FILE_ATTRIBUTE_HIDDEN) != 0;
  if (m_smbFile)
    m_lastSMBFileErr = GetLastError(); // set real error state

  return result;
}

bool CWin32File::Exists(const CURL& url)
{
  assert((!m_smbFile && url.GetProtocol().empty()) || (m_smbFile && url.IsProtocol("smb"))); // function suitable only for local or SMB files
  if (m_smbFile)
    m_lastSMBFileErr = ERROR_INVALID_DATA; // used to indicate internal errors, cleared by successful file operation

  std::string pathnameW(CWIN32Util::ConvertPathToWin32Form(url));
  if (pathnameW.empty())
    return false;

  const DWORD attrs = GetFileAttributesA(pathnameW.c_str());
  if (m_smbFile)
    m_lastSMBFileErr = GetLastError(); // set real error state

  return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

int CWin32File::Stat(const CURL& url, struct __stat64* statData)
{

  return 0;
}

int CWin32File::Stat(struct __stat64* statData)
{


  return 0;
}

#endif // _WIN32
