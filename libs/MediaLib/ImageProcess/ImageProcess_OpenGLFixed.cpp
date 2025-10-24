#include "ImageProcess_OpenGLFixed.h"

#include <utils.hpp>


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

template<typename T, std::size_t N>
constexpr std::size_t  wx_countof(T const (&)[N])
{
    return N;
}

static const std::size_t MaxGlObjects = 10;

class GLFilterManger {
    cache::lru_cache<std::string, globj> m_Processors{ MaxGlObjects };
public:
    static GLFilterManger& GetInst() {
        static GLFilterManger s_mgrOpenGL;
		return s_mgrOpenGL;
    }

    GLFilterManger() {

    }
    virtual ~GLFilterManger() {
        //程序退出后調用
        exit(-1);
    }
    void Put(std::string key, globj obj) {
        m_Processors.put(key, obj);
    }

    globj Get(std::string key) {
        if (m_Processors.exists(key)) {
            return m_Processors.get(key);
        }
        return NULL;
    }
};


void GLFilter_Set(std::string key,globj obj) {
    GLFilterManger::GetInst().Put(key, obj);
}
globj GLFilter_Get(std::string key) {
    return GLFilterManger::GetInst().Get(key);
}

namespace ApowerSoft {
    namespace ImageProcess {

        std::unordered_map<std::string, std::weak_ptr<OpenGLFixed>> OpenGLFixed::OpenGLFixed_objs { };

        std::shared_ptr<OpenGLFixed> OpenGLFixed::create(const std::string& vertexshader, const std::string& fshader,
            int width_output, int height_output, int bytes_of_color_output,
            int width_0, int height_0, int bytes_of_color_0,
            int width_1, int height_1, int bytes_of_color_1,
            int width_2, int height_2, int bytes_of_color_2,
            int width_3, int height_3, int bytes_of_color_3,
            int width_4, int height_4, int bytes_of_color_4,
            int width_5, int height_5, int bytes_of_color_5,
            int width_6, int height_6, int bytes_of_color_6,
            int width_7, int height_7, int bytes_of_color_7
        ) {
            std::ostringstream ss;
                ss << "|" << fshader;
            ss << "|" << width_output << "|" << height_output << "|" << bytes_of_color_output
                << "|" << width_0 << "|" << height_0 << "|" << bytes_of_color_0
                << "|" << width_1 << "|" << height_1 << "|" << bytes_of_color_1
                << "|" << width_2 << "|" << height_2 << "|" << bytes_of_color_2
                << "|" << width_3 << "|" << height_3 << "|" << bytes_of_color_3
                << "|" << width_4 << "|" << height_4 << "|" << bytes_of_color_4
                << "|" << width_5 << "|" << height_5 << "|" << bytes_of_color_5
                << "|" << width_6 << "|" << height_6 << "|" << bytes_of_color_6
                << "|" << width_7 << "|" << height_7 << "|" << bytes_of_color_7
                << "|";

            const auto key = ss.str();

            std::lock_guard<std::mutex> lock(create_mutex);

            const auto &weak_obj = OpenGLFixed_objs[key];
            auto obj = weak_obj.lock();
            if (!obj) {
                obj = std::make_shared<OpenGLFixed>(vertexshader, fshader,
                    width_output, height_output, bytes_of_color_output,
                    width_0, height_0, bytes_of_color_0,
                    width_1, height_1, bytes_of_color_1,
                    width_2, height_2, bytes_of_color_2,
                    width_3, height_3, bytes_of_color_3,
                    width_4, height_4, bytes_of_color_4,
                    width_5, height_5, bytes_of_color_5,
                    width_6, height_6, bytes_of_color_6,
                    width_7, height_7, bytes_of_color_7
                    );

                OpenGLFixed_objs[key] = obj;
            }
            return obj;
        }

		std::shared_ptr<OpenGLFixed> OpenGLFixed::create(const std::string& vshader, const std::string& fshader,
			int width, int height, int bytes_of_color) {
			return create(vshader, fshader, 
				width, height, bytes_of_color, 
				width, height, bytes_of_color);
		}

        OpenGLFixed::OpenGLFixed(const std::string& vertexshader, const std::string&  fragshader,
            int width_output, int height_output, int bytes_of_color_output,
            int width_0, int height_0, int bytes_of_color_0,
            int width_1, int height_1, int bytes_of_color_1,
            int width_2, int height_2, int bytes_of_color_2,
            int width_3, int height_3, int bytes_of_color_3,
            int width_4, int height_4, int bytes_of_color_4,
            int width_5, int height_5, int bytes_of_color_5,
            int width_6, int height_6, int bytes_of_color_6,
            int width_7, int height_7, int bytes_of_color_7
        ) : OpenGL(vertexshader, fragshader),
            _size_output{ width_output * height_output * bytes_of_color_output },
            _width_output{ width_output }, _height_output{ height_output }, _bytes_of_color_output{ bytes_of_color_output },
            _width_0{ width_0 }, _height_0{ height_0 }, _bytes_of_color_0{ bytes_of_color_0 },
            _width_1{ width_1 }, _height_1{ height_1 }, _bytes_of_color_1{ bytes_of_color_1 },
            _width_2{ width_2 }, _height_2{ height_2 }, _bytes_of_color_2{ bytes_of_color_2 },
            _width_3{ width_3 }, _height_3{ height_3 }, _bytes_of_color_3{ bytes_of_color_3 },
            _width_4{ width_4 }, _height_4{ height_4 }, _bytes_of_color_4{ bytes_of_color_4 },
            _width_5{ width_5 }, _height_5{ height_5 }, _bytes_of_color_5{ bytes_of_color_5 },
            _width_6{ width_6 }, _height_6{ height_6 }, _bytes_of_color_6{ bytes_of_color_6 },
            _width_7{ width_7 }, _height_7{ height_7 }, _bytes_of_color_7{ bytes_of_color_7 }
        {
            _source_count =
                (width_0 * height_0 * bytes_of_color_0 > 0 ? 1 : 0) +
                (width_1 * height_1 * bytes_of_color_1 > 0 ? 1 : 0) +
                (width_2 * height_2 * bytes_of_color_2 > 0 ? 1 : 0) +
                (width_3 * height_3 * bytes_of_color_3 > 0 ? 1 : 0) +
                (width_4 * height_4 * bytes_of_color_4 > 0 ? 1 : 0) +
                (width_5 * height_5 * bytes_of_color_5 > 0 ? 1 : 0) +
                (width_6 * height_6 * bytes_of_color_6 > 0 ? 1 : 0) +
                (width_7 * height_7 * bytes_of_color_7 > 0 ? 1 : 0);

            init();
        }

        OpenGLFixed::~OpenGLFixed() {
            if (_server) {
                //exit(-1);//程序退出时无法正常结束。。
                _server->invoke(std::function<std::any()>([&] {
                    fnglDeleteFramebuffers(1, &_fbo);
					
                    fnglDeleteRenderbuffers(wx_countof(_fbo_buffer), _fbo_buffer);
                    fnglDeleteTextures(wx_countof(_fbo_texture), _fbo_texture);
                    fnglDeleteBuffers(wx_countof(_pbo), _pbo);
                    return std::any();
                }));
            }
        }

        void OpenGLFixed::init() {
            if (_source_count < 1 || _source_count > 8) {
                throw std::out_of_range("source_count: [1, 8]");
            }
            if (_size_output <= 0) {
                throw std::out_of_range("size_output: > 0");
            }
            
            if (_server) {
                _server->invoke(std::function<std::any()>([&] {
                    if (!SupportedPBO.empty()) {
                        fnglGenBuffers(wx_countof(_pbo), _pbo);
                    }

                    if (!_server->gen_fbo_with_buffer(_fbo, _fbo_buffer[Dest], _width_output, _height_output, _bytes_of_color_output)) {
                        std::cerr << "Build fbos failed." << std::endl;
                    }

                    if (!SupportedPBO.empty()) {
                        fnglBindBuffer(GL_PIXEL_PACK_BUFFER, _pbo[Dest]);
                        ON_SCOPE_EXIT{
                                fnglBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
                        };
                        
                        if (SupportedImmutableBuffer.empty()) {
                            fnglBufferData(GL_PIXEL_PACK_BUFFER, _size_output, nullptr, GL_STREAM_READ);
                        }
                        else {
                            fnglBufferStorage(GL_PIXEL_PACK_BUFFER, _size_output, nullptr, GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

                            _pbo_map_ptr[Dest] = fnglMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, _size_output, GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
                        }
                    }

                    std::vector<int> widths{ _width_0, _width_1, _width_2, _width_3, _width_4, _width_5, _width_6, _width_7 };
                    std::vector<int> heights{ _height_0, _height_1, _height_2, _height_3, _height_4, _height_5, _height_6, _height_7 };
                    std::vector<int> bpcs{ _bytes_of_color_0, _bytes_of_color_1, _bytes_of_color_2, _bytes_of_color_3, _bytes_of_color_4, _bytes_of_color_5, _bytes_of_color_6, _bytes_of_color_7 };
                    std::vector<int> types{ Src0, Src1, Src2, Src3, Src4, Src5, Src6, Src7 };
                    for (auto i = 0; i < types.size(); ++i) {
                        const auto size = widths[i] * heights[i] * bpcs[i];
                        if (size > 0) {
                            if (!_server->gen_texture(_fbo_texture[types[i]], widths[i], heights[i], bpcs[i])) {
                                std::cerr << "Build src texture failed." << std::endl;
                            }

                            if (!SupportedPBO.empty()) {
                                fnglBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pbo[types[i]]);
                                ON_SCOPE_EXIT{
                                        fnglBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
                                };

                                if (SupportedImmutableBuffer.empty()) {
                                    fnglBufferData(GL_PIXEL_UNPACK_BUFFER, size, nullptr, GL_STREAM_DRAW);//pbo对象对应的静态内存
                                }
                                else {
                                    fnglBufferStorage(GL_PIXEL_UNPACK_BUFFER, size, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);//持久化内存,显存或内存

                                    _pbo_map_ptr[types[i]] = fnglMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, size, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);//通过内存指针暴露
                                }
                            }
                        }
                    }
                    return std::any();
                }));

				_vbo = _server->VBO();
				vertexelement = 0;
            }
        }

		void OpenGLFixed::SetVertexBuffer(std::vector<float> vertex_fan, std::vector<GLuint> vertex_element)
		{
			_server->invoke(std::function<std::any()>([&] {
				fnglGenBuffers(1, &_vbo);
				fnglBindBuffer(GL_ARRAY_BUFFER, _vbo);
				ON_SCOPE_EXIT{
						fnglBindBuffer(GL_ARRAY_BUFFER, 0);
				};
				fnglBufferData(GL_ARRAY_BUFFER, vertex_fan.size() * sizeof(float), vertex_fan.data(), GL_STATIC_DRAW);
				
				glGenVertexArrays(1, &vertexarray);
				glBindVertexArray(vertexarray);
				indicescount = vertex_element.size();

				fnglGenBuffers(1, &vertexelement);
				fnglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexelement);
				fnglBufferData(GL_ELEMENT_ARRAY_BUFFER, vertex_element.size() * sizeof(GLuint), vertex_element.data(), GL_STATIC_DRAW);
				return std::any();
			}));
			
		}

        bool OpenGLFixed::render_filter(pbyte output,
                                        cpbyte pixels_0, cpbyte pixels_1, cpbyte pixels_2, cpbyte pixels_3, cpbyte pixels_4,
                                        cpbyte pixels_5, cpbyte pixels_6, cpbyte pixels_7) {
            if (!_server) {
                std::memcpy(output, pixels_0, _width_0 * _height_0 * _bytes_of_color_0);
                return false;
            }

#undef debug_log

//#ifdef debug_log
//            static int log_count = 0;
//            static std::ostringstream oss;
//            oss << TimePoint::nowToString() << ", Begin fixed multi. " << std::endl;
//#endif // debug_log

            const auto result = _server->invoke(std::function<std::any()>([&] {
                try {
                    {
                        std::vector<cpbyte> pixels{ pixels_0, pixels_1, pixels_2, pixels_3, pixels_4, pixels_5, pixels_6, pixels_7 };
                        std::vector<int> widths{ _width_0, _width_1, _width_2, _width_3, _width_4, _width_5, _width_6, _width_7 };
                        std::vector<int> heights{ _height_0, _height_1, _height_2, _height_3, _height_4, _height_5, _height_6, _height_7 };
                        std::vector<int> bpcs{ _bytes_of_color_0, _bytes_of_color_1, _bytes_of_color_2, _bytes_of_color_3, _bytes_of_color_4, _bytes_of_color_5, _bytes_of_color_6, _bytes_of_color_7 };
                        std::vector<int> types{ Src0, Src1, Src2, Src3, Src4, Src5, Src6, Src7 };
                        for (auto i = 0; i < types.size(); i++) {
                            if (pixels[i]) {
                                fnglBindTexture(GL_TEXTURE_2D, _fbo_texture[types[i]]);

								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, TEXTURE_WRAP_Parameter);
								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, TEXTURE_WRAP_Parameter);

                                ON_SCOPE_EXIT{
                                        fnglBindTexture(GL_TEXTURE_2D, 0);
                                };

                                if (SupportedPBO.empty()) {//1
                                    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, widths[i], heights[i], Server::get_pix_format(bpcs[i]), GL_UNSIGNED_BYTE, pixels[i]);
                                }
                                else {
                                    fnglBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pbo[types[i]]);
                                    ON_SCOPE_EXIT{
                                            fnglBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
                                        };

                                    const auto size = widths[i] * heights[i] * bpcs[i];

                                    if (SupportedImmutableBuffer.empty() || !_pbo_map_ptr[types[i]]) {//2.1,每一帧都要处理
                                        void *ptr = nullptr;
                                        if (fnglMapBuffer && (ptr = fnglMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY))) {
                                            ON_SCOPE_EXIT{
                                                    fnglUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
                                            };

                                            std::memcpy(ptr, pixels[i], size);
                                        }
                                        else {//<2.1
                                            fnglBufferSubData(GL_PIXEL_UNPACK_BUFFER, 0, size, pixels[i]);
                                        }
                                    }
                                    else {//2.2
                                        std::memcpy(_pbo_map_ptr[types[i]], pixels[i], size);
                                    }

                                    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, widths[i], heights[i], Server::get_pix_format(bpcs[i]), GL_UNSIGNED_BYTE, nullptr);
                                }
                            }
                        }
                    }
//#ifdef debug_log
//                    glFinish();
//                    oss << TimePoint::nowToString() << ", init. " << std::endl;
//#endif // debug_log

                    auto shaders{ _shaders };
                    if (shaders.empty()) {
                        shaders.emplace_back(_default_shader);
                    }
//#ifdef debug_log
//                    glFinish();
//                    oss << TimePoint::nowToString() << ", create shader. " << std::endl;
//#endif // debug_log

                    {
//#ifdef debug_log
//                        glFinish();
//                        oss << TimePoint::nowToString() << ", render begin. " << std::endl;
//#endif // debug_log

                        fnglBindFramebuffer(GL_FRAMEBUFFER, _fbo);

                        ON_SCOPE_EXIT{
                                fnglBindFramebuffer(GL_FRAMEBUFFER, 0);
                        };

                        glDrawBuffer(GL_COLOR_ATTACHMENT0);
                        glViewport(0, 0, _width_output, _height_output);
                        glClear(GL_COLOR_BUFFER_BIT);

//#ifdef debug_log
//                        glFinish();
//                        oss << TimePoint::nowToString() << ", render init. " << std::endl;
//#endif // debug_log
                        {
                            const auto shader{ shaders.front() };

                            shader->use();
                            ON_SCOPE_EXIT{
                                    shader->use(false);
                            };

							if (vertexelement>0)
								glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexelement);
							else
								glBindVertexArray(vertexarray);

                            shader->attribute_VBO_f("position", _vbo);
                            shader->uniform_iv_tex("iChannel", {
                                _fbo_texture[Src0],
                                _fbo_texture[Src1],
                                _fbo_texture[Src2],
                                _fbo_texture[Src3],
                                _fbo_texture[Src4],
                                _fbo_texture[Src5],
                                _fbo_texture[Src6],
                                _fbo_texture[Src7]
                                });
                            shader->uniform_fv(1, "iChannelTime", { 0, 0, 0, 0, 0, 0, 0, 0 });
                            shader->uniform_fv(3, "iChannelResolution", {
                                static_cast<float>(_width_0),
                                static_cast<float>(_height_0),
                                0,
                                static_cast<float>(_width_1),
                                static_cast<float>(_height_1),
                                0,
                                static_cast<float>(_width_2),
                                static_cast<float>(_height_2),
                                0,
                                static_cast<float>(_width_3),
                                static_cast<float>(_height_3),
                                0,
                                static_cast<float>(_width_4),
                                static_cast<float>(_height_4),
                                0,
                                static_cast<float>(_width_5),
                                static_cast<float>(_height_5),
                                0,
                                static_cast<float>(_width_6),
                                static_cast<float>(_height_6),
                                0,
                                static_cast<float>(_width_7),
                                static_cast<float>(_height_7),
                                0
                                });

							shader->uniform_fv(2, "iResolution", { static_cast<float>(_width_output), static_cast<float>(_height_output)});
                            shader->uniform_fv(1, "iDuration", { 1 });
                            shader->uniform_fv(1, "iTime", { 0 });
                            shader->uniform_fv(1, "iTimeDelta", { 0 });
                            shader->uniform_iv(1, "iFrame", { 0 });
                            shader->uniform_fv(4, "iDate", get_datetime());
                            shader->uniform_fv(4, "iMouse", { 0, 0, 0, 0 });
                            shader->uniform_fv(1, "iSampleRate", { 0 });
							try {
                                const auto &&shader_param = _shader_params.find(-1);
                                if (shader_param != _shader_params.end()) {
                                    for (auto &&[name, value] : shader_param->second) {
                                        shader->uniform_fv(std::get<0>(value), name.c_str(), std::get<1>(value));
                                    }
                                }
                            }
                            catch (...) {}
                            try {
                                const auto &&shader_param = _shader_params.find(0);
                                if (shader_param != _shader_params.end()) {
                                    for (auto &&[name, value] : shader_param->second) {
                                        shader->uniform_fv(std::get<0>(value), name.c_str(), std::get<1>(value));
                                    }
                                }
                            }
                            catch (...) {}
                            try {
                                (*shader)();
                            }
                            catch (...) {}
//#ifdef debug_log
//                            glFinish();
//                            oss << TimePoint::nowToString() << ", render set var. " << std::endl;
//#endif // debug_log
							if (vertexelement > 0)
								glDrawElements(
									GL_TRIANGLE_FAN,      // mode
									indicescount,    // count
									GL_UNSIGNED_INT,   // type
									(void*)0           // element array buffer offset
								);							
							else
								glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
                            glFlush();

							glBindVertexArray(0);
//#ifdef debug_log
//                            glFinish();
//                            oss << TimePoint::nowToString() << ", render end. " << std::endl;
//#endif // debug_log
                        }
                    }
                    {
//#ifdef debug_log
//                    glFinish();
//                    oss << TimePoint::nowToString() << ", copy begin. " << std::endl;
//#endif // debug_log
                        fnglBindFramebuffer(GL_FRAMEBUFFER, _fbo);
                        ON_SCOPE_EXIT{
                                fnglBindFramebuffer(GL_FRAMEBUFFER, 0);
                        };

                        glReadBuffer(GL_COLOR_ATTACHMENT0);
//#ifdef debug_log
//                        glFinish();
//                        oss << TimePoint::nowToString() << ", copy waited. " << std::endl;
//#endif // debug_log

                        if (SupportedPBO.empty()) {
                            glFinish();

                            glReadPixels(0, 0, _width_output, _height_output, Server::get_pix_format(_bytes_of_color_output), GL_UNSIGNED_BYTE, output);
//#ifdef debug_log
//                            glFinish();
//                            oss << TimePoint::nowToString() << ", copy read. " << std::endl;
//#endif // debug_log
                        }
                        else {
                            fnglBindBuffer(GL_PIXEL_PACK_BUFFER, _pbo[Dest]);
                            ON_SCOPE_EXIT{
                                    fnglBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
                                };

                            glReadPixels(0, 0, _width_output, _height_output, Server::get_pix_format(_bytes_of_color_output), GL_UNSIGNED_BYTE, nullptr);
//#ifdef debug_log
//                            glFinish();
//                            oss << TimePoint::nowToString() << ", copy read. " << std::endl;
//#endif // debug_log

                            if (SupportedImmutableBuffer.empty() || !_pbo_map_ptr[Dest]) {
                                void *ptr = nullptr;
                                if (fnglMapBuffer && (ptr = fnglMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY))) {
                                    ON_SCOPE_EXIT{
                                            fnglUnmapBuffer(GL_PIXEL_PACK_BUFFER);
                                    };

//#ifdef debug_log
//                                    glFinish();
//                                    oss << TimePoint::nowToString() << ", copy map. " << std::endl;
//#endif // debug_log

                                    glFinish();

                                    std::memcpy(output, ptr, _size_output);
                                }
                                else {
                                    glFinish();

                                    fnglGetBufferSubData(GL_PIXEL_PACK_BUFFER, 0, _size_output, output);
                                }
                            }
                            else {
                                glFinish();

                                std::memcpy(output, _pbo_map_ptr[Dest], _size_output);
                            }
                        }

//#ifdef debug_log
//                        glFinish();
//                        oss << TimePoint::nowToString() << ", copy end. " << std::endl;
//#endif // debug_log
                     }

                    return true;
                }
                catch (std::exception &ex) {
                    std::memcpy(output, pixels_0, _width_0* _height_0* _bytes_of_color_0);
                    std::cerr << "render_filter failed: " << ex.what() << std::endl;
                }
                return false;
            }));

//#ifdef debug_log
//            oss << TimePoint::nowToString() << ", End. " << std::endl;
//            if (++log_count >= 50) {
//                std::cout << oss.str();
//                log_count = 0;
//                oss.clear();
//            }
//#endif // debug_log

#if !defined(_WIN32) && (defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)))
            return result.empty() ? false : boost::any_cast<bool>(result);
#else
            return !result.has_value() ? false : std::any_cast<bool>(result);
#endif
        }

        bool OpenGLFixed::Filter(pbyte output,
                                 cpbyte pixels_0, cpbyte pixels_1, cpbyte pixels_2, cpbyte pixels_3, cpbyte pixels_4,
                                 cpbyte pixels_5, cpbyte pixels_6, cpbyte pixels_7) {
            bool bRet = render_filter(output,
                                 pixels_0, pixels_1, pixels_2, pixels_3, pixels_4,
                                 pixels_5, pixels_6, pixels_7);



            return bRet;
        }

        OpenGL::bytes OpenGLFixed::Filter(
            cpbyte pixels_0, cpbyte pixels_1, cpbyte pixels_2, cpbyte pixels_3, cpbyte pixels_4,
            cpbyte pixels_5, cpbyte pixels_6, cpbyte pixels_7) {
            bytes output(_size_output);
            if (render_filter(output.data(),
                              pixels_0, pixels_1, pixels_2, pixels_3, pixels_4,
                              pixels_5, pixels_6, pixels_7
            )) {
                return output;
            }

            return bytes { };
        }
    }
}
