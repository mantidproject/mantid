// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidKernel/WarningSuppressions.h"
#include <boost/python/class.hpp>

#include <algorithm>
#include <string>
#include <vector>

// DetectorGroup is deprecated, but the Python export remains until it is removed
GNU_DIAG_OFF("deprecated-declarations")
MSVC_DIAG_OFF(4996)

using Mantid::Geometry::DetectorGroup;
using Mantid::Geometry::IDetector;
using namespace boost::python;

namespace {
constexpr auto DEPRECATION_MESSAGE_SUFFIX =
    " is deprecated in Mantid 7.1. "
    "For more information, see the instrument access layers concept page: "
    "https://docs.mantidproject.org/nightly/concepts/InstrumentAccessLayers.html";

void warnDeprecated(std::string const &name) {
  if (PyErr_Warn(PyExc_DeprecationWarning, ("'DetectorGroup." + name + "'" + DEPRECATION_MESSAGE_SUFFIX).c_str()) < 0)
    throw boost::python::error_already_set();
}

bool isMaskedDeprecated(DetectorGroup const &self) {
  if (PyErr_Warn(PyExc_DeprecationWarning, "'DetectorGroup::isMasked' is deprecated, "
                                           "use 'SpectrumInfo::isMasked' instead.") < 0)
    throw boost::python::error_already_set();
  bool masked = true;
  for (auto const &det : self.getDetectors()) {
    auto const &detInfo = det->parameterMap().detectorInfo();
    masked &= detInfo.isMasked(det->index());
  }
  return masked;
}

bool isMonitorDeprecated(DetectorGroup const &self) {
  if (PyErr_Warn(PyExc_DeprecationWarning, "'DetectorGroup::isMonitor' is "
                                           "deprecated, use "
                                           "'SpectrumInfo::isMonitor' instead.") < 0)
    throw boost::python::error_already_set();
  auto const &dets = self.getDetectors();
  return std::all_of(dets.cbegin(), dets.cend(),
                     [](auto const &det) { return det->parameterMap().detectorInfo().isMonitor(det->index()); });
}

std::vector<Mantid::detid_t> getDetectorIDsDeprecated(DetectorGroup const &self) {
  warnDeprecated("getDetectorIDs");
  return self.getDetectorIDs();
}

std::string getNameSeparatorDeprecated(DetectorGroup const &self) {
  warnDeprecated("getNameSeparator");
  return self.getNameSeparator();
}
} // namespace

void export_DetectorGroup() {
  class_<DetectorGroup, bases<IDetector>, boost::noncopyable>("DetectorGroup", no_init)
      .def("isMasked", &isMaskedDeprecated, arg("self"),
           "Returns the value of the masked flag. True means ignore this "
           "detector")
      .def("isMonitor", &isMonitorDeprecated, arg("self"),
           "Returns True if the detector is marked as a monitor in the IDF")
      .def("getDetectorIDs", &getDetectorIDsDeprecated, arg("self"),
           "Returns the list of detector IDs within this group")
      .def("getNameSeparator", &getNameSeparatorDeprecated, arg("self"),
           "Returns separator for list of names of detectors");
}

MSVC_DIAG_ON(4996)
GNU_DIAG_ON("deprecated-declarations")
