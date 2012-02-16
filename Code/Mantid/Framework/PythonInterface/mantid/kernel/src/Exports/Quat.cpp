#include "MantidKernel/Quat.h"
#include "MantidKernel/V3D.h"
#include <boost/python/class.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/python/copy_const_reference.hpp>

using Mantid::Kernel::Quat;
using Mantid::Kernel::V3D;

using boost::python::init;
using boost::python::class_;
using boost::python::self;
using boost::python::copy_const_reference;
using boost::python::return_value_policy;

void export_Quat()
{
  //Quat class
  class_< Quat >("Quat", init<>("Construct a default Quat that will perform no transformation."))
    .def(init<double, double, double, double>("Constructor with values"))
    .def(init<V3D, V3D>("Construct a Quat between two vectors"))
    .def(init<V3D, V3D, V3D>("Construct a Quaternion that performs a reference frame rotation.\nThe initial X,Y,Z vectors are aligned as expected: X=(1,0,0), Y=(0,1,0), Z=(0,0,1)"))
    .def(init<double,V3D>("Constructor from an angle(degrees) and an axis."))
    .def("rotate", &Quat::rotate, "Rotate the quaternion by the given vector")
    .def("real", &Quat::real, "Returns the real part of the quaternion")
    .def("imagI", &Quat::imagI, "Returns the ith imaginary component")
    .def("imagJ", &Quat::imagJ, "Returns the jth imaginary component")
    .def("imagK", &Quat::imagK, "Returns the kth imaginary component")
    .def("len", &Quat::len, "Returns the 'length' of the quaternion")
    .def("len2", &Quat::len2, "Returns the square of the 'length' of the quaternion")
    .def(self + self)
    .def(self += self)
      // cppcheck-suppress duplicateExpression
    .def(self - self)
    .def(self -= self)
    .def(self * self)
    .def(self *= self)
      // cppcheck-suppress duplicateExpression
    .def(self == self)
      // cppcheck-suppress duplicateExpression
    .def(self != self)
    .def("__getitem__", (const double&(Quat::*)(int) const)&Quat::operator[], return_value_policy<copy_const_reference>())
    .def(boost::python::self_ns::str(self))
    ;

}
