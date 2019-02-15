// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Rendering/ShapeInfo.h"
#include "MantidKernel/Tolerance.h"
#include "MantidKernel/V3D.h"
#include <cassert>
#include <cmath>

namespace Mantid {
using Kernel::V3D;
namespace Geometry {
namespace detail {
ShapeInfo::ShapeInfo()
    : m_points(), m_radius(0), m_height(0),
      m_shape(ShapeInfo::GeometryShape::NOSHAPE) {}

const std::vector<Kernel::V3D> &ShapeInfo::points() const { return m_points; }

double ShapeInfo::radius() const { return m_radius; }

double ShapeInfo::height() const { return m_height; }

ShapeInfo::GeometryShape ShapeInfo::shape() const { return m_shape; }

void ShapeInfo::getObjectGeometry(ShapeInfo::GeometryShape &shape,
                                  std::vector<Kernel::V3D> &points,
                                  double &radius, double &height) const {
  shape = m_shape;
  points = m_points;
  radius = m_radius;
  height = m_height;
}

ShapeInfo::CuboidGeometry ShapeInfo::cuboidGeometry() const {
  assert(m_shape == GeometryShape::CUBOID);
  CuboidGeometry g{m_points[0], m_points[1], m_points[2], m_points[3]};
  return g;
}

ShapeInfo::HexahedronGeometry ShapeInfo::hexahedronGeometry() const {
  assert(m_shape == GeometryShape::HEXAHEDRON);
  HexahedronGeometry g{m_points[0], m_points[1], m_points[2], m_points[3],
                       m_points[4], m_points[5], m_points[6], m_points[7]};
  return g;
}

ShapeInfo::SphereGeometry ShapeInfo::sphereGeometry() const {
  assert(m_shape == GeometryShape::SPHERE);
  SphereGeometry g{m_points.front(), m_radius};
  return g;
}

ShapeInfo::CylinderGeometry ShapeInfo::cylinderGeometry() const {
  assert(m_shape == GeometryShape::CYLINDER);
  CylinderGeometry g{m_points.front(), m_points.back(), m_radius, m_height};
  return g;
}

ShapeInfo::ConeGeometry ShapeInfo::coneGeometry() const {
  assert(m_shape == GeometryShape::CONE);
  ConeGeometry g{m_points.front(), m_points.back(), m_radius, m_height};
  return g;
}

void ShapeInfo::setCuboid(const V3D &p1, const V3D &p2, const V3D &p3,
                          const V3D &p4) {
  m_shape = GeometryShape::CUBOID;
  m_points.assign({p1, p2, p3, p4});
  m_radius = 0;
  m_height = 0;
}

void ShapeInfo::setHexahedron(const V3D &p1, const V3D &p2, const V3D &p3,
                              const V3D &p4, const V3D &p5, const V3D &p6,
                              const V3D &p7, const V3D &p8) {
  m_shape = GeometryShape::HEXAHEDRON;
  m_points.assign({p1, p2, p3, p4, p5, p6, p7, p8});
  m_radius = 0;
  m_height = 0;
}

void ShapeInfo::setSphere(const V3D &center, double radius) {
  m_shape = GeometryShape::SPHERE;
  m_points.assign({center});
  m_radius = radius;
  m_height = 0;
}

void ShapeInfo::setCylinder(const V3D &centerBottomBase,
                            const V3D &symmetryAxis, double radius,
                            double height) {
  m_shape = GeometryShape::CYLINDER;
  m_points.assign({centerBottomBase, symmetryAxis});
  m_radius = radius;
  m_height = height;
}

void ShapeInfo::setCone(const V3D &center, const V3D &symmetryAxis,
                        double radius, double height) {
  m_shape = GeometryShape::CONE;
  m_points.assign({center, symmetryAxis});
  m_radius = radius;
  m_height = height;
}

bool ShapeInfo::operator==(const ShapeInfo &other) {
  return m_shape == other.m_shape &&
         std::abs(m_height - other.m_height) < Kernel::Tolerance &&
         std::abs(m_radius - other.m_radius) < Kernel::Tolerance &&
         m_points == other.m_points;
}

} // namespace detail
} // namespace Geometry
} // namespace Mantid
