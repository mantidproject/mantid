// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/Unit.h"
#include "MantidPythonInterface/core/GetPointer.h"

#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/tuple.hpp>

using Mantid::Kernel::Unit;
using Mantid::Kernel::Unit_sptr;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(Unit)

namespace {
/**
 * Returns the full name of the unit & raises a deprecation warning
 * @param self A reference to calling object
 */
const std::string deprecatedName(Unit &self) {
  PyErr_Warn(PyExc_DeprecationWarning,
             "'name' is deprecated, use 'caption' instead.");
  return self.caption();
}

/**
 * Returns the label of the unit as a std::string & raises a deprecation warning
 * @param self A reference to calling object
 */
const std::string deprecatedLabel(Unit &self) {
  PyErr_Warn(PyExc_DeprecationWarning,
             "'unit.label()' is deprecated, use 'str(unit.symbol())' instead.");
  return self.label().ascii();
}

template <class T>
tuple quickConversionWrapper(Unit &self, const T &destUnitName) {
  double wavelengthFactor = 0;
  double wavelengthPower = 0;
  bool converted =
      self.quickConversion(destUnitName, wavelengthFactor, wavelengthPower);
  if (!converted) {
    throw std::runtime_error("Quick conversion is not possible from unit:" +
                             std::string(self.unitID()) +
                             " to the desired unit.");
  }
  return boost::python::make_tuple<double>(wavelengthFactor, wavelengthPower);
}
} // namespace

void export_Unit() {

  register_ptr_to_python<boost::shared_ptr<Unit>>();

  class_<Unit, boost::noncopyable>("Unit", no_init)
      .def("name", &deprecatedName, arg("self"),
           "Return the full name of the unit (deprecated, use caption)")
      .def("caption", &Unit::caption, arg("self"),
           "Return the full name of the unit")
      .def("label", &deprecatedLabel, arg("self"),
           "Returns a plain-text label to be used "
           "as the symbol for the unit (deprecated, "
           "use symbol())")
      .def("symbol", &Unit::label, arg("self"),
           "Returns a UnitLabel object that holds "
           "information on the symbol to use for unit")
      .def("unitID", &Unit::unitID, arg("self"),
           "Returns the string ID of the unit. This may/may not match "
           "its name")
      .def("quickConversion", &quickConversionWrapper<Unit>,
           (arg("self"), arg("destUnitName")),
           "Check whether the unit can be converted to another via a "
           "simple factor")
      .def("quickConversion", &quickConversionWrapper<std::string>,
           (arg("self"), arg("destination")),
           "Check whether the unit can be converted to another via a "
           "simple factor");
}
