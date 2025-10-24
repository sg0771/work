#pragma once

#include <avisynth/avisynth.h>
#include <string>

#include "opengl_GlFilter.h"

class GlLayer : public GlFilter {
protected:
    PClip _child_1;
    VideoInfo _vi_1;

	std::shared_ptr< OpenGLFixed> create_processor() override;

public:


public:
    explicit GlLayer(const PClip &child, const PClip &child_1,
        int start, int end, int width, int height, const AVSValue &shaders,
        IScriptEnvironment *env);

    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment *env) override;

    std::string ToString() override;

};
