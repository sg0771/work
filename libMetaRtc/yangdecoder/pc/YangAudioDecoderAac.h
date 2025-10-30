//
// Copyright (c) 2019-2022 yanggaofeng
//
#ifndef YangAudioDecoderAac_H
#define YangAudioDecoderAac_H

#include <yangutil/buffer/YangAudioEncoderBuffer.h>
#include <yangdecoder/YangAudioDecoder.h>
#include <yangutil/sys/YangLoadLib.h>
#include <yangutil/yangavinfotype.h>
#include <libfaad/faad.h>
#include <vector>

//using namespace std;
class YangAudioDecoderAac :public YangAudioDecoder
{
public:
	YangAudioDecoderAac(YangAudioParam  *pcontext);
	virtual ~YangAudioDecoderAac();
	void init();
	int32_t decode(YangFrame* pframe,YangDecoderCallback* pcallback);


protected:

private:

	void closeAacdec();
	int32_t isConvert;
	uint8_t *temp;
	NeAACDecHandle m_handle;

	int32_t isFirst;
	NeAACDecFrameInfo m_info;
	unsigned long m_samplerate;
	uint8_t m_channel;
	unsigned long m_bufLen;
	uint8_t *m_buffer;
};

#endif // AACDECODER_H
