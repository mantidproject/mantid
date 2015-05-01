#include "MantidGeometry/Instrument/ReferenceFrame.h"

#include <boost/shared_ptr.hpp>
#include <boost/python/class.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Geometry::ReferenceFrame;
using Mantid::Kernel::V3D;
using namespace boost::python;

// clang-format off
void export_ReferenceFrame()
// clang-format on
{
  using namespace Mantid::Geometry;

    register_ptr_to_python<boost::shared_ptr<ReferenceFrame>>();

    enum_<PointingAlong>("PointingAlong")
       .value("X", X)
       .value("Y", Y)
       .value("Z", Z)
       .export_values();  

    class_< ReferenceFrame, boost::noncopyable>("ReferenceFrame", no_init)
      .def( "pointingAlongBeam", &ReferenceFrame::pointingAlongBeam)
      .def( "pointingUp", &ReferenceFrame::pointingUp)
      .def( "vecPointingUp", &ReferenceFrame::vecPointingUp )
      .def( "vecPointingAlongBeam", &ReferenceFrame::vecPointingAlongBeam )
      .def( "pointingAlongBeamAxis", &ReferenceFrame::pointingAlongBeamAxis )
      .def( "pointingUpAxis", &ReferenceFrame::pointingUpAxis )
      .def( "pointingHorizontalAxis", &ReferenceFrame::pointingHorizontalAxis )


	;
}

