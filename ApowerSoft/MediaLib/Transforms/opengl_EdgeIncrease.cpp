#include "opengl_ChromakeyShaders.h"

#include "opengl_GlFilter.h"
class EdgeIncrease : public GlFilter
{
public:
	EdgeIncrease(const PClip& child, const int start, const int end, const int width, const int height, float edgeWidth,
		IScriptEnvironment* env);
	~EdgeIncrease();


	float m_edgeWidth;

	void SetFragParameters(std::shared_ptr< OpenGLFixed> processor)override;


};



 AVSValue __cdecl Create_EdgeIncrease(AVSValue args, void* user_data, IScriptEnvironment* env) {
	//WXLogA("------------- OPENGL File Create [%s]", __FUNCTION__);
	return new EdgeIncrease(args[0].AsClip(), args[1].AsInt(0), args[2].AsInt(200000000),
		args[3].AsInt(0), args[4].AsInt(0), args[5].AsFloat(0.0f), env);
}

EdgeIncrease::EdgeIncrease(const PClip& child, const int start, const int end, const int width, const int height, float edgeWidth,
	IScriptEnvironment* env) : GlFilter(child, start, end, width, height, AVSValue(sEdgeIncrease.c_str()), "", env)
{
	m_edgeWidth = edgeWidth;
}

EdgeIncrease::~EdgeIncrease()
{

}
void EdgeIncrease::SetFragParameters(std::shared_ptr< OpenGLFixed> processor)
{

	processor->SetUniform("edge", 1, { this->m_edgeWidth });
}

