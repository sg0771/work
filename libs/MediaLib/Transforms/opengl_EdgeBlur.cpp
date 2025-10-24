
#include "opengl_ChromakeyShaders.h"

#pragma once
#include "opengl_GlFilter.h"

class EdgeBlur : public GlFilter
{
public:
	EdgeBlur(const PClip& child, const int start, const int end, const int width, const int height, float edgeWidth,
		IScriptEnvironment* env);
	~EdgeBlur();


	float m_blurradius;

	void SetFragParameters(std::shared_ptr< OpenGLFixed> processor)override;



};



 AVSValue __cdecl Create_EdgeBlur(AVSValue args, void* user_data, IScriptEnvironment* env) {
	//WXLogA("------------- OPENGL File Create [%s]", __FUNCTION__);
	return new EdgeBlur(args[0].AsClip(), args[1].AsInt(0), args[2].AsInt(200000000),
		args[3].AsInt(0), args[4].AsInt(0), args[5].AsFloat(0.0f), env);
}

EdgeBlur::EdgeBlur(const PClip& child, const int start, const int end, const int width, const int height, float edgeWidth,
	IScriptEnvironment* env) : GlFilter(child, start, end, width, height, AVSValue(sEdgeBlur.c_str()), "", env)
{
	m_blurradius = edgeWidth;
}

EdgeBlur::~EdgeBlur()
{

}
void EdgeBlur::SetFragParameters(std::shared_ptr< OpenGLFixed> processor)
{

	processor->SetUniform("blurradius", 1, { this->m_blurradius });
}

