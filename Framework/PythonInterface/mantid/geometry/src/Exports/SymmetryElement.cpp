// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Crystal/SymmetryElement.h"
#include "MantidPythonInterface/core/GetPointer.h"

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/scope.hpp>

using namespace Mantid::Geometry;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(SymmetryElement)

namespace {
Mantid::Kernel::V3D getAxis(SymmetryElement &self) {
  try {
    auto &axisElement = dynamic_cast<SymmetryElementWithAxis &>(self);
    return Mantid::Kernel::V3D(axisElement.getAxis());
  } catch (std::bad_cast &) {
    return Mantid::Kernel::V3D(0, 0, 0);
  }
}

SymmetryElementRotation::RotationSense getRotationSense(SymmetryElement &self) {
  try {
    auto &rotationElement = dynamic_cast<SymmetryElementRotation &>(self);
    return rotationElement.getRotationSense();
  } catch (std::bad_cast &) {
    return SymmetryElementRotation::NoRotation;
  }
}
} // namespace

void export_SymmetryElement() {
  register_ptr_to_python<std::shared_ptr<SymmetryElement>>();

  scope symmetryElementScope = class_<SymmetryElement, boost::noncopyable>("SymmetryElement", no_init);

  enum_<SymmetryElementRotation::RotationSense>("RotationSense")
      .value("Positive", SymmetryElementRotation::Positive)
      .value("Negative", SymmetryElementRotation::Negative)
      .value("NoRotation", SymmetryElementRotation::NoRotation);

  class_<SymmetryElement, boost::noncopyable>("SymmetryElement", no_init)
      .def("getHMSymbol", &SymmetryElement::hmSymbol, arg("self"), return_value_policy<copy_const_reference>(),
           "Returns the Hermann-Mauguin symbol for the element.")
      .def("getAxis", &getAxis, arg("self"),
           "Returns the symmetry axis or [0,0,0] for "
           "identiy, inversion and translations.")
      .def("getRotationSense", &getRotationSense, arg("self"),
           "Returns the rotation sense of a rotation axis or None"
           "if the element is not a rotation.");
}
