#include "abstract_matrix.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>	//remove
//TODO: Add types, data and functions as required.

/** The function is used to free the resources that is being currently
    used by Matrix. If memory gets allocated dynamically using system calls
    library then this function frees the memory to be able to use by the other programs
    instead of causing memory leak.
*/
static void freeMatrix(Matrix *this, int *err)
{
	int nRows = this -> fns -> getNRows(this, err);		// get rows in matrix 
	int nCols = this -> fns -> getNCols(this, err);		// get cols in matrix
	
	if(nRows <= 0 || nCols <= 0)				// matrix validity check	
	{
		*err = EINVAL;					// set error code
	}
	else
	{
		free(this);					// standard system call			
	}
}


/** The function is used to transpose the given matrix.
    The transpose of the matrix is representation of the matrix 
    in which columns gets allocated into rows. It can be achieved
    by using swapping or just switching the co-ordinates which means rowIndex
    and colIndex.
    This function uses the switching co-ordinates indexes way.
    The Time Complexity of traspose matrix is O(n^2) in this method.  
*/
static void transpose(const Matrix *this, Matrix *result, int *err)
{
	int nRows = this -> fns -> getNRows(this, err); 	// get rows in matrix
	int nCols = this -> fns -> getNCols(this, err);   	// get cols in matrix

	if(nRows <= 0 || nCols <= 0) 				// matrix validity check
	{
		*err = EINVAL;					// set error code	
	}
	else
	{
		int nRowsT = result -> fns -> getNRows(result, err);	// get rows in matrix
		int nColsT = result -> fns -> getNCols(result, err);	// get cols in matrix

		if(nRowsT <= 0 || nColsT <= 0)			        // matrix validity check
		{
			*err = EINVAL;					// set error code
		}
		else
		{
			if(nRows != nColsT || nCols != nRowsT)			// not compatible dimensions
			{
				*err = EDOM;					// set error code
			}
			else
			{
				for(int row_counter = 0; row_counter < nRows; row_counter++)								// iterate for rows
				{	
					for(int col_counter = 0; col_counter < nCols; col_counter++)							// iterate for cols
					{
						MatrixBaseType matrixElement = this -> fns -> getElement(this, row_counter, col_counter, err); 		// get specified index element
						result -> fns -> setElement(result, col_counter, row_counter, matrixElement, err);			// set specified index element
					}
				}
			}
		}
	}
}	


/** The function is used to multiply two given matrices.
    The multiplication of two matrix is accessing elements row-wise from the first matrix
    and then getting column-wise elements from the second matrix to get each element
    in the result matrix. 
    This function uses the above approach.
    The Time Complexity of multiplication matrix is O(n^3) in this method.   	
*/
static void mul(const Matrix *this, const Matrix *multiplier, Matrix *product, int *err)
{
	int first_nRows = this -> fns -> getNRows(this, err);		// get rows in first matrix
	int first_nCols = this -> fns -> getNCols(this, err);		// get cols in first matrix
	
	if(first_nRows <= 0 || first_nCols <= 0)			// matrix validity check
	{
		*err = EINVAL;						// set error code
	}
	else 
	{
		int second_nRows = multiplier -> fns -> getNRows(multiplier, err);	// get rows in second matrix
		int second_nCols = multiplier -> fns -> getNCols(multiplier, err);	// get cols in second matrix

		if(second_nRows <= 0 || second_nCols <= 0)			// matrix validity check
		{
			*err = EINVAL;					 	// set error code
		}
		else 
		{
			int product_nRows = product -> fns -> getNRows(product, err);
			int product_nCols = product -> fns -> getNCols(product, err);

			if(product_nRows <= 0 || product_nCols <= 0)			// matrix validity check
			{
				*err = EINVAL;						// set error code
			}
			else 
			{
				MatrixBaseType result = 0;

				if(first_nCols != second_nRows || first_nRows != product_nRows || second_nCols != product_nCols)						// to multiply first cols needs to be equal to second rows
				{
					*err = EDOM;							// set error if invalid matrix to multiply
				}
				else
				{
				/**
		  		  Naive approach is to multiply matrices used in this method. Generally, there is 
		  		  going to be a lot of cache misses happen in this case as it is not always the case
		  		  where the element is present in the cache so number of page faults happen by this method.
		  		  Generally, to avoid cache there is an alternative implementation used in Smart Matrix Mul
		  		  algorithm which is being overrided with different approach.
				*/
					for(int first_counter = 0; first_counter < first_nRows; first_counter++)				// iterate over first rows
					{
						for(int second_counter = 0; second_counter < second_nCols; second_counter++)			// iterate over second cols
						{
							for(int third_counter = 0; third_counter < second_nRows; third_counter++)		// iterate over second rows
							{
								MatrixBaseType firstElement  = this -> fns -> getElement(this, first_counter, third_counter, err);	
								MatrixBaseType secondElement = multiplier -> fns -> getElement(multiplier, third_counter, second_counter, err);
								result += (firstElement * secondElement);
							}
				
							product -> fns -> setElement(product, first_counter, second_counter, result, err);	// set resultant element
							result = 0;										// reset result
						}
					}	

				}
			}
		}
	}
}

/** Initializing Function Pointers to design OOP concept in C language. 
    This is equivalent to virtual table in C++.
    The basic abstract interfaces to be able to use by sub-classes and its sub-classes 
    based on type of inheritance.	
*/
static MatrixFns matrixFns = {

	.free = freeMatrix,		// implemented above
	.transpose = transpose, 	// implemented above
	.mul = mul			// implemented above
};

/** Return implementation of functions for an abstract matrix; these are
 *  functions which can be implemented using only other matrix functions,
 *  independent of the actual implementation of the matrix.
 */
const MatrixFns *
getAbstractMatrixFns(void)
{
  	return &matrixFns;		// return address of virtual table to derive or inherit by the sub-classes
}
