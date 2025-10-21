#pragma once
#include <avisynth/avisynth.h>
#include <string>
#include "opengl_SimpleGLFilter.h"
#include "ImageProcess/filters_OpenGLFilter.h"
#include "ImageProcess/reflector_Shader.h"
#include "opengl_ChromakeyShaders.h"

struct ChromakeyExContext
{
	float red;
	float green;
	float blue;

	float dred;
	float dgreen;
	float dblue;

	float _Threshhold;
	float _BlurSize;
};

class ChromakeyExShader : public Shader
{
public:

	ChromakeyExShader(const std::string& vertexsource, const std::string& fragsource) :Shader(vertexsource, fragsource)
	{
	}
	void ApplyParameter()
	{
		SetUniform1f("R", context.red);
		SetUniform1f("G", context.green);
		SetUniform1f("B", context.blue);

		SetUniform1f("dR", context.dred);
		SetUniform1f("dG", context.dgreen);
		SetUniform1f("dB", context.dblue);

		SetUniform1f("_Threshhold", context._Threshhold);
		SetUniform1f("_BlurSize", context._BlurSize);
	}
	void SetParameter(void* param)
	{
		context = *(ChromakeyExContext*)param;
	}
private:
	ChromakeyExContext context;
};

class ChromakeyEx : public SimpleGLFilter
{
public:
	ChromakeyEx(const PClip& child, const int start, const int end, const int width, const int height, 
		int color, int dcolor, float _Threshhold, float _BlurSize, IScriptEnvironment* env)
		: SimpleGLFilter(child, { sChromakeyShader  }, start, end, env)
	{
		auto r = (color & 0xff0000) >> 16;
		auto g = (color & 0xff00) >> 8;
		auto b = color & 0xff;

		auto R = static_cast<float>(r) / 255.0;
		auto G = static_cast<float>(g) / 255.0;
		auto B = static_cast<float>(b) / 255.0;

		auto dr = (dcolor & 0xff0000) >> 16;
		auto dg = (dcolor & 0xff00) >> 8;
		auto db = dcolor & 0xff;

		auto dR = static_cast<float>(dr) / 255.0;
		auto dG = static_cast<float>(dg) / 255.0;
		auto dB = static_cast<float>(db) / 255.0;


		param.red = R;
		param.green = G;
		param.blue = B;

		param.dred = dR;
		param.dgreen = dG;
		param.dblue = dB;

		param._Threshhold = _Threshhold;
		param._BlurSize = _BlurSize;
	}
	~ChromakeyEx() {};
	void InitProcess() override
	{
		processor = new OpenGLFilter();
		processor->AddShader(new ChromakeyExShader(vertexshaderstr, fragshaderstrs[0]));
		processor->AddShader(new ChromakeyExShader(vertexshaderstr, fragshaderstrs[1]));

		processor->SetParameter(&param);
	};


private:
	ChromakeyExContext param;

};



AVSValue __cdecl Create_ChromakeyEx(AVSValue args, void* user_data, IScriptEnvironment* env) {
	////WXLogA("------------- OPENGL File Create [%s]", __FUNCTION__);
	return new ChromakeyEx(args[0].AsClip(), args[1].AsInt(0), args[2].AsInt(200000000),
		args[3].AsInt(0), args[4].AsInt(0), args[5].AsInt(0), args[6].AsInt(0),
		args[7].AsFloat(0.0f), args[8].AsFloat(0.0f), env);
}