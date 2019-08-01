// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Geometry::BoundingBox;
using Mantid::Geometry::CSGObject;
using Mantid::Geometry::IObject;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(CSGObject)

void export_Object() {
  register_ptr_to_python<boost::shared_ptr<CSGObject>>();

  class_<CSGObject, boost::python::bases<IObject>, boost::noncopyable>(
      "CSGObject", no_init)
      .def("getBoundingBox",
           (const BoundingBox &(CSGObject::*)() const) &
               CSGObject::getBoundingBox,
           arg("self"), return_value_policy<copy_const_reference>(),
           "Return the axis-aligned bounding box for this shape")

      .def("getShapeXML", &CSGObject::getShapeXML, arg("self"),
           "Returns the XML that was used to create this shape.")

      .def("volume", &CSGObject::volume, arg("self"),
           "Returns the volume of this shape.");
}
