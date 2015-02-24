#include "MantidAPI/Projection.h"

namespace Mantid {
namespace API {

Projection::Projection()
    : m_nd(2), m_dimensions(new VMD[m_nd]), m_offsets(new double[m_nd]),
      m_units(new ProjectionUnit[m_nd]) {
  m_dimensions[0] = VMD(m_nd);
  m_dimensions[1] = VMD(m_nd);
  //Identity
  m_dimensions[0][0] = 1.0;
  m_dimensions[1][1] = 1.0;
  m_offsets[0] = 0.0;
  m_offsets[1] = 0.0;
  m_units[0] = RLU;
  m_units[1] = RLU;
}

Projection::Projection(size_t nd)
    : m_nd(nd), m_units(new ProjectionUnit[m_nd]) {
  if (m_nd <= 1)
    throw std::invalid_argument("nd must be > 1");

  m_dimensions = new VMD[m_nd];
  m_offsets = new double[m_nd];

  for (size_t i = 0; i < m_nd; ++i) {
    m_dimensions[i] = VMD(m_nd);
    //We default to an identity projection
    m_dimensions[i][i] = 1.0;
    m_offsets[i] = 0.0;
    m_units[i] = RLU;
  }
}

Projection::Projection(const VMD &u, const VMD &v)
    : m_nd(2), m_dimensions(new VMD[m_nd]), m_offsets(new double[m_nd]),
      m_units(new ProjectionUnit[m_nd]) {
  m_dimensions[0] = u;
  m_dimensions[1] = v;
  m_offsets[0] = 0.0;
  m_offsets[1] = 0.0;
  m_units[0] = RLU;
  m_units[1] = RLU;
}

Projection::Projection(const VMD &u, const VMD &v, const VMD &w)
    : m_nd(3), m_dimensions(new VMD[m_nd]), m_offsets(new double[m_nd]),
      m_units(new ProjectionUnit[m_nd]) {
  m_dimensions[0] = u;
  m_dimensions[1] = v;
  m_dimensions[2] = w;
  for (size_t i = 0; i < m_nd; ++i) {
    m_offsets[i] = 0.0;
    m_units[i] = RLU;
  }
}

Projection::Projection(const VMD &u, const VMD &v, const VMD &w, const VMD &x)
    : m_nd(4), m_dimensions(new VMD[m_nd]), m_offsets(new double[m_nd]),
      m_units(new ProjectionUnit[m_nd]) {
  m_dimensions[0] = u;
  m_dimensions[1] = v;
  m_dimensions[2] = w;
  m_dimensions[3] = x;
  for (size_t i = 0; i < m_nd; ++i) {
    m_offsets[i] = 0.0;
    m_units[i] = RLU;
  }
}

Projection::Projection(const VMD &u, const VMD &v, const VMD &w, const VMD &x,
                       const VMD &y)
    : m_nd(5), m_dimensions(new VMD[m_nd]), m_offsets(new double[m_nd]),
      m_units(new ProjectionUnit[m_nd]) {
  m_dimensions[0] = u;
  m_dimensions[1] = v;
  m_dimensions[2] = w;
  m_dimensions[3] = x;
  m_dimensions[4] = y;
  for (size_t i = 0; i < m_nd; ++i) {
    m_offsets[i] = 0.0;
    m_units[i] = RLU;
  }
}

Projection::Projection(const VMD &u, const VMD &v, const VMD &w, const VMD &x,
                       const VMD &y, const VMD &z)
    : m_nd(6), m_dimensions(new VMD[m_nd]), m_offsets(new double[m_nd]),
      m_units(new ProjectionUnit[m_nd]) {
  m_dimensions[0] = u;
  m_dimensions[1] = v;
  m_dimensions[2] = w;
  m_dimensions[3] = x;
  m_dimensions[4] = y;
  m_dimensions[5] = z;
  for (size_t i = 0; i < m_nd; ++i) {
    m_offsets[i] = 0.0;
    m_units[i] = RLU;
  }
}

Projection::Projection(const Projection &other) : m_nd(other.m_nd) {
  if (m_nd <= 0)
    throw std::invalid_argument("nd must be > 0");
  m_dimensions = new VMD[m_nd];
  m_offsets = new double[m_nd];
  m_units = new ProjectionUnit[m_nd];
  for (size_t i = 0; i < m_nd; ++i) {
    m_dimensions[i] = other.m_dimensions[i];
    m_offsets[i] = other.m_offsets[i];
    m_units[i] = other.m_units[i];
  }
}

Projection &Projection::operator=(const Projection &other) {
  if (m_nd != other.m_nd) {
    m_nd = other.m_nd;
    delete[] m_dimensions;
    delete[] m_offsets;
    delete[] m_units;
    m_dimensions = new VMD[m_nd];
    m_offsets = new double[m_nd];
    m_units = new ProjectionUnit[m_nd];
  }
  for (size_t i = 0; i < m_nd; ++i) {
    m_dimensions[i] = other.m_dimensions[i];
    m_offsets[i] = other.m_offsets[i];
    m_units[i] = other.m_units[i];
  }
  return *this;
}

Projection::~Projection() {
  delete[] m_dimensions;
  delete[] m_offsets;
  delete[] m_units;
}

double Projection::getOffset(size_t nd) {
  if (nd >= m_nd)
    throw std::invalid_argument("given axis out of range");
  else
    return m_offsets[nd];
}

VMD Projection::getAxis(size_t nd) {
  if (nd >= m_nd)
    throw std::invalid_argument("given axis out of range");
  else
    return m_dimensions[nd];
}

ProjectionUnit Projection::getUnit(size_t nd) {
  if (nd >= m_nd)
    throw std::invalid_argument("given axis out of range");
  else
    return m_units[nd];
}

void Projection::setOffset(size_t nd, double offset) {
  if (nd >= m_nd)
    throw std::invalid_argument("given axis out of range");
  else
    m_offsets[nd] = offset;
}

void Projection::setAxis(size_t nd, VMD axis) {
  if (nd >= m_nd)
    throw std::invalid_argument("given axis out of range");
  else
    m_dimensions[nd] = axis;
}

void Projection::setUnit(size_t nd, ProjectionUnit unit) {
  if (nd >= m_nd)
    throw std::invalid_argument("given axis out of range");
  else
    m_units[nd] = unit;
}

} // API
} // Mantid
