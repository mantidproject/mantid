// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PYTHONINTERFACE_PROPERTYMANAGERFACTORY_H
#define MANTID_PYTHONINTERFACE_PROPERTYMANAGERFACTORY_H

#include <boost/python/dict.hpp>
#include <boost/shared_ptr.hpp>

namespace Mantid {
namespace Kernel {
class PropertyManager;
}
namespace PythonInterface {
namespace Registry {

/// Create a C++ PropertyMananager from a Python dictionary
boost::shared_ptr<Kernel::PropertyManager>
createPropertyManager(const boost::python::dict &mapping);
} // namespace Registry
} // namespace PythonInterface
} // namespace Mantid

#endif // MANTID_PYTHONINTERFACE_PROPERTYMANAGERFACTORY_H
