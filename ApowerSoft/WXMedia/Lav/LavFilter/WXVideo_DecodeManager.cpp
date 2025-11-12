/*
 *      Copyright (C) 2010-2017 Hendrik Leppkes
 *      http://www.1f0.de
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include "WXVideoImpl.h"
#include "WXVideo_DecodeManager.h"

#include "LAVVideo.h"

#include <Shlwapi.h>


CDecodeManager::CDecodeManager(CLAVVideo *pLAVVideo)
  : m_pLAVVideo(pLAVVideo)
{
  WCHAR fileName[1024];
  GetModuleFileName(nullptr, fileName, 1024);
  m_processName = PathFindFileName (fileName);
}

CDecodeManager::~CDecodeManager(void)
{
  Close();
}

STDMETHODIMP CDecodeManager::Close()
{
  DSAutoLock decoderLock(this);
  SAFE_DELETE(m_pDecoder);
  
  return S_OK;
}


STDMETHODIMP CDecodeManager::CreateDecoder(const CMediaType *pmt, AVCodecID codec)
{
  DSAutoLock decoderLock(this);

  DbgLog((LOG_TRACE, 10, L"CDecodeThread::CreateDecoder(): Creating new decoder for codec %S", avcodec_get_name(codec)));
  HRESULT hr = S_OK;
  BOOL bWMV9 = FALSE;


  BOOL bHWDecBlackList = _wcsicmp(m_processName.c_str(), L"dllhost.exe") == 0 || _wcsicmp(m_processName.c_str(), L"explorer.exe") == 0 || _wcsicmp(m_processName.c_str(), L"ReClockHelper.dll") == 0;
  DbgLog((LOG_TRACE, 10, L"-> Process is %s, blacklist: %d", m_processName.c_str(), bHWDecBlackList));

  BITMAPINFOHEADER *pBMI = nullptr;
  videoFormatTypeHandler(*pmt, &pBMI);


  softwaredec:
  // Fallback for software
  if (!m_pDecoder) {
    DbgLog((LOG_TRACE, 10, L"-> No HW Codec, using Software"));
    if (m_pLAVVideo->GetUseMSWMV9Decoder() && (codec == AV_CODEC_ID_VC1 || codec == AV_CODEC_ID_WMV3) && !m_bWMV9Failed) {
      m_pDecoder = CreateDecoderWMV9MFT();
      bWMV9 = TRUE;
    } 
      m_pDecoder = CreateDecoderAVCodec();
  }
  DbgLog((LOG_TRACE, 10, L"-> Created Codec '%s'", m_pDecoder->GetDecoderName()));

  hr = m_pDecoder->InitInterfaces(static_cast<ILAVVideoSettings *>(m_pLAVVideo), static_cast<ILAVVideoCallback *>(m_pLAVVideo));
  if (FAILED(hr)) {
    DbgLog((LOG_TRACE, 10, L"-> Init Interfaces failed (hr: 0x%x)", hr));
    goto done;
  }

  hr = m_pDecoder->InitDecoder(codec, pmt);
  if (FAILED(hr)) {
    DbgLog((LOG_TRACE, 10, L"-> Init Decoder failed (hr: 0x%x)", hr));
    goto done;
  }

done:
  if (FAILED(hr)) {
    SAFE_DELETE(m_pDecoder);
    if (bWMV9) {
      DbgLog((LOG_TRACE, 10, L"-> WMV9 MFT decoder failed, trying avcodec instead..."));
      m_bWMV9Failed = TRUE;
      bWMV9 = FALSE;
      goto softwaredec;
    }
    return hr;
  }

  m_Codec = codec;

  return hr;
}

STDMETHODIMP CDecodeManager::Decode(IMediaSample *pSample)
{
  DSAutoLock decoderLock(this);
  HRESULT hr = S_OK;

  if (!m_pDecoder)
    return E_UNEXPECTED;

  hr = m_pDecoder->Decode(pSample);

  return S_OK;
}

STDMETHODIMP CDecodeManager::Flush()
{
  DSAutoLock decoderLock(this);

  if (!m_pDecoder)
    return E_UNEXPECTED;

  return m_pDecoder->Flush();
}

STDMETHODIMP CDecodeManager::EndOfStream()
{
  DSAutoLock decoderLock(this);

  if (!m_pDecoder)
    return E_UNEXPECTED;

  return m_pDecoder->EndOfStream();
}

STDMETHODIMP CDecodeManager::InitAllocator(IMemAllocator **ppAlloc)
{
  DSAutoLock decoderLock(this);

  if (!m_pDecoder)
    return E_UNEXPECTED;

  return m_pDecoder->InitAllocator(ppAlloc);
}

STDMETHODIMP CDecodeManager::PostConnect(IPin *pPin)
{
  DSAutoLock decoderLock(this);
  HRESULT hr = S_OK;
  if (m_pDecoder) {
    hr = m_pDecoder->PostConnect(pPin);
    if (FAILED(hr)) {
      CMediaType &mt = m_pLAVVideo->GetInputMediaType();
      hr = CreateDecoder(&mt, m_Codec);
    }
  }
  return hr;
}

STDMETHODIMP CDecodeManager::BreakConnect()
{
  DSAutoLock decoderLock(this);

  if (!m_pDecoder)
    return E_UNEXPECTED;

  return m_pDecoder->BreakConnect();
}
