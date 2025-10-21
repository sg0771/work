//
// Copyright (c) 2019-2022 yanggaofeng
//
#include <yangencoder/YangAudioEncoderMeta.h>
#include <yangutil/yangtype.h>

#include "libfaac/faac.h"

YangAudioEncoderMeta::YangAudioEncoderMeta(){

}

YangAudioEncoderMeta::~YangAudioEncoderMeta(){

}


void YangAudioEncoderMeta::createMeta(uint8_t *pasc,int32_t *asclen){


	faacEncHandle hEncoder=NULL;
	unsigned long nSampleRate = 44100;
	uint32_t  nChannels = 2;

	unsigned long nInputSamples = 0;
	unsigned long nMaxOutputBytes = 0;
	hEncoder = faacEncOpen(nSampleRate, nChannels, &nInputSamples,&nMaxOutputBytes);
	faacEncConfigurationPtr aconfiguration = faacEncGetCurrentConfiguration(hEncoder);

		aconfiguration->version = MPEG4;

		aconfiguration->aacObjectType = LOW;	//MAIN;//LOW;//MAIN;
		aconfiguration->allowMidside = 1;
		aconfiguration->useTns = 0;
		aconfiguration->shortctl = SHORTCTL_NORMAL;
		//aconfiguration->
		//aconfiguration->nputformat=FAAC_INPUT_16BIT;
		aconfiguration->outputFormat = 0;

		aconfiguration->inputFormat = FAAC_INPUT_16BIT;
		aconfiguration->bitRate = 128000 / 2;

		int32_t nRet = faacEncSetConfiguration(hEncoder, aconfiguration);


			uint8_t *asc=NULL;
			unsigned long len=0;
	        faacEncGetDecoderSpecificInfo(hEncoder, &asc, &len);
            if(asc){
                memcpy(pasc,asc,len);
                *asclen=len;
                faacEncClose(hEncoder);
                hEncoder=NULL;
            }

}
