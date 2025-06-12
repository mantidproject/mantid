// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Rendering/GeometryTriangulator.h"
#include "MantidPythonInterface/core/Converters/NDArrayTypeIndex.h"
#include "MantidPythonInterface/core/Converters/WrapWithNDArray.h"
#include "MantidPythonInterface/core/GetPointer.h"

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/self.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

#include <numpy/arrayobject.h>

#define PY_ARRAY_UNIQUE_SYMBOL GEOMETRY_ARRAY_API
#define NO_IMPORT_ARRAY

using Mantid::Geometry::IObject;
using namespace Mantid::PythonInterface::Converters;
using Mantid::Geometry::BoundingBox;
using Mantid::Geometry::CSGObject;
using Mantid::Geometry::detail::GeometryTriangulator;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(CSGObject)

boost::python::object wrapMeshWithNDArray(const CSGObject *self) {
  // PyArray_SimpleNewFromData doesn't interact well with smart pointers so use raw pointer

  if (self->getShapeXML().find("infinite") != std::string::npos) {
    throw std::runtime_error("Cannot plot Shapes of infinite extent.");
  }
  auto localTriangulator = GeometryTriangulator(self);
  const auto &vertices = localTriangulator.getTriangleVertices();
  const auto &triangles = localTriangulator.getTriangleFaces();
  const size_t &numberTriangles = localTriangulator.numTriangleFaces();
  npy_intp dims[3] = {static_cast<int>(numberTriangles), 3, 3};
  auto *meshCoords = new double[dims[0] * dims[1] * dims[2]];
  for (size_t corner_index = 0; corner_index < triangles.size(); ++corner_index) {
    for (size_t xyz = 0; xyz < 3; xyz++) {
      meshCoords[3 * corner_index + xyz] = vertices[3 * triangles[corner_index] + xyz];
    } // for each coordinate of that corner
  } // for each corner of the triangle

  PyObject *ndarray = Impl::wrapWithNDArray(meshCoords, 3, dims, NumpyWrapMode::ReadWrite, OwnershipMode::Python);
  return object(handle<>(ndarray));
}

void export_Object() {
  register_ptr_to_python<std::shared_ptr<CSGObject>>();

  class_<CSGObject, boost::python::bases<IObject>, boost::noncopyable>("CSGObject", no_init)
      .def("getBoundingBox", (const BoundingBox &(CSGObject::*)() const) & CSGObject::getBoundingBox, arg("self"),
           return_value_policy<copy_const_reference>(), "Return the axis-aligned bounding box for this shape")

      .def("getShapeXML", &CSGObject::getShapeXML, arg("self"), "Returns the XML that was used to create this shape.")

      .def("volume", &CSGObject::volume, arg("self"), "Returns the volume of this shape.")

      .def("getMesh", &wrapMeshWithNDArray, (arg("self")), "Get the vertices, grouped by triangles, from mesh");
}
