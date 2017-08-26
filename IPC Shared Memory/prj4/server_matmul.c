
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
#include <semaphore.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <stdlib.h>
#include <ctype.h>

/**
* Server Error handling for Shared Memory
*/
int errNo = 0;

/**
 * File pointer to log errors
 */
FILE *filp = NULL; 

/**
 * Initialization of semaphore IPC objects.
 */

static SemOpenArgsServer semArgs[] = {

	{ 
		.posixName = SERVER_SEM_NAME,
		.oflags = O_RDWR|O_CREAT,
		.mode = ALL_RW_PERMS,
		.initValue = 1,
	},

	{ 
		.posixName = REQUEST_SEM_NAME,
		.oflags = O_RDWR|O_CREAT,
		.mode = ALL_RW_PERMS,
		.initValue = 0,
	},

	{ 
		.posixName = RESPONSE_SEM_NAME,
		.oflags = O_RDWR|O_CREAT,
		.mode = ALL_RW_PERMS,
		.initValue = 0,
	},
};

/** 
 * Free resources on an error
 * Cleanup work for server errors 
 */
 void cleanUpMemory(int fd)
 {
	/**
	 * Remvoe file 
	 */
	 int fileRet = remove(MEM_SIZE_SHARED_FILE);
	 if(fileRet != 0)
	 {
		fprintf(filp, "cleanUpMemory() : Cannot remove file\n");
		fflush(filp);
	 }
	
	
	/**
	 * Close file descriptor
	 */
	 if(close(fd) < 0)
	 {
		fprintf(filp, "cleanUpMemory() : Cannot close fd\n");
		fflush(filp);
	 }

	
	/**
	 * Remove shared memory
	 */
	int shmErr = shm_unlink(SHM_NAME);
        if(shmErr != 0)
	{
        	fprintf(filp, "cleanUpMemory() : Cannot unlink shared memory %s\n", strerror(shmErr));
		fflush(filp);
	}

 }

 /**
  * Cleanup semaphores
  */
 void cleanUpResources()
 {

	/**
         * Unlink semaphores
         */
        sem_unlink(SERVER_SEM_NAME);
        sem_unlink(REQUEST_SEM_NAME);
        sem_unlink(RESPONSE_SEM_NAME);

 }


/**
 * Daemon Service.
 * Used to do matrix multiplication, the request which is sent by the client.
 */

static void
doDaemonService(long shmMemSize)
{
	/**
	 *
	 */
	 int err = 0;


	/**
	 * Communicate from server to client error handling mechanism 
	 */
	int mapErr = 0;	

	/**
	 *  Initialize the semaphore IPC object on the server side  
	 */	
	sem_t *sems[N_SEMS];
	for (int i = 0; i < N_SEMS; i++) 
	{ 
		const SemOpenArgsServer *p = &semArgs[i];
		if ((sems[i] = sem_open(p->posixName, p->oflags, p->mode, p->initValue)) == NULL)
		{
			err = EIO;	
			fprintf(filp, "doDeamonService() : Cannot create semaphore %s %s\n", p -> posixName, strerror(err));
			fflush(filp);
		}
	}

	/** 
	 * Shared memory segment
	 */
	int fd = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL, ALL_RW_PERMS);
	if(fd < 0) 
	{
			errNo = EEXIST;
			fprintf(filp, "doDeamonService() : Shm exists already %s %s\n", SHM_NAME, strerror(errNo));
			fflush(filp);
			exit(EXIT_FAILURE);
	}

	/**
	 * Initialize the shared memory segment
	 */
	if(ftruncate(fd, shmMemSize) < 0)
	{
		err = EPERM;
		cleanUpMemory(fd);
		cleanUpResources();
		fprintf(filp, "doDeamonService() : Cannot Size shm %s to %ld, %s\n", SHM_NAME, shmMemSize, strerror(err));	
		fflush(filp);
	}

	/**
	 * Matrix Data mapped into the memory
	 */
	MatrixData *matrixData = NULL;
	if((matrixData = mmap(NULL, shmMemSize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED) 
	{
		err = ENOMEM;
		cleanUpMemory(fd);
                cleanUpResources();
		fprintf(filp, "doDeamonService() : Mapped Allocation Error %s\n", strerror(err));
		fflush(filp);
	}


	/**
         * Write memory size to the file so client can read the memory size
         */
        FILE *fp = NULL;
        fp = fopen(MEM_SIZE_SHARED_FILE, "w+");
        if(fp == NULL)
        {
            err = ENOENT;
            fprintf(filp, "makeDaemon() : Cannot open file for writing %s\n", strerror(err));
            fflush(filp);
        }

        /**
         * Write memory size to the file
         */
        fprintf(fp, "%ld", shmMemSize);
        fflush(fp);

        /**
         * Close the resource
         */
        fclose(fp);

	
	/**
	 * Wait for client request
	 */
	while(1)
	{

		/**
		 * Wait for the client request 
		 */
		if(sem_wait(sems[REQUEST_SEM]) < 0) 
		{
			mapErr = EIO;
			matrixData -> err = mapErr;	
			munmap(matrixData, shmMemSize);		
			cleanUpMemory(fd);
	                cleanUpResources();	
			fprintf(filp, "doDeamonService() : Wait error on sem %s, %s\n", REQUEST_SEM_NAME, strerror(mapErr));
			fflush(filp);
		}

		// Code For Matrix Multiplication

		int n1 = matrixData -> n1;
		int n2 = matrixData -> n2;
		int n3 = matrixData -> n3;		

		/** 
		 * Store final matrix data
		 */ 
		int sum = 0;

		/**
		 * Temporary array for Matrix A and Matrix B to compute result matrix data
		 */
		MatrixBaseType A[n1][n2];
		MatrixBaseType B[n2][n3];


		/**
	 	 * Matrix A Data
	 	 */
		for(int i = 0; i < n1; i++) 
			for(int j = 0; j < n2; j++) 
				A[i][j] = *(&matrixData -> ABCMatrices + i*n2 + j);

		/**
		 * Matrix B offset
		 */
		int BOffset = n1*n2;	

		/**
		 * Matrix B Data
		 */
		for(int i = 0; i < n2; i++)
			for(int j = 0; j < n3; j++)
				B[i][j] = *(&matrixData -> ABCMatrices + BOffset + i*n3 + j);

		/**
		 * Matrix C offset
		 */
		int COffset = BOffset + n1*n3;

		/**
		 * Matrix Multiplication 
		 */
		for(int i = 0; i < n1; i++) {
			for (int j = 0; j < n3; j++) {
				for (int k = 0; k < n2; k++) {
					sum += A[i][k] * B[k][j];
				}
				*(&matrixData -> ABCMatrices + COffset + i*n3 + j) = sum;
				sum = 0;	 
			}
		}

		/**
		 * Signal client to store the response
		 */
		if(sem_post(sems[RESPONSE_SEM]) < 0) 
		{
			mapErr = EIO;
			matrixData -> err = mapErr;
			munmap(matrixData, shmMemSize);
                        cleanUpMemory(fd);
                        cleanUpResources();
			fprintf(filp, "Cannot post sem %s, %s\n", RESPONSE_SEM_NAME, strerror(mapErr));
			fflush(filp);
		}	
	}	
}

/**
 * Function is used to create a daemon service
 */
static pid_t 
makeDaemon(long shmMemSize)
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
        fprintf(filp, "Daemon Started With PID : %d\n", getpid());
        fflush(filp);

	doDaemonService(shmMemSize);	
	assert(0);

}

/**
  * Function is send to create background service to serve multiple client request
  */
static pid_t
makeServer(long shmMemSize)
{
	return makeDaemon(shmMemSize);
}

/**
 * Function is used to check if an input contains anything other than digit.
 */
static int 
checkValidInput(const char *input)
{
    while (*input) 
    {
        if(isdigit(*input++) == 0) return 0;
    }

    return 1;
}

int main(int argc, const char *argv[])
{
	if(argc != 2) fatal("usage: %s <shm-size-kib>", argv[0]);
	const char *inputMemSize = argv[1];
	int status = checkValidInput(inputMemSize);
	
	/**
	 * Check for numbers only : Valid Input
	 */
	if(status == 0)
	{
	 	fatal("usage: %s <shm-size-kib>", argv[0]);
	}

	int shmMemSize = atoi(argv[1]);
	if(shmMemSize < 0)
	{
		fatal("usage: %s <shm-size-kib>", argv[0]);
	}

	/**
	 * Convert the number into bytes
	 */
	long shmMemSizeInBytes = shmMemSize * 1024;
	pid_t pid = makeServer(shmMemSizeInBytes);
	printf("%ld\n", (long)pid);
	return 0;
}



