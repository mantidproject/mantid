#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Instrument/RectangularDetectorPixel.h"
#include <boost/python/class.hpp>
#include "MantidPythonInterface/kernel/SharedPtrToPythonMacro.h"
#include "MantidGeometry/Instrument/CompAssembly.h"

using Mantid::Geometry::CompAssembly;
using Mantid::Geometry::RectangularDetector;
using Mantid::Geometry::RectangularDetectorPixel;
using Mantid::Geometry::Detector;
using Mantid::Geometry::IObjComponent;
using namespace boost::python;

/**
 * Enables boost.python to automatically "cast" an object up to the
 * appropriate Detector leaf type 
 */
void export_RectangularDetector()
{
  REGISTER_SHARED_PTR_TO_PYTHON(RectangularDetector);
  class_<RectangularDetector, bases< CompAssembly, IObjComponent>, boost::noncopyable>("RectangularDetector", no_init)
          .def("xpixels",&RectangularDetector::xpixels, "Returns the number of pixels in the X direction")
          .def("ypixels",&RectangularDetector::ypixels, "Returns the number of pixels in the Y direction")
          .def("xstep",&RectangularDetector::xstep, "Returns the step size in the X direction")
          .def("ystep",&RectangularDetector::ystep, "Returns the step size in the Y direction")
          .def("xsize",&RectangularDetector::xsize, "Returns the size in the X direction")
          .def("ysize",&RectangularDetector::ysize, "Returns the size in the Y direction")
          .def("xstart",&RectangularDetector::xstart, "Returns the start position in the X direction")
          .def("ystart",&RectangularDetector::ystart, "Returns the start position in the Y direction")
          .def("idstart",&RectangularDetector::idstart, "Returns the idstart")
          .def("idfillbyfirst_y",&RectangularDetector::idfillbyfirst_y, "Returns the idfillbyfirst_y")
          .def("idstepbyrow",&RectangularDetector::idstepbyrow, "Returns the idstepbyrow")
          .def("idstep",&RectangularDetector::idstep, "Returns the idstep")
          .def("minDetectorID",&RectangularDetector::minDetectorID, "Returns the minimum detector id")
          .def("maxDetectorID",&RectangularDetector::maxDetectorID, "Returns the maximum detector id")
    ;
}

void export_RectangularDetectorPixel()
{
  class_<RectangularDetectorPixel, bases<Detector>, boost::noncopyable>("RectangularDetectorPixel", no_init)
    ;
}
