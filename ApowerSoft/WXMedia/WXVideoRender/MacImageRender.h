//
//  Header.h
//  WXMedia
//
//  Created by tam on 2019/12/10.
//  Copyright © 2019年 xkt.spacexkt.space. All rights reserved.
//

#ifndef MacImageRender_h
#define MacImageRender_h

#include "WXMediaCpp.h"
#include <libyuv/libyuv.h>
#import  <Cocoa/Cocoa.h>
#import  <Appkit/Appkit.h>

//MacRender Using NSImageView
class MacImageRender : public IWXVideRenderImpl {
    
    void OnOpen(HWND hwnd, int width, int height){
        dispatch_async(dispatch_get_main_queue(), ^{
            m_view = (__bridge NSView*)hwnd;
            m_iWidth = width;
            m_iHeight = height;
            m_frame.Init(AV_PIX_FMT_ABGR, m_iWidth, m_iHeight);
            m_colorSpace = CGColorSpaceCreateDeviceRGB();
            m_context = CGBitmapContextCreate(m_frame.m_pFrame->data[0], m_iWidth, m_iHeight, 8, m_frame.m_pFrame->linesize[0], m_colorSpace,
                                                         kCGBitmapByteOrder32Little | kCGImageAlphaNoneSkipLast);
            
            m_viewImpl = [[NSImageView alloc] init];
            printf("MacImageRender %s Open OK  [%dx%d]!!!\n", __FUNCTION__, m_iWidth, m_iHeight);
            m_bOpen = 1;
        });
    }
    void OnDraw(){
        dispatch_async(dispatch_get_main_queue(), ^{
            m_viewImpl.frame = NSRect{0,0,m_view.frame.size.width,m_view.frame.size.height}; 
            CGImageRef quartzImage = CGBitmapContextCreateImage(m_context);
            NSImage *image = [[NSImage alloc] initWithCGImage:quartzImage size:NSZeroSize];
            CGImageRelease(quartzImage);
            m_viewImpl.imageScaling = m_bFixed ? NSImageScaleProportionallyDown : NSImageScaleAxesIndependently;
            [m_viewImpl setImage:image];
            image = nil;//ARC
        });
    }
    void CloseImpl(){
        if(m_bOpen){
            if(nil!=m_viewImpl){
                m_viewImpl = nil;
            }
            m_iWidth = 0;
            m_iHeight = 0;
            if(m_colorSpace){
                CGColorSpaceRelease(m_colorSpace);
                m_colorSpace = NULL;
            }
            if(m_context){
                CGContextRelease(m_context);
                m_context = NULL;
            }
            printf("MacImageRender %s  OK!!!\n", __FUNCTION__);
            m_bOpen = 0;
        }
    }
    void OnClose(){
        dispatch_async(dispatch_get_main_queue(), ^{
            CloseImpl();
        });
    }
public:
    NSView *m_view = nil;
    NSImageView  *m_viewImpl = nil;
    WXVideoFrame m_frame;
    WXVideoConvert m_conv;
    int m_bOpen = 0;
    int m_iWidth = 0;
    int m_iHeight = 0;
    int m_bFixed = 0;
    CGColorSpaceRef m_colorSpace  = NULL;
    CGContextRef m_context = NULL;
public:
    virtual int Open(HWND hwnd, int width, int height) {
        WXAutoLock al(m_mutex);
        if(hwnd && width && height){
            OnOpen(hwnd, width, height);
            return 1;
        }
        return 0;
    }
    virtual int Draw(AVFrame *frame, int bFixed, int rotate) {
        WXAutoLock al(m_mutex);
        if(!m_bOpen)
            return 0;
        if(m_iWidth != frame->width || m_iHeight != frame->height){
            printf("MacImageRender %s  Change Size %dx%d-->%dx%d !!!\n",
            __FUNCTION__,m_iWidth,m_iHeight, frame->width, frame->height);
            m_iWidth  = frame->width;
            m_iHeight = frame->height;
            if(m_context){
                CGContextRelease(m_context);
                m_context = NULL;
            }
            m_frame.Init(AV_PIX_FMT_ABGR, m_iWidth, m_iHeight);
            m_context = CGBitmapContextCreate(m_frame.m_pFrame->data[0], m_iWidth, m_iHeight, 8, m_frame.m_pFrame->linesize[0], m_colorSpace,kCGBitmapByteOrder32Little | kCGImageAlphaNoneSkipLast);
        }
        m_conv.Convert(frame,m_frame.m_pFrame);
        m_bFixed = bFixed;
        OnDraw();
        return 1;
    }
    
    virtual void Close() {
        WXAutoLock al(m_mutex);
        if(strcmp(dispatch_queue_get_label(DISPATCH_CURRENT_QUEUE_LABEL), dispatch_queue_get_label(dispatch_get_main_queue())) == 0) {
            // do something in main thread
            printf("MacImageViewRender %s wait in Main thread\n", __FUNCTION__);
            CloseImpl();
        } else {
            // do something in other thread
            printf("MacImageViewRender %s wait in other thread\n", __FUNCTION__);
            OnClose();
            int64_t ts1 = WXGetTimeMs();
            while(m_bOpen){
				SLEEPMS(10);
            }
            int64_t DelayWait = WXGetTimeMs() - ts1;
            printf("++++++++ MacImageViewRender %s DelayWait=%lld \n", __FUNCTION__,DelayWait);
        }
    }
};


#endif /* MacImageRender_h */

