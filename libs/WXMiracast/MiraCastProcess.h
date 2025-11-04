#ifndef __MIRASAST_H
#define __MIRASAST_H
#include <string>
#include "pipe.h"
#include "WXMiraCastExport.h"
class MiraCastManager
{
public:
	MiraCastManager();
	~MiraCastManager();
	static MiraCastManager& Get();
	WXMiraCastManagerStruct m_stMiraCast;
	int Start(const std::wstring &strAppName, const std::wstring &strLogName);
	void Stop(bool bWait);
	void DisconnectMiraCastMirror(const unsigned long long uniqueid);
	void SetCallBackFuns(const WXMiraCastManagerStruct *stMiraCast);
private:
	ipc_pipe_server_t mSingle;
	ipc_pipe_client_t pipe;
	bool bPipeInit = false;
};


class HandleStreamMiraCast
{
public:
	HandleStreamMiraCast();
	~HandleStreamMiraCast();
	int m_iWidth = 0;
	int m_iHeight = 0;
private:

};

#endif // !__MIRASAST_H

