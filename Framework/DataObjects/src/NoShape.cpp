// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/NoShape.h"
#include "MantidJson/Json.h"
#include <json/json.h>
#include <stdexcept>

namespace Mantid::DataObjects {

/**
 * @brief Serialize to JSON object
 * @return JSON object as std::string
 */
std::string NoShape::toJSON() const {

  Json::Value root;
  Json::Value shape(this->shapeName());
  root["shape"] = shape;

  return Mantid::JsonHelpers::jsonToString(root);
}

/**
 * @brief Clone object as deep copy
 * @return pointer to new object
 */
NoShape *NoShape::clone() const { return new NoShape; }

std::string NoShape::algorithmName() const { return std::string(); }

int NoShape::algorithmVersion() const { return -1; }

/**
 * @brief Return the unique shape name associated with this type
 * @return Shape name
 */
std::string NoShape::shapeName() const { return NoShape::noShapeName(); }

Kernel::SpecialCoordinateSystem NoShape::frame() const { return Kernel::None; }

const std::string NoShape::noShapeName() { return "none"; }

} // namespace Mantid::DataObjects
