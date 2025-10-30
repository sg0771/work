

#pragma once

#include "SubPicImpl.h"

// CMemSubPic

class CMemSubPic : public CSubPicImpl
{
protected:
	SubPicDesc m_spd;

public:
	CMemSubPic(SubPicDesc& spd);
	virtual ~CMemSubPic();

	// ISubPic
protected:
	STDMETHODIMP_(void*) GetObject() override; // returns SubPicDesc*
public:
	STDMETHODIMP GetDesc(SubPicDesc& spd) override;
	STDMETHODIMP CopyTo(ISubPic* pSubPic) override;
	STDMETHODIMP ClearDirtyRect() override;
	STDMETHODIMP Lock(SubPicDesc& spd) override;
	STDMETHODIMP Unlock(RECT* pDirtyRect) override;
	STDMETHODIMP AlphaBlt(RECT* pSrc, RECT* pDst, SubPicDesc* pTarget) override;
};

// CMemSubPicAllocator

class CMemSubPicAllocator : public CSubPicAllocatorImpl
{
protected:
	const int m_type;
	CSize m_maxsize;

	// CSubPicAllocatorImpl
	bool Alloc(bool fStatic, ISubPic** ppSubPic) override;

public:
	CMemSubPicAllocator(int type, SIZE maxsize);
};
