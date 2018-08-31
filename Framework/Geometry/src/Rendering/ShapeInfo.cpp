#include "MantidGeometry/Rendering/ShapeInfo.h"
#include "MantidKernel/Tolerance.h"
#include "MantidKernel/V3D.h"
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

void ShapeInfo::getObjectGeometry(ShapeInfo::GeometryShape &myshape,
                                  std::vector<Kernel::V3D> &points,
                                  double &myradius, double &myheight) const {
  myshape = m_shape;
  points = m_points;
  myradius = m_radius;
  myheight = m_height;
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

void ShapeInfo::setSphere(const V3D &c, double r) {
  m_shape = GeometryShape::SPHERE;
  m_points.assign({c});
  m_radius = r;
  m_height = 0;
}

void ShapeInfo::setCylinder(const V3D &c, const V3D &a, double r, double h) {
  m_shape = GeometryShape::CYLINDER;
  m_points.assign({c, a});
  m_radius = r;
  m_height = h;
}

void ShapeInfo::setCone(const V3D &c, const V3D &a, double r, double h) {
  m_shape = GeometryShape::CONE;
  m_points.assign({c, a});
  m_radius = r;
  m_height = h;
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
