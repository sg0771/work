/*
视频数据采集类
有DXGI/GDI/Window/Virutal类型
*/

#include "WindowsGdiCapture.h"
#include "WindowCapture.h"
#include "MixerCapture.h"

#include "DataSource.h"

#include "WgcCapture.h"

VideoSource *VideoSource::Create(WXCTSTR wszType) {
    if (WXStrcmp(wszType, _T("Virtual")) == 0) {
		DataSource*p = new DataSource();
		return (VideoSource*)p;
	}
#ifdef _WIN32
    else if (WXStrcmp(wszType, _T("Window")) == 0) {
		WindowCapture*p = new WindowCapture();
		return (VideoSource*)p;
	}else if (WXStrcmp(wszType, _T("Mixer")) == 0) {
		MixerCapture*p = new MixerCapture();
		return (VideoSource*)p;
    }else	if (WXStrcmp(wszType, _T("GDI")) == 0) {
        WindowsGdiCapture *p = new WindowsGdiCapture();
        return (VideoSource*)p;
    }
	else	if (WXStrcmp(wszType, _T("WGC")) == 0) {
		WgcCapture* p = new WgcCapture();
		return (VideoSource*)p;
	}
#endif
    return nullptr;
}

void VideoSource::Destroy(VideoSource * ptr) {
	if (ptr) {
        if (WXStrcmp(ptr->Type(), _T("Virtual")) == 0) {
            DataSource *p = (DataSource*)ptr;
            delete p;
        }
#ifdef _WIN32
        else if (WXStrcmp(ptr->Type(), _T("GDI")) == 0) {
			WindowsGdiCapture *p = (WindowsGdiCapture*)ptr;
			delete p;
		}else if (WXStrcmp(ptr->Type(), _T("Window")) == 0) {
			WindowCapture *p = (WindowCapture*)ptr;
			delete p;
		}
		else if (WXStrcmp(ptr->Type(), _T("Mixer")) == 0) {
			MixerCapture *p = (MixerCapture*)ptr;
			delete p;
		}
		else if (WXStrcmp(ptr->Type(), _T("WGC")) == 0) {
			WgcCapture* p = (WgcCapture*)ptr;
			delete p;
		}
#endif
	}
}