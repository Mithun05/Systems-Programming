
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
#include <semaphore.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>


/**
 * Matrix Mul Instance
 */
struct MatrixMul {

	/**
	 * File descriptor.
	 * It is used in memory segment mapping.
	 */
	int fd;

	/**
	 * Matrix Data.
	 * Data mapped in memory.
	 */
	MatrixData *matrixData;

	/**
	 * Semaphores.
	 * Used multiple semaphores instances.
	 */
	sem_t *sems[N_SEMS];

	/**
	 * Memory Size
	 * Mapped memory size 
	 */
	long shmMemSize;
};



/**
 * Dealing with an error handling mechanism
 */
extern int errno;

/** 
 * Matrix Mul instance 
 */
MatrixMul matrixMul;


/**
 * Initialization of semaphore instances to be used in order send matrix data from client to server and receive response from the server.
 */
static SemOpenArgsClient semArgs[] = {

	{ 
		.posixName = SERVER_SEM_NAME,
		.oflags = O_RDWR,
	},

	{ 
		.posixName = REQUEST_SEM_NAME,
		.oflags = O_RDWR,
	},

	{ 
		.posixName = RESPONSE_SEM_NAME,
		.oflags = O_RDWR,
	},
};

/**
 * Semaphore wait operation .
 * It blocks the client when sem value is zero.
 * This is similar to sleep operation in general.
 */
static void
semWait(sem_t *sem, const char *posixName, int *err)
{
	if(sem_wait(sem) < 0) 
	{
		*err = errno;
		fatal("Cannot wait on sem : %s\n", posixName);
	}
}

/**
 * Semaphore post operation.
 * It wakes up all the processes waiting on this sem and schdules either of them only one.
 * It is similar to wakeup call in general.
 */
static void
semPost(sem_t *sem, const char *posixName, int *err)
{
	if(sem_post(sem) < 0) 
	{
		*err = errno;		
		fatal("Cannot post sem : %s\n", posixName);
	}
}

/** Return an interface to the client end of a client-server matrix
 *  multiplier.
 *
 *  Set *err to an appropriate error number (documented in errno(3))
 *  on error.
 *
 */
MatrixMul *
newMatrixMul(int *err)
{

	/**
	 * Initialize the semaphores
	 */
	for(int i = 0; i < N_SEMS; i++) 
	{
		const SemOpenArgsClient *p = &semArgs[i];
		if((matrixMul.sems[i] = sem_open(p->posixName, p->oflags)) == NULL) 
		{
			*err = errno;
			fatal("newMatrixMul() : Cannot open semaphore %s\n", p->posixName);
			return NULL;
		}
	}


	FILE *fp = NULL;
        long shmMemSize;

	/** 
	 * Open the file for reading
	 */
        fp = fopen(MEM_SIZE_SHARED_FILE, "r");
        if(fp == NULL) 
	{
	   *err = errno;	
           fatal("newMatrixMul() : Cannot open file for reading.\n");
        }
	
	/**
	 * Read memory size
	 */ 
	fscanf(fp, "%ld", &shmMemSize);
	matrixMul.shmMemSize = shmMemSize;

	/**
	 * Close the resource
	 */
	 fclose(fp);

	/**
	 * Shared memory mapping name
	 */	
	int fd = shm_open(SHM_NAME, O_RDWR, 0);
	if (fd < 0) 
	{
		*err = errno;
		fatal("newMatrixMul() : Cannot open shm %s\n", SHM_NAME);
		return NULL;
	}

	matrixMul.fd = fd;

	matrixMul.matrixData = NULL;	

	/**
	 * Actual Memory Mapping initialization using system call given
	 */
	if((matrixMul.matrixData = mmap(NULL, shmMemSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED) 
	{
		*err = errno;
		fatal("newMatrixMul() : Cannot mmap shm %s\n", SHM_NAME);
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

    /**
     * Close file descriptor
     */
    if(close(matMul -> fd) == -1)
    {
	*err = errno;
        fatal("freeMatrixMul() : Cannot Close\n");	
    }		
	
    /**
     * Unmap mapped memory
     */				
    if(munmap(matMul -> matrixData, matMul -> shmMemSize) < 0) 
    {
	*err = errno;
	fatal("freeMatrixMul() : Cannot Unmap\n");
    }

  
    /**
     * Close semaphores
     */	
     for(int i = 0; i < N_SEMS; i++)
     {
             sem_close(matrixMul.sems[i]); 
     }
	
		
}

void
mulMatrixMul(const MatrixMul *matMul, int n1, int n2, int n3,
		CONST MatrixBaseType a[n1][n2],
		CONST MatrixBaseType b[n2][n3],
		MatrixBaseType c[n1][n3], int *err)
{


	/**
	 * Check for the map size whether array would fit in or not
	 * First entry is for integer error value
	 */
	long totalSize = sizeof(int) + sizeof(n1) + sizeof(n2) + sizeof(n3) + n1*n2*sizeof(MatrixBaseType) + n2*n3*sizeof(MatrixBaseType) + n1*n3*sizeof(MatrixBaseType);
	if(totalSize > matMul -> shmMemSize)
	{
		fprintf(stderr, "matMul(): Value too large for defined data type\n");
		freeMatrixMul((struct MatrixMul *) matMul, err);
		exit(EXIT_FAILURE);
	}

	/**	
	 * Wait semaphore
	 */
	semWait(matMul -> sems[SERVER_SEM], SERVER_SEM_NAME, err);

	/**
	 * Deal with an error on server side
	 */
	matMul -> matrixData -> err = 0;

	/**
	 * Set dimension data into the memory
	 * rows and columns
	 */ 
	matMul -> matrixData -> n1 = n1;
	matMul -> matrixData -> n2 = n2;
	matMul -> matrixData -> n3 = n3;

	
	/**
	 * Matrix data A mapped into the memory
	 */ 
	for(int i = 0; i < n1; i++)
		for(int j = 0; j < n2; j++) 
			*(&matMul -> matrixData -> ABCMatrices + i*n2 + j) = a[i][j];

	/**
	 * Matrix B offset - Relative address to the beginning memory block A
	 * 
	 */
	int BOffset = n1 * n2;


	/**
	 * Matrix data B mapped into the memory
	 *
	 */
	for(int i = 0; i < n2; i++) 
		for(int j = 0; j < n3; j++) 
			*(&matMul -> matrixData -> ABCMatrices + BOffset + i*n3 + j) = b[i][j];


	semPost(matMul -> sems[REQUEST_SEM], REQUEST_SEM_NAME, err);
	semWait(matMul -> sems[RESPONSE_SEM], RESPONSE_SEM_NAME, err);

	/**
	 * Check for an error in memory mapping while receving reponse from the server
	 */
	if(matMul -> matrixData -> err != 0)
	{
		*err = matMul -> matrixData -> err;
		fprintf(stderr, "matMul(): error in server %s\n", strerror(*err));
                exit(EXIT_FAILURE);	
	}
	

	/**
	 * Matrix C offset - Relative address to the beginning memory block A
	 */
	int COffest = BOffset + n1*n3;


	/**
	 * Set Matrix C into the memory
	 */
	for(int i = 0; i < n1; i++) 
		for(int j = 0; j < n3; j++)  
			c[i][j] = *(&matMul -> matrixData -> ABCMatrices + COffest + i*n3 + j);

	/**
	 * Ready for the next client request to handle
	 */
	semPost(matMul -> sems[SERVER_SEM], SERVER_SEM_NAME, err);
}

