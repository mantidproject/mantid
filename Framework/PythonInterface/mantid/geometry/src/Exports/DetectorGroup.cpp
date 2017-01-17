#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidBeamline/DetectorInfo.h"
#include <boost/python/class.hpp>

using Mantid::Geometry::DetectorGroup;
using Mantid::Geometry::IDetector;
using namespace boost::python;

namespace {
bool isMaskedDeprecated(const DetectorGroup &self) {
  PyErr_Warn(PyExc_DeprecationWarning,
             "'DetectorGroup::isMasked' is deprecated, "
             "use 'SpectrumInfo::isMasked' instead.");
  const auto &dets = self.getDetectors();
  bool masked = true;
  for (const auto &det : dets) {
    const auto &detInfo = det->parameterMap().detectorInfo();
    masked &= detInfo.isMasked(det->index());
  }
  return masked;
}

bool isMonitorDeprecated(const DetectorGroup &self) {
  PyErr_Warn(PyExc_DeprecationWarning, "'DetectorGroup::isMonitor' is "
                                       "deprecated, use "
                                       "'SpectrumInfo::isMonitor' instead.");
  const auto &dets = self.getDetectors();
  for (const auto &det : dets) {
    const auto &detInfo = det->parameterMap().detectorInfo();
    if (!detInfo.isMonitor(det->index()))
      return false;
  }
  return true;
}
}

void export_DetectorGroup() {
  class_<DetectorGroup, bases<IDetector>, boost::noncopyable>("DetectorGroup",
                                                              no_init)
      .def("isMasked", &isMaskedDeprecated, arg("self"),
           "Returns the value of the masked flag. True means ignore this "
           "detector")
      .def("isMonitor", &isMonitorDeprecated, arg("self"),
           "Returns True if the detector is marked as a monitor in the IDF")
      .def("getDetectorIDs", &DetectorGroup::getDetectorIDs, arg("self"),
           "Returns the list of detector IDs within this group")
      .def("getNameSeparator", &DetectorGroup::getNameSeparator, arg("self"),
           "Returns separator for list of names of detectors");
}
