
#include "VSFilterImpl.h"

#include "MemSubPic.h"

CMemSubPic::CMemSubPic(SubPicDesc& spd)
	: m_spd(spd)
{

	//m_maxsize.SetSize(spd.w, spd.h);
	//m_rcDirty.SetRect(0, 0, spd.w, spd.h);
	m_maxsize.cx = spd.w;
	m_maxsize.cy = spd.h;
	m_rcDirty.left = 0;
	m_rcDirty.top = 0;
	m_rcDirty.right = spd.w;
	m_rcDirty.bottom = spd.h;
}

CMemSubPic::~CMemSubPic()
{
	SAFE_DELETE_ARRAY(m_spd.bits);
}

// ISubPic

STDMETHODIMP_(void*) CMemSubPic::GetObject()
{
	return (void*)&m_spd;
}

STDMETHODIMP CMemSubPic::GetDesc(SubPicDesc& spd)
{
	spd.type    = m_spd.type;
	spd.w       = m_size.cx;
	spd.h       = m_size.cy;
	spd.bpp     = m_spd.bpp;
	spd.pitch   = m_spd.pitch;
	spd.bits    = m_spd.bits;
	spd.bitsU   = m_spd.bitsU;
	spd.bitsV   = m_spd.bitsV;
	spd.vidrect = m_vidrect;

	return S_OK;
}

STDMETHODIMP CMemSubPic::CopyTo(ISubPic* pSubPic)
{
	HRESULT hr;
	if (FAILED(hr = __super::CopyTo(pSubPic))) {
		return hr;
	}

	SubPicDesc src, dst;
	if (FAILED(GetDesc(src)) || FAILED(pSubPic->GetDesc(dst))) {
		return E_FAIL;
	}

	ASSERT(src.bpp == 32 && dst.bpp == 32);
	CRect rc = m_rcDirty;
	const UINT copyW_bytes = rc.Width() * 4;
	UINT copyH = rc.Height();

	BYTE* s = src.bits + src.pitch * m_rcDirty.top + m_rcDirty.left * 4;
	BYTE* d = dst.bits + dst.pitch * m_rcDirty.top + m_rcDirty.left * 4;

	while (copyH--) {
		memcpy(d, s, copyW_bytes);
		s += src.pitch;
		d += dst.pitch;
	}

	return S_OK;
}

STDMETHODIMP CMemSubPic::ClearDirtyRect()
{
	CRect rc = m_rcDirty;
	if (rc.IsRectEmpty()) {
		return S_FALSE;
	}

	ASSERT(m_spd.bpp == 32);

	BYTE* ptr = m_spd.bits + m_spd.pitch * m_rcDirty.top + m_rcDirty.left * 4;
	const UINT dirtyW = rc.Width();
	UINT dirtyH = rc.Height();

	while (dirtyH-- > 0) {
		fill_u32(ptr, m_bInvAlpha ? 0x00000000 : 0xFF000000, dirtyW);
		ptr += m_spd.pitch;
	}

	//m_rcDirty.SetRectEmpty();
	m_rcDirty.left = 0;
	m_rcDirty.top = 0;
	m_rcDirty.right = 0;
	m_rcDirty.bottom = 0;
	return S_OK;
}

STDMETHODIMP CMemSubPic::Lock(SubPicDesc& spd)
{
	return GetDesc(spd);
}

STDMETHODIMP CMemSubPic::Unlock(RECT* pDirtyRect)
{
	m_rcDirty = pDirtyRect ? *pDirtyRect : CRect(0, 0, m_spd.w, m_spd.h);

	return S_OK;
}

STDMETHODIMP CMemSubPic::AlphaBlt(RECT* pSrc, RECT* pDst, SubPicDesc* pTarget)
{
	ASSERT(pTarget);

	if (!pSrc || !pDst || !pTarget) {
		return E_POINTER;
	}

	const SubPicDesc& src = m_spd;
	SubPicDesc dst = *pTarget;

	if (src.type != dst.type) {
		return E_INVALIDARG;
	}

	CRect rs(*pSrc), rd(*pDst);

	if (dst.h < 0) {
		dst.h     = -dst.h;
		rd.bottom = dst.h - rd.bottom;
		rd.top    = dst.h - rd.top;
	}

	if (rs.Width() != rd.Width() || rs.Height() != abs(rd.Height())) {
		return E_INVALIDARG;
	}

	ASSERT(src.bpp == 32 && dst.bpp == 32);

	const int w = rs.Width();
	const int h = rs.Height();
	BYTE* s = src.bits + src.pitch * rs.top + (rs.left * 4);
	BYTE* d = dst.bits + dst.pitch * rd.top + (rd.left * 4);

	if (rd.top > rd.bottom) {
		d = dst.bits + dst.pitch * (rd.top - 1) + (rd.left * 4);
		dst.pitch = -dst.pitch;
	}

	for (int j = 0; j < h; j++, s += src.pitch, d += dst.pitch) {
		BYTE* s2 = s;
		BYTE* s2end = s2 + w * 4;

		uint32_t* d2 = (uint32_t*)d;
		for (; s2 < s2end; s2 += 4, d2++) {
#ifdef _WIN64
			uint32_t ia = 256-s2[3];
			if (s2[3] < 0xff) {
				*d2 = ((((*d2&0x00ff00ff)*s2[3])>>8) + (((*((uint32_t*)s2)&0x00ff00ff)*ia)>>8)&0x00ff00ff)
					| ((((*d2&0x0000ff00)*s2[3])>>8) + (((*((uint32_t*)s2)&0x0000ff00)*ia)>>8)&0x0000ff00);
			}
#else
			if (s2[3] < 0xff) {
				*d2 = ((((*d2&0x00ff00ff)*s2[3])>>8) + (*((uint32_t*)s2)&0x00ff00ff)&0x00ff00ff)
					| ((((*d2&0x0000ff00)*s2[3])>>8) + (*((uint32_t*)s2)&0x0000ff00)&0x0000ff00);
			}
#endif
		}
	}

	dst.pitch = abs(dst.pitch);

	return S_OK;
}

//
// CMemSubPicAllocator
//

CMemSubPicAllocator::CMemSubPicAllocator(int type, SIZE maxsize)
	: CSubPicAllocatorImpl(maxsize, false)
	, m_type(type)
	, m_maxsize(maxsize)
{
}

// ISubPicAllocatorImpl

bool CMemSubPicAllocator::Alloc(bool fStatic, ISubPic** ppSubPic)
{
	if (!ppSubPic) {
		return false;
	}

	SubPicDesc spd;
	spd.w     = m_maxsize.cx;
	spd.h     = m_maxsize.cy;
	spd.bpp   = 32;
	spd.pitch = spd.w * 4;
	spd.type  = m_type;
	spd.bits  = new BYTE[spd.pitch * spd.h];
	if (!spd.bits) {
		return false;
	}

	*ppSubPic = new CMemSubPic(spd);
	if (!(*ppSubPic)) {
		return false;
	}

	(*ppSubPic)->AddRef();
	(*ppSubPic)->SetInverseAlpha(m_bInvAlpha);

	return true;
}
