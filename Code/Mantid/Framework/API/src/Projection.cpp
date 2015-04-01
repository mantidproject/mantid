#include "MantidAPI/Projection.h"

#include <boost/algorithm/string.hpp>

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

Projection::Projection(ITableWorkspace_const_sptr ws) {
  if (!ws) {
    throw std::runtime_error(
        "Null ITableWorkspace given to Projection constructor");
  }

  const size_t numRows = ws->rowCount();
  if (numRows != 3)
    throw std::runtime_error("3 rows must be provided to create a projection");

  if (ws->getColumn("name")->size() != numRows)
    throw std::runtime_error("Insufficient values in 'name' column.");
  if (ws->getColumn("value")->size() != numRows)
    throw std::runtime_error("Insufficient values in 'value' column.");
  if (ws->getColumn("offset")->size() != numRows)
    throw std::runtime_error("Insufficient values in 'offset' column.");
  if (ws->getColumn("type")->size() != numRows)
    throw std::runtime_error("Insufficient values in 'unit' column.");

  for (size_t i = 0; i < numRows; i++) {
    const std::string name = ws->getColumn("name")->cell<std::string>(i);
    const std::string valueStr = ws->getColumn("value")->cell<std::string>(i);
    const double offset = ws->getColumn("offset")->cell<double>(i);
    const std::string unitStr = ws->getColumn("type")->cell<std::string>(i);

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

    // Check the values
    std::vector<std::string> valueStrVec;
    boost::split(valueStrVec, valueStr, boost::is_any_of(","));
    std::vector<double> valueDblVec;
    for (auto it = valueStrVec.begin(); it != valueStrVec.end(); ++it)
      valueDblVec.push_back(boost::lexical_cast<double>(*it));
    if (valueDblVec.size() != 3) {
      throw std::runtime_error("Dimension " + name + " must contain 3 values");
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
    m_dimensions[index] = V3D(valueDblVec[0], valueDblVec[1], valueDblVec[2]);
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
