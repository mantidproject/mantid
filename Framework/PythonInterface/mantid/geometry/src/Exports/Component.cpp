#include "MantidGeometry/Instrument/Component.h"
#include <boost/python/class.hpp>
#include <boost/python/overloads.hpp>

using Mantid::Geometry::Component;
using Mantid::Geometry::IComponent;
using namespace boost::python;

namespace {
// Default parameter function overloads
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Component_getParameterNames,
                                       Component::getParameterNames, 0, 1)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Component_hasParameter,
                                       Component::hasParameter, 1, 2)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Component_getNumberParameter,
                                       Component::getNumberParameter, 1, 2)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Component_getBoolParameter,
                                       Component::getBoolParameter, 1, 2)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Component_getPositionParameter,
                                       Component::getPositionParameter, 1, 2)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Component_getRotationParameter,
                                       Component::getRotationParameter, 1, 2)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Component_getStringParameter,
                                       Component::getStringParameter, 1, 2)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Component_getIntParameter,
                                       Component::getIntParameter, 1, 2)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Component_getParameterType,
                                       Component::getParameterType, 1, 2)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Component_getRotation,
                                       Component::getRotation, 0, 0)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Component_getRelativePos,
                                       Component::getRelativePos, 0, 0)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Component_getParamShortDescription,
                                       Component::getParamShortDescription, 1,
                                       2)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Component_getParamDescription,
                                       Component::getParamDescription, 1, 2)
}

void export_Component() {
  class_<Component, bases<IComponent>, boost::noncopyable>("Component", no_init)
      .def("getParameterNames", &Component::getParameterNames,
           Component_getParameterNames())
      .def("hasParameter", &Component::hasParameter, Component_hasParameter())
      .def("getNumberParameter", &Component::getNumberParameter,
           Component_getNumberParameter())
      .def("getBoolParameter", &Component::getBoolParameter,
           Component_getBoolParameter())
      .def("getPositionParameter", &Component::getPositionParameter,
           Component_getPositionParameter())
      .def("getRotationParameter", &Component::getRotationParameter,
           Component_getRotationParameter())
      .def("getStringParameter", &Component::getStringParameter,
           Component_getStringParameter())
      .def("getIntParameter", &Component::getIntParameter,
           Component_getIntParameter())
      //
      .def("getRotation", &Component::getRotation, Component_getRotation())
      .def("getRelativePos", &Component::getRelativePos,
           Component_getRelativePos())
      //
      .def("getParamShortDescription", &Component::getParamShortDescription,
           Component_getParamShortDescription())
      .def("getParamDescription", &Component::getParamDescription,
           Component_getParamDescription())
      .def("getShortDescription", &Component::getShortDescription,
           "Return the short description of current parameterized component")
      .def("getDescription", &Component::getDescription,
           "Return the description of current parameterized component")
      .def("setDescription", &Component::setDescription,
           "Set component's description, works only if the component is "
           "parameterized component")

      // HACK -- python should return parameters regardless of type. this is
      // untill rows below do not work
      .def("getParameterType", &Component::getParameterType,
           Component_getParameterType())
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
