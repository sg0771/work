/*
音频混音器
输入PCM16数据，然后定时取出混音，发给Mixer
by TamXie
*/
#ifndef _AUDIO_MIXER_WITH_PROCESS_H_
#define _AUDIO_MIXER_WITH_PROCESS_H_

#include <WXMediaCpp.h>
#include "SoundTouch/SoundTouch.h"

#define USE_GIPS 0
#if USE_GIPS
#include "./gips/audio_processing.h"
#include "./gips/module_common_types.h"
#else
#include "librnnoise/librnnoise.h"
#endif


class AudioMixerWithProcess {

	WXFifo m_outFifo;//输出缓存
	WXDataBuffer m_bufBase;
	WXDataBuffer m_bufSystem;
	WXDataBuffer m_bufMic;

	//系统声音变调器
	int m_nSystemScale = 100;
	soundtouch::SoundTouch m_systemSoundTouch;
	int  m_nSystemLevel = 100;//原始音量

	//麦克风变调器
	int m_nMicScale = 100;
	soundtouch::SoundTouch m_micSoundTouch;
	int  m_nMicLevel = 100;//原始音量

	int m_capture_level = 128;//AGC
	BOOL m_bNS  = FALSE;
	BOOL m_bAEC = FALSE;
	BOOL m_bAGC = FALSE;
	BOOL m_bVAD = FALSE;

#if USE_GIPS
	webrtc::AudioProcessing* m_apm = nullptr;
	webrtc::AudioFrame m_CaptureFrame;
	webrtc::AudioFrame m_RenderFrame;
	int ApmProcessRender(uint8_t* RenderBuf);
	int ApmProcessCapture(uint8_t* CaptureBuf);
#else
	DenoiseState* m_st[2] = { nullptr,nullptr };//RNNoise
#endif



	int m_nCount = 0;//
	WXFifo* m_systemFifo = nullptr;
	WXFifo* m_micFifo = nullptr;
public:
	AudioMixerWithProcess(WXCTSTR wszName) {
		m_bufSystem.Init(nullptr, AUDIO_FRAME_SIZE);
		m_bufMic.Init(nullptr, AUDIO_FRAME_SIZE);
		m_bufBase.Init(nullptr, AUDIO_FRAME_SIZE);
	}

	virtual ~AudioMixerWithProcess() {
#if USE_GIPS
		SAFE_DELETE(m_apm);
#else
		if (m_st[0]) {
			rnnoise_destroy(m_st[0]);
			m_st[0] = nullptr;
		}	
		if (m_st[1]) {
			rnnoise_destroy(m_st[1]);
			m_st[1] = nullptr;
		}
#endif
	}

	void AddSystem(WXFifo* obj, int nScale, int nLevel) {
		m_nCount++;
		m_systemFifo = obj;
		m_nSystemLevel = std::max(std::min(nLevel, 150), 0);
		m_systemSoundTouch.setChannels(AUDIO_CHANNELS);//声道数
		m_systemSoundTouch.setSampleRate(AUDIO_SAMPLE_RATE);//采样频率
		m_nSystemScale = std::max(std::min(nScale, 200), 50);
		m_systemSoundTouch.setPitch(m_nSystemScale / 100.0f);//变调不变速算法
	}

	void AddMic(WXFifo* obj, int nScale, int nLevel, int bNs, int bAgc, int bVad);

	WXFifo* GetOutput() {
		return  &m_outFifo;
	}

	void Process();

	//音量控制
	void DoVolume(int bSystem, int16_t* buf, int count) {
		int nLevel = bSystem ? m_nSystemLevel : m_nMicLevel;
		for (int i = 0; i < count; i++){
			buf[i] = S16Level(buf[i], nLevel);
		}
	}

	//变调处理
	void DoScale(int bSystem, int16_t* buf, int size) {
		if (bSystem) {
			if (m_nSystemScale != 100) {
				m_systemSoundTouch.putSamples(buf, AUDIO_SAMPLE_RATE / 100);//填充到变调器
				m_systemSoundTouch.receiveSamples(buf, AUDIO_SAMPLE_RATE / 100);
			}
		}else {
			if (m_nMicScale != 100) {
				m_micSoundTouch.putSamples(buf, AUDIO_SAMPLE_RATE / 100);//填充到变调器
				m_micSoundTouch.receiveSamples(buf, AUDIO_SAMPLE_RATE / 100);
			}
		}
	}

	void ChangeLevel(int bSystem, int nLevel) {
		if (bSystem) {
			m_nSystemLevel = std::max(std::min(nLevel, 150), 0);
		}else {
			m_nMicLevel = std::max(std::min(nLevel, 150), 0);
		}
	}

	void ChangeScale(int bSystem, int nScale) {
		if (bSystem) {
			m_nSystemScale = std::max(std::min(nScale, 200), 50);
			m_systemSoundTouch.setPitch(m_nMicScale / 100.0f);//变调不变速算法
		}else {
			m_nMicScale = std::max(std::min(nScale, 200), 50);
			m_micSoundTouch.setPitch(m_nMicScale / 100.0f);//变调不变速算法
		}
	}

};

#endif

