#include "opengl_PushBase.h"

AVSValue __cdecl Create_PushBase(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return new PushBase(args[0].AsClip(), args[1].AsClip(), args[2].AsInt(200000000), args[3], env);
}
PushBase::PushBase(const PClip &_child, const PClip &_child2, const int duration, const AVSValue &vshader, IScriptEnvironment *env):base(child, __FUNCTION__)
{
	this->child = child;
	_child_1 = _child2;
	this->vi = _child_0->GetVideoInfo();
	m_strVertexShader = vshader.AsString();
}
std::string PushBase::ToString() {
	return m_strVertexShader + std::to_string(vi.width) + "x" + std::to_string(vi.height);

}
std::shared_ptr< OpenGLFixed> PushBase::create_processor() {
	
	if (!GLFilter_Get(this->ToString())) {
		if (vi.HasVideo()) {

			auto processor = ApowerSoft::ImageProcess::OpenGLFixed::create(m_strVertexShader, "",
				vi.width, vi.height, 4);
			try {
				GLFilter_Set(this->ToString(),processor);
				processor->SetDuration(m_frame_count * vi.fps_denominator / static_cast<double>(vi.fps_numerator));
			}
			catch (...) {
				
			}
		}
	}
	return GLFilter_Get(this->ToString());
}


PVideoFrame PushBase::GetFrame(int n, IScriptEnvironment *env) {
	auto src = base::GetFrame(n, env);	
	if (src == nullptr || src.m_ptr == nullptr) {
		return nullptr;
	}

	auto dst = env->NewVideoFrame(vi);
	if (dst == nullptr || dst.m_ptr == nullptr) {
		return nullptr;
	}
	return dst;
//逐个对FilterFrame调用step处理
//每一个step输出到同一个framebuffer中
}
PushBase::~PushBase()
{
}
