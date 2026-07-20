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
    const std::vector<double> *vertices = &self->getTriangleVertices();
    const std::vector<uint32_t> *triangleFaces = &self->getTriangleFaces();
    std::unique_ptr<GeometryTriangulator> localTriangulator;

    if (triangleFaces->empty() || vertices->empty()) {
      localTriangulator = std::make_unique<GeometryTriangulator>(self);
      vertices = &localTriangulator->getTriangleVertices();
      triangleFaces = &localTriangulator->getTriangleFaces();
      if (triangleFaces->empty() || vertices->empty()) {
        return getEmptyArrayObject();
      }
    }

    const size_t numberTriangles = triangleFaces->size() / 3;
    npy_intp dims[3] = {static_cast<npy_intp>(numberTriangles), 3, 3};
    auto *meshCoords = new double[numberTriangles * 9];

    const size_t vertexCapacity = vertices->size();
    for (size_t corner = 0; corner < triangleFaces->size(); ++corner) {
      const size_t src = static_cast<size_t>((*triangleFaces)[corner]) * 3;
      const size_t dst = corner * 3;
      if (src + 2 < vertexCapacity) {
        meshCoords[dst] = (*vertices)[src];
        meshCoords[dst + 1] = (*vertices)[src + 1];
        meshCoords[dst + 2] = (*vertices)[src + 2];
      } else {
        meshCoords[dst] = 0.0;
        meshCoords[dst + 1] = 0.0;
        meshCoords[dst + 2] = 0.0;
      }
    }

    PyObject *ndarray = Impl::wrapWithNDArray(meshCoords, 3, dims, NumpyWrapMode::ReadOnly, OwnershipMode::Python);
    return object(handle<>(ndarray));

  } catch (const std::exception &e) {
    g_log.error(e.what());
    return getEmptyArrayObject();
  }
}

void export_Object() {
  register_ptr_to_python<std::shared_ptr<CSGObject>>();

  class_<CSGObject, boost::python::bases<IObject>, boost::noncopyable>("CSGObject", no_init)
      .def("getBoundingBox", (const BoundingBox &(CSGObject::*)() const) & CSGObject::getBoundingBox, arg("self"),
           return_value_policy<copy_const_reference>(), "Return the axis-aligned bounding box for this shape")

      .def("getShapeXML", &CSGObject::getShapeXML, arg("self"), "Returns the XML that was used to create this shape.")

      .def("volume", &CSGObject::volume, arg("self"), "Returns the volume of this shape.")

      .def("getMesh", &wrapMeshWithNDArray, (arg("self")), "Get the vertices, grouped by triangles, from mesh")
      .def("hasValidShape", &CSGObject::hasValidShape, arg("self"), "Check if the shape is valid")
      .def("shapeInfo", &CSGObject::shapeInfo, arg("self"), return_value_policy<copy_const_reference>(),
           "Get the shape information for this object");
}
