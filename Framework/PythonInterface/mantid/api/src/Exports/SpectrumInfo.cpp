#include "MantidAPI/SpectrumInfo.h"
#include <boost/python/class.hpp>

using Mantid::API::SpectrumInfo;
using namespace boost::python;

void export_SpectrumInfo() {
  class_<SpectrumInfo, boost::noncopyable>("SpectrumInfo", no_init)
      .def("isMonitor", &SpectrumInfo::isMonitor, (arg("self"), arg("index")),
           "Returns true if the detector(s) associated with the spectrum are "
           "monitors.")
      .def("isMasked", &SpectrumInfo::isMasked, (arg("self"), arg("index")),
           "Returns true if the detector(s) associated with the spectrum are "
           "masked.")
      .def("hasDetectors", &SpectrumInfo::hasDetectors, (arg("self")),
           "Returns true if the spectrum is associated with detectors in the "
           "instrument.")
      .def("position", &SpectrumInfo::position, (arg("self"), arg("index")),
           "Returns the position of the detector(s) associated with the spectrum.")
      .def("l2", &SpectrumInfo::l2, (arg("self"), arg("index")),
           "Returns the distance from the sample of the detector(s) "
           "associated with the spectrum.")
      .def("phi", &SpectrumInfo::phi, (arg("self"), arg("index")),
           "Returns phi angle of the detector(s) associated with the spectrum.")
      .def("twoTheta", &SpectrumInfo::twoTheta, (arg("self"), arg("index")),
           "Returns twoTheta of the detector(s) associated with the spectrum.");
}
