#include <windows.h>
#include <avisynth/avisynth.h>
EXTERN_C{
#include "fre.h"
}

using base = GenericVideoFilter;

class fremixer : public base {
public:
	explicit fremixer(const PClip& _child, const PClip& _RightClip, int _overlap, const char* filter_name, const char* filter_params,
		IScriptEnvironment* env);

	~fremixer() override;
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) override;
	bool __stdcall GetParity(int n);
	Frei0rContext* context;
	PClip RightClip;
	int overlap;
	__int64 video_fade_start;
	__int64 video_fade_end;

	__int64 audio_fade_end;
	__int64 audio_fade_start;
};



AVSValue __cdecl Create_FreMixer(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return new fremixer(args[0].AsClip(), args[1].AsClip(), args[2].AsInt(),
		args[3].AsString(""), args[4].AsString(""),
		env);
}

fremixer::fremixer(const PClip &_child, const PClip & _RightClip, int _overlap, const char* filter_name, const char* filter_params, IScriptEnvironment *env) : base{ _child,__FUNCTION__ },RightClip(_RightClip), overlap(_overlap) {

	memset(&vi, 0, sizeof vi);
	vi = _child->GetVideoInfo();
	context = new  Frei0rContext();
	context->dl_name = _strdup(filter_name);
	context->params = _strdup(filter_params);
	frei0r_init(context);
	if (context->construct == NULL) { //¿â¼ÓÔØÊ§°Ü!
		delete context;
		context = NULL;
		return;
	}

	context->instance = context->construct(vi.width, vi.height);
	frei0r_set_params(context, context->params);


	const VideoInfo& rvi = RightClip->GetVideoInfo();

	video_fade_start = vi.num_frames - overlap;
	video_fade_end = vi.num_frames - 1;

	audio_fade_end = vi.num_audio_samples - 1;
	audio_fade_start = vi.AudioSamplesFromFrames(video_fade_start);

	vi.num_frames = video_fade_start + rvi.num_frames;
}
fremixer::~fremixer() {}

PVideoFrame fremixer::GetFrame(int en, IScriptEnvironment *env) {

	if (en < video_fade_start)
	{
		return child->GetFrame(en, env);
	}
	if (en > video_fade_end)
	{
		return RightClip->GetFrame(en - video_fade_start, env);
	}
	int n = en - video_fade_start;

	PVideoFrame LeftFrame = child->GetFrame(en, env);
	if (LeftFrame == nullptr || LeftFrame.m_ptr == nullptr) {
		return nullptr;
	}
	PVideoFrame RightFrame = RightClip->GetFrame(n, env);
	if (RightFrame == nullptr || RightFrame.m_ptr == nullptr) {
		return nullptr;
	}
	PVideoFrame FilterFrame = child->GetFrame(n, env);
	if (FilterFrame == nullptr || FilterFrame.m_ptr == nullptr) {
		return nullptr;
	}
	const BYTE* src = FilterFrame->GetReadPtr();

	PVideoFrame dst = env->NewVideoFrame(vi);
	if (dst == nullptr || dst.m_ptr == nullptr) {
		return nullptr;
	}

	double param_x = 0.2 +0.2*((double)n) / overlap;

	context->set_param_value(context->instance, &param_x, 0);
	context->set_param_value(context->instance, &param_x, 2);
	context->update2(context->instance, (double)(n + 1), (const uint32_t*)LeftFrame->GetReadPtr(),(const uint32_t*)RightFrame->GetReadPtr(),NULL, (uint32_t*)dst->GetWritePtr());
	return dst;
}
bool fremixer::GetParity(int n)
{
	return (n < video_fade_start) ? child->GetParity(n) : RightClip->GetParity(n - video_fade_start);
}
