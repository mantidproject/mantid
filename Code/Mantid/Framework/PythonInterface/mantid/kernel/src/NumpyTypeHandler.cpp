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
        Kernel::Property *prop = alg->getPointerToProperty(name);
        const std::type_info * propTypeInfo = prop->type_info();
        PyArrayObject *nparray=(PyArrayObject*)value.ptr();
        if( typeid(std::vector<double>) == *propTypeInfo )
        {
          this->setDoubleArrayProperty(alg, name, nparray);
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

      //-----------------------------------------------------------------------
      // Private memebers
      //-----------------------------------------------------------------------
      /**
       *  Handle double-type properties, i.e PropertyWithValue<std::vector<double> > types
       */
       void NumpyTypeHandler::setDoubleArrayProperty(Kernel::IPropertyManager* alg, const std::string &name, PyArrayObject * nparray)
       {
         // If we have a property of type double then we should still be able to accept integer arrays
         // MG: Not overly happy about how I'm doing this
         std::vector<double> propValues;
         switch(PyArray_TYPE(nparray))
         {
         case NPY_DOUBLE:
            propValues = VectorDelegate::toStdVectorFromNumpy<double, npy_double>(nparray);
           break;
         case NPY_INT64:
           propValues = VectorDelegate::toStdVectorFromNumpy<double, npy_int64>(nparray);
           break;
         case NPY_INT32:
           propValues = VectorDelegate::toStdVectorFromNumpy<double, npy_int32>(nparray);
           break;
         default:
           throw std::invalid_argument(
               std::string("NumpyTypeHandler::set - Cannot convert from ") + nparray->descr->type
                   + " to a C++ double without loss of precision");
         }

         alg->setProperty(name, propValues);
       }

    }
  }
}
