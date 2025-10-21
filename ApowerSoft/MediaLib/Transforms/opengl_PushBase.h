#pragma once
#include <avisynth/avisynth.h>
#include <string>
#include "opengl_GlFilter.h"
#include "opengl_GlLayer.h"

using base = GenericVideoFilter;
class PushBase : public GenericVideoFilter
{
	std::string m_strVertexShader = "";
	std::string m_error = "";

	int m_frame_count = 0;

	PClip _child_0;
	PClip _child_1;
	VideoInfo _vi;
public:
	PushBase(const PClip &child, const PClip &child2, const int duration, const AVSValue &vshader, IScriptEnvironment *env);
	virtual ~PushBase();

	virtual std::shared_ptr< OpenGLFixed> create_processor();
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment *env) override;

	virtual std::string ToString();
};

