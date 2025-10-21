//
// Copyright (c) 2019-2022 yanggaofeng
//
#include <yangdecoder/pc/YangAudioDecoderAac.h>
#include <yangutil/yangavinfotype.h>
#include <yangutil/sys/YangLog.h>

YangAudioDecoderAac::YangAudioDecoderAac(YangAudioParam *pcontext) {
	m_context=pcontext;
	m_samplerate = 44100;
	m_channel = 2;
	m_bufLen = 4096;
	isFirst = 1;
	isConvert = 0;
	m_handle = NULL;
	m_buffer = new uint8_t[4096];
	temp = NULL;
}

YangAudioDecoderAac::~YangAudioDecoderAac() {

	closeAacdec();
	temp = NULL;
	m_handle = NULL;
	yang_deleteA(m_buffer);
}

void YangAudioDecoderAac::init() {

	if(m_isInit) return;

	if (m_handle == NULL)
		m_handle = NeAACDecOpen();
	if (!m_handle) {
		yang_error("NeAACDecOpen failed");

	}
	NeAACDecConfigurationPtr conf = NeAACDecGetCurrentConfiguration(m_handle);
	if (!conf) {
		printf("NeAACDecGetCurrentConfiguration failed\n");
		// goto error;
	}
	conf->defObjectType = LC;
	conf->defSampleRate = 44100;

	conf->outputFormat = FAAD_FMT_16BIT;
	conf->dontUpSampleImplicitSBR = 1;

	NeAACDecSetConfiguration(m_handle, conf);
	m_alen=4096;
	m_isInit=1;
}


int32_t YangAudioDecoderAac::decode(YangFrame* pframe,YangDecoderCallback* pcallback){
	if (isFirst&&m_handle) {
		long res = NeAACDecInit(m_handle, m_buffer, m_bufLen, &m_samplerate,&m_channel);
		isFirst = 0;
	}

	if(m_handle) temp = (uint8_t *) NeAACDecDecode(m_handle, &m_info, pframe->payload, pframe->nb);

	if (temp&&pcallback){
		pframe->payload=temp;
		pframe->nb=4096;
		pcallback->onAudioData(pframe);
	}
	return Yang_Ok;

}

void YangAudioDecoderAac::closeAacdec() {
	if (m_handle) {
		NeAACDecClose(m_handle);
	}
	m_handle = NULL;
}

