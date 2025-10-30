
#include "ImageProcess_OpenGL.h"
#include "Utils.hpp"


//OpenGL32.dll
#define  glewGetProcAddress(name)   LibInst::GetInst().m_wglGetProcAddress((LPCSTR)name)
#define  glGetStringi             LibInst::GetInst().m_glGetStringi
#define  glGetIntegerv            LibInst::GetInst().m_glGetIntegerv
#define  glGetString              LibInst::GetInst().m_glGetString
#define  glClear                  LibInst::GetInst().m_glClear
#define  wglCreateContext LibInst::GetInst().m_wglCreateContext
#define wglDeleteContext LibInst::GetInst().m_wglDeleteContext
#define wglGetProcAddress LibInst::GetInst().m_wglGetProcAddress
#define wglGetCurrentDC LibInst::GetInst().m_wglGetCurrentDC
#define wglGetCurrentContext LibInst::GetInst().m_wglGetCurrentContext
#define wglMakeCurrent LibInst::GetInst().m_wglMakeCurrent
#define wglShareLists  LibInst::GetInst().m_wglShareLists

#define glTexImage2D LibInst::GetInst().m_glTexImage2D
#define glDrawBuffer LibInst::GetInst().m_glDrawBuffer
#define glDrawArrays LibInst::GetInst().m_glDrawArrays
#define glDisable LibInst::GetInst().m_glDisable
#define glDrawElements LibInst::GetInst().m_glDrawElements
#define glFinish LibInst::GetInst().m_glFinish
#define glFlush LibInst::GetInst().m_glFlush
#define glReadBuffer LibInst::GetInst().m_glReadBuffer
#define glReadPixels LibInst::GetInst().m_glReadPixels
#define glTexParameteri LibInst::GetInst().m_glTexParameteri
#define glTexSubImage2D LibInst::GetInst().m_glTexSubImage2D
#define glViewport LibInst::GetInst().m_glViewport
#define glBindTexture LibInst::GetInst().m_glBindTexture
#define glCopyTexSubImage2D LibInst::GetInst().m_glCopyTexSubImage2D
#define glGetError LibInst::GetInst().m_glGetError
#define glDeleteTextures LibInst::GetInst().m_glDeleteTextures
#define glGenTextures LibInst::GetInst().m_glGenTextures
#define glBlendFunc LibInst::GetInst().m_glBlendFunc
#define glEnable LibInst::GetInst().m_glEnable

#define glDrawElements LibInst::GetInst().m_glDrawElements

namespace ApowerSoft {
    namespace ImageProcess {

        std::mutex OpenGL::create_mutex{ };
        std::unordered_map<std::string, std::weak_ptr<OpenGL>> OpenGL::OpenGL_objs{ };

        OpenGL::OpenGL() {
            _server = InterInitialize();

            std::cout << "New OpenGL object: " << this << " @ (" << _server << "). (" << _server_holder.use_count() << ")" << std::endl;
			TEXTURE_WRAP_Parameter = GL_MIRRORED_REPEAT;
            SetDuration(1);
            SetTime(0);
            SetStrength(1);
        }

        OpenGL::OpenGL(std::string vertexsource, std::string fshader ) : OpenGL{ } {
            if (_server) {
                _server->invoke(std::function<std::any()>([&] {
                    _shaders.emplace_back(shader_program::create(vertexsource, fshader));
                    return std::any();
                }));
            }
        }

     

        OpenGL::~OpenGL() {
            if (_server) {
                _server->invoke(std::function<std::any()>([&] {
                    _shaders.clear();
                    return std::any();
                }));
                _server.reset();
            }

            std::cout << "~ OpenGL object. (" << _server_holder.use_count() << ")" << std::endl;

            if (!_server_holder_0 && _server_holder && _server_holder.unique()) {
                InterDeinitialize();
            }
        }

        void OpenGL::SetDuration(const float value) {
            SetUniform("iDuration", 1, { value });
        }

        void OpenGL::SetTime(const float value) {
            SetUniform("iTime", 1, { value });
        }

        void OpenGL::SetStrength(const float value) {
            SetUniform("strength", 1, { value });
        }

        std::string OpenGL::GetOpenGLVersion() {
            return _server_holder->Version();
        }

        std::string OpenGL::GetOpenGLLogs() {
            return "";
        }



        void OpenGL::SetUniform(const std::string &name, const int elems_count, const std::initializer_list<GLfloat> &values) {
            SetUniform(-1, name, elems_count, values);
        }

        void OpenGL::SetUniform(const int index, const std::string &name, const int elems_count, const std::initializer_list<GLfloat> &values) {
            _shader_params[index][name] = std::make_tuple(elems_count, std::vector<GLfloat>(values));
        }

        void OpenGL::SetUniformMat(const std::string &name, const int elems_count, const std::initializer_list<GLfloat> &values) {
            //_shader_params[index][name] = std::make_tuple(elems_count, std::vector<GLfloat>(values));
        }

        void ApowerSoft::ImageProcess::OpenGL::GetOpenGLLogs(char* str, int len) {

        }
    }
}

#include <MediaLibAPI.h>
MEDIALIB_API void ImageProcessGetOpenGLLogs(char* str, int len) {

    ApowerSoft::ImageProcess::OpenGL::GetOpenGLLogs(str, len);
}


static int ssa = 0;
MEDIALIB_API int ImageProcessInitialize() {

    if (ssa > 0) {
        return 1;
    }
    ssa++;
	return ApowerSoft::ImageProcess::OpenGL::Initialize();
}

MEDIALIB_API void ImageProcessDeinitialize() {
    ssa--;
    if (ssa <= 0) {
        ApowerSoft::ImageProcess::OpenGL::Deinitialize();
    }
}
