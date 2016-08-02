#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"

#include <boost/shared_ptr.hpp>
#include <boost/python/class.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Geometry::ReferenceFrame;
using Mantid::Kernel::V3D;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(ReferenceFrame)

void export_ReferenceFrame() {
  using namespace Mantid::Geometry;

  register_ptr_to_python<boost::shared_ptr<ReferenceFrame>>();

  enum_<PointingAlong>("PointingAlong")
      .value("X", X)
      .value("Y", Y)
      .value("Z", Z)
      .export_values();

  class_<ReferenceFrame, boost::noncopyable>("ReferenceFrame", no_init)
      .def("pointingAlongBeam", &ReferenceFrame::pointingAlongBeam, arg("self"))
      .def("pointingUp", &ReferenceFrame::pointingUp, arg("self"))
      .def("vecPointingUp", &ReferenceFrame::vecPointingUp, arg("self"))
      .def("vecPointingAlongBeam", &ReferenceFrame::vecPointingAlongBeam,
           arg("self"))
      .def("pointingAlongBeamAxis", &ReferenceFrame::pointingAlongBeamAxis,
           arg("self"))
      .def("pointingUpAxis", &ReferenceFrame::pointingUpAxis, arg("self"))
      .def("pointingHorizontalAxis", &ReferenceFrame::pointingHorizontalAxis,
           arg("self"));
}
