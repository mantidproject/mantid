#include "MantidGeometry/IComponent.h"
#include "MantidPythonInterface/kernel/SharedPtrToPythonMacro.h"

#include <boost/python/class.hpp>

using Mantid::Geometry::IComponent;
using namespace boost::python;

namespace
{
  /**
   * A wrapper to IComponent::getDistance that accepts the second component
   * as a "reference to a non-const" object rather than the proper "reference to
   * const". This avoids a warning on RHEL6 in release mode:
   *    warning: dereferencing pointer ‘p.600’ does break strict-aliasing rules
   *
   * @param self The calling object
   * @param other A component that forms the other end of the line for the calculation
   * @return The distance between self & the other component in metres
   */
  double getDistance(IComponent & self, IComponent & other)
  {
    return self.getDistance(other);
  }

}

void export_IComponent()
{
  REGISTER_SHARED_PTR_TO_PYTHON(IComponent);

  class_<IComponent, boost::noncopyable>("IComponent", no_init)
    .def("getPos", &IComponent::getPos, "Returns the absolute position of the component")
    .def("getDistance", &getDistance, "Returns the distance, in metres, between this and the given component")
    .def("getName", &IComponent::getName, "Returns the name of the component")
    .def("getFullName", &IComponent::getFullName,"Returns full path name of component")
    .def("type", &IComponent::type, "Returns the type of the component represented as a string")
    ;

}

