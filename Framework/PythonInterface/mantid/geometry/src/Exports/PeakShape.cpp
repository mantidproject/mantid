#include "MantidGeometry/Crystal/PeakShape.h"
#include "MantidPythonInterface/kernel/GetPointer.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Geometry::PeakShape;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(PeakShape)

void export_PeakShape() {
  register_ptr_to_python<Mantid::Geometry::PeakShape_sptr>();

  class_<PeakShape, boost::noncopyable>("PeakShape", no_init)
      .def("toJSON", &PeakShape::toJSON, arg("self"),
           "Serialize object to JSON")
      .def("shapeName", &PeakShape::shapeName, arg("self"),
           "Shape name for type of shape")
      .def("algorithmVersion", &PeakShape::algorithmVersion, arg("self"),
           "Number of source integration algorithm version")
      .def("algorithmName", &PeakShape::algorithmName, arg("self"),
           "Name of source integration algorithm");
}
