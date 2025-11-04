#include "../WXAirPlayAPI.h"
#include "NetworkServices.h"
#include "mycommon.h"

#include <WXBase.h>

extern BOOL WStringToString(const std::wstring& wstr, std::string& str);

AIRPLAY_API int StartAirplay(const AirplayManagerStruct &stAirplay)
{
	int port = 46666;
	if (stAirplay.m_iPort != -1 && stAirplay.m_iPort != 0) {
		port = stAirplay.m_iPort;
	}
	CNetworkServices::Get().SetCallBackFuns(stAirplay);
	CNetworkServices::Get().Set_Ports(stAirplay.m_iDataPort, stAirplay.m_iMirrorPort);

	
    std::string uid;
	WStringToString(stAirplay.m_uid, uid);
	CNetworkServices::Get().SetUID(uid);
	return CNetworkServices::Get().Start(stAirplay.m_arcAppName, stAirplay.m_arcLogName, stAirplay.m_arcPcVersion, port);
}

AIRPLAY_API void StopAirplay()
{
	CNetworkServices::Get().Stop(false);
	CNetworkServices::Get().Stop(true);
}

AIRPLAY_API void DisconnectAirplay(uint64_t uniqueid)
{
	CNetworkServices::Get().DisconnectAirplay(uniqueid);
}

AIRPLAY_API void DisconnectAirplayMirror(uint64_t uniqueid)
{
	CNetworkServices::Get().DisconnectAirplayMirror(uniqueid);
}

AIRPLAY_API void SetWindowsParentHandle(HWND iHandle)
{
	CNetworkServices::Get().SetWindowsParentHandle(iHandle);
}

AIRPLAY_API void LockSDL()
{
	LockSDLCommon();
}

AIRPLAY_API void UnLockSDL()
{
	UnlockSDLCommon();
}

AIRPLAY_API void SetRecording(int iStatus, uint64_t uniqueid)
{
	CNetworkServices::Get().SetRecordStatus(iStatus, uniqueid);
}

AIRPLAY_API bool GetCurrentWindowsPicture(const char *arcPath)
{
	return GetMirrorPicture(arcPath);
}
