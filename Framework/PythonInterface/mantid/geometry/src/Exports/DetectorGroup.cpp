#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include <boost/python/class.hpp>

using Mantid::Geometry::DetectorGroup;
using Mantid::Geometry::IDetector;
using namespace boost::python;

void export_DetectorGroup() {
  class_<DetectorGroup, bases<IDetector>, boost::noncopyable>("DetectorGroup",
                                                              no_init)
      .def("getDetectorIDs", &DetectorGroup::getDetectorIDs, arg("self"),
           "Returns the list of detector IDs within this group")
      .def("getNameSeparator", &DetectorGroup::getNameSeparator, arg("self"),
           "Returns separator for list of names of detectors");
}
