/*
	音频测试功能
	循环播放一个音频文件
	Add By Tam
	2022.05.10
*/

#include "WXMediaCpp.h"

WXMEDIA_API int WXAudioFilePlayerStartAutoClose(WXCTSTR wszFileName) {
	return 0;
}

//static WXLocker s_lockGlobal;
WXMEDIA_API void* WXAudioFilePlayerStart(WXCTSTR wszFileName) {
	WXAutoLock al(s_lockGlobal);
	void* play = WXFfplayCreate(L"FFPLAY", wszFileName, 100, 0);
	if (play) {
		WXFfplayLoop(play, TRUE);
		WXFfplayStart(play);
		return play;
	}
	return nullptr;
}

WXMEDIA_API void WXAudioFilePlayerStop(void* ptr) {
	WXAutoLock al(s_lockGlobal);
	if (ptr) {
		WXFfplayStop(ptr);
		WXFfplayDestroy(ptr);
	}
}
