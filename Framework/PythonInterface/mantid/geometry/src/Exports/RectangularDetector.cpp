// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using namespace Mantid::Geometry;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(RectangularDetector)

/**
 * Enables boost.python to automatically "cast" an object up to the
 * appropriate Detector leaf type
 */
void export_RectangularDetector() {
  register_ptr_to_python<boost::shared_ptr<RectangularDetector>>();

  class_<RectangularDetector, bases<CompAssembly, IObjComponent>,
         boost::noncopyable>("RectangularDetector", no_init)
      .def("xpixels", &RectangularDetector::xpixels, arg("self"),
           "Returns the number of pixels in the X direction")
      .def("ypixels", &RectangularDetector::ypixels, arg("self"),
           "Returns the number of pixels in the Y direction")
      .def("xstep", &RectangularDetector::xstep, arg("self"),
           "Returns the step size in the X direction")
      .def("ystep", &RectangularDetector::ystep, arg("self"),
           "Returns the step size in the Y direction")
      .def("xsize", &RectangularDetector::xsize, arg("self"),
           "Returns the size in the X direction")
      .def("ysize", &RectangularDetector::ysize, arg("self"),
           "Returns the size in the Y direction")
      .def("xstart", &RectangularDetector::xstart, arg("self"),
           "Returns the start position in the X direction")
      .def("ystart", &RectangularDetector::ystart, arg("self"),
           "Returns the start position in the Y direction")
      .def("idstart", &RectangularDetector::idstart, arg("self"),
           "Returns the idstart")
      .def("idfillbyfirst_y", &RectangularDetector::idfillbyfirst_y,
           arg("self"), "Returns the idfillbyfirst_y")
      .def("idstepbyrow", &RectangularDetector::idstepbyrow, arg("self"),
           "Returns the idstepbyrow")
      .def("idstep", &RectangularDetector::idstep, arg("self"),
           "Returns the idstep")
      .def("minDetectorID", &RectangularDetector::minDetectorID, arg("self"),
           "Returns the minimum detector id")
      .def("maxDetectorID", &RectangularDetector::maxDetectorID, arg("self"),
           "Returns the maximum detector id");
}
