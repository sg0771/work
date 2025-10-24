#pragma once

#include <string>
#include <vector>
#include <map>

interface __declspec(uuid("EBAFBCBE-BDE0-489A-9789-05D5692E3A93"))
	IDSMResourceBag :
	public IUnknown {
	STDMETHOD_(DWORD, ResGetCount)() PURE;
	STDMETHOD(ResGet)(DWORD iIndex, BSTR* ppName, BSTR* ppDesc, BSTR* ppMime, BYTE** ppData, DWORD* pDataLen, DWORD_PTR* pTag) PURE;
	STDMETHOD(ResSet)(DWORD iIndex, LPCWSTR pName, LPCWSTR pDesc, LPCWSTR pMime, const BYTE* pData, DWORD len, DWORD_PTR tag) PURE;
	STDMETHOD(ResAppend)(LPCWSTR pName, LPCWSTR pDesc, LPCWSTR pMime, BYTE* pData, DWORD len, DWORD_PTR tag) PURE;
	STDMETHOD(ResRemoveAt)(DWORD iIndex) PURE;
	STDMETHOD(ResRemoveAll)(DWORD_PTR tag) PURE;
};


class CDSMResource
{
public:
  CDSMResource();
  CDSMResource(LPCWSTR name, LPCWSTR desc, LPCWSTR mime, BYTE* pData, int len, DWORD_PTR tag = 0);

  CDSMResource& operator=(const CDSMResource& r);

public:
  DWORD_PTR tag;
  std::wstring name, desc, mime;
  std::vector<BYTE> data;
};

class CDSMResourceBag : public IDSMResourceBag
{
public:
  CDSMResourceBag();
  virtual ~CDSMResourceBag();

  // IDSMResourceBag
  STDMETHODIMP_(DWORD) ResGetCount();
  STDMETHODIMP ResGet(DWORD iIndex, BSTR * ppName, BSTR * ppDesc, BSTR * ppMime, BYTE** ppData, DWORD * pDataLen, DWORD_PTR * pTag = nullptr);
  STDMETHODIMP ResSet(DWORD iIndex, LPCWSTR pName, LPCWSTR pDesc, LPCWSTR pMime, const BYTE * pData, DWORD len, DWORD_PTR tag = 0);
  STDMETHODIMP ResAppend(LPCWSTR pName, LPCWSTR pDesc, LPCWSTR pMime, BYTE * pData, DWORD len, DWORD_PTR tag = 0);
  STDMETHODIMP ResRemoveAt(DWORD iIndex);
  STDMETHODIMP ResRemoveAll(DWORD_PTR tag = 0);

private:
  CCritSec m_csResources;
  std::vector<CDSMResource> m_resources;
};
