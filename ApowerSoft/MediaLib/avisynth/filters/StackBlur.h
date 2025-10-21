#pragma once

#include "avisynth/avisynth_stdafx.h"


class StackBlur : public GenericVideoFilter
{
public:
	StackBlur(PClip _child, IScriptEnvironment* env);
	~StackBlur();
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
	static AVSValue __cdecl Create(AVSValue args, void* mode, IScriptEnvironment* env);
	
	int Radius;
};

