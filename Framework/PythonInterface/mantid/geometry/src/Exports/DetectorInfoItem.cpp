// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument/DetectorInfoItem.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/V3D.h"

#include <boost/python/class.hpp>
#include <boost/python/module.hpp>

using Mantid::Geometry::DetectorInfo;
using Mantid::Geometry::DetectorInfoItem;
using Mantid::Kernel::V3D;
using namespace boost::python;

// Export DetectorInfoItem
void export_DetectorInfoItem() {

  // Export to Python
  class_<DetectorInfoItem<DetectorInfo>>("DetectorInfoItem", no_init)
      .add_property("isMonitor", &DetectorInfoItem<DetectorInfo>::isMonitor)
      .add_property("isMasked", &DetectorInfoItem<DetectorInfo>::isMasked)
      .add_property("twoTheta", &DetectorInfoItem<DetectorInfo>::twoTheta)
      .add_property("position", &DetectorInfoItem<DetectorInfo>::position)
      .add_property("rotation", &DetectorInfoItem<DetectorInfo>::rotation)
      .add_property("l2", &DetectorInfoItem<DetectorInfo>::l2)
      .add_property("index", &DetectorInfoItem<DetectorInfo>::index)
      .def("setMasked", &DetectorInfoItem<DetectorInfo>::setMasked,
           (arg("self"), arg("masked")), "Set the mask flag for the detector");
}
