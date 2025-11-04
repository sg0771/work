#ifndef AIRPLAY_H
#define AIRPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../WXAirPlayAPI.h"

/* Define syslog style log levels */
#define AIRPLAY_LOG_EMERG       0       /* system is unusable */
#define AIRPLAY_LOG_ALERT       1       /* action must be taken immediately */
#define AIRPLAY_LOG_CRIT        2       /* critical conditions */
#define AIRPLAY_LOG_ERR         3       /* error conditions */
#define AIRPLAY_LOG_WARNING     4       /* warning conditions */
#define AIRPLAY_LOG_NOTICE      5       /* normal but significant condition */
#define AIRPLAY_LOG_INFO        6       /* informational */
#define AIRPLAY_LOG_DEBUG       7       /* debug-level messages */


typedef struct airplay_s airplay_t;

typedef void (*airplay_log_callback_t)(void *cls, int level, const char *msg);

struct airplay_callbacks_s {
	void* cls;

	/* Compulsory callback functions */
	void* (*audio_init)(void *cls, int bits, int channels, int samplerate);
	void  (*audio_process)(void *cls, void *session, const void *buffer, int buflen);
	void  (*audio_destroy)(void *cls, void *session);

	/* Optional but recommended callback functions */
	void  (*audio_flush)(void *cls, void *session);
	void  (*audio_set_volume)(void *cls, void *session, float volume);
	void  (*audio_set_metadata)(void *cls, void *session, const void *buffer, int buflen);
	void  (*audio_set_coverart)(void *cls, void *session, const void *buffer, int buflen);
};
typedef struct airplay_callbacks_s airplay_callbacks_t;

AIRPLAY_API airplay_t *airplay_init(int max_clients, airplay_callbacks_t *callbacks, const char *pemkey, int *error);
AIRPLAY_API airplay_t *airplay_init_from_key(int max_clients, airplay_callbacks_t *callback);

AIRPLAY_API int airplay_start(airplay_t *airplay, unsigned short *port, unsigned short *mirror_port, const char *hwaddr, int hwaddrlen, const char *password);
AIRPLAY_API int airplay_is_running(airplay_t *airplay);
AIRPLAY_API void airplay_stop(airplay_t *airplay);

AIRPLAY_API void airplay_destroy(airplay_t *airplay);

AIRPLAY_API void airplay_disconnect(airplay_t *airplay, uint64_t uniqueid);

AIRPLAY_API void mirror_disconnect(airplay_t *airplay, uint64_t uniqueid);

AIRPLAY_API void mirror_exit();

AIRPLAY_API void StartMirror(const char *strParam1, const char *strParam2, const char *remoteip);

AIRPLAY_API void GetFirstPicture(uint64_t uniqueid);

AIRPLAY_API void StopRecord(uint64_t uniqueid);

#ifdef __cplusplus
}
#endif
#endif
