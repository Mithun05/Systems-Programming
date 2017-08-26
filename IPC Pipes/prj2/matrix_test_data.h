#ifndef _MATRIX_TEST_DATA_H
#define _MATRIX_TEXT_DATA_H

#include "matrix_mul.h"

/** struct to allow defining test matrices */
typedef struct TestData {
  const char *desc;
  int nRows, nCols;
  MatrixBaseType *data;      //pointer to matrix data
  struct TestData *next;
} TestData;

/** Append list of new test data from fileName to *link.  Will
 *  terminate program on most errors.
 */
void newTestData(const char *fileName, TestData **link);

/** Free previously created testData */
void freeTestData(TestData *testData);


#endif //ifndef _MATRIX_TEST_DATA_H
