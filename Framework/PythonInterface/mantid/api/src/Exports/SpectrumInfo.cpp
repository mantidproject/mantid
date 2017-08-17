#include "MantidAPI/SpectrumInfo.h"
#include <boost/python/class.hpp>

using Mantid::API::SpectrumInfo;
using namespace boost::python;

void export_SpectrumInfo() {
  // WARNING SpectrumInfo is work in progress and not ready for exposing more of
  // its functionality to Python, and should not yet be used in user scripts. DO
  // NOT ADD EXPORTS TO OTHER METHODS without contacting the team working on
  // Instrument-2.0.
  class_<SpectrumInfo, boost::noncopyable>("SpectrumInfo", no_init)
      .def("isMonitor", &SpectrumInfo::isMonitor, (arg("self"), arg("index")),
           "Returns true if the detector(s) associated with the spectrum are "
           "monitors.")
      .def("isMasked", &SpectrumInfo::isMasked, (arg("self"), arg("index")),
           "Returns true if the detector(s) associated with the spectrum are "
           "masked.")
      .def("hasDetectors", &SpectrumInfo::hasDetectors, (arg("self")),
           "Returns true if the spectrum is associated with detectors in the "
           "instrument.");
}
