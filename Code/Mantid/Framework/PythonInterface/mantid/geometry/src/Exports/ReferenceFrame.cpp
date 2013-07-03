#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidPythonInterface/kernel/SharedPtrToPythonMacro.h"

#include <boost/shared_ptr.hpp>
#include <boost/python/class.hpp>
#include <boost/python/enum.hpp>

using Mantid::Geometry::ReferenceFrame;
using Mantid::Kernel::V3D;
using namespace boost::python;

void export_ReferenceFrame()
{
  using namespace Mantid::Geometry;

    REGISTER_SHARED_PTR_TO_PYTHON(ReferenceFrame);

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


	;
}

