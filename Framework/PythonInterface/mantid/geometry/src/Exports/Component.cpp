// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument/Component.h"
#include "MantidKernel/WarningSuppressions.h"
#include <boost/python/class.hpp>
#include <boost/python/overloads.hpp>

using Mantid::Geometry::Component;
using Mantid::Geometry::IComponent;
using namespace boost::python;

// forward declare
namespace Mantid::Kernel {
class Quat;
class V3D;
} // namespace Mantid::Kernel

namespace {
// Deprecation wrappers. Absolute/relative placement is available from the
// index-based ComponentInfo access layer. See the 'Instrument Access via
// SpectrumInfo, DetectorInfo, ComponentInfo' concept page. (The parameter and
// description accessors are intentionally not deprecated yet; they have no
// ComponentInfo equivalent and await the parameter-map redesign.)
Mantid::Kernel::Quat getRotationDeprecated(const Component &self) {
  PyErr_Warn(PyExc_DeprecationWarning, "'Component.getRotation()' is deprecated in Mantid 7.0, "
                                       "use 'ComponentInfo.rotation' instead. "
                                       "For more information, see the instrument access layers concept page: "
                                       "https://docs.mantidproject.org/nightly/concepts/InstrumentAccessLayers.html");
  return self.getRotation();
}

Mantid::Kernel::V3D getRelativePosDeprecated(const Component &self) {
  PyErr_Warn(PyExc_DeprecationWarning, "'Component.getRelativePos' is deprecated in Mantid 7.0, "
                                       "use 'ComponentInfo.relativePosition' instead. "
                                       "For more information, see the instrument access layers concept page: "
                                       "https://docs.mantidproject.org/nightly/concepts/InstrumentAccessLayers.html");
  return self.getRelativePos();
}

GNU_DIAG_OFF("unused-local-typedef")
// Ignore -Wconversion warnings coming from boost::python
// Seen with GCC 7.1.1 and Boost 1.63.0
GNU_DIAG_OFF("conversion")

// Default parameter function overloads
// cppcheck-suppress unknownMacro
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Component_getParameterNames, Component::getParameterNames, 0, 1)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Component_hasParameter, Component::hasParameter, 1, 2)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Component_getNumberParameter, Component::getNumberParameter, 1, 2)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Component_getBoolParameter, Component::getBoolParameter, 1, 2)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Component_getPositionParameter, Component::getPositionParameter, 1, 2)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Component_getRotationParameter, Component::getRotationParameter, 1, 2)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Component_getStringParameter, Component::getStringParameter, 1, 2)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Component_getIntParameter, Component::getIntParameter, 1, 2)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Component_getParameterType, Component::getParameterType, 1, 2)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Component_getParamShortDescription, Component::getParamShortDescription, 1, 2)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Component_getParamDescription, Component::getParamDescription, 1, 2)

GNU_DIAG_ON("conversion")
GNU_DIAG_ON("unused-local-typedef")
} // namespace
void export_Component() {
  class_<Component, bases<IComponent>, boost::noncopyable>("Component", no_init)
      .def("getParameterNames", &Component::getParameterNames,
           Component_getParameterNames((arg("self"), arg("recursive") = true)))
      .def("hasParameter", &Component::hasParameter,
           Component_hasParameter((arg("self"), arg("name"), arg("recursive") = true)))
      .def("getNumberParameter", &Component::getNumberParameter,
           Component_getNumberParameter((arg("self"), arg("pname"), arg("recursive") = true)))
      .def("getBoolParameter", &Component::getBoolParameter,
           Component_getBoolParameter((arg("self"), arg("pname"), arg("recursive") = true)))
      .def("getPositionParameter", &Component::getPositionParameter,
           Component_getPositionParameter((arg("self"), arg("pname"), arg("recursive") = true)))
      .def("getRotationParameter", &Component::getRotationParameter,
           Component_getRotationParameter((arg("self"), arg("pname"), arg("recursive") = true)))
      .def("getStringParameter", &Component::getStringParameter,
           Component_getStringParameter((arg("self"), arg("pname"), arg("recursive") = true)))
      .def("getIntParameter", &Component::getIntParameter,
           Component_getIntParameter((arg("self"), arg("pname"), arg("recursive") = true)))
      //
      .def("getRotation", &getRotationDeprecated, arg("self"),
           "Returns the absolute rotation as a Quat (deprecated, use ComponentInfo.rotation)")
      .def("getRelativePos", &getRelativePosDeprecated, arg("self"),
           "Returns the relative position as a V3D (deprecated, use ComponentInfo.relativePosition)")
      //
      .def("getParamShortDescription", &Component::getParamShortDescription,
           Component_getParamShortDescription((arg("self"), arg("pname"), arg("recursive") = true)))
      .def("getParamDescription", &Component::getParamDescription,
           Component_getParamDescription((arg("self"), arg("pname"), arg("recursive") = true)))

      .def("getShortDescription", &Component::getShortDescription, arg("self"),
           "Return the short description of current parameterized component")
      .def("getDescription", &Component::getDescription, arg("self"),
           "Return the description of current parameterized component")
      .def("setDescription", &Component::setDescription, (arg("self"), arg("descr")),
           "Set component's description, works only if the component is "
           "parameterized component")

      // HACK -- python should return parameters regardless of type. this is
      // untill rows below do not work
      .def("getParameterType", &Component::getParameterType,
           Component_getParameterType((arg("self"), arg("pname"), arg("recursive") = true)))
      .def("getFittingParameter", &Component::getFittingParameter, (arg("self"), arg("pname"), arg("xvalue")),
           "Get fit parameter from the parameter map."
           " The value of the parameter is determined from a look up table or a formula")
      //// this does not work for some obvious or not obvious reasons
      //.def("getParameter", &Component::getNumberParameter,
      // Component_getNumberParameter())
      //.def("getParameter", &Component::getBoolParameter,
      // Component_getBoolParameter())
      //.def("getParameter", &Component::getStringParameter,
      // Component_getStringParameter())
      //.def("getParameter", &Component::getPositionParameter,
      // Component_getPositionParameter())
      //.def("getParameter", &Component::getRotationParameter,
      // Component_getRotationParameter())
      ;
}
