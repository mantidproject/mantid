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
  for (size_t i = 0; i < 3; ++i) {
    m_offsets[i] = 0.0;
    m_units[i] = RLU;
  }
}

Projection::Projection(const V3D &u, const V3D &v, const V3D &w) {
  if (fabs(w.scalar_prod(u.cross_prod(v))) <= 0.00001)
    throw std::runtime_error("u, v, and w must not be coplanar!");

  m_dimensions[0] = u;
  m_dimensions[1] = v;
  m_dimensions[2] = w;
  for (size_t i = 0; i < 3; ++i) {
    m_offsets[i] = 0.0;
    m_units[i] = RLU;
  }
}

Projection::Projection(const ITableWorkspace &ws) {
  if (ws.columnCount() != 4)
    throw std::runtime_error(
        "4 columns must be provided to create a projection");

  const size_t numRows = ws.rowCount();
  if (numRows != 3)
    throw std::runtime_error("3 rows must be provided to create a projection");

  Column_const_sptr nameCol = ws.getColumn("name");
  Column_const_sptr valueCol = ws.getColumn("value");
  Column_const_sptr offsetCol = ws.getColumn("offset");
  Column_const_sptr unitCol = ws.getColumn("type");

  for (size_t i = 0; i < numRows; i++) {
    const std::string name = nameCol->cell<std::string>(i);
    const V3D value = valueCol->cell<V3D>(i);
    const double offset = offsetCol->cell<double>(i);
    const std::string unitStr = unitCol->cell<std::string>(i);

    //Check the name
    size_t index;
    if (name == "u") {
      index = 0;
    } else if (name == "v") {
      index = 1;
    } else if (name == "w") {
      index = 2;
    } else {
      throw std::runtime_error("Invalid dimension name: " + name);
    }

    // Check the unit
    ProjectionUnit unit;
    if (unitStr == "r") {
      unit = RLU;
    } else if (unitStr == "a") {
      unit = INV_ANG;
    } else {
      throw std::runtime_error("Unknown type: " + unitStr);
    }

    // Apply the data
    m_dimensions[index] = value;
    m_offsets[index] = offset;
    m_units[index] = unit;
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
