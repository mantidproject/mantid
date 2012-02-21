//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/kernel/Converters/VectorToNDArray.h"
#include "MantidPythonInterface/kernel/Converters/NDArrayTypeIndex.h"

#include <boost/python/detail/prefix.hpp> // Safe include of Python.h
#define PY_ARRAY_UNIQUE_SYMBOL KERNEL_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>

namespace Mantid { namespace PythonInterface
  {
  namespace Converters
  {
    namespace Impl
    {
      /**
       * Defines the wrapWithNDArray specialization for vector container types
       *
       * Wraps a vector in a numpy array structure without copying the data
       * @param cvector :: A reference to the std::vector to wrap
       * @param mode :: A mode switch to define whether the final array is read only/read-write
       * @return A pointer to a numpy ndarray object
       */
      template<typename ContainerType>
      PyObject *wrapWithNDArray(const ContainerType & cvector, const NumpyWrapMode mode)
      {
        npy_intp dims[1] = { cvector.size() };
        int datatype = NDArrayTypeIndex<typename ContainerType::value_type>::typenum;
        PyObject *nparray = PyArray_SimpleNewFromData(1, dims, datatype,(void*)&(cvector[0]));
        if( mode == ReadOnly )
        {
          PyArrayObject * np = (PyArrayObject *)nparray;
          np->flags &= ~NPY_WRITEABLE;
        }
        return nparray;
      }

      /**
       * Returns a new numpy array with the a copy of the data from cvector
       * @param cvector :: A reference to a std::vector
       * @return
       */
      template<typename ContainerType>
      PyObject *cloneToNDArray(const ContainerType & cvector)
      {
        npy_intp dims[1] = { cvector.size() };
        int datatype = NDArrayTypeIndex<typename ContainerType::value_type>::typenum;
        PyArrayObject *nparray = (PyArrayObject *)PyArray_NewFromDescr(&PyArray_Type,
            PyArray_DescrFromType(datatype),
            1, // rank 1
            dims, // Length in each dimension
            NULL, NULL,
            0, NULL);
        double *dest = (double*)PyArray_DATA(nparray); // HEAD of the contiguous numpy data array
        std::copy(cvector.begin(), cvector.end(), dest);
        return (PyObject *)nparray;
      }

      //-----------------------------------------------------------------------
      // Explicit instantiations
      //-----------------------------------------------------------------------
      #define INSTANTIATE(ElementType) \
        template DLLExport PyObject * wrapWithNDArray<std::vector<ElementType> >(const std::vector<ElementType> &, const NumpyWrapMode);\
        template DLLExport PyObject * cloneToNDArray<std::vector<ElementType> >(const std::vector<ElementType> &);

      INSTANTIATE(int16_t);
      INSTANTIATE(uint16_t);
      INSTANTIATE(int32_t);
      INSTANTIATE(uint32_t);
      INSTANTIATE(int64_t);
      INSTANTIATE(uint64_t);
#ifdef __APPLE__
      INSTANTIATE(unsigned long);
#endif
      INSTANTIATE(double);

    }
  }
}}

