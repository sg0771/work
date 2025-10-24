#pragma once

#ifndef __SHADER_PROGRAM_H__
#define __SHADER_PROGRAM_H__

#include <mutex>
#include <vector>
#include <string>
#include <functional>

#include "../glew/glew.h"

namespace ApowerSoft {
    namespace ImageProcess {

        class shader_program {
            inline static const GLchar *_v_shader_source_default =
                R"(
                    #version 130

                    #ifdef GL_ES
                    precision highp float;
                    #endif

                    attribute vec2 position;

                    void main() {
                        gl_Position = vec4(position, 0.0, 1.0);
                    }
                )";

		

            inline static const GLchar *_f_shader_source_default =
                R"(
                    #version 130

                    #ifdef GL_ES
                    precision mediump float;
                    #endif

					uniform vec2      iResolution;           // viewport resolution (in pixels)
                    uniform float     iDuration;             // shader playback total time (in seconds)
                    uniform float     iTime;                 // shader playback current time (in seconds)
                    uniform float     iTimeDelta;            // render time (in seconds)
                    uniform int       iFrame;                // shader playback frame
                    uniform float     iChannelTime[8];       // channel playback time (in seconds)
                    uniform vec3      iChannelResolution[8]; // channel resolution (in pixels)
                    uniform vec4      iMouse;                // mouse pixel coords. xy: current (if MLB down), zw: click
                    uniform sampler2D iChannel[8];           // input channel. XX = 2D/Cube
                    uniform vec4      iDate;                 // (year, month, day, time in seconds)
                    uniform float     iSampleRate;           // sound sample rate (i.e., 44100)

                    


                    float iProgress = iTime / iDuration;
                    float iAspect = iResolution.x / iResolution.y;

                    const float PI = 3.141592653589793;

                    ////////////////////////////////////////////////////////////////////////////////////////////////////////////

                    void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
                        fragColor = texture2D(iChannel[0], fragCoord / iResolution.xy);
                    }

                    ////////////////////////////////////////////////////////////////////////////////////////////////////////////

                    void main() {
                        mainImage(gl_FragColor, gl_FragCoord.xy);
                    }
                )";

			inline static const GLchar *_f_shader_source_prefix =
				R"(
                    #version 130

                    #ifdef GL_ES
                    precision mediump float;
                    #endif

                    uniform vec2      iResolution;           // viewport resolution (in pixels)
                    uniform float     iDuration;             // shader playback total time (in seconds)
                    uniform float     iTime;                 // shader playback current time (in seconds)
                    uniform float     iTimeDelta;            // render time (in seconds)
                    uniform int       iFrame;                // shader playback frame
                    uniform float     iChannelTime[8];       // channel playback time (in seconds)
                    uniform vec3      iChannelResolution[8]; // channel resolution (in pixels)
                    uniform vec4      iMouse;                // mouse pixel coords. xy: current (if MLB down), zw: click
                    uniform sampler2D iChannel[8];           // input channel. XX = 2D/Cube
                    uniform vec4      iDate;                 // (year, month, day, time in seconds)
                    uniform float     iSampleRate;           // sound sample rate (i.e., 44100)



                    float iProgress = iTime / iDuration;
                    float iAspect = iResolution.x / iResolution.y;

                    float global_time;
					float PREFIX(float aa)
					{
						return  iTime / iDuration; 
					}

                    float iGlobalTime =  iTime / iDuration; 
					const float PI = 3.141592653589793;

					vec4 INPUT1(vec2 position)
					{
						vec2 p = gl_FragCoord.xy / iResolution.xy;
  						return texture2D(iChannel[0], position);
					}

					vec4 INPUT2(vec2 position)
					{
						vec2 p = gl_FragCoord.xy / iResolution.xy;
  						return texture2D(iChannel[1], position);
					}
					vec4 FUNCNAME(vec2 tc); 

                )";

            static std::mutex create_mutex;
            static std::unordered_map<std::string, std::weak_ptr<shader_program>> shader_program_objs;
			
            GLuint _prog;
            std::function<void(GLuint)> _action;



            void build_from_file(const std::string &v_file, const std::string &f_file);

            void build(const std::string &v_source, const std::string &f_source);

            static GLuint compile(GLuint type, const GLchar **source, int count = 1);

            template <int N>
            static GLuint compile(const GLuint type, const GLchar *(&source)[N]) {
                return compile(type, source, N);
            }

            GLuint link(GLuint vert, GLuint frag) const;
			static std::string glslversionstr;
        public:

            shader_program(const shader_program &) = delete;
            shader_program& operator=(const shader_program &) = delete;

            shader_program(shader_program &&) = default;
            shader_program& operator=(shader_program &&) = default;

            shader_program(const std::string &v_source, const std::string &f_source, std::function<void(GLuint)> action = nullptr);

            shader_program() = delete;
            virtual ~shader_program();

            static std::shared_ptr<shader_program> create(const std::string &f_source = "");
            static std::shared_ptr<shader_program> create(const std::string &v_source, const std::string &f_source, std::function<void(GLuint)> action = nullptr);

			
			static void setShaderVersion(std::string str) { 
				//shader_program::glslversionstr = str; 
				//glslversionstr = str;
			}
            operator GLuint() const;
            void operator ()() const;

            void use(bool use = true) const;

            void attribute_VBO_f(const GLchar *name, GLuint v0, GLint size = 2) const;

            template <int N>
            void uniform_fv(const int elems_count, const GLchar *name, const GLfloat (&values)[N]) const {
                uniform_fv(elems_count, name, N, values);
            }

            void uniform_fv(int elems_count, const GLchar *name, const std::vector<GLfloat> &values) const;
            void uniform_fv(int elems_count, const GLchar *name, GLsizei value_count, const GLfloat *values) const;
			
            template <int N>
            void uniform_iv(const int elems_count, const GLchar *name, const GLint (&values)[N]) const {
                uniform_iv(elems_count, name, N, values);
            }

            void uniform_iv(int elems_count, const GLchar *name, const std::vector<GLint> &values) const;
            void uniform_iv(int elems_count, const GLchar *name, GLsizei value_count, const GLint *values) const;

            template <int N>
            void uniform_iv_tex(const GLchar *name, const GLuint (&values)[N]) const {
                uniform_iv_tex(name, N, values);
            }

            void uniform_iv_tex(const GLchar *name, const std::vector<GLuint> &values) const;
            void uniform_iv_tex(const GLchar *name, GLsizei value_count, const GLuint *values) const;

            void uniform_i_tex(const GLchar *name, GLuint v0, int index = 0) const;
        };

    }
}
#endif /* __SHADER_PROGRAM_H__ */
