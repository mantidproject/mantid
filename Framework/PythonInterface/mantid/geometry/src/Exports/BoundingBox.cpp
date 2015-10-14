#include "MantidGeometry/Objects/BoundingBox.h"
#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>

using Mantid::Geometry::BoundingBox;
using Mantid::Kernel::V3D;
using namespace boost::python;

void export_BoundingBox() {
  class_<BoundingBox>("BoundingBox", "Constructs a zero-sized box")
      .def(init<double, double, double, double, double, double>(
          (arg("xmax"), arg("ymax"), arg("zmax"), arg("xmin"), arg("ymin"),
           arg("zmin")),
          "Constructs a box from the six given points"))

      .def("minPoint", &BoundingBox::minPoint,
           return_value_policy<copy_const_reference>(),
           "Returns a V3D containing the values of the minimum of the box. See "
           "mantid.kernel.V3D")

      .def("maxPoint", &BoundingBox::maxPoint,
           return_value_policy<copy_const_reference>(),
           "Returns a V3D containing the values of the minimum of the box. See "
           "mantid.kernel.V3D")

      .def("centrePoint", &BoundingBox::centrePoint,
           "Returns a V3D containing the coordinates of the centre point. See "
           "mantid.kernel.V3D")

      .def("width", &BoundingBox::width, "Returns a V3D containing the widths "
                                         "for each dimension. See "
                                         "mantid.kernel.V3D")

      .def("isNull", &BoundingBox::isNull,
           "Returns true if the box has no dimensions that have been set")

      .def("isPointInside", &BoundingBox::isPointInside,
           "Returns true if the given point is inside the object. See "
           "mantid.kernel.V3D")

      .def("doesLineIntersect",
           (bool (BoundingBox::*)(const V3D &, const V3D &) const) &
               BoundingBox::doesLineIntersect,
           (arg("startPoint"), arg("lineDir")),
           "Returns true if the line given by the starting point & direction "
           "vector passes through the box");
}
