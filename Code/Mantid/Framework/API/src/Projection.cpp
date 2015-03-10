#include "MantidAPI/Projection.h"

namespace Mantid {
namespace API {

Projection::Projection() {
  m_dimensions[0][0] = 1.0;
  m_dimensions[1][1] = 1.0;
  m_dimensions[2][2] = 1.0;
  m_offsets[0] = 0.0;
  m_offsets[1] = 0.0;
  m_offsets[2] = 0.0;
  m_units[0] = RLU;
  m_units[1] = RLU;
  m_units[2] = RLU;
}

Projection::Projection(const V3D &u, const V3D &v) {
  m_dimensions[0] = u;
  m_dimensions[1] = v;
  m_dimensions[2] = u.cross_prod(v);
  m_offsets[0] = 0.0;
  m_offsets[1] = 0.0;
  m_offsets[2] = 0.0;
  m_units[0] = RLU;
  m_units[1] = RLU;
  m_units[2] = RLU;
}

Projection::Projection(const V3D &u, const V3D &v, const V3D &w) {
  m_dimensions[0] = u;
  m_dimensions[1] = v;
  m_dimensions[2] = w;
  for (size_t i = 0; i < 3; ++i) {
    m_offsets[i] = 0.0;
    m_units[i] = RLU;
  }
}

Projection::Projection(const Projection &other) {
  for (size_t i = 0; i < 3; ++i) {
    m_dimensions[i] = other.m_dimensions[i];
    m_offsets[i] = other.m_offsets[i];
    m_units[i] = other.m_units[i];
  }
}

Projection &Projection::operator=(const Projection &other) {
  for (size_t i = 0; i < 3; ++i) {
    m_dimensions[i] = other.m_dimensions[i];
    m_offsets[i] = other.m_offsets[i];
    m_units[i] = other.m_units[i];
  }
  return *this;
}

Projection::~Projection() { }

double Projection::getOffset(size_t nd) {
  if (nd >= 3)
    throw std::invalid_argument("given axis out of range");
  else
    return m_offsets[nd];
}

V3D Projection::getAxis(size_t nd) {
  if (nd >= 3)
    throw std::invalid_argument("given axis out of range");
  else
    return m_dimensions[nd];
}

ProjectionUnit Projection::getUnit(size_t nd) {
  if (nd >= 3)
    throw std::invalid_argument("given axis out of range");
  else
    return m_units[nd];
}

void Projection::setOffset(size_t nd, double offset) {
  if (nd >= 3)
    throw std::invalid_argument("given axis out of range");
  else
    m_offsets[nd] = offset;
}

void Projection::setAxis(size_t nd, V3D axis) {
  if (nd >= 3)
    throw std::invalid_argument("given axis out of range");
  else
    m_dimensions[nd] = axis;
}

void Projection::setUnit(size_t nd, ProjectionUnit unit) {
  if (nd >= 3)
    throw std::invalid_argument("given axis out of range");
  else
    m_units[nd] = unit;
}

} // API
} // Mantid
