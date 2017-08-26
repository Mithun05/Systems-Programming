#ifndef _MATRIX_MUL_H
#define _MATRIX_MUL_H

#include <stdio.h>

/** The type of each matrix entry */
typedef int MatrixBaseType;

/** Use this macro to handle MatrixBaseType in printf(), scanf() routines */
#define MATRIX_BASE_TYPE_FMT "%d"

//opaque type to be defined by implementation.
typedef struct MatrixMul MatrixMul;

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
 *  index of the column in the multiplier and Z is their dot-product
 *  computed by child process PID.  The spacing must be exactly as
 *  shown above and all values must be output in decimal with no
 *  leading zeros or redundant + signs.
 *
 */
MatrixMul *newMatrixMul(int nWorkers, FILE *trace, int *err);

/** Free all resources used by matMul.  Specifically, free all memory
 *  and return only after all child processes have been set up to
 *  exit.  Set *err appropriately (as documented in errno(3)) on error.
 */
void freeMatrixMul(MatrixMul *matMul, int *err);

/* Input matrices should be declared const MatrixBaseType[][], but
 * doing so results in warnings on gcc 4.9.2-10.  Hence this macro.
 */
#if __GNUC__ == 4 && __GNUC_MINOR__ == 9 && __GNUC_PATCHLEVEL__ == 2
#define CONST
#else
#define CONST const
#endif

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
void mulMatrixMul(const MatrixMul *matMul, int n1, int n2, int n3,
                  CONST MatrixBaseType a[n1][n2],
                  CONST MatrixBaseType b[n2][n3],
                  MatrixBaseType c[n1][n3], int *err);


#endif //ifndef _MATRIX_MUL_H
