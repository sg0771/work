/*
音频混音器
by TamXie
*/

#include  "AudioMixerWithProcess.h"

void AudioMixerWithProcess::AddMic(WXFifo* obj, int nScale, int nLevel, int bNs, int bAgc, int bVad) {
	m_nCount++;
	m_micFifo = obj;

	m_nMicLevel = std::max(std::min(nLevel, 150), 0);
	m_nMicScale = std::max(std::min(nScale, 200), 50);

	m_micSoundTouch.setChannels(AUDIO_CHANNELS);//声道数
	m_micSoundTouch.setSampleRate(AUDIO_SAMPLE_RATE);//采样频率
	m_micSoundTouch.setPitch(m_nMicScale / 100.0f);//变调不变速算法
	
	m_bAEC = WXGetGlobalValue(L"AEC");

	if (m_systemFifo == nullptr) {//没有扬声器输入，不需要AEC处理
		m_bAEC = 0;
	}

	m_bNS  = bNs;
	m_bAGC = bAgc;
	m_bVAD = bVad;
	
#if USE_GIPS
	if (m_bVAD || m_bAGC || m_bNS || m_bAEC) {
		float frame_step = 10;  // ms
		m_CaptureFrame.sample_rate_hz_ = AUDIO_SAMPLE_RATE;
		m_CaptureFrame.samples_per_channel_ = AUDIO_SAMPLE_RATE / 100;
		m_CaptureFrame.num_channels_ = AUDIO_CHANNELS;

		m_RenderFrame.sample_rate_hz_ = AUDIO_SAMPLE_RATE;
		m_RenderFrame.samples_per_channel_ = AUDIO_SAMPLE_RATE / 100;
		m_RenderFrame.num_channels_ = AUDIO_CHANNELS;

		m_apm = webrtc::AudioProcessing::Create();

		m_apm->level_estimator()->Enable(true);

		webrtc::Config config;
		config.Set<webrtc::ExtendedFilter>(new webrtc::ExtendedFilter(true));
		config.Set<webrtc::DelayAgnostic>(new webrtc::DelayAgnostic(true));
		m_apm->SetExtraOptions(config);

		m_apm->high_pass_filter()->Enable(true);

		if (m_bAEC) { //噪声处理
			m_apm->echo_cancellation()->Enable(true);
			m_apm->echo_cancellation()->enable_metrics(true);
			m_apm->echo_cancellation()->set_suppression_level(webrtc::EchoCancellation::kModerateSuppression);
		}

		if (m_bNS) { //噪声处理
			m_apm->noise_suppression()->Enable(true);
			m_apm->noise_suppression()->set_level(webrtc::NoiseSuppression::kVeryHigh);
		}

		if (m_bAGC) { //自动增益
			m_apm->gain_control()->Enable(true);
			m_apm->gain_control()->set_analog_level_limits(0, 65535);
			m_apm->gain_control()->set_mode(webrtc::GainControl::kAdaptiveAnalog);
		}

		if (m_bVAD) { //人声识别
			m_apm->voice_detection()->Enable(true);
			m_apm->voice_detection()->set_likelihood(webrtc::VoiceDetection::kHighLikelihood);
			m_apm->voice_detection()->set_frame_size_ms(10);
		}
		int err = m_apm->Initialize(AUDIO_SAMPLE_RATE, 
			AUDIO_SAMPLE_RATE, 
			AUDIO_SAMPLE_RATE,
			webrtc::AudioProcessing::ChannelLayout::kStereo,
			webrtc::AudioProcessing::ChannelLayout::kStereo,
			webrtc::AudioProcessing::ChannelLayout::kStereo);
		m_nMicLevel = m_nMicLevel * 115 / 100;//声音小
	}
#else
	if (m_bNS) {
		m_st[0] = rnnoise_create(NULL);
		m_st[1] = rnnoise_create(NULL);
	}
#endif
}

#if USE_GIPS
int AudioMixerWithProcess::ApmProcessRender(uint8_t* RenderBuf) {
	if (m_apm && m_bAEC && RenderBuf) { //系统播放声音
		m_apm->set_stream_delay_ms(120);//经验值
		m_apm->echo_cancellation()->set_stream_drift_samples(0);
		memcpy(m_RenderFrame.data_, RenderBuf, AUDIO_FRAME_SIZE);
		int err = m_apm->ProcessReverseStream(&m_RenderFrame);//处理AEC参考声音
		return err;
	}
	return -1;
}

int AudioMixerWithProcess::ApmProcessCapture(uint8_t* CaptureBuf) {
	if (m_apm) {
		memcpy(m_CaptureFrame.data_, CaptureBuf, AUDIO_FRAME_SIZE);
		if (m_bAGC) {
			m_apm->gain_control()->set_stream_analog_level(m_capture_level);
		}
		int err = m_apm->ProcessStream(&m_CaptureFrame);//音频处理
		if (m_bAGC) {
			m_capture_level = m_apm->gain_control()->stream_analog_level();
		}
		int vad_flag = (int)m_apm->voice_detection()->stream_has_voice();//检测是否有人声
		memcpy(CaptureBuf, m_CaptureFrame.data_, AUDIO_FRAME_SIZE);
		return vad_flag;
	}
	return -1;//
}
#endif

void AudioMixerWithProcess::Process() {
	memset(m_bufBase.GetBuffer(), 0, AUDIO_FRAME_SIZE);

	if (m_systemFifo) { //系统声音
		int nRead = m_systemFifo->Read2(m_bufSystem.GetBuffer(), AUDIO_FRAME_SIZE);
#if USE_GIPS
		ApmProcessRender(m_bufSystem.GetBuffer());//填充到AEC参考声音
#endif
	}
	if (m_micFifo) {// 麦克风
		int nRead = m_micFifo->Read2(m_bufMic.GetBuffer(), AUDIO_FRAME_SIZE);
#if USE_GIPS
		ApmProcessCapture(m_bufMic.GetBuffer());//GIPS 音频处理
#else
		if (m_st[0] && m_st[1]) {
			float temp[2][AUDIO_FRAME_SIZE / 4];
			short* pcm = (short*)m_bufMic.GetBuffer();
			for (size_t i = 0; i < AUDIO_FRAME_SIZE / 4; i++)
			{
				temp[0][i] = pcm[2 * i];
				temp[1][i] = pcm[2 * i + 1];
			}
			rnnoise_process_frame(m_st[0], temp[0], temp[0]);
			rnnoise_process_frame(m_st[1], temp[1], temp[1]);
			for (size_t i = 0; i < AUDIO_FRAME_SIZE / 4; i++)
			{
				pcm[2 * i] = temp[0][i];
				pcm[2 * i + 1] = temp[1][i];
			}
		}

#endif
	}

	if (m_systemFifo) { //叠加系统声音
		DoVolume(TRUE, (int16_t*)m_bufSystem.GetBuffer(), AUDIO_FRAME_SIZE / 2);
		DoScale(TRUE, (int16_t*)m_bufSystem.GetBuffer(), AUDIO_FRAME_SIZE / 2);
		AddAudio((int16_t*)m_bufBase.GetBuffer(), (int16_t*)m_bufSystem.GetBuffer(), AUDIO_FRAME_SIZE / 2);
	}
	if (m_micFifo) {   //叠加麦克风声音
		DoVolume(FALSE, (int16_t*)m_bufMic.GetBuffer(), AUDIO_FRAME_SIZE / 2);
		DoScale(FALSE, (int16_t*)m_bufMic.GetBuffer(), AUDIO_FRAME_SIZE / 2);
		AddAudio((int16_t*)m_bufBase.GetBuffer(), (int16_t*)m_bufMic.GetBuffer(), AUDIO_FRAME_SIZE / 2);
	}
	m_outFifo.Write(m_bufBase.GetBuffer(), AUDIO_FRAME_SIZE);//写数据
}


