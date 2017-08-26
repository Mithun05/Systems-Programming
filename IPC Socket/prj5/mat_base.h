#ifndef _MAT_BASE_H
#define _MAT_BASE_H

/** Common matrix declarations needed by modules, client and server. */

/** The type of each matrix entry */
typedef int MatrixBaseType;

/** Use this macro to handle MatrixBaseType in printf(), scanf() routines */
#define MATRIX_BASE_TYPE_FMT "%d"

/* Input matrices should be declared const MatrixBaseType[][], but
 * doing so results in warnings on gcc 4.9.2-10.  Hence this macro.
 */
#if __GNUC__ == 4 && __GNUC_MINOR__ == 9 && __GNUC_PATCHLEVEL__ == 2
#define CONST
#else
 #define CONST const
#endif

#endif //ifndef _MAT_BASE_H
