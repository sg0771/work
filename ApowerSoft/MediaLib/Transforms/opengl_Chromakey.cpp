
#include "opengl_ChromakeyShaders.h"
#include "Utils.hpp"

#include <avisynth/avisynth.h>
#include <string>
#include "opengl_GlFilter.h"

class Chromakey : public GlFilter
{
public:
	Chromakey(const PClip& child, const int start, const int end, const int width,
		const int height, float R, float G, float B, float dR, float dG, float dB, float _threshhold, float _blurSize,
		IScriptEnvironment* env);
	~Chromakey();

	float m_R;
	float m_G;
	float m_B;

	float m_dR;
	float m_dG;
	float m_dB;

	float m_threshhold;
	float m_blurSize;

	void SetFragParameters(std::shared_ptr< OpenGLFixed> processor)override;

	// 暂时不使用自动计算阈值的方法
	//PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) override;

};


AVSValue __cdecl Create_Chromakey(AVSValue args, void* user_data, IScriptEnvironment* env) {
	//WX//WXLogA("------------- OPENGL File Create [%s]", __FUNCTION__);
	return new Chromakey(args[0].AsClip(), args[1].AsInt(0), args[2].AsInt(200000000),
		args[3].AsInt(0), args[4].AsInt(0), args[5].AsFloat(0.0f), args[6].AsFloat(0.0f),
		args[7].AsFloat(0.0f), args[8].AsFloat(0.0f), args[9].AsFloat(0.0f),
		args[10].AsFloat(0.0f), args[11].AsFloat(0.0f), args[12].AsFloat(0.0f), env);
}

Chromakey::Chromakey(const PClip& child, const int start, const int end, const int width,
	const int height, float R, float G, float B, float dR, float dG, float dB, float _threshhold, float _blurSize,
	IScriptEnvironment* env) : GlFilter(child, start, end, width, height, AVSValue((G > R) ? sChromakeyShader.c_str() : sChromakeyShaderB.c_str()), "", env)
{
	m_R = R;
	m_G = G;
	m_B = B;
	m_dR = dR;
	m_dG = dG;
	m_dB = dB;

    m_threshhold = _threshhold;
}

Chromakey::~Chromakey()
{

}

void Chromakey::SetFragParameters(std::shared_ptr< OpenGLFixed> processor)
{
    // BGR格式
    float type = ((m_G > m_R) ? 0.0 : 1.0);
    processor->SetUniform("isBlueScreen", 1, { type });
    processor->SetUniform("colorDiffThreshold", 1, { m_threshhold });
}




