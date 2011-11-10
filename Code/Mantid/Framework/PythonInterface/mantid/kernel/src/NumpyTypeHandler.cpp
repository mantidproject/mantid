//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/kernel/NumpyTypeHandler.h"
#include "MantidPythonInterface/kernel/VectorDelegate.h"
#include "MantidKernel/IPropertyManager.h"

// See http://docs.scipy.org/doc/numpy/reference/c-api.array.html#PY_ARRAY_UNIQUE_SYMBOL
#define PY_ARRAY_UNIQUE_SYMBOL KERNEL_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/ndarrayobject.h>

namespace Mantid
{
  namespace PythonInterface
  {
    namespace PropertyMarshal
    {

      /**
       * Handle Python -> C++ calls to a property manager and get the correct type from the
       * python object
       * @param alg :: A pointer to an IPropertyManager
       * @param name :: The name of the property
       * @param value :: A boost python object that points to a numpy array
       */
      void NumpyTypeHandler::set(Kernel::IPropertyManager* alg,
                                    const std::string &name, boost::python::object value)
      {
        if( !PyArray_Check(value.ptr()) )
        {
          throw std::invalid_argument(std::string("NumpyTypeHandler::set - Cannot handle non-numpy array. Type passed: ") + value.ptr()->ob_type->tp_name);
        }
        PyArrayObject *nparray=(PyArrayObject*)value.ptr();
        // A numpy array is a homogeneous array, i.e each type is identical and the underlying array is contiguous
        const int ndim = PyArray_NDIM(nparray);
        if( ndim > 1 ) throw std::invalid_argument("NumpyTypeHandler::set - Currently unable to handle arrays with greater than 1.");

        // Numpy has type checking macros
        Kernel::Property *prop = alg->getPointerToProperty(name);
        const std::type_info * propTypeInfo = prop->type_info();
        if( typeid(std::vector<double>) == *propTypeInfo )
        {
          if( PyArray_CanCastSafely(nparray->descr->type_num, NPY_DOUBLE) )
          {
            std::vector<double> propValues = VectorDelegate::toStdVectorFromNumpy<double>(nparray);
            alg->setProperty(name, propValues);
          }
          else
          {
            throw std::invalid_argument(
                std::string("NumpyTypeHandler::set - Cannot convert from ") + nparray->descr->type
                    + " to a C++ double without loss of precision");
          }
        }
        else
        {
          throw std::invalid_argument("NumpyTypeHandler::set - Unable to handle non-double std::vector property types.");
        }
      }

      /**
       * Is the python object an instance a sequence type
       * @param value :: A python object
       * @returns True if it is, false otherwise
       */
      bool NumpyTypeHandler::isInstance(const boost::python::object & value) const
      {
        UNUSED_ARG(value);
        return false;
      }

    }
  }
}
