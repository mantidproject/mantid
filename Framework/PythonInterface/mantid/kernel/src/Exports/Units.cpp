// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/Unit.h"

#include <boost/python/class.hpp>

using Mantid::Kernel::Unit;
using Mantid::Kernel::UnitLabel;
using namespace Mantid::Kernel::Units;
using namespace boost::python;

namespace {
/**
 * Proxy to construct a UnitLabel directly from a Python str without
 * having to construct a UnitLabel object
 * @param self The calling object
 * @param caption The name of the unit
 * @param label The symbol for the unit
 */
void setLabelFromStdString(Label &self, const std::string &caption, const std::string &label) {
  self.setLabel(caption, label);
}
} // namespace

// We only export the concrete unit classes that
// have additional functionality over the base class
void export_Label() {
  class_<Label, bases<Unit>, boost::noncopyable>("Label", no_init)
      .def("setLabel", &setLabelFromStdString, (arg("self"), arg("caption"), arg("label")),
           "Set the caption (e.g.Temperature) & label (K) on the unit")

      .def("setLabel", (void(Label::*)(const std::string &, const UnitLabel &)) & Label::setLabel,
           (arg("self"), arg("caption"), arg("label")),
           "Set the caption (e.g.Temperature) "
           "& label (K) on the unit, See the "
           "UnitLabel class");
}
