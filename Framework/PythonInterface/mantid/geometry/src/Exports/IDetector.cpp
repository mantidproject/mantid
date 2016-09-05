#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidGeometry/IDetector.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Geometry::IDetector;
using Mantid::Geometry::IObjComponent;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(IDetector)

void export_IDetector() {
  register_ptr_to_python<boost::shared_ptr<IDetector>>();

  class_<IDetector, bases<IObjComponent>, boost::noncopyable>("IDetector",
                                                              no_init)
      .def("getID", &IDetector::getID, arg("self"), "Returns the detector ID")
      .def("isMasked", &IDetector::isMasked, arg("self"),
           "Returns the value of the masked flag. True means ignore this "
           "detector")
      .def("isMonitor", &IDetector::isMonitor, arg("self"),
           "Returns True if the detector is marked as a monitor in the IDF")
      .def("solidAngle", &IDetector::solidAngle, (arg("self"), arg("observer")),
           "Return the solid angle in steradians between this "
           "detector and an observer")
      .def("getTwoTheta", &IDetector::getTwoTheta,
           (arg("self"), arg("observer"), arg("axis")),
           "Calculate the angle between this detector, another component and "
           "an axis")
      .def("getPhi", &IDetector::getPhi, arg("self"),
           "Returns the azimuthal angle of this detector");
}
