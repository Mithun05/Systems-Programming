
/**
 * The source file includes client code to send matrix data to the server.
 * It maps the shared memory segment created by the server.
 */


/**
 * Custom header files
 */
#include "common.h"
#include "matmul.h"
#include "errors.h"

//#define DO_TRACE 1
#include "trace.h"


/**
 * Standard header files
 */
 #include <sys/socket.h>
 #include <sys/types.h>
 #include <netinet/in.h>
 #include <netdb.h>
 #include <stdio.h>
 #include <string.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include <errno.h>
 #include <arpa/inet.h> 


/**
 * Matrix Mul Instance
 */
struct MatrixMul {

	/**
	 * Socket fd
  	 */
 	 int sockFd;

	/**
	 *
	 */
	struct sockaddr_in sIn;
};



/** 
 * Matrix Mul instance 
 */
 MatrixMul matrixMul;


/** Return an interface to the client end of a client-server matrix
 *  multiplier.
 *
 *  Set *err to an appropriate error number (documented in errno(3))
 *  on error.
 *
 */
MatrixMul *
newMatrixMul(const char *hostName, const char *portNo, FILE *trace, int *err)
{

	int portNum = 0;
	int errNum = 0;

	fprintf(stdout, "Input : %s %s\n", hostName, portNo);
	
	portNum = atoi(portNo);
        fprintf(stdout, "port : %d\n", portNum);

	if(portNum < 1024)
	{
		*err = errNum;               // set error no
                fatal("HOST-ADDR PORT\n");  // set error msg
                return NULL;				
	}

	if((matrixMul.sockFd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    	{
		*err = errNum;
        	fatal("Could not create socket\n");
        	return NULL;
    	} 	

	memset(&matrixMul.sIn, 0, sizeof(matrixMul.sIn));

	matrixMul.sIn.sin_family = AF_INET;
	
	if(inet_pton(AF_INET, hostName, &matrixMul.sIn.sin_addr.s_addr) <= 0) 
	{
		*err = errNum;
 		fatal("Cannot convert host address\n");
		return NULL;
 	}
	
	matrixMul.sIn.sin_port = htons((unsigned short) portNum);

	if(connect(matrixMul.sockFd, (const struct sockaddr*) &matrixMul.sIn, sizeof(matrixMul.sIn)) < 0) 
	{
		*err = errNum;	
 		fatal("Cannot connect\n");
		return NULL;
 	}

	return &matrixMul;		
}

/** Free all resources used by matMul.  Specifically, free all memory
 *  used by matMul, Set *err appropriately (as documented in errno(3))
 *  on error.
 */

void
freeMatrixMul(MatrixMul *matMul, int *err)
{
	int errno;
	
	if(close(matMul -> sockFd) == -1)
        {
                *err = errno;
                fatal("Close Error : \n");
        }		
}

void
mulMatrixMul(const MatrixMul *matMul, int n1, int n2, int n3,
		CONST MatrixBaseType a[n1][n2],
		CONST MatrixBaseType b[n2][n3],
		MatrixBaseType c[n1][n3], int *err)
{

	RequestData requestData;
	ResponseData responseData;
	int readSize1, readSize2;
	
	requestData.n1 = n1;
        requestData.n2 = n2;
        requestData.n3 = n3;

        unsigned long asize = n1 * n2 * sizeof(int);
        requestData.aSize =  asize;

        unsigned long bsize = n2 * n3 * sizeof(int);
        requestData.bSize = bsize;
	
	//Send some data
        if(send(matMul -> sockFd, &requestData, sizeof(requestData), MSG_NOSIGNAL) < 0)
        {
	    *err = EIO;		
            fatal("Send failed\n");
        }

	requestData.A = (int*) malloc(n1 * n2 * sizeof(int));

        requestData.B = (int*) malloc(n2 * n3 * sizeof(int));

        for(int i = 0; i <  n1; i++)
                for(int j = 0; j < n2; j++)
                        *(requestData.A + i*n2 + j) = a[i][j];	

	//Send some data
        if(send(matMul -> sockFd, requestData.A, asize, 0) < 0)
        {                                             
            *err = EIO;        
            fatal("Send failed\n");
        }

	for(int i = 0; i <  n2; i++)
                for(int j = 0; j < n3; j++)
                        *(requestData.B + i*n3 + j) = b[i][j];

	//Send some data
        if(send(matMul -> sockFd, requestData.B, bsize, MSG_NOSIGNAL) < 0)
        {
            *err = EIO;
            fatal("Send failed\n");
        }

	if((readSize1 = recv(matMul -> sockFd, &responseData, sizeof(responseData), 0)) < 0)
        {
           *err = EACCES;
           fatal("Cannot receive\n");
        }

	if(readSize1 > 0)
	{
           fprintf(stdout, "utime: %ld, stime: %ld, wall: %ld \n", responseData.user, responseData.sys, responseData.wall);
	}

	int errCheck = responseData.err;
	if(errCheck == 0)
        {
                unsigned long csize = responseData.cSize;
                responseData.C = malloc(csize);
                if(responseData.C == NULL)
                {
                        *err = errno;
                        fatal("Malloc Error\n");
                }

                /**
                 * Set response matrix data into C
                 */
		if((readSize2 = recv(matMul -> sockFd, responseData.C, csize, 0)) < 0)
        	{ 
           		*err = EACCES;
		        fatal("Cannot receive\n");
        	} 
                if(readSize2 > 0)
                {
                        for(int i = 0; i < n1; i++)
                                for(int j = 0; j < n3; j++)
                                        c[i][j] = *(responseData.C + i*n3 + j);
                }
        }
	else
	{
		fprintf(stderr, "mulMatrixMul(): %s", strerror(errCheck));
                fprintf(stdout, "\n");

                free(requestData.A);
                free(requestData.B);
                free(responseData.C);
                freeMatrixMul((MatrixMul*) matMul, &errCheck);
                exit(EXIT_FAILURE);
	}

	//free all memory
        free(requestData.A);
        free(requestData.B);
        free(responseData.C);  
}

