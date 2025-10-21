//
//  AudioDataSound.cpp
//  WXMedia
//
//  Created by tam on 2019/12/19.
//  Copyright © 2019年 xkt.spacexkt.space. All rights reserved.
//

#include "AudioDataSound.hpp"

static WXLocker s_lock;
static AudioDataSound *s_obj = nullptr;
WXMEDIA_API void AudioDataPush(int sample_rate, int channel, uint8_t *buf, int buf_size)
{
	WXAutoLock al(s_lock);
    if(s_obj){
        s_obj->Push(sample_rate, channel, buf, buf_size);
    }
}


AudioDataSound::AudioDataSound(int outSampleRate, int outChannel){
    WXAutoLock al(s_lock);
    m_outSampleRate = outSampleRate;
    m_outChannel = outChannel;
    m_outSize = m_outSampleRate * m_outChannel * 2 / 100;

    WXLogA("AudioDataSound m_outSize=%d", m_outSize);

    m_tmpBuffer = std::shared_ptr<WXDataBuffer>(new WXDataBuffer);
    m_resampler = std::shared_ptr < AudioResampler>(new AudioResampler(L"AudioDataSound"));
     
    m_outputFifo = std::shared_ptr<WXFifo>(new WXFifo);

    m_tmpBuffer->Init(nullptr, m_outSize);

    for (int i=0;i < 10;i++)
        m_outputFifo->Write(m_tmpBuffer->GetBuffer(), m_outSize);

    s_obj = this;
}

AudioDataSound::~AudioDataSound(){
    WXAutoLock al(s_lock);
    s_obj = nullptr;
    WXLogA("%s m_nTotal=%lld",__FUNCTION__, m_nTotal);
}


void AudioDataSound::Push(int sample_rate, int channel, uint8_t *buf, int buf_size){
    WXAutoLock al(s_lock);
    //if (!m_bEnable) {
    //    return;
    //}
    if (sample_rate == m_outSampleRate && channel == m_outChannel) {
        m_outputFifo->Write(buf, buf_size);//直接输出
        m_nTotal += buf_size;
    }else {

        if (m_inSampleRate!= sample_rate || m_inChannel != channel) {
            m_inSampleRate = sample_rate;
            m_inChannel = channel;
            m_resampler->Init(false, m_inSampleRate, m_inChannel, false, m_outSampleRate, m_outChannel);
        }
        m_resampler->Write(buf, buf_size);
        while (m_resampler->Size() >= m_outSize){
            m_resampler->Read(m_tmpBuffer->GetBuffer(), m_outSize);
            m_outputFifo->Write(m_tmpBuffer->GetBuffer(), m_outSize);

            m_nTotal += m_outSize;
        } 
    }
}
