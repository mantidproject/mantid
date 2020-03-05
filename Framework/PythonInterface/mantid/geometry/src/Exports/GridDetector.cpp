#include "MantidGeometry/Instrument/GridDetector.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/GridDetectorPixel.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using namespace Mantid::Geometry;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(GridDetector)

/**
 * Enables boost.python to automatically "cast" an object up to the
 * appropriate Detector leaf type
 */
void export_GridDetector() {
  register_ptr_to_python<boost::shared_ptr<GridDetector>>();

  class_<GridDetector, bases<CompAssembly, IObjComponent>, boost::noncopyable>(
      "GridDetector", no_init)
      .def("xpixels", &GridDetector::xpixels, arg("self"),
           "Returns the number of pixels in the X direction")
      .def("ypixels", &GridDetector::ypixels, arg("self"),
           "Returns the number of pixels in the Y direction")
      .def("zpixels", &GridDetector::zpixels, arg("self"),
           "Returns the number of pixels in the Z direction")
      .def("xstep", &GridDetector::xstep, arg("self"),
           "Returns the step size in the X direction")
      .def("ystep", &GridDetector::ystep, arg("self"),
           "Returns the step size in the Y direction")
      .def("zstep", &GridDetector::zstep, arg("self"),
           "Returns the step size in the Z direction")
      .def("xsize", &GridDetector::xsize, arg("self"),
           "Returns the size in the X direction")
      .def("ysize", &GridDetector::ysize, arg("self"),
           "Returns the size in the Y direction")
      .def("zsize", &GridDetector::zsize, arg("self"),
           "Returns the size in the Z direction")
      .def("xstart", &GridDetector::xstart, arg("self"),
           "Returns the start position in the X direction")
      .def("ystart", &GridDetector::ystart, arg("self"),
           "Returns the start position in the Y direction")
      .def("zstart", &GridDetector::zstart, arg("self"),
           "Returns the start position in the Z direction")
      .def("idstart", &GridDetector::idstart, arg("self"),
           "Returns the idstart")
      .def("idFillOrder", &GridDetector::idFillOrder, arg("self"),
           "Returns the idFillOrder")
      .def("idstepbyrow", &GridDetector::idstepbyrow, arg("self"),
           "Returns the idstepbyrow")
      .def("idstep", &GridDetector::idstep, arg("self"), "Returns the idstep")
      .def("minDetectorID", &GridDetector::minDetectorID, arg("self"),
           "Returns the minimum detector id")
      .def("maxDetectorID", &GridDetector::maxDetectorID, arg("self"),
           "Returns the maximum detector id");
}

void export_GridDetectorPixel() {
  class_<GridDetectorPixel, bases<Detector>, boost::noncopyable>(
      "GridDetectorPixel", no_init);
}
