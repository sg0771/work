
#include "VSFilterImpl.h"

#include "BaseSub.h"

#include <WXMediaCpp.h>

CBaseSub::CBaseSub(SUBTITLE_TYPE nType)
	: m_nType(nType)
	, m_bResizedRender(FALSE)
	, convertType(ColorConvert::convertType::DEFAULT)
{
}

CBaseSub::~CBaseSub()
{
}

void CBaseSub::InitSpd(SubPicDesc& spd, int nWidth, int nHeight)
{
	if (m_spd.w != nWidth || m_spd.h != nHeight || !m_pTempSpdBuff) {
		m_spd.type    = 0;
		m_spd.w       = nWidth;
		m_spd.h       = nHeight;
		m_spd.bpp     = 32;
		m_spd.pitch   = m_spd.w * 4;
		m_spd.vidrect = CRect(0, 0, m_spd.w, m_spd.h);

		m_pTempSpdBuff.reset(new BYTE[m_spd.pitch * m_spd.h]);
		m_spd.bits    = m_pTempSpdBuff.get();
	}

	if (!m_bResizedRender && (m_spd.w != spd.w || m_spd.h != spd.h)) {
		m_bResizedRender = TRUE;

		BYTE* p = m_spd.bits;
		for (int y = 0; y < m_spd.h; y++, p += m_spd.pitch) {
			fill_u32(p, 0xFF000000, m_spd.w);
		}
	}
}


void CBaseSub::FinalizeRender(SubPicDesc& spd)
{
	if (m_bResizedRender) {
		m_bResizedRender = FALSE;

#if 0
		// StretchBlt ...
		int filter = (spd.w < m_spd.w && spd.h < m_spd.h) ? CResampleRGB32::FILTER_BOX : CResampleRGB32::FILTER_BILINEAR;

		HRESULT hr = m_resampleRgb32.SetParameters(spd.w, spd.h, m_spd.w, m_spd.h, filter, true);
		if (S_OK == hr) {
			hr = m_resampleRgb32.Process(spd.bits, m_spd.bits); //可以用libyuv代替
		}
#else
		libyuv::ARGBScale(m_spd.bits, m_spd.w * 4, m_spd.w, m_spd.h,
			spd.bits, spd.w * 4, spd.w, spd.h, libyuv::kFilterBilinear);

#endif
		

		//ASSERT(hr == S_OK);
	}
}
