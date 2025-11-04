#pragma once
/*
 * Many concepts and protocol specification in this code are taken from
 * the Boxee project. http://www.boxee.tv
 *
 *      Copyright (C) 2011-2013 Team XBMC
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

#include "../utils/utils.h"

#ifdef HAS_AIRPLAY

#include <map>


#include <Winsock2.h>
#include <WS2tcpip.h>
#include <ws2bth.h>


#ifndef SHUT_RDWR
#define SHUT_RDWR SD_BOTH
#endif

#ifndef SHUT_RD
#define SHUT_RD SD_RECEIVE
#endif

#ifndef SHUT_WR
#define SHUT_WR SD_SEND
#endif


#ifndef AF_BTH
#define AF_BTH          32
#endif

#ifndef BTHPROTO_RFCOMM
#define BTHPROTO_RFCOMM 3
#endif

#ifndef AF_BLUETOOTH
#define AF_BLUETOOTH AF_BTH
#endif

#ifndef BTPROTO_RFCOMM
#define BTPROTO_RFCOMM BTHPROTO_RFCOMM
#endif

typedef int socklen_t;


#include "../utils/Thread.h"
#include "IAnnouncer.h"

#include "../utils/HttpParser.h"

#include <SDL2/SDL.h>

extern "C"{
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

#include <WXMedia.h>
#include <WXBase.h>

#include "../plist/plist.h"

#define AIRPLAY_SERVER_VERSION_STR "101.28"
#define CHROMA "YUYV"

class CAirPlayServer : public AThread, public ANNOUNCEMENT::IAnnouncer
{
public:
  // IAnnouncer IF
  virtual void Announce(ANNOUNCEMENT::AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data);

  //AirPlayServer impl.
  static bool StartServer(int *port, bool nonlocal);
  static void StopServer(bool bWait);
  static bool IsRunning();
  static bool SetCredentials(bool usePassword, const std::string& password);
  static bool IsPlaying(){ return m_isPlaying > 0;}

  static int m_isPlaying;
  static int m_isPaused;
  static int m_isPaused2;
  static void DisconnectPlay(uint64_t uniqueid);

  char* ProcessAirplayVideo(int fd, int counter, unsigned char *remote, char *method, char *url, char *data, int datalen, int neednew, char* sessionid, char* contenttype,
	  char* auth, char* photoAction, char* photoCacheId);
protected:
  void Process();

private:
  CAirPlayServer(int port, bool nonlocal);
  ~CAirPlayServer();
  bool SetInternalCredentials(bool usePassword, const std::string& password);
  bool Initialize();
  void Deinitialize();

public:
  class CTCPClient
  {
  public:
    CTCPClient();
    ~CTCPClient();
    CTCPClient(const CTCPClient& client);
    CTCPClient& operator=(const CTCPClient& client);
    void PushBuffer(CAirPlayServer *host, const char *buffer,
                    int length, std::string &sessionId,
                    std::map<std::string, int> &reverseSockets);
    void ComposeReverseEvent(std::string& reverseHeader, std::string& reverseBody, int state);
	int ProcessRequestNew(std::string& responseHeader,
		std::string& responseBody, char* method, char* url, char* data, int datalen, char* sessionid, char* contenttype, char* querystring, char* authorization2, char* photoAction2, char* photoCacheId2);
    void Disconnect();

    int m_socket;
    struct sockaddr_storage m_cliaddr;
    socklen_t m_addrlen;
    CCriticalSection m_critSection;
    int  m_sessionCounter;
    std::string m_sessionId;
	std::string m_strAddr;
	std::string m_strAddrReal;


	
	unsigned int mediaWidth;
	unsigned int mediaHeight;
	//
	bool isRendering;
	bool isEndReached;

	void *pixelData;
	long long totalDuration;
	long currentPosition;
	long lastPosition;
	int isPlaying;
	int isPaused;

	class AirPlayOutputFunctions
	{
	public:
		static void airplay_open(void *cls, char *url, char *secondurl, float fPosition, uint64_t uniqueid);
		static void airplay_play(void *cls, uint64_t uniqueid);
		static void airplay_pause(void *cls, uint64_t uniqueid);
		static void airplay_stop(void *cls, uint64_t uniqueid);
		static void airplay_seek(void *cls, long fPosition, uint64_t uniqueid);
		static void airplay_setvolume(void *cls, int volume, uint64_t uniqueid);
		static void airplay_showphoto(void *cls, unsigned char *data, long long size, uint64_t uniqueid);
		static long airplay_getduration(void *cls, uint64_t uniqueid);
		static long airplay_getpostion(void *cls, uint64_t uniqueid);
		static void airplay_setlastpostion(void *cls, long last, uint64_t uniqueid);
		static long airplay_getlastpostion(void *cls, uint64_t uniqueid);
		static int  airplay_isplaying(void *cls, uint64_t uniqueid);
		static int  airplay_ispaused(void *cls, uint64_t uniqueid);
	};

  public:
	  //SDL
	  int screen_w, screen_h;
	  SDL_Window *screen;
	  SDL_Renderer *renderer;
	  SDL_Texture *bmp;
	  SDL_Rect rect;
  private:
    int ProcessRequest( std::string& responseHeader,
                        std::string& response);

    void ComposeAuthRequestAnswer(std::string& responseHeader, std::string& responseBody);
    bool checkAuthorization(const std::string& authStr, const std::string& method, const std::string& uri);
    void Copy(const CTCPClient& client);

    HttpParser* m_httpParser;
    bool m_bAuthenticated;
    int  m_lastEvent;
    std::string m_authNonce;
  };

  CCriticalSection m_connectionLock;
  std::vector<CTCPClient*> m_connections;
  std::map<std::string, int> m_reverseSockets;
  std::map<std::string, int> m_videoplays;
  int m_ServerSocket;
  int m_port;
  bool m_nonlocal;
  bool m_usePassword;
  std::string m_password;
  int m_origVolume;

public:
	static std::map<uint64_t, int> g_mapAirPlaying;
	static int IsAirplayPlaying(uint64_t uuid);
	static void SetAirplayPlaying(uint64_t uuid, int iStatus);
	static int iMirroring;
	static int IsMirroring();
	static void SetMirroring(int iMirror);
	void AnnounceToClients(int state, std::string &strSessionId);
	static CCriticalSection ServerInstanceLock;
	static CAirPlayServer *ServerInstance;
	static std::string m_strTmpDir;
};
#endif
