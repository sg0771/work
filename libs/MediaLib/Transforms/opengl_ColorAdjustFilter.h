#pragma once

#include <MediaLibAPI.h>
#include "ImageProcess/Filters_OpenGLFilter.h"
#include "ImageProcess/reflector_Shader.h"


class ColorAdjustShader : public Shader
{
public:

	ColorAdjustShader(const std::string & vertexsource, const std::string & fragsource) :Shader(vertexsource, fragsource)
	{
	}
	void ApplyParameter()
	{
		SetUniform1f("Saturation", context.sat);
		SetUniform1f("Hue", context.hue);
		SetUniform1f("Brightness", context.bright);
		SetUniform1f("Contrast", context.cont);
		SetUniform1f("Highlights", context.highlights);
		SetUniform1f("Shadows", context.shadows);/**/
	}
	void SetParameter(void * param)
	{
		context = *(ColorAdjustContext*)param;
	}
private:
	ColorAdjustContext context;
};



