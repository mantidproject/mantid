#include "MantidDataObjects/NoShape.h"
#include <stdexcept>
#include <jsoncpp/json/json.h>

namespace Mantid {
namespace DataObjects {

/**
 * @brief Constructor
 */
NoShape::NoShape() {}

/**
 * @brief Destructor
 */
NoShape::~NoShape() {}

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

std::string NoShape::algorithmName() const
{
    return std::string();
}

int NoShape::algorithmVersion() const
{
    return -1;
}

/**
 * @brief Return the unique shape name associated with this type
 * @return Shape name
 */
std::string NoShape::shapeName() const { return "none"; }

API::SpecialCoordinateSystem NoShape::frame() const
{
    return API::None;
}


} // namespace DataObjects
} // namespace Mantid
