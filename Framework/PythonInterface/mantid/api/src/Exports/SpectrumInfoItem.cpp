// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/SpectrumInfoItem.h"
#include "MantidKernel/V3D.h"

#include "MantidAPI/SpectrumInfo.h"
#include <boost/python/class.hpp>
#include <boost/python/module.hpp>

using Mantid::API::SpectrumInfo;
using Mantid::API::SpectrumInfoItem;
using Mantid::Kernel::V3D;
using namespace boost::python;

// Export SpectrumInfoItem
void export_SpectrumInfoItem() {

  // Export to Python
  class_<SpectrumInfoItem<SpectrumInfo>>("SpectrumInfoItem", no_init)
      .add_property("isMonitor", &SpectrumInfoItem<SpectrumInfo>::isMonitor)
      .add_property("isMasked", &SpectrumInfoItem<SpectrumInfo>::isMasked)
      .add_property("twoTheta", &SpectrumInfoItem<SpectrumInfo>::twoTheta)
      .add_property("signedTwoTheta", &SpectrumInfoItem<SpectrumInfo>::signedTwoTheta)
      .add_property("l2", &SpectrumInfoItem<SpectrumInfo>::l2)
      .add_property("hasDetectors", &SpectrumInfoItem<SpectrumInfo>::hasDetectors)
      .add_property("hasUniqueDetector", &SpectrumInfoItem<SpectrumInfo>::hasUniqueDetector)
      .add_property("spectrumDefinition",
                    make_function(&SpectrumInfoItem<SpectrumInfo>::spectrumDefinition, return_internal_reference<>()))
      .add_property("position", &SpectrumInfoItem<SpectrumInfo>::position)
      .def("setMasked", &SpectrumInfoItem<SpectrumInfo>::setMasked, (arg("self"), arg("masked")),
           "Set the mask flag for the spectrum");
}
