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

bool isMaskedDeprecated(IDetector const &self) {
  if (PyErr_Warn(PyExc_DeprecationWarning, "'DetectorGroup::isMasked' is deprecated, "
                                           "use 'SpectrumInfo::isMasked' instead.") < 0)
    throw boost::python::error_already_set();
  ALLOW_DEPRECATED(auto const &group = dynamic_cast<DetectorGroup const &>(self));
  bool masked = true;
  for (auto const &det : group.getDetectors()) {
    auto const &detInfo = det->parameterMap().detectorInfo();
    masked &= detInfo.isMasked(det->index());
  }
  return masked;
}

bool isMonitorDeprecated(IDetector const &self) {
  if (PyErr_Warn(PyExc_DeprecationWarning, "'DetectorGroup::isMonitor' is "
                                           "deprecated, use "
                                           "'SpectrumInfo::isMonitor' instead.") < 0)
    throw boost::python::error_already_set();
  ALLOW_DEPRECATED(auto const &group = dynamic_cast<DetectorGroup const &>(self));
  auto const &dets = group.getDetectors();
  return std::all_of(dets.cbegin(), dets.cend(),
                     [](auto const &det) { return det->parameterMap().detectorInfo().isMonitor(det->index()); });
}

std::vector<Mantid::detid_t> getDetectorIDsDeprecated(IDetector const &self) {
  warnDeprecated("getDetectorIDs");
  ALLOW_DEPRECATED(auto const &group = dynamic_cast<DetectorGroup const &>(self));
  return group.getDetectorIDs();
}

std::string getNameSeparatorDeprecated(IDetector const &self) {
  warnDeprecated("getNameSeparator");
  ALLOW_DEPRECATED(auto const &group = dynamic_cast<DetectorGroup const &>(self));
  return group.getNameSeparator();
}
} // namespace

void export_DetectorGroup() {
  GNU_DIAG_OFF("deprecated-declarations")
  MSVC_DIAG_OFF(4996)
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
  MSVC_DIAG_ON(4996)
  GNU_DIAG_ON("deprecated-declarations")
}
