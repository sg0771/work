#pragma once
#include <avisynth/avisynth.h>
#include <string>
#include "opengl_SimpleGLFilter.h"
#include "opengl_ColorAdjustFilter.h"

class ColorAdjust :	public SimpleGLFilter
{
public:
	ColorAdjust(const PClip &child, const int start, const int end, const int width, const int height, float _saturation, float _hue, float _brightness, float _contrast, float _highlights, float _shadows, const AVSValue &shaders,
		IScriptEnvironment *env) : SimpleGLFilter(child, { shaders.AsString() }, start, end, env)
	{
		param.sat = _saturation;
		param.hue = _hue;
		param.bright = _brightness;
		param.cont = _contrast;
		param.highlights = _highlights;
		param.shadows = _shadows;
	}
	~ColorAdjust() {};

	void InitProcess() override
	{
		processor = new OpenGLFilter();
		processor->AddShader(new ColorAdjustShader(vertexshaderstr, fragshaderstrs[0]));
		processor->SetParameter(&param);
	};
	

	private:
		ColorAdjustContext param;

};


AVSValue __cdecl Create_ColorAdjust(AVSValue args, void* user_data, IScriptEnvironment* env) {
	////WXLogA("------------- OPENGL File Create [%s]", __FUNCTION__);

	return new ColorAdjust(args[0].AsClip(), args[1].AsInt(0), args[2].AsInt(200000000),
		args[3].AsInt(0), args[4].AsInt(0), args[5].AsFloat(0.0f), args[6].AsFloat(0.0f), args[7].AsFloat(0.0f), args[8].AsFloat(0.0f), args[9].AsFloat(0.0f), args[10].AsFloat(0.0f), args[11], env);
}
