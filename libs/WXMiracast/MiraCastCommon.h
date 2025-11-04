#ifndef __MIRACASTCOMMON_H
#define __MIRACASTCOMMON_H
#include "WXMiraCastExport.h"
enum RETCODE_AIRPLAY
{
	ErrorCode_Unknow = -1,
	ErrorCode_NoError = 0,
	ErrorCode_AppleServiceNotRunning,
	ErrorCode_StartAirtunesServerFail,
	ErrorCode_StartAirplayServerFail,
	ErrorCode_StartMirrorServerFail,
	ErrorCode_StartChromecastServerFail,
	ErrorCode_UpdateCertificateFail,
	ErrorCode_StartMiracastServerFail,
};


#endif // !__MIRACASTCOMMON_H

