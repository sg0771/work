#pragma once

#include <avisynth/avisynth.h>
#include "ImageProcess/ImageProcess_OpenGLFixed.h"
#include <Utils.hpp>
#include <wxlog.h>

using namespace ApowerSoft::ImageProcess;

using base = GenericVideoFilter;

class GlFilter : public GenericVideoFilter {
protected:
	std::string vertexshaderstr;
	std::string fragshaderstr;
    std::string _error;

    int _frame_count, _frame_start, _frame_end;

    PClip _child_0;
    VideoInfo _vi_0;

    virtual std::shared_ptr< OpenGLFixed> create_processor();
	virtual void SetFragParameters(std::shared_ptr< OpenGLFixed> processor) {};
	
public:




public:
    explicit GlFilter(const PClip &child,
        int start, int end, int width, int height, const AVSValue &shaders, const AVSValue& vertexshader,
        IScriptEnvironment *env);
    ~GlFilter() override;

    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment *env) override;

    virtual std::string ToString();
};

