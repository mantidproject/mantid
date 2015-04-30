#include "MantidKernel/UnitConversion.h"
#include <boost/python/class.hpp>
#include <boost/python/args.hpp>

using Mantid::Kernel::UnitConversion;
using Mantid::Kernel::DeltaEMode;
using namespace boost::python;

// clang-format off
void export_UnitConversion()
// clang-format on
{
  // Function pointer typedef
  typedef double (*StringVersion)(const std::string & src, const std::string & dest,
      const double srcValue,
      const double l1, const double l2,
      const double twoTheta, const DeltaEMode::Type emode, const double efixed);

  class_<UnitConversion, boost::noncopyable>("UnitConversion", no_init)
    .def("run", (StringVersion)&UnitConversion::run,
         (arg("src"), arg("dest"), arg("srcValue"), arg("l1"), arg("l2"), arg("twoTheta"),
          arg("emode"), arg("efixed")),
         "Performs a unit conversion on a single value.")
    .staticmethod("run")
  ;
}

