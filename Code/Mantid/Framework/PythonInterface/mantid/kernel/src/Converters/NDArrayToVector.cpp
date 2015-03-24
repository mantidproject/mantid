//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/kernel/Converters/NDArrayToVector.h"
#include "MantidPythonInterface/kernel/Converters/NDArrayTypeIndex.h"
#include <boost/python/extract.hpp>

// See http://docs.scipy.org/doc/numpy/reference/c-api.array.html#PY_ARRAY_UNIQUE_SYMBOL
//#define PY_ARRAY_UNIQUE_SYMBOL KERNEL_ARRAY_API
//#define NO_IMPORT_ARRAY
//#include <numpy/arrayobject.h>

#include "MantidPythonInterface/kernel/Converters/NumpyFunctions.h"
/*namespace{
PyObject* function_PyArray_IterNew(PyArrayObject *arr)
{
  return PyArray_IterNew((PyObject *)arr);
}
}*/

namespace Mantid
{
  namespace PythonInterface
  {
    namespace
    {
      //-------------------------------------------------------------------------
      // Template helpers
      //-------------------------------------------------------------------------
      /**
       * Templated structure to fill a std::vector
       * with values from a numpy array
       */
      template <typename DestElementType>
      struct fill_vector
      {
        void operator()(PyArrayObject *arr, std::vector<DestElementType> & cvector)
        {
          // Use the iterator API to iterate through the array
          // and assign each value to the corresponding vector
          typedef union {
            DestElementType* output;
            void *input;
          } npy_union;
          npy_union data;
          PyObject *iter = Converters::Impl::func_PyArray_IterNew(arr);
          npy_intp index(0);
          do
          {
            data.input = PyArray_ITER_DATA(iter);
            cvector[index] = *data.output;
            ++index;
            PyArray_ITER_NEXT(iter);
          } while (PyArray_ITER_NOTDONE(iter));
        }
      };

      /**
       * Specialization for a std::string to convert the values directly
       * to the string representation
       */
      template <>
      struct fill_vector<std::string>
      {
        void operator()(PyArrayObject *arr, std::vector<std::string> & cvector)
        {
          using namespace boost::python;
          PyObject *flattened = PyArray_Ravel(arr, NPY_CORDER);
          object nparray = object(handle<>(flattened));
          const Py_ssize_t nelements = PyArray_Size((PyObject*)arr);
          for(Py_ssize_t i = 0; i < nelements; ++i)
          {
            boost::python::object item = nparray[i];
            PyObject *s = PyObject_Str(item.ptr());
            cvector[i] = boost::python::extract<std::string>(s)();
          }
        }
      };

      /**
       * Templated structure to check the numpy type against the C type
       * and convert the array if necessary. It is assumed that
       * the object points to numpy array, no checking is performed
       */
      template <typename DestElementType>
      struct coerce_type
      {
        boost::python::object operator()(const boost::python::object &arr)
        {
          int sourceType = PyArray_TYPE((PyArrayObject*)arr.ptr());
          int destType = Converters::NDArrayTypeIndex<DestElementType>::typenum;
          boost::python::object result = arr;
          if( sourceType != destType )
          {
            PyObject * sameType = PyArray_Cast((PyArrayObject*)arr.ptr(), destType);
            result = boost::python::object(boost::python::handle<>(sameType));
          }
          return result;
        }
      };

      /**
       * Specialized version for std::string as we don't need
       * to convert the underlying representation
       */
      template <>
      struct coerce_type<std::string>
      {
        boost::python::object operator()(const boost::python::object &arr)
        {
          return arr;
        }
      };
    }

    namespace Converters
    {
      //-------------------------------------------------------------------------
      // NDArrayToVector definitions
      //-------------------------------------------------------------------------

      /**
       * Constructor
       * @param value :: A boost python object wrapping a numpy.ndarray
       */
      template<typename DestElementType>
      NDArrayToVector<DestElementType>::
      NDArrayToVector(const boost::python::object & value)
      : m_arr(value)
        {
        typeCheck();
        }

      /**
       * Creates a vector of the DestElementType from the numpy array of
       * given input type
       * @return A vector of the DestElementType created from the numpy array
       */
      template<typename DestElementType>
      const std::vector<DestElementType>
      NDArrayToVector<DestElementType>::operator()()
      {
        npy_intp length = PyArray_SIZE((PyArrayObject*)m_arr.ptr()); // Returns the total number of elements in the array
        std::vector<DestElementType> cvector(length);
        if(length > 0)
        {
          fill_vector<DestElementType>()((PyArrayObject*)m_arr.ptr(), cvector);
        }
        return cvector;
      }

      /**
       * Checks if the python object points to a numpy.ndarray and also checks if the type
       * is compatible with the DestElementType. If the types do not match the
       * array is cast to the DestElementType
       * @throws std::invalid_argument if the object is not of type numpy.ndarray
       */
      template<typename DestElementType>
      void
      NDArrayToVector<DestElementType>::typeCheck()
      {
        if( !PyArray_Check(m_arr.ptr()) )
        {
          throw std::invalid_argument(std::string("NDArrayConverter expects ndarray type, found ")
          + m_arr.ptr()->ob_type->tp_name);
        }
        m_arr = coerce_type<DestElementType>()(m_arr);

      }

      //------------------------------------------------------------------------
      // Explicit instantiations
      //------------------------------------------------------------------------
      #define INSTANTIATE_TOVECTOR(ElementType)\
        template DLLExport struct NDArrayToVector<ElementType>;

      ///@cond Doxygen doesn't seem to like this...
      INSTANTIATE_TOVECTOR(int)
      INSTANTIATE_TOVECTOR(long)
      INSTANTIATE_TOVECTOR(long long)
      INSTANTIATE_TOVECTOR(unsigned int)
      INSTANTIATE_TOVECTOR(unsigned long)
      INSTANTIATE_TOVECTOR(unsigned long long)
      INSTANTIATE_TOVECTOR(double)
      INSTANTIATE_TOVECTOR(bool)
      INSTANTIATE_TOVECTOR(std::string)
      ///@endcond
    }
  }
}

