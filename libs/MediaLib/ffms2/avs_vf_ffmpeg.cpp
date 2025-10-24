
#include "avs_vf_ffmpeg.h"

ff_vfcontext* AVS_VF_Ffmpeg::s_vfcontext = NULL;

AVS_VF_Ffmpeg::AVS_VF_Ffmpeg(PClip _child, const char* _filterstr, int _start, int _end, IScriptEnvironment* env) :GenericVideoFilter(_child,__FUNCTION__ )
	{
		this->start = _start;
		this->end = _end;
		m_filtersrc = _filterstr;
	}

PVideoFrame AVS_VF_Ffmpeg::GetFrame(int n, IScriptEnvironment* env)
	{ 
		PVideoFrame frame= child->GetFrame(n,  env); 
		if (n<start||n>=end)
		{
			return frame;
		}
		
		if (AVS_VF_Ffmpeg::s_vfcontext == NULL)
		{
			AVS_VF_Ffmpeg::s_vfcontext = (ff_vfcontext*)malloc(sizeof(ff_vfcontext));
			init_filter_3dlut(AVS_VF_Ffmpeg::s_vfcontext, vi, m_filtersrc.c_str());
		}
		else if(AVS_VF_Ffmpeg::s_vfcontext->vi.width != vi.width || AVS_VF_Ffmpeg::s_vfcontext->vi.height != vi.height)
		{
			deinit_filter_3dlut(AVS_VF_Ffmpeg::s_vfcontext);
			init_filter_3dlut(AVS_VF_Ffmpeg::s_vfcontext, vi, m_filtersrc.c_str());
		}

		filter_3dlut(env, AVS_VF_Ffmpeg::s_vfcontext, frame);
		return frame;
	}

AVSValue __cdecl AVS_VF_Ffmpeg::Create(AVSValue args, void*, IScriptEnvironment* env)
{
	return new AVS_VF_Ffmpeg(args[0].AsClip(), args[1].AsString(), args[2].AsInt(), args[3].AsInt(), env);
}

AVS_VF_Ffmpeg::~AVS_VF_Ffmpeg()
{
	if (AVS_VF_Ffmpeg::s_vfcontext != NULL)
	{
		deinit_filter_3dlut(AVS_VF_Ffmpeg::s_vfcontext);
		AVS_VF_Ffmpeg::s_vfcontext = NULL;
	}
	
}