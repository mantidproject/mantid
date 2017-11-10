#include "MantidKernel/UnitConversion.h"
#include <boost/python/class.hpp>

using Mantid::Kernel::UnitConversion;
using Mantid::Kernel::DeltaEMode;
using namespace boost::python;

void export_UnitConversion() {
  // Function pointer typedef
  typedef double (*StringVersion)(
      const std::string &src, const std::string &dest, const double srcValue,
      const double l1, const double l2, const double theta,
      const DeltaEMode::Type emode, const double efixed);

  class_<UnitConversion, boost::noncopyable>("UnitConversion", no_init)
      .def("run", (StringVersion)&UnitConversion::run,
           (arg("src"), arg("dest"), arg("srcValue"), arg("l1"), arg("l2"),
            arg("theta"), arg("emode"), arg("efixed")),
           "Performs a unit conversion on a single value.")
      .staticmethod("run");
}
