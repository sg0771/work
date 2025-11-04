#include "WXMiraCastExport.h"
#include <string>
#include <xstring>
#include "MiraCastProcess.h"



MIRACAST_API int WXInitMiraCast(const WXMiraCastManagerStruct * tCallBackFunc)
{	
	MiraCastManager::Get().SetCallBackFuns(tCallBackFunc);
	return 0;
}

void WXUninitMiracast()
{

}

MIRACAST_API int WXStartMiracast()
{
	return MiraCastManager::Get().Start(MiraCastManager::Get().m_stMiraCast.m_arcAppName, MiraCastManager::Get().m_stMiraCast.m_arcLogName);
	return 0;
}

MIRACAST_API int  WXStopMiraCast()
{
	MiraCastManager::Get().Stop(0);
	return 0;
}

MIRACAST_API void WXDisconnectMiraCastMirror(const UINT64 uniqueid)
{
	MiraCastManager::Get().DisconnectMiraCastMirror(uniqueid);
}

MIRACAST_API void WXMiraCastSetLogLevel(int iLevel)
{
	
}
void WXSetMiraCastPassword(const char* pszPwd)
{

}
MIRACAST_API void WXMirCastSetMaxBufferLength(unsigned long long llUniqueid, int iBufLen)
{
	
}
MIRACAST_API void WXSetLogLevel(int iLevel)
{
	
}
int main()
{
	return 0;
}
