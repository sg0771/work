//
// Copyright (c) 2019-2022 yanggaofeng
//

#ifndef YANGENCODER_INCLUDE_YANGAUDIOENCODERMETA_H_
#define YANGENCODER_INCLUDE_YANGAUDIOENCODERMETA_H_

#include "yangutil/sys/YangLoadLib.h"

class YangAudioEncoderMeta{
public:
	YangAudioEncoderMeta();
	~YangAudioEncoderMeta();
	void createMeta(uint8_t *pasc,int32_t *asclen);
};



#endif /* YANGENCODER_INCLUDE_YANGAUDIOENCODERMETA_H_ */
