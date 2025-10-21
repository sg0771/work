
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
#define glClearColor LibInst::GetInst().m_glClearColor
#define glGetFloatv LibInst::GetInst().m_glGetFloatv
#define glTexParameterfv LibInst::GetInst().m_glTexParameterfv

#define glDrawElements LibInst::GetInst().m_glDrawElements

namespace ApowerSoft {
    namespace ImageProcess {

        std::atomic_int OpenGL::Server::context_state { };
        std::mutex OpenGL::Server::context_state_mutex { };
        std::condition_variable OpenGL::Server::context_state_cv { };

        std::atomic_flag OpenGL::Server::context_is_building { };
        std::atomic<std::thread::id> OpenGL::Server::context_thread_id { };

        std::shared_ptr<OpenGL::Server> OpenGL::_server_holder_0 { };
        std::shared_ptr<OpenGL::Server> OpenGL::_server_holder { };
        std::shared_ptr<shader_program> OpenGL::_default_shader { };

        std::string OpenGL::SupportedVBO { };
        std::string OpenGL::SupportedFBO { };
        std::string OpenGL::SupportedPBO { };
        std::string OpenGL::SupportedImmutableBuffer { };
        std::string OpenGL::SupportedImmutableTexture { };
        std::string OpenGL::SupportedAnisotropic { };

        std::function<void(GLsizei, GLuint*)> OpenGL::fnglGenBuffers { };
        std::function<void(GLenum, GLuint)> OpenGL::fnglBindBuffer { };
        std::function<void(GLsizei, const GLuint*)> OpenGL::fnglDeleteBuffers { };
        std::function<void(GLenum, GLsizeiptr, const void*, GLenum)> OpenGL::fnglBufferData { };
        std::function<void(GLenum, GLintptr, GLsizeiptr, const void*)> OpenGL::fnglBufferSubData { };
        std::function<void(GLenum, GLintptr, GLsizeiptr, void*)> OpenGL::fnglGetBufferSubData { };
        std::function<void*(GLenum, GLenum)> OpenGL::fnglMapBuffer { };
        std::function<GLboolean(GLenum)> OpenGL::fnglUnmapBuffer { };

        std::function<void(GLenum, GLsizei, GLenum, GLsizei, GLsizei)> OpenGL::fnglTexStorage2D { };
        std::function<void(GLenum, GLsizeiptr, const void *, GLbitfield)> OpenGL::fnglBufferStorage { };
        std::function<void*(GLenum, GLintptr, GLsizeiptr, GLbitfield)> OpenGL::fnglMapBufferRange { };

        std::function<void(GLsizei, GLuint*)> OpenGL::fnglGenTextures { };
        std::function<void(GLenum, GLuint)> OpenGL::fnglBindTexture { };
        std::function<void(GLsizei, const GLuint*)> OpenGL::fnglDeleteTextures { };
        std::function<void(GLenum)> OpenGL::fnglActiveTexture { };

        std::function<void(GLsizei, GLuint*)> OpenGL::fnglGenRenderbuffers { };
        std::function<void(GLenum, GLuint)> OpenGL::fnglBindRenderbuffer { };
        std::function<void(GLsizei, const GLuint*)> OpenGL::fnglDeleteRenderbuffers { };
        std::function<void(GLenum, GLenum, GLsizei, GLsizei)> OpenGL::fnglRenderbufferStorage { };

        std::function<void(GLsizei, GLuint*)> OpenGL::fnglGenFramebuffers { };
        std::function<void(GLenum, GLuint)> OpenGL::fnglBindFramebuffer { };
        std::function<void(GLsizei, const GLuint*)> OpenGL::fnglDeleteFramebuffers { };
        std::function<void(GLenum, GLenum, GLenum, GLuint)> OpenGL::fnglFramebufferRenderbuffer { };
        std::function<void(GLenum, GLenum, GLenum, GLuint, GLint)> OpenGL::fnglFramebufferTexture2D { };
        std::function<GLenum(GLenum)> OpenGL::fnglCheckFramebufferStatus { };

        std::function<GLuint(GLenum)> OpenGL::fnglCreateShader { };
        std::function<void(GLuint)> OpenGL::fnglDeleteShader { };
        std::function<void(GLuint, GLsizei, const GLchar *const*, const GLint*)> OpenGL::fnglShaderSource { };
        std::function<void(GLuint)> OpenGL::fnglCompileShader { };
        std::function<void(GLuint, GLenum, GLint*)> OpenGL::fnglGetShaderiv { };
        std::function<void(GLuint, GLsizei, GLsizei*, GLchar*)> OpenGL::fnglGetShaderInfoLog { };
        std::function<GLuint()> OpenGL::fnglCreateProgram { };
        std::function<void(GLuint)> OpenGL::fnglDeleteProgram { };
        std::function<void(GLuint)> OpenGL::fnglLinkProgram { };
        std::function<void(GLuint)> OpenGL::fnglUseProgram { };
        std::function<void(GLuint, GLenum, GLint*)> OpenGL::fnglGetProgramiv { };
        std::function<void(GLuint, GLsizei, GLsizei*, GLchar*)> OpenGL::fnglGetProgramInfoLog { };
        std::function<void(GLuint, GLuint)> OpenGL::fnglAttachShader { };
        std::function<GLuint(GLuint, const GLchar*)> OpenGL::fnglGetAttribLocation { };
        std::function<GLuint(GLuint, const GLchar*)> OpenGL::fnglGetUniformLocation { };
        std::function<void(GLuint)> OpenGL::fnglEnableVertexAttribArray { };
        std::function<void(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*)> OpenGL::fnglVertexAttribPointer { };
        std::function<void(GLint, GLint)> OpenGL::fnglUniform1i { };
        std::function<void(GLint, GLsizei, const GLint*)> OpenGL::fnglUniform1iv { };
        std::function<void(GLint, GLsizei, const GLint*)> OpenGL::fnglUniform2iv { };
        std::function<void(GLint, GLsizei, const GLint*)> OpenGL::fnglUniform3iv { };
        std::function<void(GLint, GLsizei, const GLint*)> OpenGL::fnglUniform4iv { };
        std::function<void(GLint, GLsizei, const GLfloat*)> OpenGL::fnglUniform1fv { };
        std::function<void(GLint, GLsizei, const GLfloat*)> OpenGL::fnglUniform2fv { };
        std::function<void(GLint, GLsizei, const GLfloat*)> OpenGL::fnglUniform3fv { };
        std::function<void(GLint, GLsizei, const GLfloat*)> OpenGL::fnglUniform4fv { };

        OpenGL::Server::Server() : _window{ nullptr }, _vbo{ 0 }, _anisotropy{ 0 } {
            try {
                init_context();

                init_extensions();

                glDisable(GL_DEPTH_TEST);
                glDisable(GL_STENCIL_TEST);
                glEnable(GL_TEXTURE_2D);

                glClearColor(0, 0, 0, 0);

                if (GLEW_ARB_texture_filter_anisotropic) {
                    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &_anisotropy);
                }
                else if (GLEW_EXT_texture_filter_anisotropic) {
                    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &_anisotropy);
                }

                init_vertex();
            }
            catch (const std::exception& ex) {
                std::cerr << ex.what() << std::endl;
               
                processor.stop();
                if (fnglDeleteBuffers) {
                    fnglDeleteBuffers(1, &_vbo);
                }
                if (_func_cleanup_context) {
                    _func_cleanup_context();
                }

                throw;
            }

            context_thread_id = std::this_thread::get_id();
            std::cout << "new OpenGL server: " << context_thread_id << ". (" << this << ")" << std::endl;
        }

        OpenGL::Server::~Server() {
            context_thread_id = std::thread::id{ };

            processor.stop();
            if (fnglDeleteBuffers) {
                fnglDeleteBuffers(1, &_vbo);
            }
            if (_func_cleanup_context) {
                _func_cleanup_context();
            }

            std::cout << "~ OpenGL server: " << std::this_thread::get_id() << ". (" << this << ")" << std::endl;
        }

        void OpenGL::Server::init_context() {
            static const char * title = "ApowerSoft::ImageProcess";

            std::ostringstream oss;

            //glfwSetErrorCallback([](int code, const char* msg) {
            //    std::ostringstream os;
            //    os << "glfw ERROR: " << code << ", " << msg << std::endl;
            //    std::cerr << os.str();
            //});

            if (glfwInit() == GLFW_TRUE) {
                std::ostringstream os;
                
                const auto ctx = glfwGetCurrentContext();
                glfwDefaultWindowHints();
                glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
#ifdef __APPLE__
                glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
                glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
                glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
                glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
                const auto wnd = glfwCreateWindow(1, 1, title, nullptr, ctx);
                if (wnd != nullptr) {
                    glfwMakeContextCurrent(wnd);

                    _window = wnd;
                    os << "glfw Window:" << _window << std::endl;
                    std::cout << os.str();
                    _func_cleanup_context = [=] {
                        glfwDestroyWindow(wnd);
                        glfwTerminate();
                    };
                }
                else {
                    os << "glfwCreateWindow failed." << std::endl;
                    std::cerr << os.str();

                }
            }
            else {
                std::ostringstream os;
                os << "glfwInit failed." << std::endl;
                std::cerr << os.str();
            }


            if (_window == nullptr) {
                std::ostringstream os;
#ifdef _WIN32
                HWND wnd = nullptr;
                HDC dc = nullptr;
                HGLRC rc = nullptr;

                do {
                    WNDCLASSA wc;
                    PIXELFORMATDESCRIPTOR pfd;

                    ZeroMemory(&wc, sizeof(WNDCLASSA));
                    wc.hInstance = GetModuleHandle(nullptr);
                    wc.lpfnWndProc = DefWindowProc;
                    wc.lpszClassName = title;
                    if (0 == RegisterClassA(&wc)) break;

                    wnd = CreateWindowA(title, title, 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
                    if (nullptr == wnd) break;

                    dc = GetDC(wnd);
                    if (nullptr == dc) break;

                    ZeroMemory(&pfd, sizeof(PIXELFORMATDESCRIPTOR));
                    int pixelformat = -1;
                    if (pixelformat == -1) {
                        pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
                        pfd.nVersion = 1;
                        pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
                        pixelformat = ChoosePixelFormat(dc, &pfd);
                        if (pixelformat == 0) break;
                    }

                    if (FALSE == SetPixelFormat(dc, pixelformat, &pfd)) break;

                    rc = wglCreateContext(dc);

					glEnable(GL_BLEND);
					glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                    if (nullptr == rc) break;
                    if (FALSE == wglMakeCurrent(dc, rc)) break;

                    _window = wnd;
                    os << "native Window:" << _window << std::endl;
                    std::cout << os.str();
                    _func_cleanup_context = [=] {
                        if (nullptr != rc) wglMakeCurrent(nullptr, nullptr);
                        if (nullptr != rc) wglDeleteContext(rc);
                        if (nullptr != wnd && nullptr != dc) ReleaseDC(wnd, dc);
                        if (nullptr != wnd) DestroyWindow(wnd);
                        UnregisterClassA(title, GetModuleHandle(nullptr));
                    };
                } while (false);
#endif
            }

            oss << "OpenGL context: " << _window << std::endl;

            std::cout << oss.str();



            if (_window == nullptr) {
                throw std::runtime_error("OpenGL error: context not valid.");
            }
        }

        void OpenGL::Server::init_extensions() {
            glewExperimental = GL_TRUE;
            GLenum err = glewInit();
            if (err != GLEW_OK) {
                std::ostringstream os;
                os << "glewInit failed: " << glewGetErrorString(err) << std::endl;
                std::cerr << os.str();

                throw std::runtime_error(os.str());
            }

            _version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
            if (_version.empty()) {
                GLint value1, value2;
                glGetIntegerv(GL_MAJOR_VERSION, &value1);
                glGetIntegerv(GL_MINOR_VERSION, &value2);

                std::ostringstream os;
                os << "int = " << value1 << "." << value2;
                _version = os.str();
            }

            SupportedVBO = GLEW_VERSION_1_5 ? "1.5" : GLEW_ARB_vertex_buffer_object ? "ARB" : "";
            SupportedFBO = GLEW_ARB_framebuffer_object ? "ARB" : GLEW_EXT_framebuffer_object ? "EXT" : "";
            SupportedPBO = GLEW_ARB_pixel_buffer_object ? "ARB" : GLEW_EXT_pixel_buffer_object ? "EXT" : "";
            SupportedImmutableBuffer = GLEW_ARB_buffer_storage ? "ARB" : GLEW_EXT_buffer_storage ? "EXT" : "";
            SupportedImmutableTexture = GLEW_ARB_texture_storage ? "ARB" : GLEW_EXT_texture_storage ? "EXT" : "";
            SupportedAnisotropic = GLEW_ARB_texture_filter_anisotropic ? "ARB" : GLEW_EXT_texture_filter_anisotropic ? "EXT" : "";

            std::ostringstream oss;
            oss << std::boolalpha;

            oss << "OpenGL version: " << _version << std::endl;
            oss << "OpenGL vendor: " << reinterpret_cast<const char *>(glGetString(GL_VENDOR)) << std::endl;
            oss << "OpenGL renderer: " << reinterpret_cast<const char *>(glGetString(GL_RENDERER)) << std::endl;

            GLint value;
            glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &value);
            oss << "OpenGL MAX_TEXTURE_IMAGE_UNITS: " << value << std::endl;
            glGetIntegerv(GL_MAX_TEXTURE_SIZE, &value);
            oss << "OpenGL MAX_TEXTURE_SIZE: " << value << std::endl;

            oss << "OpenGL support: 1.1 = " << static_cast<bool>(GLEW_VERSION_1_1) << std::endl;
            oss << "OpenGL support: 1.3 = " << static_cast<bool>(GLEW_VERSION_1_3) << std::endl;
            oss << "OpenGL support: 1.5 = " << static_cast<bool>(GLEW_VERSION_1_5) << std::endl;
            oss << "OpenGL support: 2.0 = " << static_cast<bool>(GLEW_VERSION_2_0) << std::endl;
            oss << "OpenGL support: 3.0 = " << static_cast<bool>(GLEW_VERSION_3_0) << std::endl;
            oss << "OpenGL support: 4.4 = " << static_cast<bool>(GLEW_VERSION_4_4) << std::endl;


            oss << "OpenGL support: vbo = " << SupportedVBO << std::endl;
            oss << "OpenGL support: fbo = " << SupportedFBO << std::endl;
            oss << "OpenGL support: pbo = " << SupportedPBO << std::endl;
            oss << "OpenGL support: immutable buffer = " << SupportedImmutableBuffer << std::endl;
            oss << "OpenGL support: immutable texture = " << SupportedImmutableTexture << std::endl;
            oss << "OpenGL support: anisotropic = " << SupportedAnisotropic << std::endl;

            if (GLEW_VERSION_1_1) {
                fnglGenTextures = glGenTextures;
                fnglBindTexture = glBindTexture;
                fnglDeleteTextures = glDeleteTextures;
            }
            else if (GLEW_EXT_texture_object) {
                fnglGenTextures = glGenTexturesEXT;
                fnglBindTexture = glBindTextureEXT;
                fnglDeleteTextures = glDeleteTexturesEXT;
            }

            if (GLEW_VERSION_1_3) {
                fnglActiveTexture = glActiveTexture;
            }
            else if (GLEW_ARB_multitexture) {
                fnglActiveTexture = glActiveTextureARB;
            }

            if (GLEW_VERSION_1_5) {
                fnglGenBuffers = glGenBuffers;
                fnglBindBuffer = glBindBuffer;
                fnglDeleteBuffers = glDeleteBuffers;
                fnglBufferData = glBufferData;
                fnglBufferSubData = glBufferSubData;
                fnglGetBufferSubData = glGetBufferSubData;
                fnglMapBuffer = glMapBuffer;
                fnglUnmapBuffer = glUnmapBuffer;
            }
            else if (GLEW_ARB_vertex_buffer_object) {
                fnglGenBuffers = glGenBuffersARB;
                fnglBindBuffer = glBindBufferARB;
                fnglDeleteBuffers = glDeleteBuffersARB;
                fnglBufferData = glBufferDataARB;
                fnglBufferSubData = glBufferSubDataARB;
                fnglGetBufferSubData = glGetBufferSubDataARB;
                fnglMapBuffer = glMapBufferARB;
                fnglUnmapBuffer = glUnmapBufferARB;
            }

            if (GLEW_VERSION_2_0) {
                fnglCreateShader = glCreateShader;
                fnglDeleteShader = glDeleteShader;
                fnglShaderSource = glShaderSource;
                fnglCompileShader = glCompileShader;
                fnglGetShaderiv = glGetShaderiv;
                fnglGetShaderInfoLog = glGetShaderInfoLog;
                fnglCreateProgram = glCreateProgram;
                fnglDeleteProgram = glDeleteProgram;
                fnglLinkProgram = glLinkProgram;
                fnglUseProgram = glUseProgram;
                fnglGetProgramiv = glGetProgramiv;
                fnglGetProgramInfoLog = glGetProgramInfoLog;
                fnglAttachShader = glAttachShader;
                fnglGetAttribLocation = glGetAttribLocation;
                fnglGetUniformLocation = glGetUniformLocation;
                fnglEnableVertexAttribArray = glEnableVertexAttribArray;
                fnglVertexAttribPointer = glVertexAttribPointer;
                fnglUniform1i = glUniform1i;
                fnglUniform1iv = glUniform1iv;
                fnglUniform2iv = glUniform2iv;
                fnglUniform3iv = glUniform3iv;
                fnglUniform4iv = glUniform4iv;
                fnglUniform1fv = glUniform1fv;
                fnglUniform2fv = glUniform2fv;
                fnglUniform3fv = glUniform3fv;
                fnglUniform4fv = glUniform4fv;
            }

            if (GLEW_ARB_framebuffer_object) {
                fnglGenRenderbuffers = glGenRenderbuffers;
                fnglBindRenderbuffer = glBindRenderbuffer;
                fnglDeleteRenderbuffers = glDeleteRenderbuffers;
                fnglRenderbufferStorage = glRenderbufferStorage;

                fnglGenFramebuffers = glGenFramebuffers;
                fnglBindFramebuffer = glBindFramebuffer;
                fnglDeleteFramebuffers = glDeleteFramebuffers;
                fnglFramebufferRenderbuffer = glFramebufferRenderbuffer;
                fnglFramebufferTexture2D = glFramebufferTexture2D;
                fnglCheckFramebufferStatus = glCheckFramebufferStatus;
            }
           /* else if (GLEW_EXT_framebuffer_object) {
                fnglGenRenderbuffers = glGenRenderbuffersEXT;
                fnglBindRenderbuffer = glBindRenderbufferEXT;
                fnglDeleteRenderbuffers = glDeleteRenderbuffersEXT;
                fnglRenderbufferStorage = glRenderbufferStorageEXT;

                fnglGenFramebuffers = glGenFramebuffersEXT;
                fnglBindFramebuffer = glBindFramebufferEXT;
                fnglDeleteFramebuffers = glDeleteFramebuffersEXT;
                fnglFramebufferRenderbuffer = glFramebufferRenderbufferEXT;
                fnglFramebufferTexture2D = glFramebufferTexture2DEXT;
                fnglCheckFramebufferStatus = glCheckFramebufferStatusEXT;
            }*/

            if (GLEW_ARB_texture_storage) {
                fnglTexStorage2D = glTexStorage2D;
            }
            else if (GLEW_EXT_texture_storage) {
                fnglTexStorage2D = glTexStorage2DEXT;
            }

            if (GLEW_ARB_buffer_storage) {
                fnglBufferStorage = glBufferStorage;
            }
            else if (GLEW_EXT_buffer_storage) {
                fnglBufferStorage = glBufferStorageEXT;
            }

            if (GLEW_ARB_map_buffer_range) {
                fnglMapBufferRange = glMapBufferRange;
            }
            else if (GLEW_EXT_map_buffer_range) {
                fnglMapBufferRange = glMapBufferRangeEXT;
            }

			if (GLEW_VERSION_3_0)
			{
				shader_program::setShaderVersion("#version 130");
			}
			else
			{
				shader_program::setShaderVersion ("#version 120");
			}

            int count_not_supported = 0;

#define CheckAPIRequired(name) if (!name) { oss << "OpenGL not supported: " << #name << std::endl; count_not_supported++; }
#define CheckAPI(name) if (!name) { oss << "OpenGL not supported: " << #name << std::endl; }

            CheckAPIRequired(fnglGenBuffers);
            CheckAPIRequired(fnglBindBuffer);
            CheckAPIRequired(fnglDeleteBuffers);
            CheckAPIRequired(fnglBufferData);

            CheckAPI(fnglBufferSubData);
            CheckAPI(fnglGetBufferSubData);
            CheckAPI(fnglMapBuffer);
            CheckAPI(fnglUnmapBuffer);

            CheckAPI(fnglTexStorage2D);
            CheckAPI(fnglBufferStorage);
            CheckAPI(fnglMapBufferRange);

            CheckAPIRequired(fnglGenTextures);
            CheckAPIRequired(fnglBindTexture);
            CheckAPIRequired(fnglDeleteTextures);
            CheckAPIRequired(fnglActiveTexture);

            CheckAPIRequired(fnglGenRenderbuffers);
            CheckAPIRequired(fnglBindRenderbuffer);
            CheckAPIRequired(fnglDeleteRenderbuffers);
            CheckAPIRequired(fnglRenderbufferStorage);

            CheckAPIRequired(fnglGenFramebuffers);
            CheckAPIRequired(fnglBindFramebuffer);
            CheckAPIRequired(fnglDeleteFramebuffers);
            CheckAPIRequired(fnglFramebufferRenderbuffer);
            CheckAPIRequired(fnglFramebufferTexture2D);
            CheckAPIRequired(fnglCheckFramebufferStatus);

            CheckAPIRequired(fnglCreateShader);
            CheckAPIRequired(fnglDeleteShader);
            CheckAPIRequired(fnglShaderSource);
            CheckAPIRequired(fnglCompileShader);
            CheckAPIRequired(fnglGetShaderiv);
            CheckAPIRequired(fnglGetShaderInfoLog);
            CheckAPIRequired(fnglCreateProgram);
            CheckAPIRequired(fnglDeleteProgram);
            CheckAPIRequired(fnglLinkProgram);
            CheckAPIRequired(fnglUseProgram);
            CheckAPIRequired(fnglGetProgramiv);
            CheckAPIRequired(fnglGetProgramInfoLog);
            CheckAPIRequired(fnglAttachShader);
            CheckAPIRequired(fnglGetAttribLocation);
            CheckAPIRequired(fnglGetUniformLocation);
            CheckAPIRequired(fnglEnableVertexAttribArray);
            CheckAPIRequired(fnglVertexAttribPointer);
            CheckAPIRequired(fnglUniform1i);
            CheckAPIRequired(fnglUniform1iv);
            CheckAPIRequired(fnglUniform2iv);
            CheckAPIRequired(fnglUniform3iv);
            CheckAPIRequired(fnglUniform4iv);
            CheckAPIRequired(fnglUniform1fv);
            CheckAPIRequired(fnglUniform2fv);
            CheckAPIRequired(fnglUniform3fv);
            CheckAPIRequired(fnglUniform4fv);

            std::cout << oss.str() << std::endl;

            if (count_not_supported > 0) {
                throw std::runtime_error("OpenGL error: Required API not supported.");
            }
        }

        void OpenGL::Server::init_vertex() {
			//Log_info("begin:%s", __FUNCTION__);
            static const float vertex_fan[] = {
                -1.0f,-1.0f,
                1.0f,-1.0f,
                1.0f,1.0f,
                -1.0f,1.0f,
            };

            fnglGenBuffers(1, &_vbo);
            fnglBindBuffer(GL_ARRAY_BUFFER, _vbo);
            ON_SCOPE_EXIT{
                    fnglBindBuffer(GL_ARRAY_BUFFER, 0);
            };
            fnglBufferData(GL_ARRAY_BUFFER, sizeof vertex_fan, vertex_fan, GL_STATIC_DRAW);
        }

        bool OpenGL::Server::gen_texture(GLuint &_texture, const int width, const int height, const int bytes_of_color /*= 1*/, cpbyte pixels /*= nullptr*/) {
            _texture = 0;
            const auto result = invoke(std::function<std::any()>([&] {
                GLuint texture { };

                fnglGenTextures(1, &texture);
                fnglBindTexture(GL_TEXTURE_2D, texture);
                ON_SCOPE_EXIT {
                        fnglBindTexture(GL_TEXTURE_2D, 0);
                    };

                //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

                glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, std::data({ 0.0f, 0.0f, 0.0f, 0.0f }));

                //glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                //if (GLEW_ARB_texture_filter_anisotropic) {
                //    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, _anisotropy);
                //}
                //else if (GLEW_EXT_texture_filter_anisotropic) {
                //    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, _anisotropy);
                //}
                
                if (SupportedPBO.empty()) {
                    if (SupportedImmutableTexture.empty()) {
                        glTexImage2D(GL_TEXTURE_2D, 0, get_pix_format_internal(bytes_of_color), width, height, 0, get_pix_format(bytes_of_color), GL_UNSIGNED_BYTE, pixels);
                    }
                    else {
                        fnglTexStorage2D(GL_TEXTURE_2D, std::floor(std::log2(std::max(width, height))) + 1, get_pix_format_internal(bytes_of_color), width, height);

                        if (pixels) {
                            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, get_pix_format(bytes_of_color), GL_UNSIGNED_BYTE, pixels);
                        }
                    }
                }
                else {
                    if (SupportedImmutableTexture.empty()) {
                        glTexImage2D(GL_TEXTURE_2D, 0, get_pix_format_internal(bytes_of_color), width, height, 0, get_pix_format(bytes_of_color), GL_UNSIGNED_BYTE, nullptr);
                    }
                    else {
                        fnglTexStorage2D(GL_TEXTURE_2D, std::floor(std::log2(std::max(width, height))) + 1, get_pix_format_internal(bytes_of_color), width, height);
                    }

                    if (pixels) {
                        GLuint pbo{ };

                        fnglGenBuffers(1, &pbo);
                        fnglBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
                        ON_SCOPE_EXIT{
                                fnglBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
                                fnglDeleteBuffers(1, &pbo);
                        };

                        const auto size = width * height * bytes_of_color;
                        fnglBufferData(GL_PIXEL_UNPACK_BUFFER, size, nullptr, GL_STATIC_DRAW);

                        void *ptr = nullptr;
                        if (fnglMapBuffer && (ptr = fnglMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY))) {
                            ON_SCOPE_EXIT{
                                    fnglUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
                            };

                            std::memcpy(ptr, pixels, size);
                        }
                        else {
                            fnglBufferSubData(GL_PIXEL_UNPACK_BUFFER, 0, size, pixels);
                        }

                        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, get_pix_format(bytes_of_color), GL_UNSIGNED_BYTE, nullptr);
                    }
                }

                _texture = texture;
				//Log_info("end:%s", __FUNCTION__);
                return true;
            }));
#if !defined(_WIN32) && (defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)))
            return result.empty() ? false : boost::any_cast<bool>(result);
#else
            return !result.has_value() ? false : std::any_cast<bool>(result);
#endif
        }

        bool OpenGL::Server::gen_fbo_with_buffer(GLuint &_fbo, GLuint &_buffer, const int width, const int height, const int bytes_of_color /*= 1*/) {
            _fbo = 0;
            _buffer = 0;

            const auto result = invoke(std::function<std::any()>([&] {
                GLuint fbo { }, renderbuffer { };

                fnglGenFramebuffers(1, &fbo);
                fnglBindFramebuffer(GL_FRAMEBUFFER, fbo);
                ON_SCOPE_EXIT {
                        fnglBindFramebuffer(GL_FRAMEBUFFER, 0);
                    };

                fnglGenRenderbuffers(1, &renderbuffer);
                fnglBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
                ON_SCOPE_EXIT {
                        fnglBindRenderbuffer(GL_RENDERBUFFER, 0);
                    };

                fnglRenderbufferStorage(GL_RENDERBUFFER, get_pix_format_internal(bytes_of_color), width, height);

                fnglFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, renderbuffer);

                if (fnglCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                    return false;
                }

                _fbo = fbo;
                _buffer = renderbuffer;

                return true;
            }));
#if !defined(_WIN32) && (defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)))
            return result.empty() ? false : boost::any_cast<bool>(result);
#else
            return !result.has_value() ? false : std::any_cast<bool>(result);
#endif
        }
		
        
        GLenum OpenGL::Server::get_pix_format_internal(const int bytes_of_color /*= 3*/) {
            return bytes_of_color <= 1 ? GL_R8 : bytes_of_color == 4 ? GL_RGBA8 : GL_RGB8;
        }

        GLenum OpenGL::Server::get_pix_format(const int bytes_of_color /*= 3*/) {
            return bytes_of_color <= 1 ? GL_RED : bytes_of_color == 4 ? GL_RGBA : GL_RGB;
        }

        std::any OpenGL::Server::invoke(const std::function<std::any()> &func) {

			

            if (std::this_thread::get_id() == context_thread_id) {
                return func();
            }

            auto task = std::packaged_task<std::any()> { func };
            auto future = task.get_future();
            processor.push(std::move(task));
            return future.get();
        }

        GLuint OpenGL::Server::VBO() const {
            return _vbo;
        }

        std::string OpenGL::Server::Version() const {
            return _version;
        }

        std::initializer_list<float> OpenGL::get_datetime() {
            auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            std::tm tm { };
#if !defined(_WIN32) && (defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)))
            localtime_r(&now, &tm);
#else
            localtime_s(&tm, &now);
#endif

            return { tm.tm_year + 1900.f, tm.tm_mon + 1.f, static_cast<float>(tm.tm_mday), tm.tm_hour * 3600.f + tm.tm_min * 3600.f + tm.tm_sec };
        }

        std::shared_ptr<OpenGL::Server> OpenGL::InterInitialize() {
			//Log_info("begin:%s", __FUNCTION__);
            std::cout << "InterInitialize." << std::endl;

            if (!_server_holder) {
                if (!Server::context_is_building.test_and_set()) {
                    if (!_server_holder) {
                        {
                            std::lock_guard<std::mutex> lock(Server::context_state_mutex);
                            Server::context_state = 1;
                        }
                        Server::context_state_cv.notify_all();

                        std::thread {
                            [&] {
                                ON_SCOPE_EXIT {
                                        std::cout << "Server thread exited." << std::this_thread::get_id() << std::endl;
                                        _server_holder.reset();

                                        {
                                            std::lock_guard<std::mutex> lock(Server::context_state_mutex);
                                            Server::context_state = 0;
                                        }
                                        Server::context_state_cv.notify_all();
                                    };

								{
									std::shared_ptr<Server> server;

									ON_SCOPE_EXIT {
											std::cout << "Server create finished." << std::this_thread::get_id() << std::endl;
											_server_holder = server;
											Server::context_is_building.clear();

											{
												std::lock_guard<std::mutex> lock(Server::context_state_mutex);
												Server::context_state = server ? 2 : 0;
											}
											Server::context_state_cv.notify_all();
										};

									try {
										server = std::make_shared<Server>();
                                    } catch (const std::exception& ex) {
									  std::cerr << ex.what() << std::endl;
									  server.reset();
									}

									auto ntry = 5;
									while (!server && ntry-- > 0) {
										std::this_thread::sleep_for(std::chrono::milliseconds(200));

										try {
											server = std::make_shared<Server>();
                                        } catch (const std::exception& ex) {
                                            std::cerr << ex.what() << std::endl;
                                            server.reset();
                                        }
                                    }
                                }

                                if (_server_holder) {
                                    try {
                                        _default_shader = shader_program::create();
                                    }
                                    catch (...) {
                                        MessageBox(NULL, L"sorry , opengl error", L"", MB_OK);
                                        _server_holder.reset();
                                        _server_holder = NULL;
                                        return;
                                    }
                                    ON_SCOPE_EXIT{
												_default_shader.reset();
											};

										try {
											while (_server_holder && _server_holder->processor.is_ok()) {
												std::packaged_task<std::any()> task;
												if (_server_holder->processor.wait_and_pop(task) && _server_holder && _server_holder->processor.is_ok()) {
													task();
												}
											}
                                    } catch (const std::exception& ex) {
                                        std::cerr << ex.what() << std::endl;
                                    }                                    
                                }
                            }
                        }.detach();
                    } else {
                        Server::context_is_building.clear();
                    }
                }

                {
                    std::unique_lock<std::mutex> lock(Server::context_state_mutex);
                    Server::context_state_cv.wait(lock, []{ return Server::context_state == 2 || Server::context_state == 0; });
                }
            }

            if (_server_holder == NULL)
                MessageBox(NULL, L"sorry ,your graphic card driver is not support current version, please update it[OPENGL]", L"", MB_OK);

            return _server_holder;
        }

        void OpenGL::InterDeinitialize() {
			//Log_info("begin:%s", __FUNCTION__);
            if (_server_holder) {
                _server_holder->invoke(std::function<std::any()>([&] {
                    _server_holder.reset();
                    return std::any();
                }));

                {
                    std::unique_lock<std::mutex> lock(Server::context_state_mutex);
                    Server::context_state_cv.wait(lock, []{ return Server::context_state == 0; });
                }
            }
        }

        int OpenGL::Initialize() {
			return 1;
            auto ret = (_server_holder_0 = InterInitialize()) ? 1 : 0;

            return ret;
        }

        void OpenGL::Deinitialize() {
			return ;
			//Log_info("begin:%s", __FUNCTION__);
            if (_server_holder_0) {
                _server_holder_0->invoke(std::function<std::any()>([&] {
                    _server_holder_0.reset();
                    return std::any();
                }));
            }
            InterDeinitialize(); 
			//Log_info("end:%s", __FUNCTION__);
        }

    }
}