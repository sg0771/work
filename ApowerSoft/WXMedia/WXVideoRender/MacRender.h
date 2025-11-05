//
//  Header.h
//  WXMedia
//
//  Created by tam on 2019/12/10.
//  Copyright © 2019年 xkt.spacexkt.space. All rights reserved.
//

#ifndef Header_h
#define Header_h

#include "WXMediaCpp.h"
#include <libyuv/libyuv.h>
#import  <Cocoa/Cocoa.h>
#import  <OpenGL/OpenGL.h>
#import  <OpenGL/gl.h>


//MacRender Using OpenGLView
class MacRender : public IWXVideRenderImpl {
    NSView *m_view = nil;
    NSOpenGLView  *m_viewImpl = nil;
    WXDataBuffer m_pYUY2Buf;

    int m_bOpen = 0;
    int m_iWidth = 0;
    int m_iHeight = 0;
    GLuint m_texture = 0;
    double m_fRed   = 0.0;
    double m_fGreen = 0.0;
    double m_fBlue  = 0.0;
    double m_fAlpha = 0.0;
    int m_iFixed = 0;
    struct SwsContext *m_pSwsCtx = nullptr;
public:
    virtual int Open(HWND hwnd, int width, int height) {
        WXAutoLock al(m_mutex);
        if(hwnd && width && height){
            dispatch_async(dispatch_get_main_queue(), ^{
                m_view = (__bridge NSView*)hwnd;
                m_iWidth = width;
                m_iHeight = height;
                m_bOpen = 1;
                m_pYUY2Buf.Init(nullptr, m_iWidth * m_iHeight * 2);
                m_viewImpl = [[NSOpenGLView alloc] init];
                [m_view addSubview:m_viewImpl];
                printf("MacRender[%p] Open OK  [%dx%d]!!!\n", this, m_iWidth, m_iHeight);
            });
            return m_bOpen;
        }
        return 0;
    }
    virtual int Draw(AVFrame *frame, int bFixed, int rotate) {
        WXAutoLock al(m_mutex);
        if(!m_bOpen)
            return 0;
        if(frame->format == AV_PIX_FMT_YUV420P){
            libyuv::I420ToYUY2(frame->data[0], frame->linesize[0],
                               frame->data[1], frame->linesize[1],
                               frame->data[2], frame->linesize[2],
                               m_pYUY2Buf.m_pBuf + (m_iHeight - 1) * m_iWidth * 2, -m_iWidth * 2,
                               m_iWidth, m_iHeight);
        }else if(frame->format == AV_PIX_FMT_RGB32){
            libyuv::ARGBToYUY2(frame->data[0], frame->linesize[0],
                               m_pYUY2Buf.m_pBuf + (m_iHeight - 1) * m_iWidth * 2, -m_iWidth * 2,
                               m_iWidth, m_iHeight);
        }else{
            if(m_pSwsCtx == nullptr){
                m_pSwsCtx = sws_getContext(m_iWidth, m_iHeight, (enum AVPixelFormat)frame->format,
                                           m_iWidth, m_iHeight, AV_PIX_FMT_YUYV422,
                                           SWS_FAST_BILINEAR, 0, 0, 0);
            }
            if(m_pSwsCtx){
                uint8_t *pData[4] = { m_pYUY2Buf.m_pBuf,NULL,NULL,NULL};
                int      Pitch[4] = {m_iWidth*2,0,0,0};
                sws_scale(m_pSwsCtx, frame->data, frame->linesize, 0, m_iHeight, pData, Pitch);
            }
        }
        m_iFixed = bFixed;
        
        dispatch_async(dispatch_get_main_queue(), ^{
            
            m_viewImpl.frame = m_view.frame;
            [[m_viewImpl openGLContext] makeCurrentContext];
            
            if(m_texture == 0){
                glGenTextures(1, &m_texture);
                if(m_texture){
                    glEnable(GL_TEXTURE_2D);
                    glBindTexture(GL_TEXTURE_2D, m_texture);
                }
                printf("%p glGenTextures\n",this);
            }
            
            glClearColor(m_fRed, m_fGreen, m_fBlue, m_fAlpha);
            glClear(GL_COLOR_BUFFER_BIT);
            
            glColor3f(0.0f, 1.0f, 0.0f);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_iWidth, m_iHeight, 0, GL_YCBCR_422_APPLE, GL_UNSIGNED_SHORT_8_8_REV_APPLE, m_pYUY2Buf.m_pBuf);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
            
            glBegin(GL_QUADS);
            
            {
                CGFloat sw = (int)m_view.frame.size.width;
                CGFloat sh = (int)m_view.frame.size.height;
                CGFloat desw = sw;
                CGFloat desh = sh;
                CGFloat sw1 = sh * m_iWidth / m_iHeight;
                CGFloat sh1 = sw * m_iHeight / m_iWidth;
                if (sw1 <= sw) {
                    desw = sw1;
                }else {
                    desh = sh1;
                }
                desw /= sw1;
                desh /= sh1;
                glTexCoord2f(1.0f, 1.0f);  glVertex3f(desh,   desw, 0.0f);
                glTexCoord2f(1.0f, 0.0f);  glVertex3f(desh,  -desw, 0.0f);
                glTexCoord2f(0.0f, 0.0f);  glVertex3f(-desh, -desw, 0.0f);
                glTexCoord2f(0.0f, 1.0f);  glVertex3f(-desh,  desw, 0.0f);
            }
            glEnd();
            glFlush();
        });
        return 1;
    }
    
    virtual void Close() {
        WXAutoLock al(m_mutex);
        dispatch_async(dispatch_get_main_queue(), ^{
            if(m_bOpen){
                [m_viewImpl removeFromSuperview];
                [[m_viewImpl openGLContext] makeCurrentContext];
                if(m_texture){
                    glDeleteTextures(1, &m_texture);
                    m_texture = 0;
                    printf("%p glDeleteTextures\n",this);
                }
                m_view = nil;
                
                m_viewImpl = nil;
                m_bOpen = 0;
                m_iWidth = 0;
                m_iHeight = 0;
                printf("MacRender[%p] Close OK!!!\n", this);
            }
        });
    }
};


#endif /* Header_h */

