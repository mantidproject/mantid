//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/kernel/SequenceTypeHandler.h"
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
        UNUSED_ARG(alg); UNUSED_ARG(name); UNUSED_ARG(value);

        // We can't avoid a copy here as the final values will have reside in an array allocated
        // by the C++ new operator

        // There are several issues to deal with here.
        //  1) The sequence type could be one of many: list, tuple + others
        //  2) We need to know the item type so that we can use the correct C++ container type
        // Currently all are mapped to a std::vector

        // How to handle each C++ type ??


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
