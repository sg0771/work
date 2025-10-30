//
// Copyright (c) 2019-2022 yanggaofeng
//
#ifndef __YangAudioEncoderAac__
#define __YangAudioEncoderAac__
#include <yangutil/buffer/YangAudioEncoderBuffer.h>
#include <yangencoder/YangAudioEncoder.h>
#include <yangutil/buffer/YangAudioBuffer.h>
#include <yangutil/sys/YangLoadLib.h>
#include "libfaac/faac.h"

class YangAudioEncoderAac: public YangAudioEncoder
{
public:
	YangAudioEncoderAac();
	~YangAudioEncoderAac(void);
	void init(YangAudioInfo *pap);
	int32_t encoder(YangFrame* pframe,YangEncoderCallback* pcallback);

private:
	uint8_t *m_aacBuffer;

	int32_t frames;
	int32_t mnInputSamples;
	uint32_t  isamples, maxsample;
	unsigned long nSampleRate;
	uint32_t  nChannels;
	uint32_t  nPCMBitSize;
	unsigned long nInputSamples;
	unsigned long nMaxOutputBytes;

	int32_t nRet;
	int32_t nBytesRead;
	int32_t nPCMBufferSize;

private:
	YangLoadLib m_lib;
	void encoder(int32_t *p_buf);
	void closeAac();
	void saveWave();
	int32_t ret;
	int32_t isRec;
	faacEncHandle hEncoder = nullptr;
};

#endif
