#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidGeometry/Objects/Object.h"
#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Geometry::Object;
using Mantid::Geometry::BoundingBox;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(Object)

void export_Object() {
  register_ptr_to_python<boost::shared_ptr<Object>>();

  class_<Object, boost::noncopyable>("Object", no_init)
      .def("getBoundingBox",
           (const BoundingBox &(Object::*)() const) & Object::getBoundingBox,
           arg("self"), return_value_policy<copy_const_reference>(),
           "Return the axis-aligned bounding box for this shape")

      .def("getShapeXML", &Object::getShapeXML, arg("self"),
           "Returns the XML that was used to create this shape.")

      .def("volume", &Object::volume, arg("self"),
           "Returns the volume of this shape.");
}
