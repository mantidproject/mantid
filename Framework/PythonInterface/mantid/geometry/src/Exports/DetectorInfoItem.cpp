// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument/DetectorInfoItem.h"
#include "MantidKernel/V3D.h"

#include <boost/python/class.hpp>
#include <boost/python/module.hpp>

using Mantid::Geometry::DetectorInfoItem;
using Mantid::Kernel::V3D;
using namespace boost::python;

// Export DetectorInfoItem
void export_DetectorInfoItem() {

  // Export to Python
  class_<DetectorInfoItem, boost::noncopyable>("DetectorInfoItem", no_init)
      .add_property("isMonitor", &DetectorInfoItem::isMonitor)
      .add_property("isMasked", &DetectorInfoItem::isMasked)
      .add_property("twoTheta", &DetectorInfoItem::twoTheta)
      .add_property("position", &DetectorInfoItem::position)
      .add_property("rotation", &DetectorInfoItem::rotation);
}
