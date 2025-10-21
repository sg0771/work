#pragma once
#include <avisynth/avisynth.h>
#include <string>
#include "opengl_GlFilter.h"
class GLColorAdjust : public GlFilter
{
public:
	GLColorAdjust(const PClip& child, const int start, const int end, const int width, const int height,
		float _saturation, float hue, float brightness, float contrast, float highlights, float shadows,
		const AVSValue& fragshaders, IScriptEnvironment* env);
	~GLColorAdjust();
	float saturation;
	float hue;
	float brightness;
	float contrast;
	float highlights;
	float shadows;

	void SetFragParameters(std::shared_ptr< OpenGLFixed> processor)override;

};



 AVSValue __cdecl Create_GLColorAdjust(AVSValue args, void* user_data, IScriptEnvironment* env) {
	//WXLogA("------------- OPENGL File Create [%s]", __FUNCTION__);
	return new GLColorAdjust(args[0].AsClip(), args[1].AsInt(0), args[2].AsInt(200000000),
		args[3].AsInt(0), args[4].AsInt(0), args[5].AsFloat(0.0f), args[6].AsFloat(0.0f), args[7].AsFloat(0.0f), args[8].AsFloat(0.0f), args[9].AsFloat(0.0f), args[10].AsFloat(0.0f), args[11], env);
}

GLColorAdjust::GLColorAdjust(const PClip &child, const int start, const int end, const int width, const int height,float _saturation, float _hue,float _brightness,float _contrast, float _highlights, float _shadows, const AVSValue &shaders,
	IScriptEnvironment *env) : GlFilter( child, start, end, width, height, shaders,"", env )
{
	saturation = _saturation;
	hue = _hue;
	brightness = _brightness;
	contrast = _contrast;
	highlights = _highlights;
	shadows = _shadows;
}

GLColorAdjust::~GLColorAdjust()
{ 
	
}
void GLColorAdjust::SetFragParameters(std::shared_ptr< OpenGLFixed> processor)
{
	
	processor->SetUniform("Saturation", 1, { this->saturation });
	processor->SetUniform("Hue", 1, { this->hue });
	processor->SetUniform("Brightness", 1, { this->brightness });
	processor->SetUniform("Contrast", 1, { this->contrast });
	processor->SetUniform("Highlights", 1, { this->highlights });
	processor->SetUniform("Shadows", 1, { this->shadows });
}

