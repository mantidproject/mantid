#include "MantidKernel/V3D.h"
#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/operators.hpp>

using namespace boost::python;
using Mantid::Kernel::V3D;

namespace
{

  /**
   * Helper forwarding method for directionAngles to allow default argument values.
   * @param self
   * @return
   */
  V3D directionAnglesDefault(V3D & self)
  {
    return self.directionAngles();
  }

  long hashV3D(V3D & self)
  {
      boost::python::object tmpObj(self.toString());

      return PyObject_Hash(tmpObj.ptr());
  }

}


void export_V3D()
{
  //V3D class
  class_< V3D >("V3D",init<>("Construct a V3D at the origin"))
    .def(init<double, double, double>("Construct a V3D with X,Y,Z coordinates"))
    .def("X", &V3D::X, return_value_policy< copy_const_reference >(), "Returns the X coordinate")
    .def("Y", &V3D::Y, return_value_policy< copy_const_reference >(), "Returns the Y coordinate")
    .def("Z", &V3D::Z, return_value_policy< copy_const_reference >(), "Returns the Z coordinate")
    .def("getX", &V3D::X, return_value_policy< copy_const_reference >(), "Returns the X coordinate") // Traditional name
    .def("getY", &V3D::Y, return_value_policy< copy_const_reference >(), "Returns the Y coordinate") // Traditional name
    .def("getZ", &V3D::Z, return_value_policy< copy_const_reference >(), "Returns the Z coordinate") // Traditional name
    .def("distance", &V3D::distance, "Returns the distance between this vector and another")
    .def("angle", &V3D::angle, "Returns the angle between this vector and another")
    .def("zenith", &V3D::zenith, "Returns the zenith between this vector and another")
    .def("scalar_prod", &V3D::scalar_prod, "Computes the scalar product between this and another vector")
    .def("cross_prod", &V3D::cross_prod, "Computes the cross product between this and another vector")
    .def("norm", &V3D::norm, "Calculates the length of the vector")
    .def("norm2", &V3D::norm2, "Calculates the squared length of the vector")
    .def(self + self)
    .def(self += self)
    .def(self - self)
    .def(self -= self)
    .def(self * self)
    .def(self *= self)
    .def(self / self)
    .def(self /= self)
    .def(self * int())
    .def(self *= int())
    .def(self * double())
    .def(self *= double())
    // cppcheck-suppress duplicateExpression
    .def(self < self)
    .def(self == self)
    .def(self != self) // must define != as Python's default is to compare object address
    .def(self_ns::str(self))
    .def(self_ns::repr(self))
    .def("__hash__", &hashV3D)
    .def("directionAngles", &V3D::directionAngles, "Calculate direction angles from direction cosines")
    .def("directionAngles", &directionAnglesDefault, "Calculate direction angles from direction cosines")
    ;
}
