#ifndef RAOP_H
#define RAOP_H

# define RAOP_API  EXTERN_C

/* Define syslog style log levels */
#define RAOP_LOG_EMERG       0       /* system is unusable */
#define RAOP_LOG_ALERT       1       /* action must be taken immediately */
#define RAOP_LOG_CRIT        2       /* critical conditions */
#define RAOP_LOG_ERR         3       /* error conditions */
#define RAOP_LOG_WARNING     4       /* warning conditions */
#define RAOP_LOG_NOTICE      5       /* normal but significant condition */
#define RAOP_LOG_INFO        6       /* informational */
#define RAOP_LOG_DEBUG       7       /* debug-level messages */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct raop_s raop_t;

typedef void (*raop_log_callback_t)(void *cls, int level, const char *msg);

struct raop_callbacks_s {
	void* cls;

	/* Compulsory callback functions */
	void* (*audio_init)(void *cls, int bits, int channels, int samplerate);
	void(*audio_process)(void *cls, void *session, const void *buffer, int buflen);
	void(*audio_destroy)(void *cls, void *session);

	/* Optional but recommended callback functions */
	void(*audio_flush)(void *cls, void *session);
	void(*audio_set_volume)(void *cls, void *session, float volume);
	void(*audio_set_metadata)(void *cls, void *session, const void *buffer, int buflen);
	void(*audio_set_coverart)(void *cls, void *session, const void *buffer, int buflen);
	void(*audio_remote_control_id)(void *cls, const char *dacp_id, const char *active_remote_header);
	//
	void(*init_mirror_param)(const char* aeskey, const char* aesiv, const char* remoteip);
	void(*get_rand_chars)(unsigned char* buffer, int iLen);
	//
	void(*set_sharekey)(unsigned char* buffer, int iLen);
	void(*get_ports)(int *iDataPort, int *iMirror_Port);
	void(*set_displayset)(const char *parcModel);
	int(*get_displaypixel)();
	char* (*get_macaddress)();
	void(*Get_model)(const unsigned char*remoteip, const char* arcModel);
	void(*Get_name)(const unsigned char*remoteip, const char* arcName);
	void(*Get_osVersion)(const unsigned char*remoteip, const char* arcOsVersion);
	void(*set_audio_frame)(unsigned char *buffer, int iLen, unsigned long long fpstme);
	char*(*processairplayvideo)(int fd, int counter, unsigned char *remote, char *method, char *url, char *data, int datalen, int neednew, char* sessionid, char* contenttype,
		char* auth, char* photoAction, char* photoCacheId);
	void(*freeResBuffer)(char* buf);
};
typedef struct raop_callbacks_s raop_callbacks_t;
#ifdef __cplusplus
}
#endif

RAOP_API raop_t* raop_init(int max_clients, raop_callbacks_t* callbacks, const char* pemkey);
RAOP_API int    raop_start(raop_t* raop, unsigned short* port, const char* hwaddr, int hwaddrlen, const char* password);
RAOP_API int    raop_is_running(raop_t* raop);
RAOP_API void   raop_stop(raop_t* raop);
RAOP_API void   raop_destroy(raop_t* raop);

#endif
