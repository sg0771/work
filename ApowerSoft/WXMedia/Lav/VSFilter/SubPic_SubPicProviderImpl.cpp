
#include "VSFilterImpl.h"

#include "SubPicProviderImpl.h"

CSubPicProviderImpl::CSubPicProviderImpl(CCritSec* pLock)
	: CUnknown(L"CSubPicProviderImpl", nullptr)
	, m_pLock(pLock)
{
}

CSubPicProviderImpl::~CSubPicProviderImpl()
{
}

STDMETHODIMP CSubPicProviderImpl::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI(ISubPicProvider)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// CSubPicProviderImpl

STDMETHODIMP CSubPicProviderImpl::Lock()
{
	return m_pLock ? m_pLock->Lock(), S_OK : E_FAIL;
}

STDMETHODIMP CSubPicProviderImpl::Unlock()
{
	return m_pLock ? m_pLock->Unlock(), S_OK : E_FAIL;
}
