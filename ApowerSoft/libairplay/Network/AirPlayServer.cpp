/*
 * Many concepts and protocol specification in this code are taken
 * from Airplayer. https://github.com/PascalW/Airplayer
 *
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2.1, or (at your option)
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

//video play
#ifdef _WIN32
#include <direct.h>
#include <io.h>
#elif LINUX
#include <stdarg.h>
#include <sys/stat.h>
#endif

#include <io.h>
#include <locale>
#include <codecvt>

#ifdef _WIN32
#define ACCESS _access
#define MKDIR(a) _mkdir((a))
#elif LINUX
#define ACCESS access
#define MKDIR(a) mkdir((a),0755)
#endif

#include "Network.h"
#include "NetworkServices.h"
#include "AirPlayServer.h"
#include "AirTunesServer.h"

#ifdef HAS_AIRPLAY

#include <queue>

#include "../utils/StringUtils.h"
#include "../utils/thread.h"
#include "../utils/md5.h"
#include "../utils/Variant.h"
#include "../utils/URL.h"
#include "../utils/File.h"
#include "../utils/SpecialProtocol.h"
#include "../utils/CurlFile.h"
#include "mycommon.h"

#include <SDL2/SDL.h>
#include <SDL_Image/SDL_image.h>

#include <Shlobj.h>

#include <WXBase.h>
#include <WXMedia.h>

#include "dnssd.h"

#include <WXBase.h>
#include <WXMedia.h>
#include <WXFfplayAPI.h>

extern void WXAirplayVideoRenderDisplay(uint64_t uid, AVFrame *frame);
extern void WXAirplayVideoRenderDestroy(uint64_t uid);

#define SERVER_PORT 20992

//new ios 9
extern "C"
{
#pragma once
	//
	//  Configuration.h
	//  Workbench
	//
	//  Created by Wai Man Chan on 10/27/14.
	//
	//

#define HomeKitLog 1
#define HomeKitReplyHeaderLog 1
#define PowerOnTest 1

//Device Setting
#define deviceName "House Light"    //Name
#define deviceIdentity "12:10:34:23:51:12"  //ID
#define _manufactuerName "ET Chan"   //Manufactuer
#define devicePassword "523-12-643" //Password
#define deviceUUID "62F47751-8F26-46BF-9552-8F4238E67D60"   //UUID, for pair verify
#define controllerRecordsAddress "/var/PHK_controller" //Where to store the client keys

//Number of client
/*
 * BEWARE: Never set the number of client to 1
 * iOS HomeKit pair setup socket will not release until the pair verify stage start
 * So you will never got the pair corrected, as it is incomplete (The error require manually reset HomeKit setting
 */
#define numberOfClient 1
 //Number of notifiable value
 /*
  * Count how many notifiable value exist in your set
  * For dynamic add/drop model, please estimate the maximum number (Too few->Buffer overflow)
  */
#define numberOfNotifiableValue 1

  //If you compiling this to microcontroller, set it to 1
#define MCU 0

//#include <openssl/sha.h>
#include <stdint.h>
//#include <unistd.h>

//	typedef SHA512_CTX SHACTX;
//#define SHAInit SHA512_Init
//#define SHAUpdate SHA512_Update
//#define SHAFinal SHA512_Final
//#define SHA_DIGESTSIZE 64
//#define SHA_BlockSize 128

}
//#include "openssl/aes.h"

//AES_KEY *g_key = NULL;

static std::map<uint64_t, WXLocker> s_mapLockPlayer;
static std::map<uint64_t, void*> s_mapPlayer;

extern CNetwork    *g_network;
HANDLE hThread;
int g_iStopPlay = 0;

static WXLocker s_AirPlay;
static WXLocker s_AirMirror;
static WXLocker s_messageLock;
static WXLocker s_airplaylock;
static WXLocker s_mutextvlc;
static WXLocker s_mutexvideo;

std::string CAirPlayServer::m_strTmpDir = "";

using namespace ANNOUNCEMENT;
////using namespace std;

#ifdef _WIN32
#define close closesocket
#endif

#define STREAM_INFO  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"\
	"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\r\n"\
	"<plist version=\"1.0\">\r\n"\
	"<dict>\r\n"\
	"<key>width</key>\r\n"\
	"<integer>1280</integer>\r\n"\
	"<key>height</key>\r\n"\
	"<integer>720</integer>\r\n"\
	"<key>version</key>\r\n"\
	"<string>110.92</string>\r\n"\
	"</dict>\r\n"\
	"</plist>\r\n"

#define RECEIVEBUFFER 1024

#define AIRPLAY_STATUS_OK                  200
#define AIRPLAY_STATUS_SWITCHING_PROTOCOLS 101
#define AIRPLAY_STATUS_NEED_AUTH           401
#define AIRPLAY_STATUS_NOT_FOUND           404
#define AIRPLAY_STATUS_METHOD_NOT_ALLOWED  405
#define AIRPLAY_STATUS_PRECONDITION_FAILED 412
#define AIRPLAY_STATUS_NOT_IMPLEMENTED     501
#define AIRPLAY_STATUS_NO_RESPONSE_NEEDED  1000

CCriticalSection CAirPlayServer::ServerInstanceLock;
CAirPlayServer *CAirPlayServer::ServerInstance = NULL;

int CAirPlayServer::m_isPlaying = 0;
int CAirPlayServer::m_isPaused = 0;
int CAirPlayServer::m_isPaused2 = 0;
std::map<uint64_t, int> CAirPlayServer::g_mapAirPlaying = std::map<uint64_t, int>();
int CAirPlayServer::iMirroring = 0;


#define EVENT_NONE     -1
#define EVENT_PLAYING   0
#define EVENT_PAUSED    1
#define EVENT_LOADING   2
#define EVENT_STOPPED   3
#define EVENT_YOUTUBE   4
#define EVENT_YOUTUBE_1 5
#define EVENT_YOUTUBE_2 6
/*const */char *eventStrings[6] = { (char*)"playing", (char*)"paused", (char*)"loading", (char*)"stopped"};

#define PLAYBACK_INFO  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"\
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\r\n"\
"<plist version=\"1.0\">\r\n"\
"<dict>\r\n"\
"<key>duration</key>\r\n"\
"<real>%f</real>\r\n"\
"<key>loadedTimeRanges</key>\r\n"\
"<array>\r\n"\
"\t\t<dict>\r\n"\
"\t\t\t<key>duration</key>\r\n"\
"\t\t\t<real>%f</real>\r\n"\
"\t\t\t<key>start</key>\r\n"\
"\t\t\t<real>0.0</real>\r\n"\
"\t\t</dict>\r\n"\
"</array>\r\n"\
"<key>playbackBufferEmpty</key>\r\n"\
"<true/>\r\n"\
"<key>playbackBufferFull</key>\r\n"\
"<false/>\r\n"\
"<key>playbackLikelyToKeepUp</key>\r\n"\
"<true/>\r\n"\
"<key>position</key>\r\n"\
"<real>%f</real>\r\n"\
"<key>rate</key>\r\n"\
"<real>%d</real>\r\n"\
"<key>readyToPlay</key>\r\n"\
"<true/>\r\n"\
"<key>seekableTimeRanges</key>\r\n"\
"<array>\r\n"\
"\t\t<dict>\r\n"\
"\t\t\t<key>duration</key>\r\n"\
"\t\t\t<real>%f</real>\r\n"\
"\t\t\t<key>start</key>\r\n"\
"\t\t\t<real>0.0</real>\r\n"\
"\t\t</dict>\r\n"\
"</array>\r\n"\
"</dict>\r\n"\
"</plist>\r\n"

#define PLAYBACK_INFO_NOT_READY  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"\
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\r\n"\
"<plist version=\"1.0\">\r\n"\
"<dict>\r\n"\
"<key>readyToPlay</key>\r\n"\
"<false/>\r\n"\
"</dict>\r\n"\
"</plist>\r\n"

#define AUDIO_MODE_XML "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"\
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\r\n"\
"<plist version=\"1.0\">\r\n"\
"<dict>\r\n"\
"<key>errorCode</key>\r\n"\
"<integer>0</integer>\r\n"\
"</dict>\r\n"\
"</plist>\r\n"

#define SERVER_INFO  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"\
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\r\n"\
"<plist version=\"1.0\">\r\n"\
"<dict>\r\n"\
"<key>deviceid</key>\r\n"\
"<string>%s</string>\r\n"\
"<key>features</key>\r\n"\
"<integer>119</integer>\r\n"\
"<key>model</key>\r\n"\
"<string>Kodi,1</string>\r\n"\
"<key>protovers</key>\r\n"\
"<string>1.0</string>\r\n"\
"<key>srcvers</key>\r\n"\
"<string>" AIRPLAY_SERVER_VERSION_STR "</string>\r\n"\
"</dict>\r\n"\
"</plist>\r\n"

#define EVENT_INFO "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\r\n"\
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n\r\n"\
"<plist version=\"1.0\">\r\n"\
"<dict>\r\n"\
"<key>category</key>\r\n"\
"<string>video</string>\r\n"\
"<key>sessionID</key>\r\n"\
"<integer>%d</integer>\r\n"\
"<key>state</key>\r\n"\
"<string>%s</string>\r\n"\
"</dict>\r\n"\
"</plist>\r\n"\

//#define EVENT_YOUTUBE_INFO "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\r\n"\
//"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n\r\n"\
//"<plist version=\"1.0\">\r\n"\
//"<dict>\r\n"\
//"<key>category</key>\r\n"\
//"<string>video</string>\r\n"\
//"<key>sessionID</key>\r\n"\
//"<integer>%d</integer>\r\n"\
//"<key>state</key>\r\n"\
//"<string>%s</string>\r\n"\
//"</dict>\r\n"\
//"</plist>\r\n"\

#define EVENT_YOUTUBE_INFO "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\r\n"\
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n\r\n"\
"<plist version=\"1.0\">\r\n"\
"<dict>\r\n" \
"<key>request</key>\r\n" \
"<dict>\r\n" \
"<key>FCUP_Response_ClientInfo</key>\r\n" \
"<integer>0</integer>\r\n" \
"<key>FCUP_Response_ClientRef</key>\r\n" \
"<integer>0</integer>\r\n" \
"<key>FCUP_Response_Headers</key>\r\n" \
"<dict>\r\n" \
"<key>X-Playback-Session-Id</key>\r\n" \
"<string>%s</string>\r\n" \
"</dict>\r\n" \
"<key>FCUP_Response_RequestID</key>\r\n" \
"<integer>0</integer>\r\n" \
"<key>FCUP_Response_URL</key>\r\n" \
"<string>mlhls://localhost/master.m3u8</string>\r\n" \
"<key>sessionID</key>\r\n" \
"<integer>%d</integer>\r\n" \
"</dict>\r\n" \
"<key>sessionID</key>\r\n" \
"<integer>%d</integer>\r\n" \
"<key>type</key>\r\n" \
"<string>unhandledURLRequest</string>\r\n" \
"</dict>\r\n" \
"</plist>\r\n" \

#define EVENT_YOUTUBE_INFO_1 "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\r\n"\
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n\r\n"\
"<plist version=\"1.0\">\r\n"\
"<dict>\r\n" \
"<key>request</key>\r\n" \
"<dict>\r\n" \
"<key>FCUP_Response_ClientInfo</key>\r\n" \
"<integer>1</integer>\r\n" \
"<key>FCUP_Response_ClientRef</key>\r\n" \
"<integer>1</integer>\r\n" \
"<key>FCUP_Response_Headers</key>\r\n" \
"<dict>\r\n" \
"<key>X-Playback-Session-Id</key>\r\n" \
"<string>%s</string>\r\n" \
"</dict>\r\n" \
"<key>FCUP_Response_RequestID</key>\r\n" \
"<integer>1</integer>\r\n" \
"<key>FCUP_Response_URL</key>\r\n" \
"<string>%s</string>\r\n" \
"<key>sessionID</key>\r\n" \
"<integer>%d</integer>\r\n" \
"</dict>\r\n" \
"<key>sessionID</key>\r\n" \
"<integer>%d</integer>\r\n" \
"<key>type</key>\r\n" \
"<string>unhandledURLRequest</string>\r\n" \
"</dict>\r\n" \
"</plist>\r\n" \

#define EVENT_YOUTUBE_INFO_2 "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\r\n"\
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n\r\n"\
"<plist version=\"1.0\">\r\n"\
"<dict>\r\n" \
"<key>request</key>\r\n" \
"<dict>\r\n" \
"<key>FCUP_Response_ClientInfo</key>\r\n" \
"<integer>2</integer>\r\n" \
"<key>FCUP_Response_ClientRef</key>\r\n" \
"<integer>2</integer>\r\n" \
"<key>FCUP_Response_Headers</key>\r\n" \
"<dict>\r\n" \
"<key>X-Playback-Session-Id</key>\r\n" \
"<string>%s</string>\r\n" \
"</dict>\r\n" \
"<key>FCUP_Response_RequestID</key>\r\n" \
"<integer>2</integer>\r\n" \
"<key>FCUP_Response_URL</key>\r\n" \
"<string>%s</string>\r\n" \
"<key>sessionID</key>\r\n" \
"<integer>%d</integer>\r\n" \
"</dict>\r\n" \
"<key>sessionID</key>\r\n" \
"<integer>%d</integer>\r\n" \
"<key>type</key>\r\n" \
"<string>unhandledURLRequest</string>\r\n" \
"</dict>\r\n" \
"</plist>\r\n" \

#define AUTH_REALM "AirPlay"
#define AUTH_REQUIRED "WWW-Authenticate: Digest realm=\""  AUTH_REALM  "\", nonce=\"%s\"\r\n"

//
#define TMSG_MEDIA_PLAY           200
#define TMSG_MEDIA_PLAY1           2001
#define TMSG_MEDIA_STOP           201
// the PAUSE is indeed a PLAYPAUSE
#define TMSG_MEDIA_PAUSE          202
#define TMSG_MEDIA_PAUSE1          2021
#define TMSG_MEDIA_RESTART        203
#define TMSG_MEDIA_UNPAUSE        204
#define TMSG_MEDIA_PAUSE_IF_PLAYING   205

std::queue<int> g_messagequeue;

int CAirPlayServer::IsAirplayPlaying(uint64_t uniqueid)
{
	WXAutoLock lock(s_AirPlay);
	return g_mapAirPlaying[uniqueid];
}
//
void CAirPlayServer::SetAirplayPlaying(uint64_t uniqueid, int iStatus)
{
	WXAutoLock lock(s_AirPlay);
	g_mapAirPlaying[uniqueid] = iStatus;
}
//
int CAirPlayServer::IsMirroring()
{
	WXAutoLock lock(s_AirMirror);
	return iMirroring;
}
//
void CAirPlayServer::SetMirroring(int iMirror)
{
	WXAutoLock lock(s_AirMirror);
	iMirroring = iMirror;
}
//

#define PLAYLIST_PICTURE 2
void CAirPlayServer::Announce(AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data)
{
  CSingleLock lock(ServerInstanceLock);

  if ( (flag & Player) && strcmp(sender, "xbmc") == 0 && ServerInstance)
  {
	  std::string tmpstr = "";
    if (strcmp(message, "OnStop") == 0)
    {
      bool shouldRestoreVolume = true;
      if (data.isMember("player") && data["player"].isMember("playerid"))
        shouldRestoreVolume = (data["player"]["playerid"] != PLAYLIST_PICTURE);

	  ServerInstance->AnnounceToClients(EVENT_STOPPED, tmpstr);
    }
    else if (strcmp(message, "OnPlay") == 0)
    {
      ServerInstance->AnnounceToClients(EVENT_PLAYING, tmpstr);
    }
    else if (strcmp(message, "OnPause") == 0)
    {
		ServerInstance->AnnounceToClients(EVENT_PAUSED, tmpstr);
    }
  }
}

void CAirPlayServer::DisconnectPlay(uint64_t uniqueid)
{
	//stop player thread
	g_iStopPlay = 0;
	CAirPlayServer::SetAirplayPlaying(uniqueid, 0);
	//
	WXAutoLock lock(s_mutextvlc);
	if (s_mapPlayer.find(uniqueid) != s_mapPlayer.end() && s_mapPlayer[uniqueid] != NULL)
	{
		CAirPlayServer::CTCPClient::AirPlayOutputFunctions::airplay_stop(s_mapPlayer[uniqueid], uniqueid);
	}
	s_mapPlayer[uniqueid] = NULL;
}

int CreatDir(char *pszDir)
{
	int i = 0;
	int iRet;
	int iLen = strlen(pszDir);
	//在末尾加/
	if (pszDir[iLen - 1] != '\\' && pszDir[iLen - 1] != '/')
	{
		pszDir[iLen] = '/';
		pszDir[iLen + 1] = '\0';
	}

	// 创建目录
	for (i = 0; i < iLen; i++)
	{
		if (pszDir[i] == '\\' || pszDir[i] == '/')
		{
			pszDir[i] = '\0';

			//如果不存在,创建
			iRet = ACCESS(pszDir, 0);
			if (iRet != 0)
			{
				iRet = MKDIR(pszDir);
				if (iRet != 0)
				{
					return -1;
				}
			}
			//支持linux,将所有\换成/
			pszDir[i] = '/';
		}
	}

	return 0;
}



bool CAirPlayServer::StartServer(int *port, bool nonlocal)
{
	m_strTmpDir = "d:/temp/";
	char arcTmpDir[MAX_PATH] = { 0 };
	if (GetTempPathA(MAX_PATH, arcTmpDir) > 0)
	{
		m_strTmpDir = arcTmpDir;
		m_strTmpDir += "/VX.PhotoCache/";
	}
	//dir not exists
	if ((_access(m_strTmpDir.c_str(), 0)) == -1)
	{
		std::string strTmp = m_strTmpDir;
		if (CreatDir((char*)strTmp.c_str()) != 0)
		{
			WXLogWriteNew("create temp dir fail.");
		}
	}
	CSpecialProtocol::SetTempPath(m_strTmpDir);
  StopServer(true);

  CSingleLock lock(ServerInstanceLock);
  ServerInstance = new CAirPlayServer(*port, nonlocal);

  ServerInstance->Create();

  // if (ServerInstance->Initialize())
  // {
  //*port = ServerInstance->m_port;
  //   ServerInstance->Create();
  //////start message process thread
  ////HANDLE messageth = (HANDLE)_beginthreadex(NULL, 0, ProcessAirPlayMessage, NULL, 0, NULL);
  ////CloseHandle(messageth);

  return true;
  // }
  // else
  //   return false;
}

void PutMessage(int iMessage)
{
	WXAutoLock lock(s_messageLock);
	g_messagequeue.push(iMessage);
}

bool CAirPlayServer::SetCredentials(bool usePassword, const std::string& password)
{
  CSingleLock lock(ServerInstanceLock);
  bool ret = false;

  if (ServerInstance)
  {
    ret = ServerInstance->SetInternalCredentials(usePassword, password);
  }
  return ret;
}

bool CAirPlayServer::SetInternalCredentials(bool usePassword, const std::string& password)
{
  m_usePassword = usePassword;
  m_password = password;
  return true;
}



void CAirPlayServer::StopServer(bool bWait)
{
  CSingleLock lock(ServerInstanceLock);
  //clean up the photo cache temp folder
  //ClearPhotoAssetCache();

  if (ServerInstance)
  {
    ServerInstance->StopThread(bWait);
    if (bWait)
    {
      delete ServerInstance;
      ServerInstance = NULL;
    }
  }
}

bool CAirPlayServer::IsRunning()
{
  if (ServerInstance == NULL)
    return false;

  return ((AThread*)ServerInstance)->IsRunning();
}

void CAirPlayServer::AnnounceToClients(int state, std::string &strSessionId)
{
  CSingleLock lock (m_connectionLock);
  
  std::vector<CTCPClient*>::iterator it2;
  for (it2 = m_connections.begin(); it2 != m_connections.end(); ++it2)
  {
	  CTCPClient* it = *it2;

	  if (strSessionId == it->m_sessionId)
	  {
		  std::string reverseHeader;
		  std::string reverseBody;
		  std::string response;
		  int reverseSocket = INVALID_SOCKET;
		  it->ComposeReverseEvent(reverseHeader, reverseBody, state);

		  // Send event status per reverse http socket (play, loading, paused)
		  // if we have a reverse header and a reverse socket
		  if (reverseHeader.size() > 0 && m_reverseSockets.find(it->m_sessionId) != m_reverseSockets.end())
		  {
			  //search the reverse socket to this sessionid
			  response = StringUtils::Format("POST /event HTTP/1.1\r\n");
			  reverseSocket = m_reverseSockets[it->m_sessionId]; //that is our reverse socket
			  response += reverseHeader;
		  }
		  response += "\r\n";

		  if (reverseBody.size() > 0)
		  {
			  response += reverseBody;
		  }

		  // don't send it to the connection object
		  // the reverse socket itself belongs to
		  if (reverseSocket != INVALID_SOCKET && reverseSocket != it->m_socket)
		  {
			  send(reverseSocket, response.c_str(), response.size(), 0);//send the event status on the eventSocket
		  }
		  break;
	  }
	  
  }
}

CAirPlayServer::CAirPlayServer(int port, bool nonlocal) : AThread("AirPlayServer")
{
  m_port = port;
  m_nonlocal = nonlocal;
  m_ServerSocket = INVALID_SOCKET;
  m_usePassword = false;
  m_origVolume = -1;
  m_videoplays = std::map<std::string, int>();
  m_reverseSockets = std::map<std::string, int>();
}

CAirPlayServer::~CAirPlayServer()
{
}

void handleZeroconfAnnouncement(int port = 36666)
{
#if defined(HAS_ZEROCONF)
  static EndTime timeout(2000);
  static int g_updateNumber = 0;
  if(timeout.IsTimePast())
  {
	  bool boddnum = true;
	  if (g_updateNumber % 2 == 0)
	  {
		  g_updateNumber = 1;
	  }
	  else 
	  {
		  boddnum = false;
		  g_updateNumber = 0;
	  }

	  //dnssd_update(CNetworkServices::m_dnssd, boddnum);

	if (1) 
	{
		dnssd_unregister(CNetworkServices::m_dnssd);

		std::string strAppName = CNetworkServices::Get().m_strAppName;// "app";
		CNetworkInterface* iface = g_network->GetFirstConnectedInterface();
		std::string address = "";
		if (iface)
		{
			address = iface->GetMacAddress();
		}
		else
		{
			if (CNetworkServices::m_strVMNetMac != "")
			{
				address = CNetworkServices::m_strVMNetMac;
			}
		}

		/*
		 *  将对应的mac地址改成唯一标志，避免串投
		 */
		std::string uid = CNetworkServices::Get().GetUID();
		{
			std::vector<std::pair<std::string, std::string> > txt;
			txt.push_back(std::make_pair("deviceid", iface != NULL ? uid : "FF:FF:FF:FF:FF:F2"));
			txt.push_back(std::make_pair("model", "AppleTV3,2C"));//Xbmc,1 AppleTV3,2
			txt.push_back(std::make_pair("srcvers", "220.68"));//101.28 200.54

			txt.push_back(std::make_pair("rmodel", "PC1.0"));

			txt.push_back(std::make_pair("rrv", "1.01"));
			txt.push_back(std::make_pair("rsv", "1.00"));

			//ios 9
			txt.push_back(std::make_pair("vv", "2"));
			txt.push_back(std::make_pair("flags", "0x44"));
			txt.push_back(std::make_pair("pk", "f3769a660475d27b4f6040381d784645e13e21c53e6d2da6a8c3d757086fc336"));
			if (iface != NULL)
			{
				txt.push_back(std::make_pair("vncip", iface->GetCurrentIPAddress().c_str()));
				//WXLogWriteNew(iface->GetCurrentIPAddress().c_str());
				txt.push_back(std::make_pair("vncport", "5900"));
				txt.push_back(std::make_pair("vncpwd", "1234"));
				txt.push_back(std::make_pair("airplayname", strAppName.c_str()));
				txt.push_back(std::make_pair("pcversion", CNetworkServices::Get().m_strPcVersion.c_str()));
			}
			txt.push_back(std::make_pair("features", "0x5A7FFFF7,0x1E"));

			if (boddnum)
				txt.push_back(std::make_pair("xbmcdummy", "evendummy"));
			else
				txt.push_back(std::make_pair("xbmcdummy", "odddummy"));

			if (!CNetworkServices::m_dnssd)
			{
				CNetworkServices::m_dnssd = dnssd_init(NULL);
				if (!CNetworkServices::m_dnssd) {
					printf("Failed to init dnssd\n");
					return;
				}
			}
			dnssd_register(CNetworkServices::m_dnssd, strAppName.c_str(), "_airplay._tcp", port, txt);
		}
		{
			std::string macAddress = "";
			std::string macAddressNew = "";
			if (iface)
			{
				macAddress = iface->GetMacAddress();
				macAddressNew = macAddress;
				StringUtils::Replace(macAddress, ":", "");
				while (macAddress.size() < 12)
				{
					macAddress = '0' + macAddress;
				}
			}
			else
			{
				macAddress = "000102030405";
				macAddressNew = "00:01:02:03:04:05";
			}

			/*
			 *  将对应的mac地址改成唯一标志，避免串投
			 */
			std::string appName = StringUtils::Format("%s@%s",
				uid.c_str(),
				strAppName.c_str());
			std::vector<std::pair<std::string, std::string> > txt;
			txt.push_back(std::make_pair("txtvers", "1"));
			txt.push_back(std::make_pair("cn", "1,3"));//0,1 1,3
			txt.push_back(std::make_pair("ch", "2"));
			txt.push_back(std::make_pair("ek", "1"));
			txt.push_back(std::make_pair("et", "0,3,5"));//0,3,5 0,1
			txt.push_back(std::make_pair("sv", "false"));
			txt.push_back(std::make_pair("tp", "UDP"));
			txt.push_back(std::make_pair("sm", "false"));
			txt.push_back(std::make_pair("ss", "16"));
			txt.push_back(std::make_pair("sr", "44100"));
			//txt.push_back(std::make_pair("pw",  usePassword?"true":"false"));
			//txt.push_back(std::make_pair("pw", "true"));
			txt.push_back(std::make_pair("vn", "65537"));
			txt.push_back(std::make_pair("da", "true"));
			txt.push_back(std::make_pair("md", "0,1,2"));
			txt.push_back(std::make_pair("am", "AppleTV3,2C"));//Kodi,1 AppleTV3,2
			txt.push_back(std::make_pair("vs", "220.68"));

			//ios 9
			txt.push_back(std::make_pair("sf", "0x44"));
			txt.push_back(std::make_pair("ft", "0x5A7FFFF7,0x1E"));//0x5A7FFFF7,0x1E        0x0A7FFFF7
			txt.push_back(std::make_pair("pk", "f3769a660475d27b4f6040381d784645e13e21c53e6d2da6a8c3d757086fc336"));

			if (boddnum)
				txt.push_back(std::make_pair("xbmcdummy", "evendummy"));
			else
				txt.push_back(std::make_pair("xbmcdummy", "odddummy"));



			if (!CNetworkServices::m_dnssd)
			{
				CNetworkServices::m_dnssd = dnssd_init(NULL);
				if (!CNetworkServices::m_dnssd) {
					printf("Failed to init dnssd\n");
					return;
				}
			}
			dnssd_register(CNetworkServices::m_dnssd, appName.c_str(), "_raop._tcp", port, txt);
		}

	}
	timeout.Set(2000);
  }
#endif
}

//
std::string&   replace_all(std::string&   str, const   std::string&   old_value, const   std::string&   new_value)
{
	while (true)   {
		std::string::size_type   pos(0);
		if ((pos = str.find(old_value)) != std::string::npos)
			str.replace(pos, old_value.length(), new_value);
		else   break;
	}
	return   str;
}

//
char* CAirPlayServer::ProcessAirplayVideo(int fd, int counter, uint8_t *remote, char *method, char *url, char *data, int datalen, int neednew, char* sessionid, char* contenttype,
	char* auth, char* photoAction, char* photoCacheId)
{
	WXAutoLock olock(s_mutexvideo);
	char* retbuffer = NULL;
	char tmp[50] = { 0 };
	sprintf(tmp, "%d.%d.%d.%d",
		remote[0], remote[1], remote[2], remote[3]);

	if (neednew == 1)
	{
		if (m_videoplays.find(tmp) != m_videoplays.end())
		{
			m_videoplays.erase(m_videoplays.find(tmp));
		}
	}
	else if (neednew == 2)
	{
		if (sessionid != NULL)
		{
			m_reverseSockets[sessionid] = fd;
		}


		//retbuffer = (char*)malloc(responseBody.size() + 1);
		//memset(retbuffer, 0, responseBody.size() + 1);
		//strcpy(retbuffer, responseBody.c_str());

		return NULL;
	}

	if (m_videoplays.find(tmp) == m_videoplays.end())
	{
		m_videoplays[tmp] = fd;

		CTCPClient* newconnection = new CTCPClient();
		newconnection->m_socket = fd;
		newconnection->m_addrlen = 4;
		newconnection->m_strAddr = tmp;
		newconnection->m_strAddrReal.assign((char*)remote, 4);
		newconnection->m_sessionCounter = counter;
		m_connections.push_back(newconnection);
		
	}
	for (int i = m_connections.size() - 1; i >= 0; i--)
	{
		int socket = m_connections[i]->m_socket;
		if (socket == fd)
		{
			// Parse the request
			std::string responseHeader = "";
			std::string responseBody = "";
			char* querystring = NULL;
			int status = m_connections[i]->ProcessRequestNew(responseHeader, responseBody, method, url, data, datalen, sessionid, contenttype, querystring,
				auth, photoAction, photoCacheId);
			//m_reverseSockets[sessionid] = fd;
			//std::string statusMsg = "OK";
			//switch (status)
			//{
			//case AIRPLAY_STATUS_NOT_IMPLEMENTED:
			//	statusMsg = "Not Implemented";
			//	break;
			//case AIRPLAY_STATUS_SWITCHING_PROTOCOLS:
			//	statusMsg = "Switching Protocols";
			//	m_reverseSockets[sessionid] = fd;//save this socket as reverse http socket for this sessionid
			//	break;
			//case AIRPLAY_STATUS_NEED_AUTH:
			//	statusMsg = "Unauthorized";
			//	break;
			//case AIRPLAY_STATUS_NOT_FOUND:
			//	statusMsg = "Not Found";
			//	break;
			//case AIRPLAY_STATUS_METHOD_NOT_ALLOWED:
			//	statusMsg = "Method Not Allowed";
			//	break;
			//case AIRPLAY_STATUS_PRECONDITION_FAILED:
			//	statusMsg = "Precondition Failed";
			//	break;
			//}

			if (responseBody != "")
			{
				retbuffer = (char*)malloc(responseBody.size() + 1);
				memset(retbuffer, 0, responseBody.size() + 1);
				memcpy(retbuffer, responseBody.c_str(), responseBody.size());
			}

			break;
		}
	}
	return retbuffer;
}

void CAirPlayServer::Process()
{
  m_bStop = false;
  static int sessionCounter = 0;

  while (!m_bStop)
  {
    handleZeroconfAnnouncement(m_port);    
	Sleep(100 * 1000);
  }

  //Deinitialize();
}

bool CAirPlayServer::Initialize()
{
  Deinitialize();
  
  //if ((m_ServerSocket = CreateTCPServerSocket(m_port, !m_nonlocal, 10, "AIRPLAY")) == INVALID_SOCKET)
  //  return false;
  m_ServerSocket = CreateTCPServerSocket(m_port, !m_nonlocal, 10, "AIRPLAY");
  int iCount = 0;
  while (m_ServerSocket < 0)
  {
	  if (iCount++ > 3)
	  {
		  WXLogWriteNew("CreateTCPServerSocket fail for 3 times");
		  break;
	  }

	  if (m_ServerSocket == -3)
	  {
		  m_port += 2;
		  m_ServerSocket = CreateTCPServerSocket(m_port, !m_nonlocal, 10, "AIRPLAY");
	  }
	  else
	  {
		  break;
	  }
  }

  if (m_ServerSocket == INVALID_SOCKET)
  {
	  return false;
  }
  
  ;//;//CLog::Log(LOGINFO, "AIRPLAY Server: Successfully initialized");
  return true;
}

void CAirPlayServer::Deinitialize()
{
  CSingleLock lock (m_connectionLock);
  for (unsigned int i = 0; i < m_connections.size(); i++)
    m_connections[i]->Disconnect();

  m_connections.clear();
  m_reverseSockets.clear();

  if (m_ServerSocket != INVALID_SOCKET)
  {
    shutdown(m_ServerSocket, SHUT_RDWR);
    close(m_ServerSocket);
    m_ServerSocket = INVALID_SOCKET;
  }
}

CAirPlayServer::CTCPClient::CTCPClient()
{
  m_socket = INVALID_SOCKET;
  m_httpParser = new HttpParser();

  m_addrlen = sizeof(struct sockaddr_storage);

  m_bAuthenticated = false;
  m_lastEvent = EVENT_NONE;
  //
  bmp = NULL;
  mediaWidth = 0;
  mediaHeight = 0;

  bmp = NULL;
  screen = NULL;
  renderer = NULL;
  pixelData = NULL;
  screen_w = 1280;
  screen_h = 720;
}

CAirPlayServer::CTCPClient::CTCPClient(const CTCPClient& client)
{
  Copy(client);
  m_httpParser = new HttpParser();
  mediaWidth = 0;
  mediaHeight = 0;
  bmp = NULL;
  screen = NULL;
  renderer = NULL;
  pixelData = NULL;
  screen_w = 1280;
  screen_h = 720;
}

CAirPlayServer::CTCPClient::~CTCPClient()
{
  delete m_httpParser;
}

CAirPlayServer::CTCPClient& CAirPlayServer::CTCPClient::operator=(const CTCPClient& client)
{
  Copy(client);
  m_httpParser = new HttpParser();
  return *this;
}

void CAirPlayServer::CTCPClient::PushBuffer(CAirPlayServer *host, const char *buffer,
                                            int length, std::string &sessionId, std::map<std::string,
                                            int> &reverseSockets)
{
  HttpParser::status_t status = m_httpParser->addBytes(buffer, length);

  if (status == HttpParser::Done)
  {
    // Parse the request
    std::string responseHeader;
    std::string responseBody;
    int status = ProcessRequest(responseHeader, responseBody);
    sessionId = m_sessionId;
    std::string statusMsg = "OK";

    switch(status)
    {
      case AIRPLAY_STATUS_NOT_IMPLEMENTED:
        statusMsg = "Not Implemented";
        break;
      case AIRPLAY_STATUS_SWITCHING_PROTOCOLS:
        statusMsg = "Switching Protocols";
        reverseSockets[sessionId] = m_socket;//save this socket as reverse http socket for this sessionid
        break;
      case AIRPLAY_STATUS_NEED_AUTH:
        statusMsg = "Unauthorized";
        break;
      case AIRPLAY_STATUS_NOT_FOUND:
        statusMsg = "Not Found";
        break;
      case AIRPLAY_STATUS_METHOD_NOT_ALLOWED:
        statusMsg = "Method Not Allowed";
        break;
      case AIRPLAY_STATUS_PRECONDITION_FAILED:
        statusMsg = "Precondition Failed";
        break;
    }

    // Prepare the response
    std::string response;
    const time_t ltime = time(NULL);
    char *date = asctime(gmtime(&ltime)); //Fri, 17 Dec 2010 11:18:01 GMT;
    date[strlen(date) - 1] = '\0'; // remove \n
    //response = StringUtils::Format("RTSP/1.0 %d %s\nDate: %s\r\n", status, statusMsg.c_str(), date);
	response = StringUtils::Format("HTTP/1.1 %d %s\n", status, statusMsg.c_str());
	if (responseHeader.size() > 0)
    {
      response += responseHeader;
    }

	//response = StringUtils::Format("%sServer: Airtunes/220.00\r\n\r\n", response.c_str());

	//response = StringUtils::Format("%sContent-Type: %s\r\n\r\n", response.c_str(), "application/octet-stream");

    response = StringUtils::Format("%sContent-Length: %ld\r\n\r\n", response.c_str(), responseBody.size());

    if (responseBody.size() > 0)
    {
      response += responseBody;
    }

    // Send the response
    //don't send response on AIRPLAY_STATUS_NO_RESPONSE_NEEDED
    if (status != AIRPLAY_STATUS_NO_RESPONSE_NEEDED)
    {
      send(m_socket, response.c_str(), response.size(), 0);
    }
    // We need a new parser...
    delete m_httpParser;
    m_httpParser = new HttpParser;
  }
}

void CAirPlayServer::CTCPClient::Disconnect()
{
  if (m_socket != INVALID_SOCKET)
  {
    CSingleLock lock (m_critSection);
    shutdown(m_socket, SHUT_RDWR);
    close(m_socket);
    m_socket = INVALID_SOCKET;
    delete m_httpParser;
    m_httpParser = NULL;
  }
}

void CAirPlayServer::CTCPClient::Copy(const CTCPClient& client)
{
  m_socket            = client.m_socket;
  m_cliaddr           = client.m_cliaddr;
  m_addrlen           = client.m_addrlen;
  m_httpParser        = client.m_httpParser;
  m_authNonce         = client.m_authNonce;
  m_bAuthenticated    = client.m_bAuthenticated;
  m_sessionCounter    = client.m_sessionCounter;
  m_strAddr = client.m_strAddr;
  m_strAddrReal = client.m_strAddrReal;
  mediaWidth = client.mediaWidth;
  mediaHeight = client.mediaHeight;
  bmp = client.bmp;
  screen = client.screen;
  renderer = client.renderer;
  pixelData = client.pixelData;
  screen_w = client.screen_w;
  screen_h = client.screen_h;
}


void CAirPlayServer::CTCPClient::ComposeReverseEvent( std::string& reverseHeader,
                                                      std::string& reverseBody,
                                                      int state)
{
	if (state == EVENT_YOUTUBE)
	{
		reverseBody = StringUtils::Format(EVENT_YOUTUBE_INFO, eventStrings[state], m_sessionCounter, m_sessionCounter);
	}
	if (state == EVENT_YOUTUBE_1)
	{
		reverseBody = StringUtils::Format(EVENT_YOUTUBE_INFO_1, eventStrings[state - 1], eventStrings[state], m_sessionCounter, m_sessionCounter);
	}
	if (state == EVENT_YOUTUBE_2)
	{
		reverseBody = StringUtils::Format(EVENT_YOUTUBE_INFO_2, eventStrings[state - 2], eventStrings[state - 1], m_sessionCounter, m_sessionCounter);
	}

  if ( m_lastEvent != state )
  { 
    switch(state)
    {
      case EVENT_PLAYING:
      case EVENT_LOADING:
      case EVENT_PAUSED:
      case EVENT_STOPPED:      
        reverseBody = StringUtils::Format(EVENT_INFO, m_sessionCounter, eventStrings[state]);
		break;
        ;//;//CLog::Log(LOGDEBUG, "AIRPLAY: sending event: %s", eventStrings[state]);
	 // case EVENT_YOUTUBE:
		//reverseBody = StringUtils::Format(EVENT_YOUTUBE_INFO, eventStrings[state], m_sessionCounter, m_sessionCounter);
  //      break;
	 // case EVENT_YOUTUBE_1:
		//  reverseBody = StringUtils::Format(EVENT_YOUTUBE_INFO_1, eventStrings[state - 1], eventStrings[state], m_sessionCounter, m_sessionCounter);
		//  break;
    }
    reverseHeader = "Content-Type: text/x-apple-plist+xml\r\n";
    reverseHeader = StringUtils::Format("%sContent-Length: %ld\r\n",reverseHeader.c_str(), reverseBody.size());
    reverseHeader = StringUtils::Format("%sx-apple-session-id: %s\r\n",reverseHeader.c_str(), m_sessionId.c_str());
    m_lastEvent = state;
  }
}

void CAirPlayServer::CTCPClient::ComposeAuthRequestAnswer(std::string& responseHeader, std::string& responseBody)
{
  int16_t random=rand();
  std::string randomStr = StringUtils::Format("%i", random);
  m_authNonce=XBMC_MD5::GetMD5(randomStr);
  responseHeader = StringUtils::Format(AUTH_REQUIRED, m_authNonce.c_str());
  responseBody.clear();
}


//as of rfc 2617
std::string calcResponse(const std::string& username,
                        const std::string& password,
                        const std::string& realm,
                        const std::string& method,
                        const std::string& digestUri,
                        const std::string& nonce)
{
  std::string response;
  std::string HA1;
  std::string HA2;

  HA1 = XBMC_MD5::GetMD5(username + ":" + realm + ":" + password);
  HA2 = XBMC_MD5::GetMD5(method + ":" + digestUri);
  StringUtils::ToLower(HA1);
  StringUtils::ToLower(HA2);
  response = XBMC_MD5::GetMD5(HA1 + ":" + nonce + ":" + HA2);
  StringUtils::ToLower(response);
  return response;
}

//helper function
//from a string field1="value1", field2="value2" it parses the value to a field
std::string getFieldFromString(const std::string &str, const char* field)
{
  std::vector<std::string> tmpAr1 = StringUtils::Split(str, ",");
  for(std::vector<std::string>::const_iterator i = tmpAr1.begin(); i != tmpAr1.end(); ++i)
  {
    if (i->find(field) != std::string::npos)
    {
		std::vector<std::string> tmpAr2 = StringUtils::Split(*i, "=");
      if (tmpAr2.size() == 2)
      {
        StringUtils::Replace(tmpAr2[1], "\"", "");//remove quotes
        return tmpAr2[1];
      }
    }
  }
  return "";
}

bool CAirPlayServer::CTCPClient::checkAuthorization(const std::string& authStr,
                                                    const std::string& method,
                                                    const std::string& uri)
{
  bool authValid = true;

  std::string username;

  if (authStr.empty())
    return false;

  //first get username - we allow all usernames for airplay (usually it is AirPlay)
  username = getFieldFromString(authStr, "username");
  if (username.empty())
  {
    authValid = false;
  }

  //second check realm
  if (authValid)
  {
    if (getFieldFromString(authStr, "realm") != AUTH_REALM)
    {
      authValid = false;
    }
  }

  //third check nonce
  if (authValid)
  {
    if (getFieldFromString(authStr, "nonce") != m_authNonce)
    {
      authValid = false;
    }
  }

  //forth check uri
  if (authValid)
  {
    if (getFieldFromString(authStr, "uri") != uri)
    {
      authValid = false;
    }
  }

  //last check response
  if (authValid)
  {
     std::string realm = AUTH_REALM;
     std::string ourResponse = calcResponse(username, ServerInstance->m_password, realm, method, uri, m_authNonce);
     std::string theirResponse = getFieldFromString(authStr, "response");
     if (!StringUtils::EqualsNoCase(theirResponse, ourResponse))
     {
       authValid = false;
       ;//;//CLog::Log(LOGDEBUG,"AirAuth: response mismatch - our: %s theirs: %s",ourResponse.c_str(), theirResponse.c_str());
     }
     else
     {
       ;//;//CLog::Log(LOGDEBUG, "AirAuth: successfull authentication from AirPlay client");
     }
  }
  m_bAuthenticated = authValid;
  return m_bAuthenticated;
}

std::string getStringFromPlist(plist_t node)
{
  std::string ret;
  char *tmpStr = NULL;
  plist_get_string_val(node, &tmpStr);
  ret = tmpStr;

  return ret;
}

void renderTexture(SDL_Texture *tex, SDL_Renderer *ren, SDL_Rect dst,
	SDL_Rect *clip) {
	SDL_RenderCopy(ren, tex, clip, &dst);
}
//
void renderTexture2(SDL_Texture *tex, SDL_Renderer *ren, int x, int y,
	SDL_Rect *clip) {
	SDL_Rect dst;
	dst.x = x;
	dst.y = y;
	if (clip != NULL) {
		dst.w = clip->w;
		dst.h = clip->h;
	}
	else {
		SDL_QueryTexture(tex, NULL, NULL, &dst.w, &dst.h);
	}
	renderTexture(tex, ren, dst, clip);
}

const char* getQueryString(char* url)
{
	const char* pos = url;
	while (*pos) {
		if (*pos == '?') {
			pos++;
			break;
		}
		pos++;
	}
	return pos;
}

struct YoutubeURL 
{
	std::string strAudio;
	std::string strVideo;
};
static std::map<uint64_t, std::string> g_IDToUrls = std::map<uint64_t, std::string>();
static std::map<uint64_t, YoutubeURL> g_IDToAVUrls = std::map<uint64_t, YoutubeURL>();

void SplitString(const std::string& s, std::vector<std::string>& v, const std::string& c)
{
	std::string::size_type pos1, pos2;
	pos2 = s.find(c);
	pos1 = 0;
	while (std::string::npos != pos2)
	{
		v.push_back(s.substr(pos1, pos2 - pos1));

		pos1 = pos2 + c.size();
		pos2 = s.find(c, pos1);
	}
	if (pos1 != s.length())
		v.push_back(s.substr(pos1));
}

std::string GetMaxResulution(const std::string& strSour) 
{
	std::vector<std::string> vecStr;
	SplitString(strSour, vecStr, "RESOLUTION=");
	int index = 1;
	int maxval = 0;
	std::string strres = "";
	for (int i = 1; i < vecStr.size(); i++) 
	{
		const std::string& tmpstr = vecStr[i];
		std::string str1 = tmpstr.substr(0, tmpstr.find("x"));
		int tmpwidth = atoi(str1.c_str());
		if (maxval < tmpwidth) 
		{
			maxval = tmpwidth;
			index = i;
			strres = str1;
		}
	}
	return strres;// vecStr[index];
}

//
int CAirPlayServer::CTCPClient::ProcessRequestNew(std::string& responseHeader,
	std::string& responseBody, char* method2, char* url, char* data, int datalen, char* sessionid, char* contenttype, char*querystring, char* authorization2,
	char* photoAction2, char* photoCacheId2)
{
	std::string method = method2 ? method2 : "";// m_httpParser->getMethod() ? m_httpParser->getMethod() : "";
	std::string uri = url ? url : "";// m_httpParser->getUri() ? m_httpParser->getUri() : "";
	std::string queryString = getQueryString((char*)uri.c_str()) ? getQueryString((char*)uri.c_str()) : "";// m_httpParser->getQueryString() ? m_httpParser->getQueryString() : "";

	std::string body;// = m_httpParser->getBody() ? m_httpParser->getBody() : "";

	if (data == NULL)
	{
		body = "";
	}
	else
	{
		body.assign(data, datalen);
	}

	std::string contentType = contenttype ? contenttype : "";// m_httpParser->getValue("content-type") ? m_httpParser->getValue("content-type") : "";
	m_sessionId = sessionid ? sessionid : "";// m_httpParser->getValue("x-apple-session-id") ? m_httpParser->getValue("x-apple-session-id") : "";
	std::string authorization = authorization2 ? authorization2 : "";// m_httpParser->getValue("authorization") ? m_httpParser->getValue("authorization") : "";
	std::string photoAction = photoAction2 ? photoAction2 : "";// m_httpParser->getValue("x-apple-assetaction") ? m_httpParser->getValue("x-apple-assetaction") : "";
	std::string photoCacheId = photoCacheId2 ? photoCacheId2 : "";// m_httpParser->getValue("x-apple-assetkey") ? m_httpParser->getValue("x-apple-assetkey") : "";

	int contentlength = datalen;

	int status = AIRPLAY_STATUS_OK;
	bool needAuth = false;

	if (m_sessionId.empty())
		m_sessionId = "00000000-0000-0000-0000-000000000000";

	if (ServerInstance->m_usePassword && !m_bAuthenticated)
	{
		needAuth = true;
	}

	size_t startQs = uri.find('?');
	if (startQs != std::string::npos)
	{
		uri.erase(startQs);
	}
	uint64_t uniqueid = 0;
	memcpy(&uniqueid, m_strAddrReal.c_str(), 4);
	printf("uri %s\n", uri.c_str());

	// This is the socket which will be used for reverse HTTP
	// negotiate reverse HTTP via upgrade
	if (uri == "/reverse")
	{
		status = AIRPLAY_STATUS_SWITCHING_PROTOCOLS;
		responseHeader = "Upgrade: PTTH/1.0\r\nConnection: Upgrade\r\n";
	}
	//
	else if (method == "GET" && uri == "/txtAPRA")
	{
		plist_t root = NULL;
		char *xml = NULL;
		int size = 0;

		const char *tmpres = "\x62\x70\x6c\x69\x73\x74\x30\x30\xd5\x01\x03\x05\x07\x09\x02\x04\x06\x08\x0a\x58\x64\x65\x76\x69\x63\x65\x49\x44\x5f\x10\x11\x34\x35\x3a\x35\x30\x3a\x35\x36\x3a\x63\x30\x3a\x34\x35\x3a\x30\x38\x54\x6e\x61\x6d\x65\x5b\x69\x54\x6f\x6f\x6c\x73\x5b\x41\x50\x50\x5d\x53\x72\x61\x70\x11\x35\x6e\x5a\x74\x78\x74\x41\x69\x72\x50\x6c\x61\x79\x4f\x10\xd0\x1a\x64\x65\x76\x69\x63\x65\x69\x64\x3d\x34\x35\x3a\x35\x30\x3a\x35\x36\x3a\x63\x30\x3a\x34\x35\x3a\x30\x38\x18\x66\x65\x61\x74\x75\x72\x65\x73\x3d\x30\x78\x35\x41\x37\x46\x46\x46\x46\x37\x2c\x30\x78\x31\x45\x0a\x66\x6c\x61\x67\x73\x3d\x30\x78\x34\x34\x10\x6d\x6f\x64\x65\x6c\x3d\x41\x70\x70\x6c\x65\x54\x56\x33\x2c\x32\x0e\x73\x72\x63\x76\x65\x72\x73\x3d\x32\x32\x30\x2e\x36\x38\x04\x76\x76\x3d\x32\x27\x70\x69\x3d\x35\x65\x36\x36\x63\x66\x39\x62\x2d\x30\x61\x33\x39\x2d\x34\x65\x30\x63\x2d\x39\x64\x33\x32\x2d\x30\x38\x31\x61\x38\x63\x65\x36\x33\x32\x33\x31\x43\x70\x6b\x3d\x35\x63\x30\x38\x61\x66\x63\x66\x32\x65\x32\x63\x38\x61\x36\x64\x32\x34\x35\x34\x62\x63\x34\x35\x64\x61\x36\x33\x36\x36\x63\x36\x32\x34\x32\x36\x30\x62\x36\x35\x35\x64\x66\x62\x38\x34\x62\x33\x32\x66\x35\x35\x30\x65\x30\x63\x31\x65\x37\x31\x63\x30\x61\x36\xa0\x57\x74\x78\x74\x52\x41\x4f\x50\x4f\x10\xe1\x09\x74\x78\x74\x76\x65\x72\x73\x3d\x31\x04\x63\x68\x3d\x32\x0a\x63\x6e\x3d\x30\x2c\x31\x2c\x32\x2c\x33\x07\x64\x61\x3d\x74\x72\x75\x65\x08\x65\x74\x3d\x30\x2c\x33\x2c\x35\x12\x66\x74\x3d\x30\x78\x35\x41\x37\x46\x46\x46\x46\x37\x2c\x30\x78\x31\x45\x08\x6d\x64\x3d\x30\x2c\x31\x2c\x32\x08\x70\x77\x3d\x66\x61\x6c\x73\x65\x08\x73\x76\x3d\x66\x61\x6c\x73\x65\x08\x73\x72\x3d\x34\x34\x31\x30\x30\x05\x73\x73\x3d\x31\x36\x06\x74\x70\x3d\x55\x44\x50\x09\x76\x73\x3d\x32\x32\x30\x2e\x36\x38\x04\x76\x76\x3d\x32\x08\x76\x6e\x3d\x36\x35\x35\x33\x37\x0d\x61\x6d\x3d\x41\x70\x70\x6c\x65\x54\x56\x33\x2c\x32\x07\x73\x66\x3d\x30\x78\x34\x34\x43\x70\x6b\x3d\x35\x63\x30\x38\x61\x66\x63\x66\x32\x65\x32\x63\x38\x61\x36\x64\x32\x34\x35\x34\x62\x63\x34\x35\x64\x61\x36\x33\x36\x36\x63\x36\x32\x34\x32\x36\x30\x62\x36\x35\x35\x64\x66\x62\x38\x34\x62\x33\x32\x66\x35\x35\x30\x65\x30\x63\x31\x65\x37\x31\x63\x30\x61\x36\xa0\x00\x08\x00\x13\x00\x1c\x00\x30\x00\x35\x00\x41\x00\x45\x00\x48\x00\x53\x01\x27\x01\x2f\x00\x00\x00\x00\x00\x00\x02\x01\x00\x00\x00\x00\x00\x00\x00\x0b\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x02\x14";
		int reslen = 586;

		plist_from_bin(tmpres, reslen, &root);
		if (root)
		{
			plist_to_xml(root, &xml, (uint32_t*)&size);
			/* TODO: in this plist, we will get param1&param2, which is the
			encoded aeskey & aesiv */
			if (xml)
				fprintf(stderr, "%s\n", xml);

			if (plist_dict_get_size(root))
			{
				plist_t tmpNode2 = plist_dict_get_item(root, "deviceID");
				CNetworkInterface* iface = g_network->GetFirstConnectedInterface();
				std::string address = "";
				if (iface)
				{
					address = iface->GetMacAddress();
				}
				else
				{
					if (CNetworkServices::m_strVMNetMac != "")
					{
						address = CNetworkServices::m_strVMNetMac;
					}
				}
				if (tmpNode2)
				{
					plist_set_string_val(tmpNode2, address.c_str());
				}
				tmpNode2 = plist_dict_get_item(root, "name");
				if (tmpNode2)
				{
					plist_set_string_val(tmpNode2, CNetworkServices::Get().m_strAppName.c_str());
				}
				tmpNode2 = plist_dict_get_item(root, "rap");
				if (tmpNode2)
				{
					plist_set_uint_val(tmpNode2, CAirTunesServer::GetAirtunePort());
				}
				tmpNode2 = plist_dict_get_item(root, "txtAirPlay");
				if (tmpNode2)
				{
					char tmpbuf[512] = { 0 };
					uint64_t len = 0;
					char *data = NULL;
					plist_get_data_val(tmpNode2, &data, &len);
					memcpy(tmpbuf, data, (size_t)len);
					char strMacAddress[256] = { 0 };
					memcpy(strMacAddress, "deviceid=", strlen("deviceid="));
					memcpy(strMacAddress + strlen("deviceid="), address.c_str(),
						strlen("deviceid=45:50:56:c0:45:08"));
					memcpy(tmpbuf + 1, strMacAddress, strlen("deviceid=45:50:56:c0:45:08"));
					char *substr = strstr(tmpbuf, "pk=");
					memcpy(substr, "pk=f3769a660475d27b4f6040381d784645e13e21c53e6d2da6a8c3d757086fc336",
						strlen("pk=c35c2c53ce4d1af5631d7badc9390a2840852a963bd521eb885c8a42a24fada5"));
					plist_set_data_val(tmpNode2, tmpbuf, len);
				}
				tmpNode2 = plist_dict_get_item(root, "txtRAOP");
				if (tmpNode2)
				{
					char tmpbuf[512] = { 0 };
					uint64_t len = 0;
					char *data = NULL;
					plist_get_data_val(tmpNode2, &data, &len);
					memcpy(tmpbuf, data, (size_t)len);
					char *substr = strstr(tmpbuf, "pk=");
					memcpy(substr, "pk=f3769a660475d27b4f6040381d784645e13e21c53e6d2da6a8c3d757086fc336",
						strlen("pk=c35c2c53ce4d1af5631d7badc9390a2840852a963bd521eb885c8a42a24fada5"));
					plist_set_data_val(tmpNode2, tmpbuf, len);
				}
			}

			//
			char *responseData = NULL;
			uint32_t responseDataLen = 0;
			plist_to_bin(root, &responseData, &responseDataLen);
			responseBody.assign((char*)responseData, responseDataLen);

			xml = NULL;
			size = 0;
			plist_to_xml(root, &xml, (uint32_t*)&size);
			/* TODO: in this plist, we will get param1&param2, which is the
			encoded aeskey & aesiv */
			if (xml)
				fprintf(stderr, "%s\n", xml);

		}
	}
	// The rate command is used to play/pause media.
	// A value argument should be supplied which indicates media should be played or paused.
	// 0.000000 => pause
	// 1.000000 => play
	else if (uri == "/rate")
	{
		const char* found = strstr(queryString.c_str(), "value=");
		int rate = found ? (int)(atof(found + strlen("value=")) + 0.5f) : 0;

		;//;//CLog::Log(LOGDEBUG, "AIRPLAY: got request %s with rate %i", uri.c_str(), rate);

		if (needAuth && !checkAuthorization(authorization, method, uri))
		{
			status = AIRPLAY_STATUS_NEED_AUTH;
		}
		else if (rate == 0)
		{
			if (m_isPlaying && !m_isPaused)
			{
				////pause video
				if (m_isPaused2)
				{
					CAirPlayServer::CTCPClient::AirPlayOutputFunctions::airplay_play(this,uniqueid);
					m_isPaused2 = 0;
				}
				else
				{
					CAirPlayServer::CTCPClient::AirPlayOutputFunctions::airplay_pause(this, uniqueid);
				}
				m_isPaused = 1;
			}
		}
		else
		{
			if (m_isPlaying && m_isPaused)
			{
				////play video
				CAirPlayServer::CTCPClient::AirPlayOutputFunctions::airplay_play(this, uniqueid);
				m_isPaused = 0;
			}
		}
	}

	// The volume command is used to change playback volume.
	// A value argument should be supplied which indicates how loud we should get.
	// 0.000000 => silent
	// 1.000000 => loud
	else if (uri == "/volume")
	{
		const char* found = strstr(queryString.c_str(), "volume=");
		float volume = found ? (float)strtod(found + strlen("volume="), NULL) : 0;

		;//;//CLog::Log(LOGDEBUG, "AIRPLAY: got request %s with volume %f", uri.c_str(), volume);

		if (needAuth && !checkAuthorization(authorization, method, uri))
		{
			status = AIRPLAY_STATUS_NEED_AUTH;
		}
		else if (volume >= 0 && volume <= 1)
		{

		}
	}
	else if (uri == "/audioMode")
	{
		responseBody = StringUtils::Format(AUDIO_MODE_XML);
		responseHeader = "Content-Type: text/x-apple-plist+xml\r\n";
	}

	// Contains a header like format in the request body which should contain a
	// Content-Location and optionally a Start-Position



	else if (uri == "/action") 
	{
		uint64_t uniqueid = 0;
		memcpy(&uniqueid, m_strAddrReal.c_str(), 4);
		if (contentType == "application/x-apple-binary-plist")
		{
			CAirPlayServer::m_isPlaying++;


			{


				const char* bodyChr = body.c_str();// m_httpParser->getBody();

				//const char* bodyChr = "\x62\x70\x6c\x69\x73\x74\x30\x30\xd2\x01\x02\x03\x04\x54\x74\x79\x70\x65\x56\x70\x61\x72\x61\x6d\x73\x5f\x10\x14\x75\x6e\x68\x61\x6e\x64\x6c\x65\x64\x55\x52\x4c\x52\x65\x73\x70\x6f\x6e\x73\x65\xd4\x05\x06\x07\x08\x09\x0a\x0b\x0b\x5f\x10\x11\x46\x43\x55\x50\x5f\x52\x65\x73\x70\x6f\x6e\x73\x65\x5f\x55\x52\x4c\x5f\x10\x12\x46\x43\x55\x50\x5f\x52\x65\x73\x70\x6f\x6e\x73\x65\x5f\x44\x61\x74\x61\x5f\x10\x17\x46\x43\x55\x50\x5f\x52\x65\x73\x70\x6f\x6e\x73\x65\x5f\x52\x65\x71\x75\x65\x73\x74\x49\x44\x5f\x10\x18\x46\x43\x55\x50\x5f\x52\x65\x73\x70\x6f\x6e\x73\x65\x5f\x53\x74\x61\x74\x75\x73\x43\x6f\x64\x65\x5f\x10\x1d\x6d\x6c\x68\x6c\x73\x3a\x2f\x2f\x6c\x6f\x63\x61\x6c\x68\x6f\x73\x74\x2f\x6d\x61\x73\x74\x65\x72\x2e\x6d\x33\x75\x38\x4f\x11\x05\x3d\x23\x45\x58\x54\x4d\x33\x55\x0a\x23\x45\x58\x54\x2d\x58\x2d\x49\x4e\x44\x45\x50\x45\x4e\x44\x45\x4e\x54\x2d\x53\x45\x47\x4d\x45\x4e\x54\x53\x0a\x23\x45\x58\x54\x2d\x58\x2d\x4d\x45\x44\x49\x41\x3a\x55\x52\x49\x3d\x22\x6d\x6c\x68\x6c\x73\x3a\x2f\x2f\x6c\x6f\x63\x61\x6c\x68\x6f\x73\x74\x2f\x69\x74\x61\x67\x2f\x32\x33\x33\x2f\x6d\x65\x64\x69\x61\x64\x61\x74\x61\x2e\x6d\x33\x75\x38\x22\x2c\x54\x59\x50\x45\x3d\x41\x55\x44\x49\x4f\x2c\x47\x52\x4f\x55\x50\x2d\x49\x44\x3d\x22\x32\x33\x33\x22\x2c\x44\x45\x46\x41\x55\x4c\x54\x3d\x59\x45\x53\x2c\x41\x55\x54\x4f\x53\x45\x4c\x45\x43\x54\x3d\x59\x45\x53\x2c\x4e\x41\x4d\x45\x3d\x22\x44\x65\x66\x61\x75\x6c\x74\x22\x0a\x23\x45\x58\x54\x2d\x58\x2d\x4d\x45\x44\x49\x41\x3a\x55\x52\x49\x3d\x22\x6d\x6c\x68\x6c\x73\x3a\x2f\x2f\x6c\x6f\x63\x61\x6c\x68\x6f\x73\x74\x2f\x69\x74\x61\x67\x2f\x32\x33\x34\x2f\x6d\x65\x64\x69\x61\x64\x61\x74\x61\x2e\x6d\x33\x75\x38\x22\x2c\x54\x59\x50\x45\x3d\x41\x55\x44\x49\x4f\x2c\x47\x52\x4f\x55\x50\x2d\x49\x44\x3d\x22\x32\x33\x34\x22\x2c\x44\x45\x46\x41\x55\x4c\x54\x3d\x59\x45\x53\x2c\x41\x55\x54\x4f\x53\x45\x4c\x45\x43\x54\x3d\x59\x45\x53\x2c\x4e\x41\x4d\x45\x3d\x22\x44\x65\x66\x61\x75\x6c\x74\x22\x0a\x23\x45\x58\x54\x2d\x58\x2d\x53\x54\x52\x45\x41\x4d\x2d\x49\x4e\x46\x3a\x42\x41\x4e\x44\x57\x49\x44\x54\x48\x3d\x31\x33\x35\x32\x37\x35\x39\x2c\x43\x4f\x44\x45\x43\x53\x3d\x22\x61\x76\x63\x31\x2e\x34\x64\x34\x30\x31\x65\x2c\x6d\x70\x34\x61\x2e\x34\x30\x2e\x32\x22\x2c\x52\x45\x53\x4f\x4c\x55\x54\x49\x4f\x4e\x3d\x38\x35\x34\x78\x34\x38\x30\x2c\x41\x55\x44\x49\x4f\x3d\x22\x32\x33\x34\x22\x2c\x46\x52\x41\x4d\x45\x2d\x52\x41\x54\x45\x3d\x32\x35\x2c\x43\x4c\x4f\x53\x45\x44\x2d\x43\x41\x50\x54\x49\x4f\x4e\x53\x3d\x4e\x4f\x4e\x45\x0a\x6d\x6c\x68\x6c\x73\x3a\x2f\x2f\x6c\x6f\x63\x61\x6c\x68\x6f\x73\x74\x2f\x69\x74\x61\x67\x2f\x32\x33\x31\x2f\x6d\x65\x64\x69\x61\x64\x61\x74\x61\x2e\x6d\x33\x75\x38\x0a\x23\x45\x58\x54\x2d\x58\x2d\x53\x54\x52\x45\x41\x4d\x2d\x49\x4e\x46\x3a\x42\x41\x4e\x44\x57\x49\x44\x54\x48\x3d\x33\x32\x33\x38\x34\x37\x2c\x43\x4f\x44\x45\x43\x53\x3d\x22\x61\x76\x63\x31\x2e\x34\x64\x34\x30\x31\x35\x2c\x6d\x70\x34\x61\x2e\x34\x30\x2e\x35\x22\x2c\x52\x45\x53\x4f\x4c\x55\x54\x49\x4f\x4e\x3d\x34\x32\x36\x78\x32\x34\x30\x2c\x41\x55\x44\x49\x4f\x3d\x22\x32\x33\x33\x22\x2c\x46\x52\x41\x4d\x45\x2d\x52\x41\x54\x45\x3d\x32\x35\x2c\x43\x4c\x4f\x53\x45\x44\x2d\x43\x41\x50\x54\x49\x4f\x4e\x53\x3d\x4e\x4f\x4e\x45\x0a\x6d\x6c\x68\x6c\x73\x3a\x2f\x2f\x6c\x6f\x63\x61\x6c\x68\x6f\x73\x74\x2f\x69\x74\x61\x67\x2f\x32\x32\x39\x2f\x6d\x65\x64\x69\x61\x64\x61\x74\x61\x2e\x6d\x33\x75\x38\x0a\x23\x45\x58\x54\x2d\x58\x2d\x53\x54\x52\x45\x41\x4d\x2d\x49\x4e\x46\x3a\x42\x41\x4e\x44\x57\x49\x44\x54\x48\x3d\x34\x30\x38\x35\x35\x30\x2c\x43\x4f\x44\x45\x43\x53\x3d\x22\x61\x76\x63\x31\x2e\x34\x64\x34\x30\x31\x35\x2c\x6d\x70\x34\x61\x2e\x34\x30\x2e\x32\x22\x2c\x52\x45\x53\x4f\x4c\x55\x54\x49\x4f\x4e\x3d\x34\x32\x36\x78\x32\x34\x30\x2c\x41\x55\x44\x49\x4f\x3d\x22\x32\x33\x34\x22\x2c\x46\x52\x41\x4d\x45\x2d\x52\x41\x54\x45\x3d\x32\x35\x2c\x43\x4c\x4f\x53\x45\x44\x2d\x43\x41\x50\x54\x49\x4f\x4e\x53\x3d\x4e\x4f\x4e\x45\x0a\x6d\x6c\x68\x6c\x73\x3a\x2f\x2f\x6c\x6f\x63\x61\x6c\x68\x6f\x73\x74\x2f\x69\x74\x61\x67\x2f\x32\x32\x39\x2f\x6d\x65\x64\x69\x61\x64\x61\x74\x61\x2e\x6d\x33\x75\x38\x0a\x23\x45\x58\x54\x2d\x58\x2d\x53\x54\x52\x45\x41\x4d\x2d\x49\x4e\x46\x3a\x42\x41\x4e\x44\x57\x49\x44\x54\x48\x3d\x38\x30\x32\x32\x31\x30\x2c\x43\x4f\x44\x45\x43\x53\x3d\x22\x61\x76\x63\x31\x2e\x34\x64\x34\x30\x31\x65\x2c\x6d\x70\x34\x61\x2e\x34\x30\x2e\x32\x22\x2c\x52\x45\x53\x4f\x4c\x55\x54\x49\x4f\x4e\x3d\x36\x34\x30\x78\x33\x36\x30\x2c\x41\x55\x44\x49\x4f\x3d\x22\x32\x33\x34\x22\x2c\x46\x52\x41\x4d\x45\x2d\x52\x41\x54\x45\x3d\x32\x35\x2c\x43\x4c\x4f\x53\x45\x44\x2d\x43\x41\x50\x54\x49\x4f\x4e\x53\x3d\x4e\x4f\x4e\x45\x0a\x6d\x6c\x68\x6c\x73\x3a\x2f\x2f\x6c\x6f\x63\x61\x6c\x68\x6f\x73\x74\x2f\x69\x74\x61\x67\x2f\x32\x33\x30\x2f\x6d\x65\x64\x69\x61\x64\x61\x74\x61\x2e\x6d\x33\x75\x38\x0a\x23\x45\x58\x54\x2d\x58\x2d\x53\x54\x52\x45\x41\x4d\x2d\x49\x4e\x46\x3a\x42\x41\x4e\x44\x57\x49\x44\x54\x48\x3d\x32\x35\x36\x36\x38\x31\x34\x2c\x43\x4f\x44\x45\x43\x53\x3d\x22\x61\x76\x63\x31\x2e\x34\x64\x34\x30\x31\x66\xf0\x18\x98\x75\x4a\x84\x94\xd9\xb3\x7c\x30\x6c\x08\x00\x45\x00\x01\x95\x00\x00\x40\x00\x40\x06\xb7\x68\xc0\xa8\x00\x43\xc0\xa8\x00\x67\xda\x54\xcc\x14\x5e\x14\x69\x9c\x65\xb4\xe4\x8c\x80\x18\x08\x00\x3e\xf0\x00\x00\x01\x01\x08\x0a\x41\x53\xef\xec\x00\x00\x97\xdd\x2c\x6d\x70\x34\x61\x2e\x34\x30\x2e\x32\x22\x2c\x52\x45\x53\x4f\x4c\x55\x54\x49\x4f\x4e\x3d\x31\x32\x38\x30\x78\x37\x32\x30\x2c\x41\x55\x44\x49\x4f\x3d\x22\x32\x33\x34\x22\x2c\x46\x52\x41\x4d\x45\x2d\x52\x41\x54\x45\x3d\x32\x35\x2c\x43\x4c\x4f\x53\x45\x44\x2d\x43\x41\x50\x54\x49\x4f\x4e\x53\x3d\x4e\x4f\x4e\x45\x0a\x6d\x6c\x68\x6c\x73\x3a\x2f\x2f\x6c\x6f\x63\x61\x6c\x68\x6f\x73\x74\x2f\x69\x74\x61\x67\x2f\x32\x33\x32\x2f\x6d\x65\x64\x69\x61\x64\x61\x74\x61\x2e\x6d\x33\x75\x38\x0a\x23\x45\x58\x54\x2d\x58\x2d\x53\x54\x52\x45\x41\x4d\x2d\x49\x4e\x46\x3a\x42\x41\x4e\x44\x57\x49\x44\x54\x48\x3d\x31\x37\x39\x33\x39\x38\x2c\x43\x4f\x44\x45\x43\x53\x3d\x22\x61\x76\x63\x31\x2e\x34\x64\x34\x30\x30\x63\x2c\x6d\x70\x34\x61\x2e\x34\x30\x2e\x35\x22\x2c\x52\x45\x53\x4f\x4c\x55\x54\x49\x4f\x4e\x3d\x32\x35\x36\x78\x31\x34\x34\x2c\x41\x55\x44\x49\x4f\x3d\x22\x32\x33\x33\x22\x2c\x46\x52\x41\x4d\x45\x2d\x52\x41\x54\x45\x3d\x32\x35\x2c\x43\x4c\x4f\x53\x45\x44\x2d\x43\x41\x50\x54\x49\x4f\x4e\x53\x3d\x4e\x4f\x4e\x45\x0a\x6d\x6c\x68\x6c\x73\x3a\x2f\x2f\x6c\x6f\x63\x61\x6c\x68\x6f\x73\x74\x2f\x69\x74\x61\x67\x2f\x32\x36\x39\x2f\x6d\x65\x64\x69\x61\x64\x61\x74\x61\x2e\x6d\x33\x75\x38\x0a\x10\x00\x00\x08\x00\x0d\x00\x12\x00\x19\x00\x30\x00\x39\x00\x4d\x00\x62\x00\x7c\x00\x97\x00\xb7\x05\xf8\x00\x00\x00\x00\x00\x00\x02\x01\x00\x00\x00\x00\x00\x00\x00\x0c\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x05\xfa";

				plist_t dict = NULL;
				plist_from_bin(bodyChr, datalen/*m_httpParser->getContentLength()*/, &dict);

				char *xml = NULL;
				uint32_t size;
				plist_to_xml(dict, &xml, &size);
				if (plist_dict_get_size(dict))
				{
					plist_t tmpNode = plist_dict_get_item(dict, "params");
					if (tmpNode)
					{
						plist_t tmpNodeurl = plist_dict_get_item(tmpNode, "FCUP_Response_URL");
						char* urlnew = NULL;
						plist_get_string_val(tmpNodeurl, &urlnew);

						tmpNode = plist_dict_get_item(tmpNode, "FCUP_Response_Data");
						char* buf = NULL;
						uint64_t len = 0;
						plist_get_data_val(tmpNode, &buf, &len);
						std::string tmpstr = "";
						tmpstr.assign(buf, (size_t)len);
						int pos = -1;
						if ((pos = tmpstr.find("mlhls:")) != -1)
						{
							//
							//get max RESOLUTION
							std::string strresolution = GetMaxResulution(tmpstr);
							if ((pos = tmpstr.find(strresolution)) != -1)
							{
								tmpstr = tmpstr.substr(pos);
								tmpstr = tmpstr.substr(tmpstr.find("mlhls:"));
								if (tmpstr.find("\n") != -1)
								{
									tmpstr = tmpstr.substr(0, tmpstr.find("\n"));
								}
								else if (tmpstr.find("\r\n") != -1)
								{
									tmpstr = tmpstr.substr(0, tmpstr.find("\r\n") - 1);
								}
							}
							{
								WXAutoLock lock(s_mutextvlc);
								g_IDToUrls[uniqueid] = tmpstr;
							}
							eventStrings[4] = (char*)m_sessionId.c_str();
							eventStrings[5] = (char*)"mlhls://localhost/itag/234/mediadata.m3u8";// (char*)tmpstr.c_str();
							ServerInstance->AnnounceToClients(EVENT_YOUTUBE_1, m_sessionId);
						}
						else if ((pos = tmpstr.find("#YT-EXT-CONDENSED-URL:BASE-URI=")) != -1)
						{
							tmpstr = tmpstr.substr(pos + strlen("#YT-EXT-CONDENSED-URL:BASE-URI=") + 1);
							tmpstr = tmpstr.substr(0, tmpstr.find(",PARAMS") - 1);
							//
							//ServerInstance->AnnounceToClients(EVENT_LOADING, m_sessionId);
							//CAirPlayServer::SetAirplayPlaying(uniqueid, 1);

							//std::string strname = "tmp.m3u8";
							//DownLoad(tmpstr.c_str(), strname.c_str());


							//std::string strname = "tmp.m3u8";
							//FILE* fp = fopen(strname.c_str(), "wb");
							//fwrite(tmpstr.c_str(), tmpstr.size(), 1, fp);
							//fclose(fp);

							std::string strurl = "";

							{
								WXAutoLock lock(s_mutextvlc);
								if (!g_IDToUrls.empty() && g_IDToUrls.find(uniqueid) != g_IDToUrls.end())
								{
									strurl = g_IDToUrls[uniqueid];
								}
								//g_IDToAVUrls
								if (g_IDToAVUrls.find(uniqueid) == g_IDToAVUrls.end())
								{
									YoutubeURL urls;
									urls.strAudio = tmpstr;
									g_IDToAVUrls[uniqueid] = urls;
								}
								else 
								{
									g_IDToAVUrls[uniqueid].strVideo = tmpstr;
								}
							}

							eventStrings[4] = (char*)m_sessionId.c_str();
							eventStrings[5] = (char*)strurl.c_str();
							ServerInstance->AnnounceToClients(EVENT_YOUTUBE_2, m_sessionId);

							if (g_IDToAVUrls[uniqueid].strAudio != "" && g_IDToAVUrls[uniqueid].strVideo != "")
							{
								CAirPlayServer::SetAirplayPlaying(uniqueid, 1);
								CAirPlayServer::CTCPClient::AirPlayOutputFunctions::airplay_open(this, (char*)g_IDToAVUrls[uniqueid].strVideo.c_str(), (char*)g_IDToAVUrls[uniqueid].strAudio.c_str(), 0, uniqueid);
							}
							//ServerInstance->AnnounceToClients(EVENT_PLAYING, m_sessionId);
						}
						tmpNode = NULL;
					}
				}

			}
		}
	}
	else if (uri == "/play")
	{
		std::string location;

		float position = 0.0;
		bool startPlayback = true;
		m_lastEvent = EVENT_NONE;
		isEndReached = false;

		if (needAuth && !checkAuthorization(authorization, method, uri))
		{
			status = AIRPLAY_STATUS_NEED_AUTH;
		}
		else if (contentType == "application/x-apple-binary-plist")
		{
			CAirPlayServer::m_isPlaying++;

			{

				const char* bodyChr = body.c_str();// m_httpParser->getBody();

				plist_t dict = NULL;
				plist_from_bin(bodyChr, datalen/*m_httpParser->getContentLength()*/, &dict);

				char *xml = NULL;
				uint32_t size;
				plist_to_xml(dict, &xml, &size);

				if (plist_dict_get_size(dict))
				{
					plist_t tmpNode = plist_dict_get_item(dict, "Start-Position");
					if (tmpNode)
					{
						double tmpDouble = 0;
						plist_get_real_val(tmpNode, &tmpDouble);
						position = (float)tmpDouble;
					}

					tmpNode = plist_dict_get_item(dict, "Content-Location");
					if (tmpNode)
					{
						location = getStringFromPlist(tmpNode);
						tmpNode = NULL;
					}

					tmpNode = plist_dict_get_item(dict, "rate");
					if (tmpNode)
					{
						double rate = 0;
						plist_get_real_val(tmpNode, &rate);
						if (rate == 0.0)
						{
							startPlayback = false;
						}
						tmpNode = NULL;
					}

					// in newer protocol versions the location is given
					// via host and path where host is ip:port and path is /path/file.mov
					//http://iPhone.local:7001/1/62babafa-2b9e-5fb8-a060-89641ee44b28.mp4
					std::string tmploc = location;
					int iPortPos = location.rfind(":");
					int iIpPos = location.find("http://");
					if (location.find(".local") != -1 && iPortPos != -1 && iIpPos != -1)
					{
						location = "http://";
						location += m_strAddr + tmploc.substr(iPortPos);
					}
					else
					{
						if (location.empty())
							tmpNode = plist_dict_get_item(dict, "host");
						if (tmpNode)
						{
							location = "http://";
							location += getStringFromPlist( tmpNode);

							tmpNode = plist_dict_get_item(dict, "path");
							if (tmpNode)
							{
								location += getStringFromPlist(tmpNode);
							}
						}
					}

					if (dict)
					{
						plist_free(dict);
					}
				}
			}
		}
		else
		{
			CAirPlayServer::m_isPlaying++;
			// Get URL to play
			std::string contentLocation = "Content-Location: ";
			size_t start = body.find(contentLocation);
			if (start == std::string::npos)
				return AIRPLAY_STATUS_NOT_IMPLEMENTED;
			start += contentLocation.size();
			int end = body.find('\n', start);
			location = body.substr(start, end - start);

			std::string startPosition = "Start-Position: ";
			start = body.find(startPosition);
			if (start != std::string::npos)
			{
				start += startPosition.size();
				int end = body.find('\n', start);
				std::string positionStr = body.substr(start, end - start);
				position = (float)atof(positionStr.c_str());
			}
		}

		if (status != AIRPLAY_STATUS_NEED_AUTH)
		{
			std::string userAgent(CURL::Encode("AppleCoreMedia/1.0.0.8F455 (AppleTV; U; CPU OS 4_3 like Mac OS X; de_de)"));
			//location += "|User-Agent=" + userAgent;
			m_isPaused = 0;
			m_isPaused2 = 0;
			//if (g_iStopPlay != 1)
			{
				//CAirPlayServer::CTCPClient::AirPlayOutputFunctions::airplay_setlastpostion(this, 0, uniqueid);

				ConnectStatusStruct stConnect;
				stConnect.eConnectStatus = STARTING;
				stConnect.iConnectType = 0;
				uint64_t uniqueid = 0;
				memcpy(&uniqueid, m_strAddrReal.c_str(), 4);
				if (CNetworkServices::Get().m_stAirplay.m_CallBackConnectInfo != NULL)
				{
					CNetworkServices::Get().m_stAirplay.m_CallBackConnectInfo(stConnect, uniqueid);
				}

				WXLogWriteNew("airplay start");

				if (location.find("mlhls:") != -1) 
				{
					{
						WXAutoLock lock(s_mutextvlc);
						if (g_IDToUrls.find(uniqueid) != g_IDToUrls.end())
						{
							g_IDToUrls.erase(uniqueid);
						}
						//g_IDToAVUrls
						if (g_IDToAVUrls.find(uniqueid) != g_IDToAVUrls.end())
						{
							g_IDToAVUrls.erase(uniqueid);
						}
					}

					eventStrings[4] = (char*)m_sessionId.c_str();
					ServerInstance->AnnounceToClients(EVENT_YOUTUBE, m_sessionId);

					//g_mapYoutube[uniqueid] = true;
				}
				else 
				{
					ServerInstance->AnnounceToClients(EVENT_LOADING, m_sessionId);
					CAirPlayServer::SetAirplayPlaying(uniqueid, 1);//start playing
																   //HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, MyMediaPlayNew, pstObj, 0, NULL);
																   //CloseHandle(hThread);
																   //
					CAirPlayServer::CTCPClient::AirPlayOutputFunctions::airplay_open(this, (char*)location.c_str(), (char*)"", position, uniqueid);

					//push into map
					/*WXAutoLock lock(g_mutextvlc);
					s_mapPlayer[uniqueid] = this;*/

					//send playing message
					//send TMSG_MEDIA_PLAY
					//PutMessage(TMSG_MEDIA_PLAY);
					ServerInstance->AnnounceToClients(EVENT_PLAYING, m_sessionId);

					//g_mapYoutube[uniqueid] = false;
				}
				
			}

			if (!startPlayback)
			{
				//send paused message
				//send TMSG_MEDIA_PAUSE

				PutMessage(TMSG_MEDIA_PAUSE);
				m_isPaused2 = 1;

				//g_messagequeue.push(TMSG_MEDIA_PAUSE);
				//ServerInstance->AnnounceToClients(EVENT_PAUSED);
				//g_dvdplayer->SeekPercentage(position * 100.0f);
			}
		}
	}

	// Used to perform seeking (POST request) and to retrieve current player position (GET request).
	// GET scrub seems to also set rate 1 - strange but true
	else if (uri == "/scrub")
	{
		if (needAuth && !checkAuthorization(authorization, method, uri))
		{
			status = AIRPLAY_STATUS_NEED_AUTH;
		}
		else if (method == "GET")
		{
			//
			//if (/*g_application.m_pPlayer->GetTotalTime()*/GetTotalTime() > 0)
			if (CAirPlayServer::CTCPClient::AirPlayOutputFunctions::airplay_getduration(this, uniqueid))
			{
				float position = (float)CAirPlayServer::CTCPClient::AirPlayOutputFunctions::airplay_getpostion(this, uniqueid);// GetCurTime()/* / 1000*/;
				responseBody = StringUtils::Format("duration: %.6f\r\nposition: %.6f\r\n",
					CAirPlayServer::CTCPClient::AirPlayOutputFunctions::airplay_getduration(this, uniqueid)/*GetTotalTime()*//* / 1000*/, position);
			}
			else
			{
				status = AIRPLAY_STATUS_METHOD_NOT_ALLOWED;
			}
		}
		else
		{
			const char* found = strstr(queryString.c_str(), "position=");
			if (found /*&& g_application.m_pPlayer->HasPlayer()*/)
			{
				int64_t position = (int64_t)(atof(found + strlen("position=")) * 1000.0);
				//SetCurTime(position);
				//g_application.m_pPlayer->SeekTime(position);
				CAirPlayServer::CTCPClient::AirPlayOutputFunctions::airplay_seek(this, (long)position, uniqueid);
			}
		}
	}

	// Sent when media playback should be stopped
	else if (uri == "/stop")
	{
		WXLogWriteNew("airplay url stop");

		;//;//CLog::Log(LOGDEBUG, "AIRPLAY: got request %s", uri.c_str());
		if (needAuth && !checkAuthorization(authorization, method, uri))
		{
			status = AIRPLAY_STATUS_NEED_AUTH;
		}
		else
		{
			//ConnectStatusStruct stConnect;
			//stConnect.eConnectStatus = DISCONNECT;
			//stConnect.iConnectType = 0;
			//uint64_t uniqueid = 0;
			//memcpy(&uniqueid, m_strAddrReal.c_str(), 4);
			//if (CNetworkServices::Get().m_stAirplay.m_CallBackConnectInfo != NULL)
			//{
			//	CNetworkServices::Get().m_stAirplay.m_CallBackConnectInfo(stConnect, uniqueid);
			//}
			////if (IsPlaying()) //only stop player if we started him
			//if (CAirPlayServer::CTCPClient::AirPlayOutputFunctions::airplay_isplaying(this, uniqueid) != 0)
			{
				WXLogWriteNew("airplay stop");
				CVariant data;
				CAirPlayServer::CTCPClient::AirPlayOutputFunctions::airplay_stop(this, uniqueid);
				
				ServerInstance->AnnounceToClients(EVENT_STOPPED, m_sessionId);
				CAirPlayServer::SetAirplayPlaying(uniqueid, 0);
			}
			//else //if we are not playing and get the stop request - we just wanna stop picture streaming
			//{
			//	//CApplicationMessenger::Get().SendAction(ACTION_PREVIOUS_MENU);
			//	WriteLog("airplay photo stop");
			//	ServerInstance->AnnounceToClients(EVENT_STOPPED, m_sessionId);
			//}
		}
		//ClearPhotoAssetCache();
	}
	//手动断开镜像连接
	else if (uri == "/stop2")
	{
		WXLogWriteNew("airplay url stop2 AAA");
		;//;//CLog::Log(LOGDEBUG, "AIRPLAY: got request %s", uri.c_str());
		if (needAuth && !checkAuthorization(authorization, method, uri))
		{
			WXLogWriteNew("airplay url stop2 checkAuthorization BBBB");
			status = AIRPLAY_STATUS_NEED_AUTH;
		}
		else
		{
			ConnectStatusStruct stConnect;
			stConnect.eConnectStatus = DISCONNECT;
			stConnect.iConnectType = 0;
			uint64_t uniqueid = 0;
			memcpy(&uniqueid, m_strAddrReal.c_str(), 4);
			if (CNetworkServices::Get().m_stAirplay.m_CallBackConnectInfo != NULL)
			{
				CNetworkServices::Get().m_stAirplay.m_CallBackConnectInfo(stConnect, uniqueid);
			}
			WXLogWriteNew("airplay url stop2 m_CallBackConnectInfo BBBB");
			CAirPlayServer::CTCPClient::AirPlayOutputFunctions::airplay_stop(this, uniqueid);
			WXLogWriteNew("airplay url stop2 airplay_stop CCC");
		}
		//ClearPhotoAssetCache();
		WXLogWriteNew("airplay url stop2 airplay_stop DDD");
	}
	// RAW JPEG data is contained in the request body
	else if (uri == "/photo")
	{
		;//;//CLog::Log(LOGDEBUG, "AIRPLAY: got request %s", uri.c_str());
		if (needAuth && !checkAuthorization(authorization, method, uri))
		{
			status = AIRPLAY_STATUS_NEED_AUTH;
		}
		else if (contentlength > 0 || photoAction == "displayCached")
		{
			XFILE::CFile m_tmpFile;
			std::string m_tmpFileName = "special://temp/airplayasset";
			bool showPhoto = true;
			bool receivePhoto = true;

			if (photoAction == "cacheOnly")
				showPhoto = false;
			else if (photoAction == "displayCached")
			{
				receivePhoto = false;
				if (photoCacheId.length())
					;//;//CLog::Log(LOGDEBUG, "AIRPLAY: Trying to show from cache asset: %s", photoCacheId.c_str());
			}

			if (photoCacheId.length())
				m_tmpFileName += photoCacheId;
			else
				m_tmpFileName += "airplay_photo";

			if (receivePhoto && contentlength > 3 &&
				data[1] == 'P' &&
				data[2] == 'N' &&
				data[3] == 'G')
			{
				m_tmpFileName += ".png";
			}
			else
			{
				m_tmpFileName += ".jpg";
			}

			int writtenBytes = 0;
			if (receivePhoto)
			{
				if (m_tmpFile.OpenForWrite(m_tmpFileName, true))
				{
					writtenBytes = m_tmpFile.Write(body.c_str(), contentlength);
					m_tmpFile.Close();
				}
				if (photoCacheId.length())
					;//;//CLog::Log(LOGDEBUG, "AIRPLAY: Cached asset: %s", photoCacheId.c_str());
			}

			if (showPhoto)
			{
				ConnectStatusStruct stConnect;
				stConnect.eConnectStatus = STARTING;
				stConnect.iConnectType = 0;
				uint64_t uniqueid = 0;
				memcpy(&uniqueid, m_strAddrReal.c_str(), 4);
				if (CNetworkServices::Get().m_stAirplay.m_CallBackConnectInfo != NULL)
				{
					CNetworkServices::Get().m_stAirplay.m_CallBackConnectInfo(stConnect, uniqueid);
				}

				if ((writtenBytes > 0 && (unsigned int)writtenBytes == contentlength) || !receivePhoto)
				{
					if (!receivePhoto && !XFILE::CFile::Exists(m_tmpFileName))
					{
						status = AIRPLAY_STATUS_PRECONDITION_FAILED; //image not found in the cache
					}
					else
					{
						SDL_Window *window = GetCurrentWindow(m_strAddrReal.c_str(), false);// GetParentWindow();
						int iScreenW = 480;
						int iScreenH = 854;
						SDL_Renderer *render = GetCurSDLRender(window, m_strAddrReal.c_str()); //GetSDLRender(window);
						m_tmpFileName = m_strTmpDir + "/" + m_tmpFileName.substr(m_tmpFileName.rfind("/") + 1);
						SDL_Texture *image = IMG_LoadTexture(render, m_tmpFileName.c_str());
						int bW, bH;
						SDL_QueryTexture(image, NULL, NULL, &bW, &bH);
						iScreenW = bW;
						iScreenH = bH;
						SDL_SetWindowSize(window, iScreenW, iScreenH);
						//report 
						int eWindowsStatus = 0;//default 竖屏
						if (iScreenW > iScreenH)
						{
							eWindowsStatus = 1;
						}
						ReportPlayWH(eWindowsStatus, iScreenW, iScreenH, m_strAddrReal.c_str());
						int iW = iScreenW, iH = iScreenH;
						int x = 0;// 406 / 2 - iW / 2;
						int y = 0;// 720 / 2 - iH / 2;
						SDL_Rect clips[4];
						int i;
						int useClip = 0;
						for (i = 0; i < 4; i++) {
							clips[i].x = i / 2 * iW;
							clips[i].y = i % 2 * iH;
							clips[i].w = iW;
							clips[i].h = iH;
						}
						//get lock
						LockSDLCommon();
						renderTexture2(image, render, x, y, &clips[useClip]);
						SDL_RenderPresent(render);
						IMG_Quit();
						//
						UnlockSDLCommon();
						SDL_PollEvent(NULL);
					}
				}
			}
		}
	}

	else if (uri == "/playback-info")
	{
		float position = 0.0f;
		float duration = 0.0f;
		float cachePosition = 0.0f;
		bool playing = false;

		;//;//CLog::Log(LOGDEBUG, "AIRPLAY: got request %s", uri.c_str());

		if (needAuth && !checkAuthorization(authorization, method, uri))
		{
			status = AIRPLAY_STATUS_NEED_AUTH;
		}
		else if (/*g_application.m_pPlayer->HasPlayer()*/CAirPlayServer::m_isPlaying > 0)
		{
			long curPos = CAirPlayServer::CTCPClient::AirPlayOutputFunctions::airplay_getpostion(this, uniqueid);
			long lastPos = CAirPlayServer::CTCPClient::AirPlayOutputFunctions::airplay_getlastpostion(this, uniqueid);
			CAirPlayServer::CTCPClient::AirPlayOutputFunctions::airplay_setlastpostion(this, curPos, uniqueid);
			if (isEndReached && curPos > 0 && lastPos > 0 && curPos == lastPos)
			{
				WXLogWriteNew("Playing got same last posion, consider as stopped");
				ConnectStatusStruct stConnect;
				stConnect.eConnectStatus = DISCONNECT;
				stConnect.iConnectType = 0;
				uint64_t uniqueid = 0;
				memcpy(&uniqueid, m_strAddrReal.c_str(), 4);
				if (CNetworkServices::Get().m_stAirplay.m_CallBackConnectInfo != NULL)
				{
					CNetworkServices::Get().m_stAirplay.m_CallBackConnectInfo(stConnect, uniqueid);
				}
				//if (IsPlaying()) //only stop player if we started him
				if (CAirPlayServer::CTCPClient::AirPlayOutputFunctions::airplay_isplaying(this, uniqueid) != 0)
				{
					WXLogWriteNew("airplay stop");
					CVariant data;
					CAirPlayServer::CTCPClient::AirPlayOutputFunctions::airplay_stop(this, uniqueid);
					ServerInstance->AnnounceToClients(EVENT_STOPPED, m_sessionId);
					CAirPlayServer::SetAirplayPlaying(uniqueid, 0);
				}
				else //if we are not playing and get the stop request - we just wanna stop picture streaming
				{
					WXLogWriteNew("airplay photo stop");
					ServerInstance->AnnounceToClients(EVENT_STOPPED, m_sessionId);
				}
			}
			else {

				if (CAirPlayServer::CTCPClient::AirPlayOutputFunctions::airplay_getduration(this, uniqueid))
				{

					//position = ((float)CAirPlayServer::CTCPClient::AirPlayOutputFunctions::airplay_getpostion(this, uniqueid)) / 1000;
					duration = ((float)CAirPlayServer::CTCPClient::AirPlayOutputFunctions::airplay_getduration(this, uniqueid)) / 1000;
					playing = !CAirPlayServer::CTCPClient::AirPlayOutputFunctions::airplay_ispaused(this, uniqueid);
					//cachePosition = position + (duration * g_application.m_pPlayer->GetCachePercentage() / 100.0f);
				}

				position = ((float)CAirPlayServer::CTCPClient::AirPlayOutputFunctions::airplay_getpostion(this, uniqueid)) / 1000;

				//if (GetTotalTime())
				//{
				//	position = ((float)GetCurTime()) / 1000;
				//	duration = ((float)GetTotalTime());
				//	playing = !m_isPaused;
				//	//cachePosition = position + (duration * g_application.m_pPlayer->GetCachePercentage() / 100.0f);
				//}

				if (duration == 0) {
					duration = position;
				}

				responseBody = StringUtils::Format(PLAYBACK_INFO, duration, cachePosition, position, (playing ? 1 : 0), duration);
				responseHeader = "Content-Type: text/x-apple-plist+xml\r\n";

				//if (g_application.m_pPlayer->IsCaching())
				//{
				//  CAirPlayServer::ServerInstance->AnnounceToClients(EVENT_LOADING);
				//}
			}
		}
		else
		{
			responseBody = StringUtils::Format(PLAYBACK_INFO_NOT_READY);
			responseHeader = "Content-Type: text/x-apple-plist+xml\r\n";
		}
	}
	else if (uri == "/server-info")
	{
		;//;//CLog::Log(LOGDEBUG, "AIRPLAY: got request %s", uri.c_str());
		CNetworkInterface* iface = g_network->GetFirstConnectedInterface();
		std::string address = "";
		if (iface)
		{
			address = iface->GetMacAddress();
		}
		else
		{
			if (CNetworkServices::m_strVMNetMac != "")
			{
				address = CNetworkServices::m_strVMNetMac;
			}
		}
		responseBody = StringUtils::Format(SERVER_INFO, address.c_str());
		responseHeader = "Content-Type: text/x-apple-plist+xml\r\n";
	}

	else if (uri == "/slideshow-features")
	{
		// Ignore for now.
	}

	else if (uri == "/authorize")
	{
		// DRM, ignore for now.
	}

	else if (uri == "/setProperty")
	{
		status = AIRPLAY_STATUS_NOT_FOUND;
	}

	else if (uri == "/getProperty")
	{
		status = AIRPLAY_STATUS_NOT_FOUND;
	}
	else
	{
		;//;//CLog::Log(LOGERROR, "AIRPLAY Server: unhandled request [%s]\n", uri.c_str());
		status = AIRPLAY_STATUS_NOT_IMPLEMENTED;
	}
	return status;
}

//
int CAirPlayServer::CTCPClient::ProcessRequest( std::string& responseHeader,
                                                std::string& responseBody)
{
	return 0;
}


#include <WXBase.h>

EXTERN_C static void WINAPI CBAVFrame(void* ctx, struct AVFrame* frame) {
	uint64_t uniqueid = (uint64_t)ctx;
	WXAirplayVideoRenderDisplay(uniqueid, frame);
}



void CAirPlayServer::CTCPClient::AirPlayOutputFunctions::airplay_open(void *cls, char *url, char *secondurl, float fPosition, uint64_t uniqueid)
{
	WXAutoLock lock(s_mapLockPlayer[uniqueid]);

	WXString strURL;
	strURL.Format("%s",url);
	WXString strURL2;
	strURL2.Format("%s", secondurl);

	if (s_mapPlayer.find(uniqueid) != s_mapPlayer.end())
	{
		WXPlayerDestroy(s_mapPlayer[uniqueid]);
		s_mapPlayer[uniqueid] = nullptr;
		s_mapPlayer.erase(uniqueid);
	}

	void* pPlay = WXPlayerCreate(L"Lav", strURL.str(), 100, 0);
	if (pPlay && strURL2.length() > 0) {
		WXPlayerAttachAudio(pPlay, strURL2.str(), 0);
	}
	long totalTime = (long)WXPlayerGetTotalTime(pPlay)/* * 1000*/;
	if (totalTime == 0) {
		WXPlayerDestroy(pPlay);
	}
	s_mapPlayer[uniqueid] = pPlay;
	WXPlayerStart(pPlay);
	WXPlayerSeek(pPlay, (int)(fPosition * totalTime));
}
//
void CAirPlayServer::CTCPClient::AirPlayOutputFunctions::airplay_play(void *cls, uint64_t uniqueid)
{
	WXAutoLock lock(s_mapLockPlayer[uniqueid]);
	if (s_mapPlayer.find(uniqueid) != s_mapPlayer.end()) {
		WXPlayerResume(s_mapPlayer[uniqueid]);
	}
}
//
void CAirPlayServer::CTCPClient::AirPlayOutputFunctions::airplay_pause(void *cls, uint64_t uniqueid)
{
	WXAutoLock lock(s_mapLockPlayer[uniqueid]);
	if (s_mapPlayer.find(uniqueid) != s_mapPlayer.end()) {
		WXPlayerPause(s_mapPlayer[uniqueid]);
	}
}
//
void CAirPlayServer::CTCPClient::AirPlayOutputFunctions::airplay_stop(void *cls, uint64_t uniqueid)
{
	WXAutoLock lock(s_mapLockPlayer[uniqueid]);
	if (s_mapPlayer.find(uniqueid) != s_mapPlayer.end()) {
		WXPlayerStop(s_mapPlayer[uniqueid]);
		WXPlayerDestroy(s_mapPlayer[uniqueid]);
		s_mapPlayer.erase(uniqueid);
		WXLogWriteNew("Airplay Stop WXAirplayVideoRenderDestroy Start !!!!!!!!!!!  ");
		WXAirplayVideoRenderDestroy(uniqueid);//销毁视频显示对象
		WXLogWriteNew("Airplay Stop WXAirplayVideoRenderDestroy Stop !!!!!!!!!!!  ");
	}
}
//
void CAirPlayServer::CTCPClient::AirPlayOutputFunctions::airplay_seek(void *cls, long fPosition, uint64_t uniqueid)
{
	WXAutoLock lock(s_mapLockPlayer[uniqueid]);
	if (s_mapPlayer.find(uniqueid) != s_mapPlayer.end()) {
		WXPlayerSeek(s_mapPlayer[uniqueid], fPosition / 1000);
	}
}
//
void CAirPlayServer::CTCPClient::AirPlayOutputFunctions::airplay_setvolume(void *cls, int volume, uint64_t uniqueid)
{
}
void CAirPlayServer::CTCPClient::AirPlayOutputFunctions::airplay_showphoto(void *cls, uint8_t *data, long long size, uint64_t uniqueid)
{
}
//
long CAirPlayServer::CTCPClient::AirPlayOutputFunctions::airplay_getduration(void *cls, uint64_t uniqueid)
{
	WXAutoLock lock(s_mapLockPlayer[uniqueid]);
	if (s_mapPlayer.find(uniqueid) != s_mapPlayer.end()) {
		int64_t time1 = WXPlayerGetTotalTime(s_mapPlayer[uniqueid]);

		int64_t time2 = WXPlayerGetTotalTime(s_mapPlayer[uniqueid]);//音频时长
		return time1 > 0 ? (long)time1 : (long)time2;
	}
	return 0;
}
//
long CAirPlayServer::CTCPClient::AirPlayOutputFunctions::airplay_getpostion(void *cls, uint64_t uniqueid)
{
	WXAutoLock lock(s_mapLockPlayer[uniqueid]);
	if (s_mapPlayer.find(uniqueid) != s_mapPlayer.end()) {
		long ltime = 0L;
		ltime = (long)WXPlayerGetCurrTime(s_mapPlayer[uniqueid]);//改s为单位了
		ltime = ltime;
		return ltime;
	}
	
	return 0;
}
//
void CAirPlayServer::CTCPClient::AirPlayOutputFunctions::airplay_setlastpostion(void *cls, long last, uint64_t uniqueid)
{
	
}
//
long CAirPlayServer::CTCPClient::AirPlayOutputFunctions::airplay_getlastpostion(void *cls, uint64_t uniqueid)
{
	return 0;
}
//
int CAirPlayServer::CTCPClient::AirPlayOutputFunctions::airplay_isplaying(void *cls, uint64_t uniqueid)
{
	return 0;
}
//
int CAirPlayServer::CTCPClient::AirPlayOutputFunctions::airplay_ispaused(void *cls, uint64_t uniqueid)
{
	return 0;
}
#endif
