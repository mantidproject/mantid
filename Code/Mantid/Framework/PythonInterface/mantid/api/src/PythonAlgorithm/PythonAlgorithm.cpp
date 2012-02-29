//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/api/PythonAlgorithm/PythonAlgorithm.h"
#include "MantidKernel/PropertyWithValue.h"
#include <boost/python/extract.hpp>

namespace Mantid
{
  namespace PythonInterface
  {
    using Mantid::Kernel::Property;
    using Mantid::Kernel::PropertyWithValue;

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
      //Property *p = new PropertyWithValue<int>(name, extract<int>(defaultValue)(), direction);
      //Property *algProp = PropertyWithValueFactory::create(defaultValue);
      //this->declareProperty(p);
    }

  }
}
