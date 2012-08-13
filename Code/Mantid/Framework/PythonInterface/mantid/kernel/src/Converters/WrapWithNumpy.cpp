//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/kernel/Converters/NumpyWrapMode.h"
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
        PyObject *nparray = PyArray_SimpleNewFromData(ndims, dims, datatype,(void*)carray);
        if( mode == ReadOnly )
        {
          PyArrayObject * np = (PyArrayObject *)nparray;
          np->flags &= ~NPY_WRITEABLE;
        }
        return nparray;
      }

      //-----------------------------------------------------------------------
      // Explicit instantiations
      //-----------------------------------------------------------------------
      #define INSTANTIATE(ElementType) \
        template DLLExport PyObject *wrapWithNDArray<ElementType>(const ElementType*, const int ndims, Py_intptr_t *dims, const NumpyWrapMode);

      INSTANTIATE(int);
      INSTANTIATE(long);
      INSTANTIATE(long long);
      INSTANTIATE(unsigned int);
      INSTANTIATE(unsigned long);
      INSTANTIATE(unsigned long long);
      INSTANTIATE(double);
      INSTANTIATE(float);
    }
  }
}}

