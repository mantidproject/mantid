//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/api/PythonAlgorithm/PythonAlgorithm.h"
#include "MantidPythonInterface/kernel/Registry/PropertyWithValueFactory.h"

namespace Mantid
{
  namespace PythonInterface
  {

    /**
     * Declare a preconstructed property
     * @param prop :: A pointer to a property
     * @param doc :: An optional doc string
     */
    void PythonAlgorithm::declareProperty(Kernel::Property *prop, const std::string & doc)
    {
      // We need to clone the property so that python doesn't own the object that gets inserted
      // into the manager
      Algorithm::declareProperty(prop->clone(), doc);
    }

    /**
     * Declare a property using the type of the defaultValue, a documentation string and validator
     * @param name :: The name of the new property
     * @param defaultValue :: A default value for the property. The type is mapped to a C++ type
     * @param validator :: A validator object
     * @param doc :: The documentation string
     * @param direction :: The direction of the property
     */
    void PythonAlgorithm::declareProperty(const std::string & name, const boost::python::object & defaultValue,
                                          const boost::python::object & validator,
                                          const std::string & doc, const int direction)
    {
      this->declareProperty(Registry::PropertyWithValueFactory::create(name, defaultValue, validator, direction), doc);
    }

    /**
     * Declare a property using the type of the defaultValue and a documentation string
     * @param name :: The name of the new property
     * @param defaultValue :: A default value for the property. The type is mapped to a C++ type
     * @param doc :: The documentation string
     * @param direction :: The direction of the property
     */
    void PythonAlgorithm::declareProperty(const std::string & name, const boost::python::object & defaultValue,
                                          const std::string & doc, const int direction)
    {
      this->declareProperty(Registry::PropertyWithValueFactory::create(name, defaultValue, direction), doc);
    }

    /**
    * Declare a property using the type of the defaultValue
    * @param name :: The name of the new property
    * @param defaultValue :: A default value for the property. The type is mapped to a C++ type
    * @param direction :: The direction of the property
    */
    void PythonAlgorithm::declareProperty(const std::string & name, const boost::python::object & defaultValue,
                                          const int direction)
    {
      declareProperty(name, defaultValue, "", direction);
    }

  }
}
