// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <boost/python/dict.hpp>
#include <memory>

namespace Mantid {
namespace Kernel {
class PropertyManager;
}
namespace PythonInterface {
namespace Registry {

/// Create a C++ PropertyMananager from a Python dictionary
std::shared_ptr<Kernel::PropertyManager> createPropertyManager(const boost::python::dict &mapping);
} // namespace Registry
} // namespace PythonInterface
} // namespace Mantid
