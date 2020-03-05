// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/IComponent.h"
#include "MantidKernel/Quat.h"
#include "MantidPythonInterface/core/GetPointer.h"

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/return_value_policy.hpp>

using Mantid::Geometry::IComponent;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(IComponent)

namespace {
/**
 * A wrapper to IComponent::getDistance that accepts the second component
 * as a "reference to a non-const" object rather than the proper "reference to
 * const". This avoids a warning on RHEL6 in release mode:
 *    warning: dereferencing pointer ‘p.600’ does break strict-aliasing rules
 *
 * @param self The calling object
 * @param other A component that forms the other end of the line for the
 *calculation
 * @return The distance between self & the other component in metres
 */
double getDistance(IComponent &self, IComponent &other) {
  return self.getDistance(other);
}
} // namespace

void export_IComponent() {
  register_ptr_to_python<boost::shared_ptr<IComponent>>();
  register_ptr_to_python<boost::shared_ptr<const IComponent>>();

  class_<IComponent, boost::noncopyable>("IComponent", no_init)
      .def("getPos", &IComponent::getPos, arg("self"),
           "Returns the absolute position of the component")
      .def("getDistance", &getDistance, (arg("self"), arg("other")),
           "Returns the distance, in metres, "
           "between this and the given component")
      .def("getName", &IComponent::getName, arg("self"),
           "Returns the name of the component")
      .def("getFullName", &IComponent::getFullName, arg("self"),
           "Returns full path name of component")
      .def("type", &IComponent::type, arg("self"),
           "Returns the type of the component represented as a string")
      .def("getRelativeRot", &IComponent::getRelativeRot, arg("self"),
           "Returns the relative rotation as a Quat");
}
