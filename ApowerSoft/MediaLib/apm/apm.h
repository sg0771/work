/*
����Webrtc����Ƶ����
����Ϊ ����Ƶ�� 8000Hz,16000Hz,32000Hz,48000Hz
������������������˫����
����S16��ʽ
��������Ϊ10ms�����ݰ�
*/

#ifndef _APM_H_
#define _APM_H_

#ifdef GIPS_EXPORTS
#define APM_API   extern "C"
#else
#define APM_API   extern "C" 
#endif

#include <stdint.h>

//������Ƶ����ģ�飬ʧ�ܷ���NULL
APM_API void* ApmCreate(int sample_rate, int channel, int bAgc, int bNs, int bVad);

//����λ10ms���ȵ����ݰ�
APM_API void  ApmProcess(void *ptr, void *buf);

//���ٴ���ģ��
APM_API void  ApmDestroy(void *ptr);

#endif