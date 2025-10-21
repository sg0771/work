/*	09/2017
	shichaog
	This is the main test File. Include noise suppresion, AEC, VAD.

*/
//#define _WIN32
//#define WEBRTC_AUDIO_PROCESSING_ONLY_BUILD
//#define NOMINMAX
//#define _CRT_SECURE_NO_WARNINGS

#include <string>
#include <iostream>

#include "audio_processing.h"
#include "module_common_types.h"

#include "apm.h"

class ApmHandler {
	webrtc::AudioProcessing* m_apm = nullptr;
	int m_nSampleRate = 48000;
	int m_nChannel = 2;
	webrtc::AudioFrame m_AudioFrame;
	int m_iAudioSize = 0;
	int m_bVAd = 0;
public:
	int Open(int sample_rate, int channel, int bAgc, int bNs, int bVad) {
		if (sample_rate == 8000 || sample_rate == 1600|| sample_rate == 32000 ||
			sample_rate == 48000 || channel == 1 || channel == 2) {
			if (bAgc == 0 && bNs == 0 && bVad == 0) {
				return 0;
			}
			m_bVAd = bVad;
			float frame_step = 10;  // ms
			m_AudioFrame.sample_rate_hz_ = sample_rate;
			m_AudioFrame.samples_per_channel_ = (size_t)(m_AudioFrame.sample_rate_hz_ * frame_step / 1000.0);
			m_AudioFrame.num_channels_ = channel;

			m_apm = webrtc::AudioProcessing::Create();
			m_apm->level_estimator()->Enable(true);
			int delay_ms = 0;
			m_apm->set_stream_delay_ms(delay_ms);

			webrtc::Config config;
			config.Set<webrtc::ExtendedFilter>(new webrtc::ExtendedFilter(true));
			config.Set<webrtc::DelayAgnostic>(new webrtc::DelayAgnostic(true));
			m_apm->SetExtraOptions(config);
			m_apm->high_pass_filter()->Enable(true);

			if (bNs) { //噪声处理
				m_apm->noise_suppression()->Enable(true);
				m_apm->noise_suppression()->set_level(webrtc::NoiseSuppression::kVeryHigh);
				m_apm->voice_detection()->Enable(true);
			}
			if (bAgc) { //自动增益
				m_apm->gain_control()->Enable(true);
				m_apm->gain_control()->set_analog_level_limits(0, 255);
				m_apm->gain_control()->set_mode(webrtc::GainControl::kAdaptiveAnalog);
			}
			if (bVad) { //人声识别
				m_apm->voice_detection()->Enable(true);
				m_apm->voice_detection()->set_likelihood(webrtc::VoiceDetection::kVeryLowLikelihood);
				m_apm->voice_detection()->set_frame_size_ms(10);
			}
			m_iAudioSize =  m_AudioFrame.samples_per_channel_ * channel * sizeof(int16_t);
		}

		return m_iAudioSize;
	}

	virtual ~ApmHandler() {
		if (m_apm) {
			delete m_apm;
			m_apm = nullptr;
		}
	}

	//10ms 
	int Process(void *buf) {
		m_apm->set_stream_delay_ms(0);
		m_apm->echo_cancellation()->set_stream_drift_samples(0);
		memcpy(m_AudioFrame.data_, buf, m_iAudioSize);
		//m_apm->ProcessReverseStream(&m_AudioFrame);

		int err = m_apm->ProcessStream(&m_AudioFrame);
		int vad_flag = (int)m_apm->voice_detection()->stream_has_voice();//可以 除以2
		memcpy(buf, m_AudioFrame.data_,  m_iAudioSize);
		return vad_flag;//
	}
};


APM_API void* ApmCreate(int sample_rate, int channel, int bAgc, int bNs, int bVad) {
	ApmHandler *obj = new ApmHandler;
	int size = obj->Open(sample_rate, channel, bAgc, bNs, bVad);
	if (size > 0) {
		return (void*)obj;
	}
	delete obj;
	return nullptr;
}

APM_API void ApmProcess(void *ptr, void *buf) {
	if (ptr) {
		ApmHandler *obj = (ApmHandler*)ptr;
		obj->Process(buf);
	}
}

APM_API void ApmDestroy(void *ptr) {
	if (ptr) {
		ApmHandler *obj = (ApmHandler*)ptr;
		delete obj;
	}
}