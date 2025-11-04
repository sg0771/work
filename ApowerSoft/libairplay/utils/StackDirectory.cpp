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

#include <stdlib.h>
#include "StackDirectory.h"
#include "FileItem.h"
#include "../utils/StringUtils.h"
#include "../utils/URL.h"

using namespace std;
namespace XFILE
{
  CStackDirectory::CStackDirectory()
  {
  }

  CStackDirectory::~CStackDirectory()
  {
  }

  std::string CStackDirectory::GetStackedTitlePath(const std::string &strPath)
  {

	return "";// GetStackedTitlePath(strPath, RegExps);
  }

  std::string CStackDirectory::GetFirstStackedFile(const std::string &strPath)
  {
    // the stacked files are always in volume order, so just get up to the first filename
    // occurence of " , "
    std::string file, folder;
    size_t pos = strPath.find(" , ");

    // remove "stack://" from the folder
    folder = folder.substr(8);
    StringUtils::Replace(file, ",,", ",");

	return "";
  }

  bool CStackDirectory::GetPaths(const std::string& strPath, vector<std::string>& vecPaths)
  {
    std::string path = strPath;
    path = path.substr(8);

    vecPaths = StringUtils::Split(path, " , ");
    if (vecPaths.empty())
      return false;

    // because " , " is used as a seperator any "," in the real paths are double escaped
    for (vector<std::string>::iterator itPath = vecPaths.begin(); itPath != vecPaths.end(); ++itPath)
      StringUtils::Replace(*itPath, ",,", ",");

    return true;
  }

  bool CStackDirectory::ConstructStackPath(const vector<std::string> &paths, std::string& stackedPath)
  {
    if (paths.size() < 2)
      return false;
    stackedPath = "stack://";
    std::string folder, file;
    stackedPath += folder;
    // double escape any occurence of commas
    StringUtils::Replace(file, ",", ",,");
    stackedPath += file;
    for (unsigned int i = 1; i < paths.size(); ++i)
    {
      stackedPath += " , ";
      file = paths[i];

      // double escape any occurence of commas
      StringUtils::Replace(file, ",", ",,");
      stackedPath += file;
    }
    return true;
  }
}

