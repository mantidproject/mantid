#include "MantidGeometry/Instrument/DetectorInfo.h"

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/return_value_policy.hpp>

using Mantid::Geometry::DetectorInfo;
using namespace boost::python;

void export_DetectorInfo() {
  
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
           "Returns True if the detector is masked.")

      .def("isEquivalent", &DetectorInfo::isEquivalent,
           (arg("self"), arg("other")),
           "Returns true if the content of this is equivalent to the content "
           "of other.");
}
