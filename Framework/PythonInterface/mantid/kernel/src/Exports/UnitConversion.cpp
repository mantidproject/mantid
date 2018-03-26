#include "MantidKernel/UnitConversion.h"
#include <boost/python/class.hpp>

using Mantid::Kernel::UnitConversion;
using Mantid::Kernel::DeltaEMode;
using namespace boost::python;

void export_UnitConversion() {
  // Function pointer typedef
  using StringVersion = double (
      *)(const std::string &, const std::string &, const double, const double,
         const double, const double, const DeltaEMode::Type, const double);

  class_<UnitConversion, boost::noncopyable>("UnitConversion", no_init)
      .def("run", (StringVersion)&UnitConversion::run,
           (arg("src"), arg("dest"), arg("srcValue"), arg("l1"), arg("l2"),
            arg("theta"), arg("emode"), arg("efixed")),
           "Performs a unit conversion on a single value.")
      .staticmethod("run");
}
