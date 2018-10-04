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
  class_<DetectorInfoItem>("DetectorInfoItem", no_init)
      .add_property("isMonitor", &DetectorInfoItem::isMonitor)
      .add_property("isMasked", &DetectorInfoItem::isMasked)
      .add_property("twoTheta", &DetectorInfoItem::twoTheta)
      .add_property("position", &DetectorInfoItem::position)
      .add_property("rotation", &DetectorInfoItem::rotation);
}
