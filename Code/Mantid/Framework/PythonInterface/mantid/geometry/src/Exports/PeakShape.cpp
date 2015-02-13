#include "MantidGeometry/Crystal/PeakShape.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Geometry::PeakShape;
using namespace boost::python;


void export_PeakShape()
{
  register_ptr_to_python<Mantid::Geometry::PeakShape_sptr>();

  class_<PeakShape, boost::noncopyable>("PeakShape", no_init)
    .def("toJSON", &PeakShape::toJSON, "Serialize object to JSON")
    .def("shapeName", &PeakShape::shapeName, "Shape name for type of shape")
    .def("algorithmVersion", &PeakShape::algorithmVersion, "Number of source integration algorithm version")
    .def("algorithmName", &PeakShape::algorithmName, "Name of source integration algorithm")
    ;
}
