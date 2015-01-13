#include "MantidDataObjects/PeakShapeSpherical.h"
#include <stdexcept>
#include <jsoncpp/json/json.h>

namespace Mantid {
namespace DataObjects {

PeakShapeSpherical::PeakShapeSpherical(const double &peakRadius,
                                       API::SpecialCoordinateSystem frame,
                                       std::string algorithmName,
                                       int algorithmVersion)
    : PeakShapeBase(frame, algorithmName, algorithmVersion),
      m_radius(peakRadius) {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
PeakShapeSpherical::~PeakShapeSpherical() {}

/**
 * @brief Copy constructor
 * @param other : source of the copy
 */
PeakShapeSpherical::PeakShapeSpherical(const PeakShapeSpherical &other)
    : PeakShapeBase(other), m_radius(other.radius()) {}

/**
 * @brief Assignment operator
 * @param other : source of the assignment
 * @return Ref to assigned object.
 */
PeakShapeSpherical &PeakShapeSpherical::
operator=(const PeakShapeSpherical &other) {
  if (this != &other) {
    PeakShapeBase::operator=(other);
    m_radius = other.radius();
  }
  return *this;
}

/**
 * @brief Serialize to JSON object
 * @return JSON object as std::string
 */
std::string PeakShapeSpherical::toJSON() const {
  Json::Value root;
  PeakShapeBase::buildCommon(root);
  root["radius"] = Json::Value(radius());

  Json::StyledWriter writer;
  return writer.write(root);
}

/**
 * @brief Clone object as deep copy
 * @return pointer to new object
 */
PeakShapeSpherical *PeakShapeSpherical::clone() const {
  return new PeakShapeSpherical(*this);
}

std::string PeakShapeSpherical::shapeName() const { return "spherical"; }

bool PeakShapeSpherical::operator==(const PeakShapeSpherical &other) const {
  return PeakShapeBase::operator==(other) && other.radius() == this->radius();
}

/**
 * @brief Get radius of sphere
 * @return radius
 */
double PeakShapeSpherical::radius() const { return m_radius; }

} // namespace DataObjects
} // namespace Mantid
