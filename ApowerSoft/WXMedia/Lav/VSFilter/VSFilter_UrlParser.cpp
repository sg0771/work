
#include "VSFilterImpl.h"

#include "UrlParser.h"


CUrlParser::CUrlParser(LPCWSTR lpszUrl)
{
	Parse(lpszUrl);
}

BOOL CUrlParser::Parse(LPCWSTR lpszUrl)
{
	Clear();
	CheckPointer(lpszUrl, FALSE);

	URL_COMPONENTSW components = { sizeof(components) };
	components.dwSchemeLength = (DWORD)-1;
	components.dwHostNameLength = (DWORD)-1;
	components.dwUserNameLength = (DWORD)-1;
	components.dwPasswordLength = (DWORD)-1;
	components.dwUrlPathLength = (DWORD)-1;
	components.dwExtraInfoLength = (DWORD)-1;

	const auto ret = InternetCrackUrlW(lpszUrl, wcslen(lpszUrl), 0, &components);
	if (ret) {
		m_szUrl = lpszUrl;

		m_szSchemeName.SetString(components.lpszScheme, components.dwSchemeLength);
		m_szHostName.SetString(components.lpszHostName, components.dwHostNameLength);
		m_szUserName.SetString(components.lpszUserName, components.dwUserNameLength);
		m_szPassword.SetString(components.lpszPassword, components.dwPasswordLength);
		m_szUrlPath.SetString(components.lpszUrlPath, components.dwUrlPathLength);
		m_szExtraInfo.SetString(components.lpszExtraInfo, components.dwExtraInfoLength);

		m_nPortNumber = components.nPort ? components.nPort : 80;
		m_nScheme = components.nScheme;

		m_szSchemeName.MakeLower();

		if (m_szUrlPath.IsEmpty()) {
			m_szUrlPath = L"/";
		}
	} else {
		Clear();
	}

	return ret;
}

void CUrlParser::Clear()
{
	m_szUrl.Empty();

	m_szSchemeName.Empty();
	m_szHostName.Empty();
	m_szUserName.Empty();
	m_szPassword.Empty();
	m_szUrlPath.Empty();
	m_szExtraInfo.Empty();

	m_nPortNumber = 0;
	m_nScheme = INTERNET_SCHEME_UNKNOWN;
}
