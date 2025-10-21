
#include "Utils.hpp"
#include <typeinfo>
#include "stb_image.h"

#pragma once
#include <avisynth/avisynth.h>
#include <string>
#include "opengl_GlFilter.h"

class lut_2d : public GlFilter
{
protected:
    PClip _child_1;
    VideoInfo _vi_1;
    std::shared_ptr< OpenGLFixed> create_processor() override;
public:
    explicit lut_2d(const PClip& child, const PClip& child_1,
        int start, int end, int width, int height, const AVSValue& shaders,
        IScriptEnvironment* env);

    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) override;

    std::string ToString() override;
};

 AVSValue __cdecl Create_lut_2d(AVSValue args, void* user_data, IScriptEnvironment* env) {

    //WXLogA("------------- OPENGL File Create [%s]", __FUNCTION__);

    return new lut_2d(args[0].AsClip(), args[1].AsClip(), args[2].AsInt(0), args[3].AsInt(200000000),
        args[4].AsInt(0), args[5].AsInt(0), args[6], env);
}

lut_2d::lut_2d(const PClip& child, const PClip& child_1,
    const int start, const int end, const int width, const int height, const AVSValue& shaders,
    IScriptEnvironment* env) : GlFilter{ child, start, end, width, height, shaders,"", env }, _child_1{ child_1 }, _vi_1{ child_1->GetVideoInfo() } {

    if (!_vi_1.HasVideo() && !_vi_1.HasAudio()) {
        env->ThrowError("No Video && No Audio!");
    }

    switch (_vi_1.pixel_type) {
    case CS_BGR24:
    case CS_BGR32:
        break;
    default: {
        _child_1 = env->Invoke("ConvertToRGB32", _child_1).AsClip();
        _vi_1 = _child_1->GetVideoInfo();
    }
           break;
    }

    try {
        _vi_1.SetFPS(vi.fps_numerator, vi.fps_denominator);
    }
    catch (...) {}
}

std::string lut_2d::ToString()
{
    return  __super::ToString() + std::to_string(_vi_0.width) + std::to_string(_vi_0.height) + std::to_string(_vi_1.width) + std::to_string(_vi_1.height) + std::to_string(_vi_1.width) + std::to_string(_vi_1.height);
}

std::shared_ptr< OpenGLFixed> lut_2d::create_processor() {

    if (!GLFilter_Get(this->ToString())) {
        if (vi.HasVideo()) {
            try {
                auto processor = ApowerSoft::ImageProcess::OpenGLFixed::create("", fragshaderstr,
                    vi.width, vi.height, vi.pixel_type == CS_BGR32 ? 4 : 3,
                    _vi_0.width, _vi_0.height, _vi_0.pixel_type == CS_BGR32 ? 4 : 3,
                    _vi_1.width, _vi_1.height, _vi_1.pixel_type == CS_BGR32 ? 4 : 3
                );


                GLFilter_Set(this->ToString(), processor);
                processor->SetTEXTURE_WRAP_Parameter(GL_MIRRORED_REPEAT);

                processor->SetDuration(_frame_count * vi.fps_denominator / static_cast<double>(vi.fps_numerator));
            }
            catch (std::exception& ex) {
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

PVideoFrame lut_2d::GetFrame(int n, IScriptEnvironment* env) {
    auto src = child->GetFrame(n, env);
    if (src == nullptr || src.m_ptr == nullptr) {
        return nullptr;
    }
    if (n < _frame_start || n > _frame_end || !_vi_1.HasVideo()) {
        return src;
    }

    auto src_1 =  _child_1->GetFrame(0, env);
    if (src_1 == nullptr || src_1.m_ptr == nullptr) {
        return nullptr;
    }

    if (!_vi_0.HasVideo()) {
        return src_1;
    }

    auto dst = env->NewVideoFrame(vi);
    if (dst == nullptr || dst.m_ptr == nullptr) {
        return nullptr;
    }
    try {
        _error.clear();

        auto processor = create_processor();


        processor->SetTime((n - _frame_start) * vi.fps_denominator / static_cast<double>(vi.fps_numerator));
        processor->Filter(dst->GetWritePtr(), src->GetReadPtr(), src_1->GetReadPtr());
    }
    catch (std::exception& ex) {
        std::ostringstream ss;
        ss << "++" << typeid(*this).name() << "::GetFrame exception: [" << utils::StringExt::trim(ex.what()) << "]";
        _error = ss.str();
    }
    catch (...) {
        std::ostringstream ss;
        ss << "++" << typeid(*this).name() << "::GetFrame exception.";
        _error = ss.str();
    }



    return dst;
}
