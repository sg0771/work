#pragma once

#include <unordered_map>
#include <vector>
#include "reflector_Core.h"
#include <string>

struct ShaderProgramSource
{
	std::string VertexSource;
	std::string FragmentSource;
};


class OPENGL_API Shader
{
public:
	Shader(const std::string & filepath);
	Shader(const std::string & vertexshaderconstent, const std::string & fragshaderconstent);
	~Shader();
	void Bind() const;
	void Unbind() const;

	//set uniform
	void SetUniform1i(const std::string & name, int value);
	void SetUniform4f(const std::string & name, float v0, float v1, float v2, float v3);
	void SetUniform1f(const std::string & name, float v0);
	void SetUniform2f(const std::string & name, float v0, float v1);
	void SetUniform2fv(const std::string & name, int count, const float* values);
	void SetUniform3fv(const std::string& name, int count, const float* values);
	void SetUniform1iv(const std::string & name, int count, const int* values);

	ShaderProgramSource ParseShader(const std::string &filepath);
	void SetTexture(const std::string& name, std::vector<int> slots);
	
	unsigned int m_RendererID=0;
	std::string m_FilePath="";
	std::unordered_map<std::string,  int> m_UniformLocationCache;
	void Render(int indexcount);
	virtual void ApplyParameter();
	virtual void SetParameter(void * param) {}
	virtual void SetProgress(float duration, float time) {
		m_duration = duration;
		m_time = time;
	};
	virtual float GetTime() { return m_time; };
	virtual float GetDuration() { return m_duration; };
private:
	unsigned int CompileShader(unsigned int type, const std::string & source);
	unsigned int CreateShader(const std::string & vertshader, const std::string & fragmentshader);
	 int GetUniformLocation(const std::string& name);
	 float m_duration, m_time;

};

