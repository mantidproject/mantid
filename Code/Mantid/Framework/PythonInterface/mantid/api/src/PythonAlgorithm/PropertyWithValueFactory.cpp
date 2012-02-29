//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/api/PythonAlgorithm/PropertyWithValueFactory.h"
#include "MantidKernel/PropertyWithValue.h"

namespace Mantid
{
  namespace PythonInterface
  {

    /**
     * Creates a PropertyWithValue<Type> instance from the given information.
     * The python type is mapped to a C type using the mapping defined by initPythonTypeMap()
     * @param name :: The name of the property
     * @param defaultValue :: A default value for this property.
     * @param direction :: Specifies whether the property is Input, InOut or Output
     * @returns A pointer to a new Property object
     */
    Kernel::Property *
    PropertyWithValueFactory::createProperty(const std::string & name , const boost::python::object & value, 
                                             const unsigned int direction)
    {
      return NULL;
    }

    /**
     * Creates a PropertyWithValue<Type> instance from the given information.
     * The python type is mapped to a C type using the mapping defined by initPythonTypeMap()
     * @param name :: The name of the property
     * @param defaultValue :: A default value for this property.
     * @param validator :: A validator object
     * @param direction :: Specifies whether the property is Input, InOut or Output
     * @returns A pointer to a new Property object
     */
    Kernel::Property *
    PropertyWithValueFactory::createProperty(const std::string & name , const boost::python::object & value, 
                                             const boost::python::object & validator, const unsigned int direction)
    {
      return NULL;
    }

  }
}
