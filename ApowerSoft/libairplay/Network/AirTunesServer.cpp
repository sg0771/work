/*
 * Many concepts and protocol specification in this code are taken
 * from Shairport, by James Laird.
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

#include "Network.h"
#if !defined(_WIN32)
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif

#include "AirTunesServer.h"

#ifdef HAS_AIRPLAY
#include "AirPlayServer.h"
#endif

#ifdef HAS_AIRTUNES

#include "../utils/File.h"
#include "../utils/Variant.h"
#include "../utils/StringUtils.h"
//#include "../utils/EndianSwap.h"
#include "../utils/URL.h"
#include <map>
#include <string>

#include "airplay.h"

#include "NetworkServices.h"

#include "dnssd.h"

#include "WXMedia.h"

#define TMP_COVERART_PATH "special://temp/airtunes_album_thumb.jpg"

extern CNetwork *g_network;


using namespace ANNOUNCEMENT;


CAirTunesServer *CAirTunesServer::ServerInstance = NULL;
std::string CAirTunesServer::m_macAddress = "";
std::string CAirTunesServer::m_macAddressNew = "";
std::string CAirTunesServer::m_metadata[3];
CCriticalSection CAirTunesServer::m_metadataLock;
bool CAirTunesServer::m_streamStarted = false;

static __inline__ uint32_t Endian_SwapBE32(uint32_t x) {
	return((x << 24) | ((x << 8) & 0x00FF0000) | ((x >> 8) & 0x0000FF00) | (x >> 24));
}

//parse daap metadata - thx to project MythTV
std::map<std::string, std::string> decodeDMAP(const char *buffer, unsigned int size)
{
  std::map<std::string, std::string> result;
  unsigned int offset = 8;
  while (offset < size)
  {
    std::string tag;
    tag.append(buffer + offset, 4);
    offset += 4;
    uint32_t length = Endian_SwapBE32(*(uint32_t *)(buffer + offset));
    offset += sizeof(uint32_t);
    std::string content;
    content.append(buffer + offset, length);//possible fixme - utf8?
    offset += length;
    result[tag] = content;
  }
  return result;
}

void CAirTunesServer::ResetMetadata()
{
  CSingleLock lock(m_metadataLock);
  RefreshCoverArt();

  m_metadata[0] = "";
  m_metadata[1] = "AirPlay";
  m_metadata[2] = "";
  RefreshMetadata();
}

void CAirTunesServer::RefreshMetadata()
{

}

void CAirTunesServer::RefreshCoverArt()
{

}

void CAirTunesServer::SetMetadataFromBuffer(const char *buffer, unsigned int size)
{

  std::map<std::string, std::string> metadata = decodeDMAP(buffer, size);
  CSingleLock lock(m_metadataLock);

  if(metadata["asal"].length())
    m_metadata[0] = metadata["asal"];//album
  if(metadata["minm"].length())    
    m_metadata[1] = metadata["minm"];//title
  if(metadata["asar"].length())    
    m_metadata[2] = metadata["asar"];//artist
  
  RefreshMetadata();
}

void CAirTunesServer::Announce(AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data)
{
  if ( (flag & Player) && strcmp(sender, "xbmc") == 0)
  {
    if (strcmp(message, "OnPlay") == 0 && m_streamStarted)
    {
      RefreshMetadata();
      RefreshCoverArt();
    }
  }
}

void CAirTunesServer::SetCoverArtFromBuffer(const char *buffer, unsigned int size)
{

}

#define RSA_KEY " \
-----BEGIN RSA PRIVATE KEY-----\
MIIEpQIBAAKCAQEA59dE8qLieItsH1WgjrcFRKj6eUWqi+bGLOX1HL3U3GhC/j0Qg90u3sG/1CUt\
wC5vOYvfDmFI6oSFXi5ELabWJmT2dKHzBJKa3k9ok+8t9ucRqMd6DZHJ2YCCLlDRKSKv6kDqnw4U\
wPdpOMXziC/AMj3Z/lUVX1G7WSHCAWKf1zNS1eLvqr+boEjXuBOitnZ/bDzPHrTOZz0Dew0uowxf\
/+sG+NCK3eQJVxqcaJ/vEHKIVd2M+5qL71yJQ+87X6oV3eaYvt3zWZYD6z5vYTcrtij2VZ9Zmni/\
UAaHqn9JdsBWLUEpVviYnhimNVvYFZeCXg/IdTQ+x4IRdiXNv5hEewIDAQABAoIBAQDl8Axy9XfW\
BLmkzkEiqoSwF0PsmVrPzH9KsnwLGH+QZlvjWd8SWYGN7u1507HvhF5N3drJoVU3O14nDY4TFQAa\
LlJ9VM35AApXaLyY1ERrN7u9ALKd2LUwYhM7Km539O4yUFYikE2nIPscEsA5ltpxOgUGCY7b7ez5\
NtD6nL1ZKauw7aNXmVAvmJTcuPxWmoktF3gDJKK2wxZuNGcJE0uFQEG4Z3BrWP7yoNuSK3dii2jm\
lpPHr0O/KnPQtzI3eguhe0TwUem/eYSdyzMyVx/YpwkzwtYL3sR5k0o9rKQLtvLzfAqdBxBurciz\
aaA/L0HIgAmOit1GJA2saMxTVPNhAoGBAPfgv1oeZxgxmotiCcMXFEQEWflzhWYTsXrhUIuz5jFu\
a39GLS99ZEErhLdrwj8rDDViRVJ5skOp9zFvlYAHs0xh92ji1E7V/ysnKBfsMrPkk5KSKPrnjndM\
oPdevWnVkgJ5jxFuNgxkOLMuG9i53B4yMvDTCRiIPMQ++N2iLDaRAoGBAO9v//mU8eVkQaoANf0Z\
oMjW8CN4xwWA2cSEIHkd9AfFkftuv8oyLDCG3ZAf0vrhrrtkrfa7ef+AUb69DNggq4mHQAYBp7L+\
k5DKzJrKuO0r+R0YbY9pZD1+/g9dVt91d6LQNepUE/yY2PP5CNoFmjedpLHMOPFdVgqDzDFxU8hL\
AoGBANDrr7xAJbqBjHVwIzQ4To9pb4BNeqDndk5Qe7fT3+/H1njGaC0/rXE0Qb7q5ySgnsCb3DvA\
cJyRM9SJ7OKlGt0FMSdJD5KG0XPIpAVNwgpXXH5MDJg09KHeh0kXo+QA6viFBi21y340NonnEfdf\
54PX4ZGS/Xac1UK+pLkBB+zRAoGAf0AY3H3qKS2lMEI4bzEFoHeK3G895pDaK3TFBVmD7fV0Zhov\
17fegFPMwOII8MisYm9ZfT2Z0s5Ro3s5rkt+nvLAdfC/PYPKzTLalpGSwomSNYJcB9HNMlmhkGzc\
1JnLYT4iyUyx6pcZBmCd8bD0iwY/FzcgNDaUmbX9+XDvRA0CgYEAkE7pIPlE71qvfJQgoA9em0gI\
LAuE4Pu13aKiJnfft7hIjbK+5kyb3TysZvoyDnb3HOKvInK7vXbKuU4ISgxB2bB3HcYzQMGsz1qJ\
2gG0N5hvJpzwwhbhXqFKA4zaaSrw622wDniAK5MlIE0tIAKKP4yxNGjoD2QYjhBGuhvkWKY=\
-----END RSA PRIVATE KEY-----"

void CAirTunesServer::AudioOutputFunctions::audio_set_metadata(void *cls, void *session, const void *buffer, int buflen)
{
  CAirTunesServer::SetMetadataFromBuffer((char *)buffer, buflen);
}

void CAirTunesServer::AudioOutputFunctions::audio_set_coverart(void *cls, void *session, const void *buffer, int buflen)
{
  CAirTunesServer::SetCoverArtFromBuffer((char *)buffer, buflen);
}

typedef struct {
	void *device;
	int buffering;
	int buflen;
	char buffer[8192];
	float volume;
} shairplay_session_t;

void* CAirTunesServer::AudioOutputFunctions::audio_init(void *cls, int bits, int channels, int samplerate)
{
	shairplay_session_t *session = (shairplay_session_t*)calloc(1, sizeof(shairplay_session_t));
	session->device = WXSoundPlayerCreate(samplerate, channels);
	session->buffering = 1;
	session->volume = 1.0f;
	return session;
}

void  CAirTunesServer::AudioOutputFunctions::audio_set_volume(void *cls, void *session, float volume)
{
	shairplay_session_t *session1 = (shairplay_session_t*)session;
	session1->volume = volume < -30.0f ? 0 : 1 - volume / -30;// pow(10.0, 0.05*volume);
}
static int audio_output(shairplay_session_t *session, const void *buffer, int buflen)
{
	short *shortbuf;
	char tmpbuf[4096];
	int tmpbuflen, i;

	tmpbuflen = (buflen > sizeof(tmpbuf)) ? sizeof(tmpbuf) : buflen;
	memcpy(tmpbuf, buffer, tmpbuflen);

	shortbuf = (short *)tmpbuf;
	for (i = 0; i < tmpbuflen / 2; i++) {
		shortbuf[i] = shortbuf[i] * session->volume;
	}
	WXSoundPlayerWriteData(session->device, (uint8_t*)tmpbuf, tmpbuflen);
	return tmpbuflen;
}
void  CAirTunesServer::AudioOutputFunctions::audio_process(void *cls, void *session, const void *buffer, int buflen)
{
	shairplay_session_t *session1 = (shairplay_session_t*)session;

	{
		int processed;

		if (session1->buffering) {
			//printf("Buffering... %d %d\n", session1->buflen + buflen, sizeof(session1->buffer));
			if (session1->buflen + buflen < sizeof(session1->buffer)) {
				memcpy(session1->buffer + session1->buflen, buffer, buflen);
				session1->buflen += buflen;
				return;
			}
			session1->buffering = 0;
			printf("Finished buffering...\n");

			processed = 0;
			while (processed < session1->buflen) {
				processed += audio_output(session1,
					session1->buffer + processed,
					session1->buflen - processed);
			}
			session1->buflen = 0;
		}

		processed = 0;
		while (processed < buflen) {
			processed += audio_output(session1,
				(char*)buffer + processed,///////////////////////////////////////////////////////////
				buflen - processed);
		}
	}
}

void  CAirTunesServer::AudioOutputFunctions::audio_destroy(void *cls, void *session)
{
	shairplay_session_t *session1 = (shairplay_session_t*)session;

	if (session1->device != NULL) {
		WXSoundPlayerDestroy(session1->device);
		session1->device = NULL;
	}
	free(session1);
}

int CAirTunesServer::AudioOutputFunctions::audio_prepare(char *driver, char *devicename, char *deviceid, raop_callbacks_t *raop_cbs)
{
	return 0;
}

void CAirTunesServer::AudioOutputFunctions::audio_shutdown()
{

}

void CAirTunesServer::AudioOutputFunctions::init_mirror_param(const char* aeskey, const char* aesiv, const char* remoteip)
{
	StartMirror(aeskey, aesiv, remoteip);
}



bool CAirTunesServer::StartServer(int port, bool nonlocal, bool usePassword, const std::string &password/*=""*/, const std::string &strAppName)
{
  bool success = false;
  std::string pw = password;
  CNetworkInterface *net = g_network->GetFirstConnectedInterface();
  StopServer(true);

  if (net)
  {
    m_macAddress = net->GetMacAddress();
	m_macAddressNew = m_macAddress;
    StringUtils::Replace(m_macAddress, ":","");
    while (m_macAddress.size() < 12)
    {
      m_macAddress = '0' + m_macAddress;
    }
  }
  else
  {
	  if (CNetworkServices::m_strVMNetMac != "")
	  {
		  m_macAddress = CNetworkServices::m_strVMNetMac;
		  m_macAddressNew = m_macAddress;
		  StringUtils::Replace(m_macAddress, ":", "");
		  while (m_macAddress.size() < 12)
		  {
			  m_macAddress = '0' + m_macAddress;
		  }
	  }
	  else
	  {
		  m_macAddress = "000102030405";
		  m_macAddressNew = "00:01:02:03:04:05";
	  }
  }

  if (!usePassword)
  {
    pw.clear();
  }

  ServerInstance = new CAirTunesServer(port, nonlocal);
  if (ServerInstance->Initialize(pw))
  {
    success = true;
    std::string appName = StringUtils::Format("%s@%s",
                                             m_macAddress.c_str(),
											 strAppName.c_str());
    std::vector<std::pair<std::string, std::string> > txt;
    txt.push_back(std::make_pair("txtvers",  "1"));
	txt.push_back(std::make_pair("cn", "0,1,3"));//0,1 1,3
	txt.push_back(std::make_pair("ch", "2"));
    txt.push_back(std::make_pair("ek", "1"));
	txt.push_back(std::make_pair("et", "0,3,5"));//0,3,5 0,1
	txt.push_back(std::make_pair("sv", "false"));
	txt.push_back(std::make_pair("tp", "UDP"));
    txt.push_back(std::make_pair("sm",  "false"));
    txt.push_back(std::make_pair("ss",  "16"));
    txt.push_back(std::make_pair("sr",  "44100"));
    //txt.push_back(std::make_pair("pw",  usePassword?"true":"false"));
	txt.push_back(std::make_pair("pw", "false"));
    //txt.push_back(std::make_pair("vn",  "65537"));
	txt.push_back(std::make_pair("vn", "3"));
    txt.push_back(std::make_pair("da",  "true"));
    txt.push_back(std::make_pair("md",  "0,1,2"));
	txt.push_back(std::make_pair("am",  "AppleTV3,2"));//Kodi,1 AppleTV3,2
	txt.push_back(std::make_pair("vs", "220.68"));

	//ios 9
	txt.push_back(std::make_pair("sf", "0x4"));
	txt.push_back(std::make_pair("ft", "0x5A7FFFF7,0x1E"));//0x5A7FFFF7,0x1E        0x0A7FFFF7
	txt.push_back(std::make_pair("pk", "f3769a660475d27b4f6040381d784645e13e21c53e6d2da6a8c3d757086fc336"));

	if (!CNetworkServices::m_dnssd)
	{
		CNetworkServices::m_dnssd = dnssd_init(NULL);
		if (!CNetworkServices::m_dnssd) {
			printf("Failed to init dnssd\n");
			return false;
		}
	}
	dnssd_register(CNetworkServices::m_dnssd, appName.c_str(), "_raop._tcp", ServerInstance->m_port, txt);
  }

  return success;
}

void CAirTunesServer::StopServer(bool bWait)
{
  if (ServerInstance)
  {
    ServerInstance->Deinitialize();
    if (bWait)
    {
      delete ServerInstance;
      ServerInstance = NULL;
    }
  }
}

 bool CAirTunesServer::IsRunning()
 {
   if (ServerInstance == NULL)
     return false;

   return ((AThread*)ServerInstance)->IsRunning();
 }

CAirTunesServer::CAirTunesServer(int port, bool nonlocal)
{
  m_port = port;
  m_pRaop = NULL;
}

CAirTunesServer::~CAirTunesServer()
{

}

bool CAirTunesServer::Initialize(const std::string &password)
{
  bool ret = false;

  Deinitialize();

  {
    raop_callbacks_t ao = {};
	AudioOutputFunctions::audio_prepare((char*)"", (char*)"", (char*)"", &ao);

    ao.cls                  = nullptr;
    ao.audio_init           = AudioOutputFunctions::audio_init;
    ao.audio_set_volume     = AudioOutputFunctions::audio_set_volume;
    ao.audio_set_metadata   = AudioOutputFunctions::audio_set_metadata;
    ao.audio_set_coverart   = AudioOutputFunctions::audio_set_coverart;
    ao.audio_process        = AudioOutputFunctions::audio_process;
    ao.audio_destroy        = AudioOutputFunctions::audio_destroy;
	ao.audio_remote_control_id = NULL;
	ao.init_mirror_param    = AudioOutputFunctions::init_mirror_param;
	ao.get_rand_chars		= CNetworkServices::GetRandChars;
	ao.set_sharekey			= CNetworkServices::Set_ShareKey;
	ao.get_ports			= CNetworkServices::Get_Ports;
	ao.set_audio_frame = NULL;
	ao.set_displayset = NULL;
	ao.get_displaypixel = CNetworkServices::Get_DisplaySet;
	ao.get_macaddress = CAirTunesServer::Get_MacAddress;
	ao.Get_model = CNetworkServices::Get_model;
	ao.Get_name = CNetworkServices::Get_name;
	ao.Get_osVersion = CNetworkServices::Get_osVersion;
	ao.processairplayvideo = CNetworkServices::ProcessAirplayVideo;
	ao.freeResBuffer = CNetworkServices::freeResBuffer;

	m_pRaop = raop_init(10, &ao, RSA_KEY);//1 - we handle one client at a time max
    ret = m_pRaop != NULL;    

    if(ret)
    {
      char macAdr[6];    
      unsigned short port = (unsigned short)m_port;
      

;
  
      CNetworkInterface *net = g_network->GetFirstConnectedInterface();

	  if (net)
	  {
		  net->GetMacAddressRaw(macAdr);
	  }
	  else if (CNetworkServices::m_strVMNetMac != "")
	  {
		  memcpy(macAdr, CNetworkServices::m_VMNetRawMac, 6);
	  }
	  ret = raop_start(m_pRaop, &port, CNetworkServices::m_VMNetRawMac, 6, password.c_str()) >= 0;
	  m_port = port;
    }
  }
  return ret;
}

void CAirTunesServer::Deinitialize()
{

	if(m_pRaop != nullptr) {
		raop_stop(m_pRaop);
		raop_destroy(m_pRaop);
		m_pRaop = nullptr;
	}
}
//
char* CAirTunesServer::Get_MacAddress()
{
	return (char*)m_macAddressNew.c_str();
}
//
int CAirTunesServer::GetAirtunePort()
{
	return ServerInstance->m_port;
}

#endif

