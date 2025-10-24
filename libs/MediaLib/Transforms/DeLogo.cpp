
#include "Utils.hpp"
#include <avisynth/avisynth.h>
#include "LibDelogoAPI.h"

using base = GenericVideoFilter;

class DeLogo : public GenericVideoFilter {
protected:
    std::string _error;

    int _frame_start, _frame_end, _border;

    WXPixelFMT _fmt;
    std::vector<std::array<int, 4>> _rects;

    VideoInfo _vi_0;

public:


public:
    explicit DeLogo(const PClip& child,
        int start, int end, const char* rects, int border,
        IScriptEnvironment* env);
    ~DeLogo() override;

    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) override;

    //void __stdcall Invoke(const char* name, const void* args) override;
    void UpdateRects(const char* rectstr);
};



int CLAMP(const int& value, const int& low, const int& high)
{
	return value < low ? low : (value > high ? high : value);
}

AVSValue __cdecl Create_DeLogo(AVSValue args, void* user_data, IScriptEnvironment* env) {
    return new DeLogo(args[0].AsClip(), args[1].AsInt(0), args[2].AsInt(200000000),
        args[3].AsString(""), args[4].AsInt(0), env);
}


DeLogo::DeLogo(const PClip &_child,
    int start, int end, const char *rectstr, int border,
    IScriptEnvironment *env) : base{ _child,__FUNCTION__ }, _frame_start{ start }, _frame_end{ end }, _border{ border }, _vi_0{ vi } {

    if (!_vi_0.HasVideo() && !_vi_0.HasAudio()) {
        env->ThrowError("No Video && No Audio!");
    }

    switch (_vi_0.pixel_type) {
    case CS_BGR24:
        _fmt = WX_PIX_FMT_RGB;
        break;
    case CS_BGR32:
        _fmt = WX_PIX_FMT_RGBA;
        break;
    case CS_YV12:
        _fmt = WX_PIX_FMT_YUV420;
        break;
    default: {
        child = env->Invoke("ConvertToYV12", child).AsClip();
        _vi_0 = child->GetVideoInfo();
        _fmt = WX_PIX_FMT_YUV420;
    }
        break;
    }

    vi = _vi_0;

	UpdateRects(rectstr);
}


void DeLogo::UpdateRects(const char* rectstr)
{
	_rects.clear();
	try {
		std::string rects{ rectstr };
		if (!rects.empty()) {
			std::vector<std::string> elems;
			utils::StringExt::split(elems, rects, "|", utils::StringExt::split_no_empties);

			for (auto && rect : elems) {
				std::vector<std::string> vals;
				utils::StringExt::split(vals, rect, ",", utils::StringExt::split_no_empties);

				try {
					auto x = std::stoi(vals[0]);
					auto y = std::stoi(vals[1]);
					auto w = std::stoi(vals[2]);
					auto h = std::stoi(vals[3]);

					// std::cerr << "--DeLogo rect -> " << x << ", " << y << ", " << w << ", " << h << " @ " << vi.width << ", " << vi.height << ", " << _fmt << std::endl;

					x = CLAMP(x, std::min(0, vi.width - 1), std::max(0, vi.width - 1));
					y = CLAMP(y, std::min(0, vi.height - 1), std::max(0, vi.height - 1));
					w = CLAMP(w, std::min(0, vi.width - x), std::max(0, vi.width - x));
					h = CLAMP(h, std::min(0, vi.height - y), std::max(0, vi.height - y));

					// std::cerr << "--DeLogo rect -> " << x << ", " << y << ", " << w << ", " << h << std::endl;

					_rects.emplace_back(std::array<int, 4>{ x, y, w, h });
				}
				catch (...) {}
			}
		}
	}
	catch (...) {}

	if (!_rects.empty()) {
		_rects.resize(_rects.size());
	}
}

DeLogo::~DeLogo() {
    
}

PVideoFrame DeLogo::GetFrame(int n, IScriptEnvironment *env) {
    auto src = base::GetFrame(n, env);
    if (n < _frame_start || n > _frame_end) {
        return src;
    }

    env->MakeWritable(&src);

    const uint8_t *src_p[NUM_DATA_POINTERS]{ src->GetReadPtr(PLANAR_Y), src->GetReadPtr(PLANAR_U), src->GetReadPtr(PLANAR_V) };
    uint8_t *dst_p[NUM_DATA_POINTERS]{ src->GetWritePtr(PLANAR_Y), src->GetWritePtr(PLANAR_U), src->GetWritePtr(PLANAR_V) };
    int src_ls[NUM_DATA_POINTERS]{ src->GetPitch(PLANAR_Y), src->GetPitch(PLANAR_U), src->GetPitch(PLANAR_V) };

    std::vector<int*> rects;
    for (auto && rect : _rects) {
        rects.emplace_back(rect.data());
    }
    WXDelogos(dst_p, src_ls, const_cast<uint8_t **>(src_p), src_ls, vi.width, vi.height, _fmt, rects.data(), rects.size(), 4, _border);
   
    return src;

}
