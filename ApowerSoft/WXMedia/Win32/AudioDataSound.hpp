//
//  AudioDataSound.hpp
//  WXMedia
//
//  Created by tam on 2019/12/19.
//  Copyright © 2019年 xkt.spacexkt.space. All rights reserved.
//  录制投屏声音
//  输入输出S16 数据
//

#ifndef AudioDataSound_hpp
#define AudioDataSound_hpp

#include <WXMediaCpp.h>

class AudioDataSound{

    int m_inSampleRate = 48000;
    int m_inChannel = 2;
    
    int m_outSize = AUDIO_FRAME_SIZE;
    int m_outSampleRate = AUDIO_SAMPLE_RATE;
    int m_outChannel = AUDIO_CHANNELS;

    //BOOL m_bEnable = FALSE;

    int64_t m_nTotal = 0;

    std::shared_ptr<WXDataBuffer> m_tmpBuffer = nullptr;
    std::shared_ptr<AudioResampler>m_resampler = nullptr;// = L"AudioDataSound";
    std::shared_ptr<WXFifo> m_outputFifo = nullptr;

public:
    void Enable(BOOL b) {
     //   m_bEnable = b;
    }
    AudioDataSound(int outSampleRate, int outChannel);
    virtual ~AudioDataSound();
    WXFifo* GetOutput() {
        return m_outputFifo.get();
    }
    void Push(int sample_rate, int channel, uint8_t *buf, int buf_size);
};

#endif /* AudioDataSound_hpp */
