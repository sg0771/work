#pragma once

#include "ImageProcess_OpenGL.h"

typedef  void(__stdcall *GLRenderCallBack) (GLuint shaderprogram);

namespace ApowerSoft {
    namespace ImageProcess {

        class /*ImageProcess_API*/ OpenGLFixed : public OpenGL {
            int _size_output{ }, _source_count { };
            int _width_output{ }, _height_output{ }, _bytes_of_color_output{ };
            int _width_0{ }, _height_0{ }, _bytes_of_color_0{ };
            int _width_1{ }, _height_1{ }, _bytes_of_color_1{ };
            int _width_2{ }, _height_2{ }, _bytes_of_color_2{ };
            int _width_3{ }, _height_3{ }, _bytes_of_color_3{ };
            int _width_4{ }, _height_4{ }, _bytes_of_color_4{ };
            int _width_5{ }, _height_5{ }, _bytes_of_color_5{ };
            int _width_6{ }, _height_6{ }, _bytes_of_color_6{ };
            int _width_7{ }, _height_7{ }, _bytes_of_color_7{ };

            enum { Src0, Src1, Src2, Src3, Src4, Src5, Src6, Src7, Dest, BufferCount };

            GLuint _fbo { }, _fbo_texture[BufferCount] { }, _fbo_buffer[BufferCount] { };
            GLuint _pbo[BufferCount] { };
            void * _pbo_map_ptr[BufferCount] { };

            static std::unordered_map<std::string, std::weak_ptr<OpenGLFixed>> OpenGLFixed_objs;

        public:
            OpenGLFixed(const OpenGLFixed &other) = delete;
            OpenGLFixed(OpenGLFixed &&other) noexcept = delete;
            OpenGLFixed& operator=(const OpenGLFixed &other) = delete;
            OpenGLFixed& operator=(OpenGLFixed &&other) noexcept = delete;

            OpenGLFixed(const std::string& vertexshader, const std::string& fshader,
                int width_output, int height_output, int bytes_of_color_output,
                int width_0, int height_0, int bytes_of_color_0,
                int width_1 = 0, int height_1 = 0, int bytes_of_color_1 = 4,
                int width_2 = 0, int height_2 = 0, int bytes_of_color_2 = 4,
                int width_3 = 0, int height_3 = 0, int bytes_of_color_3 = 4,
                int width_4 = 0, int height_4 = 0, int bytes_of_color_4 = 4,
                int width_5 = 0, int height_5 = 0, int bytes_of_color_5 = 4,
                int width_6 = 0, int height_6 = 0, int bytes_of_color_6 = 4,
                int width_7 = 0, int height_7 = 0, int bytes_of_color_7 = 4
            );

            OpenGLFixed() = delete;
            virtual ~OpenGLFixed() override;

            static std::shared_ptr<OpenGLFixed> create(const std::string& vertexshader, const std::string& fragshader,
                int width_output, int height_output, int bytes_of_color_output,
                int width_0, int height_0, int bytes_of_color_0,
                int width_1 = 0, int height_1 = 0, int bytes_of_color_1 = 4,
                int width_2 = 0, int height_2 = 0, int bytes_of_color_2 = 4,
                int width_3 = 0, int height_3 = 0, int bytes_of_color_3 = 4,
                int width_4 = 0, int height_4 = 0, int bytes_of_color_4 = 4,
                int width_5 = 0, int height_5 = 0, int bytes_of_color_5 = 4,
                int width_6 = 0, int height_6 = 0, int bytes_of_color_6 = 4,
                int width_7 = 0, int height_7 = 0, int bytes_of_color_7 = 4
            );

			static std::shared_ptr<OpenGLFixed> create(const std::string& vshader, const std::string& fshader,int width, int height, int bytes_of_color);


			void SetVertexBuffer(std::vector<float> vertex_fan, std::vector<GLuint> vertex_element);

        private:
			GLuint _vbo;
			GLuint vertexarray;
			GLuint vertexelement;
			int indicescount;
            void init();

            bool render_filter(pbyte output,
                               cpbyte pixels_0,
                               cpbyte pixels_1 = nullptr,
                               cpbyte pixels_2 = nullptr,
                               cpbyte pixels_3 = nullptr,
                               cpbyte pixels_4 = nullptr,
                               cpbyte pixels_5 = nullptr,
                               cpbyte pixels_6 = nullptr,
                               cpbyte pixels_7 = nullptr
            );

        public:
			GLRenderCallBack rendercallback;
			
            bool Filter(pbyte output,
                        cpbyte pixels_0,
                        cpbyte pixels_1 = nullptr,
                        cpbyte pixels_2 = nullptr,
                        cpbyte pixels_3 = nullptr,
                        cpbyte pixels_4 = nullptr,
                        cpbyte pixels_5 = nullptr,
                        cpbyte pixels_6 = nullptr,
                        cpbyte pixels_7 = nullptr
            );
            bytes Filter(cpbyte pixels_0,
                         cpbyte pixels_1 = nullptr,
                         cpbyte pixels_2 = nullptr,
                         cpbyte pixels_3 = nullptr,
                         cpbyte pixels_4 = nullptr,
                         cpbyte pixels_5 = nullptr,
                         cpbyte pixels_6 = nullptr,
                         cpbyte pixels_7 = nullptr
            );
        };

    }
}


using globj = std::shared_ptr<ApowerSoft::ImageProcess::OpenGLFixed>;
globj GLFilter_Get(std::string key);       //Opengl ÂË¾µ»»³É
void GLFilter_Set(std::string key, globj obj);