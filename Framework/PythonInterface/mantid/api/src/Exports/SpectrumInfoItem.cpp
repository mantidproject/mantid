// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/SpectrumInfoItem.h"
#include "MantidKernel/V3D.h"

#include <boost/python/class.hpp>
#include <boost/python/module.hpp>

using Mantid::API::SpectrumInfoItem;
using Mantid::Kernel::V3D;
using namespace boost::python;

// Export SpectrumInfoItem
void export_SpectrumInfoItem() {

  // Export to Python
  class_<SpectrumInfoItem>("SpectrumInfoItem", no_init)
      .add_property("isMonitor", &SpectrumInfoItem::isMonitor)
      .add_property("isMasked", &SpectrumInfoItem::isMasked)
      .add_property("twoTheta", &SpectrumInfoItem::twoTheta)
      .add_property("signedTwoTheta", &SpectrumInfoItem::signedTwoTheta)
      .add_property("l2", &SpectrumInfoItem::l2)
      .add_property("hasUniqueDetector", &SpectrumInfoItem::hasUniqueDetector)
      .add_property("spectrumDefinition",
                    make_function(&SpectrumInfoItem::spectrumDefinition,
                                  return_internal_reference<>()))
      .add_property("position", &SpectrumInfoItem::position);
}
