// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/V3D.h"
#include "MantidKernel/WarningSuppressions.h"
#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/errors.hpp>
#include <boost/python/list.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/return_arg.hpp>
#include <boost/python/return_value_policy.hpp>

using namespace boost::python;
using Mantid::Kernel::V3D;

namespace {

/**
 * Helper forwarding method for directionAngles to allow default argument
 * values.
 * @param self
 * @return
 */
V3D directionAnglesDefault(V3D &self) { return self.directionAngles(); }

long hashV3D(V3D &self) {
  boost::python::object tmpObj(self.toString());

  return PyObject_Hash(tmpObj.ptr());
}

double getV3DItem(V3D &self, int index) {
  switch (index) {
  case -3:
  case 0:
    return self.X();
  case -2:
  case 1:
    return self.Y();
  case -1:
  case 2:
    return self.Z();
  }
  PyErr_SetString(PyExc_IndexError, "index out of range");
  throw boost::python::error_already_set();
}

void setV3DItem(V3D &self, int index, double value) {
  switch (index) {
  case -3:
  case 0:
    self.setX(value);
    break;
  case -2:
  case 1:
    self.setY(value);
    break;
  case -1:
  case 2:
    self.setZ(value);
    break;
  default:
    PyErr_SetString(PyExc_IndexError, "index out of range");
    throw boost::python::error_already_set();
  }
}

int getV3DLength(V3D &self) {
  UNUSED_ARG(self);
  return 3;
}
} // namespace

class V3DPickleSuite : public boost::python::pickle_suite {
public:
  static dict getstate(const V3D &vector) {
    dict data;
    data['x'] = vector.X();
    data['y'] = vector.Y();
    data['z'] = vector.Z();
    return data;
  }

  static void setstate(V3D &vector, dict state) {
    vector.setX(extract<double>(state['x']));
    vector.setY(extract<double>(state['y']));
    vector.setZ(extract<double>(state['z']));
  }
};

void export_V3D() {
  // V3D class
  GNU_DIAG_OFF("self-assign-overloaded")
  class_<V3D>("V3D", init<>("Construct a V3D at the origin"))
      .def_pickle(V3DPickleSuite())
      .def(init<double, double, double>(
          "Construct a V3D with X,Y,Z coordinates"))
      .def("X", &V3D::X, arg("self"), "Returns the X coordinate")
      .def("Y", &V3D::Y, arg("self"), "Returns the Y coordinate")
      .def("Z", &V3D::Z, arg("self"), "Returns the Z coordinate")
      .def("getX", &V3D::X, arg("self"),
           "Returns the X coordinate") // Traditional name
      .def("getY", &V3D::Y, arg("self"),
           "Returns the Y coordinate") // Traditional name
      .def("getZ", &V3D::Z, arg("self"),
           "Returns the Z coordinate") // Traditional name
      .def("distance", &V3D::distance, (arg("self"), arg("other")),
           "Returns the distance between this vector and another")
      .def("angle", &V3D::angle, (arg("self"), arg("other")),
           "Returns the angle between this vector and another")
      .def("cosAngle", &V3D::cosAngle, (arg("self"), arg("other")),
           "Returns cos(angle) between this vector and another")
      .def("zenith", &V3D::zenith, (arg("self"), arg("other")),
           "Returns the zenith between this vector and another")
      .def("scalar_prod", &V3D::scalar_prod, (arg("self"), arg("other")),
           "Computes the scalar product between this and another vector")
      .def("cross_prod", &V3D::cross_prod, (arg("self"), arg("other")),
           "Computes the cross product between this and another vector")
      .def("norm", &V3D::norm, arg("self"),
           "Calculates the length of the vector")
      .def("norm2", &V3D::norm2, arg("self"),
           "Calculates the squared length of the vector")
      .def("__add__", &V3D::operator+,(arg("left"), arg("right")))
      .def("__iadd__", &V3D::operator+=, return_self<>(),
           (arg("self"), arg("other")))
      .def("__sub__",
           static_cast<V3D (V3D::*)(const V3D &) const>(&V3D::operator-),
           (arg("left"), arg("right")))
      .def("__isub__", &V3D::operator-=, return_self<>(),
           (arg("self"), arg("other")))
      .def("__neg__", static_cast<V3D (V3D::*)() const>(&V3D::operator-),
           (arg("self")))
      .def("__len__", &getV3DLength, (arg("self")),
           "Returns the length of the vector for list-like interface. Always "
           "returns 3.")
      .def("__getitem__", &getV3DItem, (arg("self"), arg("index")),
           "Access the V3D-object like a list for getting elements.")
      .def("__setitem__", &setV3DItem,
           (arg("self"), arg("index"), arg("value")),
           "Access the V3D-object like a list for setting elements.")

      .def(self * self)
      .def(self *= self)
      // cppcheck-suppress duplicateExpression
      .def(self / self)
      // cppcheck-suppress duplicateExpression
      .def(self /= self)
      .def(self * int())
      .def(self *= int())
      .def(self * double())
      .def(self *= double())
      // cppcheck-suppress duplicateExpression
      .def(self < self)
      .def(self == self)
      .def(self != self) // must define != as Python's default is to compare
                         // object address
      .def(self_ns::str(self))
      .def(self_ns::repr(self))
      .def("__hash__", &hashV3D)
      .def("directionAngles", &V3D::directionAngles,
           (arg("self"), arg("inDegrees")),
           "Calculate direction angles from direction cosines")
      .def("directionAngles", &directionAnglesDefault, arg("self"),
           "Calculate direction angles from direction cosines");
  GNU_DIAG_ON("self-assign-overloaded")
}
