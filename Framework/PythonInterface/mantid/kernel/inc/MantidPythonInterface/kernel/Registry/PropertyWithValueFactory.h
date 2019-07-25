// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PYTHONINTERFACE_PROPERTYWITHVALUEFACTORY_H_
#define MANTID_PYTHONINTERFACE_PROPERTYWITHVALUEFACTORY_H_

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/kernel/Registry/PropertyValueHandler.h"
#include <boost/python/list.hpp>
#include <memory>

namespace Mantid {
//---------------------------------------------------------------------------
// Forward declarations
//---------------------------------------------------------------------------
namespace Kernel {
class Property;
}

namespace PythonInterface {
namespace Registry {
/**
 * Defines a static factory class that creates PropertyWithValue
 * instances from python objects.
 */
class DLLExport PropertyWithValueFactory {
public:
  static std::unique_ptr<Kernel::Property>
  create(const std::string &name, const boost::python::object &defaultValue,
         const boost::python::object &validator, const unsigned int direction);

  static std::unique_ptr<Kernel::Property>
  create(const std::string &name, const boost::python::object &defaultValue,
         const unsigned int direction);

  static std::unique_ptr<Kernel::Property>
  createTimeSeries(const std::string &name,
                   const boost::python::list &defaultValue);

private:
  /// Return a handler that maps the python type to a C++ type
  static const PropertyValueHandler &lookup(PyObject *const object);
  /// Return a string based on the python array type
  static const std::string isArray(PyObject *const object);
};
} // namespace Registry
} // namespace PythonInterface
} // namespace Mantid

#endif // MANTID_PYTHONINTERFACE_PROPERTYWITHVALUEFACTORY_H_
