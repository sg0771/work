/*
音频混音器
by TamXie
*/

#include  "AudioMixer.h"

AudioMixer::AudioMixer(WXCTSTR wszName) {
    m_strName.Format(wszName);
    m_bufBase.Init(nullptr, AUDIO_FRAME_SIZE);
    m_bufAdd.Init(nullptr, AUDIO_FRAME_SIZE);
}

AudioMixer::~AudioMixer() {
    if (m_t1) {
        double ff = (double)m_t2 / (double)m_t1;
        WXLogA("AudioMixer m_t1=%lld m_t2=%lld Rate=%0.2f", m_t1, m_t2, ff);
    }
}

//强制10ms运行一次
void AudioMixer::Process() {
    memset(m_bufBase.GetBuffer(), 0, AUDIO_FRAME_SIZE);
    if (m_nCount == 1) {
        int ret = m_vecInput[0]->Read2(m_bufBase.GetBuffer(), AUDIO_FRAME_SIZE);
        m_t1 += AUDIO_FRAME_SIZE;
        m_t2 += ret;
    }else {
        for (int i = 0; i < m_nCount; i++) {//混音叠加
            m_vecInput[i]->Read2(m_bufAdd.GetBuffer(), AUDIO_FRAME_SIZE);
            AddAudio((int16_t*)m_bufBase.GetBuffer(), (int16_t*)m_bufAdd.GetBuffer(), AUDIO_FRAME_SIZE / 2);
        }
    }
    m_outFifo.Write(m_bufBase.GetBuffer(), AUDIO_FRAME_SIZE);//写数据
}

void AudioMixer::AddInput(WXFifo *obj) {
	m_vecInput.push_back(obj);
    m_nCount++;
}

WXFifo* AudioMixer::GetOutput() {
	return  &m_outFifo;
}



