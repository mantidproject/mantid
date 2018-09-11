#include "MantidKernel/VMD.h"
#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/return_arg.hpp>
#include <boost/python/return_internal_reference.hpp>
#include <boost/python/return_value_policy.hpp>

using Mantid::Kernel::VMD;
using Mantid::Kernel::VMD_t;
using namespace boost::python;

namespace {
/**
 * Safe operator access. Returns the value at the given index
 * checking whether the index is valid. VMD does no checking
 * @param self The calling python object
 * @param index An index whose value is to be returned
 * @throws An out_of_range error if the index is out of range
 */
VMD_t getItem(VMD &self, const size_t index) {
  if (index < self.getNumDims()) {
    return self[index];
  } else
    throw std::out_of_range(
        "VMD index out of range. index=" + std::to_string(index) +
        ", len=" + std::to_string(self.getNumDims()));
}

/**
 * Set the value at the given index
 * @param self The calling python object
 * @param index An index whose value is to be set
 * @param value The new value for the index
 * @throws An out_of_range error if the index is out of range
 */
void setItem(VMD &self, const size_t index, const VMD_t value) {
  if (index < self.getNumDims()) {
    self[index] = value;
  } else
    throw std::out_of_range(
        "VMD index out of range. index=" + std::to_string(index) +
        ", len=" + std::to_string(self.getNumDims()));
}
} // namespace

void export_VMD() {
  class_<VMD>("VMD",
              init<>(arg("self"),
                     "Default constructor gives an object with 1 dimension"))
      .def(init<VMD_t, VMD_t>(
          "Constructs a 2 dimensional vector at the point given",
          (arg("self"), arg("val0"), arg("val1"))))
      .def(init<VMD_t, VMD_t, VMD_t>(
          "Constructs a 3 dimensional vector at the point given",
          (arg("self"), arg("val0"), arg("val1"), arg("val2"))))
      .def(init<VMD_t, VMD_t, VMD_t, VMD_t>(
          "Constructs a 4 dimensional vector at the point given",
          (arg("self"), arg("val0"), arg("val1"), arg("val2"), arg("val3"))))
      .def(init<VMD_t, VMD_t, VMD_t, VMD_t, VMD_t>(
          "Constructs a 5 dimensional vector at the point given",
          (arg("self"), arg("val0"), arg("val1"), arg("val2"), arg("val3"),
           arg("val4"))))
      .def(init<VMD_t, VMD_t, VMD_t, VMD_t, VMD_t, VMD_t>(
          "Constructs a 6 dimensional vector at the point given",
          (arg("self"), arg("val0"), arg("val1"), arg("val2"), arg("val3"),
           arg("val4"), arg("val5"))))

      .def("getNumDims", &VMD::getNumDims, arg("self"),
           "Returns the number of dimensions the contained in the vector")

      .def("scalar_prod", &VMD::scalar_prod, (arg("self"), arg("other")),
           "Returns the scalar product of this vector with another. If the "
           "number of dimensions do not match a RuntimeError is raised")

      .def("cross_prod", &VMD::cross_prod, (arg("self"), arg("other")),
           "Returns the cross product of this vector with another. If the "
           "number of dimensions do not match a RuntimeError is raised")

      .def("norm", &VMD::norm, arg("self"), "Returns the length of the vector")

      .def("norm2", &VMD::norm2, arg("self"),
           "Returns the the squared length of the vector")

      .def("normalize", &VMD::normalize, arg("self"),
           "Normalizes the length of the vector "
           "to unity and returns the length "
           "before it was normalized")

      .def("angle", &VMD::angle, (arg("self"), arg("other")),
           "Returns the angle between the vectors in "
           "radians (0 < theta < pi). If the dimensions "
           "do not match a RuntimeError is raised")

      //----------------------------- special methods
      //--------------------------------
      .def("__getitem__", &getItem, (arg("self"), arg("value")))
      .def("__setitem__", &setItem, (arg("self"), arg("index"), arg("value")))
      .def(self == self)
      .def(self != self) // must define != as Python's default is to compare
                         // object address
      .def("__add__", &VMD::operator+,(arg("left"), arg("right")))
      .def("__iadd__", &VMD::operator+=, return_self<>(),
           (arg("self"), arg("other")))
      .def("__sub__", &VMD::operator-,(arg("left"), arg("right")))
      .def("__isub__", &VMD::operator-=, return_self<>(),
           (arg("self"), arg("other")))
      .def(self * self)
      .def(self *= self)
      // cppcheck-suppress duplicateExpression
      .def(self / self)
      // cppcheck-suppress duplicateExpression
      .def(self /= self);
}
