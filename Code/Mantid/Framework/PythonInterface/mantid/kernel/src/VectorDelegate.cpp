//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/kernel/VectorDelegate.h"
#include <boost/python/extract.hpp>

// See http://docs.scipy.org/doc/numpy/reference/c-api.array.html#PY_ARRAY_UNIQUE_SYMBOL
#define PY_ARRAY_UNIQUE_SYMBOL KERNEL_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/ndarrayobject.h>

#include <iostream>

namespace Mantid
{
  namespace PythonInterface
  {
    namespace VectorDelegate
    {
      /**
       *  Check that the list contains items of the required type for the C++ type
       *  @param value :: A pointer to a python list object
       *  @return ""if the list can be converted to a vector, otherwise it returns an error string
       */
      std::string isSequenceType(PyObject *value)
      {
        std::string error("");
        if( !PySequence_Check(value) )
        {
          error = std::string("Cannot convert ") + value->ob_type->tp_name + " object to a std::vector.";
        }
        return error;
      }

      /**
       * Convert a python object into a std::vector
       * @param value :: A pointer to a python list
       * @return A new vector created from the python values (i.e. a copy)
       */
      template<typename ElementType>
      const std::vector<ElementType> toStdVector(PyObject *value)
      {
        const std::string msg = isSequenceType(value);
        if( !msg.empty() )
        {
          throw std::invalid_argument(msg);
        }
        Py_ssize_t len = PySequence_Size(value);
        std::vector<ElementType> result((size_t)len);
        for( Py_ssize_t i = 0; i < len; ++i )
        {
          PyObject *item = PySequence_Fast_GET_ITEM(value, i);
          ElementType element = boost::python::extract<ElementType>(item)();
          result[i]= element;
        }
        return result;
      }

      /**
       * Convert a python array object into a std::vector
       * @param arr :: A pointer to a numpy array
       * @return A new vector created from the python values (i.e. a copy)
       */
      template<typename ElementType>
      const std::vector<ElementType> toStdVectorFromNumpy(PyArrayObject *arr)
      {
        npy_intp length = PyArray_SIZE(arr);
        std::vector<ElementType> result(length);
        if(length == 0)
        {
          return result;
        }

        /** Use the iterator API to iterate through the array
         * and assign each value to the corresponding vector
         */
        PyObject *iter = PyArray_IterNew((PyObject*)arr);
        size_t index(0);
        do
        {
          void *item = PyArray_ITER_DATA(iter);
          ElementType *v = (ElementType*)item;
          result[index] = *v;
          ++index;
          PyArray_ITER_NEXT(iter);
        }
        while(PyArray_ITER_NOTDONE(iter));
        return result;
      }

      //--------------------------------------------------------------------------
      // Concrete Instantiations
      //--------------------------------------------------------------------------
      template DLLExport const std::vector<int> toStdVector<int>(PyObject *value);
      template DLLExport const std::vector<size_t> toStdVector<size_t>(PyObject *value);
      template DLLExport const std::vector<double> toStdVector<double>(PyObject *value);

      template DLLExport const std::vector<double> toStdVectorFromNumpy<double>(PyArrayObject *value);
    }
  }
}

