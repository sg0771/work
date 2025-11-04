/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#include "network.h"
#include "NetworkServices.h"
#ifdef HAS_AIRPLAY
#include "AirPlayServer.h"
#endif // HAS_AIRPLAY

#ifdef HAS_AIRTUNES
#include "AirTunesServer.h"
#endif // HAS_AIRTUNES



#include "../utils/DllLibCurl.h"
#include <list>

////using namespace std;

#ifdef HAS_UPNP
using namespace UPNP;
#endif // HAS_UPNP

#include "airplay.h"

#include "../utils/CurlFile.h"
#include "../utils/URL.h"
#include <SDL2/SDL.h>

#include <IPHlpApi.h>

#include "dnssd.h"

#include "WXMedia.h"

CNetwork    *g_network;
DllLibCurlGlobal g_curlInterface;
airplay_t *g_airplay = NULL;
static WXLocker s_Record;
dnssd_s * CNetworkServices::m_dnssd = NULL;

CNetworkServices& CNetworkServices::Get()
{
  static CNetworkServices sNetworkServices;
  return sNetworkServices;
}

std::map<uint64_t, std::string> CNetworkServices::m_mapModel = std::map<uint64_t, std::string>();// = "iPhone";
std::map<uint64_t, std::string> CNetworkServices::m_mapName = std::map<uint64_t, std::string>();
std::map<uint64_t, std::string> CNetworkServices::m_mapOsVersion = std::map<uint64_t, std::string>();
std::string CNetworkServices::m_strShareKey = "";
std::string CNetworkServices::m_strVMNetMac = ""; 
std::string CNetworkServices::m_strVMNetIP = "";
char CNetworkServices::m_VMNetRawMac[6] = {0};
unsigned short CNetworkServices::m_iDataPort = 6001;
unsigned short CNetworkServices::m_iMirrorPort = 6000;

void CNetworkServices::SetCallBackFuns(const AirplayManagerStruct &stAirplay)
{
	if (stAirplay.m_CallBackWindowStatus != NULL)
	{
		m_stAirplay.m_CallBackWindowStatus = stAirplay.m_CallBackWindowStatus;
	}
	//
	if (stAirplay.m_CallBackBmpData != NULL)
	{
		m_stAirplay.m_CallBackBmpData = stAirplay.m_CallBackBmpData;
	}
	//
	if (stAirplay.m_CallBackConnectInfo != NULL)
	{
		m_stAirplay.m_CallBackConnectInfo = stAirplay.m_CallBackConnectInfo;
	}
	//
	if (stAirplay.m_CallBackWindowPixel != NULL)
	{
		m_stAirplay.m_CallBackWindowPixel = stAirplay.m_CallBackWindowPixel;
	}
	if (stAirplay.m_CallBackFrameData != NULL)
	{
		m_stAirplay.m_CallBackFrameData = stAirplay.m_CallBackFrameData;
	}
	//
	if (stAirplay.m_CallBackNeedConvertVideoData != NULL)
	{
		m_stAirplay.m_CallBackNeedConvertVideoData = stAirplay.m_CallBackNeedConvertVideoData;
	}
	//
	if (stAirplay.m_CallBackGetParentWindow != NULL)
	{
		m_stAirplay.m_CallBackGetParentWindow = stAirplay.m_CallBackGetParentWindow;
	}
	//
	if (stAirplay.m_CallBackGetMaxDevNum != NULL)
	{
		m_stAirplay.m_CallBackGetMaxDevNum = stAirplay.m_CallBackGetMaxDevNum;
	}
	//
	if (stAirplay.m_CallBackCheckWindowExist != NULL)
	{
		m_stAirplay.m_CallBackCheckWindowExist = stAirplay.m_CallBackCheckWindowExist;
	}
}

bool CNetworkServices::OnSettingChanging(const CSetting *setting)
{
  if (setting == NULL)
    return false;

  const std::string &settingId = setting->GetId();
#ifdef HAS_WEB_SERVER
  if (settingId == "services.webserver" ||
      settingId == "services.webserverport")
  {
    //if (IsWebserverRunning() && !StopWebserver())
    //  return false;

    //if (CSettings::Get().GetBool("services.webserver"))
    //{
    //  if (!StartWebserver())
    //  {
    //    //CGUIDialogOK::ShowAndGetInput(33101, 33100);
    //    return false;
    //  }
    //}
  }
  else if (settingId == "services.esport" ||
           settingId == "services.webserverport")
    return ValidatePort(((CSettingInt*)setting)->GetValue());
  else
#endif // HAS_WEB_SERVER


#ifdef HAS_AIRPLAY
  if (settingId == "services.airplay")
  {
    if (((CSettingBool*)setting)->GetValue())
    {
      // note - airtunesserver has to start before airplay server (ios7 client detection bug)
#ifdef HAS_AIRTUNES
      if (!StartAirTunesServer())
      {
        //CGUIDialogOK::ShowAndGetInput(1274, 33100);
        return false;
      }
#endif //HAS_AIRTUNES
      
      if (!StartAirPlayServer())
      {
        //CGUIDialogOK::ShowAndGetInput(1273, 33100);
        return false;
      }      
    }
    else
    {
      bool ret = true;
#ifdef HAS_AIRTUNES
      if (!StopAirTunesServer(true))
        ret = false;
#endif //HAS_AIRTUNES
      
      if (!StopAirPlayServer(true))
        ret = false;

      if (!ret)
        return false;
    }
  }
  else if (settingId == "services.airplaypassword" ||
           settingId == "services.useairplaypassword")
  {
    //if (!CSettings::Get().GetBool("services.airplay"))
    //  return false;

    //if (!CAirPlayServer::SetCredentials(CSettings::Get().GetBool("services.useairplaypassword"),
    //                                    CSettings::Get().GetString("services.airplaypassword")))
      //return false;
  }
  else
#endif //HAS_AIRPLAY

#ifdef HAS_UPNP
  if (settingId == "services.upnpserver")
  {
    if (((CSettingBool*)setting)->GetValue())
    {
      if (!StartUPnPServer())
        return false;

      // always stop and restart the client and controller if necessary
      StopUPnPClient();
      StopUPnPController();
      StartUPnPClient();
      StartUPnPController();
    }
    else
      return StopUPnPServer();
  }
  else if (settingId == "services.upnprenderer")
  {
    if (((CSettingBool*)setting)->GetValue())
      return StartUPnPRenderer();
    else
      return StopUPnPRenderer();
  }
  else if (settingId == "services.upnpcontroller")
  {
    // always stop and restart
    StopUPnPController();
    if (((CSettingBool*)setting)->GetValue())
      return StartUPnPController();
  }
  else
#endif // HAS_UPNP

  if (settingId == "services.esenabled")
  {
#ifdef HAS_EVENT_SERVER
    if (((CSettingBool*)setting)->GetValue())
    {
      if (!StartEventServer())
      {
        //CGUIDialogOK::ShowAndGetInput(33102, 33100);
        return false;
      }
    }
    else
      return StopEventServer(true, true);
#endif // HAS_EVENT_SERVER


  }
  else if (settingId == "services.esport")
  {
#ifdef HAS_EVENT_SERVER
    // restart eventserver without asking user
    if (!StopEventServer(true, false))
      return false;

    if (!StartEventServer())
    {
      //CGUIDialogOK::ShowAndGetInput(33102, 33100);
      return false;
    }

#if defined(TARGET_DARWIN_OSX)
    // reconfigure XBMCHelper for port changes
    XBMCHelper::GetInstance().Configure();
#endif // TARGET_DARWIN_OSX
#endif // HAS_EVENT_SERVER
  }
  else if (settingId == "services.esallinterfaces")
  {
#ifdef HAS_EVENT_SERVER
    //if (CSettings::Get().GetBool("services.esenabled"))
    //{
    //  if (!StopEventServer(true, true))
    //    return false;

    //  if (!StartEventServer())
    //  {
    //    //CGUIDialogOK::ShowAndGetInput(33102, 33100);
    //    return false;
    //  }
    //}
#endif // HAS_EVENT_SERVER

  }

#ifdef HAS_EVENT_SERVER
  else if (settingId == "services.esinitialdelay" ||
           settingId == "services.escontinuousdelay")
  {
    //if (CSettings::Get().GetBool("services.esenabled"))
    //  return RefreshEventServer();
  }
#endif // HAS_EVENT_SERVER

  return true;
}

void CNetworkServices::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL)
    return;

//  const std::string &settingId = setting->GetId();
//#ifdef HAS_WEB_SERVER
//  if (settingId == "services.webserverusername" ||
//      settingId == "services.webserverpassword")
//  {
//    m_webserver.SetCredentials(CSettings::Get().GetString("services.webserverusername"),
//                               CSettings::Get().GetString("services.webserverpassword"));
//  }
//  else
//#endif // HAS_WEB_SERVER
//  if (settingId == "smb.winsserver" ||
//      settingId == "smb.workgroup")
//  {
//    // okey we really don't need to restart, only deinit samba, but that could be damn hard if something is playing
//    // TODO - General way of handling setting changes that require restart
//    if (CGUIDialogYesNo::ShowAndGetInput(14038, 14039))
//    {
//      CSettings::Get().Save();
//      CApplicationMessenger::Get().RestartApp();
//    }
//  }
}

bool CNetworkServices::OnSettingUpdate(CSetting* &setting, const char *oldSettingId)
{
  if (setting == NULL)
    return false;

  const std::string &settingId = setting->GetId();
  if (settingId == "services.webserverusername")
  {
    // if webserverusername is xbmc and pw is not empty we treat it as altered
    // and don't change the username to kodi - part of rebrand
    //if (CSettings::Get().GetString("services.webserverusername") == "xbmc" &&
    //    !CSettings::Get().GetString("services.webserverpassword").empty())
    //  return true;
  }
  return false;
}


BOOL WStringToString(const std::wstring &wstr, std::string &str)
{
	int dwMinSize = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)wstr.c_str(), -1, NULL, 0, NULL, FALSE);
	//char *arcRealPath = (char*)malloc(dwMinSize);
	str.resize(dwMinSize, '\0');

	int nResult = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)wstr.c_str(), -1, (LPSTR)str.c_str(), dwMinSize, NULL, NULL);

	if (nResult == 0)
	{
		return FALSE;
	}
	return TRUE;
}
//
BOOL StringToWString(const std::string &str, std::wstring &wstr)
{
	int nLen = (int)str.length();
	wstr.resize(nLen, L' ');

	int nResult = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)str.c_str(), nLen, (LPWSTR)wstr.c_str(), nLen);

	if (nResult == 0)
	{
		return FALSE;
	}

	return TRUE;
}
//
void shairplay_log2(void *cls, int level, const char *msg)
{
}

int CNetworkServices::Start(const std::wstring &strAppName, const std::wstring &strLogName, const std::wstring &strPcVersion,int port)
{
	std::wstring ss = strLogName;
	ss += L".WXMedia.log";

	WXDeviceInitMirror(ss.c_str());
	
	AirplayMDNSInit();
	

	WStringToString(strAppName, m_strAppName);

	WXLogWriteNew(m_strAppName.c_str());
	WStringToString(strPcVersion, m_strPcVersion);

	g_network = new CNetworkWin32();
	g_curlInterface.Load();
	g_curlInterface.Unload();

  // note - airtunesserver has to start before airplay server (ios7 client detection bug)
  if (!StartAirTunesServer(port))
  {
	  return ErrorCode_StartAirtunesServerFail;
  }
  //
  if (!StartAirPlayServer(port))
  {
	  return ErrorCode_StartAirplayServerFail;
  }

  //start 7100 for airplay mirroring
  airplay_callbacks_t airplay_cbs;
  //unsigned short dataport = 46001;
  //unsigned short mirror_port = 46000;
  //if (m_stAirplay.m_CallBackGetMaxDevNum != NULL)
  //{
	 // airplay = airplay_init_from_keyfile(m_stAirplay.m_CallBackGetMaxDevNum(), &airplay_cbs, "airport.key", NULL);
  //}
  //else
  {
	  g_airplay = airplay_init_from_key(10, &airplay_cbs);
  }


  if (airplay_start(g_airplay, &CNetworkServices::m_iDataPort, &CNetworkServices::m_iMirrorPort, "", 0, "") != 1)
  {
	  WXLogWriteNew("airplay_start fail");
	  return ErrorCode_StartMirrorServerFail;
  }
  //CNetworkServices::m_iDataPort = dataport;
  //CNetworkServices::m_iMirrorPort = mirror_port;
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER);
  SDL_EventState(SDL_SYSWMEVENT, SDL_IGNORE);
  SDL_EventState(SDL_USEREVENT, SDL_IGNORE);

  return ErrorCode_NoError;
}

void CNetworkServices::Stop(bool bWait)
{
  if (bWait)
  {
	airplay_stop(g_airplay);
	return;
  }
  WXLogWriteNew("CNetworkServices::Stop begin");
 
  StopAirPlayServer(bWait);
  StopAirTunesServer(bWait);

#ifdef _WIN32
 // WSACleanup();
#endif
  if (CAirPlayServer::IsMirroring() != 0)
  {
	  mirror_exit();
	  while (CAirPlayServer::IsMirroring() == 1)
	  {
		  ::Sleep(1);
	  }
	  SDL_Quit();
  }
  WXLogWriteNew("CNetworkServices::Stop success");
}


char *MyGetHostName(void)
{
	char *lpName = NULL;
	DWORD dwLen = 0;

	GetComputerName(NULL, &dwLen);
	lpName = (char *)malloc(dwLen);
	memset(lpName, 0, dwLen);
	GetComputerNameA(lpName, &dwLen);
	return (lpName ? lpName : NULL);
}

char *MyGetHostIp(void)
{
	char *ip = NULL;
	WSADATA wsaData = { 0 };
	struct hostent *phostinfo = NULL;

	ip = (char *)malloc(sizeof(char) * 16);
	memset(ip, 0, 16);
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	phostinfo = gethostbyname("");
	lstrcpynA(ip, inet_ntoa(*(struct in_addr *)(*phostinfo->h_addr_list)), 16);
	WSACleanup();
	if (ip)
	{
		CNetworkServices::m_strVMNetIP = ip;
		return ip;
	}
	else {
		free(ip);
		return NULL;
	}
}

std::string MyGetHostMac(void)
{
	IPAddr ip = inet_addr(MyGetHostIp());
	PVOID *pMac = NULL;
	ULONG MacAddr[2] = { 0 };    // Mac地址长度6字节
	ULONG uMacSize = 6;

	// 通过ARP报文响应获取MAC地址
	DWORD dwRet = SendARP(ip, 0, &MacAddr, &uMacSize);
	if (dwRet == NO_ERROR)
	{
		BYTE *bPhyAddr = (BYTE *)MacAddr;
		memcpy(CNetworkServices::m_VMNetRawMac, bPhyAddr, 6);
		
		if (uMacSize)
		{
			char *sMac = (char *)malloc(sizeof(char) * 18);
			int n = 0;

			memset(sMac, 0, 18);
			sprintf_s(sMac, (size_t)18, "%.2X:%.2X:%.2X:%.2X:%.2X:%.2X", (int)bPhyAddr[0], (int)bPhyAddr[1], (int)bPhyAddr[2], (int)bPhyAddr[3], (int)bPhyAddr[4], (int)bPhyAddr[5]);
			std::string retMac = sMac;
			free(sMac);
			return retMac;
		}
		else
		{
			printf("Mac地址获取失败！\n");
		}
	}
	else
	{
		printf("ARP报文发送失败:%d\n", dwRet);
	}
	return "";
}
bool CNetworkServices::StartAirPlayServer(int port)
{
	int iPort = 36666;
	if (port != -1) {
		iPort = port;
	}

	WXLogWriteNew("start airplay server");
#ifdef HAS_AIRPLAY
	//if (!g_network->IsAvailable())
	//{
	//	WriteErrorLogNew("start airplay server");
	//	return false;
	//}

	if (IsAirPlayServerRunning())
	{
		WXLogWriteNew("airplay server already start");
		return true;
	}
  
	if (!CAirPlayServer::StartServer(&iPort, true))
	{
		WXLogWriteNew("start airplay server fail");
		return false;
	}
  
 /* if (!CAirPlayServer::SetCredentials(false,
	  ""))
	  return false;*/


#ifdef HAS_ZEROCONF
  std::vector<std::pair<std::string, std::string> > txt;
  CNetworkInterface* iface = g_network->GetFirstConnectedInterface();
  std::string address = "";
  if (iface)
  {
	  address = iface->GetMacAddress();
	  //address = iface->GenerateMacAddress();
  }
  else
  {
	  if (CNetworkServices::m_strVMNetMac != "")
	  {
		  address = CNetworkServices::m_strVMNetMac;
	  }
  }
  txt.push_back(std::make_pair("deviceid", address != "" ? address : "FF:FF:FF:FF:FF:F2"));


  txt.push_back(std::make_pair("model", "AppleTV3,2"));//Xbmc,1 AppleTV3,2
  txt.push_back(std::make_pair("srcvers", "220.68"));//101.28 200.54

  txt.push_back(std::make_pair("rmodel", "PC1.0"));

  txt.push_back(std::make_pair("rrv", "1.01"));
  txt.push_back(std::make_pair("rsv", "1.00"));

  //ios 9
  txt.push_back(std::make_pair("vv", "2"));
  txt.push_back(std::make_pair("flags", "0x4"));
  txt.push_back(std::make_pair("pk", "f3769a660475d27b4f6040381d784645e13e21c53e6d2da6a8c3d757086fc336"));
  std::string ip = "";
  if (iface != NULL)
  {
	  ip = iface->GetCurrentIPAddress();
  }
  else if(CNetworkServices::m_strVMNetIP != "")
  {
	  ip = CNetworkServices::m_strVMNetIP;
  }
	txt.push_back(std::make_pair("vncip", ip.c_str()));
	WXLogWriteNew("vncip:");
	WXLogWriteNew(ip.c_str());
	txt.push_back(std::make_pair("vncport", "5900"));
	txt.push_back(std::make_pair("vncpwd", "1234"));
	txt.push_back(std::make_pair("airplayname", m_strAppName.c_str()));
	txt.push_back(std::make_pair("pcversion", m_strPcVersion.c_str()));

  if (true/*CSettings::Get().GetBool("services.airplayios8compat")*/)
  {
    // for ios8 clients we need to announce mirroring support
    // else we won't get video urls anymore.
    // We also announce photo caching support (as it seems faster and
    // we have implemented it anyways). 
    //txt.push_back(std::make_pair("features", "0x20F7"));
	  txt.push_back(std::make_pair("features", "0x5A7FFFE4,0x1E"));
	 //txt.push_back(std::make_pair("features", "0x0A7FFFF7"));
	 //txt.push_back(std::make_pair("features", "0x0A7FFFF7"));
  }
  else
  {
    txt.push_back(std::make_pair("features", "0x77"));
  }

  if (!CNetworkServices::m_dnssd)
  {
	  CNetworkServices::m_dnssd = dnssd_init(NULL);
	  if (!CNetworkServices::m_dnssd) {
		  printf("Failed to init dnssd\n");
		  return false;
	  }
  }
  dnssd_register(CNetworkServices::m_dnssd, m_strAppName.c_str(), "_airplay._tcp", iPort, txt);

#endif // HAS_ZEROCONF

  return true;
#endif // HAS_AIRPLAY
  return false;
}

bool CNetworkServices::IsAirPlayServerRunning()
{
#ifdef HAS_AIRPLAY
  return CAirPlayServer::IsRunning();
#endif // HAS_AIRPLAY
  return false;
}

bool CNetworkServices::StopAirPlayServer(bool bWait)
{
#ifdef HAS_AIRPLAY
  if (!IsAirPlayServerRunning())
    return true;

  CAirPlayServer::StopServer(bWait);

  return true;
#endif // HAS_AIRPLAY
  return false;
}

bool CNetworkServices::StartAirTunesServer(int port)
{
	WXLogWriteNew("start airtunes server");
#ifdef HAS_AIRTUNES
	if (!g_network->IsAvailable())
	{
		std::string macAddress = MyGetHostMac();//如果用的是虚拟网卡，需要获取虚拟网卡的MAC地址
		if (macAddress == "")
		{
			WXLogWriteNew("start airplay server");
			return false;
		}
		CNetworkServices::m_strVMNetMac = macAddress;
	}

	if (IsAirTunesServerRunning())
	{
		WXLogWriteNew("airtunes server already start");
		return true;
	}

	int iPort = 36666;
	if (port != -1) {
		iPort = port;
	}


  if (!CAirTunesServer::StartServer(iPort, true,
                                    false,
                                    "", m_strAppName))
  {
   // CLog::Log(LOGERROR, "Failed to start AirTunes Server");
	  WXLogWriteNew("start airtunes server fail");
    return false;
  }

  return true;
#endif // HAS_AIRTUNES
  return false;
}

bool CNetworkServices::IsAirTunesServerRunning()
{
#ifdef HAS_AIRTUNES
  return CAirTunesServer::IsRunning();
#endif // HAS_AIRTUNES
  return false;
}

bool CNetworkServices::StopAirTunesServer(bool bWait)
{
#ifdef HAS_AIRTUNES
  if (!IsAirTunesServerRunning())
    return true;

  CAirTunesServer::StopServer(bWait);
  return true;
#endif // HAS_AIRTUNES
  return false;
}

void CNetworkServices::DisconnectAirplay(uint64_t uniqueid)
{
	CAirPlayServer::DisconnectPlay(uniqueid);
}

void CNetworkServices::DisconnectAirplayMirror(uint64_t uniqueid)
{
	airplay_disconnect(g_airplay, uniqueid);
}

void CNetworkServices::DisconnectMirrorNew(uint64_t uniqueid)
{
	mirror_disconnect(g_airplay, uniqueid);
}

void CNetworkServices::SetWindowsParentHandle(HWND iHandle)
{
	m_iParentHandle = iHandle;
}

void CNetworkServices::SetRecordStatus(int iStatus, uint64_t uniqueid)
{
	WXAutoLock oLock(s_Record);
	m_mapRecordStatus[uniqueid] = iStatus;
}

int CNetworkServices::GetRecordStatus(uint64_t uniqueid)
{
	WXAutoLock oLock(s_Record);
	return m_mapRecordStatus[uniqueid];
}

void CNetworkServices::SetUID(std::string& uid)
{
	m_uid = uid;
}

std::string& CNetworkServices::GetUID()
{
	return m_uid;
}

bool CNetworkServices::ValidatePort(int port)
{
  if (port <= 0 || port > 65535)
    return false;

  return true;
}

char* CNetworkServices::ProcessAirplayVideo(int fd, int counter, unsigned char *remote, char *method, char *url, char *data, int datalen, int neednew, char* sessionid, char* contenttype,
	char* auth, char* photoAction, char* photoCacheId)
{
	if (NULL == CAirPlayServer::ServerInstance)
	{
		return (char*)"";
	}
	return CAirPlayServer::ServerInstance->ProcessAirplayVideo(fd, counter, remote, method, url, data, datalen, neednew, sessionid, contenttype, auth, photoAction, photoCacheId);
}
void CNetworkServices::freeResBuffer(char* buf)
{
	if (buf)
	{
		free(buf);
	}
}

//void CNetworkServices::Set_Audio(unsigned char *buffer, int iLen, uint64_t fpstime)
//{
//	CNetworkServices::Get().m_stAirplay.m_CallBackReportAudioData(buffer, iLen, fpstime);
//}

struct AudioData
{
	unsigned char *m_data;
	int m_iLen;
	uint64_t m_fps;
};
std::map<uint64_t, std::list<AudioData> > g_mapAudiolist;

//std::list<AudioData> g_audiolist;
static WXLocker s_Audio;

void CNetworkServices::Get_Audio(unsigned char *buffer, int *iLen, uint64_t *fpstime, uint64_t uniqueid)
{
	WXAutoLock oLock(s_Audio);
	if (!g_mapAudiolist[uniqueid].empty())
	{
		std::list<AudioData>::iterator itrList = g_mapAudiolist[uniqueid].begin();
		memcpy(buffer, itrList->m_data, itrList->m_iLen);
		*iLen = itrList->m_iLen;
		*fpstime = itrList->m_fps;
		free(itrList->m_data);
		g_mapAudiolist[uniqueid].erase(itrList);
	}
}
//
void CNetworkServices::Remove_Audio(uint64_t uniqueid)
{
	WXAutoLock oLock(s_Audio);
	while (!g_mapAudiolist[uniqueid].empty())
	{
		std::list<AudioData>::iterator itrList = g_mapAudiolist[uniqueid].begin();
		free(itrList->m_data);
		g_mapAudiolist[uniqueid].erase(itrList);
	}
}

