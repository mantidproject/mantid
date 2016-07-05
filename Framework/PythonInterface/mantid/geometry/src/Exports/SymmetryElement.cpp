#include "MantidGeometry/Crystal/SymmetryElement.h"

#include <boost/python/class.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/scope.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using namespace Mantid::Geometry;
using namespace boost::python;

namespace {
Mantid::Kernel::V3D getAxis(SymmetryElement &self) {
  try {
    SymmetryElementWithAxis &axisElement =
        dynamic_cast<SymmetryElementWithAxis &>(self);
    return Mantid::Kernel::V3D(axisElement.getAxis());
  } catch (std::bad_cast &) {
    return Mantid::Kernel::V3D(0, 0, 0);
  }
}

object getRotationSense(SymmetryElement &self) {
  using RotationSenseToPython =
      to_python_value<SymmetryElementRotation::RotationSense>;
  try {
    SymmetryElementRotation &rotationElement =
        dynamic_cast<SymmetryElementRotation &>(self);
    auto sense = rotationElement.getRotationSense();
    if (sense != SymmetryElementRotation::None) {
      return object(handle<>(RotationSenseToPython()(sense)));
    } else {
      return object();
    }
  } catch (std::bad_cast &) {
    return object();
  }
}
}

void export_SymmetryElement() {
  register_ptr_to_python<boost::shared_ptr<SymmetryElement>>();

  scope symmetryElementScope =
      class_<SymmetryElement, boost::noncopyable>("SymmetryElement", no_init);

  enum_<SymmetryElementRotation::RotationSense>("RotationSense")
      .value("Positive", SymmetryElementRotation::Positive)
      .value("Negative", SymmetryElementRotation::Negative);

  class_<SymmetryElement, boost::noncopyable>("SymmetryElement", no_init)
      .def("getHMSymbol", &SymmetryElement::hmSymbol, arg("self"),
           "Returns the Hermann-Mauguin symbol for the element.")
      .def("getAxis", &getAxis, arg("self"),
           "Returns the symmetry axis or [0,0,0] for "
           "identiy, inversion and translations.")
      .def("getRotationSense", &getRotationSense, arg("self"),
           "Returns the rotation sense of a rotation axis or None"
           "if the element is not a rotation.");
}
