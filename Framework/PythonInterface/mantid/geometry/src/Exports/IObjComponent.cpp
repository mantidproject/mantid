#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/Objects/Object.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Geometry::IObjComponent;
using Mantid::Geometry::IComponent;
using namespace boost::python;

namespace {
/**
 * Returns a non-const pointer to the shape object. Avoids a bug with boost
 * python 1.43
 * that can't register shared_ptr to const types.
 * @param self A reference to the calling object to emulate method on Python
 * object
 */
boost::shared_ptr<Mantid::Geometry::Object> getShape(IObjComponent &self) {
  return boost::const_pointer_cast<Mantid::Geometry::Object>(self.shape());
}
}

void export_IObjComponent() {
  register_ptr_to_python<boost::shared_ptr<IObjComponent>>();

  class_<IObjComponent, boost::python::bases<IComponent>, boost::noncopyable>(
      "IObjComponent", no_init)
      .def("shape", &getShape, "Get the object that represents the physical "
                               "shape of this component");
}
