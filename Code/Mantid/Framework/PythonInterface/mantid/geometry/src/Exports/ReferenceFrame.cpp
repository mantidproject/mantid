#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include <boost/shared_ptr.hpp>
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/enum.hpp>

using Mantid::Geometry::ReferenceFrame;
using namespace boost::python;

void export_ReferenceFrame()
{
  using namespace Mantid::Geometry;

    register_ptr_to_python<boost::shared_ptr<const ReferenceFrame> >();

    enum_<PointingAlong>("PointingAlong")
       .value("X", X)
       .value("Y", Y)
       .value("Z", Z)
       .export_values();  

    class_< ReferenceFrame, boost::noncopyable>("ReferenceFrame", no_init)
      .def( "pointingAlongBeam", &ReferenceFrame::pointingAlongBeam)
      .def( "pointingUp", &ReferenceFrame::pointingUp)
	;
}

