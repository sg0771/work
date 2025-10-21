/*
基于Webrtc的音频处理
输入为 采样频率 8000Hz,16000Hz,32000Hz,48000Hz
采样声道单声道或者双声道
数据S16格式
输入数据为10ms的数据包
*/

#ifndef _APM_H_
#define _APM_H_

#ifdef GIPS_EXPORTS
#define APM_API   extern "C"
#else
#define APM_API   extern "C" 
#endif

#include <stdint.h>

//构造音频处理模块，失败返回NULL
APM_API void* ApmCreate(int sample_rate, int channel, int bAgc, int bNs, int bVad);

//输入位10ms长度的数据包
APM_API void  ApmProcess(void *ptr, void *buf);

//销毁处理模块
APM_API void  ApmDestroy(void *ptr);

#endif