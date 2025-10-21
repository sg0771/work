#pragma once

#include "./ff_filter.h"

class AVS_VF_Ffmpeg : public GenericVideoFilter {

	std::string m_filtersrc ="";
	int start = 0;
	int end = 0;
	//构造函数, 填充基本数据结构, 需要确定一点, 
	//构建多个filtercontext花费时间较多, 100个-> 4s
	//按需构造, getframe中再构造
public:
	static ff_vfcontext* s_vfcontext;
	AVS_VF_Ffmpeg(PClip _child, const char* filterstr, int start, int end, IScriptEnvironment* env);
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
	static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);
	~AVS_VF_Ffmpeg();
};