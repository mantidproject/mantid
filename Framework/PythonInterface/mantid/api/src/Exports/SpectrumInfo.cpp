// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/SpectrumInfoItem.h"
#include "MantidAPI/SpectrumInfoIterator.h"
#include "MantidPythonInterface/api/SpectrumInfoPythonIterator.h"
#include "MantidTypes/SpectrumDefinition.h"

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/list.hpp>
#include <boost/python/return_value_policy.hpp>

using Mantid::API::SpectrumInfo;
using Mantid::API::SpectrumInfoItem;
using Mantid::API::SpectrumInfoIterator;
using Mantid::PythonInterface::SpectrumInfoPythonIterator;
using Mantid::SpectrumDefinition;
using namespace boost::python;

// Helper method to make the python iterator
SpectrumInfoPythonIterator make_pyiterator(SpectrumInfo &spectrumInfo) {
  return SpectrumInfoPythonIterator(spectrumInfo);
}

// Export SpectrumInfo
void export_SpectrumInfo() {
  class_<SpectrumInfo, boost::noncopyable>("SpectrumInfo", no_init)
      .def("__iter__", make_pyiterator)
      .def("__len__", &SpectrumInfo::size, arg("self"),
           "Returns the number of spectra.")
      .def("size", &SpectrumInfo::size, arg("self"),
           "Returns the number of spectra.")
      .def("isMonitor", &SpectrumInfo::isMonitor, (arg("self"), arg("index")),
           "Returns True if the detector(s) associated with the spectrum are "
           "monitors.")
      .def("isMasked", &SpectrumInfo::isMasked, (arg("self"), arg("index")),
           "Returns True if the detector(s) associated with the spectrum are "
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
      .def("azimuthal", &SpectrumInfo::azimuthal, (arg("self"), arg("index")),
           "Returns the out-of-plane angle in radians angle w.r.t. to "
           "vecPointingHorizontal "
           "direction.")
      .def("l1", &SpectrumInfo::l1, arg("self"),
           "Returns the distance from the source to the sample.")
      .def("l2", &SpectrumInfo::l2, (arg("self"), arg("index")),
           "Returns the distance from the sample to the spectrum.")
      .def("hasDetectors", &SpectrumInfo::hasDetectors, (arg("self")),
           "Returns True if the spectrum is associated with detectors in the "
           "instrument.")
      .def("hasUniqueDetector", &SpectrumInfo::hasUniqueDetector,
           (arg("self"), arg("index")),
           "Returns True if the spectrum is associated with exactly one "
           "detector.")
      .def("position", &SpectrumInfo::position, (arg("self"), arg("index")),
           "Returns the absolute position of the spectrum with the given "
           "index.")
      .def("sourcePosition", &SpectrumInfo::sourcePosition, arg("self"),
           "Returns the absolute source position.")
      .def("samplePosition", &SpectrumInfo::samplePosition, arg("self"),
           "Returns the absolute sample position.")
      .def("getSpectrumDefinition", &SpectrumInfo::spectrumDefinition,
           return_value_policy<return_by_value>(), (arg("self"), arg("index")),
           "Returns the SpectrumDefinition of the spectrum with the given "
           "index.");
}
