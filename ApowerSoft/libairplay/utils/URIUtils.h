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
#pragma once

#include <string>

class URIUtils
{
public:
  static void Split(const std::string& strFileNameAndPath, 
                    std::string& strPath, std::string& strFileName);
  static bool IsProtocol(const std::string& url, const std::string& type);
  static bool IsDOSPath(const std::string& path);
  static bool IsURL(const std::string& strFile);
  static bool IsStack(const std::string& strFile);
  static bool IsSpecial(const std::string& strFile);
  static void AddSlashAtEnd(std::string& strFolder);
  static bool HasSlashAtEnd(const std::string& strFile, bool checkURL = false);
  static std::string FixSlashesAndDups(const std::string& path, const char slashCharacter = '/', const size_t startFrom = 0);
  static std::string CanonicalizePath(const std::string& path, const char slashCharacter = '\\');
  static std::string AddFileToFolder(const std::string &strFolder, const std::string &strFile);
};

