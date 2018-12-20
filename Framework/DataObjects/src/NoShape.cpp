#include "MantidDataObjects/NoShape.h"
#include <json/json.h>
#include <stdexcept>

namespace Mantid {
namespace DataObjects {

/**
 * @brief Serialize to JSON object
 * @return JSON object as std::string
 */
std::string NoShape::toJSON() const {

  Json::Value root;
  Json::Value shape(this->shapeName());
  root["shape"] = shape;

  Json::StyledWriter writer;
  return writer.write(root);
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

} // namespace DataObjects
} // namespace Mantid
