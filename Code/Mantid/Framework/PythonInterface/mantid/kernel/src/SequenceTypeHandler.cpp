//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/kernel/SequenceTypeHandler.h"
#include "MantidPythonInterface/kernel/VectorDelegate.h"
#include "MantidKernel/IPropertyManager.h"

namespace Mantid
{
  namespace PythonInterface
  {
    namespace PropertyMarshal
    {


      /**
       * Set function to handle Python -> C++ calls to a property manager and get the correct type
       * @param alg :: A pointer to an IPropertyManager
       * @param name :: The name of the property
       * @param value :: A boost python object that stores the container values
       */
      void SequenceTypeHandler::set(Kernel::IPropertyManager* alg,
                                    const std::string &name, boost::python::object value)
      {
        // We can't avoid a copy here as the final values will have reside in an array allocated
        // by the C++ new operator

        // We need some way of instantiating the correct vector type. We'll take the type from the
        // first element in the list
        Kernel::Property *prop = alg->getPointerToProperty(name);
        const std::type_info * propTypeInfo = prop->type_info();
        PyObject *firstElement = PySequence_Fast_GET_ITEM(value.ptr(), 0);
        if( PyString_Check(firstElement) || *propTypeInfo == typeid(std::vector<std::string>) )
        {
          std::vector<std::string> propValues = VectorDelegate::toStdVector<std::string>(value.ptr());
          alg->setProperty(name, propValues);
        }
        else if( PyInt_Check(firstElement) || PyLong_Check(firstElement) )
        {
          // The actual property could be a variety of flavours of integer type
          // MG (2011/11/09): I'm not over the moon about this implementation but I don't
          // have a better idea.
          if( typeid(std::vector<int>) == *propTypeInfo )
          {
            std::vector<int> propValues = VectorDelegate::toStdVector<int>(value.ptr());
            alg->setProperty(name, propValues);
          }
          else if( typeid(std::vector<size_t>) == *propTypeInfo )
          {
            std::vector<size_t> propValues = VectorDelegate::toStdVector<size_t>(value.ptr());
            alg->setProperty(name, propValues);
          }
          else
          {
            throw std::invalid_argument("SequenceTypeHandler::set - The property type for '" + name  + "' could not been handled.");
          }
        }
        else if( PyFloat_Check(firstElement) )
        {
          std::vector<double> propValues = VectorDelegate::toStdVector<double>(value.ptr());
          alg->setProperty(name, propValues);
        }
        else
        {
          throw std::invalid_argument("SequenceTypeHandler::set - The first element of the value for the '" + name + "' property cannot be handled.");
        }

      }

      /**
       * Is the python object an instance a sequence type
       * @param value :: A python object
       * @returns True if it is, false otherwise
       */
      bool SequenceTypeHandler::isInstance(const boost::python::object & value) const
      {
        UNUSED_ARG(value);
        return false;
      }

    }
  }
}
