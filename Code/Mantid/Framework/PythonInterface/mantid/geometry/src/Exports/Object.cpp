#include "MantidGeometry/Objects/Object.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Geometry::Object;
using namespace boost::python;

void export_Object()
{
  register_ptr_to_python<boost::shared_ptr<Object>>();

  class_<Object, boost::noncopyable>("Object", no_init)
    .def("getShapeXML", &Object::getShapeXML, 
         "Returns the XML that was used to create this shape.")
    ;
}

