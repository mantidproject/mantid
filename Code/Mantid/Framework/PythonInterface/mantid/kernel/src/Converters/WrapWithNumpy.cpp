//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/kernel/Converters/WrapWithNumpy.h"
#include "MantidPythonInterface/kernel/Converters/NDArrayTypeIndex.h"

#include <boost/python/list.hpp>
#define PY_ARRAY_UNIQUE_SYMBOL KERNEL_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>

#include <string>

namespace Mantid { namespace PythonInterface
  {
  namespace Converters
  {
    namespace Impl
    {
      namespace
      {
        /**
         * Flip the writable flag to ensure the array is read only
         * Numpy v1.7 removed access to the flags fields directly
         * and introduced the PyClear_Flags function.
         * @param arr A pointer to a numpy array
         */
        void markReadOnly(PyArrayObject *arr)
        {
          #if NPY_API_VERSION >= 0x00000007 //(1.7)
	    PyArray_CLEARFLAGS(arr, NPY_ARRAY_WRITEABLE);
          #else
            arr->flags &= ~NPY_WRITEABLE;
          #endif  
        }
      }

      /**
       * Defines the wrapWithNDArray specialization for C array types
       *
       * Wraps an array in a numpy array structure without copying the data
       * @param carray :: A pointer to the HEAD of the array
       * @param ndims :: The dimensionality of the array
       * @param dims :: The length of the arrays in each dimension
       * @param mode :: A mode switch to define whether the final array is read only/read-write
       * @return A pointer to a numpy ndarray object
       */
      template<typename ElementType>
      PyObject *wrapWithNDArray(const ElementType * carray, const int ndims,
                                Py_intptr_t *dims, const NumpyWrapMode mode)
      {
        int datatype = NDArrayTypeIndex<ElementType>::typenum;
        PyArrayObject *nparray = (PyArrayObject*)
	  PyArray_SimpleNewFromData(ndims, dims, datatype,
				    static_cast<void*>(const_cast<ElementType *>(carray)));

        if( mode == ReadOnly ) markReadOnly(nparray);
        return (PyObject*)nparray;
      }

      //-----------------------------------------------------------------------
      // Explicit instantiations
      //-----------------------------------------------------------------------
      #define INSTANTIATE_WRAPNUMPY(ElementType) \
        template DLLExport PyObject *wrapWithNDArray<ElementType>(const ElementType*, const int ndims, Py_intptr_t *dims, const NumpyWrapMode);

      ///@cond Doxygen doesn't seem to like this...
      INSTANTIATE_WRAPNUMPY(int)
      INSTANTIATE_WRAPNUMPY(long)
      INSTANTIATE_WRAPNUMPY(long long)
      INSTANTIATE_WRAPNUMPY(unsigned int)
      INSTANTIATE_WRAPNUMPY(unsigned long)
      INSTANTIATE_WRAPNUMPY(unsigned long long)
      INSTANTIATE_WRAPNUMPY(double)
      INSTANTIATE_WRAPNUMPY(float)
      ///@endcond
    }
  }
}}

