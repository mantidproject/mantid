#include "MantidGeometry/Instrument/DetectorGroup.h"
#include <boost/python/class.hpp>

using Mantid::Geometry::DetectorGroup;
using Mantid::Geometry::IDetector;
using namespace boost::python;

void export_DetectorGroup()
{
  class_<DetectorGroup, bases<IDetector>, boost::noncopyable>("DetectorGroup", no_init)
    .def("getDetectorIDs", &DetectorGroup::getDetectorIDs, "Returns the list of detector IDs within this group")
    ;
}

