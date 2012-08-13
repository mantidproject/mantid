//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
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
       * Returns a new numpy array with the a copy of the data from array. A specialization
       * exists for strings so that they simply create a standard python list.
       * @param carray :: A reference to a carray
       * @param ndims :: The dimensionality of the array
       * @param dims :: The length of the arrays in each dimension
       * @return
       */
      template<typename ElementType>
      PyObject *cloneToNDArray(const ElementType * carray, const int ndims, Py_intptr_t *dims)
      {
        int datatype = NDArrayTypeIndex<ElementType>::typenum;
        PyObject *nparray =
          PyArray_NewFromDescr(&PyArray_Type,
                               PyArray_DescrFromType(datatype),
                               ndims, // rank 1
                               dims, // Length in each dimension
                               NULL, NULL,
                               0, NULL);
        // Compute total number of elements
        size_t length(dims[0]);
        if(ndims > 1)
        {
          for(int i = 1; i < ndims; ++i)
          {
            length *= dims[i];
          }
        }
        void *arrayData = PyArray_DATA(nparray);
        const void *data = (void*)carray;
        std::memcpy(arrayData, data, PyArray_ITEMSIZE(nparray) * length);
        return (PyObject*)nparray;
      }

      /**
       * Returns a new python list of strings from the given array of strings.
       * @param carray :: A reference to a std::vector
       * @param ndims :: The dimensionality of the array
       * @param dims :: The length of the arrays in each dimension
       * @return
       */
      template<>
      PyObject *cloneToNDArray(const std::string * carray, const int ndims, Py_intptr_t *dims)
      {
        boost::python::list pystrs;
        const std::string *iter = carray;
        for(int i = 0; i < ndims; ++i)
        {
          const Py_intptr_t length = dims[i];
          for(Py_intptr_t j = 0; j < length; ++j)
          {
            pystrs.append(iter->c_str());
            ++iter;
          }
        }
        PyObject *rawptr = pystrs.ptr();
        Py_INCREF(rawptr); // Make sure it survives after the wrapper decrefs the count
        return rawptr;
      }

      //-----------------------------------------------------------------------
      // Explicit instantiations
      //-----------------------------------------------------------------------
      #define INSTANTIATE(ElementType) \
        template DLLExport PyObject *cloneToNDArray<ElementType>(const ElementType *, const int ndims, Py_intptr_t *dims);

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

