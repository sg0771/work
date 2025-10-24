#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include "reflector_Shader.h"
#include "reflector_Renderer.h"
#include "../glew/glew.h"

#include <wxlog.h>


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

Shader::Shader(const std::string & filepath):m_FilePath(filepath),m_RendererID(0)
{
	
	ShaderProgramSource shadersource = ParseShader(filepath);
	m_RendererID = CreateShader(shadersource.VertexSource, shadersource.FragmentSource);
}

Shader::Shader(const std::string & vertexsource, const std::string & fragmentsource)
{
	m_RendererID = CreateShader(vertexsource, fragmentsource);
}

Shader::~Shader()
{
	GLCall(glDeleteProgram(m_RendererID));
}

void Shader::Bind() const
{
	GLCall(glUseProgram(m_RendererID));
}

void Shader::Unbind() const
{
	GLCall(glUseProgram(0));
}

void Shader::SetUniform4f(const std::string & name, float v0, float v1, float v2, float v3)
{
	GLCall(glUniform4f(GetUniformLocation(name), v0, v1, v2, v3));
}

void Shader::SetUniform1i(const std::string & name, int value)
{
	GLCall(glUniform1i(GetUniformLocation(name), value));
}
void Shader::SetUniform1f(const std::string & name, float v0)
{
	GLCall(glUniform1f(GetUniformLocation(name), v0));
}
void Shader::SetUniform2f(const std::string & name, float v0, float v1)
{
	GLCall(glUniform2f(GetUniformLocation(name), v0, v1));
}


void Shader::SetUniform1iv(const std::string & name, int count,  const int* values)
{
	GLCall(glUniform1iv(GetUniformLocation(name), count, values));
}

void Shader::SetUniform2fv(const std::string & name, int count, const float* values)
{
	GLCall(glUniform2fv(GetUniformLocation(name), count, values));
}

void Shader::SetUniform3fv(const std::string& name, int count, const float* values)
{
	GLCall(glUniform3fv(GetUniformLocation(name), count, values));
}

 int Shader::GetUniformLocation(const std::string& name)
{
	if (m_UniformLocationCache.find(name)!= m_UniformLocationCache.end())
		return m_UniformLocationCache[name];
	
	GLCall( int location = glGetUniformLocation(m_RendererID, name.c_str()));
	if (location==-1)
		std::cout << "waring uniform"<<std::endl;

	m_UniformLocationCache[name] = location;
	return location;
}


 ShaderProgramSource Shader::ParseShader(const std::string &filepath)
{
	std::fstream stream(filepath);

	enum class ShaderType
	{
		None = -1, VERTEX = 0, FRAGMENT = 1
	};

	ShaderType type;
	std::string line;
	std::stringstream ss[2];

	while (getline(stream, line))
	{
		if (line.find("#shader") != std::string::npos)
		{
			if (line.find("vertex") != std::string::npos)
				type = ShaderType::VERTEX;
			else if (line.find("fragment") != std::string::npos)
				type = ShaderType::FRAGMENT;
		}
		else
		{
			ss[(int)type] << line << "\n";
		}
	}
	return { ss[0].str(),ss[1].str() };
}

 
 void Shader::SetTexture(const std::string & name, std::vector< int> slots)
 {
	 SetUniform1iv(name, slots.size(), slots.data());
 }


 void Shader::Render(int indexcount)
 {
	 ApplyParameter();
	 
	 GLCall(glDrawElements(GL_TRIANGLES, indexcount, GL_UNSIGNED_INT, nullptr));
 }

 void Shader::ApplyParameter()
 {
 }

 unsigned int Shader::CompileShader(unsigned int type, const std::string& source) {

	GLCall( unsigned int id = glCreateShader(type));
	const char* src = source.c_str();
	GLCall(glShaderSource(id, 1, &src, 0));
	GLCall(glCompileShader(id));

	int result;
	GLCall(glGetShaderiv(id, GL_COMPILE_STATUS, &result));
	if (result == GL_FALSE)
	{
		int length;
		GLCall(glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length));
		char * message = (char*)alloca(length * sizeof(char));
		GLCall(glGetShaderInfoLog(id, length, &length, message));
		//Log_info("failed to compile %s, \n %s", (type == GL_VERTEX_SHADER ? "vertex" : "fragment", source.c_str()));
		//Log_info(message);
		/*std::cout << "failed to compile " << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") << std::endl;
		std::cout << message << std::endl;
*/
		GLCall(glDeleteShader(id));
	}

	return id;
}

unsigned int Shader::CreateShader(const std::string& vertshader, const std::string& fragmentshader)
{
	GLCall(unsigned int program = glCreateProgram());

	unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertshader);
	unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentshader);
	GLCall(glAttachShader(program, vs));
	GLCall(glAttachShader(program, fs));
	GLCall(glLinkProgram(program));
	GLCall(glValidateProgram(program));

	GLCall(glDeleteShader(vs));
	GLCall(glDeleteShader(fs));

	return program;
}


