
#pragma once

#include <avisynth/avisynth.h>
#include <string>
#include <memory>
#include "ImageProcess/Filters_OpenGLFilter.h"
#include "utils.hpp"
#include "opengl_CommonShaderContent.h"

#include <wxlog.h>

using base = GenericVideoFilter;
  
class SimpleGLFilter : public GenericVideoFilter {
protected:
	std::string vertexshaderstr;
	std::vector<std::string> fragshaderstrs;

	int _frame_count, _frame_start, _frame_end;
	std::vector<PClip > clips;
	OpenGLFilter* processor;
	public:


public:
	explicit SimpleGLFilter(const AVSValue &childs, const std::vector<std::string> &fragmentshaders, const int start,
		const int end, IScriptEnvironment *env);

	~SimpleGLFilter() override;
	virtual void InitProcess();
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment *env) override;
};
