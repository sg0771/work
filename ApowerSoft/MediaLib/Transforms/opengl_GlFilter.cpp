
#include "opengl_GlFilter.h"
#include "Utils.hpp"
#include <typeinfo>
#include <iostream>
#include <fstream>

#include <memory>
#include "Utils.hpp"


AVSValue __cdecl Create_GlFilter(AVSValue args, void* user_data, IScriptEnvironment* env) {

    //WXLogA("------------- OPENGL File Create [%s]", __FUNCTION__);

    return new GlFilter(args[0].AsClip(), args[1].AsInt(0), args[2].AsInt(200000000),
        args[3].AsInt(0), args[4].AsInt(0), args[5], args[6].AsString(""), env);
}

GlFilter::GlFilter(const PClip &child, const int start,
    const int end, const int width, const int height, const AVSValue &shaders, const AVSValue& vertexshader,
    IScriptEnvironment *env) : base{ child ,__FUNCTION__ }, _frame_count{ 0 }, _frame_start{ start }, _frame_end{ end }, _child_0{ child }, _vi_0{ child->GetVideoInfo() } {

    //usegl = true;

    if (!_vi_0.HasVideo() && !_vi_0.HasAudio()) {
        env->ThrowError("No Video && No Audio!");
    }

    switch (_vi_0.pixel_type) {
        case CS_BGR24:
        case CS_BGR32:
            break;
        default: {
            _child_0 = env->Invoke("ConvertToRGB32", _child_0).AsClip();
            _vi_0 = _child_0->GetVideoInfo();
        }
            break;
    }

    try {
        auto fps = env->GetTimelineInfo()->FrameRate;
        _vi_0.SetFPS(lround(fps * 1001), 1001);
    }
    catch (...) {}

    const auto orig_frames = _vi_0.num_frames;

    _frame_start = std::min(std::max(_frame_start, 0), orig_frames - 1);
    _frame_end = std::min(std::max(_frame_end, _frame_start), orig_frames - 1);
    _frame_count = _frame_end - _frame_start + 1;

    this->child = _child_0;
    this->vi = _vi_0;

    try {
        if (width > 0) {
            vi.width = width;
        }
        if (height > 0) {
            vi.height = height;
        }
    }
    catch (...) {}
	vertexshaderstr = strdup(vertexshader.AsString(""));
	fragshaderstr = strdup(shaders.AsString(""));
    
}

GlFilter::~GlFilter() {
    //Processors.remove( this->ToString());
}

std::shared_ptr< OpenGLFixed> GlFilter::create_processor() {
    
    
    if (!GLFilter_Get(this->ToString())) {
        if (vi.HasVideo()) {

            try {
                auto processor = ApowerSoft::ImageProcess::OpenGLFixed::create(vertexshaderstr, fragshaderstr,vi.width, vi.height, 4);
                GLFilter_Set(this->ToString(), processor);

                processor->SetDuration(_frame_count * vi.fps_denominator / static_cast<double>(vi.fps_numerator));

				
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
	return GLFilter_Get(this->ToString());
}
void __stdcall SetCustomUniforms11(GLuint shaderprogram) { };


std::string GlFilter::ToString() {
    return  fragshaderstr+ std::to_string(vi.width) + std::to_string(vi.height);

}

PVideoFrame GlFilter::GetFrame(int n, IScriptEnvironment *env) {
    auto src = base::GetFrame(n, env);

    if (n < _frame_start || n > _frame_end ) {
        return src;
    }

    auto dst = env->NewVideoFrame(vi);
    if (dst == nullptr || dst.m_ptr == nullptr) {
        return nullptr;
    }
    try {
        _error.clear();

        auto processor = create_processor();

        
		SetFragParameters(processor);
        processor->SetTime((n - _frame_start) * vi.fps_denominator / static_cast<double>(vi.fps_numerator));
        processor->Filter(dst->GetWritePtr(), src->GetReadPtr());

    }
    catch (std::exception &ex) {
        std::ostringstream ss;
        ss << "++" << typeid(*this).name() << "::GetFrame exception: [" << utils::StringExt::trim(ex.what()) << "]";
        _error = ss.str();
    } catch (...) {
        std::ostringstream ss;
        ss << "++" << typeid(*this).name() << "::GetFrame exception.";
        _error = ss.str();
    }

    return dst;
}

