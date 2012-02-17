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
       * Wraps a vector in a numpy array structure without copying the data
       * @param cvector :: A reference to the std::vector to wrap
       * @param mode :: A mode switch to define whether the final array is read only/read-write
       * @return A pointer to a numpy ndarray object
       */
      template<typename VectorType>
      PyObject *wrapWithNDArray(const VectorType & cvector, const WrapMode mode)
      {
        npy_intp dims[1] = { cvector.size() };
        int datatype = NDArrayTypeIndex<typename VectorType::value_type>::typenum;
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
      template<typename VectorType>
      PyObject *cloneToNDArray(const VectorType & cvector)
      {
        npy_intp dims[1] = { cvector.size() };
        int datatype = NDArrayTypeIndex<typename VectorType::value_type>::typenum;
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
      #define INSTANTIATE(VectorType) \
        template DLLExport PyObject * wrapWithNDArray<VectorType>(const VectorType &, const WrapMode);\
        template DLLExport PyObject * cloneToNDArray<VectorType>(const VectorType &);

      INSTANTIATE(std::vector<int16_t>);
      INSTANTIATE(std::vector<uint16_t>);
      INSTANTIATE(std::vector<int32_t>);
      INSTANTIATE(std::vector<uint32_t>);
      INSTANTIATE(std::vector<int64_t>);
      INSTANTIATE(std::vector<uint64_t>);
#ifdef __APPLE__
      INSTANTIATE(std::vector<unsigned long>);
#endif
      INSTANTIATE(std::vector<double>);
    }
  }
}}

