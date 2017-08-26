#include "matrix_mul.h"

#include "errors.h"

//#define DO_TRACE 1
#include "trace.h"

#include <errno.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

/**
 * The Header file for wait system call
 */
 #include <sys/wait.h>


/**
 * The declaration holds the constant dimension of Communication Channel
 * 0 for reading from pipe and 1 for writing to pipe so the count is 2
 */
 #define FDCONSTANT 2

/**
 * The declaration holds to clanup memory and exit processes on error in method 
 */
 MatrixBaseType cleanUpFlag;

/**
 * The Variable holds the declaration of one of the dimensions of file descriptor.
 */

 MatrixBaseType fileDescCols; 

/**
 * The Variable holds the declaration of errno macro
 */
 extern MatrixBaseType errno;

/**
 * The Struct contains information about number of Worker processes,
 * Worker processes pool, pipes for inter-process communication between client 
 * and workers, trace enabled/disabled flag.	
 */

 struct MatrixMul {
	
     /*
      *	Total number of workers - Slave processes
      */ 
     MatrixBaseType noOfWorkers;
	
     /*
      * Check enable/disable trace flag
      */
     _Bool traceFlag;		

     /*
      * This pipe associated with sending matrix multiplicand and 
      * matrix multiplier data to worker processes.
      * Pipe Function performs :	
      * 1. It reads rows and columns elements from two matrices and sends that data to
      *    to worker processes which is selected on round-robin fashion.
      * 2. Read dot product from the worker process and then store it into the final matrix.		
      */				
     MatrixBaseType **fileDescParentToWorkers;	

     /*
      * This pipe associated with performing matrix multiplication i.e dot product
      * with data received from client process
      * Function it performs :
      * 1. Reads input data that was sent by the client
      * 2. Performs dot product
      * 3. Send the result back to the client process		
      */	  	
     MatrixBaseType **fileDescWorkersToParent;			

     /*
      * Worker processes pool - Each Unique Process Id - Child processes
      */	
     pid_t *workers;			

 };

/**
 * Matrix Multiplier Variable Declaration
 */
struct MatrixMul *matrixMul;


/** Return a multi-process matrix multiplier with nWorkers worker
 *  processes.  Set *err to an appropriate error number (documented in
 *  errno(3)) on error.
 *
 *  If trace is not NULL, turn on tracing for calls to mulMatrixMul()
 *  using returned MatrixMul.  Specifically, each dot-product
 *  computation must be logged to trace as a line in the form:
 *
 *  INDEX[PID]: [I]x[J] = Z
 *
 *  where INDEX in [0, nWorkers) is the index of the worker and PID is
 *  its pid, I is the index of the row in the multiplicand, J is the
 *  index of the row in the multiplier and Z is their dot-product
 *  computed by child process PID.  The spacing must be exactly as
 *  shown above and all values must be output in decimal with no
 *  leading zeros or redundant + signs.
 *
 */
MatrixMul *
newMatrixMul(int nWorkers, FILE *trace, int *err)
{

	/**
	 * File descriptor 0 for reading and 1 for writing
	 * This is the second dimension of each worker process file descriptor
 	 * Total number of pipes = total number of worker processes 
	 */
	fileDescCols = FDCONSTANT;				

	/**
	 * Memory allocation
	 */
 	matrixMul = malloc(sizeof(struct MatrixMul));
	if(matrixMul == NULL)
	{
		*err = errno;		     // set error no
		fatal("Malloc Error : \n");  // set error msg
		return NULL;
	}

	matrixMul -> noOfWorkers = nWorkers;	// number of worker processes
	
	/**
	 * This is used to check whether trace is null or not.
         * And set the traceflag accordingly for tracing the output of dot product 
	 */
	if(trace != NULL)
	{
		matrixMul -> traceFlag = true;	// enable log for the output
	}
	else
	{
		matrixMul -> traceFlag = false;	// disable the log for the output
	}

	matrixMul -> workers = malloc(sizeof(MatrixBaseType) * nWorkers);
	
	if(matrixMul -> workers == NULL)
	{
		*err = errno;			// set error no
		free(matrixMul);		// free memory
		fatal("Malloc Error : \n");	// set error msg
		return NULL;
	}

	/**
	 * Setting up file descriptor for inter-process communication
         * Pipe per each worker process 
	 * This is used for client to worker process communication
         */
	matrixMul -> fileDescParentToWorkers = malloc(sizeof(MatrixBaseType *) * nWorkers);
	if(matrixMul -> fileDescParentToWorkers == NULL)
	{
		*err = errno;			// set error no
		free(matrixMul -> workers);	
		free(matrixMul);
		fatal("Malloc Error : \n");     // set error msg
		return NULL;
	}

	/**
 	 * Specifies two indexes for each row associated process
	 * File descriptor - 0 for reading and 1 for writing  
	 * Dynamic allocation
	 */
	for(int counter = 0; counter < nWorkers; counter++)
	{
		matrixMul -> fileDescParentToWorkers[counter] = malloc(sizeof(MatrixBaseType) * fileDescCols);
	}


	/**
         * Setting up file descriptor for inter-process communication
         * Pipe per each worker process 
         * This is used for workers to client process communication
         */
	matrixMul -> fileDescWorkersToParent = malloc(sizeof(MatrixBaseType *) * nWorkers);
        if(matrixMul -> fileDescWorkersToParent == NULL)
        {
                *err = errno;		// set error no
                free(matrixMul -> workers);
                free(matrixMul);
                fatal("Malloc Error : \n");	// set error msg
                return NULL;
        }

	/**
         * Specifies two indexes for each row associated process
         * File descriptor - 0 for reading and 1 for writing  
         * Dynamic allocation
         */
	for(int counter = 0; counter < nWorkers; counter++)
        {
                matrixMul -> fileDescWorkersToParent[counter] = malloc(sizeof(MatrixBaseType) * fileDescCols);
        }

		
	/**
	 * Creating worker process pool.
         * Creating pipes for parent - children process and children - parent communication 
         * Setting up error codes 
         */	
	for(int worker_counter = 0; worker_counter < nWorkers; worker_counter++)
        {
		  /**
		   * creating pipes pair		
		   */
                  if(pipe(matrixMul -> fileDescParentToWorkers[worker_counter]) < 0 || pipe(matrixMul -> fileDescWorkersToParent[worker_counter]) < 0)
                  {
			cleanUpFlag = 1;
                        *err = errno;
			fatal("Cannot create pipe : \n");
                        exit(EXIT_FAILURE);
                  }

		  /**
		   * fork error checking
		   */	
                  if((matrixMul -> workers[worker_counter] = fork()) == -1)
                  {
			cleanUpFlag = 1;
                        *err = errno;
			fatal("Cannot create worker process : %d \n", worker_counter);   
                   	exit(EXIT_FAILURE);
                  }

		  /**
		   * worker process
		   */
                  else if(matrixMul -> workers[worker_counter] == 0)
                  {
			if(close(matrixMul -> fileDescWorkersToParent[worker_counter][0]) < 0)
			{
				cleanUpFlag = 1;
				*err = errno;
				fatal("Child : %d, pid :%ld, close(fd[%d][0]) \n", worker_counter, (long) getpid(), worker_counter);
			}
			if(close(matrixMul -> fileDescParentToWorkers[worker_counter][0]) < 0)
			{
				cleanUpFlag = 1;
                                *err = errno;
                                fatal("Child : %d, pid :%ld, close(fd[%d][0]) \n", worker_counter, (long) getpid(), worker_counter);
			}

                        matrixMul -> workers[worker_counter] = getpid();
                        exit(EXIT_SUCCESS);
                  }
 		  
 		  else
		  {
			/**
			 * This check stands to create process fan and not process chain.
			 * Either you use wait in the parent process like shown below.
			 * The alternative is to use break inside the child.I prefer this one. 
			 */
			wait(NULL);
		  } 
        }
	
					
	return matrixMul;	 		// return matrix multiplier structure to be used further 
}

/** Free all resources used by matMul.  Specifically, free all memory
 *  and return only after all child processes have been set up to
 *  exit.  Set *err appropriately (as documented in errno(3)) on error.
 */
void
freeMatrixMul(MatrixMul *matMul, int *err)
{

	int status;
	/**
         * Wait for all child processes to finish and exit if an error occurs
	 * This only finishes the child process when an error occurs. 
         */
	if(cleanUpFlag == 1)
	{
        	for(int waitCounter = 0; waitCounter < matMul -> noOfWorkers; waitCounter++)
        	{
                	waitpid(matMul-> workers[waitCounter], &status, 0);
        	}
		cleanUpFlag = 0;
	}


	/**
	 * Free all the memory used by Matrix Multiplier resources
	 */

	if(matMul != NULL)
	{
		free(matMul -> workers);
		for(int counter = 0; counter < matMul -> noOfWorkers; counter++)
		{
			free(matMul -> fileDescParentToWorkers[counter]);
		}
		free(matMul -> fileDescParentToWorkers);

		for(int counter = 0; counter < matMul -> noOfWorkers; counter++)
		{
			free(matMul -> fileDescWorkersToParent[counter]);	
		}
		free(matMul -> fileDescWorkersToParent);
		free(matMul);
	}
	else
	{
		 *err = errno;
		 fatal("Cannot free the allocated memory : \n");
	}

}

/** Set matrix c[n1][n3] to a[n1][n2] * b[n2][n3].  It is assumed that
 *  the caller has allocated c[][] appropriately.  Set *err to an
 *  appropriate error number (documented in errno(3)) on error.  If
 *  *err is returned as non-zero, then the matMul object may no longer
 *  be valid and future calls to mulMatrixMul() may have unpredictable
 *  behavior.  It is the responsibility of the caller to call
 *  freeMatrixMul() after an error.
 *
 *  All dot-products of rows from a[][] and columns from b[][] must be
 *  performed using the worker processes which were already created in
 *  newMatrixMul() and all IPC must be handled using anonymous pipes.
 *  The multiplication should be set up in such a way so as to allow
 *  the worker processes to work on different dot-products
 *  concurrently.
 */
void
mulMatrixMul(const MatrixMul *matMul, int n1, int n2, int n3,
             CONST MatrixBaseType a[n1][n2],
             CONST MatrixBaseType b[n2][n3],
             MatrixBaseType c[n1][n3], int *err)
{
	
	/**
	 * Matrix Multiplication variable declarations
	 */
        MatrixBaseType firstCounter, secondCounter, thirdCounter;

	/**
	 * Multiplicand element
	 */
	MatrixBaseType multiplicandElement;
	
	/**
	 * Multiplier element
	 */
	MatrixBaseType multiplierElement;

	/**
  	 * Worker process status flag variable declaration
	 */
	MatrixBaseType status;

	/**
	 * Counter to check number of processes to wait for variable declaration
	 */
        MatrixBaseType waitCounter;
	
	/**
	 * Worker process selection variable declaration
	 * While sending the data, it should get dispersed to multiple worker processes concurrently
	 * Using round-robin fashion to distribute data to processes
 	 * This variable holds the typical worker process id.  
	 */
        MatrixBaseType workerProcessPoolIdSelection = 0;


	/**
	 * Matrix Multiplication Mechanism 
	 * Selects Worker process ID from worker process pool in round-robin fashion
	 * Sends matrix data to workers 
	 * Reports error codes
	 * All client- worker communication is using anonymous pipes 
	 * Send all the matrix data to worker processes
	 */
	for(firstCounter = 0; firstCounter < n1; firstCounter++)
        {
			/**
			 * Select worker process id from the pool
			 */
			workerProcessPoolIdSelection = (workerProcessPoolIdSelection + 1) % matMul -> noOfWorkers;

                        for(secondCounter = 0; secondCounter < n3; secondCounter++)
                        {

                                for(thirdCounter = 0; thirdCounter < n2; thirdCounter++)
                                {

				   multiplicandElement = a[firstCounter][thirdCounter];
				   multiplierElement   = b[thirdCounter][secondCounter];

				   /*
				    * Matrix Element
				    */	
				   if(write(matMul -> fileDescParentToWorkers[workerProcessPoolIdSelection][1], &multiplicandElement, sizeof(multiplicandElement)) == -1 ) 
				   {
    					    cleanUpFlag = 1;
                                            *err = errno;
                                            fatal("Error writing to pipe : \n");
                                            freeMatrixMul((struct MatrixMul *) matMul, err);
                                            exit(EXIT_FAILURE);

                                   }

				   /**
				    * Matrix Element
				    */	
                                   if (write(matMul -> fileDescParentToWorkers[workerProcessPoolIdSelection][1], &multiplierElement, sizeof(multiplierElement)) == -1 ) 
				   {
					    cleanUpFlag = 1;
                                            *err = errno;
                                            fatal("Error writing to pipe : \n");
                                            freeMatrixMul((struct MatrixMul *) matMul, err);
                                            exit(EXIT_FAILURE);

                                   }
		
                                }
                      }
        }


	/**
	 * Multiplicand Element
	 */
	MatrixBaseType multiplicandElementR;
	
	/**
	 * Multiplier Element
	 */
	MatrixBaseType multiplierElementR;

	/**
  	 * Retrieve selected process id from the worker processes pool
	 */
	MatrixBaseType workerProcessPoolIdDisplay = 0;

	/**
	 * Holds the results of the dot product of matrix performed by the different worker process
	 */
	MatrixBaseType sum = 0;

	
	/**
	 * Worker processes from the pool used to perform the matrix multiplication
	 * Dot product calculation and log the output
	 * Closing unused file descriptor
	 * Reading dispersed matrix data from the parent 
	 * Send the dot-product result to client
	 */
	for(firstCounter = 0; firstCounter < n1; firstCounter++)
        {		
			/**
			 * Worker process selection id
			 */
			workerProcessPoolIdDisplay = (workerProcessPoolIdDisplay + 1) % matMul -> noOfWorkers;

			/**
			 * Closing unused file descriptors
			 */
			close(matMul -> fileDescParentToWorkers[workerProcessPoolIdDisplay][1]);

                        for(secondCounter = 0; secondCounter < n3; secondCounter++)
                        {
				sum = 0;
				for(thirdCounter = 0; thirdCounter < n2; thirdCounter++)
				{
				 	ssize_t numRead;
                      			if((numRead = read(matMul -> fileDescParentToWorkers[workerProcessPoolIdDisplay][0], &multiplicandElementR, sizeof(multiplicandElementR))) == -1 ) 
					{
	                                    cleanUpFlag = 1;
                                            *err = errno;
                                            fatal("Error writing to pipe : \n");
                                            freeMatrixMul((struct MatrixMul *) matMul, err);
                                            exit(EXIT_FAILURE);

					}
                                	else if(numRead == 0) 
					{
	  				    cleanUpFlag = 1;
                                            *err = errno;
                                            fatal("Pipe closed : \n");
                                            freeMatrixMul((struct MatrixMul *) matMul, err);
                                            exit(EXIT_FAILURE);
                                	}

                      			if((numRead = read(matMul -> fileDescParentToWorkers[workerProcessPoolIdDisplay][0], &multiplierElementR, sizeof(multiplierElementR))) == -1 ) 
					{
	    				    cleanUpFlag = 1;
                                            *err = errno;
                                            fatal("Error writing to pipe : \n");
                                            freeMatrixMul((struct MatrixMul *) matMul, err);
                                            exit(EXIT_FAILURE);

                                	}
	
         	                        else if(numRead == 0) 
					{
					    cleanUpFlag = 1;
                                            *err = errno;
                                            fatal("Pipe closed : \n");
                                            freeMatrixMul((struct MatrixMul *) matMul, err);
                                            exit(EXIT_FAILURE);

                                 	}

					/**
					 * Perform Matrix Multiplication
					 */
	
	                                 sum += multiplicandElementR * multiplierElementR;
 
				}

				/**
				 * Enable/Disable trace flag
				 * If flag is true then log the output in the required format
				 * If flag is false then no need to log the output
				 * Shown the ith row of multiplicand and ith column of multiplier, product, worker process id and its pool index 
				 */

				if(matMul -> traceFlag)
                                {
                                     fprintf(stdout, "%d[%d]: [%d]x[%d] = %d\n", 
								workerProcessPoolIdDisplay, 
									matMul-> workers[workerProcessPoolIdDisplay], 
											firstCounter, secondCounter, sum);
                                }

				/**
				 * Send the dot-product performed by the typical worker to the client process 
				 * To update sum in final matrix
				 */

				 if(write(matMul -> fileDescWorkersToParent[workerProcessPoolIdDisplay][1], &sum, sizeof(sum)) == -1 ) 
				 {
					    cleanUpFlag = 1;
                                            *err = errno;
                                            fatal("Pipe closed : \n");
                                            freeMatrixMul((struct MatrixMul *) matMul, err);
                                            exit(EXIT_FAILURE);
        		         }
                        }
	
        }


	/**
	 * The Variable holds the final result of final matrix
	 */
	MatrixBaseType result = 0;

	/**
	 * This is used to retrieve product performed by each worker and
	 * update the result into the final matrix by the parent
	 */
	MatrixBaseType workerProcessPoolId = 0;

        for(firstCounter = 0; firstCounter < n1; firstCounter++)
        {
		/**
		 * Remember which typical worker sending the data to the client 
		 */
		workerProcessPoolId = (workerProcessPoolId + 1) % matMul -> noOfWorkers;		

                for(secondCounter = 0; secondCounter < n3; secondCounter++)
                {
                                ssize_t numRead;
                                if((numRead = read(matMul -> fileDescWorkersToParent[workerProcessPoolId][0], &result, sizeof(result))) == -1)
                                {
					 cleanUpFlag = 1;
                                         *err = errno;
                                         fatal("Error reading from the pipe : \n");
                                         freeMatrixMul((struct MatrixMul *) matMul, err);
                                         exit(EXIT_FAILURE);

                                }
                                else if(numRead == 0) 
				{
					  cleanUpFlag = 1;
                                          *err = errno;
                                          fatal("Pipe closed : \n");
                                          freeMatrixMul((struct MatrixMul *) matMul, err);
                                          exit(EXIT_FAILURE);

                                }
                                else 
				{
                                 	c[firstCounter][secondCounter] = result;
                                }
                }
        }


	/**
	 * Removed or close the file descriptors after finishing the task
	 *
	 */
	 for(MatrixBaseType removeCounter = 0 ; removeCounter < matMul -> noOfWorkers; removeCounter++)
	 {
		close(matMul -> fileDescParentToWorkers[removeCounter][0]);
		close(matMul -> fileDescWorkersToParent[removeCounter][0]);
		close(matMul -> fileDescParentToWorkers[removeCounter][1]);
                close(matMul -> fileDescWorkersToParent[removeCounter][1]);
	 }


	/**
	 * Wait for all child processes to finish and exit
	 */
        for(waitCounter = 0; waitCounter < matMul -> noOfWorkers; waitCounter++)
        {
                waitpid(matMul-> workers[waitCounter], &status, 0);
        }

	// Done Multi-process Matrix Multiplier with Client and Worker processes	
}
