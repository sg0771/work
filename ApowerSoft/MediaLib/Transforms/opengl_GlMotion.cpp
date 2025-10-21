
#include "Utils.hpp"

//#include "zlog.h"
#include <typeinfo>

#pragma once

#include <avisynth/avisynth.h>
#include <string>

#include "opengl_GlLayer.h"

class GlMotion : public GlLayer {
protected:
    float _child_1_x, _child_1_y;

    std::shared_ptr< OpenGLFixed> create_processor() override;

public:


public:
    explicit GlMotion(const PClip& child, const PClip& child_1,
        int start, int end, int width, int height, const AVSValue& shaders, const AVSValue& rect,
        IScriptEnvironment* env);
};



AVSValue __cdecl Create_GlMotion(AVSValue args, void* user_data, IScriptEnvironment* env) {

    //WXLogA("------------- OPENGL File Create [%s]", __FUNCTION__);
    return new GlMotion(args[0].AsClip(), args[1].AsClip(), args[2].AsInt(0), args[3].AsInt(200000000),
        args[4].AsInt(0), args[5].AsInt(0), args[6], args[7], env);
}

GlMotion::GlMotion(const PClip &child, const PClip &child_1,
    const int start, const int end, const int width, const int height, const AVSValue &shaders, const AVSValue &child_1_position,
    IScriptEnvironment *env) : GlLayer{ child, child_1, start, end, width, height, shaders, env },
    _child_1_x{ 0 }, _child_1_y{ 0 } {

    if (child_1_position.IsArray()) {
        if (child_1_position.ArraySize() > 0) {
            _child_1_x = child_1_position[0].AsFloat();
        }
        if (child_1_position.ArraySize() > 1) {
            _child_1_y = child_1_position[1].AsFloat();
        }
    }
    else if (child_1_position.IsFloat()) {
        _child_1_x = child_1_position.AsFloat();
    }
}

std::shared_ptr< OpenGLFixed> GlMotion::create_processor() {
    
    if (!GLFilter_Get(this->ToString())) {
        if (vi.HasVideo()) {
            try {

                auto processor = ApowerSoft::ImageProcess::OpenGLFixed::create("", fragshaderstr,
                    vi.width, vi.height, vi.pixel_type == CS_BGR32 ? 4 : 3,
                    _vi_0.width, _vi_0.height, _vi_0.pixel_type == CS_BGR32 ? 4 : 3,
                    _vi_1.width, _vi_1.height, _vi_1.pixel_type == CS_BGR32 ? 4 : 3
                );
                GLFilter_Set(this->ToString(),processor);

                processor->SetDuration(_frame_count * vi.fps_denominator / static_cast<double>(vi.fps_numerator));
				processor->SetTEXTURE_WRAP_Parameter(GL_CLAMP_TO_EDGE);
            }
            catch (std::exception &ex) {
                std::ostringstream ss;
                ss << "++" << typeid(*this).name() << "::create_processor exception: [" << utils::StringExt::trim(ex.what()) << "]";
                _error = ss.str();
            }
            catch (...) {
                std::ostringstream ss;
                ss << "++" << typeid(*this).name() << "::create_processor exception.";
                _error = ss.str();
            }

            if (!_error.empty()) {
                
            }
        }
    }      
    auto processor = GLFilter_Get(this->ToString());
    processor->SetUniform("fChannelPosition", 2, { 0, 0, _child_1_x, _child_1_y, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 });
	return processor;
}
