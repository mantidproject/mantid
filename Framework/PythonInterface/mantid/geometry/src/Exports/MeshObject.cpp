// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidGeometry/Objects/MeshObjectCommon.h"
#include "MantidPythonInterface/core/Converters/NDArrayTypeIndex.h"
#include "MantidPythonInterface/core/Converters/WrapWithNDArray.h"
#include "MantidPythonInterface/core/GetPointer.h"

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/self.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#define PY_ARRAY_UNIQUE_SYMBOL GEOMETRY_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>

using Mantid::Geometry::IObject;
using Mantid::Geometry::MeshObject;
using namespace Mantid::PythonInterface::Converters;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(MeshObject)

boost::python::object wrapMeshWithNDArray(MeshObject &self) {
  // PyArray_SimpleNewFromData doesn't interact well with smart pointers so use raw pointer
  auto *meshCoords = new std::vector<double>;
  auto vertices = self.getV3Ds();
  auto triangles = self.getTriangles();
  size_t numberTriangles = triangles.size() / 3;
  npy_intp dims[3] = {numberTriangles, 3, 3};
  for (size_t iTriangle = 0; iTriangle < numberTriangles; ++iTriangle) {
    for (int corner = 0; corner < 3; corner++) {
      auto coords = std::vector<double>(vertices[triangles[(3 * iTriangle) + corner]]);
      for (double coord : coords) {
        meshCoords->push_back(coord);
      } // for each coordinate of that corner
    }   // for each corner of the triangle
  }     // for each triangle
  PyObject *ndarray =
      Impl::wrapWithNDArray(meshCoords->data(), 3, dims, NumpyWrapMode::ReadOnly, OwnershipMode::Python);

  return boost::python::object(handle<>(ndarray));
}

void export_MeshObject() {
  register_ptr_to_python<MeshObject *>();

  class_<MeshObject, boost::python::bases<IObject>, boost::noncopyable>("MeshObject", no_init)
      .def("getMesh", &wrapMeshWithNDArray, (arg("self")), "Get the vertices, grouped by triangles, from mesh");
}