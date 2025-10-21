#pragma once

/*
输入48000Hz 2Ch S16 数据
NS+VAD处理
*/

#include "avisynth/avisynth_stdafx.h"

#include "apm/apm.h"

class Gips : public GenericVideoFilter
	/**
	  * Amplify a clip's audio track
	 **/
{
public:
	Gips(PClip _child, int channel);
	virtual ~Gips();
	void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env);

	static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);
	static Gips* instance;

private:
	int m_nSampleRate = 48000;
	int m_nChannel = 1;
	int m_nSize = 960; ///480x4
	void* m_pApm = nullptr;

};