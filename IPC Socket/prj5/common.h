#ifndef _COMMON_H
#define _COMMON_H

#include "mat_base.h"

#include <stdbool.h>
#include <stdio.h>



/**
 * Client Request structure
 */
typedef struct RequestData {

        MatrixBaseType n1;                              // array dimension
        MatrixBaseType n2;                              // array dimension
        MatrixBaseType n3;                              // array dimension
        unsigned long aSize;                            // Matrix A array size
        unsigned long bSize;                            // Matrix B array size
        MatrixBaseType *A;                              // Matrix A Data
        MatrixBaseType *B;                              // Matrix B Data        

} RequestData;


/**
 * Server Response Data
 */
typedef struct ResponseData {

        long user;                                      // user time                            
        long wall;                                      // wall time
        long sys;                                       // system time
        int  err;                                       // deal with server error messages
        unsigned long cSize;                            // Matrix C array size
        MatrixBaseType *C;                              // Matrix C Data

} ResponseData;


#endif //ifndef _COMMON_H
