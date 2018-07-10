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

      .def("setMasked", &SpectrumInfo::setMasked,
           (arg("self"), arg("index"), arg("masked")),
           "Set the mask flag of the spectrum with the given index.")

      .def("hasDetectors", &SpectrumInfo::hasDetectors, (arg("self")),
           "Returns true if the spectrum is associated with detectors in the "
           "instrument.")

      .def("twoTheta", &SpectrumInfo::twoTheta, (arg("self"), arg("index")),
           "Returns the scattering angle 2 theta in radians w.r.t. beam "
           "direction.")

      .def("signedTwoTheta", &SpectrumInfo::signedTwoTheta,
           (arg("self"), arg("index")),
           "Returns the signed scattering angle 2 theta in radians w.r.t. beam "
           "direction.")

      .def("size", &SpectrumInfo::size, arg("self"),
           "Returns the number of spectra.")

      .def("hasUniqueDetector", &SpectrumInfo::hasUniqueDetector,
           (arg("self"), arg("index")),
           "Returns true if the spectrum is associated with exactly one "
           "detector.")

      .def("l1", &SpectrumInfo::l1, arg("self"),
           "Returns the distance from the source to the sample.")

      .def("l2", &SpectrumInfo::l2, (arg("self"), arg("index")),
           "Returns the distance from the sample to the spectrum.")

      .def("position", &SpectrumInfo::position, (arg("self"), arg("index")),
           "Returns the position of the spectrum with the given index.")

      .def("sourcePosition", &SpectrumInfo::sourcePosition, arg("self"),
           "Returns the source position.")

      .def("samplePosition", &SpectrumInfo::samplePosition, arg("self"),
           "Returns the sample position.")
}
