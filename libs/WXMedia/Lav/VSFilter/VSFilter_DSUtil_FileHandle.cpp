
#include "VSFilterImpl.h"

#include "FileHandle.h"

//
// Returns the file portion from a path
//
CStringW GetFileOnly(LPCWSTR Path)
{
	CStringW cs = Path;
	::PathStripPathW(cs.GetBuffer(0));
	cs.ReleaseBuffer(-1);
	return cs;
}

//
// Returns the folder portion from a path
//
CStringW GetFolderOnly(LPCWSTR Path)
{
	CStringW cs = Path; // Force CStringW to make a copy
	::PathRemoveFileSpecW(cs.GetBuffer(0));
	cs.ReleaseBuffer(-1);
	return cs;
}

//
// Adds a backslash to the end of a path if it is needed
//
CStringW AddSlash(LPCWSTR Path)
{
	CStringW cs = Path;
	::PathAddBackslashW(cs.GetBuffer(MAX_PATH));
	cs.ReleaseBuffer(-1);
	if(cs.IsEmpty()) {
		cs = L"\\";
	}
	return cs;
}

//
// Removes a backslash from the end of a path if it is there
//
CStringW RemoveSlash(LPCWSTR Path)
{
	CString cs = Path;
	::PathRemoveBackslashW(cs.GetBuffer(MAX_PATH));
	cs.ReleaseBuffer(-1);
	return cs;
}

//
// Returns just the .ext part of the file path
//
CStringW GetFileExt(LPCWSTR Path)
{
	CStringW cs = ::PathFindExtensionW(Path);
	return cs;
}

//
// Exchanges one file extension for another and returns the new fiel path
//
CStringW RenameFileExt(LPCWSTR Path, LPCWSTR Ext)
{
	CStringW cs = Path;
	::PathRenameExtensionW(cs.GetBuffer(MAX_PATH), Ext);
	return cs;
}

//
// Removes the file name extension from a path, if one is present
//
CStringW RemoveFileExt(LPCWSTR Path)
{
	CStringW cs = Path;
	::PathRemoveExtensionW(cs.GetBuffer(MAX_PATH));
	return cs;
}

//
// Generate temporary files with any extension
//
BOOL GetTemporaryFilePath(CStringW strExtension, CStringW& strFileName)
{
	WCHAR lpszTempPath[MAX_PATH] = { 0 };
	if (!GetTempPathW(MAX_PATH, lpszTempPath)) {
		return FALSE;
	}

	WCHAR lpszFilePath[MAX_PATH] = { 0 };
	do {
		if (!GetTempFileNameW(lpszTempPath, L"mpc", 0, lpszFilePath)) {
			return FALSE;
		}

		DeleteFile(lpszFilePath);

		strFileName = lpszFilePath;
		strFileName.Replace(L".tmp", strExtension);

		DeleteFile(strFileName);
	} while (_waccess(strFileName, 00) != -1);

	return TRUE;
}

//
// Compact Path
//
CStringW CompactPath(LPCWSTR Path, UINT cchMax)
{
	CStringW cs = Path;
	WCHAR pathbuf[MAX_PATH] = { 0 };
	if (::PathCompactPathExW(pathbuf, cs, cchMax, 0)) {
		cs = pathbuf;
	}

	return cs;
}

//
// Get path of specified module
//
CStringW GetModulePath(HMODULE hModule)
{
	CStringW path;
	DWORD bufsize = MAX_PATH;
	DWORD len = 0;
	while (1) {
		len = GetModuleFileNameW(hModule, path.GetBuffer(bufsize), bufsize);
		if (len < bufsize) {
			break;
		}
		bufsize *= 2;
	}
	path.ReleaseBufferSetLength(len);

	ASSERT(path.GetLength());
	return path;
}

//
// Get path of the executable file of the current process.
//
CStringW GetProgramPath()
{
	return GetModulePath(nullptr);
}

//
// Get program directory with backslash
//
CStringW GetProgramDir()
{
	CStringW path = GetModulePath(nullptr);
	path.Truncate(path.ReverseFind('\\') + 1); // do not use this method for random paths

	return path;
}

//
// Get application path from "App Paths" subkey
//
CStringW GetRegAppPath(LPCWSTR appFileName, const bool bCurrentUser)
{
	CStringW keyName(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\");
	keyName.Append(appFileName);

	const HKEY hKeyParent = bCurrentUser ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE;
	CStringW appPath;
	CRegKey key;

	if (ERROR_SUCCESS == key.Open(hKeyParent, keyName, KEY_READ)) {
		ULONG nChars = 0;
		if (ERROR_SUCCESS == key.QueryStringValue(nullptr, nullptr, &nChars)) {
			if (ERROR_SUCCESS == key.QueryStringValue(nullptr, appPath.GetBuffer(nChars), &nChars)) {
				appPath.ReleaseBuffer(nChars);
			}
		}
		key.Close();
	}

	return appPath;
}



void CleanPath(CStringW& path)
{
	// remove double quotes enclosing path
	path.Trim();
	if (path.GetLength() >= 2 && path[0] == '\"' && path[path.GetLength() - 1] == '\"') {
		path = path.Mid(1, path.GetLength() - 2);
	}
};

bool CFileGetStatus(LPCWSTR lpszFileName, CFileStatus& status)
{
	try {
		return !!CFile::GetStatus(lpszFileName, status);
	}
	catch (CException* e) {
		// MFCBUG: E_INVALIDARG / "Parameter is incorrect" is thrown for certain cds (vs2003)
		// http://groups.google.co.uk/groups?hl=en&lr=&ie=UTF-8&threadm=OZuXYRzWDHA.536%40TK2MSFTNGP10.phx.gbl&rnum=1&prev=/groups%3Fhl%3Den%26lr%3D%26ie%3DISO-8859-1
		//DLog(L"CFile::GetStatus() has thrown an exception");
		e->Delete();
		return false;
	}
}
