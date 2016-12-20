#include "MantidAPI/DetectorInfo.h"
#include <boost/python/class.hpp>

using Mantid::API::DetectorInfo;
using namespace boost::python;

void export_DetectorInfo() {
  class_<DetectorInfo, boost::noncopyable>("DetectorInfo", no_init)
      .def("__len__", &DetectorInfo::size, (arg("self")),
           "Returns the size of the DetectorInfo, i.e., the number of "
           "detectors in the instrument.")
      .def("size", &DetectorInfo::size, (arg("self")),
           "Returns the size of the DetectorInfo, i.e., the number of "
           "detectors in the instrument.")
      .def("isMasked", &DetectorInfo::isMasked, (arg("self"), arg("index")),
           "Returns True if the detector is masked.");
}
