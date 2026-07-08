// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
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

namespace Mantid::Kernel {
class V3D;
} // namespace Mantid::Kernel

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
double getDistance(const IComponent &self, const IComponent &other) { return self.getDistance(other); }

// Deprecation wrappers. Legacy component-tree access is being retired in favour
// of the index-based ComponentInfo access layer. See the 'Instrument Access via
// SpectrumInfo, DetectorInfo, ComponentInfo' concept page.
Mantid::Kernel::V3D getPosDeprecated(const IComponent &self) {
  PyErr_Warn(PyExc_DeprecationWarning, "'IComponent.getPos' is deprecated in Mantid 7.0, "
                                       "use 'ComponentInfo.position' instead. "
                                       "For more information, see the instrument access layers concept page: "
                                       "https://docs.mantidproject.org/nightly/concepts/InstrumentAccessLayers.html");
  return self.getPos();
}

std::string getNameDeprecated(const IComponent &self) {
  PyErr_Warn(PyExc_DeprecationWarning, "'IComponent.getName' is deprecated in Mantid 7.0, "
                                       "use 'ComponentInfo.name' instead. "
                                       "For more information, see the instrument access layers concept page: "
                                       "https://docs.mantidproject.org/nightly/concepts/InstrumentAccessLayers.html");
  return self.getName();
}

std::string getFullNameDeprecated(const IComponent &self) {
  PyErr_Warn(PyExc_DeprecationWarning, "'IComponent.getFullName' is deprecated in Mantid 7.0, "
                                       "use 'ComponentInfo.name' instead. "
                                       "For more information, see the instrument access layers concept page: "
                                       "https://docs.mantidproject.org/nightly/concepts/InstrumentAccessLayers.html");
  return self.getFullName();
}

std::string typeDeprecated(const IComponent &self) {
  PyErr_Warn(PyExc_DeprecationWarning, "'IComponent.type' is deprecated in Mantid 7.0, "
                                       "use 'ComponentInfo.componentType' instead. "
                                       "For more information, see the instrument access layers concept page: "
                                       "https://docs.mantidproject.org/nightly/concepts/InstrumentAccessLayers.html");
  return self.type();
}

Mantid::Kernel::Quat getRelativeRotDeprecated(const IComponent &self) {
  PyErr_Warn(PyExc_DeprecationWarning, "'IComponent.getRelativeRot' is deprecated in Mantid 7.0, "
                                       "use 'ComponentInfo.relativeRotation' instead. "
                                       "For more information, see the instrument access layers concept page: "
                                       "https://docs.mantidproject.org/nightly/concepts/InstrumentAccessLayers.html");
  return self.getRelativeRot();
}
} // namespace

void export_IComponent() {
  register_ptr_to_python<std::shared_ptr<IComponent>>();
  register_ptr_to_python<std::shared_ptr<const IComponent>>();

  class_<IComponent, boost::noncopyable>("IComponent", no_init)
      .def("getPos", &getPosDeprecated, arg("self"),
           "Returns the absolute position of the component (deprecated, use ComponentInfo.position)")
      .def("getDistance", &getDistance, (arg("self"), arg("other")),
           "Returns the distance, in metres, "
           "between this and the given component")
      .def("getName", &getNameDeprecated, arg("self"),
           "Returns the name of the component (deprecated, use ComponentInfo.name)")
      .def("getFullName", &getFullNameDeprecated, arg("self"),
           "Returns full path name of component (deprecated, use ComponentInfo.name)")
      .def("type", &typeDeprecated, arg("self"),
           "Returns the type of the component represented as a string "
           "(deprecated, use ComponentInfo.componentType)")
      .def("getRelativeRot", &getRelativeRotDeprecated, arg("self"),
           "Returns the relative rotation as a Quat (deprecated, use ComponentInfo.relativeRotation)");
}
