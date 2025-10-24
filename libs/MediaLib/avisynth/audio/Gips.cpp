#include "avisynth/avisynth_stdafx.h"
#include "Gips.h"
#include "convertaudio.h"

//
//extern "C" {
//#include "..\..\include\libswresample\swresample.h"
//}


//构造webrtc那几个instance
Gips::Gips(PClip _child, int channel) : GenericVideoFilter(ConvertAudio::Create(_child, SAMPLE_INT16 | SAMPLE_FLOAT | SAMPLE_INT32, SAMPLE_FLOAT), __FUNCTION__)
{
	m_nChannel = channel;
	m_nSize = 48000 * 2 * m_nChannel / 100;//1920
	m_pApm = ApmCreate(m_nSampleRate, m_nChannel, 1, 1, 1);
}

Gips::~Gips()
{
	if (m_pApm) {
		ApmDestroy(m_pApm);
		m_pApm = nullptr;
	}
}

//假定输入都是48000双通道 10ms  S16数据  960 个short 1920byte 
//count 应该是sample数量 也就是 short的数量
void __stdcall Gips::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env)
{
	child->GetAudio(buf, start, count, env);
	
	//每次读取10msdata
	uint8_t* pcm = (uint8_t*)buf;

	int totalSize = (int)(count * sizeof(int16_t));

	for (int  i = 0; i < totalSize / m_nSize; i++)
	{
		ApmProcess(m_pApm, pcm + i * m_nSize);
	}
}

Gips* Gips::instance;
AVSValue __cdecl Gips::Create(AVSValue args, void*, IScriptEnvironment* env) {
	if (!args[0].AsClip()->GetVideoInfo().AudioChannels())
		return args[0];
	return new Gips(args[0].AsClip(), args[1].AsInt()); 
}

