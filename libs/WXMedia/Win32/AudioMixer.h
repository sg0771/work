/*
音频混音器
输入输出都是指定格式PCM16音频数据
*/
#ifndef _AUDIO_MIXER_H_
#define _AUDIO_MIXER_H_

#include <WXMediaCpp.h>
class AudioMixer{
	std::vector<WXFifo*>m_vecInput;
	int m_nCount = 0;
	WXFifo m_outFifo;//输出缓存
	WXDataBuffer m_bufBase;
	WXDataBuffer m_bufAdd;
    WXString m_strName = L"AudioMixer";

	int64_t m_t1 = 0;
	int64_t m_t2 = 0;
public:
	AudioMixer(WXCTSTR wszName);
	virtual ~AudioMixer();
	void AddInput(WXFifo *obj);
	WXFifo* GetOutput();//输出
	void Process();
};

#endif

