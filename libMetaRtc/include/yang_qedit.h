#ifndef _YANG_QEDIT_H
#define _YANG_QEDIT_H



//DirectShowÍ·ÎÄ¼þ
#include <strmif.h>
#include <atlbase.h>
#include <dvdmedia.h>
#include <amvideo.h>
#include <control.h>
#include <uuids.h>
#include <ks.h>
#include <ksmedia.h>

EXTERN_C const IID   IID_ISampleGrabber;
EXTERN_C const IID   IID_ISampleGrabberCB;

EXTERN_C const CLSID CLSID_SampleGrabber;

struct ISampleGrabberCB;
MIDL_INTERFACE("0579154A-2B53-4994-B0D0-E773148EFF85")
ISampleGrabberCB : public IUnknown{
public:
	virtual HRESULT STDMETHODCALLTYPE SampleCB(double SampleTime,IMediaSample *pSample) = 0;
	virtual HRESULT STDMETHODCALLTYPE BufferCB(double SampleTime,BYTE *pBuffer,long BufferLen) = 0;
};

struct ISampleGrabber;
MIDL_INTERFACE("6B652FFF-11FE-4fce-92AD-0266B5D7C78F")
ISampleGrabber : public IUnknown{
public:
	virtual HRESULT STDMETHODCALLTYPE SetOneShot(BOOL OneShot) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetMediaType(const AM_MEDIA_TYPE *pType) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetConnectedMediaType(AM_MEDIA_TYPE *pType) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetBufferSamples(BOOL BufferThem) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetCurrentBuffer(long *pBufferSize, long *pBuffer) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetCurrentSample(IMediaSample **ppSample) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetCallback(ISampleGrabberCB *pCallback,long WhichMethodToCallback) = 0;
};

#endif