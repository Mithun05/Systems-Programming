#include "common.h"
#include "matmul.h"

#include "errors.h"

//#define DO_TRACE 1
#include "trace.h"
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>

//char clientFifo[CLIENT_FIFO_NAME_LEN];
extern int errno;

//int serverFd, clientFd; 

struct MatrixMul {
//	Request req;
	int res;
} matrix;

//Request req;
//  Response resp;
//MatrixMul matrix;

/** Return an interface to the client end of a client-server matrix
 *  multiplier set up to multiply using multiplication module
 *  specified by modulePath with server daemon running in directory
 *  serverDir.
 *
 *  If modulePath is relative, then it must be found on the server's
 *  LD_LIBRARY_PATH interpreted relative to serverDir.  The name of
 *  the multiplication function in the loaded module is the last
 *  component of the modulePath with the extension (if any) removed.
 *
 *  If trace is non-NULL, then turn on tracing for all subsequent
 *  calls to mulMatrixMul() which use the returned MatrixMul.
 *  Specifically, after completing each matrix multiplication, the
 *  client should log a single line on trace in the format:
 *
 *  utime: UTIME, stime: STIME, wall: WALL
 *
 *  where UTIME, STIME and WALL gives the amount of user time, system
 *  time and wall time in times() clock ticks needed within the server
 *  to perform only the multiplication function provided by the
 *  module. The spacing must be exactly as shown above and all the
 *  clock tick values must be output in decimal with no leading zeros
 *  or redundant + signs.
 *
 *  Set *err to an appropriate error number (documented in errno(3))
 *  on error.
 *
 *  This call should result in the creation of a new worker process on
 *  the server, spawned using the double-fork technique.  The worker
 *  process must load and link the specified module.  All future
 *  multiplication requests on the returned MatrixMul must be
 *  performed using the specified module within this worker process.
 *  All IPC between the client and server processes must be performed
 *  using only named pipes (FIFO's).
 */
MatrixMul *
newMatrixMul(const char *serverDir, const char *modulePath,
             FILE *trace, int *err)
{
	char clientFifo[CLIENT_FIFO_NAME_LEN];

	int serverFd, clientFd;
	Request req;
	Response resp;


	
//	MatrixMul matrix;
	matrix.res = 1;

	
	snprintf(clientFifo, CLIENT_FIFO_NAME_LEN, CLIENT_FIFO_TEMPLATE, (long) getpid());
	char *dirPath = (char*) serverDir;	
	char *newp = strcat(dirPath, "/");
	char *p = strcat(newp, clientFifo);
	printf("clientfifo %s\n", clientFifo);
	umask(0);
 	if(mkfifo(p, 0666) == -1 && errno != EEXIST) 
		fatal("mkfifo %s\n", clientFifo);
 /*   	if(atexit(removeFifo) != 0)
        	fatal("atexit\n");
*/
	req.pid = getpid();
	strcpy(req.module, modulePath);
	strcpy(req.dir, serverDir);	
	printf("client req newmul %d %s %s\n", req.pid, req.module, req.dir);

	char *s = strcat(newp, REQUESTS_FIFO);
        serverFd = open(s, O_WRONLY);
        if(serverFd == -1)
                fatal("client open dd %s\n", REQUESTS_FIFO);

        if(write(serverFd, &req, sizeof(Request)) != sizeof(Request))
                fatal("Can't dd write to server");

        clientFd = open(p, O_RDONLY);
        if(clientFd == -1)
                fatal("client open pp %s\n", clientFifo);

        if(read(clientFd, &resp, sizeof(Response)) != sizeof(Response))
                fatal("Can't pp read response from server\n");

	unlink(clientFifo);

        printf("utime %2.7f stime %2.7f walltime %2.7f\n", resp.user, resp.sys, resp.wall);

	return &matrix;
}

/*
static void
removeFifo(void)
{
       unlink(clientFifo);
}
*/

/** Free all resources used by matMul.  Specifically, free all memory
 *  used by matMul, remove all FIFO's created specifically for matMul
 *  and set up the worker process on the server to terminate.  Set
 *  *err appropriately (as documented in errno(3)) on error.
 */
void
freeMatrixMul(MatrixMul *matMul, int *err)
{
}

/** Set matrix c[n1][n3] to a[n1][n2] * b[n2][n3].  It is assumed that
 *  the caller has allocated c[][] appropriately.  Set *err to an
 *  appropriate error number (documented in errno(3)) on error.  If
 *  *err is returned as non-zero, then the matMul object may no longer
 *  be valid and future calls to mulMatrixMul() may have unpredictable
 *  behavior.  It is the responsibility of the caller to call
 *  freeMatrixMul() after an error.
 *
 *  The multiplication must be entirely on the server using the
 *  specified module by the worker process which was spawned when
 *  matMul was created.  Note that a single matMul instance may be
 *  used for performing multiple multiplications.  All IPC must be
 *  handled using FIFOs.
 */
void
mulMatrixMul(const MatrixMul *matMul, int n1, int n2, int n3,
             CONST MatrixBaseType a[n1][n2],
             CONST MatrixBaseType b[n2][n3],
             MatrixBaseType c[n1][n3], int *err)
{

/*
	for(int i =0; i < n1; i++)
		for(int j=0;j< n2; j++)
			matMul->req.a[i][j] = a[i][j];


	for(int i= 0; i < n2; i++)
		for(int j = 0; j < n3; j++)
			matMul->req.b[i][j] = b[i][j];
	
	matMul->req.n1 = n1;
	matMul->req.n2 = n2;
	matMul->req.n3 = n3;

//	char *dirPath = req.dir;
	printf("client req matmul %d %d %d", matMul->req.n1, matMul->req.n2, matMul->req.n3);
	serverFd = open(REQUESTS_FIFO, O_WRONLY); 
	if(serverFd == -1)
		fatal("client open %s\n", REQUESTS_FIFO);

	if (write(serverFd, &matMul, sizeof(MatrixMul)) != sizeof(MatrixMul))
		fatal("Can't write to server");

	clientFd = open(clientFifo, O_RDONLY); 
	if (clientFd == -1)
		fatal("client open %s\n", clientFifo);

	if (read(clientFd, &resp, sizeof(Response)) != sizeof(struct Response))
		fatal("Can't read response from server\n");
	    
	printf("utime %2.7f stime %2.7f walltime %2.7f\n", resp.user, resp.sys, resp.wall);
 //   	exit(EXIT_SUCCESS);
*/
  for (int i = 0; i < n1; i++) {
    for (int j = 0; j < n3; j++) {
      c[i][j] = 0;
      for (int k = 0; k < n2; k++) c[i][j] += a[i][k]*b[k][j];
    }
  }
				
}
