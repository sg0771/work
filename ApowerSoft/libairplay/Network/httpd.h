/**
 *  Copyright (C) 2011-2012  Juho Vähä-Herttua
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 */

#ifndef HTTPD_H
#define HTTPD_H

#include <stdint.h>

typedef struct http_request_s http_request_t;

http_request_t* http_request_init(void);

int http_request_add_data(http_request_t* request, const char* data, int datalen);
int http_request_is_complete(http_request_t* request);
int http_request_has_error(http_request_t* request);

const char* http_request_get_error_name(http_request_t* request);
const char* http_request_get_error_description(http_request_t* request);
const char* http_request_get_method(http_request_t* request);
const char* http_request_get_url(http_request_t* request);
const char* http_request_get_header(http_request_t* request, const char* name);
const char* http_request_get_data(http_request_t* request, int* datalen);

void http_request_destroy(http_request_t* request);
void http_request_dump_headers(http_request_t* request);

typedef struct http_response_s http_response_t;

http_response_t* http_response_init(const char* protocol, int code, const char* message);
http_response_t* http_response_init1(char* data, int size);

void http_response_add_header(http_response_t* response, const char* name, const char* value);
void http_response_finish(http_response_t* response, const char* data, int datalen);

void http_response_set_disconnect(http_response_t* response, int disconnect);
int http_response_get_disconnect(http_response_t* response);

const char* http_response_get_data(http_response_t* response, int* datalen);

void http_response_destroy(http_response_t* response);



#define HTTP_PARSER_VERSION_MAJOR 2
#define HTTP_PARSER_VERSION_MINOR 0

#include <sys/types.h>
#if defined(_WIN32) && !defined(__MINGW32__) && (!defined(_MSC_VER) || _MSC_VER<1600)
#include <BaseTsd.h>
typedef __int8 int8_t;
typedef unsigned __int8 uint8_t;
typedef __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
typedef SIZE_T size_t;
typedef SSIZE_T ssize_t;
#else
#include <stdint.h>
#endif

/* Compile with -DHTTP_PARSER_STRICT=0 to make less checks, but run
 * faster
 */
#ifndef HTTP_PARSER_STRICT
# define HTTP_PARSER_STRICT 0
#endif

 /* Compile with -DHTTP_PARSER_DEBUG=1 to add extra debugging information to
  * the error reporting facility.
  */
#ifndef HTTP_PARSER_DEBUG
# define HTTP_PARSER_DEBUG 0
#endif


  /* Maximium header size allowed */
#define HTTP_MAX_HEADER_SIZE (80*1024)


typedef struct http_parser http_parser;
typedef struct http_parser_settings http_parser_settings;


/* Callbacks should return non-zero to indicate an error. The parser will
 * then halt execution.
 *
 * The one exception is on_headers_complete. In a HTTP_RESPONSE parser
 * returning '1' from on_headers_complete will tell the parser that it
 * should not expect a body. This is used when receiving a response to a
 * HEAD request which may contain 'Content-Length' or 'Transfer-Encoding:
 * chunked' headers that indicate the presence of a body.
 *
 * http_data_cb does not return data chunks. It will be call arbitrarally
 * many times for each string. E.G. you might get 10 callbacks for "on_path"
 * each providing just a few characters more data.
 */
typedef int (*http_data_cb) (http_parser*, const char* at, size_t length);
typedef int (*http_cb) (http_parser*);


/* Request Methods */
#define HTTP_METHOD_MAP(XX)         \
  XX(0,  DELETE,      DELETE)       \
  XX(1,  GET,         GET)          \
  XX(2,  HEAD,        HEAD)         \
  XX(3,  POST,        POST)         \
  XX(4,  PUT,         PUT)          \
  /* pathological */                \
  XX(5,  CONNECT,     CONNECT)      \
  XX(6,  OPTIONS,     OPTIONS)      \
  XX(7,  TRACE,       TRACE)        \
  /* webdav */                      \
  XX(8,  COPY,        COPY)         \
  XX(9,  LOCK,        LOCK)         \
  XX(10, MKCOL,       MKCOL)        \
  XX(11, MOVE,        MOVE)         \
  XX(12, PROPFIND,    PROPFIND)     \
  XX(13, PROPPATCH,   PROPPATCH)    \
  XX(14, SEARCH,      SEARCH)       \
  XX(15, UNLOCK,      UNLOCK)       \
  /* subversion */                  \
  XX(16, REPORT,      REPORT)       \
  XX(17, MKACTIVITY,  MKACTIVITY)   \
  XX(18, CHECKOUT,    CHECKOUT)     \
  XX(19, MERGE,       MERGE)        \
  /* upnp */                        \
  XX(20, MSEARCH,     M-SEARCH)     \
  XX(21, NOTIFY,      NOTIFY)       \
  XX(22, SUBSCRIBE,   SUBSCRIBE)    \
  XX(23, UNSUBSCRIBE, UNSUBSCRIBE)  \
  /* RFC-5789 */                    \
  XX(24, PATCH,       PATCH)        \
  XX(25, PURGE,       PURGE)        \
  /* RFC-2326 (RTSP) */             \
  XX(26, DESCRIBE,    DESCRIBE)     \
  XX(27, ANNOUNCE,    ANNOUNCE)     \
  XX(28, SETUP,       SETUP)        \
  XX(29, PLAY,        PLAY)         \
  XX(30, PAUSE,       PAUSE)        \
  XX(31, TEARDOWN,    TEARDOWN)     \
  XX(32, GET_PARAMETER, GET_PARAMETER) \
  XX(33, SET_PARAMETER, SET_PARAMETER) \
  XX(34, REDIRECT,    REDIRECT)     \
  XX(35, RECORD,      RECORD)       \
  /* RAOP */                        \
  XX(36, FLUSH,       FLUSH)        \

enum http_method
{
#define XX(num, name, string) HTTP_##name = num,
    HTTP_METHOD_MAP(XX)
#undef XX
};


enum http_parser_type { HTTP_REQUEST, HTTP_RESPONSE, HTTP_BOTH };


/* Flag values for http_parser.flags field */
enum flags
{
    F_CHUNKED = 1 << 0
    , F_CONNECTION_KEEP_ALIVE = 1 << 1
    , F_CONNECTION_CLOSE = 1 << 2
    , F_TRAILING = 1 << 3
    , F_UPGRADE = 1 << 4
    , F_SKIPBODY = 1 << 5
};


/* Map for errno-related constants
 *
 * The provided argument should be a macro that takes 2 arguments.
 */
#define HTTP_ERRNO_MAP(XX)                                           \
  /* No error */                                                     \
  XX(OK, "success")                                                  \
                                                                     \
  /* Callback-related errors */                                      \
  XX(CB_message_begin, "the on_message_begin callback failed")       \
  XX(CB_url, "the on_url callback failed")                           \
  XX(CB_header_field, "the on_header_field callback failed")         \
  XX(CB_header_value, "the on_header_value callback failed")         \
  XX(CB_headers_complete, "the on_headers_complete callback failed") \
  XX(CB_body, "the on_body callback failed")                         \
  XX(CB_message_complete, "the on_message_complete callback failed") \
                                                                     \
  /* Parsing-related errors */                                       \
  XX(INVALID_EOF_STATE, "stream ended at an unexpected time")        \
  XX(HEADER_OVERFLOW,                                                \
     "too many header bytes seen; overflow detected")                \
  XX(CLOSED_CONNECTION,                                              \
     "data received after completed connection: close message")      \
  XX(INVALID_VERSION, "invalid HTTP version")                        \
  XX(INVALID_STATUS, "invalid HTTP status code")                     \
  XX(INVALID_METHOD, "invalid HTTP method")                          \
  XX(INVALID_URL, "invalid URL")                                     \
  XX(INVALID_HOST, "invalid host")                                   \
  XX(INVALID_PORT, "invalid port")                                   \
  XX(INVALID_PATH, "invalid path")                                   \
  XX(INVALID_QUERY_STRING, "invalid query string")                   \
  XX(INVALID_FRAGMENT, "invalid fragment")                           \
  XX(LF_EXPECTED, "LF character expected")                           \
  XX(INVALID_HEADER_TOKEN, "invalid character in header")            \
  XX(INVALID_CONTENT_LENGTH,                                         \
     "invalid character in content-length header")                   \
  XX(INVALID_CHUNK_SIZE,                                             \
     "invalid character in chunk size header")                       \
  XX(INVALID_CONSTANT, "invalid constant string")                    \
  XX(INVALID_INTERNAL_STATE, "encountered unexpected internal state")\
  XX(STRICT, "strict mode assertion failed")                         \
  XX(PAUSED, "parser is paused")                                     \
  XX(UNKNOWN, "an unknown error occurred")


 /* Define HPE_* values for each errno value above */
#define HTTP_ERRNO_GEN(n, s) HPE_##n,
enum http_errno {
    HTTP_ERRNO_MAP(HTTP_ERRNO_GEN)
};
#undef HTTP_ERRNO_GEN


/* Get an http_errno value from an http_parser */
#define HTTP_PARSER_ERRNO(p)            ((enum http_errno) (p)->http_errno)

/* Get the line number that generated the current error */
#if HTTP_PARSER_DEBUG
#define HTTP_PARSER_ERRNO_LINE(p)       ((p)->error_lineno)
#else
#define HTTP_PARSER_ERRNO_LINE(p)       0
#endif


struct http_parser {
    /** PRIVATE **/
    unsigned char type : 2;     /* enum http_parser_type */
    unsigned char flags : 6;    /* F_* values from 'flags' enum; semi-public */
    unsigned char state;        /* enum state from http_parser.c */
    unsigned char header_state; /* enum header_state from http_parser.c */
    unsigned char index;        /* index into current matcher */

    uint32_t nread;          /* # bytes read in various scenarios */
    uint64_t content_length; /* # bytes in body (0 if no Content-Length header) */

    /** READ-ONLY **/
    unsigned short http_major;
    unsigned short http_minor;
    unsigned short status_code; /* responses only */
    unsigned char method;       /* requests only */
    unsigned char http_errno : 7;

    /* 1 = Upgrade header was present and the parser has exited because of that.
     * 0 = No upgrade header present.
     * Should be checked when http_parser_execute() returns in addition to
     * error checking.
     */
    unsigned char upgrade : 1;

#if HTTP_PARSER_DEBUG
    uint32_t error_lineno;
#endif

    /** PUBLIC **/
    void* data; /* A pointer to get hook to the "connection" or "socket" object */
};


struct http_parser_settings {
    http_cb      on_message_begin;
    http_data_cb on_url;
    http_data_cb on_header_field;
    http_data_cb on_header_value;
    http_cb      on_headers_complete;
    http_data_cb on_body;
    http_cb      on_message_complete;
};


enum http_parser_url_fields
{
    UF_SCHEMA = 0
    , UF_HOST = 1
    , UF_PORT = 2
    , UF_PATH = 3
    , UF_QUERY = 4
    , UF_FRAGMENT = 5
    , UF_USERINFO = 6
    , UF_MAX = 7
};


/* Result structure for http_parser_parse_url().
 *
 * Callers should index into field_data[] with UF_* values iff field_set
 * has the relevant (1 << UF_*) bit set. As a courtesy to clients (and
 * because we probably have padding left over), we convert any port to
 * a uint16_t.
 */
struct http_parser_url {
    uint16_t field_set;           /* Bitmask of (1 << UF_*) values */
    uint16_t port;                /* Converted UF_PORT string */

    struct {
        uint16_t off;               /* Offset into buffer in which field starts */
        uint16_t len;               /* Length of run in buffer */
    } field_data[UF_MAX];
};


void http_parser_init(http_parser* parser, enum http_parser_type type);


size_t http_parser_execute(http_parser* parser,
    const http_parser_settings* settings,
    const char* data,
    size_t len);


/* If http_should_keep_alive() in the on_headers_complete or
 * on_message_complete callback returns 0, then this should be
 * the last message on the connection.
 * If you are the server, respond with the "Connection: close" header.
 * If you are the client, close the connection.
 */
int http_should_keep_alive(const http_parser* parser);

/* Returns a string version of the HTTP method. */
const char* http_method_str(enum http_method m);

/* Return a string name of the given error */
const char* http_errno_name(enum http_errno err);

/* Return a string description of the given error */
const char* http_errno_description(enum http_errno err);

/* Parse a URL; return nonzero on failure */
int http_parser_parse_url(const char* buf, size_t buflen,
    int is_connect,
    struct http_parser_url* u);

/* Pause or un-pause the parser; a nonzero value pauses */
void http_parser_pause(http_parser* parser, int paused);

/* Checks if this is the final chunk of the body. */
int http_body_is_final(const http_parser* parser);

typedef struct httpd_s httpd_t;

struct httpd_callbacks_s {
	void* opaque;
	void* (*conn_init)(void *opaque, unsigned char *local, int locallen, unsigned char *remote, int remotelen, char* ip);
	void  (*conn_request)(void *ptr, http_request_t *request, http_response_t **response);
	void  (*conn_destroy)(void *ptr);
	void  (*conn_datafeed)(void *ptr, unsigned char *data, int size, uint64_t ip, uint64_t pts);
	void(*conn_mirror_destroy)(const char* remoteip);
	void(*set_mirror_stream)(uint64_t uniqueid, int disconnect);
	int(*get_mirror_stream)(uint64_t uniqueid);
};
typedef struct httpd_callbacks_s httpd_callbacks_t;

httpd_t *httpd_init(httpd_callbacks_t *callbacks, int max_connections);
int httpd_is_running(httpd_t *httpd);
int httpd_start(httpd_t *httpd, unsigned short *port, int iMirrorPort);
void httpd_stop(httpd_t *httpd);
void httpd_destroy(httpd_t *httpd);
int httpd_get_mirror_streaming(httpd_t *httpd);
void httpd_set_mirror_streaming(httpd_t *httpd, uint64_t uniqueid);
void httpd_disconnect(httpd_t *httpd, uint64_t uniqueid);
int httpd_get_disconnect(httpd_t *httpd, uint64_t uniqueid);
void httpd_remove_connection_new(httpd_t *httpd, uint64_t uniqueid);

#endif
