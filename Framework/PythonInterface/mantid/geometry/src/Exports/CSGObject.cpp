// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Rendering/GeometryTriangulator.h"
#include "MantidKernel/Logger.h"
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

namespace {
Mantid::Kernel::Logger g_log("CSGObject");
}

GET_POINTER_SPECIALIZATION(CSGObject)

boost::python::object getEmptyArrayObject() {
  npy_intp dims[3] = {0, 3, 3};
  auto *emptyData = new double[1]; // Minimal allocation for empty array
  emptyData[0] = 0.0;
  PyObject *ndarray = Impl::wrapWithNDArray(emptyData, 3, dims, NumpyWrapMode::ReadOnly, OwnershipMode::Python);
  return object(handle<>(ndarray));
}

boost::python::object wrapMeshWithNDArray(const CSGObject *self) {
  if (self->getShapeXML().find("infinite") != std::string::npos) {
    throw std::runtime_error("Cannot plot Shapes of infinite extent.");
  }
  try {
    auto localTriangulator = GeometryTriangulator(self);
    const auto &vertices = localTriangulator.getTriangleVertices();
    const auto &triangles = localTriangulator.getTriangleFaces();
    const size_t &numberTriangles = localTriangulator.numTriangleFaces();
    npy_intp dims[3] = {static_cast<npy_intp>(numberTriangles), 3, 3};

    if (numberTriangles == 0 || vertices.empty() || triangles.empty()) {
      return getEmptyArrayObject();
    }

    size_t totalSize = dims[0] * dims[1] * dims[2];
    auto *meshCoords = new double[totalSize];
    std::fill(meshCoords, meshCoords + totalSize, 0.0);

    // Copy triangle data with bounds checking
    for (size_t corner_index = 0; corner_index < triangles.size() && corner_index < totalSize; ++corner_index) {
      for (size_t xyz = 0; xyz < 3; xyz++) {
        size_t vertex_idx = triangles[corner_index];
        if (vertex_idx * 3 + xyz < vertices.size()) {
          meshCoords[3 * corner_index + xyz] = vertices[3 * vertex_idx + xyz];
        }
      } // for each coordinate of that corner
    } // for each corner of the triangle
    PyObject *ndarray = Impl::wrapWithNDArray(meshCoords, 3, dims, NumpyWrapMode::ReadOnly, OwnershipMode::Python);
    return object(handle<>(ndarray));

  } catch (const std::exception &e) {
    // Return empty array on failure
    g_log.error(e.what());
    return getEmptyArrayObject();
  }
}

void export_Object() {
  register_ptr_to_python<std::shared_ptr<CSGObject>>();
  register_ptr_to_python<std::shared_ptr<IObject>>();

  class_<CSGObject, boost::python::bases<IObject>, boost::noncopyable>("CSGObject", no_init)
      .def("getBoundingBox", (const BoundingBox &(CSGObject::*)() const) & CSGObject::getBoundingBox, arg("self"),
           return_value_policy<copy_const_reference>(), "Return the axis-aligned bounding box for this shape")

      .def("getShapeXML", &CSGObject::getShapeXML, arg("self"), "Returns the XML that was used to create this shape.")

      .def("volume", &CSGObject::volume, arg("self"), "Returns the volume of this shape.")

      .def("getMesh", &wrapMeshWithNDArray, (arg("self")), "Get the vertices, grouped by triangles, from mesh");
}
