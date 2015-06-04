#include "MantidKernel/Quat.h"
#include "MantidKernel/V3D.h"
#include <boost/python/class.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/return_value_policy.hpp>

using Mantid::Kernel::Quat;
using Mantid::Kernel::V3D;

using boost::python::args;
using boost::python::init;
using boost::python::class_;
using boost::python::self;
using boost::python::copy_const_reference;
using boost::python::return_value_policy;

/**
 * Python exports of the Mantid::Kernel::Quat class.
 */
void export_Quat() {
  boost::python::register_ptr_to_python<boost::shared_ptr<Quat>>();

  // Quat class
  class_<Quat>(
      "Quat", "Quaternions are the 3D generalization of complex numbers. "
              "Quaternions are used for roations in 3D spaces and often "
              "implemented for "
              "computer graphics applications.",
      init<>(args("self"),
             "Construct a default Quat that will perform no transformation."))
      .def(init<double, double, double, double>(
          args("self", "w", "a", "b", "c"), "Constructor with values"))
      .def(init<V3D, V3D>(args("self", "src", "dest"),
                          "Construct a Quat between two vectors"))
      .def(init<V3D, V3D, V3D>(args("self", "rX", "rY", "rZ"),
                               "Construct a Quaternion that performs a "
                               "reference frame rotation.\nThe initial X,Y,Z "
                               "vectors are aligned as expected: X=(1,0,0), "
                               "Y=(0,1,0), Z=(0,0,1)"))
      .def(init<double, V3D>(args("self", "deg", "axis"),
                             "Constructor from an angle(degrees) and an axis."))
      .def("rotate", &Quat::rotate, args("self", "v"),
           "Rotate the quaternion by the given vector")
      .def("real", &Quat::real, args("self"),
           "Returns the real part of the quaternion")
      .def("imagI", &Quat::imagI, args("self"),
           "Returns the ith imaginary component")
      .def("imagJ", &Quat::imagJ, args("self"),
           "Returns the jth imaginary component")
      .def("imagK", &Quat::imagK, args("self"),
           "Returns the kth imaginary component")
      .def("len", &Quat::len, args("self"),
           "Returns the 'length' of the quaternion")
      .def("len2", &Quat::len2, args("self"),
           "Returns the square of the 'length' of the quaternion")
      .def(self + self)
      .def(self += self)
      .def(self - self)
      .def(self -= self)
      .def(self * self)
      .def(self *= self)
      .def(self == self)
      .def(self != self)
      .def("__getitem__",
           (const double &(Quat::*)(int) const) & Quat::operator[],
           return_value_policy<copy_const_reference>())
      .def(boost::python::self_ns::str(self));
}
