//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/api/PythonAlgorithm/PythonAlgorithm.h"
#include "MantidPythonInterface/api/PythonAlgorithm/PropertyWithValueFactory.h"

namespace Mantid
{
  namespace PythonInterface
  {

    /**
    * Declare a property using the type of the defaultValue
    * @param name :: The name of the new property
    * @param defaultValue :: A default value for the property. The type is mapped to a C++ type
    * @param direction :: The direction of the property
    */
    void PythonAlgorithm::declareProperty(const std::string & name, const boost::python::object & defaultValue,
      const int direction)
    {
      using namespace boost::python;
      this->declareProperty(PropertyWithValueFactory::create(name, defaultValue, direction));
    }

  }
}
