// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PYTHONINTERFACE_PROPERTYVALUEHANDLER_H_
#define MANTID_PYTHONINTERFACE_PROPERTYVALUEHANDLER_H_

#include "MantidKernel/System.h"
#include <boost/python/object.hpp>
#include <memory>
#include <string>

namespace Mantid {
namespace Kernel {
// Forward declarations
class DataItem;
class IPropertyManager;
class Property;
} // namespace Kernel
namespace PythonInterface {
namespace Registry {
/**
 * This class provides a base-class objects that are able to take
 * a python object and set it on an algorithm property.
 *
 * The set function should call the setProperty method once it has the
 * correct C++ type from the Python object
 */
struct DLLExport PropertyValueHandler {
  /// Virtual Destructor
  virtual ~PropertyValueHandler() = default;
  /// Overload to set the named property's value on the property manager
  virtual void set(Kernel::IPropertyManager *alg, const std::string &name,
                   const boost::python::object &value) const = 0;
  /// Overload to create a Property type from the given value with no validation
  virtual std::unique_ptr<Kernel::Property>
  create(const std::string &name, const boost::python::object &value,
         const boost::python::object &validator,
         const unsigned int direction) const = 0;
};
} // namespace Registry
} // namespace PythonInterface
} // namespace Mantid

#endif /* MANTID_PYTHONINTERFACE_PROPERTYVALUEHANDLER_H_ */
