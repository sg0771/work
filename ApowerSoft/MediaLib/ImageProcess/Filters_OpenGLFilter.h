#pragma once
#include <vector>
#include <imageprocess/reflector_Shader.h>
#include "Utils.hpp"

class OPENGL_API IOpenGLFilter
{
protected:
float m_duration, m_time;
public:
	 virtual void Render(std::vector< FilterFrame> frames, int width, int height, void* output = NULL)=0;
	 virtual void UpdateProgress(float duration, float time) {
		 m_duration = duration;
		 m_time = time;
	 }
	 void virtual SetParameter(void * param) {}
 };

class OPENGL_API  IMulGLFilter: public IOpenGLFilter
 {
 public:
	 virtual void Render(std::vector< FilterFrame> frames, int width, int height, void* output = NULL) = 0;
	 std::vector<IOpenGLFilter*> childFilters;
	 virtual void ApplyParameter() {
	 }
	 void virtual SetParameter(void * param);
	 void UpdateProgress(float duration, float time) override;
 };

class OPENGL_API  OpenGLFilter:public IOpenGLFilter {

public:
	OpenGLFilter(int _baseslot = 0) : baseslot(_baseslot) {}
	OpenGLFilter(Shader* shader, int _baseslot = 0);
	~OpenGLFilter() {}
	virtual void Render(std::vector< FilterFrame> frames, int width, int height, void* output = NULL);
	virtual void Render(std::vector< FilterFrame> frames, const std::vector<float>& sizes, void* output = NULL);
	void UpdateProgress(float duration, float time) override;
	
	void virtual SetParameter(void * param);
	void AddShader(Shader * shader);

protected:
	std::vector<Shader*>shaders;
	//ת��, �˾�����Ҫ�߼�ʹ��0,1��texture slot, 
	//��Ч,, ʹ��4,5��texture slot
	int baseslot=0;

	};



