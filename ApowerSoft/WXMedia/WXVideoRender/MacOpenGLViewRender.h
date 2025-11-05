//
//  Header.h
//  WXMedia
//
//  Created by tam on 2019/12/10.
//  Copyright © 2019年 xkt.spacexkt.space. All rights reserved.
//

#ifndef MacOpenGLViewRender_h
#define MacOpenGLViewRender_h

#include "WXMediaCpp.h"
#include <libyuv/libyuv.h>
#import  <Cocoa/Cocoa.h>
#import  <AppKit/AppKit.h>

#import  <OpenGL/OpenGL.h>
#import  <OpenGL/gl.h>


//MacRender Using OpenGLView
class MacOpenGLViewRender : public IWXVideRenderImpl {
public:
    NSView *m_view = nil;
    NSOpenGLView  *m_viewImpl = nil;
    WXVideoFrame m_frame;
    WXVideoConvert m_conv;
    
    int m_bOpen = 0;
    
    int m_iWidth = 0;
    int m_iHeight = 0;
    GLuint m_texture = 0;
    double m_fRed   = 0.0;
    double m_fGreen = 0.0;
    double m_fBlue  = 0.0;
    double m_fAlpha = 0.0;
    int m_bFixed = 0;
private:
    void OnOpen(HWND hwnd, int width, int height){
        dispatch_async(dispatch_get_main_queue(), ^{
            m_view = (__bridge NSView*)hwnd;
            m_iWidth = width;
            m_iHeight = height;
            m_bOpen = 1;
            m_frame.Init(AV_PIX_FMT_YUYV422, m_iWidth, m_iHeight);
            m_viewImpl = [[NSOpenGLView alloc] init];
            printf("MacOpenGLViewRender %s Open OK  [%dx%d]!!!\n", __FUNCTION__, m_iWidth, m_iHeight);
        });
    }
    
    void OnDraw(){
        dispatch_async(dispatch_get_main_queue(), ^{
            m_viewImpl.frame = NSRect{0,0,m_view.frame.size.width,m_view.frame.size.height};
            [[m_viewImpl openGLContext] makeCurrentContext];
            if(m_texture == 0){
                glGenTextures(1, &m_texture);
                if(m_texture){
                    glEnable(GL_TEXTURE_2D);
                    glBindTexture(GL_TEXTURE_2D, m_texture);
                }
                printf("MacOpenGLViewRender %s glGenTextures\n",__FUNCTION__);
            }
            
            glClearColor(m_fRed, m_fGreen, m_fBlue, m_fAlpha);
            glClear(GL_COLOR_BUFFER_BIT);
            
            glColor3f(0.0f, 1.0f, 0.0f);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_iWidth, m_iHeight, 0,
                         GL_YCBCR_422_APPLE, GL_UNSIGNED_SHORT_8_8_REV_APPLE, m_frame.m_pFrame->data[0]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
            
            glBegin(GL_QUADS);
            
            CGFloat desw = 1.0;
            CGFloat desh = 1.0;
            if(m_bFixed){
                CGFloat sw = (int)m_view.frame.size.width;
                CGFloat sh = (int)m_view.frame.size.height;
                desw = sw;
                desh = sh;
                CGFloat sw1 = sh * m_iWidth / m_iHeight;
                CGFloat sh1 = sw * m_iHeight / m_iWidth;
                if (sw1 <= sw) {
                    desw = sw1;
                }else {
                    desh = sh1;
                }
                desw /= sw1;
                desh /= sh1;
            }
            glTexCoord2f(0.0f, 0.0f);  glVertex3f(-desh,   desw, 0.0f);
            glTexCoord2f(0.0f, 1.0f);  glVertex3f(-desh,  -desw, 0.0f);
            glTexCoord2f(1.0f, 1.0f);  glVertex3f(desh, -desw, 0.0f);
            glTexCoord2f(1.0f, 0.0f);  glVertex3f(desh,  desw, 0.0f);
            glEnd();
            glFlush();
        });
    }
    
    void CloseImpl(){
        if(m_bOpen){
            if(m_texture){
                glDeleteTextures(1, &m_texture);
                m_texture = 0;
                printf("MacOpenGLViewRender %s glDeleteTextures\n",__FUNCTION__);
            }
            if(nil != m_viewImpl){
                [[m_viewImpl openGLContext] makeCurrentContext];
                m_viewImpl = nil;
            }
            m_iWidth = 0;
            m_iHeight = 0;
           printf("-----Tam %s OK!!!\n", __FUNCTION__);
            m_bOpen = 0;
        }
    }
    void OnClose(){
        dispatch_async(dispatch_get_main_queue(), ^{
            CloseImpl();
         });//UI线程
    }
public:
    
    virtual int Open(HWND hwnd, int width, int height) {
        WXAutoLock al(m_mutex);
        if(hwnd && width && height){
            OnOpen(hwnd, width, height);
            return 1;
        }
        return 0;
    }
    
    virtual int Draw(AVFrame *frame, int bFixed, int nRotate) {
        WXAutoLock al(m_mutex);
        if(!m_bOpen)
            return 0;
        if(m_iWidth != frame->width || m_iHeight != frame->height){
            printf("MacOpenGLViewRender %s  Change Size %dx%d-->%dx%d !!!\n",
                   __FUNCTION__,m_iWidth,m_iHeight, frame->width, frame->height);
            m_iWidth  = frame->width;
            m_iHeight = frame->height;
            m_frame.Init(AV_PIX_FMT_YUYV422, m_iWidth, m_iHeight);
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
            printf("MacOpenGLViewRender %s wait in Main thread\n", __FUNCTION__);
            CloseImpl();
        } else {
            // do something in other thread
            printf("MacOpenGLViewRender %s wait in Other thread\n", __FUNCTION__);
            OnClose();
            int64_t ts1 = WXGetTimeMs();
            while(m_bOpen){
				SLEEPMS(10);
            }
            int64_t DelayWait = WXGetTimeMs() - ts1;
            printf("++++++++ MacOpenGLViewRender %s DelayWait=%lld \n", __FUNCTION__,DelayWait);
        }
    }
};

#endif /* MacOpenGLViewRender_h */
