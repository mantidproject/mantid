//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/kernel/Converters/VectorToNDArray.h"
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
       * Returns a new numpy array with the a copy of the data from cvector. A specialization
       * exists for strings so that they simply create a standard python list.
       * @param cvector :: A reference to a std::vector
       * @return
       */
      template<typename ContainerType>
      PyObject *cloneToNDArray(const ContainerType & cvector)
      {
        npy_intp dims[1] = { cvector.size() };
        int datatype = NDArrayTypeIndex<typename ContainerType::value_type>::typenum;
        PyObject *nparray =
          PyArray_NewFromDescr(&PyArray_Type,
                               PyArray_DescrFromType(datatype),
                               1, // rank 1
                               dims, // Length in each dimension
                               NULL, NULL,
                               0, NULL);

        void *arrayData = PyArray_DATA(nparray);
        const void *data = cvector.data();
        std::memcpy(arrayData, data, PyArray_ITEMSIZE(nparray) * dims[0]);
        return (PyObject*)nparray;
      }

      /**
       * Returns a new python list of strings from the given vector
       * exists for strings so that they simply create a standard python list.
       * @param cvector :: A reference to a std::vector
       * @return
       */
      template<>
      PyObject *cloneToNDArray(const std::vector<std::string> & cvector)
      {
        boost::python::list pystrs;
        for(auto iter = cvector.begin(); iter != cvector.end(); ++iter)
        {
          pystrs.append(iter->c_str());
        }
        PyObject *rawptr = pystrs.ptr();
        Py_INCREF(rawptr); // Make sure it survies after the wrapper decrefs the count
        return rawptr;
      }

      //-----------------------------------------------------------------------
      // Explicit instantiations
      //-----------------------------------------------------------------------
      #define INSTANTIATE(ElementType) \
        template DLLExport PyObject * wrapWithNDArray<std::vector<ElementType> >(const std::vector<ElementType> &, const NumpyWrapMode);\
        template DLLExport PyObject * cloneToNDArray<std::vector<ElementType> >(const std::vector<ElementType> &);

      INSTANTIATE(int);
      INSTANTIATE(long);
      INSTANTIATE(long long);
      INSTANTIATE(unsigned int);
      INSTANTIATE(unsigned long);
      INSTANTIATE(unsigned long long);
      INSTANTIATE(double);
      INSTANTIATE(std::string);
    }
  }
}}

