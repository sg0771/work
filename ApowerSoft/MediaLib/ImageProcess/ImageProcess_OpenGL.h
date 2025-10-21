#pragma once

#include "Utils.hpp"
#include "ImageProcess_ShaderProgram.h"

//using any = std::any;

#include "../glfw/glfw3.h"
#include "../glew/glew.h"

namespace ApowerSoft {
    namespace ImageProcess {

        class /*ImageProcess_API*/ OpenGL {
        protected:
            using byte = uint8_t;
            using pbyte = byte *;
            using cpbyte = const byte *;
            using bytes = std::vector<byte>;

            using vertex = std::string;
            using vertexes = std::vector<vertex>;
            using fragment = std::string;
            using fragments = std::vector<fragment>;
            using shader = std::pair<vertex, fragment>;
            using shaders = std::vector<shader>;

            using custom_fragment = std::pair<fragment, std::function<void(GLuint)>>;
            using custom_fragments = std::vector<custom_fragment>;
            using custom_shader = std::pair<shader, std::function<void(GLuint)>>;
            using custom_shaders = std::vector<custom_shader>;

            class Server {
                void *_window;
                GLuint _vbo;
                GLfloat _anisotropy;

                std::function<void()> _func_cleanup_context;

                void init_context();
                void init_extensions();
                void init_vertex();

                std::string _version;
            public:
                static std::atomic_int context_state;
                static std::mutex context_state_mutex;
                static std::condition_variable context_state_cv;

                static std::atomic_flag context_is_building;
                static std::atomic<std::thread::id> context_thread_id;

            public:
                Server(const Server &other) = delete;
                Server(Server &&other) noexcept = delete;
                Server& operator=(const Server &other) = delete;
                Server& operator=(Server &&other) noexcept = delete;

                Server();
                ~Server();

                GLuint VBO() const;
                std::string Version() const;

                blocking_queue<std::packaged_task<std::any()>> processor;
                std::any invoke(const std::function<std::any()> &func);

                bool gen_texture(GLuint &_texture, int width, int height, int bytes_of_color = 1, cpbyte pixels = nullptr);
                //bool gen_fbo_with_texture(GLuint &_fbo, GLuint &_texture, int width, int height, int bytes_of_color = 1, cpbyte pixels = nullptr);
                bool gen_fbo_with_buffer(GLuint &_fbo, GLuint &_buffer, int width, int height, int bytes_of_color = 1);
                /*bool gen_fbo_with_buffer_multi(GLuint &_fbo, GLuint &_buffer_r, GLuint &_buffer_g, GLuint &_buffer_b, int width, int height);
                bool gen_fbo_with_buffer_multi(GLuint &_fbo, GLuint &_buffer_y_00, GLuint &_buffer_y_01, GLuint &_buffer_y_10, GLuint &_buffer_y_11, GLuint &_buffer_u, GLuint &_buffer_v, int width, int height);*/
                static GLenum get_pix_format_internal(int bytes_of_color = 3);
                static GLenum get_pix_format(int bytes_of_color = 3);
            };

            static std::shared_ptr<Server> _server_holder_0;
            static std::shared_ptr<Server> _server_holder;
            static std::shared_ptr<shader_program> _default_shader;

            static std::initializer_list<float> get_datetime();

            static std::mutex create_mutex;

            std::shared_ptr<Server> _server;
            std::vector<std::shared_ptr<shader_program>> _shaders;
            std::unordered_map<int, std::unordered_map<std::string, std::tuple<int, std::vector<GLfloat>>>> _shader_params;
			int  TEXTURE_WRAP_Parameter;


        private:
            static std::unordered_map<std::string, std::weak_ptr<OpenGL>> OpenGL_objs;
            
            static std::shared_ptr<Server> InterInitialize();
            static void InterDeinitialize();

        public:
            static std::string SupportedVBO;
            static std::string SupportedFBO;
            static std::string SupportedPBO;
            static std::string SupportedImmutableBuffer;
            static std::string SupportedImmutableTexture;
            static std::string SupportedAnisotropic;

            static std::function<void(GLsizei, GLuint*)> fnglGenBuffers;
            static std::function<void(GLenum, GLuint)> fnglBindBuffer;
            static std::function<void(GLsizei, const GLuint*)> fnglDeleteBuffers;
            static std::function<void(GLenum, GLsizeiptr, const void*, GLenum)> fnglBufferData;
            static std::function<void(GLenum, GLintptr, GLsizeiptr, const void*)> fnglBufferSubData;
            static std::function<void(GLenum, GLintptr, GLsizeiptr, void*)> fnglGetBufferSubData;
            static std::function<void*(GLenum, GLenum)> fnglMapBuffer;
            static std::function<GLboolean(GLenum)> fnglUnmapBuffer;

            static std::function<void(GLenum, GLsizei, GLenum, GLsizei, GLsizei)> fnglTexStorage2D;
            static std::function<void(GLenum, GLsizeiptr, const void *, GLbitfield)> fnglBufferStorage;
            static std::function<void*(GLenum, GLintptr, GLsizeiptr, GLbitfield)> fnglMapBufferRange;

            static std::function<void(GLsizei, GLuint*)> fnglGenTextures;
            static std::function<void(GLenum, GLuint)> fnglBindTexture;
            static std::function<void(GLsizei, const GLuint*)> fnglDeleteTextures;
            static std::function<void(GLenum)> fnglActiveTexture;

            static std::function<void(GLsizei, GLuint*)> fnglGenRenderbuffers;
            static std::function<void(GLenum, GLuint)> fnglBindRenderbuffer;
            static std::function<void(GLsizei, const GLuint*)> fnglDeleteRenderbuffers;
            static std::function<void(GLenum, GLenum, GLsizei, GLsizei)> fnglRenderbufferStorage;

            static std::function<void(GLsizei, GLuint*)> fnglGenFramebuffers;
            static std::function<void(GLenum, GLuint)> fnglBindFramebuffer;
            static std::function<void(GLsizei, const GLuint*)> fnglDeleteFramebuffers;
            static std::function<void(GLenum, GLenum, GLenum, GLuint)> fnglFramebufferRenderbuffer;
            static std::function<void(GLenum, GLenum, GLenum, GLuint, GLint)> fnglFramebufferTexture2D;
            static std::function<GLenum(GLenum)> fnglCheckFramebufferStatus;

            static std::function<GLuint(GLenum)> fnglCreateShader;
            static std::function<void(GLuint)> fnglDeleteShader;
            static std::function<void(GLuint, GLsizei, const GLchar *const*, const GLint*)> fnglShaderSource;
            static std::function<void(GLuint)> fnglCompileShader;
            static std::function<void(GLuint, GLenum, GLint*)> fnglGetShaderiv;
            static std::function<void(GLuint, GLsizei, GLsizei*, GLchar*)> fnglGetShaderInfoLog;
            static std::function<GLuint()> fnglCreateProgram;
            static std::function<void(GLuint)> fnglDeleteProgram;
            static std::function<void(GLuint)> fnglLinkProgram;
            static std::function<void(GLuint)> fnglUseProgram;
            static std::function<void(GLuint, GLenum, GLint*)> fnglGetProgramiv;
            static std::function<void(GLuint, GLsizei, GLsizei*, GLchar*)> fnglGetProgramInfoLog;
            static std::function<void(GLuint, GLuint)> fnglAttachShader;
            static std::function<GLuint(GLuint, const GLchar*)> fnglGetAttribLocation;
            static std::function<GLuint(GLuint, const GLchar*)> fnglGetUniformLocation;
            static std::function<void(GLuint)> fnglEnableVertexAttribArray;
            static std::function<void(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*)> fnglVertexAttribPointer;
            static std::function<void(GLint, GLint)> fnglUniform1i;
            static std::function<void(GLint, GLsizei, const GLint*)> fnglUniform1iv;
            static std::function<void(GLint, GLsizei, const GLint*)> fnglUniform2iv;
            static std::function<void(GLint, GLsizei, const GLint*)> fnglUniform3iv;
            static std::function<void(GLint, GLsizei, const GLint*)> fnglUniform4iv;
            static std::function<void(GLint, GLsizei, const GLfloat*)> fnglUniform1fv;
            static std::function<void(GLint, GLsizei, const GLfloat*)> fnglUniform2fv;
            static std::function<void(GLint, GLsizei, const GLfloat*)> fnglUniform3fv;
            static std::function<void(GLint, GLsizei, const GLfloat*)> fnglUniform4fv;

        public:
            static int Initialize();
            static void Deinitialize();

            static std::string GetOpenGLVersion();
            static std::string GetOpenGLLogs();
            static void GetOpenGLLogs(char *str, int len);

        public:
            OpenGL(const OpenGL &other) = delete;
            OpenGL(OpenGL &&other) noexcept = delete;
            OpenGL& operator=(const OpenGL &other) = delete;
            OpenGL& operator=(OpenGL &&other) noexcept = delete;

           

            explicit OpenGL(std::string vertexsource, std::string fragsource);
           
            OpenGL();
            virtual ~OpenGL();

        public:
            void SetUniform(const std::string &name, int elems_count, const std::initializer_list<GLfloat> &values);
            void SetUniform(int index, const std::string &name, int elems_count, const std::initializer_list<GLfloat> &values);
			void SetUniformMat(const std::string &name, int elems_count, const std::initializer_list<GLfloat> &values);

            void SetDuration(float value);
            void SetTime(float value);
            void SetStrength(float value);
			inline void SetTEXTURE_WRAP_Parameter(int value) {
				TEXTURE_WRAP_Parameter = value;
			}

        private:
        
            
        };

    }
}