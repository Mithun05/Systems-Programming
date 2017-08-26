#include "abstract_matrix.h"
#include "dense_matrix.h"

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>

//TODO: Add types, data and functions as required.

static _Bool isInit = false;			// to initialize virtual table only once

/** The following struct represents DenseMatrix structure.
    It contains super class Matrix interface, number of rows,
    number of columns and type of elements in matrix.
    This structure uses the flexi-array representation. 	
    The "flexi-array" is an incomplete array type which contains no element size in it.
    It is an array of empty or zero length. C99 is designated it as "struct hack"
    if last type contains only one element however both are same.
    It allocates everything within the single bolck of memory.	
*/
typedef struct {
	DenseMatrix;						// super class interface
	int nRows;						// no of rows
	int nCols;						// no of cols
	int element[];						// flexi-array i.e Empty size array
} DenseMatrixImpl;						// Object(we can say now)


/** 
    This function returns the name of the class.
*/
static const char * getKlass(const Matrix *this, int *err)
{
	int nCols = this -> fns -> getNRows(this, err);		// get number of rows
	int nRows = this -> fns -> getNCols(this, err);		// get number of cols

	if(nCols <= 0 || nRows <= 0)				// matrix validity check
	{
		*err = EINVAL;					// set error code
		return NULL;
	}
	else
	{
		return "denseMatrix";				// get string literal
	}
}

/**
   This function returns the total number of rows in the dense matrix.	
*/
static int getNRows(const Matrix *this, int *err)
{
	const DenseMatrixImpl *denseMatrixImpl = (const DenseMatrixImpl *) this;  	 // cast to specific
	if(denseMatrixImpl -> nRows <= 0)						 // matrix validity check 
	{
		*err = EINVAL;								 // set error code
		return -1;
	}
	else
	{
		return denseMatrixImpl -> nRows;					  // get number of rows 
	}
}

/**
   This function returns the total number of columns in the dense matrix.
*/
static int getNCols(const Matrix *this, int *err)
{
	const DenseMatrixImpl *denseMatrixImpl = (const DenseMatrixImpl *) this;  // cast to specific
	if(denseMatrixImpl -> nCols <= 0)					  // matrix validity check	
	{
		*err = EINVAL;							  // set error code
		return -1;								
	}
	else
	{	
		return denseMatrixImpl -> nCols;			         // get number of cols
	}
}

/**
   This function returns the dense matrix specified element.
*/
static MatrixBaseType getElement(const Matrix *this, int rowIndex, int colIndex, int *err)
{
	const DenseMatrixImpl *denseMatrixImpl = (const DenseMatrixImpl *) this;  // cast to specific
	int nCols = getNCols(this, err);					  // get cols
	int nRows = getNRows(this, err);					  // get rows
	if(nCols <= 0 || nRows <= 0)						  // matrix validity check
	{
		*err = EINVAL;							  // set error code 
		return -1;
	}
	else
	{
		if(rowIndex < 0 || colIndex < 0 || rowIndex > nRows || colIndex > nCols)
		{
			*err = EDOM;								 // set error code 
			return -1;
		}
		else
		{
			return denseMatrixImpl -> element[rowIndex * nCols + colIndex];		  // get specified element
		}
	}
}

/**
  This function is used to set element into the dense matrix.
*/
static void setElement(Matrix *this, int rowIndex, int colIndex, MatrixBaseType Element, int *err)
{
	DenseMatrixImpl *denseMatrixImpl = (DenseMatrixImpl *) this;		// cast to specific
	int nCols = getNCols(this, err);					// get cols
	int nRows = getNRows(this, err);					// get rows
	if(nCols <= 0 || nRows <= 0)						// matrix validity check
	{
		*err = EINVAL;							// set error code
	}
	else
	{
		if(rowIndex < 0 || colIndex < 0 || rowIndex > nRows || colIndex > nCols)
		{
			*err = EDOM;						// set error code
		}
		else
		{
			denseMatrixImpl -> element[rowIndex * nCols + colIndex] = Element;	// set specified element
		}
	}
}

/** Initializing Function Pointers to design OOP concept in C language. 
    This is equivalent to virtual table in C++.
    The basic abstract interfaces to be able to use by sub-classes and its sub-classes 
    based on type of inheritance.       
*/
static DenseMatrixFns denseMatrixFns = {

	.getKlass   = getKlass,		// implemented above  - override
        .getNRows   = getNRows,		// implemented above  - override	
        .getNCols   = getNCols,		// implemented above  - override
 	.getElement = getElement,	// implemented above  - override
	.setElement = setElement	// implemented above  - override

};

/** Return a newly allocated matrix with all entries in consecutive
 *  memory locations (row-major layout).  All entries in the newly
 *  created matrix are initialized to 0.  Set *err to EINVAL if nRows
 *  or nCols <= 0, to ENOMEM if not enough memory.
 */
DenseMatrix *
newDenseMatrix(int nRows, int nCols, int *err)
{

	DenseMatrixImpl *denseMatrix = NULL;
	if(nRows <= 0 || nCols <= 0)	// check valid matrix indexes	
	{
		*err = EINVAL;		// set error code 
	} 
	else
	{
		/**
	   	  This memory allocation stores structure elements in a consecutive memory location.
	   	  All elements are being stored contiguously.
		*/
		denseMatrix = (DenseMatrixImpl *) malloc(sizeof(DenseMatrixImpl) + nRows * nCols * sizeof(int));	// dynamic memory allocation

		if(!denseMatrix)		// check for enough memory allocation
		{
			*err = ENOMEM;		// set error code
			 return NULL;
		}
		else
		{
			if(!isInit)								// check init bool variable	
			{
				const MatrixFns *fns = getAbstractMatrixFns();			// get super class
				denseMatrixFns.transpose = fns -> transpose;			// inherit super method transpose	
				denseMatrixFns.mul = fns -> mul;				// inherit super method mul
				denseMatrixFns.free = fns -> free;				// inherit super method free
				isInit = true;							// one instance to exit for entire program
			}	

			denseMatrix -> fns = (MatrixFns *) &denseMatrixFns;			// override virtual pointer by sub-class

			denseMatrix -> nRows = nRows;						// allocate memory for rows
			denseMatrix -> nCols = nCols;						// allocate memory for cols
	
			for(int counter = 0; counter < nRows * nCols; counter++)
			{
				denseMatrix -> element[counter] = counter;			// initialize to offset values
			}
		}
	}
	return (DenseMatrix *) denseMatrix;					// return new dense matrix	 	

}

/** Return implementation of functions for a dense matrix; these functions
 *  can be used by sub-classes to inherit behavior from this class.
 */
const DenseMatrixFns *
getDenseMatrixFns(void)
{
 	 return &denseMatrixFns;     // return address of virtual table to derive or inherit by the sub-classes					    
}
