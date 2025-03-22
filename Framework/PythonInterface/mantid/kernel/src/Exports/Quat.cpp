// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/Quat.h"
#include "MantidKernel/V3D.h"
#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/return_arg.hpp>
#include <boost/python/return_value_policy.hpp>

using boost::python::arg;
using boost::python::class_;
using boost::python::copy_const_reference;
using boost::python::init;
using boost::python::return_value_policy;
using Mantid::Kernel::Quat;
using Mantid::Kernel::V3D;

/**
 * Python exports of the Mantid::Kernel::Quat class.
 */
void export_Quat() {
  boost::python::register_ptr_to_python<std::shared_ptr<Quat>>();

  // Quat class
  class_<Quat>("Quat",
               "Quaternions are the 3D generalization of complex numbers. "
               "Quaternions are used for roations in 3D spaces and often "
               "implemented for "
               "computer graphics applications.",
               init<>(arg("self"), "Construct a default Quat that will perform no transformation."))
      .def(init<double, double, double, double>((arg("self"), arg("w"), arg("a"), arg("b"), arg("c")),
                                                "Constructor with values"))
      .def(init<V3D, V3D>((arg("self"), arg("src"), arg("dest")), "Construct a Quat between two vectors"))
      .def(init<V3D, V3D, V3D>((arg("self"), arg("rX"), arg("rY"), arg("rZ")),
                               "Construct a Quaternion that performs a "
                               "reference frame rotation.\nThe initial X,Y,Z "
                               "vectors are aligned as expected: X=(1,0,0), "
                               "Y=(0,1,0), Z=(0,0,1)"))
      .def(init<double, V3D>((arg("self"), arg("deg"), arg("axis")), "Constructor from an angle(degrees) and an axis."))
      .def("rotate", &Quat::rotate, (arg("self"), arg("v")), "Rotate the quaternion by the given vector")
      .def("real", &Quat::real, arg("self"), "Returns the real part of the quaternion")
      .def("imagI", &Quat::imagI, arg("self"), "Returns the ith imaginary component")
      .def("imagJ", &Quat::imagJ, arg("self"), "Returns the jth imaginary component")
      .def("imagK", &Quat::imagK, arg("self"), "Returns the kth imaginary component")
      .def("len", &Quat::len, arg("self"), "Returns the 'length' of the quaternion")
      .def("len2", &Quat::len2, arg("self"), "Returns the square of the 'length' of the quaternion")
      .def("getEulerAngles", &Quat::getEulerAngles, (arg("self"), arg("convention") = "YZX"),
           "Default convention is \'YZX\'.")
      // cppcheck-suppress syntaxError
      .def("__add__", &Quat::operator+, (arg("left"), arg("right")))
      .def("__iadd__", &Quat::operator+=, boost::python::return_self<>(), (arg("self"), arg("other")))
      .def("__sub__", &Quat::operator-, (arg("left"), arg("right")))
      .def("__isub__", &Quat::operator-=, boost::python::return_self<>(), (arg("self"), arg("other")))
      .def("__mul__", &Quat::operator*, (arg("left"), arg("right")))
      .def("__imul__", &Quat::operator*=, boost::python::return_self<>(), (arg("self"), arg("other")))
      .def("__eq__", &Quat::operator==, (arg("self"), arg("other")))
      .def("__ne__", &Quat::operator!=, (arg("self"), arg("other")))
      .def("__getitem__", (double (Quat::*)(int) const) & Quat::operator[], (arg("self"), arg("index")))
      .def("__str__", &Quat::toString, arg("self"));
  //.def(boost::python::self_ns::str(self));
}
