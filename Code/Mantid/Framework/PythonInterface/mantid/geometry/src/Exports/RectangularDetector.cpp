#include "MantidGeometry/Instrument/RectangularDetector.h"
#include <boost/python/class.hpp>
#include "MantidPythonInterface/kernel/SharedPtrToPythonMacro.h"
#include "MantidGeometry/Instrument/CompAssembly.h"

using Mantid::Geometry::CompAssembly;
using Mantid::Geometry::RectangularDetector;
using Mantid::Geometry::IDetector;
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
    ;
}

