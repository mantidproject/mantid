// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/PropertyWithValue.h"
#include "MantidPythonInterface/core/Converters/ContainerDtype.h"

#ifndef Q_MOC_RUN
#include <boost/python/bases.hpp>
#include <boost/python/class.hpp>
#include <boost/python/return_by_value.hpp>
#include <boost/python/return_value_policy.hpp>
#endif

// Call the dtype helper function
template <typename HeldType> std::string dtype(Mantid::Kernel::PropertyWithValue<HeldType> &self) {
  // Check for the special case of a string
  if (std::is_same<HeldType, std::string>::value) {
    std::stringstream ss;
    std::string val = self.value();
    ss << "S" << val.size();
    std::string ret_val = ss.str();
    return ret_val;
  }

  return Mantid::PythonInterface::Converters::dtype(self);
}

namespace Mantid {
namespace PythonInterface {

/**
 * A helper struct to export PropertyWithValue<> types to Python.
 */
template <typename HeldType, typename ValueReturnPolicy = boost::python::return_by_value>
struct PropertyWithValueExporter {
  static void define(const char *pythonClassName) {
    using namespace boost::python;
    using namespace Mantid::Kernel;

    class_<PropertyWithValue<HeldType>, bases<Property>, boost::noncopyable>(
        pythonClassName, init<std::string, HeldType, unsigned int>(
                             (arg("self"), arg("name"), arg("value"), arg("direction") = Direction::Input)))
        .add_property("value",
                      make_function(&PropertyWithValue<HeldType>::operator(), return_value_policy<ValueReturnPolicy>()))
        .def("dtype", &dtype<HeldType>, arg("self"));
  }
};
} // namespace PythonInterface
} // namespace Mantid
