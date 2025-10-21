/*
MACOSX OPENGL RENDER AVFRAME
*/


#include "WXVideoRender.h"
#include "MacImageRender.h"
#include "MacOpenGLViewRender.h"

//支持动态切换分辨率
class WXVideoRender :public IWXVideoRender {
	WXLocker m_mutex;
	WXVideoFrame m_pRGBAFrame;//用 DisplayRGBA。和 GDI+

public://绘制属性

    MacImageRender *m_image = nullptr;
    MacOpenGLViewRender *m_opengl = nullptr;

    NSView *m_view = nil;
    IWXVideRenderImpl *m_mac = nullptr;;

	HWND m_hWnd = nullptr;//显示窗口
	int m_iWidth = 0;  //视频宽度
	int m_iHeight = 0; //视频高度
	int m_bFixed = FALSE; //保持比例输出
	int m_iRotateType = RENDER_ROTATE_NONE;//旋转角度
	int m_bOpen = FALSE;//是否打开成功
	int m_bkColor = 0;//背景色
	int m_iWndWidth = 0;
	int m_iWndHeight = 0;
    
    int m_mode = RENDER_TYPE_OPENGLVIEW;
	WXVideoRender() {

	}
	virtual ~WXVideoRender() {
		Close();
	}
public:

	//数据输入
	virtual void Display(AVFrame *frame) {
		WXAutoLock al(m_mutex);
		if (m_bOpen && frame) { //有数据输入
			if (m_pRGBAFrame.m_pFrame == nullptr) {
				m_pRGBAFrame.Init(AV_PIX_FMT_RGB32, m_iWidth, m_iHeight);// 直接渲染RGBA数据的时候用的
			}
            if(frame->width  == 0 || frame->height == 0){
                return;
            }
			if (m_iWidth == frame->width && m_iHeight == frame->height) { //正常绘制
				m_mac->Draw(frame, m_bFixed, m_iRotateType);
			}
			else { //创新创建渲染
				m_iWidth  = frame->width;
				m_iHeight =  frame->height;
				m_pRGBAFrame.Init(AV_PIX_FMT_RGB32, m_iWidth, m_iHeight);// 直接渲染RGBA数据的时候用的
                
                m_image->Close();
                delete m_image;
                m_opengl->Close();
                delete m_opengl;
                m_mac = nullptr;
                
                SLEEPMS(1000);
                
                m_image = new MacImageRender;
                m_image->Open(m_hWnd, m_iWidth, m_iHeight);
                
                m_opengl = new MacOpenGLViewRender;
                m_opengl->Open(m_hWnd, m_iWidth, m_iHeight);
                
                if(m_mode == RENDER_TYPE_OPENGLVIEW){
                    m_mac = (IWXVideRenderImpl*)m_opengl;
                    dispatch_async(dispatch_get_main_queue(), ^{[m_view addSubview:m_opengl->m_viewImpl];});
                }else{
                    m_mac = (IWXVideRenderImpl*)m_image;
                    dispatch_async(dispatch_get_main_queue(), ^{[m_view addSubview:m_image->m_viewImpl];});
                }
                
				m_mac->Draw(frame, m_bFixed, m_iRotateType);
			}
		}
	}

	virtual void  Close() {
		if (m_bOpen) {
			WXAutoLock al(m_mutex);
            m_image->Close();
            delete m_image;
            m_opengl->Close();
            delete m_opengl;
            m_mac = nullptr;
			m_bOpen = FALSE;
		}
	}

	virtual void SetRotate(int rotate) {
		WXAutoLock al(m_mutex);
        m_iRotateType = std::min(std::max(0, rotate), 3);
	}

	virtual int  Open(HWND view, int width, int height, int bRGBA, int fixed, int rotate) {
		WXAutoLock al(m_mutex);

		WXLogWriteNew("%s", __FUNCTION__);
		m_hWnd = view;
        m_view = (__bridge NSView*)view;
		m_iWidth = width / 2 * 2;
		m_iHeight = height / 2 * 2;
		m_pRGBAFrame.Init(AV_PIX_FMT_RGB32, m_iWidth, m_iHeight);// 直接渲染RGBA数据的时候用的
		m_bFixed = fixed;
		m_iRotateType = rotate;
        
        m_image = new MacImageRender;
        m_image->Open(m_hWnd, m_iWidth, m_iHeight);
        
        m_opengl = new MacOpenGLViewRender;
        m_opengl->Open(m_hWnd, m_iWidth, m_iHeight);

        if(m_mode == RENDER_TYPE_OPENGLVIEW){
            m_mac = (IWXVideRenderImpl*)m_opengl;
            dispatch_async(dispatch_get_main_queue(), ^{[m_view addSubview:m_opengl->m_viewImpl];});
        }else{
            m_mac = (IWXVideRenderImpl*)m_image;
            dispatch_async(dispatch_get_main_queue(), ^{[m_view addSubview:m_image->m_viewImpl];});
        }

        m_bOpen = true;
		return m_bOpen;
	}

	virtual void  DisplayRGBA(uint8_t *pBuf, int Pitch) {
		WXAutoLock al(m_mutex);
		libyuv::ARGBCopy(pBuf, Pitch,
                         m_pRGBAFrame.m_pFrame->data[0], m_pRGBAFrame.m_pFrame->linesize[0],
                         m_iWidth, m_iHeight);
		Display(m_pRGBAFrame.m_pFrame);
	}

	virtual void    ChangeWindowProc(int bSetWndProc) {}
	virtual void    SetBgColor(DWORD color) {}

	virtual void    SetDrawType(int type) {
        WXAutoLock al(m_mutex);
        if (type == -1) {
            m_bFixed = 1;
        }else if (type == -2) {
            m_bFixed = 0;
        }else if (type == RENDER_TYPE_OPENGLVIEW && m_mode != RENDER_TYPE_OPENGLVIEW) {
            m_mode = type;
            m_mac = (IWXVideRenderImpl*)m_opengl;
            dispatch_async(dispatch_get_main_queue(), ^{
                [m_view willRemoveSubview:m_image->m_viewImpl];
                [m_view addSubview:m_opengl->m_viewImpl];
            });
            printf("Using OpenGLView\n");
        }else if (type == RENDER_TYPE_IMAGEVIEW && m_mode != RENDER_TYPE_IMAGEVIEW){
            m_mode = type;
            m_mac = (IWXVideRenderImpl*)m_image;
            dispatch_async(dispatch_get_main_queue(), ^{
                [m_view willRemoveSubview:m_opengl->m_viewImpl];
                [m_view addSubview:m_image->m_viewImpl];
            });
            printf("Using ImageView\n");
        }
	}
	virtual int     GetParam(int mode) { return 0; }
};


//----------------------------------------------------------------------------------------------------------------------
WXMEDIA_API IWXVideoRender *IWXVideoRenderCreate2(HWND hwnd, int width, int height) {
	if (hwnd == nullptr || width <= 0 || height <= 0)return nullptr;
	return IWXVideoRenderCreate(0, hwnd, width, height, FALSE, TRUE, RENDER_ROTATE_NONE);
}

WXMEDIA_API IWXVideoRender *IWXVideoRenderCreate(int type, HWND hwnd, int width, int height, int bRGBA, int fixed, int rotate) {

	if (hwnd == nullptr || width <= 0 || height <= 0)return nullptr;
	WXVideoRender *render = new WXVideoRender;

	if (render->Open(hwnd, width, height, 0, fixed, rotate)) {
		WXLogWriteNewW(L"render->Open OK");
		return (IWXVideoRender*)render;
	}
	return nullptr;
}

WXMEDIA_API IWXVideoRender *IWXVideoRenderCreateAsync(int type, HWND hwnd, int width, int height, int bRGBA, int fixed, int rotate) {
	if (hwnd == nullptr || width <= 0 || height <= 0)return nullptr;
	return IWXVideoRenderCreate(0, hwnd, width, height, FALSE, fixed, RENDER_ROTATE_NONE);
}

WXMEDIA_API void IWXVideoRenderDestroy(IWXVideoRender*p) {
	if (p) {
		WXVideoRender *render = (WXVideoRender *)p;
		delete render;
	}
}

//视频显示 C Styte Interface
//2019.12.12
WXMEDIA_API void *WXVideoRenderCreate(HWND hWnd, int width, int height) {
    WXVideoRender *render = new WXVideoRender;
    if (render->Open(hWnd, width, height, 0, 0, 0)) {
        WXLogWriteNew("%s OK",__FUNCTION__);
        return (void*)render;
    }
    delete render;
    return nullptr;
}

WXMEDIA_API void WXVideoRenderDestroy(void* ptr) {
    if (ptr) {
        WXVideoRender *render = (WXVideoRender*)ptr;
        render->Close();
        delete render;
    }
}

WXMEDIA_API void WXVideoRenderDisplay(void* ptr, struct AVFrame *frame, int bFixed, int nRotate) {
    if (ptr) {
        WXVideoRender *render = (WXVideoRender*)ptr;
        render->SetDrawType(bFixed ? -1 : -2);
        render->SetRotate(nRotate);
        render->Display(frame);
    }
}

//默认会按机器性能分别初始化 D3DX D3D GDI
//如果机器异常，可以通过调用下面的接口来强制调用某个API
WXMEDIA_API void WXVideoRenderChangeMode(void* ptr, int mode) {
    if (ptr) {
        WXVideoRender *render = (WXVideoRender*)ptr;
        render->SetDrawType(av_clip(mode,RENDER_TYPE_OPENGLVIEW, RENDER_TYPE_IMAGEVIEW));
    }
}

