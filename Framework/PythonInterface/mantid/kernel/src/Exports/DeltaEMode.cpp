// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/DeltaEMode.h"
#include "MantidPythonInterface/core/Policies/VectorToNumpy.h"

#include <boost/python/class.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/return_value_policy.hpp>

using Mantid::Kernel::DeltaEMode;
namespace Policies = Mantid::PythonInterface::Policies;
using namespace boost::python;

void export_DeltaEMode() {
  enum_<Mantid::Kernel::DeltaEMode::Type>("DeltaEModeType")
      .value("Elastic", DeltaEMode::Elastic)
      .value("Direct", DeltaEMode::Direct)
      .value("Indirect", DeltaEMode::Indirect)
      .export_values();

  class_<DeltaEMode, boost::noncopyable>("DeltaEMode", no_init)
      .def("asString", &DeltaEMode::asString, arg("self"),
           "Returns the given type translated to a string")
      .def("fromString", &DeltaEMode::fromString, arg("modeStr"),
           "Returns the enumerated type translated from a string")
      .def("availableTypes", &DeltaEMode::availableTypes,
           return_value_policy<Policies::VectorToNumpy>(),
           "Returns a list of known delta E Modes as strings")
      .staticmethod("availableTypes");
}
