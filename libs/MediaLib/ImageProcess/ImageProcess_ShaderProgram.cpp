
#include "Utils.hpp"
#include "ImageProcess_ShaderProgram.h"
#include "ImageProcess_OpenGL.h"

namespace ApowerSoft {
    namespace ImageProcess {

        std::mutex shader_program::create_mutex { };
        std::unordered_map<std::string, std::weak_ptr<shader_program>> shader_program::shader_program_objs { };

        std::shared_ptr<shader_program> shader_program::create(const std::string & f_source /*= ""*/) {
            return create("", f_source);
        }

        std::shared_ptr<shader_program> shader_program::create(const std::string & v_source, const std::string & f_source, std::function<void(GLuint)> action /*= nullptr*/) {
			//Log_info("begin:%s", __FUNCTION__);
            std::ostringstream ss;
            ss << "|" << v_source << "|" << f_source << "|" << action.target<void(GLuint)>() << "|";
            const auto key = ss.str();

            std::lock_guard<std::mutex> lock(create_mutex);

            const auto &weak_obj = shader_program_objs[key];
            auto obj = weak_obj.lock();
            if (!obj) {
                obj = std::make_shared<shader_program>(v_source, f_source, action);
                shader_program_objs[key] = obj;
            }
			//Log_info("end:%s", __FUNCTION__);
            return obj;
        }

        shader_program::shader_program(const std::string &v_source, const std::string &f_source,
                                       std::function<void(GLuint)> action /*= nullptr*/) : _prog{ 0 }, _action { std::move(action) } {
            build_from_file(v_source, f_source);
        }

        shader_program::~shader_program() {
            if(OpenGL::fnglDeleteProgram != nullptr)
            OpenGL::fnglDeleteProgram(_prog);
        }

        shader_program::operator GLuint() const {
            return _prog;
        }

        void shader_program::operator ()() const {
            if (_action) {
                try {
                    _action(_prog);
                } catch (std::exception &ex) {
                    std::cerr << "Shader custom action failed: " << ex.what() << std::endl;
                } catch (...) {
                    std::cerr << "Shader custom action failed." << std::endl;
                }
            }
        }

        void shader_program::use(const bool use /*= true*/) const {
            OpenGL::fnglUseProgram(use ? _prog : 0);
        }

        void shader_program::attribute_VBO_f(const GLchar *name, const GLuint v0, const GLint size /*= 2*/) const {
            if (_prog > 0) {
                const auto location = OpenGL::fnglGetAttribLocation(_prog, name);
                if (location >= 0) {
                    OpenGL::fnglEnableVertexAttribArray(location);
                    OpenGL::fnglBindBuffer(GL_ARRAY_BUFFER, v0);
                    OpenGL::fnglVertexAttribPointer(location, size, GL_FLOAT, GL_FALSE, 0, nullptr);
                }
            }
        }

        void shader_program::uniform_fv(const int elems_count, const GLchar *name, const std::vector<GLfloat> &values) const {
            uniform_fv(elems_count, name, values.size(), values.data());
        }

        void shader_program::uniform_fv(const int elems_count, const GLchar *name, const GLsizei value_count, const GLfloat *values) const {
            if (_prog > 0) {
                const auto location = OpenGL::fnglGetUniformLocation(_prog, name);
				
                if (location >= 0) {
                    const auto count = static_cast<int>(std::ceil(static_cast<double>(value_count) / static_cast<double>(elems_count)));
                    switch (elems_count) {
                        case 1:
                            OpenGL::fnglUniform1fv(location, count, values);
                            break;
                        case 2:
                            OpenGL::fnglUniform2fv(location, count, values);
                            break;
                        case 3:
                            OpenGL::fnglUniform3fv(location, count, values);
                            break;
                        case 4:
                            OpenGL::fnglUniform4fv(location, count, values);
                            break;
                        default: ;
                    }
                }
            }
        }


        void shader_program::uniform_iv(const int elems_count, const GLchar *name, const std::vector<GLint> &values) const {
            uniform_iv(elems_count, name, values.size(), values.data());
        }

        void shader_program::uniform_iv(const int elems_count, const GLchar *name, const GLsizei value_count, const GLint *values) const {
            if (_prog > 0) {
                const auto location = OpenGL::fnglGetUniformLocation(_prog, name);
                if (location >= 0) {
                    const auto count = static_cast<int>(std::ceil(static_cast<double>(value_count) / static_cast<double>(elems_count)));
                    switch (elems_count) {
                        case 1:
                            OpenGL::fnglUniform1iv(location, count, values);
                            break;
                        case 2:
                            OpenGL::fnglUniform2iv(location, count, values);
                            break;
                        case 3:
                            OpenGL::fnglUniform3iv(location, count, values);
                            break;
                        case 4:
                            OpenGL::fnglUniform4iv(location, count, values);
                            break;
                        default: ;
                    }
                }
            }
        }

        void shader_program::uniform_i_tex(const GLchar *name, const GLuint v0, const int index /*= 0*/) const {
            if (_prog > 0) {
                const auto location = OpenGL::fnglGetUniformLocation(_prog, name);
                if (location >= 0) {
                    OpenGL::fnglActiveTexture(GL_TEXTURE0 + index);
                    OpenGL::fnglBindTexture(GL_TEXTURE_2D, v0);
                    OpenGL::fnglUniform1i(location, index);
                }
            }
        }

        void shader_program::uniform_iv_tex(const GLchar *name, const std::vector<GLuint> &values) const {
            uniform_iv_tex(name, values.size(), values.data());
        }
        
        void shader_program::uniform_iv_tex(const GLchar *name, const GLsizei value_count, const GLuint *values) const {
            if (_prog > 0) {
                const auto location = OpenGL::fnglGetUniformLocation(_prog, name);
                if (location >= 0) {
                    std::vector<GLint> indexes;
                    for (auto i = 0; i < value_count; ++i) {
                        OpenGL::fnglActiveTexture(GL_TEXTURE0 + i);
                        OpenGL::fnglBindTexture(GL_TEXTURE_2D, values[i]);
                        indexes.emplace_back(i);
                    }
                    uniform_iv(1, name, value_count, indexes.data());
                }
            }
        }

        void shader_program::build_from_file(const std::string &v_file, const std::string &f_file) {
			//Log_info("begin:%s", __FUNCTION__);
            std::string v_source, f_source;

            if (!v_file.empty()) {
                if (WXBase::Exists(v_file)) {
                    std::ostringstream ss;
                    ss << std::ifstream { v_file }.rdbuf();
                    v_source = ss.str();
                } else {
                    v_source = v_file;
                }
            }
            if (!f_file.empty()) {
                if (WXBase::Exists(f_file)) {
                    std::ostringstream ss;
                    ss << std::ifstream { f_file }.rdbuf();
                    f_source = std::string(_f_shader_source_prefix)+  ss.str();
                } else {
                    f_source = std::string(_f_shader_source_prefix) + f_file;
                }
            }

            build(v_source, f_source);
        }

        void shader_program::build(const std::string &v_source, const std::string &f_source) {
			//Log_info("begin:%s", __FUNCTION__);
            const GLchar *v_sources[] = { !v_source.empty() ? (std::string( _v_shader_source_default)+v_source).c_str() : _v_shader_source_default };
            const GLchar *f_sources[] = { !f_source.empty() ? f_source.c_str() : _f_shader_source_default };
			
            auto vert = compile(GL_VERTEX_SHADER, v_sources);
            auto frag = compile(GL_FRAGMENT_SHADER, f_sources);
            ON_SCOPE_EXIT {
                    OpenGL::fnglDeleteShader(vert);
                    OpenGL::fnglDeleteShader(frag);
            };

            _prog = link(vert, frag);
        }

        GLuint shader_program::compile(const GLuint type, const GLchar **source, const int count) {
            const auto shader = OpenGL::fnglCreateShader(type);
            if (shader > 0) {
                OpenGL::fnglShaderSource(shader, count, source, nullptr);
                OpenGL::fnglCompileShader(shader);

                GLint compiled;
                OpenGL::fnglGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
                if (!compiled) {
                    GLint length;
                    OpenGL::fnglGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
                    std::vector<char> buf(length);
                    OpenGL::fnglGetShaderInfoLog(shader, length, &length, buf.data());
                    const std::string log(buf.begin(), buf.end());
                    throw std::runtime_error("fnglCompileShader: " + log);
                }
            }

            return shader;
        }

        GLuint shader_program::link(GLuint vert, GLuint frag) const {
            const auto program = OpenGL::fnglCreateProgram();
            if (program > 0) {
                OpenGL::fnglAttachShader(program, vert);
                OpenGL::fnglAttachShader(program, frag);
                OpenGL::fnglLinkProgram(program);

                GLint linked;
                OpenGL::fnglGetProgramiv(program, GL_LINK_STATUS, &linked);
                if (!linked) {
                    GLint length;
                    OpenGL::fnglGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
                    std::vector<char> buf(length);
                    OpenGL::fnglGetProgramInfoLog(program, length, &length, buf.data());
                    const std::string log { buf.begin(), buf.end() };
                    throw std::runtime_error("fnglLinkProgram: " + log);
                }

                return program;
            }

            return 0;
        }

    }
}
