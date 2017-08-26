
/**
 * The source code includes iterative server design
 * to handle client request. It does the work of multiplication of matrices.
 * It creates required semaphore for the synchronization and shared memory segment used to perform 
 * matrix multiplication, the data sent by the client to server. 	
 */

/**
 * Common header files
 */
#include "common.h"
#include "mat_base.h"
#include "errors.h"
/**
 * custom header files
 */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <assert.h>
#include <fcntl.h>
#include <signal.h>

/**
 * File pointer to log errors
 */
FILE *filp = NULL; 


/** 
 * Standard matrix multiplication using regular 2-dimensional matrices 
 */
	void
naiveMatmul(int n1, int n2, int n3,
		CONST MatrixBaseType a[n1][n2],
		CONST MatrixBaseType b[n2][n3], MatrixBaseType c[n1][n3], int *err)
{
	fprintf(filp, "Matrix Multiplication : Begin\n");
	fflush(filp);

	for (int i = 0; i < n1; i++) {
		for (int j = 0; j < n3; j++) {
			c[i][j] = 0;
			for (int k = 0; k < n2; k++) c[i][j] += a[i][k]*b[k][j];
		}
	}

	fprintf(filp, "Matrix Multiplication : Completed\n");
	fflush(filp);
} 


/**
 *
 * Do task
 */
void *doTask(void *connStream)
{

	fprintf(filp, "Matrix Multiplication : Thread Handler\n");
	fflush(filp);


	int err = 0;
	int connFd = *(int*) connStream;
	int readSize, readSize1, readSize2;
	RequestData requestData;
	ResponseData responseData;


	while(1)
	{
		if((readSize = recv(connFd, &requestData, sizeof(RequestData), 0)) < 0)
		{	
			err = EACCES;
			fprintf(filp, "doTask() : Cannot receive : %s\n", strerror(err));
			fflush(filp);
		}

		int n1 = 0, n2 = 0, n3 = 0;
		unsigned long asize = 0, bsize = 0;
		if(readSize > 0)
		{
			n1 = requestData.n1;
			n2 = requestData.n2;
			n3 = requestData.n3;
			asize = requestData.aSize;
			bsize = requestData.bSize;
			fprintf(filp, "n1 : %d, n2 : %d, n3 : %d asize : %ld bsize : %ld \n", n1, n2, n3, asize, bsize);
			fflush(filp);
		}

		requestData.A = malloc(asize);
		if(requestData.A == NULL)
		{
			err = ENOMEM; 
			fprintf(filp, "Malloc Error %s\n", strerror(err));
			fflush(filp);
		}				

		if((readSize1 = recv(connFd, requestData.A, asize, 0)) < 0)
		{           
			err = EACCES;
			fprintf(filp, "doTask() : Cannot receive : %s\n", strerror(err));
			fflush(filp);
		}

		if(readSize1 > 0)
		{

			fprintf(filp, "Matrix Data A : \n");
			fflush(filp);

			for (int i = 0; i <  n1; i++) {
				for (int j = 0; j < n2; j++) {
					fprintf(filp, "%d \t", *(requestData.A + i*n2 + j));
					fflush(filp);
				}
				fprintf(filp, "\n");
				fflush(filp);
			}
		}

		requestData.B = malloc(bsize);
		if(requestData.B == NULL)
		{
			err = ENOMEM;
			fprintf(filp, "Malloc Error %s\n", strerror(err));
			fflush(filp);
		}

		if((readSize2 = recv(connFd, requestData.B, bsize, 0)) < 0)
		{
			err = EACCES;
			fprintf(filp, "doTask() : Cannot receive : %s\n", strerror(err));
			fflush(filp);
		}

		if(readSize2 > 0)
		{

			fprintf(filp, "Matrix Data B : \n");
			fflush(filp);

			for (int i = 0; i <  n2; i++) {
				for (int j = 0; j < n3; j++) {
					fprintf(filp, "%d \t", *(requestData.A + i*n3 + j));
					fflush(filp);
				}
				fprintf(filp, "\n");
				fflush(filp);
			}
		}

		unsigned long csize =  n1 * n3 * sizeof(int);
		responseData.cSize = csize;
		responseData.C = malloc(csize);

		struct tms tmsBegin, tmsEnd;
		clock_t tBegin, tEnd;


		if((tBegin = times(&tmsBegin)) < 0)
		{
			fprintf(filp, "Time begin error\n");
			fflush(filp);
		}

		naiveMatmul(n1, n2, n3, (MatrixBaseType (*)[n2]) requestData.A, (MatrixBaseType (*)[n3]) requestData.B, (MatrixBaseType (*)[n3]) responseData.C, &err);

		if((tEnd = times(&tmsEnd)) < 0)
		{
			fprintf(filp, "Time end error\n");
			fflush(filp);
		}

		responseData.wall = (tEnd - tBegin);
		responseData.user = (tmsEnd.tms_utime - tmsBegin.tms_utime);
		responseData.sys =  (tmsEnd.tms_stime - tmsBegin.tms_stime);
		responseData.err =  err;

		fprintf(filp, "Wall : %ld User : %ld Sys : %ld Err : %d\n", responseData.wall, responseData.user, responseData.sys, responseData.err);
		fflush(filp);

		//Send some data
		if(send(connFd, &responseData, sizeof(ResponseData), MSG_NOSIGNAL) < 0)
		{
			err = EIO;
			fprintf(filp, "Send failed %s\n", strerror(err));
			fflush(filp);
		}		

		//Send some data
		if(send(connFd, responseData.C, csize, MSG_NOSIGNAL) < 0)
		{
			err = EIO;
			fprintf(filp, "Send failed %s\n", strerror(err));
			fflush(filp);
		}

		fprintf(filp, "Matrix Multiplication has been sent to the client\n");
		fflush(filp);

		// free allocated memory
		free(connStream); 
		free(requestData.A);
		free(requestData.B);
		free(responseData.C);	
	}

	return 0;
}

/**
 * Daemon Service.
 * Used to do matrix multiplication, the request which is sent by the client.
 */

	static void
doDaemonService(int sockFd)
{
	/**
	 * An error handling
	 */
	int err = 0;
	int connFd;
	struct sockaddr_in rSin;
	socklen_t rLen;
	int *newSock;
	int pRet;

	fprintf(filp, "doDaemonService() : Waiting For Client Request/Service\n");
	fflush(filp);

	/**
	 * Wait for client request
	 */
	rLen = sizeof(rSin);
	while((connFd = accept(sockFd, (struct sockaddr*) &rSin, &rLen)))
	{
		fprintf(filp, "doDaemonService() : Processing Client Request/Service\n");
		fflush(filp);

		sigset_t set;
		sigemptyset (&set);
		sigaddset (&set, SIGPIPE);
		pthread_sigmask(SIG_BLOCK, &set, NULL);

		pthread_t pThread;
		newSock = malloc(1);
		*newSock = connFd;

		fprintf(filp, "Matrix Multiplication : Create Thread\n");
		fflush(filp);

		pRet = pthread_create(&pThread, NULL, doTask, (void *) newSock);
		if(pRet)
		{
			err = EACCES;
			fprintf(filp, "makeWorker() : Cannot create thread : %s\n", strerror(err));
			fflush(filp);
		}

		fprintf(filp, "Matrix Multiplication : Waiting for a thread to complete\n");
		fflush(filp);

		pthread_join(pThread, NULL);
	}	
	if(connFd < 0)
	{
		err = EACCES;
		fprintf(filp, "doDaemonService() : Cannot accept : %s\n", strerror(err));
		fflush(filp);
	}
}

/**
 * Function is used to create a daemon service
 */
	static pid_t 
makeDaemon(int port)
{
	pid_t processId, sId;
	int err = 0;
	processId = fork();
	if(processId < 0)
	{
		err = EACCES;
		fatal("makeDaemon() : Fork Failed %s\n", strerror(err));
	}
	if(processId > 0)
	{
		return processId;
	}

	umask(0);
	sId = setsid();
	if(sId < 0)
	{
		err = EACCES;
		fatal("makeDaemon() : Session Failed %s\n", strerror(err));
	}

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	/**
	 * Log the error messages into the txt file
	 */

	filp = fopen("ServerLog.txt", "w+");
	fprintf(filp, "makeDaemon() : Daemon Started With PID %d\n", getpid());
	fflush(filp);

	int sockFd;
	struct sockaddr_in sOn;

	if((sockFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{
		err = EACCES;
		fprintf(filp, "makeDaemon() : Cannot create socket %s\n", strerror(err));
		fflush(filp);
	}

	memset(&sOn, '0', sizeof(sOn));

	sOn.sin_family = AF_INET;
	sOn.sin_addr.s_addr = inet_addr("127.0.0.1");
	sOn.sin_port = htons((unsigned short) port);

	if(bind(sockFd, (struct sockaddr *) &sOn, sizeof(sOn)) < 0) 
	{	
		err = EACCES;
		fprintf(filp, "makeDaemon() : Cannot bind %s\n", strerror(err));
		fflush(filp);	
	}

	if(listen(sockFd, 10) < 0) 
	{
		err = EACCES;
		fprintf(filp, "makeDaemon() : Cannot listen %s\n", strerror(err));
		fflush(filp);   	
		exit(EXIT_FAILURE);
	}

	doDaemonService(sockFd);	
	assert(0);

}

/**
 * Function is send to create background service to serve multiple client request
 */
	static pid_t
makeServer(int port)
{
	return makeDaemon(port);
}


int main(int argc, const char *argv[])
{
	if(argc != 2) fatal("usage: %s <port-no>", argv[0]);

	int port = atoi(argv[1]);
	if(port < 1024)
	{
		fatal("usage: %s <port-no>", argv[0]);
	}

	pid_t pid = makeServer(port);
	printf("%ld\n", (long)pid);
	return 0;
}



