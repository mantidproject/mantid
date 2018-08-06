#include <boost/python/class.hpp>
#include <boost/python/module.hpp>

#include "MantidGeometry/Instrument/DetectorInfoItem.h"
#include "MantidKernel/V3D.h"

using Mantid::Geometry::DetectorInfoItem;
using Mantid::Kernel::V3D;
using namespace boost::python;

void export_DetectorInfoItem() {

  // Export to Python
  class_<DetectorInfoItem>("DetectorInfoItem", no_init)
      .add_property("position", &DetectorInfoItem::position);
  //.def("position", &DetectorInfoItem::position, arg("self"));
}
