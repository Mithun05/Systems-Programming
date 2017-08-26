#ifndef _COMMON_H
#define _COMMON_H

/**
 * Standard header files
 */ 
#include "mat_base.h"
#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>



/**
 * Name of the POSIX IPC Object.
 * It contains the prefix which identifies the typical
 * user's shared memory segment.
 */

 #define POSIX_IPC_NAME_PREFIX "/mdeshpa1-"

/**
 * Name of the POSIX IPC actual Object.
 * It contains the name of the user's shared memory segment.
 */

 #define SHM_NAME POSIX_IPC_NAME_PREFIX "shm"

/**
 * Client-Server Semaphore Object 
 */ 

 #define SERVER_SEM_NAME POSIX_IPC_NAME_PREFIX "server"

/** Name of well-known requests FIFO used by both clients
 *  and server. It is used to send memory size from the server
 *  towards client.
 */
 #define MEM_SIZE_SHARED_FILE "MemorySize.txt"

/**
 * Client-Server Request Semaphore Object
 */
 #define REQUEST_SEM_NAME POSIX_IPC_NAME_PREFIX "request"

/**
 * Client-Server Response Semaphore Object 
 */
 #define RESPONSE_SEM_NAME POSIX_IPC_NAME_PREFIX "response"

/**
 * Enum constants
 */
 enum {
  	SERVER_SEM,
  	REQUEST_SEM,
  	RESPONSE_SEM,
  	N_SEMS
 };


/**
 * Permissions for the shared memory segment
 */ 
 #define ALL_RW_PERMS (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWGRP)


/**
 * Semaphore used for server side
 */
 typedef struct SemOpenArgsServer {

	const char *posixName;
  	int oflags;
  	mode_t mode;
  	unsigned initValue;

 } SemOpenArgsServer;

/**
 * Semaphore used for client side
 */
 typedef struct SemOpenArgsClient {

	  const char *posixName;
	  int oflags;

 } SemOpenArgsClient;


/**
 * Matrix Data. 
 * It has rows and column dimension of each matrix.
 * It contains the matrix used to store all the three matrices in shared memory segment.
 */ 
 typedef struct MatrixData {

	int err;
	int n1;
	int n2;
	int n3;
	MatrixBaseType ABCMatrices;

 } MatrixData;

#endif //ifndef _COMMON_H
