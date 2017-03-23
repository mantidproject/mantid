#include "MantidAPI/DetectorInfo.h"
#include <boost/python/class.hpp>

using Mantid::API::DetectorInfo;
using namespace boost::python;

void export_DetectorInfo() {
  // WARNING DetectorInfo is work in progress and not ready for exposing more of
  // its functionality to Python, and should not yet be used in user scripts. DO
  // NOT ADD EXPORTS TO OTHER METHODS without contacting the team working on
  // Instrument-2.0.
  bool (DetectorInfo::*isMonitor)(const size_t) const =
      &DetectorInfo::isMonitor;
  bool (DetectorInfo::*isMasked)(const size_t) const = &DetectorInfo::isMasked;
  class_<DetectorInfo, boost::noncopyable>("DetectorInfo", no_init)
      .def("__len__", &DetectorInfo::size, (arg("self")),
           "Returns the size of the DetectorInfo, i.e., the number of "
           "detectors in the instrument.")
      .def("size", &DetectorInfo::size, (arg("self")),
           "Returns the size of the DetectorInfo, i.e., the number of "
           "detectors in the instrument.")
      .def("isMonitor", isMonitor, (arg("self"), arg("index")),
           "Returns True if the detector is a monitor.")
      .def("isMasked", isMasked, (arg("self"), arg("index")),
           "Returns True if the detector is masked.");
}
