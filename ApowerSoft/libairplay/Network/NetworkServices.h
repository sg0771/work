#pragma once
/*
 *      Copyright (C) 2013 Team XBMC
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
#include <string>
#include "../utils/utils.h"


#include "libSetting.h"

#include "../WXAirPlayAPI.h"
#include <map>

#ifdef HAS_WEB_SERVER
class CWebServer;
class CHTTPImageHandler;
class CHTTPImageTransformationHandler;
class CHTTPVfsHandler;

#ifdef HAS_WEB_INTERFACE
#ifdef HAS_PYTHON
class CHTTPPythonHandler;
#endif
class CHTTPWebinterfaceHandler;
class CHTTPWebinterfaceAddonsHandler;
#endif // HAS_WEB_INTERFACE
#endif // HAS_WEB_SERVER

class CNetworkServices : public ISettingCallback
{
public:
  static CNetworkServices& Get();
  
  virtual bool OnSettingChanging(const CSetting *setting);
  virtual void OnSettingChanged(const CSetting *setting);
  virtual bool OnSettingUpdate(CSetting* &setting, const char *oldSettingId);

  int Start(const std::wstring &strAppName, const std::wstring &strLogName, const std::wstring &strPcVersion,int port = -1);
  void Stop(bool bWait);

  bool StartAirPlayServer(int port = -1);
  bool IsAirPlayServerRunning();
  bool StopAirPlayServer(bool bWait);
  bool StartAirTunesServer(int port = -1);
  bool IsAirTunesServerRunning();
  bool StopAirTunesServer(bool bWait);

  void DisconnectAirplay(uint64_t uniqueid);
  void DisconnectAirplayMirror(uint64_t uniqueid);
  void SetWindowsParentHandle(HWND iHandle);
  void DisconnectMirrorNew(uint64_t uniqueid);
  void SetCallBackFuns(const AirplayManagerStruct &stAirplay);
  void SetRecordStatus(int iStatus, uint64_t uniqueid);
  int GetRecordStatus(uint64_t uniqueid);
  void SetUID(std::string& uid);

  std::string& GetUID();

  static void GetRandChars(unsigned char *arcBuffer, int iLen)
  {
	  //srand(time(NULL));
	  for (short i = 0; i < iLen; i++)
	  {
		  arcBuffer[i] = rand();
	  }
  }

  static char* ProcessAirplayVideo(int fd, int counter, unsigned char *remote, char *method, char *url, char *data, int datalen, int neednew, char* sessionid, char* contenttype,
	  char* auth, char* photoAction, char* photoCacheId);
  static void freeResBuffer(char* buf);
 
  static void Set_Ports(int dataPort, int mirrorPort)
  {
	  m_iDataPort = dataPort;
	  m_iMirrorPort = mirrorPort;
  }

  static void Get_Ports(int *iDataPort, int *iMirror_Port)
  {
	  *iDataPort = m_iDataPort;
	  *iMirror_Port = m_iMirrorPort;
  }

  static void Set_ShareKey(unsigned char* buffer, int iLen)
  {
	  m_strShareKey.assign((char*)buffer, iLen);
  }

  static int Get_DisplaySet()
  {
	  return CNetworkServices::Get().m_stAirplay.m_CallBackWindowPixel();
  }

  static void Get_model(const unsigned char* remoteip, const char* arcModel)
  {
	  uint64_t uniqueid = 0;
	  memcpy(&uniqueid, remoteip, 4);
	  m_mapModel[uniqueid] = arcModel;
  }

  static void Get_name(const unsigned char* remoteip, const char* arcName)
  {
	  uint64_t uniqueid = 0;
	  memcpy(&uniqueid, remoteip, 4);
	  m_mapName[uniqueid] = arcName;
  }

  static void Get_osVersion(const unsigned char* remoteip, const char* osVersion)
  {
	  uint64_t uniqueid = 0;
	  memcpy(&uniqueid, remoteip, 4);
	  m_mapOsVersion[uniqueid] = osVersion;
  }

  static void Get_Audio(unsigned char *buffer, int *iLen, uint64_t *fpstime, uint64_t uniqueid);
  static void Remove_Audio(uint64_t uniqueid);
  
private:
	CNetworkServices() :m_bLog(0)/*, m_iRecordStatus(0)*/
	{
		memset(&m_stAirplay, 0, sizeof(AirplayManagerStruct));
	}
  CNetworkServices(const CNetworkServices&);
  CNetworkServices const& operator=(CNetworkServices const&);
  virtual ~CNetworkServices(){}

  bool ValidatePort(int port);
  

  //Logger m_pTestLogger;
  int m_bLog;

public:
	static std::string m_strVMNetMac;
	static char m_VMNetRawMac[6];
	static std::string m_strVMNetIP;
	std::string m_strAppName;
	std::string m_strPcVersion;
	AirplayManagerStruct m_stAirplay;
    HWND m_iParentHandle;
	std::string m_uid; //用于标志服务，替代mac地址

    std::map<unsigned long long, int> m_mapRecordStatus;

    static std::string m_strShareKey;
    static unsigned short m_iDataPort;
    static unsigned short m_iMirrorPort;

    static std::map<unsigned long long, std::string> m_mapModel;
    static std::map<unsigned long long, std::string> m_mapName;
    static std::map<unsigned long long, std::string> m_mapOsVersion;
    static struct dnssd_s* m_dnssd;
};
