//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/kernel/Converters/CloneToNumpy.h"
#include "MantidPythonInterface/kernel/Converters/NDArrayTypeIndex.h"
#include "MantidKernel/WarningSuppressions.h"
#include <boost/python/list.hpp>

GCC_DIAG_OFF(cast-qual)
#define PY_ARRAY_UNIQUE_SYMBOL KERNEL_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>
GCC_DIAG_ON(cast-qual)

#include <string>

namespace Mantid { namespace PythonInterface
  {
  namespace Converters
  {
    namespace Impl
    {
      /**
       * Returns a new numpy array with the a copy of the data from 1D vector with the
       * exception of string elements where a Python list is produced
       * @param cvector :: A reference to the cvector to clone
       * @return The new cloned array
       */
      template<typename ElementType>
      PyObject *clone1D(const std::vector<ElementType> & cvector)
      {
        Py_intptr_t dims[1] = { static_cast<int>(cvector.size()) };
        return cloneND(cvector.data(), 1, dims);
      }

      /**
       * Specialisation for vector<bool> that stores the underlying data differently
       * Returns a new numpy array with the a copy of the data vector of booleans
       * @param cvector :: A reference to the cvector to clone
       * @return The new cloned array
       */
      template<>
      PyObject *clone1D(const std::vector<bool> & cvector)
      {
        Py_intptr_t dims[1] = { static_cast<int>(cvector.size()) };
        int datatype = NDArrayTypeIndex<bool>::typenum;
        PyArrayObject *nparray = (PyArrayObject*)
          PyArray_NewFromDescr(&PyArray_Type,
                               PyArray_DescrFromType(datatype),
                               1, // rank
                               dims, // Length in each dimension
                               NULL, NULL,
                               0, NULL);
        for(Py_intptr_t i = 0; i < dims[0]; ++i)
        {
          void *itemPtr = PyArray_GETPTR1(nparray, i);
          PyArray_SETITEM(nparray, (char*)itemPtr, PyBool_FromLong(static_cast<long int>(cvector[i])));
        }
        return (PyObject*)nparray;
      }

      /**
       * Returns a new numpy array with the a copy of the data from array. A specialization
       * exists for strings so that they simply create a standard python list.
       * @param carray :: A reference to a carray
       * @param ndims :: The dimensionality of the array
       * @param dims :: The length of the arrays in each dimension
       * @return
       */
      template<typename ElementType>
      PyObject *cloneND(const ElementType * carray, const int ndims, Py_intptr_t *dims)
      {
        int datatype = NDArrayTypeIndex<ElementType>::typenum;
        PyArrayObject *nparray = (PyArrayObject*)
          PyArray_NewFromDescr(&PyArray_Type,
                               PyArray_DescrFromType(datatype),
                               ndims, // rank
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
        const void *data = static_cast<void*>(const_cast<ElementType *>(carray));
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
      PyObject *cloneND(const std::string * carray, const int ndims, Py_intptr_t *dims)
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
      #define INSTANTIATE_CLONE1D(ElementType) \
        template DLLExport PyObject *clone1D<ElementType>(const std::vector<ElementType> & cvector); \

      #define INSTANTIATE_CLONEND(ElementType) \
        template DLLExport PyObject *cloneND<ElementType>(const ElementType *, const int ndims, Py_intptr_t *dims);

      #define INSTANTIATE_CLONE(ElementType) \
        INSTANTIATE_CLONE1D(ElementType) \
        INSTANTIATE_CLONEND(ElementType)

      ///@cond Doxygen doesn't seem to like this...
      INSTANTIATE_CLONE(int)
      INSTANTIATE_CLONE(long)
      INSTANTIATE_CLONE(long long)
      INSTANTIATE_CLONE(unsigned int)
      INSTANTIATE_CLONE(unsigned long)
      INSTANTIATE_CLONE(unsigned long long)
      INSTANTIATE_CLONE(double)
      INSTANTIATE_CLONE(float)
      // Need further 1D specialisation for string
      INSTANTIATE_CLONE1D(std::string)
      // Need further ND specialisation for bool
      INSTANTIATE_CLONEND(bool)
      ///@endcond
    }
  }
}}

