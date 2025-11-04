/*
 * ipinput.h
 *
 *  Created on: 2011-9-18
 *      Author: wudegang
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <windows.h>


#ifndef IPINPUT_H_
#define IPINPUT_H_


typedef struct _new_queue {
	CRITICAL_SECTION locker;
	unsigned char* buf;
	int bufsize;
	int write_ptr;
	int read_ptr;

} NewQueue;



typedef struct _udp_param {
	char** argv;
	NewQueue *queue;
	int size;
	int sock;
} UdpParam;


void* udp_ts_recv(void* param);
void init_queue(NewQueue *que, int size);
void free_queue(NewQueue* que);
void put_queue(NewQueue* que, unsigned char* buf, int size);
int get_queue(NewQueue* que, unsigned char* buf, int size);

#endif /* IPINPUT_H_ */
