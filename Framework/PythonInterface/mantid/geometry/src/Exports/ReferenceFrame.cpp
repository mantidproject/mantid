// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidPythonInterface/core/GetPointer.h"

#include <boost/python/class.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/shared_ptr.hpp>

using Mantid::Geometry::ReferenceFrame;
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
