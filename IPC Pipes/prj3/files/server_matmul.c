#include "common.h"
#include "mat_base.h"

//#define DO_TRACE 1
#include "trace.h"

#include "errors.h"

#include <sys/types.h>

/**
 * Standard header files
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <dlfcn.h>
#include <sys/times.h>

int serverFd, dummyFd, clientFd;
extern int errno;

Response resp;
Request req;
int err = 1;


static void * getMulFn(const char *modulePath, void **dlHandleP, const char *fnName, int *err)
{
  printf("in fucntion\n");	
  void *dlHandle = (*dlHandleP) ? *dlHandleP : dlopen(modulePath, RTLD_NOW);
  if (!dlHandle) {
    return NULL;
  }
  *dlHandleP = dlHandle;
  void *fn = dlsym(dlHandle, fnName);
  if (!fn)  
  {
	return NULL;
  }
  return fn;
}


void interact(Request m, MatrixMulFn *MatrixMulFn)
{	
	printf("interact\n");
	struct tms tmsBegin, tmsEnd;
	clock_t tBegin, tEnd;
	int n1 = 2;
	int n2 = 2;
	int n3 = 2;
	int a[n1][n2];
	int b[n2][n3];
	int c[n1][n3];

	for(int i=0;i < n1;i++)
	   for(int j=0; j < n2;j++)
		a[i][j] = 1;

	for(int i=0;i < n2;i++)
           for(int j=0; j < n3;j++)
                b[i][j] = 1;
	
	if ((tBegin = times(&tmsBegin)) < 0)
        {
            fatal("times()\n");
        }
		
	MatrixMulFn(n1,n2,n3,a,b,c,&err);		
	if ((tEnd = times(&tmsEnd)) < 0) 
	{
    	    fatal("times()\n");
  	}
	long tickRate = sysconf(_SC_CLK_TCK);
	resp.wall = (tEnd - tBegin)*1.0/tickRate;	
	resp.user = (tmsEnd.tms_utime - tmsBegin.tms_utime)*1.0/tickRate; 
	resp.sys =  (tmsEnd.tms_stime - tmsBegin.tms_stime)*1.0/tickRate;

}

void doWorkerService()
{
	printf("worker\n");	
	char *token = strtok(req.module, ".");	
	printf("fun name %s", token);
	void *dlHandle = NULL;	
	MatrixMulFn *naive = getMulFn(req.module, &dlHandle, token, &err);
	interact(req, naive);		
}

void doTrace()
{
	printf("trace");
	char clientFifo[CLIENT_FIFO_NAME_LEN];
	snprintf(clientFifo, CLIENT_FIFO_NAME_LEN, CLIENT_FIFO_TEMPLATE, (long) req.pid);
	char *newp = strcat(req.dir, "/");
	char *p = strcat(newp, clientFifo);
	clientFd = open(p, O_WRONLY);
	if (clientFd == -1) 
	{ 
    		fatal("server open %s\n", clientFifo);
	}
	if(write(clientFd, &resp, sizeof(Response)) != sizeof(Response))
		fprintf(stderr, "Error writing to FIFO");
	if(close(clientFd) == -1)
    		fatal("close\n");

}

void makeWorker()
{
		printf("Daemon started\n");
                pid_t pid; 
                if ((pid = fork()) < 0)
                {       
                        printf("fork error 1: %s\n", strerror(errno));
                        exit(EXIT_FAILURE);
                }
                else if (pid == 0)
                {       
                        if((pid = fork()) < 0)
                        {   
                            printf("fork error 2: %s\n", strerror(errno));
                            exit(EXIT_FAILURE);
                        }
                        else if (pid > 0)
                        {   
                            exit(0);  /* parent from 2nd fork == 1st child */
                        }
                        
                        /* second child. */       
                        printf("second child, pid = %d\n", getpid());
                        
                        //Read PID of the client
                 /*       pid_t clientPID = 0;
                          read(serverFd, &clientPID, sizeof(clientPID))
		 

        		serverFd = open(REQUESTS_FIFO, O_RDONLY);
        		if(serverFd == -1)
                		fatal("server open %s", REQUESTS_FIFO);

                	if(read(serverFd, &req, sizeof(Request)) != sizeof(Request)) {
                        	fprintf(stderr, "Error reading request; discarding\n");
                	}
		*/
			doWorkerService();	
                        
                        //Server make other reads here
                        //and other server operations
                        
                       exit(0); /* I think this exit(0) should kill the second child */
                }
                
                /* wait for first child */
               if(waitpid(pid, NULL, 0) != pid)
               {        
                        printf("waitpid error: %s\n", strerror(errno));
                        exit(EXIT_FAILURE);
               }
}	 

void doDaemonService()
{
	makeWorker();
	doTrace();	
}


pid_t makeDaemon(const char *serverDir)
{
        pid_t processId, sid;

        processId = fork();
        if(processId < 0)
        {
                fatal("fork failed\n");
                exit(EXIT_FAILURE);
        }
        if(processId > 0)
        {
                return processId;
        }
	umask(0);
        sid = setsid();
        if(sid < 0)
        {
                exit(EXIT_FAILURE);
        }
//	char *path = strcat("/", (char *)serverDir);
//	char *p = strcat(path, "/");
	printf("Dir %s\n", serverDir);
        if(chdir(serverDir) < 0)
	{
		exit(EXIT_FAILURE);
	}
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
	assert(0);
}

static pid_t
makeServer(const char *serverDir)
{
	return makeDaemon(serverDir);
}

int
main(int argc, const char *argv[])
{

  if (argc != 2) fatal("usage: %s <server-dir>", argv[0]);
  const char *serverDir = argv[1];
  pid_t pid = makeServer(serverDir);
  printf("%ld\n", (long)pid);

/*        char *path = strcat((char*) serverDir, "/");
	char *p = strcat(path, REQUESTS_FIFO);
	printf("path %s\n", p);
        if(mkfifo(p, S_IRUSR | S_IWUSR | S_IWGRP) == -1 && errno!= EEXIST)
        {
                fatal("mkfifo failed\n");
                exit(EXIT_FAILURE);
        }

        serverFd = open(p, O_RDONLY);
        if(serverFd == -1)
                fatal("server open %s", REQUESTS_FIFO);
        dummyFd = open(p, O_WRONLY);
        if(dummyFd == -1)
                fatal("server opne ");

        if(signal(SIGPIPE, SIG_IGN) == SIG_ERR)
                fatal("server signal");

        while(1)
        {
                if(read(serverFd, &req, sizeof(Request)) != sizeof(Request))
                {
                   fprintf(stderr, "Error reading request; discarding\n");
                }
                doDaemonService();
        }

*/

  return 0;

}
