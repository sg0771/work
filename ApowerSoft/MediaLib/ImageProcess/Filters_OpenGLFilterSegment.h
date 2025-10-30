#pragma once
#include <string>
class OpenGLFilterSegment {
	unsigned int pixelshader = 0;
	std::string pixelshaderpath;

public:
	OpenGLFilterSegment(const std::string &pixelshader);
	~OpenGLFilterSegment();
	
	void Bind()const;
	void Unbind()const;
};