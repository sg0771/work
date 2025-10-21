

#pragma once

#include "MemSubPic.h"

enum {MSP_P010,MSP_P016,MSP_RGB32,MSP_RGB24,MSP_RGB16,MSP_RGB15,MSP_YUY2,MSP_NV12,MSP_YV12,MSP_IYUV,MSP_AYUV,MSP_RGBA};

// CMemSubPicEx

class CMemSubPicEx : public CMemSubPic
{
public:
	CMemSubPicEx(SubPicDesc& spd);

	// ISubPic
	STDMETHODIMP Unlock(RECT* pDirtyRect) override;
	STDMETHODIMP AlphaBlt(RECT* pSrc, RECT* pDst, SubPicDesc* pTarget) override;
};

// CMemSubPicExAllocator

class CMemSubPicExAllocator : public CMemSubPicAllocator
{
//protected:
	// CSubPicAllocatorImpl
	bool Alloc(bool fStatic, ISubPic** ppSubPic) override;

public:
	CMemSubPicExAllocator(int type, SIZE maxsize);
};
