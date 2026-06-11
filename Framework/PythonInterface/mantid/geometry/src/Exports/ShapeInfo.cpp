// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Rendering/ShapeInfo.h"
#include <boost/python/class.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/enum.hpp>

using Mantid::Geometry::detail::ShapeInfo;
using namespace boost::python;

namespace {
dict cylinderGeometry(const ShapeInfo &self) {
  if (self.shape() != ShapeInfo::GeometryShape::CYLINDER) {
    PyErr_SetString(PyExc_RuntimeError,
                    "cylinderGeometry() is only valid for ShapeInfo objects with shape() == GeometryShape.CYLINDER");
    throw boost::python::error_already_set();
  }
  const auto cylGeom = self.cylinderGeometry();
  dict cylGeomDict;
  cylGeomDict["centreOfBottomBase"] = cylGeom.centreOfBottomBase;
  cylGeomDict["axis"] = cylGeom.axis;
  cylGeomDict["radius"] = cylGeom.radius;
  cylGeomDict["height"] = cylGeom.height;
  return cylGeomDict;
}

dict cuboidGeometry(const ShapeInfo &self) {
  if (self.shape() != ShapeInfo::GeometryShape::CUBOID) {
    PyErr_SetString(PyExc_RuntimeError,
                    "cuboidGeometry() is only valid for ShapeInfo objects with shape() == GeometryShape.CUBOID");
    throw boost::python::error_already_set();
  }
  const auto cubGeom = self.cuboidGeometry();
  dict cubGeomDict;
  cubGeomDict["leftFrontBottom"] = cubGeom.leftFrontBottom;
  cubGeomDict["leftFrontTop"] = cubGeom.leftFrontTop;
  cubGeomDict["leftBackBottom"] = cubGeom.leftBackBottom;
  cubGeomDict["rightFrontBottom"] = cubGeom.rightFrontBottom;
  return cubGeomDict;
}

} // namespace

void export_ShapeInfo() {
  enum_<ShapeInfo::GeometryShape>("GeometryShape")
      .value("NOSHAPE", ShapeInfo::GeometryShape::NOSHAPE)
      .value("CUBOID", ShapeInfo::GeometryShape::CUBOID)
      .value("HEXAHEDRON", ShapeInfo::GeometryShape::HEXAHEDRON)
      .value("SPHERE", ShapeInfo::GeometryShape::SPHERE)
      .value("CYLINDER", ShapeInfo::GeometryShape::CYLINDER)
      .value("CONE", ShapeInfo::GeometryShape::CONE)
      .value("HOLLOWCYLINDER", ShapeInfo::GeometryShape::HOLLOWCYLINDER)
      .export_values();

  class_<ShapeInfo>("ShapeInfo", no_init)
      .def("shape", &ShapeInfo::shape, arg("self"), "Returns the geometry shape type (GeometryShape enum).")
      .def("radius", &ShapeInfo::radius, arg("self"),
           "Returns the radius for sphere, cylinder, cone or hollow cylinder.")
      .def("innerRadius", &ShapeInfo::innerRadius, arg("self"), "Returns the inner radius for a hollow cylinder.")
      .def("height", &ShapeInfo::height, arg("self"), "Returns the height for cylinder, cone or hollow cylinder.")
      .def("cylinderGeometry", &cylinderGeometry, arg("self"),
           "Returns a CylinderGeometry struct with the geometry information for a cylinder shape. Only valid if "
           "shape() returns GeometryShape.CYLINDER.")
      .def("cuboidGeometry", &cuboidGeometry, arg("self"),
           "Returns a CuboidGeometry struct with the geometry information for a cuboid shape. Only valid if shape() "
           "returns GeometryShape.CUBOID.");
}
