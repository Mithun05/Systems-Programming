#include "abstract_matrix.h"			// getting implicit declaration warning so added
#include "dense_matrix.h"
#include "smart_mul_matrix.h"

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>				// getting implicit declaration warning so added
#include <stdio.h>	//remove
#include <limits.h>     // remove
//TODO: Add types, data and functions as required.

static _Bool isInit = false;                    // to initialize virtual table only once


/** The following struct represents SmartMulMatrix structure.
    It contains super class Matrix interface, number of rows,
    number of columns and type of elements in matrix.
    This structure uses the flexi-array representation.         
    The "flexi-array" is an incomplete array type which contains no element size in it.
    It is an array of empty or zero length. C99 is designated it as "struct hack"
    if last type contains only one element however both are same.
    It allocates everything within the single bolck of memory.  
*/
typedef struct {
        SmartMulMatrix;			// super class interface
        int nRows;			// number of rows
        int nCols;			// number of cols
        int element[];			// flexi-array i.e empty size array
} SmartMulMatrixImpl;			// Object (we can say now)

/** 
    This function returns the name of the class.
*/
static const char * getKlass(const Matrix *this,  int *err)
{
	int nRows = this -> fns -> getNRows(this, err);		// get rows
	int nCols = this -> fns -> getNCols(this, err);		// get cols
	if(nRows <= 0  || nCols <= 0)				// matrix validity check
	{
		*err = EINVAL;				        // set error code
		return NULL;
	}
	else
	{
        	return "smartMulMatrix";			// return class name
	}
}

/**
   This function returns the total number of rows in the smart mul  matrix.  
*/
static int getNRows(const Matrix *this, int *err)
{
        const SmartMulMatrixImpl *smartMulMatrixImpl = (const SmartMulMatrixImpl *) this;		// cast to specific
	if(smartMulMatrixImpl -> nRows <= 0)								// validity check					
	{
		*err = EINVAL;										// set error code
		return -1;
	}
	else
	{
        	return smartMulMatrixImpl -> nRows;							// get rows
	}
}

/**
   This function returns the total number of columns in the smart mul matrix.
*/
static int getNCols(const Matrix *this, int *err)
{
        const SmartMulMatrixImpl *smartMulMatrixImpl = (const SmartMulMatrixImpl *) this;		// cast to specific
	if(smartMulMatrixImpl -> nCols <= 0)								// get cols
	{
		*err = EINVAL;										// set error code
		return -1;
	}
	else
	{
        	return smartMulMatrixImpl -> nCols;							// get cols
	}
}

/**
   This function returns the smart mul matrix specified element.
*/
static MatrixBaseType getElement(const Matrix *this, int rowIndex, int colIndex, int *err)
{
        const SmartMulMatrixImpl *smartMulMatrixImpl = (const SmartMulMatrixImpl *) this;		// cast to specific
	int nCols = getNCols(this, err);								// get cols	
	int nRows = getNRows(this, err);								// get rows
        if(nCols <= 0 || nRows <= 0)									// matrix validity check
        {
                *err = EINVAL;										// set error code				
		return -1;
        }
	else
	{
		if(rowIndex < 0 || colIndex < 0 || rowIndex > nRows || colIndex > nCols)		// matrix validity check
                {
                        *err = EDOM;									// set error code
			return -1;
                }
		else
		{
        		return smartMulMatrixImpl -> element[rowIndex * nCols + colIndex];		// get specified element
		}
	}
}

/**
  This function is used to set element into the smart mul matrix.
*/
static void setElement(Matrix *this, int rowIndex, int colIndex, MatrixBaseType Element, int *err)
{
        SmartMulMatrixImpl *smartMulMatrixImpl = (SmartMulMatrixImpl *) this;				// cast to specific
        int nCols = getNCols(this, err);								// get cols	
	int nRows = getNRows(this, err);								// get rows
        if(nCols <= 0 || nRows <= 0)									// matrix validity check	
        {
                *err = EINVAL;										// set error code
        }
	else
	{
		if(rowIndex < 0 || colIndex < 0 || rowIndex > nRows || colIndex > nCols)		// matrix validity check
                {
                        *err = EDOM;									// set error code		
                }
		else
		{
        		smartMulMatrixImpl -> element[rowIndex * nCols + colIndex] = Element;				// set specified element
		}
	}
}

/** The function is used to multiply two given matrices.
    The multiplication of two matrix is accessing elements row-wise from the first matrix
    and then getting column-wise elements from the second matrix to get each element
    in the result matrix. 
    This function uses the above approach however it uses the transpose of the multiplier.
    The Time Complexity of following multiplication matrix is O(n^3) + O(n^2) in this method.      
    Goal : By taking the transpose of the multiplier, there is going to be minimized
    page faults through which elements are being accessed sequentially.
    This approach improve the cache performance more significantly.	
*/
static void mul(const Matrix *this, const Matrix *multiplier, Matrix *product, int *err)
{
	int first_nRows = this -> fns -> getNRows(this, err);          // get rows in first matrix
	int first_nCols = this -> fns -> getNCols(this, err);	       // get cols in first matrix	

	if(first_nCols <= 0 || first_nRows <= 0)
	{
		*err = EINVAL;
	}
	else 
	{
        	int second_nRows = multiplier -> fns -> getNRows(multiplier, err);     // get rows in second matrix
	        int second_nCols = multiplier -> fns -> getNCols(multiplier, err);     // get cols in second matrix

		if(second_nRows <= 0 || second_nCols <= 0)
		{
			*err = EINVAL;						      // set error code	
		}
		else 
		{
			int product_nRows = product -> fns -> getNRows(product, err);
        		int product_nCols = product -> fns -> getNCols(product, err);

        		if(product_nRows <= 0 || product_nCols <= 0)
        		{
                		*err = EINVAL;                                          // set error code
        		}
			else 
			{
				if(first_nCols != second_nRows)				       // check for matrix validation
				{
					*err = EDOM;						// set error code for invalid matrix
				}
				else
				{
					int multiplierTranspose[second_nCols][second_nRows];	// temporary array to store transpose matrix element

					/**
		  			The following code takes the transpose of a multiplier matrix.
	          			The time complexity to perform transpose of a matrix is O(n^2).		
					*/
					for (int first_t_counter = 0; first_t_counter < second_nRows; first_t_counter++) 
					{
						for (int second_t_counter = 0; second_t_counter < second_nCols; second_t_counter++)
      						{  
							MatrixBaseType multiplierElement = multiplier -> fns -> getElement(multiplier, first_t_counter, second_t_counter, err);		// get multiplier element
							multiplierTranspose[second_t_counter][first_t_counter] =  multiplierElement;		// set an element as a transposed in tranpose matrix 
						}
					}

					/**
		  			  Naive matrix multiplication with a little trick to improve cache optimization
		  			  The time complexity of the method is O(n^3).
					*/
        				for(int first_counter = 0; first_counter < first_nRows; first_counter++)                         	// iterate over first rows
        				{
           					for(int second_counter = 0; second_counter < second_nCols; second_counter++)                    // iterate over second cols
               					{
				  			MatrixBaseType result = 0;	
                		  			for(int third_counter = 0; third_counter < second_nRows; third_counter++)             // iterate over second rows
                      		  			{
                           					MatrixBaseType firstElement  = this -> fns -> getElement(this, first_counter, third_counter, err);
                           					result += (firstElement * multiplierTranspose[second_counter][third_counter]);
                      		  			}

                           	  				product -> fns -> setElement(product, first_counter, second_counter, result, err);     // set resultant element
                           	  			result = 0;                                                                            // reset result
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
static SmartMulMatrixFns smartMulMatrixFns = {

        .getKlass = getKlass,			// implemented above - override
        .getNRows = getNRows,			// implemented above - override	
        .getNCols = getNCols,			// implemented above - override
        .getElement = getElement,		// implemented above - override
        .setElement = setElement,		// implemented above - override
        .mul = mul				// implemented above - override

};

/** Return a newly allocated matrix with all entries in consecutive
 *  memory locations (row-major layout).  All entries in the newly
 *  created matrix are initialized to 0.  The return'd matrix uses
 *  a smart multiplication algorithm to avoid caching issues;
 *  specifically, transpose the multiplier and use a modified
 *  multiplication algorithm with the transposed multiplier.
 *
 *  Set *err to EINVAL if nRows or nCols <= 0, to ENOMEM if not enough
 *  memory.
 */
SmartMulMatrix *newSmartMulMatrix(int nRows, int nCols, int *err)
{

	SmartMulMatrixImpl *smartMulMatrix = NULL;
	if(nRows <=0 || nCols <=0)      // check valid matrix indexes   
        {
                *err = EINVAL;          // set error code 
		 return NULL;
        }
	else
	{

        	/**
           	  This memory allocation stores structure elements in a consecutive memory location.
           	  All elements are being stored contiguously.
        	*/
        	smartMulMatrix = (SmartMulMatrixImpl *) malloc(sizeof(SmartMulMatrixImpl) + nRows * nCols * sizeof(int));       // dynamic memory allocation

        	if(!smartMulMatrix)                // check for enough memory allocation
        	{
                	*err = ENOMEM;             // set error code
			return NULL;
        	}
		else
		{
        		if(!isInit)                                                             // check init bool variable     
        		{
				const DenseMatrixFns *fns = getDenseMatrixFns();		// get super class
                		smartMulMatrixFns.transpose = fns -> transpose;                 // inherit super method transpose       
                		smartMulMatrixFns.free = fns -> free;                           // inherit super method free
               			isInit = true;                                                  // one instance to exit for entire program
        		}

        		smartMulMatrix -> fns = (MatrixFns *) &smartMulMatrixFns;               // override virtual pointer by sub-class

	        	smartMulMatrix -> nRows = nRows;                                        // allocate memory for rows
        		smartMulMatrix -> nCols = nCols;                                        // allocate memory for cols

	        	for(int counter = 0; counter < nRows * nCols; counter++)
        		{
                		smartMulMatrix -> element[counter] = counter;                   // initialize to offset values
        		}
		}
	}

        return (SmartMulMatrix *) smartMulMatrix;                               // return new smartmul matrix               
}

/** Return implementation of functions for a smart multiplication
 *  matrix; these functions can be used by sub-classes to inherit
 *  behavior from this class.
 */
const SmartMulMatrixFns *
getSmartMulMatrixFns(void)
{
  	return &smartMulMatrixFns;		// return address of virtual table to derive or inherit by the sub-classes
}
