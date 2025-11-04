#pragma once
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
//-----------------------------------------------------------------------
//
//  File:      StringUtils.h
//
//  Purpose:   ATL split string utility
//  Author:    Paul J. Weiss
//
//  Modified to support J O'Leary's std::string class by kraqh3d
//
//------------------------------------------------------------------------

#include <stdarg.h>
#include <stdint.h>
#include <string>
#include <vector>

#include "XBDateTime.h"

class StringUtils
{
public:

  static std::string Format( const char *fmt, ...);
  static std::string FormatV( const char *fmt, va_list args);
  static std::wstring Format( const wchar_t *fmt, ...);
  static std::wstring FormatV( const wchar_t *fmt, va_list args);
  static void ToUpper(std::string &str);
  static void ToUpper(std::wstring &str);
  static void ToLower(std::string &str);
  static void ToLower(std::wstring &str);
  static void ToCapitalize(std::string &str);
  static void ToCapitalize(std::wstring &str);
  static bool EqualsNoCase(const std::string &str1, const std::string &str2);
  static bool EqualsNoCase(const std::string &str1, const char *s2);
  static bool EqualsNoCase(const char *s1, const char *s2);
  static int  CompareNoCase(const std::string &str1, const std::string &str2);
  static int  CompareNoCase(const char *s1, const char *s2);
  static std::string Left(const std::string &str, size_t count);
  static std::string Mid(const std::string &str, size_t first, size_t count = std::string::npos);
  static std::string Right(const std::string &str, size_t count);
  static std::string& Trim(std::string &str);
  static std::string& Trim(std::string &str, const char* const chars);
  static std::string& TrimLeft(std::string &str);
  static std::string& TrimLeft(std::string &str, const char* const chars);
  static std::string& TrimRight(std::string &str);
  static std::string& TrimRight(std::string &str, const char* const chars);
  static std::string& RemoveDuplicatedSpacesAndTabs(std::string& str);
  static int Replace(std::string &str, char oldChar, char newChar);
  static int Replace(std::string &str, const std::string &oldStr, const std::string &newStr);
  static int Replace(std::wstring &str, const std::wstring &oldStr, const std::wstring &newStr);
  static bool StartsWith(const std::string &str1, const std::string &str2);
  static bool StartsWith(const std::string &str1, const char *s2);
  static bool StartsWith(const char *s1, const char *s2);
  static bool StartsWithNoCase(const std::string &str1, const std::string &str2);
  static bool StartsWithNoCase(const std::string &str1, const char *s2);
  static bool StartsWithNoCase(const char *s1, const char *s2);
  static bool EndsWith(const std::string &str1, const std::string &str2);
  static bool EndsWith(const std::string &str1, const char *s2);
  static bool EndsWithNoCase(const std::string &str1, const std::string &str2);
  static bool EndsWithNoCase(const std::string &str1, const char *s2);

  static std::string Join(const std::vector<std::string> &strings, const std::string& delimiter);

  static std::vector<std::string> Split(const std::string& input, const std::string& delimiter, unsigned int iMaxStrings = 0);
  static std::vector<std::string> Split(const std::string& input, const char delimiter, size_t iMaxStrings = 0);
  static int FindNumber(const std::string& strInput, const std::string &strFind);
  static int64_t AlphaNumericCompare(const wchar_t *left, const wchar_t *right);
  static long TimeStringToSeconds(const std::string &timeString);
  static void RemoveCRLF(std::string& strLine);

  static size_t utf8_strlen(const char *s);

  static bool IsNaturalNumber(const std::string& str);
  static bool IsInteger(const std::string& str);

  inline static bool isasciidigit(char chr) // locale independent 
  {
    return chr >= '0' && chr <= '9'; 
  }
  inline static bool isasciixdigit(char chr) // locale independent 
  {
    return (chr >= '0' && chr <= '9') || (chr >= 'a' && chr <= 'f') || (chr >= 'A' && chr <= 'F'); 
  }
  static int asciidigitvalue(char chr); // locale independent 
  static int asciixdigitvalue(char chr); // locale independent 
  inline static bool isasciiuppercaseletter(char chr) // locale independent
  {
    return (chr >= 'A' && chr <= 'Z'); 
  }
  inline static bool isasciilowercaseletter(char chr) // locale independent
  {
    return (chr >= 'a' && chr <= 'z'); 
  }
  inline static bool isasciialphanum(char chr) // locale independent
  {
    return isasciiuppercaseletter(chr) || isasciilowercaseletter(chr) || isasciidigit(chr); 
  }
  static std::string SizeToString(int64_t size);
  static const std::string Empty;
  static size_t FindWords(const char *str, const char *wordLowerCase);
  static int FindEndBracket(const std::string &str, char opener, char closer, int startPos = 0);
  static int DateStringToYYYYMMDD(const std::string &dateString);
  static void WordToDigits(std::string &word);
  static std::string CreateUUID();
  static bool ValidateUUID(const std::string &uuid); // NB only validates syntax
  static double CompareFuzzy(const std::string &left, const std::string &right);
  static int FindBestMatch(const std::string &str, const std::vector<std::string> &strings, double &matchscore);
  static bool ContainsKeyword(const std::string &str, const std::vector<std::string> &keywords);

  /*! \brief Escapes the given string to be able to be used as a parameter.

   Escapes backslashes and double-quotes with an additional backslash and
   adds double-quotes around the whole string.

   \param param String to escape/paramify
   \return Escaped/Paramified string
   */
  static std::string Paramify(const std::string &param);

  /*! \brief Split a string by the specified delimiters.
   Splits a string using one or more delimiting characters, ignoring empty tokens.
   Differs from Split() in two ways:
    1. The delimiters are treated as individual characters, rather than a single delimiting string.
    2. Empty tokens are ignored.
   \return a vector of tokens
   */
  static std::vector<std::string> Tokenize(const std::string& input, const std::string& delimiters);
  static void Tokenize(const std::string& input, std::vector<std::string>& tokens, const std::string& delimiters);
  static std::vector<std::string> Tokenize(const std::string& input, const char delimiter);
  static void Tokenize(const std::string& input, std::vector<std::string>& tokens, const char delimiter);
private:
  static std::string m_lastUUID;
};

struct sortstringbyname
{
  bool operator()(const std::string& strItem1, const std::string& strItem2)
  {
    return StringUtils::CompareNoCase(strItem1, strItem2) < 0;
  }
};
