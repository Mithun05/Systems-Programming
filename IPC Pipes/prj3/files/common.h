#ifndef _COMMON_H
#define _COMMON_H

#include "mat_base.h"

#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

/** Name of well-known requests FIFO in server-dir used by both clients
 *  and server.
 */
#define REQUESTS_FIFO "REQUESTS"

/* add declarations common to both server and client */

#define CLIENT_FIFO_TEMPLATE "client.%ld"

#define CLIENT_FIFO_NAME_LEN (sizeof(CLIENT_FIFO_TEMPLATE) + 20)

typedef struct Request {
	pid_t pid;
	char module[100];
	char dir[100];
} Request; 		

typedef struct Response {
	float wall;
	float user;
	float sys;
} Response;

#endif //ifndef _COMMON_H
