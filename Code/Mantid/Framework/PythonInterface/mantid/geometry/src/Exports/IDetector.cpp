#include "MantidGeometry/IDetector.h"
#include "MantidPythonInterface/kernel/SharedPtrToPythonMacro.h"
#include <boost/python/class.hpp>

using Mantid::Geometry::IDetector;
using Mantid::Geometry::IObjComponent;
using namespace boost::python;

void export_IDetector()
{
  REGISTER_SHARED_PTR_TO_PYTHON(IDetector);
  
  class_<IDetector, bases<IObjComponent>, boost::noncopyable>("IDetector", no_init)
    .def("getID", &IDetector::getID, "Returns the detector ID")
    .def("isMasked", &IDetector::isMasked, "Returns the value of the masked flag. True means ignore this detector")
    .def("isMonitor", &IDetector::isMonitor, "Returns True if the detector is marked as a monitor in the IDF")
    .def("solidAngle", &IDetector::solidAngle, "Return the solid angle in steradians between this detector and an observer")
    .def("getTwoTheta", &IDetector::getTwoTheta, "Calculate the angle between this detector, another component and an axis")
    .def("getPhi", &IDetector::getPhi, "Returns the azimuthal angle of this detector")
  ;
}

