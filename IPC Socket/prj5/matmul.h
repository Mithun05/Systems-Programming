#ifndef _MAT_MUL_H
#define _MAT_MUL_H

/** Declarations for client */

#include "mat_base.h"

#include <stdbool.h>
#include <stdio.h>

//opaque type to be defined by implementation.
typedef struct MatrixMul MatrixMul;

/** Return an interface to the client end of a client-server matrix
 *  multiplier.
 *  
 *
 *  Set *err to an appropriate error number (documented in errno(3))
 *  on error.
 *
 */
MatrixMul *newMatrixMul(const char *hostName, const char *portNo, FILE *trace, int *err);

/** Free all resources used by matMul.  Specifically, free all memory
 *  used by matMul, Set *err appropriately (as documented in errno(3))
 *  on error.
 */
void freeMatrixMul(MatrixMul *matMul, int *err);

/** Set matrix c[n1][n3] to a[n1][n2] * b[n2][n3].  It is assumed that
 *  the caller has allocated c[][] appropriately.  Set *err to an
 *  appropriate error number (documented in errno(3)) on error.  If
 *  *err is returned as non-zero, then the matMul object may no longer
 *  be valid and future calls to mulMatrixMul() may have unpredictable
 *  behavior.  It is the responsibility of the caller to call
 *  freeMatrixMul() after an error.
 *
 *  The multiplication must be performed entirely on the server.The 
 *  Client Server communication should be performed using Socket TCP/
 *  IP protocol.
 */
void mulMatrixMul(const MatrixMul *matMul, int n1, int n2, int n3,
                  CONST MatrixBaseType a[n1][n2],
                  CONST MatrixBaseType b[n2][n3],
                  MatrixBaseType c[n1][n3], int *err);


#endif //ifndef _MAT_MUL_H
