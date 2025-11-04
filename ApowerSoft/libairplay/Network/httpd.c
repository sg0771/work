
#include <WinSock2.h>
#include <Windows.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "httpd.h"

#include <WXMedia.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <in6addr.h>

#include <ws2ipdef.h>

#include "../utils/utils.h"


#define THREAD_RETVAL DWORD WINAPI
typedef int socklen_t;

#ifndef SHUT_RD
#  define SHUT_RD SD_RECEIVE
#endif
#ifndef SHUT_WR
#  define SHUT_WR SD_SEND
#endif
#ifndef SHUT_RDWR
#  define SHUT_RDWR SD_BOTH
#endif

//#define ENABLE_IPV6

struct http_connection_s {
	int connected;

	int socket_fd;
	void *user_data;
	char ip[5];
	http_request_t *request;
};
typedef struct http_connection_s http_connection_t;

struct httpd_s {
	httpd_callbacks_t callbacks;

	int max_connections;
	int open_connections;
	http_connection_t *connections;

	/* These variables only edited mutex locked */
	int running;
	int joined;
    HANDLE thread;
    mutex_handle_t run_mutex;
	unsigned short port;
	int is_mirror;
	int stream_start;

	/* Server fds for accepting connections */
	int server_fd4;
	int server_fd6;
	//int mirror_disconn;
};

httpd_t * httpd_init( httpd_callbacks_t *callbacks, int max_connections)
{
	httpd_t *httpd;

	assert(callbacks);
	assert(max_connections > 0);

	/* Allocate the httpd_t structure */
	httpd = calloc(1, sizeof(httpd_t));
	if (!httpd) {
		return NULL;
	}

	httpd->max_connections = max_connections;
	httpd->connections = calloc(max_connections, sizeof(http_connection_t));
	if (!httpd->connections) {
		free(httpd);
		return NULL;
	}

	/* Save callback pointers */
	if (callbacks != NULL)
	{
		memcpy(&httpd->callbacks, callbacks, sizeof(httpd_callbacks_t));
	}

	/* Initial status joined */
	httpd->running = 0;
	httpd->joined = 1;

	return httpd;
}

void
httpd_destroy(httpd_t *httpd)
{
	if (httpd) {
		httpd_stop(httpd);

		free(httpd->connections);
		free(httpd);
	}
}

static void httpd_add_connection(httpd_t *httpd, int fd, 
	unsigned char *local, int local_len,
	unsigned char *remote, int remote_len)
{
	int i;

	for (i=0; i<httpd->max_connections; i++) {
		if (!httpd->connections[i].connected) {
			break;
		}
	}
	if (i == httpd->max_connections) {
		/* This code should never be reached, we do not select server_fds when full */
		////WXLogWriteNew( "Max connections reached");
		shutdown(fd, SHUT_RDWR);
		closesocket(fd);
		return;
	}

	httpd->open_connections++;
	httpd->connections[i].socket_fd = fd;
	httpd->connections[i].connected = 1;

	memcpy(httpd->connections[i].ip, remote, 4);

	if (httpd->is_mirror == 1)
	{
		httpd->connections[i].user_data = httpd->callbacks.conn_init(httpd->callbacks.opaque, local, local_len, remote, remote_len, httpd->connections[i].ip);
	}
	else
	{
		httpd->connections[i].user_data = httpd->callbacks.conn_init(httpd->callbacks.opaque, local, local_len, remote, remote_len, "");
	}
}


static unsigned char* wx_get_address(void* sockaddr, int* length)
{
    unsigned char ipv4_prefix[] = { 0,0,0,0,0,0,0,0,0,0,255,255 };
    struct sockaddr* address = (struct sockaddr*)sockaddr;

    assert(address);
    assert(length);
    unsigned char* dst = NULL;
    if (address->sa_family == AF_INET) {
        struct sockaddr_in* sin = (struct sockaddr_in*)address;
        *length = sizeof(sin->sin_addr.s_addr);
        dst = (unsigned char*)&(sin->sin_addr.s_addr);
    }
    else if (address->sa_family == AF_INET6) {
        struct sockaddr_in6* sin6 = (struct sockaddr_in6*)address;
        if (!memcmp(sin6->sin6_addr.s6_addr, ipv4_prefix, 12)) {
            /* Actually an embedded IPv4 address */
            *length = sizeof(sin6->sin6_addr.s6_addr) - 12;
            dst = (sin6->sin6_addr.s6_addr + 12);
        }
        else {
            *length = sizeof(sin6->sin6_addr.s6_addr);
            dst = sin6->sin6_addr.s6_addr;
        }
    }
    return dst;
}

static int
httpd_accept_connection(httpd_t *httpd, int server_fd, int is_ipv6)
{
	struct sockaddr_storage remote_saddr;
	socklen_t remote_saddrlen;
	struct sockaddr_storage local_saddr;
	socklen_t local_saddrlen;

	int ret, fd;

	remote_saddrlen = sizeof(remote_saddr);
	fd = accept(server_fd, (struct sockaddr *)&remote_saddr, &remote_saddrlen);
	if (fd == -1) {
		/* FIXME: Error happened */
		return -1;
	}

	local_saddrlen = sizeof(local_saddr);
	ret = getsockname(fd, (struct sockaddr *)&local_saddr, &local_saddrlen);
	if (ret == -1) {
		closesocket(fd);
		return 0;
	}


    int local_len = 0;
    unsigned char* local = wx_get_address(&local_saddr, &local_len);

    int remote_len = 0;
    unsigned char* remote = wx_get_address(&remote_saddr, &remote_len);

	httpd_add_connection(httpd, fd, local, local_len, remote, remote_len);
	return 1;
}

static void httpd_remove_connection(httpd_t *httpd, http_connection_t *connection)
{
	MUTEX_LOCK(httpd->run_mutex);
	if (connection->request) {
		http_request_destroy(connection->request);
		connection->request = NULL;
	}
	if (connection->user_data)
	{
		httpd->callbacks.conn_destroy(connection->user_data);
		connection->user_data = NULL;
	}
	shutdown(connection->socket_fd, SHUT_WR);
	closesocket(connection->socket_fd);
	connection->connected = 0;
	/* for mirroring server, only one connection */
	if (httpd->is_mirror)
	{
		httpd->stream_start = 0;
		//httpd->mirror_disconn = 0;
		uint64_t uniqueid = 0;
		memcpy(&uniqueid, connection->ip, 4);
		httpd->callbacks.set_mirror_stream(uniqueid, 0);
		httpd->callbacks.conn_mirror_destroy(connection->ip);
	}
	if (httpd->open_connections > 0)
	{
		httpd->open_connections--;
	}
	MUTEX_UNLOCK(httpd->run_mutex);
}

void httpd_remove_connection_new(httpd_t *httpd, uint64_t uniqueid)
{
	http_connection_t *connection = NULL;
	int i = 0;
	MUTEX_LOCK(httpd->run_mutex);
	for (; i < httpd->max_connections; i++)
	{
		connection = &httpd->connections[i];
		uint64_t uniqueidtmp = 0;
		memcpy(&uniqueidtmp, connection->ip, 4);
		if (connection->connected && (uniqueidtmp == uniqueid))
		{
			break;
		}
	}
	MUTEX_UNLOCK(httpd->run_mutex);
	if (connection != NULL)
	{
		httpd_remove_connection(httpd, connection);
	}
}

#define INT_32_MAX 0x100000000

//s -> us
uint64_t ntptopts(uint64_t ntp) {
	return (((ntp >> 32) & 0xffffffff) * 1000000) + ((ntp & 0xffffffff) * 1000 * 1000 / INT_32_MAX);
}

int byteutils_get_int(unsigned char* b, int offset) {
	return ((b[offset + 3] & 0xff) << 24) | ((b[offset + 2] & 0xff) << 16) | ((b[offset + 1] & 0xff) << 8) | (b[offset] & 0xff);
}

uint64_t byteutils_get_int2(unsigned char* b, int offset) {
	return ((uint64_t)(b[offset + 3] & 0xff) << 24) | ((uint64_t)(b[offset + 2] & 0xff) << 16) | ((uint64_t)(b[offset + 1] & 0xff) << 8) | ((uint64_t)b[offset] & 0xff);
}

uint64_t byteutils_get_long(unsigned char* b, int offset) {
	return (byteutils_get_int2(b, offset + 4)) << 32 | byteutils_get_int2(b, offset);
}

short byteutils_get_short(unsigned char* b, int offset) {
	return (short)((b[offset + 1] << 8) | (b[offset] & 0xff));
}

static THREAD_RETVAL
httpd_thread(void *arg)
{
	httpd_t *httpd = arg;
	//char buffer[1024 * 256];
	char buffer[1024] = {0};
	int i;
	unsigned char packet[128];
	memset(packet, 0, 128);
	unsigned int readstart = 0;

	unsigned char* payload_in = malloc(5 * 1024 * 1024);
	uint64_t pts_base = 0;
	uint64_t pts = 0;
	assert(httpd);
	while (1) {
		fd_set rfds;
		struct timeval tv;
		int nfds=0;
		int ret;

		MUTEX_LOCK(httpd->run_mutex);
		if (!httpd->running) {
			MUTEX_UNLOCK(httpd->run_mutex);
			break;
		}
		MUTEX_UNLOCK(httpd->run_mutex);

		/* Set timeout value to 5ms */
		tv.tv_sec = 1;
		tv.tv_usec = 1000;

		/* Get the correct nfds value and set rfds */
		FD_ZERO(&rfds);
		if (httpd->open_connections < httpd->max_connections) {
			if (httpd->server_fd4 != -1) {
				FD_SET(httpd->server_fd4, &rfds);
				if (nfds <= httpd->server_fd4) {
					nfds = httpd->server_fd4+1;
				}
			}
			if (httpd->server_fd6 != -1) {
				FD_SET(httpd->server_fd6, &rfds);
				if (nfds <= httpd->server_fd6) {
					nfds = httpd->server_fd6+1;
				}
			}
		}
		for (i=0; i<httpd->max_connections; i++) {
			int socket_fd;
			if (!httpd->connections[i].connected) {
				continue;
			}
			socket_fd = httpd->connections[i].socket_fd;
			FD_SET(socket_fd, &rfds);
			if (nfds <= socket_fd) {
				nfds = socket_fd+1;
			}
		}


		ret = select(nfds, &rfds, NULL, NULL, &tv);
		if (ret == 0) {
			/* Timeout happened */
			continue;
		} else if (ret == -1) {
			/* FIXME: Error happened */
			////WXLogWriteNew( "Error in select");
			break;
		}

		if (httpd->open_connections < httpd->max_connections &&
		    httpd->server_fd4 != -1 && FD_ISSET(httpd->server_fd4, &rfds)) {
			ret = httpd_accept_connection(httpd, httpd->server_fd4, 0);
			if (ret == -1) {
				break;
			} else if (ret == 0) {
				continue;
			}
		}
		if (httpd->open_connections < httpd->max_connections &&
		    httpd->server_fd6 != -1 && FD_ISSET(httpd->server_fd6, &rfds)) {
			ret = httpd_accept_connection(httpd, httpd->server_fd6, 1);
			if (ret == -1) {
				break;
			} else if (ret == 0) {
				continue;
			}
		}
		for (i=0; i<httpd->max_connections; i++) {
			http_connection_t *connection = &httpd->connections[i];

			if (!connection->connected) {
				continue;
			}
			if (!FD_ISSET(connection->socket_fd, &rfds)) {
				continue;
			}

			/* If not in the middle of request, allocate one */
			if (!connection->request) {
				connection->request = http_request_init();
				assert(connection->request);
			}
			memset(buffer, 0, sizeof(buffer));
			//check if or not disconnect
			uint64_t uniqueid = 0;
			memcpy(&uniqueid, connection->ip, 4);
			if (httpd_get_mirror_streaming(httpd) && httpd_get_disconnect(httpd, uniqueid))
			{
				httpd_remove_connection(httpd, connection);
				continue;
			}

			if (!httpd_get_mirror_streaming(httpd)) { //如果不是镜像
				ret = recv(connection->socket_fd, buffer, sizeof(buffer), 0);
				//logger_log(httpd->logger, LOGGER_DEBUG, "Receiving on socket %d, %d bytes, connection->ip: %s", connection->socket_fd, ret, connection->ip);
				if (ret == 0) {
					//logger_log(httpd->logger, LOGGER_INFO, "Connection closed for socket %d", connection->socket_fd);
					httpd_remove_connection(httpd, connection);
					continue;
				}
			}
			else {
				int stream_fd = connection->socket_fd;


				// packetlen初始0
				ret = recv(stream_fd, packet + readstart, 4 - readstart, 0);


				if (ret == 0) {
					/* TCP socket closed */
					//logger_log(raop_rtp_mirror->logger, LOGGER_INFO, "TCP socket closed");
					httpd_remove_connection(httpd, connection);
					break;
				}
				else if (ret == -1) {
					/* FIXME: Error happened */
					//logger_log(raop_rtp_mirror->logger, LOGGER_INFO, "Error in recv");
					httpd_remove_connection(httpd, connection);
					break;
				}

				readstart += ret;
				if (readstart < 4) {
					continue;
				}
				if ((packet[0] == 80 && packet[1] == 79 && packet[2] == 83 && packet[3] == 84) || (packet[0] == 71 && packet[1] == 69 && packet[2] == 84)) {
					// POST或者GET
					//logger_log(raop_rtp_mirror->logger, LOGGER_DEBUG, "handle http data");
				}
				else {
					// 普通数据块
					do {
						// 读取剩下的124字节
						ret = recv(stream_fd, packet + readstart, 128 - readstart, 0);
						readstart = readstart + ret;
					} while (readstart < 128);
					unsigned payloadsize = byteutils_get_int(packet, 0);
					// FIXME: 这里计算方式需要再确认
					short payloadtype = (short)(byteutils_get_short(packet, 4) & 0xff);
					short payloadoption = byteutils_get_short(packet, 6);

					// 处理内容数据
					if (payloadtype == 0) {
						uint64_t payloadntp = byteutils_get_long(packet, 8);
						// 读取时间
						if (pts_base == 0) {
							pts_base = ntptopts(payloadntp);
						}
						else {
							pts = ntptopts(payloadntp) - pts_base;
						}
					}
					//pts = WXGetTimeMS();
					
					//
					if (payloadsize > 0 && payloadsize < 0x500000)
					{
						// 这里是加密的数据
						memset(payload_in, 0, 128 + payloadsize + 1);
						memcpy(payload_in, packet, 128);
						readstart = 0;
						do {
							// payload数据
							ret = recv(stream_fd, 128 + payload_in + readstart, payloadsize - readstart, 0);
							readstart = readstart + ret;
						} while (readstart < payloadsize);

						if (payloadtype == 0 || (payloadtype & 255) == 1)
						{
							httpd->callbacks.conn_datafeed(connection->user_data, payload_in, payloadsize + 128, uniqueid, pts);
						}
					}
					
				}

				readstart = 0;

				memset(packet, 0, 128);
			}

			//ret = recv(connection->socket_fd, buffer, sizeof(buffer), 0);

			//////WXLogWriteNew("Receiving on socket %d, %d bytes, connection->ip: %s", connection->socket_fd, ret, connection->ip);
			//if (ret == 0) {
			//	//WXLogWriteNew( "Connection closed for socket %d", connection->socket_fd);
			//	httpd_remove_connection(httpd, connection);
			//	continue;
			//}
			//if (httpd_get_mirror_streaming(httpd)) 
			//{
			//	httpd->callbacks.conn_datafeed(connection->user_data, buffer, ret, uniqueid);
			//	continue;
			//}

			if (httpd->is_mirror == 0)
			{
				/* Parse HTTP request from data read from connection */
				http_request_add_data(connection->request, buffer, ret);
				if (http_request_has_error(connection->request)) {
					////WXLogWriteNew( "Error in parsing: %s", http_request_get_error_name(connection->request));
					printf("Error in parsing: %s", http_request_get_error_name(connection->request));
					httpd_remove_connection(httpd, connection);
					continue;
				}

				/* If request is finished, process and deallocate */
				if (http_request_is_complete(connection->request)) {
					http_response_t *response = NULL;

					httpd->callbacks.conn_request(connection->user_data, connection->request, &response);
					http_request_destroy(connection->request);
					connection->request = NULL;

					if (response) {
						const char *data;
						int datalen;
						int written;
						int ret;

						/* Get response data and datalen */
						data = http_response_get_data(response, &datalen);

						written = 0;
						while (written < datalen) {
							ret = send(connection->socket_fd, data + written, datalen - written, 0);
							if (ret == -1) {
								/* FIXME: Error happened */
								//WXLogWriteNew( "Error in sending data");
								break;
							}
							written += ret;
						}

						if (http_response_get_disconnect(response)) {
							//WXLogWriteNew( "Disconnecting on software request");
							httpd_remove_connection(httpd, connection);
						}
					}
					else {
						//WXLogWriteNew("Didn't get response");
						//httpd_remove_connection(httpd, connection);
					}
					http_response_destroy(response);
				}
				else {
					//WXLogWriteNew("Request not complete, waiting for more data...");
				}
			}

		}
	}

	/* Remove all connections that are still connected */
	for (i=0; i<httpd->max_connections; i++) {
		http_connection_t *connection = &httpd->connections[i];

		if (!connection->connected) {
			continue;
		}
		//WXLogWriteNew( "Removing connection for socket %d", connection->socket_fd);
		httpd_remove_connection(httpd, connection);
	}

	/* Close server sockets since they are not used any more */
	if (httpd->server_fd4 != -1) {
		closesocket(httpd->server_fd4);
		httpd->server_fd4 = -1;
	}
	if (httpd->server_fd6 != -1) {
		closesocket(httpd->server_fd6);
		httpd->server_fd6 = -1;
	}
	free(payload_in);
	//WXLogWriteNew( "Exiting HTTP thread");

	return 0;
}

EXTERN_C  int netutils_init_socket(unsigned short* port, int use_ipv6, int use_udp);

int
httpd_start(httpd_t *httpd, unsigned short *port, int iMirrorPort)
{
	/* How many connection attempts are kept in queue */
	int backlog = 5;

	assert(httpd);
	assert(port);

	MUTEX_CREATE(httpd->run_mutex);///////////////////////////////////////////////////////////////////////////////////

	MUTEX_LOCK(httpd->run_mutex);
	if (httpd->running || !httpd->joined) {
		MUTEX_UNLOCK(httpd->run_mutex);
		return 0;
	}

	httpd->server_fd6 = -1;
	httpd->server_fd4 = netutils_init_socket(port, 0, 0);
	if (httpd->server_fd4 == -1) {
		int iError = SOCKET_GET_ERROR();
		//WXLogWriteNew("Error initialising socket %d", iError);
		MUTEX_UNLOCK(httpd->run_mutex);
		if (iError == 10048 || iError == 10013)
		{
			return -3;
		}
		return -1;
	}
#ifdef ENABLE_IPV6
	httpd->server_fd6 = netutils_init_socket(port, 1, 0);
	if (httpd->server_fd6 == -1) {
		int iError = SOCKET_GET_ERROR();
		logger_log(httpd->logger, LOGGER_WARNING, "Error initialising IPv6 socket %d", iError);
		logger_log(httpd->logger, LOGGER_WARNING, "Continuing without IPv6 support");
		if (iError == 10048 || iError == 10013)
		{
			return -3;
		}
	}
#endif

	if (httpd->server_fd4 != -1 && listen(httpd->server_fd4, backlog) == -1) {
		//WXLogWriteNew("Error listening to IPv4 socket");
		closesocket(httpd->server_fd4);
		closesocket(httpd->server_fd6);
		MUTEX_UNLOCK(httpd->run_mutex);
		return -2;
	}
#ifdef ENABLE_IPV6
	if (httpd->server_fd6 != -1 && listen(httpd->server_fd6, backlog) == -1) {
		logger_log(httpd->logger, LOGGER_ERR, "Error listening to IPv6 socket");
		closesocket(httpd->server_fd4);
		closesocket(httpd->server_fd6);
		MUTEX_UNLOCK(httpd->run_mutex);
		return -2;
	}
#endif
	//WXLogWriteNew( "Initialized server socket(s)");

	/* Set values correctly and create new thread */
	httpd->running = 1;
	httpd->joined = 0;
	httpd->port = *port;
	httpd->is_mirror = iMirrorPort;
	httpd->stream_start = 0;
	//httpd->mirror_disconn = 0;
	THREAD_CREATE(httpd->thread, httpd_thread, httpd);
	MUTEX_UNLOCK(httpd->run_mutex);

	return 1;
}

int
httpd_is_running(httpd_t *httpd)
{
	int running;

	assert(httpd);

	MUTEX_LOCK(httpd->run_mutex);
	running = httpd->running || !httpd->joined;
	MUTEX_UNLOCK(httpd->run_mutex);

	return running;
}

void
httpd_stop(httpd_t *httpd)
{
	assert(httpd);

	MUTEX_LOCK(httpd->run_mutex);
	if (!httpd->running || httpd->joined) {
		MUTEX_UNLOCK(httpd->run_mutex);
		return;
	}
	httpd->running = 0;
	httpd->stream_start = 0;
	//httpd->mirror_disconn = 0;

	if (httpd->server_fd4 > 0) {
		closesocket(httpd->server_fd4);
		httpd->server_fd4 = -1;
	}

	MUTEX_UNLOCK(httpd->run_mutex);

	THREAD_JOIN(httpd->thread);

	MUTEX_LOCK(httpd->run_mutex);
	httpd->joined = 1;
	MUTEX_UNLOCK(httpd->run_mutex);
}

int
httpd_get_mirror_streaming(httpd_t *httpd)
{
	int status;

	assert(httpd);

	MUTEX_LOCK(httpd->run_mutex);
	//status = httpd->is_mirror && httpd->stream_start;
	status = httpd->is_mirror;
	MUTEX_UNLOCK(httpd->run_mutex);

	return status;
}

void httpd_disconnect(httpd_t *httpd, uint64_t uniqueid)
{
	assert(httpd);
	MUTEX_LOCK(httpd->run_mutex);
	//httpd->mirror_disconn = 1;
	httpd->callbacks.set_mirror_stream(uniqueid, 1);
	MUTEX_UNLOCK(httpd->run_mutex);
}

int httpd_get_disconnect(httpd_t *httpd, uint64_t uniqueid)
{
	int status;
	assert(httpd);
	MUTEX_LOCK(httpd->run_mutex);
	//status = httpd->mirror_disconn;
	status = httpd->callbacks.get_mirror_stream(uniqueid);
	MUTEX_UNLOCK(httpd->run_mutex);
	return status;
}

void
httpd_set_mirror_streaming(httpd_t *httpd, uint64_t uniqueid)
{
	//assert(httpd && httpd->is_mirror);
	if (httpd != NULL && httpd->is_mirror)
	{
		MUTEX_LOCK(httpd->run_mutex);
		//httpd->stream_start = 1;
		//httpd->mirror_disconn = 0;
		httpd->callbacks.set_mirror_stream(uniqueid, 0);
		MUTEX_UNLOCK(httpd->run_mutex);
	}
}


/* Based on src/http/ngx_http_parse.c from NGINX copyright Igor Sysoev
 *
 * Additional changes are licensed under the same terms as NGINX and
 * copyright Joyent, Inc. and other Node contributors. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <assert.h>
#include <stddef.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#ifndef ULLONG_MAX
# define ULLONG_MAX ((uint64_t) -1) /* 2^64-1 */
#endif

#ifndef MIN
# define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif



#ifndef BIT_AT
# define BIT_AT(a, i)                                                \
  (!!((unsigned int) (a)[(unsigned int) (i) >> 3] &                  \
   (1 << ((unsigned int) (i) & 7))))
#endif

#ifndef ELEM_AT
# define ELEM_AT(a, i, v) ((unsigned int) (i) < _ARRAY_SIZE(a) ? (a)[(i)] : (v))
#endif

#if HTTP_PARSER_DEBUG
#define SET_ERRNO(e)                                                 \
do {                                                                 \
  parser->http_errno = (e);                                          \
  parser->error_lineno = __LINE__;                                   \
} while (0)
#else
#define SET_ERRNO(e)                                                 \
do {                                                                 \
  parser->http_errno = (e);                                          \
} while(0)
#endif


 /* Run the notify callback FOR, returning ER if it fails */
#define CALLBACK_NOTIFY_(FOR, ER)                                    \
do {                                                                 \
  assert(HTTP_PARSER_ERRNO(parser) == HPE_OK);                       \
                                                                     \
  if (settings->on_##FOR) {                                          \
    if (0 != settings->on_##FOR(parser)) {                           \
      SET_ERRNO(HPE_CB_##FOR);                                       \
    }                                                                \
                                                                     \
    /* We either errored above or got paused; get out */             \
    if (HTTP_PARSER_ERRNO(parser) != HPE_OK) {                       \
      return (ER);                                                   \
    }                                                                \
  }                                                                  \
} while (0)

/* Run the notify callback FOR and consume the current byte */
#define CALLBACK_NOTIFY(FOR)            CALLBACK_NOTIFY_(FOR, p - data + 1)

/* Run the notify callback FOR and don't consume the current byte */
#define CALLBACK_NOTIFY_NOADVANCE(FOR)  CALLBACK_NOTIFY_(FOR, p - data)

/* Run data callback FOR with LEN bytes, returning ER if it fails */
#define CALLBACK_DATA_(FOR, LEN, ER)                                 \
do {                                                                 \
  assert(HTTP_PARSER_ERRNO(parser) == HPE_OK);                       \
                                                                     \
  if (FOR##_mark) {                                                  \
    if (settings->on_##FOR) {                                        \
      if (0 != settings->on_##FOR(parser, FOR##_mark, (LEN))) {      \
        SET_ERRNO(HPE_CB_##FOR);                                     \
      }                                                              \
                                                                     \
      /* We either errored above or got paused; get out */           \
      if (HTTP_PARSER_ERRNO(parser) != HPE_OK) {                     \
        return (ER);                                                 \
      }                                                              \
    }                                                                \
    FOR##_mark = NULL;                                               \
  }                                                                  \
} while (0)

/* Run the data callback FOR and consume the current byte */
#define CALLBACK_DATA(FOR)                                           \
    CALLBACK_DATA_(FOR, p - FOR##_mark, p - data + 1)

/* Run the data callback FOR and don't consume the current byte */
#define CALLBACK_DATA_NOADVANCE(FOR)                                 \
    CALLBACK_DATA_(FOR, p - FOR##_mark, p - data)

/* Set the mark FOR; non-destructive if mark is already set */
#define MARK(FOR)                                                    \
do {                                                                 \
  if (!FOR##_mark) {                                                 \
    FOR##_mark = p;                                                  \
  }                                                                  \
} while (0)


#define PROXY_CONNECTION "proxy-connection"
#define CONNECTION "connection"
#define CONTENT_LENGTH "content-length"
#define TRANSFER_ENCODING "transfer-encoding"
#define UPGRADE "upgrade"
#define CHUNKED "chunked"
#define KEEP_ALIVE "keep-alive"
#define CLOSE "close"


static const char* method_strings[] =
{
#define XX(num, name, string) #string,
  HTTP_METHOD_MAP(XX)
#undef XX
};


/* Tokens as defined by rfc 2616. Also lowercases them.
 *        token       = 1*<any CHAR except CTLs or separators>
 *     separators     = "(" | ")" | "<" | ">" | "@"
 *                    | "," | ";" | ":" | "\" | <">
 *                    | "/" | "[" | "]" | "?" | "="
 *                    | "{" | "}" | SP | HT
 */
static const char tokens[256] = {
    /*   0 nul    1 soh    2 stx    3 etx    4 eot    5 enq    6 ack    7 bel  */
            0,       0,       0,       0,       0,       0,       0,       0,
            /*   8 bs     9 ht    10 nl    11 vt    12 np    13 cr    14 so    15 si   */
                    0,       0,       0,       0,       0,       0,       0,       0,
                    /*  16 dle   17 dc1   18 dc2   19 dc3   20 dc4   21 nak   22 syn   23 etb */
                            0,       0,       0,       0,       0,       0,       0,       0,
                            /*  24 can   25 em    26 sub   27 esc   28 fs    29 gs    30 rs    31 us  */
                                    0,       0,       0,       0,       0,       0,       0,       0,
                                    /*  32 sp    33  !    34  "    35  #    36  $    37  %    38  &    39  '  */
                                            0,      '!',      0,      '#',     '$',     '%',     '&',    '\'',
                                            /*  40  (    41  )    42  *    43  +    44  ,    45  -    46  .    47  /  */
                                                    0,       0,      '*',     '+',      0,      '-',     '.',      0,
                                                    /*  48  0    49  1    50  2    51  3    52  4    53  5    54  6    55  7  */
                                                           '0',     '1',     '2',     '3',     '4',     '5',     '6',     '7',
                                                           /*  56  8    57  9    58  :    59  ;    60  <    61  =    62  >    63  ?  */
                                                                  '8',     '9',      0,       0,       0,       0,       0,       0,
                                                                  /*  64  @    65  A    66  B    67  C    68  D    69  E    70  F    71  G  */
                                                                          0,      'a',     'b',     'c',     'd',     'e',     'f',     'g',
                                                                          /*  72  H    73  I    74  J    75  K    76  L    77  M    78  N    79  O  */
                                                                                 'h',     'i',     'j',     'k',     'l',     'm',     'n',     'o',
                                                                                 /*  80  P    81  Q    82  R    83  S    84  T    85  U    86  V    87  W  */
                                                                                        'p',     'q',     'r',     's',     't',     'u',     'v',     'w',
                                                                                        /*  88  X    89  Y    90  Z    91  [    92  \    93  ]    94  ^    95  _  */
                                                                                               'x',     'y',     'z',      0,       0,       0,      '^',     '_',
                                                                                               /*  96  `    97  a    98  b    99  c   100  d   101  e   102  f   103  g  */
                                                                                                      '`',     'a',     'b',     'c',     'd',     'e',     'f',     'g',
                                                                                                      /* 104  h   105  i   106  j   107  k   108  l   109  m   110  n   111  o  */
                                                                                                             'h',     'i',     'j',     'k',     'l',     'm',     'n',     'o',
                                                                                                             /* 112  p   113  q   114  r   115  s   116  t   117  u   118  v   119  w  */
                                                                                                                    'p',     'q',     'r',     's',     't',     'u',     'v',     'w',
                                                                                                                    /* 120  x   121  y   122  z   123  {   124  |   125  }   126  ~   127 del */
                                                                                                                           'x',     'y',     'z',      0,      '|',      0,      '~',       0 };


static const int8_t unhex[256] =
{ -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,-1,-1,-1,-1,-1,-1
,-1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1
,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
,-1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1
,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
};


#if HTTP_PARSER_STRICT
# define T(v) 0
#else
# define T(v) v
#endif


static const uint8_t normal_url_char[32] = {
    /*   0 nul    1 soh    2 stx    3 etx    4 eot    5 enq    6 ack    7 bel  */
            0 | 0 | 0 | 0 | 0 | 0 | 0 | 0,
            /*   8 bs     9 ht    10 nl    11 vt    12 np    13 cr    14 so    15 si   */
                    0 | T(2) | 0 | 0 | T(16) | 0 | 0 | 0,
                    /*  16 dle   17 dc1   18 dc2   19 dc3   20 dc4   21 nak   22 syn   23 etb */
                            0 | 0 | 0 | 0 | 0 | 0 | 0 | 0,
                            /*  24 can   25 em    26 sub   27 esc   28 fs    29 gs    30 rs    31 us  */
                                    0 | 0 | 0 | 0 | 0 | 0 | 0 | 0,
                                    /*  32 sp    33  !    34  "    35  #    36  $    37  %    38  &    39  '  */
                                            0 | 2 | 4 | 0 | 16 | 32 | 64 | 128,
                                            /*  40  (    41  )    42  *    43  +    44  ,    45  -    46  .    47  /  */
                                                    1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
                                                    /*  48  0    49  1    50  2    51  3    52  4    53  5    54  6    55  7  */
                                                            1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
                                                            /*  56  8    57  9    58  :    59  ;    60  <    61  =    62  >    63  ?  */
                                                                    1 | 2 | 4 | 8 | 16 | 32 | 64 | 0,
                                                                    /*  64  @    65  A    66  B    67  C    68  D    69  E    70  F    71  G  */
                                                                            1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
                                                                            /*  72  H    73  I    74  J    75  K    76  L    77  M    78  N    79  O  */
                                                                                    1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
                                                                                    /*  80  P    81  Q    82  R    83  S    84  T    85  U    86  V    87  W  */
                                                                                            1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
                                                                                            /*  88  X    89  Y    90  Z    91  [    92  \    93  ]    94  ^    95  _  */
                                                                                                    1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
                                                                                                    /*  96  `    97  a    98  b    99  c   100  d   101  e   102  f   103  g  */
                                                                                                            1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
                                                                                                            /* 104  h   105  i   106  j   107  k   108  l   109  m   110  n   111  o  */
                                                                                                                    1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
                                                                                                                    /* 112  p   113  q   114  r   115  s   116  t   117  u   118  v   119  w  */
                                                                                                                            1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
                                                                                                                            /* 120  x   121  y   122  z   123  {   124  |   125  }   126  ~   127 del */
                                                                                                                                    1 | 2 | 4 | 8 | 16 | 32 | 64 | 0, };

#undef T

enum state
{
    s_dead = 1 /* important that this is > 0 */

    , s_start_req_or_res
    , s_res_or_resp_H
    , s_start_res
    , s_res_H
    , s_res_HT
    , s_res_HTT
    , s_res_HTTP
    , s_res_first_http_major
    , s_res_http_major
    , s_res_first_http_minor
    , s_res_http_minor
    , s_res_first_status_code
    , s_res_status_code
    , s_res_status
    , s_res_line_almost_done

    , s_start_req

    , s_req_method
    , s_req_spaces_before_url
    , s_req_schema
    , s_req_schema_slash
    , s_req_schema_slash_slash
    , s_req_server_start
    , s_req_server
    , s_req_server_with_at
    , s_req_path
    , s_req_query_string_start
    , s_req_query_string
    , s_req_fragment_start
    , s_req_fragment
    , s_req_http_start
    , s_req_http_H
    , s_req_http_HT
    , s_req_http_HTT
    , s_req_http_HTTP
    , s_req_first_http_major
    , s_req_http_major
    , s_req_first_http_minor
    , s_req_http_minor
    , s_req_line_almost_done

    , s_header_field_start
    , s_header_field
    , s_header_value_start
    , s_header_value
    , s_header_value_lws

    , s_header_almost_done

    , s_chunk_size_start
    , s_chunk_size
    , s_chunk_parameters
    , s_chunk_size_almost_done

    , s_headers_almost_done
    , s_headers_done

    /* Important: 's_headers_done' must be the last 'header' state. All
     * states beyond this must be 'body' states. It is used for overflow
     * checking. See the PARSING_HEADER() macro.
     */

    , s_chunk_data
    , s_chunk_data_almost_done
    , s_chunk_data_done

    , s_body_identity
    , s_body_identity_eof

    , s_message_done
};


#define PARSING_HEADER(state) (state <= s_headers_done)


enum header_states
{
    h_general = 0
    , h_C
    , h_CO
    , h_CON

    , h_matching_connection
    , h_matching_proxy_connection
    , h_matching_content_length
    , h_matching_transfer_encoding
    , h_matching_upgrade

    , h_connection
    , h_content_length
    , h_transfer_encoding
    , h_upgrade

    , h_matching_transfer_encoding_chunked
    , h_matching_connection_keep_alive
    , h_matching_connection_close

    , h_transfer_encoding_chunked
    , h_connection_keep_alive
    , h_connection_close
};

enum http_host_state
{
    s_http_host_dead = 1
    , s_http_userinfo_start
    , s_http_userinfo
    , s_http_host_start
    , s_http_host_v6_start
    , s_http_host
    , s_http_host_v6
    , s_http_host_v6_end
    , s_http_host_port_start
    , s_http_host_port
};

/* Macros for character classes; depends on strict-mode  */
#define CR                  '\r'
#define LF                  '\n'
#define LOWER(c)            (unsigned char)(c | 0x20)
#define IS_ALPHA(c)         (LOWER(c) >= 'a' && LOWER(c) <= 'z')
#define IS_NUM(c)           ((c) >= '0' && (c) <= '9')
#define IS_ALPHANUM(c)      (IS_ALPHA(c) || IS_NUM(c))
#define IS_HEX(c)           (IS_NUM(c) || (LOWER(c) >= 'a' && LOWER(c) <= 'f'))
#define IS_MARK(c)          ((c) == '-' || (c) == '_' || (c) == '.' || \
  (c) == '!' || (c) == '~' || (c) == '*' || (c) == '\'' || (c) == '(' || \
  (c) == ')')
#define IS_USERINFO_CHAR(c) (IS_ALPHANUM(c) || IS_MARK(c) || (c) == '%' || \
  (c) == ';' || (c) == ':' || (c) == '&' || (c) == '=' || (c) == '+' || \
  (c) == '$' || (c) == ',')

#if HTTP_PARSER_STRICT
#define TOKEN(c)            (tokens[(unsigned char)c])
#define IS_URL_CHAR(c)      (BIT_AT(normal_url_char, (unsigned char)c))
#define IS_HOST_CHAR(c)     (IS_ALPHANUM(c) || (c) == '.' || (c) == '-')
#else
#define TOKEN(c)            ((c == ' ') ? ' ' : tokens[(unsigned char)c])
#define IS_URL_CHAR(c)                                                         \
  (BIT_AT(normal_url_char, (unsigned char)c) || ((c) & 0x80))
#define IS_HOST_CHAR(c)                                                        \
  (IS_ALPHANUM(c) || (c) == '.' || (c) == '-' || (c) == '_')
#endif


#define start_state (parser->type == HTTP_REQUEST ? s_start_req : s_start_res)


#if HTTP_PARSER_STRICT
# define STRICT_CHECK(cond)                                          \
do {                                                                 \
  if (cond) {                                                        \
    SET_ERRNO(HPE_STRICT);                                           \
    goto error;                                                      \
  }                                                                  \
} while (0)
# define NEW_MESSAGE() (http_should_keep_alive(parser) ? start_state : s_dead)
#else
# define STRICT_CHECK(cond)
# define NEW_MESSAGE() start_state
#endif


/* Map errno values to strings for human-readable output */
#define HTTP_STRERROR_GEN(n, s) { "HPE_" #n, s },
static struct {
    const char* name;
    const char* description;
} http_strerror_tab[] = {
  HTTP_ERRNO_MAP(HTTP_STRERROR_GEN)
};
#undef HTTP_STRERROR_GEN

int http_message_needs_eof(const http_parser* parser);

/* Our URL parser.
 *
 * This is designed to be shared by http_parser_execute() for URL validation,
 * hence it has a state transition + byte-for-byte interface. In addition, it
 * is meant to be embedded in http_parser_parse_url(), which does the dirty
 * work of turning state transitions URL components for its API.
 *
 * This function should only be invoked with non-space characters. It is
 * assumed that the caller cares about (and can detect) the transition between
 * URL and non-URL states by looking for these.
 */
static enum state
parse_url_char(enum state s, const char ch)
{
    if (ch == ' ' || ch == '\r' || ch == '\n') {
        return s_dead;
    }

#if HTTP_PARSER_STRICT
    if (ch == '\t' || ch == '\f') {
        return s_dead;
    }
#endif

    switch (s) {
    case s_req_spaces_before_url:
        /* Proxied requests are followed by scheme of an absolute URI (alpha).
         * All methods except CONNECT are followed by '/' or '*'.
         */

        if (ch == '/' || ch == '*') {
            return s_req_path;
        }

        if (IS_ALPHA(ch)) {
            return s_req_schema;
        }

        break;

    case s_req_schema:
        if (IS_ALPHA(ch)) {
            return s;
        }

        if (ch == ':') {
            return s_req_schema_slash;
        }

        break;

    case s_req_schema_slash:
        if (ch == '/') {
            return s_req_schema_slash_slash;
        }

        break;

    case s_req_schema_slash_slash:
        if (ch == '/') {
            return s_req_server_start;
        }

        break;

    case s_req_server_with_at:
        if (ch == '@') {
            return s_dead;
        }

        /* FALLTHROUGH */
    case s_req_server_start:
    case s_req_server:
        if (ch == '/') {
            return s_req_path;
        }

        if (ch == '?') {
            return s_req_query_string_start;
        }

        if (ch == '@') {
            return s_req_server_with_at;
        }

        if (IS_USERINFO_CHAR(ch) || ch == '[' || ch == ']') {
            return s_req_server;
        }

        break;

    case s_req_path:
        if (IS_URL_CHAR(ch)) {
            return s;
        }

        switch (ch) {
        case '?':
            return s_req_query_string_start;

        case '#':
            return s_req_fragment_start;
        }

        break;

    case s_req_query_string_start:
    case s_req_query_string:
        if (IS_URL_CHAR(ch)) {
            return s_req_query_string;
        }

        switch (ch) {
        case '?':
            /* allow extra '?' in query string */
            return s_req_query_string;

        case '#':
            return s_req_fragment_start;
        }

        break;

    case s_req_fragment_start:
        if (IS_URL_CHAR(ch)) {
            return s_req_fragment;
        }

        switch (ch) {
        case '?':
            return s_req_fragment;

        case '#':
            return s;
        }

        break;

    case s_req_fragment:
        if (IS_URL_CHAR(ch)) {
            return s;
        }

        switch (ch) {
        case '?':
        case '#':
            return s;
        }

        break;

    default:
        break;
    }

    /* We should never fall out of the switch above unless there's an error */
    return s_dead;
}

size_t http_parser_execute(http_parser* parser,
    const http_parser_settings* settings,
    const char* data,
    size_t len)
{
    char c, ch;
    int8_t unhex_val;
    const char* p = data;
    const char* header_field_mark = 0;
    const char* header_value_mark = 0;
    const char* url_mark = 0;
    const char* body_mark = 0;

    /* We're in an error state. Don't bother doing anything. */
    if (HTTP_PARSER_ERRNO(parser) != HPE_OK) {
        return 0;
    }

    if (len == 0) {
        switch (parser->state) {
        case s_body_identity_eof:
            /* Use of CALLBACK_NOTIFY() here would erroneously return 1 byte read if
             * we got paused.
             */
            CALLBACK_NOTIFY_NOADVANCE(message_complete);
            return 0;

        case s_dead:
        case s_start_req_or_res:
        case s_start_res:
        case s_start_req:
            return 0;

        default:
            SET_ERRNO(HPE_INVALID_EOF_STATE);
            return 1;
        }
    }


    if (parser->state == s_header_field)
        header_field_mark = data;
    if (parser->state == s_header_value)
        header_value_mark = data;
    switch (parser->state) {
    case s_req_path:
    case s_req_schema:
    case s_req_schema_slash:
    case s_req_schema_slash_slash:
    case s_req_server_start:
    case s_req_server:
    case s_req_server_with_at:
    case s_req_query_string_start:
    case s_req_query_string:
    case s_req_fragment_start:
    case s_req_fragment:
        url_mark = data;
        break;
    }

    for (p = data; p != data + len; p++) {
        ch = *p;

        if (PARSING_HEADER(parser->state)) {
            ++parser->nread;
            /* Buffer overflow attack */
            if (parser->nread > HTTP_MAX_HEADER_SIZE) {
                SET_ERRNO(HPE_HEADER_OVERFLOW);
                goto error;
            }
        }

    reexecute_byte:
        switch (parser->state) {

        case s_dead:
            /* this state is used after a 'Connection: close' message
             * the parser will error out if it reads another message
             */
            if (ch == CR || ch == LF)
                break;

            SET_ERRNO(HPE_CLOSED_CONNECTION);
            goto error;

        case s_start_req_or_res:
        {
            if (ch == CR || ch == LF)
                break;
            parser->flags = 0;
            parser->content_length = ULLONG_MAX;

            if (ch == 'H') {
                parser->state = s_res_or_resp_H;

                CALLBACK_NOTIFY(message_begin);
            }
            else {
                parser->type = HTTP_REQUEST;
                parser->state = s_start_req;
                goto reexecute_byte;
            }

            break;
        }

        case s_res_or_resp_H:
            if (ch == 'T') {
                parser->type = HTTP_RESPONSE;
                parser->state = s_res_HT;
            }
            else {
                if (ch != 'E') {
                    SET_ERRNO(HPE_INVALID_CONSTANT);
                    goto error;
                }

                parser->type = HTTP_REQUEST;
                parser->method = HTTP_HEAD;
                parser->index = 2;
                parser->state = s_req_method;
            }
            break;

        case s_start_res:
        {
            parser->flags = 0;
            parser->content_length = ULLONG_MAX;

            switch (ch) {
            case 'H':
                parser->state = s_res_H;
                break;

            case CR:
            case LF:
                break;

            default:
                SET_ERRNO(HPE_INVALID_CONSTANT);
                goto error;
            }

            CALLBACK_NOTIFY(message_begin);
            break;
        }

        case s_res_H:
            STRICT_CHECK(ch != 'T');
            parser->state = s_res_HT;
            break;

        case s_res_HT:
            STRICT_CHECK(ch != 'T');
            parser->state = s_res_HTT;
            break;

        case s_res_HTT:
            STRICT_CHECK(ch != 'P');
            parser->state = s_res_HTTP;
            break;

        case s_res_HTTP:
            STRICT_CHECK(ch != '/');
            parser->state = s_res_first_http_major;
            break;

        case s_res_first_http_major:
            if (ch < '0' || ch > '9') {
                SET_ERRNO(HPE_INVALID_VERSION);
                goto error;
            }

            parser->http_major = ch - '0';
            parser->state = s_res_http_major;
            break;

            /* major HTTP version or dot */
        case s_res_http_major:
        {
            if (ch == '.') {
                parser->state = s_res_first_http_minor;
                break;
            }

            if (!IS_NUM(ch)) {
                SET_ERRNO(HPE_INVALID_VERSION);
                goto error;
            }

            parser->http_major *= 10;
            parser->http_major += ch - '0';

            if (parser->http_major > 999) {
                SET_ERRNO(HPE_INVALID_VERSION);
                goto error;
            }

            break;
        }

        /* first digit of minor HTTP version */
        case s_res_first_http_minor:
            if (!IS_NUM(ch)) {
                SET_ERRNO(HPE_INVALID_VERSION);
                goto error;
            }

            parser->http_minor = ch - '0';
            parser->state = s_res_http_minor;
            break;

            /* minor HTTP version or end of request line */
        case s_res_http_minor:
        {
            if (ch == ' ') {
                parser->state = s_res_first_status_code;
                break;
            }

            if (!IS_NUM(ch)) {
                SET_ERRNO(HPE_INVALID_VERSION);
                goto error;
            }

            parser->http_minor *= 10;
            parser->http_minor += ch - '0';

            if (parser->http_minor > 999) {
                SET_ERRNO(HPE_INVALID_VERSION);
                goto error;
            }

            break;
        }

        case s_res_first_status_code:
        {
            if (!IS_NUM(ch)) {
                if (ch == ' ') {
                    break;
                }

                SET_ERRNO(HPE_INVALID_STATUS);
                goto error;
            }
            parser->status_code = ch - '0';
            parser->state = s_res_status_code;
            break;
        }

        case s_res_status_code:
        {
            if (!IS_NUM(ch)) {
                switch (ch) {
                case ' ':
                    parser->state = s_res_status;
                    break;
                case CR:
                    parser->state = s_res_line_almost_done;
                    break;
                case LF:
                    parser->state = s_header_field_start;
                    break;
                default:
                    SET_ERRNO(HPE_INVALID_STATUS);
                    goto error;
                }
                break;
            }

            parser->status_code *= 10;
            parser->status_code += ch - '0';

            if (parser->status_code > 999) {
                SET_ERRNO(HPE_INVALID_STATUS);
                goto error;
            }

            break;
        }

        case s_res_status:
            /* the human readable status. e.g. "NOT FOUND"
             * we are not humans so just ignore this */
            if (ch == CR) {
                parser->state = s_res_line_almost_done;
                break;
            }

            if (ch == LF) {
                parser->state = s_header_field_start;
                break;
            }
            break;

        case s_res_line_almost_done:
            STRICT_CHECK(ch != LF);
            parser->state = s_header_field_start;
            break;

        case s_start_req:
        {
            if (ch == CR || ch == LF)
                break;
            parser->flags = 0;
            parser->content_length = ULLONG_MAX;

            if (!IS_ALPHA(ch)) {
                SET_ERRNO(HPE_INVALID_METHOD);
                goto error;
            }

            parser->method = (enum http_method)0;
            parser->index = 1;
            switch (ch) {
            case 'A': parser->method = HTTP_ANNOUNCE; break;
            case 'C': parser->method = HTTP_CONNECT; /* or COPY, CHECKOUT */ break;
            case 'D': parser->method = HTTP_DELETE; /* or DESCRIBE */ break;
            case 'F': parser->method = HTTP_FLUSH; break;
            case 'G': parser->method = HTTP_GET; /* or GET_PARAMETER */ break;
            case 'H': parser->method = HTTP_HEAD; break;
            case 'L': parser->method = HTTP_LOCK; break;
            case 'M': parser->method = HTTP_MKCOL; /* or MOVE, MKACTIVITY, MERGE, M-SEARCH */ break;
            case 'N': parser->method = HTTP_NOTIFY; break;
            case 'O': parser->method = HTTP_OPTIONS; break;
            case 'P': parser->method = HTTP_POST;
                /* or PROPFIND|PROPPATCH|PUT|PATCH|PURGE|PLAY|PAUSE */
                break;
            case 'R': parser->method = HTTP_REPORT; /* or REDIRECT, RECORD */ break;
            case 'S': parser->method = HTTP_SUBSCRIBE; /* or SEARCH, SETUP, SET_PARAMETER */ break;
            case 'T': parser->method = HTTP_TRACE; /* or TEARDOWN */ break;
            case 'U': parser->method = HTTP_UNLOCK; /* or UNSUBSCRIBE */ break;
            default:
                SET_ERRNO(HPE_INVALID_METHOD);
                goto error;
            }
            parser->state = s_req_method;

            CALLBACK_NOTIFY(message_begin);

            break;
        }

        case s_req_method:
        {
            const char* matcher;
            if (ch == '\0') {
                SET_ERRNO(HPE_INVALID_METHOD);
                goto error;
            }

            matcher = method_strings[parser->method];
            if (ch == ' ' && matcher[parser->index] == '\0') {
                parser->state = s_req_spaces_before_url;
            }
            else if (ch == matcher[parser->index]) {
                ; /* nada */
            }
            else if (parser->method == HTTP_CONNECT) {
                if (parser->index == 1 && ch == 'H') {
                    parser->method = HTTP_CHECKOUT;
                }
                else if (parser->index == 2 && ch == 'P') {
                    parser->method = HTTP_COPY;
                }
                else {
                    goto error;
                }
            }
            else if (parser->index == 2 && parser->method == HTTP_DELETE && ch == 'S') {
                parser->method = HTTP_DESCRIBE;
            }
            else if (parser->index == 3 && parser->method == HTTP_GET && ch == '_') {
                parser->method = HTTP_GET_PARAMETER;
            }
            else if (parser->method == HTTP_MKCOL) {
                if (parser->index == 1 && ch == 'O') {
                    parser->method = HTTP_MOVE;
                }
                else if (parser->index == 1 && ch == 'E') {
                    parser->method = HTTP_MERGE;
                }
                else if (parser->index == 1 && ch == '-') {
                    parser->method = HTTP_MSEARCH;
                }
                else if (parser->index == 2 && ch == 'A') {
                    parser->method = HTTP_MKACTIVITY;
                }
                else {
                    goto error;
                }
            }
            else if (parser->method == HTTP_SUBSCRIBE) {
                if (parser->index == 1 && ch == 'E') {
                    parser->method = HTTP_SEARCH; /* or HTTP_SETUP or HTTP_SET_PARAMETER */
                }
                else {
                    goto error;
                }
            }
            else if (parser->method == HTTP_TRACE) {
                if (parser->index == 1 && ch == 'E') {
                    parser->method = HTTP_TEARDOWN;
                }
                else {
                    goto error;
                }
            }
            else if (parser->index == 1 && parser->method == HTTP_POST) {
                if (ch == 'R') {
                    parser->method = HTTP_PROPFIND; /* or HTTP_PROPPATCH */
                }
                else if (ch == 'U') {
                    parser->method = HTTP_PUT; /* or HTTP_PURGE */
                }
                else if (ch == 'A') {
                    parser->method = HTTP_PATCH; /* or HTTP_PAUSE */
                }
                else if (ch == 'L') {
                    parser->method = HTTP_PLAY;
                }
                else {
                    goto error;
                }
            }
            else if (parser->index == 2) {
                if (parser->method == HTTP_PUT) {
                    if (ch == 'R') parser->method = HTTP_PURGE;
                }
                else if (parser->method == HTTP_PATCH) {
                    if (ch == 'U') parser->method = HTTP_PAUSE;
                }
                else if (parser->method == HTTP_REPORT && ch == 'D') {
                    parser->method = HTTP_REDIRECT;
                }
                else if (parser->method == HTTP_REPORT && ch == 'C') {
                    parser->method = HTTP_RECORD;
                }
                else if (parser->method == HTTP_SEARCH) {
                    if (ch == 'T') parser->method = HTTP_SETUP; /* or HTTP_SET_PARAMETER */
                }
                else if (parser->method == HTTP_UNLOCK) {
                    if (ch == 'S') parser->method = HTTP_UNSUBSCRIBE;
                }
                else {
                    goto error;
                }
            }
            else if (parser->index == 3 && parser->method == HTTP_SETUP && ch == '_') {
                parser->method = HTTP_SET_PARAMETER;
            }
            else if (parser->index == 4 && parser->method == HTTP_PROPFIND && ch == 'P') {
                parser->method = HTTP_PROPPATCH;
            }
            else {
                SET_ERRNO(HPE_INVALID_METHOD);
                goto error;
            }

            ++parser->index;
            break;
        }

        case s_req_spaces_before_url:
        {
            if (ch == ' ') break;

            MARK(url);
            if (parser->method == HTTP_CONNECT) {
                parser->state = s_req_server_start;
            }

            parser->state = parse_url_char((enum state)parser->state, ch);
            if (parser->state == s_dead) {
                SET_ERRNO(HPE_INVALID_URL);
                goto error;
            }

            break;
        }

        case s_req_schema:
        case s_req_schema_slash:
        case s_req_schema_slash_slash:
        case s_req_server_start:
        {
            switch (ch) {
                /* No whitespace allowed here */
            case ' ':
            case CR:
            case LF:
                SET_ERRNO(HPE_INVALID_URL);
                goto error;
            default:
                parser->state = parse_url_char((enum state)parser->state, ch);
                if (parser->state == s_dead) {
                    SET_ERRNO(HPE_INVALID_URL);
                    goto error;
                }
            }

            break;
        }

        case s_req_server:
        case s_req_server_with_at:
        case s_req_path:
        case s_req_query_string_start:
        case s_req_query_string:
        case s_req_fragment_start:
        case s_req_fragment:
        {
            switch (ch) {
            case ' ':
                parser->state = s_req_http_start;
                CALLBACK_DATA(url);
                break;
            case CR:
            case LF:
                parser->http_major = 0;
                parser->http_minor = 9;
                parser->state = (ch == CR) ?
                    s_req_line_almost_done :
                    s_header_field_start;
                CALLBACK_DATA(url);
                break;
            default:
                parser->state = parse_url_char((enum state)parser->state, ch);
                if (parser->state == s_dead) {
                    SET_ERRNO(HPE_INVALID_URL);
                    goto error;
                }
            }
            break;
        }

        case s_req_http_start:
            switch (ch) {
            case 'H':
            case 'R':
                parser->state = s_req_http_H;
                break;
            case ' ':
                break;
            default:
                SET_ERRNO(HPE_INVALID_CONSTANT);
                goto error;
            }
            break;

        case s_req_http_H:
            STRICT_CHECK(ch != 'T');
            parser->state = s_req_http_HT;
            break;

        case s_req_http_HT:
            STRICT_CHECK(ch != 'T');
            parser->state = s_req_http_HTT;
            break;

        case s_req_http_HTT:
            STRICT_CHECK(ch != 'P');
            parser->state = s_req_http_HTTP;
            break;

        case s_req_http_HTTP:
            STRICT_CHECK(ch != '/');
            parser->state = s_req_first_http_major;
            break;

            /* first digit of major HTTP version */
        case s_req_first_http_major:
            if (ch < '1' || ch > '9') {
                SET_ERRNO(HPE_INVALID_VERSION);
                goto error;
            }

            parser->http_major = ch - '0';
            parser->state = s_req_http_major;
            break;

            /* major HTTP version or dot */
        case s_req_http_major:
        {
            if (ch == '.') {
                parser->state = s_req_first_http_minor;
                break;
            }

            if (!IS_NUM(ch)) {
                SET_ERRNO(HPE_INVALID_VERSION);
                goto error;
            }

            parser->http_major *= 10;
            parser->http_major += ch - '0';

            if (parser->http_major > 999) {
                SET_ERRNO(HPE_INVALID_VERSION);
                goto error;
            }

            break;
        }

        /* first digit of minor HTTP version */
        case s_req_first_http_minor:
            if (!IS_NUM(ch)) {
                SET_ERRNO(HPE_INVALID_VERSION);
                goto error;
            }

            parser->http_minor = ch - '0';
            parser->state = s_req_http_minor;
            break;

            /* minor HTTP version or end of request line */
        case s_req_http_minor:
        {
            if (ch == CR) {
                parser->state = s_req_line_almost_done;
                break;
            }

            if (ch == LF) {
                parser->state = s_header_field_start;
                break;
            }

            /* XXX allow spaces after digit? */

            if (!IS_NUM(ch)) {
                SET_ERRNO(HPE_INVALID_VERSION);
                goto error;
            }

            parser->http_minor *= 10;
            parser->http_minor += ch - '0';

            if (parser->http_minor > 999) {
                SET_ERRNO(HPE_INVALID_VERSION);
                goto error;
            }

            break;
        }

        /* end of request line */
        case s_req_line_almost_done:
        {
            if (ch != LF) {
                SET_ERRNO(HPE_LF_EXPECTED);
                goto error;
            }

            parser->state = s_header_field_start;
            break;
        }

        case s_header_field_start:
        {
            if (ch == CR) {
                parser->state = s_headers_almost_done;
                break;
            }

            if (ch == LF) {
                /* they might be just sending \n instead of \r\n so this would be
                 * the second \n to denote the end of headers*/
                parser->state = s_headers_almost_done;
                goto reexecute_byte;
            }

            c = TOKEN(ch);

            if (!c) {
                SET_ERRNO(HPE_INVALID_HEADER_TOKEN);
                goto error;
            }

            MARK(header_field);

            parser->index = 0;
            parser->state = s_header_field;

            switch (c) {
            case 'c':
                parser->header_state = h_C;
                break;

            case 'p':
                parser->header_state = h_matching_proxy_connection;
                break;

            case 't':
                parser->header_state = h_matching_transfer_encoding;
                break;

            case 'u':
                parser->header_state = h_matching_upgrade;
                break;

            default:
                parser->header_state = h_general;
                break;
            }
            break;
        }

        case s_header_field:
        {
            c = TOKEN(ch);

            if (c) {
                switch (parser->header_state) {
                case h_general:
                    break;

                case h_C:
                    parser->index++;
                    parser->header_state = (c == 'o' ? h_CO : h_general);
                    break;

                case h_CO:
                    parser->index++;
                    parser->header_state = (c == 'n' ? h_CON : h_general);
                    break;

                case h_CON:
                    parser->index++;
                    switch (c) {
                    case 'n':
                        parser->header_state = h_matching_connection;
                        break;
                    case 't':
                        parser->header_state = h_matching_content_length;
                        break;
                    default:
                        parser->header_state = h_general;
                        break;
                    }
                    break;

                    /* connection */

                case h_matching_connection:
                    parser->index++;
                    if (parser->index > sizeof(CONNECTION) - 1
                        || c != CONNECTION[parser->index]) {
                        parser->header_state = h_general;
                    }
                    else if (parser->index == sizeof(CONNECTION) - 2) {
                        parser->header_state = h_connection;
                    }
                    break;

                    /* proxy-connection */

                case h_matching_proxy_connection:
                    parser->index++;
                    if (parser->index > sizeof(PROXY_CONNECTION) - 1
                        || c != PROXY_CONNECTION[parser->index]) {
                        parser->header_state = h_general;
                    }
                    else if (parser->index == sizeof(PROXY_CONNECTION) - 2) {
                        parser->header_state = h_connection;
                    }
                    break;

                    /* content-length */

                case h_matching_content_length:
                    parser->index++;
                    if (parser->index > sizeof(CONTENT_LENGTH) - 1
                        || c != CONTENT_LENGTH[parser->index]) {
                        parser->header_state = h_general;
                    }
                    else if (parser->index == sizeof(CONTENT_LENGTH) - 2) {
                        parser->header_state = h_content_length;
                    }
                    break;

                    /* transfer-encoding */

                case h_matching_transfer_encoding:
                    parser->index++;
                    if (parser->index > sizeof(TRANSFER_ENCODING) - 1
                        || c != TRANSFER_ENCODING[parser->index]) {
                        parser->header_state = h_general;
                    }
                    else if (parser->index == sizeof(TRANSFER_ENCODING) - 2) {
                        parser->header_state = h_transfer_encoding;
                    }
                    break;

                    /* upgrade */

                case h_matching_upgrade:
                    parser->index++;
                    if (parser->index > sizeof(UPGRADE) - 1
                        || c != UPGRADE[parser->index]) {
                        parser->header_state = h_general;
                    }
                    else if (parser->index == sizeof(UPGRADE) - 2) {
                        parser->header_state = h_upgrade;
                    }
                    break;

                case h_connection:
                case h_content_length:
                case h_transfer_encoding:
                case h_upgrade:
                    if (ch != ' ') parser->header_state = h_general;
                    break;

                default:
                    assert(0 && "Unknown header_state");
                    break;
                }
                break;
            }

            if (ch == ':') {
                parser->state = s_header_value_start;
                CALLBACK_DATA(header_field);
                break;
            }

            if (ch == CR) {
                parser->state = s_header_almost_done;
                CALLBACK_DATA(header_field);
                break;
            }

            if (ch == LF) {
                parser->state = s_header_field_start;
                CALLBACK_DATA(header_field);
                break;
            }

            SET_ERRNO(HPE_INVALID_HEADER_TOKEN);
            goto error;
        }

        case s_header_value_start:
        {
            if (ch == ' ' || ch == '\t') break;

            MARK(header_value);

            parser->state = s_header_value;
            parser->index = 0;

            if (ch == CR) {
                parser->header_state = h_general;
                parser->state = s_header_almost_done;
                CALLBACK_DATA(header_value);
                break;
            }

            if (ch == LF) {
                parser->state = s_header_field_start;
                CALLBACK_DATA(header_value);
                break;
            }

            c = LOWER(ch);

            switch (parser->header_state) {
            case h_upgrade:
                parser->flags |= F_UPGRADE;
                parser->header_state = h_general;
                break;

            case h_transfer_encoding:
                /* looking for 'Transfer-Encoding: chunked' */
                if ('c' == c) {
                    parser->header_state = h_matching_transfer_encoding_chunked;
                }
                else {
                    parser->header_state = h_general;
                }
                break;

            case h_content_length:
                if (!IS_NUM(ch)) {
                    SET_ERRNO(HPE_INVALID_CONTENT_LENGTH);
                    goto error;
                }

                parser->content_length = ch - '0';
                break;

            case h_connection:
                /* looking for 'Connection: keep-alive' */
                if (c == 'k') {
                    parser->header_state = h_matching_connection_keep_alive;
                    /* looking for 'Connection: close' */
                }
                else if (c == 'c') {
                    parser->header_state = h_matching_connection_close;
                }
                else {
                    parser->header_state = h_general;
                }
                break;

            default:
                parser->header_state = h_general;
                break;
            }
            break;
        }

        case s_header_value:
        {

            if (ch == CR) {
                parser->state = s_header_almost_done;
                CALLBACK_DATA(header_value);
                break;
            }

            if (ch == LF) {
                parser->state = s_header_almost_done;
                CALLBACK_DATA_NOADVANCE(header_value);
                goto reexecute_byte;
            }

            c = LOWER(ch);

            switch (parser->header_state) {
            case h_general:
                break;

            case h_connection:
            case h_transfer_encoding:
                assert(0 && "Shouldn't get here.");
                break;

            case h_content_length:
            {
                uint64_t t;

                if (ch == ' ') break;

                if (!IS_NUM(ch)) {
                    SET_ERRNO(HPE_INVALID_CONTENT_LENGTH);
                    goto error;
                }

                t = parser->content_length;
                t *= 10;
                t += ch - '0';

                /* Overflow? */
                if (t < parser->content_length || t == ULLONG_MAX) {
                    SET_ERRNO(HPE_INVALID_CONTENT_LENGTH);
                    goto error;
                }

                parser->content_length = t;
                break;
            }

            /* Transfer-Encoding: chunked */
            case h_matching_transfer_encoding_chunked:
                parser->index++;
                if (parser->index > sizeof(CHUNKED) - 1
                    || c != CHUNKED[parser->index]) {
                    parser->header_state = h_general;
                }
                else if (parser->index == sizeof(CHUNKED) - 2) {
                    parser->header_state = h_transfer_encoding_chunked;
                }
                break;

                /* looking for 'Connection: keep-alive' */
            case h_matching_connection_keep_alive:
                parser->index++;
                if (parser->index > sizeof(KEEP_ALIVE) - 1
                    || c != KEEP_ALIVE[parser->index]) {
                    parser->header_state = h_general;
                }
                else if (parser->index == sizeof(KEEP_ALIVE) - 2) {
                    parser->header_state = h_connection_keep_alive;
                }
                break;

                /* looking for 'Connection: close' */
            case h_matching_connection_close:
                parser->index++;
                if (parser->index > sizeof(CLOSE) - 1 || c != CLOSE[parser->index]) {
                    parser->header_state = h_general;
                }
                else if (parser->index == sizeof(CLOSE) - 2) {
                    parser->header_state = h_connection_close;
                }
                break;

            case h_transfer_encoding_chunked:
            case h_connection_keep_alive:
            case h_connection_close:
                if (ch != ' ') parser->header_state = h_general;
                break;

            default:
                parser->state = s_header_value;
                parser->header_state = h_general;
                break;
            }
            break;
        }

        case s_header_almost_done:
        {
            STRICT_CHECK(ch != LF);

            parser->state = s_header_value_lws;

            switch (parser->header_state) {
            case h_connection_keep_alive:
                parser->flags |= F_CONNECTION_KEEP_ALIVE;
                break;
            case h_connection_close:
                parser->flags |= F_CONNECTION_CLOSE;
                break;
            case h_transfer_encoding_chunked:
                parser->flags |= F_CHUNKED;
                break;
            default:
                break;
            }

            break;
        }

        case s_header_value_lws:
        {
            if (ch == ' ' || ch == '\t')
                parser->state = s_header_value_start;
            else
            {
                parser->state = s_header_field_start;
                goto reexecute_byte;
            }
            break;
        }

        case s_headers_almost_done:
        {
            STRICT_CHECK(ch != LF);

            if (parser->flags & F_TRAILING) {
                /* End of a chunked request */
                parser->state = NEW_MESSAGE();
                CALLBACK_NOTIFY(message_complete);
                break;
            }

            parser->state = s_headers_done;

            /* Set this here so that on_headers_complete() callbacks can see it */
            parser->upgrade =
                (parser->flags & F_UPGRADE || parser->method == HTTP_CONNECT);

            /* Here we call the headers_complete callback. This is somewhat
             * different than other callbacks because if the user returns 1, we
             * will interpret that as saying that this message has no body. This
             * is needed for the annoying case of recieving a response to a HEAD
             * request.
             *
             * We'd like to use CALLBACK_NOTIFY_NOADVANCE() here but we cannot, so
             * we have to simulate it by handling a change in errno below.
             */
            if (settings->on_headers_complete) {
                switch (settings->on_headers_complete(parser)) {
                case 0:
                    break;

                case 1:
                    parser->flags |= F_SKIPBODY;
                    break;

                default:
                    SET_ERRNO(HPE_CB_headers_complete);
                    return p - data; /* Error */
                }
            }

            if (HTTP_PARSER_ERRNO(parser) != HPE_OK) {
                return p - data;
            }

            goto reexecute_byte;
        }

        case s_headers_done:
        {
            STRICT_CHECK(ch != LF);

            parser->nread = 0;

            /* Exit, the rest of the connect is in a different protocol. */
            if (parser->upgrade) {
                parser->state = NEW_MESSAGE();
                CALLBACK_NOTIFY(message_complete);
                return (p - data) + 1;
            }

            if (parser->flags & F_SKIPBODY) {
                parser->state = NEW_MESSAGE();
                CALLBACK_NOTIFY(message_complete);
            }
            else if (parser->flags & F_CHUNKED) {
                /* chunked encoding - ignore Content-Length header */
                parser->state = s_chunk_size_start;
            }
            else {
                if (parser->content_length == 0) {
                    /* Content-Length header given but zero: Content-Length: 0\r\n */
                    parser->state = NEW_MESSAGE();
                    CALLBACK_NOTIFY(message_complete);
                }
                else if (parser->content_length != ULLONG_MAX) {
                    /* Content-Length header given and non-zero */
                    parser->state = s_body_identity;
                }
                else {
                    if (parser->type == HTTP_REQUEST ||
                        !http_message_needs_eof(parser)) {
                        /* Assume content-length 0 - read the next */
                        parser->state = NEW_MESSAGE();
                        CALLBACK_NOTIFY(message_complete);
                    }
                    else {
                        /* Read body until EOF */
                        parser->state = s_body_identity_eof;
                    }
                }
            }

            break;
        }

        case s_body_identity:
        {
            uint64_t to_read = MIN(parser->content_length,
                (uint64_t)((data + len) - p));

            assert(parser->content_length != 0
                && parser->content_length != ULLONG_MAX);

            /* The difference between advancing content_length and p is because
             * the latter will automaticaly advance on the next loop iteration.
             * Further, if content_length ends up at 0, we want to see the last
             * byte again for our message complete callback.
             */
            MARK(body);
            parser->content_length -= to_read;
            p += to_read - 1;

            if (parser->content_length == 0) {
                parser->state = s_message_done;

                /* Mimic CALLBACK_DATA_NOADVANCE() but with one extra byte.
                 *
                 * The alternative to doing this is to wait for the next byte to
                 * trigger the data callback, just as in every other case. The
                 * problem with this is that this makes it difficult for the test
                 * harness to distinguish between complete-on-EOF and
                 * complete-on-length. It's not clear that this distinction is
                 * important for applications, but let's keep it for now.
                 */
                CALLBACK_DATA_(body, p - body_mark + 1, p - data);
                goto reexecute_byte;
            }

            break;
        }

        /* read until EOF */
        case s_body_identity_eof:
            MARK(body);
            p = data + len - 1;

            break;

        case s_message_done:
            parser->state = NEW_MESSAGE();
            CALLBACK_NOTIFY(message_complete);
            break;

        case s_chunk_size_start:
        {
            assert(parser->nread == 1);
            assert(parser->flags & F_CHUNKED);

            unhex_val = unhex[(unsigned char)ch];
            if (unhex_val == -1) {
                SET_ERRNO(HPE_INVALID_CHUNK_SIZE);
                goto error;
            }

            parser->content_length = unhex_val;
            parser->state = s_chunk_size;
            break;
        }

        case s_chunk_size:
        {
            uint64_t t;

            assert(parser->flags & F_CHUNKED);

            if (ch == CR) {
                parser->state = s_chunk_size_almost_done;
                break;
            }

            unhex_val = unhex[(unsigned char)ch];

            if (unhex_val == -1) {
                if (ch == ';' || ch == ' ') {
                    parser->state = s_chunk_parameters;
                    break;
                }

                SET_ERRNO(HPE_INVALID_CHUNK_SIZE);
                goto error;
            }

            t = parser->content_length;
            t *= 16;
            t += unhex_val;

            /* Overflow? */
            if (t < parser->content_length || t == ULLONG_MAX) {
                SET_ERRNO(HPE_INVALID_CONTENT_LENGTH);
                goto error;
            }

            parser->content_length = t;
            break;
        }

        case s_chunk_parameters:
        {
            assert(parser->flags & F_CHUNKED);
            /* just ignore this shit. TODO check for overflow */
            if (ch == CR) {
                parser->state = s_chunk_size_almost_done;
                break;
            }
            break;
        }

        case s_chunk_size_almost_done:
        {
            assert(parser->flags & F_CHUNKED);
            STRICT_CHECK(ch != LF);

            parser->nread = 0;

            if (parser->content_length == 0) {
                parser->flags |= F_TRAILING;
                parser->state = s_header_field_start;
            }
            else {
                parser->state = s_chunk_data;
            }
            break;
        }

        case s_chunk_data:
        {
            uint64_t to_read = MIN(parser->content_length,
                (uint64_t)((data + len) - p));

            assert(parser->flags & F_CHUNKED);
            assert(parser->content_length != 0
                && parser->content_length != ULLONG_MAX);

            /* See the explanation in s_body_identity for why the content
             * length and data pointers are managed this way.
             */
            MARK(body);
            parser->content_length -= to_read;
            p += to_read - 1;

            if (parser->content_length == 0) {
                parser->state = s_chunk_data_almost_done;
            }

            break;
        }

        case s_chunk_data_almost_done:
            assert(parser->flags & F_CHUNKED);
            assert(parser->content_length == 0);
            STRICT_CHECK(ch != CR);
            parser->state = s_chunk_data_done;
            CALLBACK_DATA(body);
            break;

        case s_chunk_data_done:
            assert(parser->flags & F_CHUNKED);
            STRICT_CHECK(ch != LF);
            parser->nread = 0;
            parser->state = s_chunk_size_start;
            break;

        default:
            assert(0 && "unhandled state");
            SET_ERRNO(HPE_INVALID_INTERNAL_STATE);
            goto error;
        }
    }

    /* Run callbacks for any marks that we have leftover after we ran our of
     * bytes. There should be at most one of these set, so it's OK to invoke
     * them in series (unset marks will not result in callbacks).
     *
     * We use the NOADVANCE() variety of callbacks here because 'p' has already
     * overflowed 'data' and this allows us to correct for the off-by-one that
     * we'd otherwise have (since CALLBACK_DATA() is meant to be run with a 'p'
     * value that's in-bounds).
     */

    assert(((header_field_mark ? 1 : 0) +
        (header_value_mark ? 1 : 0) +
        (url_mark ? 1 : 0) +
        (body_mark ? 1 : 0)) <= 1);

    CALLBACK_DATA_NOADVANCE(header_field);
    CALLBACK_DATA_NOADVANCE(header_value);
    CALLBACK_DATA_NOADVANCE(url);
    CALLBACK_DATA_NOADVANCE(body);

    return len;

error:
    if (HTTP_PARSER_ERRNO(parser) == HPE_OK) {
        SET_ERRNO(HPE_UNKNOWN);
    }

    return (p - data);
}


/* Does the parser need to see an EOF to find the end of the message? */
int
http_message_needs_eof(const http_parser* parser)
{
    if (parser->type == HTTP_REQUEST) {
        return 0;
    }

    /* See RFC 2616 section 4.4 */
    if (parser->status_code / 100 == 1 || /* 1xx e.g. Continue */
        parser->status_code == 204 ||     /* No Content */
        parser->status_code == 304 ||     /* Not Modified */
        parser->flags & F_SKIPBODY) {     /* response to a HEAD request */
        return 0;
    }

    if ((parser->flags & F_CHUNKED) || parser->content_length != ULLONG_MAX) {
        return 0;
    }

    return 1;
}


int
http_should_keep_alive(const http_parser* parser)
{
    if (parser->http_major > 0 && parser->http_minor > 0) {
        /* HTTP/1.1 */
        if (parser->flags & F_CONNECTION_CLOSE) {
            return 0;
        }
    }
    else {
        /* HTTP/1.0 or earlier */
        if (!(parser->flags & F_CONNECTION_KEEP_ALIVE)) {
            return 0;
        }
    }

    return !http_message_needs_eof(parser);
}


const char*
http_method_str(enum http_method m)
{
    return ELEM_AT(method_strings, m, "<unknown>");
}


void
http_parser_init(http_parser* parser, enum http_parser_type t)
{
    void* data = parser->data; /* preserve application data */
    memset(parser, 0, sizeof(*parser));
    parser->data = data;
    parser->type = t;
    parser->state = (t == HTTP_REQUEST ? s_start_req : (t == HTTP_RESPONSE ? s_start_res : s_start_req_or_res));
    parser->http_errno = HPE_OK;
}

const char*
http_errno_name(enum http_errno err) {
    assert(err < (sizeof(http_strerror_tab) / sizeof(http_strerror_tab[0])));
    return http_strerror_tab[err].name;
}

const char*
http_errno_description(enum http_errno err) {
    assert(err < (sizeof(http_strerror_tab) / sizeof(http_strerror_tab[0])));
    return http_strerror_tab[err].description;
}

static enum http_host_state
http_parse_host_char(enum http_host_state s, const char ch) {
    switch (s) {
    case s_http_userinfo:
    case s_http_userinfo_start:
        if (ch == '@') {
            return s_http_host_start;
        }

        if (IS_USERINFO_CHAR(ch)) {
            return s_http_userinfo;
        }
        break;

    case s_http_host_start:
        if (ch == '[') {
            return s_http_host_v6_start;
        }

        if (IS_HOST_CHAR(ch)) {
            return s_http_host;
        }

        break;

    case s_http_host:
        if (IS_HOST_CHAR(ch)) {
            return s_http_host;
        }

        /* FALLTHROUGH */
    case s_http_host_v6_end:
        if (ch == ':') {
            return s_http_host_port_start;
        }

        break;

    case s_http_host_v6:
        if (ch == ']') {
            return s_http_host_v6_end;
        }

        /* FALLTHROUGH */
    case s_http_host_v6_start:
        if (IS_HEX(ch) || ch == ':') {
            return s_http_host_v6;
        }

        break;

    case s_http_host_port:
    case s_http_host_port_start:
        if (IS_NUM(ch)) {
            return s_http_host_port;
        }

        break;

    default:
        break;
    }
    return s_http_host_dead;
}

static int
http_parse_host(const char* buf, struct http_parser_url* u, int found_at) {
    enum http_host_state s;

    const char* p;
    size_t buflen = u->field_data[UF_HOST].off + u->field_data[UF_HOST].len;

    u->field_data[UF_HOST].len = 0;

    s = found_at ? s_http_userinfo_start : s_http_host_start;

    for (p = buf + u->field_data[UF_HOST].off; p < buf + buflen; p++) {
        enum http_host_state new_s = http_parse_host_char(s, *p);

        if (new_s == s_http_host_dead) {
            return 1;
        }

        switch (new_s) {
        case s_http_host:
            if (s != s_http_host) {
                u->field_data[UF_HOST].off = p - buf;
            }
            u->field_data[UF_HOST].len++;
            break;

        case s_http_host_v6:
            if (s != s_http_host_v6) {
                u->field_data[UF_HOST].off = p - buf;
            }
            u->field_data[UF_HOST].len++;
            break;

        case s_http_host_port:
            if (s != s_http_host_port) {
                u->field_data[UF_PORT].off = p - buf;
                u->field_data[UF_PORT].len = 0;
                u->field_set |= (1 << UF_PORT);
            }
            u->field_data[UF_PORT].len++;
            break;

        case s_http_userinfo:
            if (s != s_http_userinfo) {
                u->field_data[UF_USERINFO].off = p - buf;
                u->field_data[UF_USERINFO].len = 0;
                u->field_set |= (1 << UF_USERINFO);
            }
            u->field_data[UF_USERINFO].len++;
            break;

        default:
            break;
        }
        s = new_s;
    }

    /* Make sure we don't end somewhere unexpected */
    switch (s) {
    case s_http_host_start:
    case s_http_host_v6_start:
    case s_http_host_v6:
    case s_http_host_port_start:
    case s_http_userinfo:
    case s_http_userinfo_start:
        return 1;
    default:
        break;
    }

    return 0;
}

int
http_parser_parse_url(const char* buf, size_t buflen, int is_connect,
    struct http_parser_url* u)
{
    enum state s;
    const char* p;
    enum http_parser_url_fields uf, old_uf;
    int found_at = 0;

    u->port = u->field_set = 0;
    s = is_connect ? s_req_server_start : s_req_spaces_before_url;
    uf = old_uf = UF_MAX;

    for (p = buf; p < buf + buflen; p++) {
        s = parse_url_char(s, *p);

        /* Figure out the next field that we're operating on */
        switch (s) {
        case s_dead:
            return 1;

            /* Skip delimeters */
        case s_req_schema_slash:
        case s_req_schema_slash_slash:
        case s_req_server_start:
        case s_req_query_string_start:
        case s_req_fragment_start:
            continue;

        case s_req_schema:
            uf = UF_SCHEMA;
            break;

        case s_req_server_with_at:
            found_at = 1;

            /* FALLTROUGH */
        case s_req_server:
            uf = UF_HOST;
            break;

        case s_req_path:
            uf = UF_PATH;
            break;

        case s_req_query_string:
            uf = UF_QUERY;
            break;

        case s_req_fragment:
            uf = UF_FRAGMENT;
            break;

        default:
            assert(!"Unexpected state");
            return 1;
        }

        /* Nothing's changed; soldier on */
        if (uf == old_uf) {
            u->field_data[uf].len++;
            continue;
        }

        u->field_data[uf].off = p - buf;
        u->field_data[uf].len = 1;

        u->field_set |= (1 << uf);
        old_uf = uf;
    }

    /* host must be present if there is a schema */
    /* parsing http:///toto will fail */
    if ((u->field_set & ((1 << UF_SCHEMA) | (1 << UF_HOST))) != 0) {
        if (http_parse_host(buf, u, found_at) != 0) {
            return 1;
        }
    }

    /* CONNECT requests can only contain "hostname:port" */
    if (is_connect && u->field_set != ((1 << UF_HOST) | (1 << UF_PORT))) {
        return 1;
    }

    if (u->field_set & (1 << UF_PORT)) {
        /* Don't bother with endp; we've already validated the string */
        unsigned long v = strtoul(buf + u->field_data[UF_PORT].off, NULL, 10);

        /* Ports have a max value of 2^16 */
        if (v > 0xffff) {
            return 1;
        }

        u->port = (uint16_t)v;
    }

    return 0;
}

void
http_parser_pause(http_parser* parser, int paused) {
    /* Users should only be pausing/unpausing a parser that is not in an error
     * state. In non-debug builds, there's not much that we can do about this
     * other than ignore it.
     */
    if (HTTP_PARSER_ERRNO(parser) == HPE_OK ||
        HTTP_PARSER_ERRNO(parser) == HPE_PAUSED) {
        SET_ERRNO((paused) ? HPE_PAUSED : HPE_OK);
    }
    else {
        assert(0 && "Attempting to pause parser in error state");
    }
}

int
http_body_is_final(const struct http_parser* parser) {
    return parser->state == s_message_done;
}


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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>




struct http_request_s {
    http_parser parser;
    http_parser_settings parser_settings;

    const char* method;
    char* url;

    char** headers;
    int headers_size;
    int headers_index;

    char* data;
    int datalen;

    int complete;
};

static int
on_url(http_parser* parser, const char* at, size_t length)
{
    http_request_t* request = parser->data;
    int urllen = request->url ? strlen(request->url) : 0;

    request->url = realloc(request->url, urllen + length + 1);
    assert(request->url);

    request->url[urllen] = '\0';
    strncat(request->url, at, length);
    return 0;
}

static int
on_header_field(http_parser* parser, const char* at, size_t length)
{
    http_request_t* request = parser->data;

    /* Check if our index is a value */
    if (request->headers_index % 2 == 1) {
        request->headers_index++;
    }

    /* Allocate space for new field-value pair */
    if (request->headers_index == request->headers_size) {
        request->headers_size += 2;
        request->headers = realloc(request->headers,
            request->headers_size * sizeof(char*));
        assert(request->headers);
        request->headers[request->headers_index] = NULL;
        request->headers[request->headers_index + 1] = NULL;
    }

    /* Allocate space in the current header string */
    if (request->headers[request->headers_index] == NULL) {
        request->headers[request->headers_index] = calloc(1, length + 1);
    }
    else {
        request->headers[request->headers_index] = realloc(
            request->headers[request->headers_index],
            strlen(request->headers[request->headers_index]) + length + 1
        );
    }
    assert(request->headers[request->headers_index]);

    strncat(request->headers[request->headers_index], at, length);
    return 0;
}

static int
on_header_value(http_parser* parser, const char* at, size_t length)
{
    http_request_t* request = parser->data;

    /* Check if our index is a field */
    if (request->headers_index % 2 == 0) {
        request->headers_index++;
    }

    /* Allocate space in the current header string */
    if (request->headers[request->headers_index] == NULL) {
        request->headers[request->headers_index] = calloc(1, length + 1);
    }
    else {
        request->headers[request->headers_index] = realloc(
            request->headers[request->headers_index],
            strlen(request->headers[request->headers_index]) + length + 1
        );
    }
    assert(request->headers[request->headers_index]);

    strncat(request->headers[request->headers_index], at, length);
    return 0;
}

static int
on_body(http_parser* parser, const char* at, size_t length)
{
    http_request_t* request = parser->data;

    request->data = realloc(request->data, request->datalen + length);
    assert(request->data);

    memcpy(request->data + request->datalen, at, length);
    request->datalen += length;
    return 0;
}

static int
on_message_complete(http_parser* parser)
{
    http_request_t* request = parser->data;

    request->method = http_method_str(request->parser.method);
    request->complete = 1;
    return 0;
}

http_request_t*
http_request_init(void)
{
    http_request_t* request;

    request = calloc(1, sizeof(http_request_t));
    if (!request) {
        return NULL;
    }
    http_parser_init(&request->parser, HTTP_REQUEST);
    request->parser.data = request;

    request->parser_settings.on_url = &on_url;
    request->parser_settings.on_header_field = &on_header_field;
    request->parser_settings.on_header_value = &on_header_value;
    request->parser_settings.on_body = &on_body;
    request->parser_settings.on_message_complete = &on_message_complete;

    return request;
}

void
http_request_destroy(http_request_t* request)
{
    int i;

    if (request) {
        free(request->url);
        for (i = 0; i < request->headers_size; i++) {
            free(request->headers[i]);
        }
        free(request->headers);
        free(request->data);
        free(request);
    }
}

int
http_request_add_data(http_request_t* request, const char* data, int datalen)
{
    int ret;

    assert(request);

    ret = http_parser_execute(&request->parser,
        &request->parser_settings,
        data, datalen);
    return ret;
}

int
http_request_is_complete(http_request_t* request)
{
    assert(request);
    return request->complete;
}

int
http_request_has_error(http_request_t* request)
{
    assert(request);
    return (HTTP_PARSER_ERRNO(&request->parser) != HPE_OK);
}

const char*
http_request_get_error_name(http_request_t* request)
{
    assert(request);
    return http_errno_name(HTTP_PARSER_ERRNO(&request->parser));
}

const char*
http_request_get_error_description(http_request_t* request)
{
    assert(request);
    return http_errno_description(HTTP_PARSER_ERRNO(&request->parser));
}

const char*
http_request_get_method(http_request_t* request)
{
    assert(request);
    return request->method;
}

const char*
http_request_get_url(http_request_t* request)
{
    assert(request);
    return request->url;
}

const char*
http_request_get_header(http_request_t* request, const char* name)
{
    int i;

    assert(request);

    for (i = 0; i < request->headers_size; i += 2) {
        if (!stricmp(request->headers[i], name)) {
            return request->headers[i + 1];
        }
    }
    return NULL;
}

const char*
http_request_get_data(http_request_t* request, int* datalen)
{
    assert(request);

    if (datalen) {
        *datalen = request->datalen;
    }
    return request->data;
}

void
http_request_dump_headers(http_request_t* request)
{
    int i;

    assert(request);

    for (i = 0; i < request->headers_size; i += 2) {
        fprintf(stderr, "%s:%s\n", request->headers[i], request->headers[i + 1]);
    }
}


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




struct http_response_s {
    int complete;
    int disconnect;

    char* data;
    int data_size;
    int data_length;
};


static void
http_response_add_data(http_response_t* response, const char* data, int datalen)
{
    int newdatasize;

    assert(response);
    assert(data);
    assert(datalen > 0);

    newdatasize = response->data_size;
    while (response->data_size + datalen > newdatasize) {
        newdatasize *= 2;
    }
    if (newdatasize != response->data_size) {
        response->data = realloc(response->data, newdatasize);
        assert(response->data);
    }
    memcpy(response->data + response->data_length, data, datalen);
    response->data_length += datalen;
    response->data[response->data_length] = 0;
}


http_response_t*
http_response_init(const char* protocol, int code, const char* message)
{
    http_response_t* response;
    char codestr[4];

    assert(code >= 100 && code < 1000);

    /* Convert code into string */
    memset(codestr, 0, sizeof(codestr));
    snprintf(codestr, sizeof(codestr), "%u", code);

    response = calloc(1, sizeof(http_response_t));
    if (!response) {
        return NULL;
    }

    /* Allocate response data */
    response->data_size = 1024;
    response->data = malloc(response->data_size);
    if (!response->data) {
        free(response);
        return NULL;
    }

    /* Add first line of response to the data array */
    http_response_add_data(response, protocol, strlen(protocol));
    http_response_add_data(response, " ", 1);
    http_response_add_data(response, codestr, strlen(codestr));
    http_response_add_data(response, " ", 1);
    http_response_add_data(response, message, strlen(message));
    http_response_add_data(response, "\r\n", 2);

    return response;
}

http_response_t*
http_response_init1(char* data, int size)
{
    http_response_t* response;

    response = calloc(1, sizeof(http_response_t));
    if (!response) {
        return NULL;
    }

    /* Allocate response data */
    response->data_length = size;
    response->data_size = size;
    response->data = malloc(response->data_size);
    if (!response->data) {
        free(response);
        return NULL;
    }
    memcpy(response->data, data, size);
    response->complete = 1;

    return response;
}

void
http_response_destroy(http_response_t* response)
{
    if (response) {
        free(response->data);
        free(response);
    }
}

void
http_response_add_header(http_response_t* response, const char* name, const char* value)
{
    assert(response);
    assert(name);
    assert(value);

    http_response_add_data(response, name, strlen(name));
    http_response_add_data(response, ": ", 2);
    http_response_add_data(response, value, strlen(value));
    http_response_add_data(response, "\r\n", 2);
}

void
http_response_finish(http_response_t* response, const char* data, int datalen)
{
    assert(response);
    assert(datalen == 0 || (data && datalen > 0));

    if (data && datalen > 0) {
        const char* hdrname = "Content-Length";
        char hdrvalue[16];

        memset(hdrvalue, 0, sizeof(hdrvalue));
        snprintf(hdrvalue, sizeof(hdrvalue) - 1, "%d", datalen);

        /* Add Content-Length header first */
        http_response_add_data(response, hdrname, strlen(hdrname));
        http_response_add_data(response, ": ", 2);
        http_response_add_data(response, hdrvalue, strlen(hdrvalue));
        http_response_add_data(response, "\r\n\r\n", 4);

        /* Add data to the end of response */
        http_response_add_data(response, data, datalen);
    }
    else {
        /* Add extra end of line after headers */
        http_response_add_data(response, "\r\n", 2);
    }
    response->complete = 1;
}

void
http_response_set_disconnect(http_response_t* response, int disconnect)
{
    assert(response);

    response->disconnect = !!disconnect;
}

int
http_response_get_disconnect(http_response_t* response)
{
    assert(response);

    return response->disconnect;
}

const char*
http_response_get_data(http_response_t* response, int* datalen)
{
    assert(response);
    assert(datalen);
    assert(response->complete);

    *datalen = response->data_length;
    return response->data;
}
