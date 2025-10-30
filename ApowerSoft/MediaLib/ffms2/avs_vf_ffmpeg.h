#pragma once

#include "./ff_filter.h"

class AVS_VF_Ffmpeg : public GenericVideoFilter {

	std::string m_filtersrc ="";
	int start = 0;
	int end = 0;
	//���캯��, ���������ݽṹ, ��Ҫȷ��һ��, 
	//�������filtercontext����ʱ��϶�, 100��-> 4s
	//���蹹��, getframe���ٹ���
public:
	static ff_vfcontext* s_vfcontext;
	AVS_VF_Ffmpeg(PClip _child, const char* filterstr, int start, int end, IScriptEnvironment* env);
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
	static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);
	~AVS_VF_Ffmpeg();
};