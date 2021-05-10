// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/UnitConversion.h"
#include <boost/python/class.hpp>
#include <boost/python/suite/indexing/map_indexing_suite.hpp>

using Mantid::Kernel::DeltaEMode;
using Mantid::Kernel::UnitConversion;
using namespace boost::python;

double deprecatedSignature(const std::string &src, const std::string &dest, const double srcValue, const double l1,
                           const double l2, const double theta, const DeltaEMode::Type emode, const double efixed) {
  PyErr_Warn(PyExc_DeprecationWarning, ".run(src, dest, srcValue, l1, l2, theta, emode, efixed) is deprecated. "
                                       "Use .run(src, dest, srcValue, l1, emode, params) instead.");
  return UnitConversion::run(src, dest, srcValue, l1, l2, theta, emode, efixed);
}

void export_UnitConversion() {
  // Function pointer typedef
  using newStringVersion = double (*)(const std::string &, const std::string &, const double, const double,
                                      const DeltaEMode::Type, const Mantid::Kernel::UnitParametersMap &);

  class_<std::unordered_map<Mantid::Kernel::UnitParams, double>>("UnitParametersMap")
      .def(map_indexing_suite<std::unordered_map<Mantid::Kernel::UnitParams, double>>());

  class_<UnitConversion, boost::noncopyable>("UnitConversion", no_init)
      .def("run", &deprecatedSignature,
           (arg("src"), arg("dest"), arg("srcValue"), arg("l1"), arg("l2"), arg("theta"), arg("emode"), arg("efixed")),
           "Performs a unit conversion on a single value (deprecated).")
      .def("run", (newStringVersion)&UnitConversion::run,
           (arg("src"), arg("dest"), arg("srcValue"), arg("l1"), arg("emode"), arg("params")),
           "Performs a unit conversion on a single value.")
      .staticmethod("run");
}
