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

#pragma once

 // {C0BE9565-4C05-4644-9492-57547A4048DC}
DEFINE_GUID(IID_IStreamSourceControl,
	0xc0be9565, 0x4c05, 0x4644, 0x94, 0x92, 0x57, 0x54, 0x7a, 0x40, 0x48, 0xdc);

interface __declspec(uuid("C0BE9565-4C05-4644-9492-57547A4048DC"))
	IStreamSourceControl : public IUnknown
{
	// Get the duration of the stream being played.
	// Duration is in DirectShow reference time, 100ns units.
	STDMETHOD(GetStreamDuration) (REFERENCE_TIME* prtDuration) PURE;

	// Seek the stream to a specified time
	//
	// Position is in DirectShow reference time, 100ns units.
	//
	// If the source returns a failure code, the demuxer will do byte-based seeking itself (ie. when the stream supports this)
	// On success, it'll re-open the stream and start reading from the start (byte position 0).
	STDMETHOD(SeekStream) (REFERENCE_TIME rtPosition) PURE;
};
class CLAVSplitter;

class CLAVInputPin : public CBasePin, public CCritSec, public IStreamSourceControl
{
public:
  CLAVInputPin(TCHAR* pName, CLAVSplitter *pFilter, CCritSec* pLock, HRESULT* phr);
  ~CLAVInputPin(void);

  HRESULT GetAVIOContext(AVIOContext** ppContext);

  DECLARE_IUNKNOWN;
  STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

  HRESULT CheckMediaType(const CMediaType* pmt);
  HRESULT CheckConnect(IPin* pPin);
  HRESULT BreakConnect();
  HRESULT CompleteConnect(IPin* pPin);

  STDMETHODIMP BeginFlush();
  STDMETHODIMP EndFlush();

  CMediaType& CurrentMediaType() { return m_mt; }

  // IStreamSourceControl
  STDMETHODIMP GetStreamDuration(REFERENCE_TIME *prtDuration) { CheckPointer(m_pStreamControl, E_NOTIMPL); return m_pStreamControl->GetStreamDuration(prtDuration); }
  STDMETHODIMP SeekStream(REFERENCE_TIME rtPosition) { CheckPointer(m_pStreamControl, E_NOTIMPL); return m_pStreamControl->SeekStream(rtPosition); }

protected:
  static int Read(void *opaque, uint8_t *buf, int buf_size);
  static int64_t Seek(void *opaque, int64_t offset, int whence);

  LONGLONG m_llPos = 0;

private:
  IAsyncReader *m_pAsyncReader = nullptr;
  AVIOContext *m_pAVIOContext  = nullptr;

  IStreamSourceControl *m_pStreamControl = nullptr;

  BOOL m_bURLSource = false;
};
