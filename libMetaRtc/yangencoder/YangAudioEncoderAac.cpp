//
// Copyright (c) 2019-2022 yanggaofeng
//
#include <yangencoder/YangAudioEncoderAac.h>


YangAudioEncoderAac::YangAudioEncoderAac() {
	nSampleRate = 44100;
	nChannels = 2;
		nInputSamples = 0;
	nMaxOutputBytes = 0;

	m_aacBuffer = NULL;
	ret = 0;
	nPCMBitSize = 0;
	nInputSamples = 0;
	nMaxOutputBytes = 0;
	isRec = 0;
	hEncoder = NULL;
	isamples = 0;
	maxsample = 0;
	mnInputSamples = 0;
	nRet = 0;

	nBytesRead = 0;
	nPCMBufferSize = 0;
	frames = 0;

}
YangAudioEncoderAac::~YangAudioEncoderAac(void) {
	closeAac();
	yang_delete(m_aacBuffer);
}



void YangAudioEncoderAac::init(YangAudioInfo *pap) {
	if(m_isInit) return;

	setAudioPara(pap);
	hEncoder = faacEncOpen(nSampleRate, nChannels, &nInputSamples,&nMaxOutputBytes);
	isamples = nInputSamples;
	maxsample = nMaxOutputBytes;
	mnInputSamples = nInputSamples * 2;

	m_aacBuffer = new uint8_t[nMaxOutputBytes];
	faacEncConfigurationPtr aconfiguration = faacEncGetCurrentConfiguration(hEncoder);

	aconfiguration->version = MPEG4;

	aconfiguration->aacObjectType = LOW;	//MAIN;//LOW;//MAIN;
	aconfiguration->allowMidside = 1;
	aconfiguration->useTns = 0;
	aconfiguration->shortctl = SHORTCTL_NORMAL;
	//aconfiguration->
	//aconfiguration->nputformat=FAAC_INPUT_16BIT;
	aconfiguration->outputFormat = pap->enableAudioHeader;

	aconfiguration->inputFormat = FAAC_INPUT_16BIT;
	aconfiguration->bitRate = 128000 / nChannels;
	nRet =faacEncSetConfiguration(hEncoder, aconfiguration);
	m_isInit=1;


}

int32_t YangAudioEncoderAac::encoder(YangFrame* pframe,YangEncoderCallback* pcallback){
	if(!hEncoder) return 1;
	ret = faacEncEncode(hEncoder, (int32_t*)pframe->payload, isamples, m_aacBuffer, maxsample);

				if (ret > 0&&pcallback){
					pframe->payload=m_aacBuffer;
					pframe->nb=ret;
					pcallback->onAudioData(pframe);
					return Yang_Ok;
				}else
					return 1;
}


void YangAudioEncoderAac::closeAac() {
	if(hEncoder) faacEncClose(hEncoder);
	hEncoder = NULL;
}

