

#include "../NetWork/Network.h"
#include "URIUtils.h"
#include "SpecialProtocol.h"
#include "StackDirectory.h"
#include "URL.h"
#include "StringUtils.h"
#include <vector>

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
    strPath = strFileNameAndPath.substr(0, i + 1);
    // everything to the right of the directory separator
    strFileName = strFileNameAndPath.substr(i + 1);
}

std::string URLEncodePath(const std::string& strPath)
{
    std::vector< std::string> segments = StringUtils::Split(strPath, "/");
    for (std::vector< std::string>::iterator i = segments.begin(); i != segments.end(); ++i)
        *i = CURL::Encode(*i);

    return StringUtils::Join(segments, "/");
}

std::string URLDecodePath(const std::string& strPath)
{
    std::vector< std::string> segments = StringUtils::Split(strPath, "/");
    for (std::vector< std::string>::iterator i = segments.begin(); i != segments.end(); ++i)
        *i = CURL::Decode(*i);

    return StringUtils::Join(segments, "/");
}


bool URIUtils::IsProtocol(const std::string& url, const std::string& type)
{
    return StringUtils::StartsWithNoCase(url, type + "://");
}

bool URIUtils::IsStack(const std::string& strFile)
{
    return IsProtocol(strFile, "stack");
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

bool URIUtils::IsDOSPath(const std::string& path)
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
        if (!file.empty() && file != strFolder)
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
    if (path.empty())
        return path;

    const std::string slashStr(1, slashCharacter);
    std::vector<std::string> pathVec, resultVec;
    StringUtils::Tokenize(path, pathVec, slashStr);

    for (std::vector<std::string>::const_iterator it = pathVec.begin(); it != pathVec.end(); ++it)
    {
        if (*it == ".")
        { /* skip - do nothing */
        }
        else if (*it == ".." && !resultVec.empty() && resultVec.back() != "..")
            resultVec.pop_back();
        else
            resultVec.push_back(*it);
    }

    std::string result;
    if (path[0] == slashCharacter)
        result.push_back(slashCharacter); // add slash at the begin

    result += StringUtils::Join(resultVec, slashStr);

    if (path[path.length() - 1] == slashCharacter && !result.empty() && result[result.length() - 1] != slashCharacter)
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

