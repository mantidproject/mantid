// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/FunctionProperty.h"
#include "MantidPythonInterface/core/PropertyWithValueExporter.h"
#include <boost/python/class.hpp>

using Mantid::API::FunctionProperty;
using Mantid::API::IFunction;
using Mantid::Kernel::Direction;
using Mantid::Kernel::PropertyWithValue;
using Mantid::PythonInterface::PropertyWithValueExporter;
using namespace boost::python;

void export_FunctionProperty() {
  // FuncitonProperty has base PropertyWithValue<std::shared_ptr<IFunction>>
  // which must be exported
  using HeldType = std::shared_ptr<IFunction>;
  PropertyWithValueExporter<HeldType>::define("FunctionPropertyWithValue");

  class_<FunctionProperty, bases<PropertyWithValue<HeldType>>, boost::noncopyable>("FunctionProperty", no_init)
      .def(
          init<const std::string &, const unsigned int>((arg("self"), arg("name"), arg("direction") = Direction::Input),
                                                        "Constructs a FunctionProperty with the given name"));
}
