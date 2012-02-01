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
        const std::type_info & propTypeInfo = *(prop->type_info());
        PyArrayObject *nparray=(PyArrayObject*)value.ptr();
        // If we are going to a std::vector<string> just try and extract that out
        if( typeid(std::vector<std::string>) == propTypeInfo )
        {
          this->setStringArrayProperty(alg, name, nparray);
        }
        else if( typeid(std::vector<double>) == propTypeInfo )
        {
          this->setDoubleArrayProperty(alg, name, nparray);
        }
        else if( PyArray_ISINTEGER(nparray) )
        {
          this->setIntNumpyProperty(alg, name, propTypeInfo, nparray);
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
      // Private members
      //-----------------------------------------------------------------------
      /**
       *  Handle double-type properties, i.e PropertyWithValue<std::vector<double> > types
       *  @param alg :: A pointer to a property manager
       *  @param name :: The name of the property to set
       *  @param nparray :: A pointer to a numpy array
       */
       void NumpyTypeHandler::setDoubleArrayProperty(Kernel::IPropertyManager* alg, const std::string &name, PyArrayObject * nparray)
       {
         // If we have a property of type vector_double then we should be able to set that with an array of integers as well as doubles
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
       /**
        *  Handle integer-type properties, i.e PropertyWithValue<std::vector<double> > types
        *  @param alg :: A pointer to a property manager
        *  @param name :: The name of the property to set
        *  @param typeInfo :: A reference to the typeinfo object for the property's value type
        *  @param nparray :: A pointer to a numpy array
        */
        void NumpyTypeHandler::setIntNumpyProperty(Kernel::IPropertyManager* alg, const std::string &name,
            const std::type_info & typeInfo, PyArrayObject * nparray)
        {
          // We know that the numpy array is some kind of integer signed/unsigned
          int typeNum = PyArray_TYPE(nparray);

          /**
           * We need to make sure that some type isn't accidentally cast down to a lower
           * precision type when going into a property
           */

          if( typeInfo == typeid(std::vector<size_t>) )
          {
            std::vector<size_t> propValues;
            switch(typeNum)
            {
            case NPY_INT32:
              propValues = VectorDelegate::toStdVectorFromNumpy<size_t, npy_int32>(nparray);
              break;
            case NPY_INT64:
              propValues = VectorDelegate::toStdVectorFromNumpy<size_t, npy_int64>(nparray);
              break;
            case NPY_UINT:
              propValues = VectorDelegate::toStdVectorFromNumpy<size_t, npy_uint>(nparray);
              break;
            default:
              throw std::invalid_argument(
                  std::string("NumpyTypeHandler::set - Cannot convert from ") + nparray->descr->typeobj->tp_name
                      + " to a C++ size_t without loss of precision");
            }
            alg->setProperty(name, propValues);
          }
          else if( typeInfo == typeid(std::vector<int32_t>) )
          {
            std::vector<int32_t> propValues;
            switch(typeNum)
            {
              case NPY_INT32:
                propValues = VectorDelegate::toStdVectorFromNumpy<int32_t, npy_int32>(nparray);
                break;
              default:
                throw std::invalid_argument(
                    std::string("NumpyTypeHandler::set - Cannot convert from ") + nparray->descr->typeobj->tp_name
                      + " to a C++ 32-bit int without loss of precision");
            }
            alg->setProperty(name, propValues);
          }
          else if( typeInfo == typeid(std::vector<int64_t>) )
          {
            std::vector<int64_t> propValues;
            switch(typeNum)
            {
              case NPY_INT64:
                propValues = VectorDelegate::toStdVectorFromNumpy<int64_t, npy_int64>(nparray);
                break;
              default:
                throw std::invalid_argument(
                    std::string("NumpyTypeHandler::set - Cannot convert from ") + nparray->descr->typeobj->tp_name
                      + " to a C++ 64-bit int without loss of precision");
            }
            alg->setProperty(name, propValues);
          }
        }

        /**
         * @param alg :: A pointer to the property manager
         * @param name :: The name of the property
         * @param nparray :: A numpy array
         */
        void NumpyTypeHandler::setStringArrayProperty(Kernel::IPropertyManager* alg, const std::string &name, PyArrayObject * nparray)
        {
          /// @todo: Needs to be merged with delegate code so that we can avoid turning it into a list
          PyObject *lst = PyArray_ToList(nparray);
          std::vector<std::string> propValues = VectorDelegate::toStdVector<std::string>(lst);
          Py_DECREF(lst); // List is temporary, make sure we drop the reference
          alg->setProperty(name, propValues);
        }


    }
  }
}
