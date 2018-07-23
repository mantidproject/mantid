#include "MantidAPI/SpectrumInfo.h"
#include "MantidTypes/SpectrumDefinition.h"

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/list.hpp>
#include <boost/python/return_value_policy.hpp>

using Mantid::SpectrumDefinition;
using Mantid::API::SpectrumInfo;
using namespace boost::python;

// Helper function to create and return a list of all the spectrum definitions
boost::python::list getSpectrumDefinitionList(const SpectrumInfo &self) {
  // Create the list
  boost::python::list spectrumDefinitionList;

  // Get all the spectrum definitions
  for (size_t i = 0; i < self.size(); i++) {
    spectrumDefinitionList.append(self.spectrumDefinition(i));
  }

  return spectrumDefinitionList;
}

void export_SpectrumInfo() {
  // WARNING SpectrumInfo is work in progress and not ready for exposing more of
  // its functionality to Python, and should not yet be used in user scripts. DO
  // NOT ADD EXPORTS TO OTHER METHODS without contacting the team working on
  // Instrument-2.0.
  class_<SpectrumInfo, boost::noncopyable>("SpectrumInfo", no_init)
      .def("__len__", &SpectrumInfo::size, arg("self"),
           "Returns the number of spectra.")

      .def("size", &SpectrumInfo::size, arg("self"),
           "Returns the number of spectra.")

      .def("isMonitor", &SpectrumInfo::isMonitor, (arg("self"), arg("index")),
           "Returns true if the detector(s) associated with the spectrum are "
           "monitors.")

      .def("isMasked", &SpectrumInfo::isMasked, (arg("self"), arg("index")),
           "Returns true if the detector(s) associated with the spectrum are "
           "masked.")

      .def("setMasked", &SpectrumInfo::setMasked,
           (arg("self"), arg("index"), arg("masked")),
           "Set the mask flag of the spectrum with the given index.")

      .def("twoTheta", &SpectrumInfo::twoTheta, (arg("self"), arg("index")),
           "Returns the scattering angle 2 theta in radians w.r.t. beam "
           "direction.")

      .def("signedTwoTheta", &SpectrumInfo::signedTwoTheta,
           (arg("self"), arg("index")),
           "Returns the signed scattering angle 2 theta in radians w.r.t. beam "
           "direction.")

      .def("l1", &SpectrumInfo::l1, arg("self"),
           "Returns the distance from the source to the sample.")

      .def("l2", &SpectrumInfo::l2, (arg("self"), arg("index")),
           "Returns the distance from the sample to the spectrum.")

      .def("hasDetectors", &SpectrumInfo::hasDetectors, (arg("self")),
           "Returns true if the spectrum is associated with detectors in the "
           "instrument.")

      .def("hasUniqueDetector", &SpectrumInfo::hasUniqueDetector,
           (arg("self"), arg("index")),
           "Returns true if the spectrum is associated with exactly one "
           "detector.")

      .def("position", &SpectrumInfo::position, (arg("self"), arg("index")),
           "Returns the absolute position of the spectrum with the given "
           "index.")

      .def("sourcePosition", &SpectrumInfo::sourcePosition, arg("self"),
           "Returns the absolute source position.")

      .def("samplePosition", &SpectrumInfo::samplePosition, arg("self"),
           "Returns the absolute sample position.")

      .def("getAllSpectrumDefinitions", &getSpectrumDefinitionList, arg("self"),
           "Returns the SpectrumDefinition of the spectrum.");
}
