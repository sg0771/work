

#pragma once

CStringW GetFileOnly(LPCWSTR Path);
CStringW GetFolderOnly(LPCWSTR Path);
CStringW AddSlash(LPCWSTR Path);
CStringW RemoveSlash(LPCWSTR Path);
CStringW GetFileExt(LPCWSTR Path);
CStringW RenameFileExt(LPCWSTR Path, LPCWSTR Ext);
CStringW RemoveFileExt(LPCWSTR Path);
BOOL     GetTemporaryFilePath(CStringW strExtension, CStringW& strFileName);
CStringW CompactPath(LPCWSTR Path, UINT cchMax);

// Get path of specified module
CStringW GetModulePath(HMODULE hModule);

// Get path of the executable file of the current process
CStringW GetProgramPath();

// Get programm directory with slash
CStringW GetProgramDir();

// Get application path from "App Paths" subkey
CStringW GetRegAppPath(LPCWSTR appFileName, const bool bUser);

// wFunc can be FO_MOVE or FO_COPY.
// To move a folder, add "\" to the end of the source path.
// To copy a folder, add "\*" to the end of the source path.
int FileOperation(const CStringW& source, const CStringW& target, const UINT wFunc);

void CleanPath(CStringW& path);

bool CFileGetStatus(LPCWSTR lpszFileName, CFileStatus& status);
