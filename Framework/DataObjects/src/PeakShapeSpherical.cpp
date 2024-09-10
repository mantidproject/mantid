// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/PeakShapeSpherical.h"
#include "MantidJson/Json.h"
#include <json/json.h>
#include <stdexcept>
#include <utility>

namespace Mantid::DataObjects {

/**
 * @brief Constructor
 * @param peakRadius : Peak radius
 * @param frame : Coordinate frame used for the integration
 * @param algorithmName : Algorithm name used for the integration
 * @param algorithmVersion : Algorithm version used for the integration
 */
PeakShapeSpherical::PeakShapeSpherical(const double &peakRadius, Kernel::SpecialCoordinateSystem frame,
                                       std::string algorithmName, int algorithmVersion)
    : PeakShapeBase(frame, std::move(algorithmName), algorithmVersion), m_radius(peakRadius) {}

/**
 * @brief PeakShapeSpherical::PeakShapeSpherical
 * @param peakRadius : Peak radius
 * @param peakInnerRadius : Peak inner radius
 * @param peakOuterRadius : Peak outer radius
 * @param frame : Coordinate frame used for the integration
 * @param algorithmName : Algorithm name
 * @param algorithmVersion : Algorithm version
 */
PeakShapeSpherical::PeakShapeSpherical(const double &peakRadius, const double &peakInnerRadius,
                                       const double &peakOuterRadius, Kernel::SpecialCoordinateSystem frame,
                                       std::string algorithmName, int algorithmVersion)
    : PeakShapeBase(frame, std::move(algorithmName), algorithmVersion), m_radius(peakRadius),
      m_backgroundInnerRadius(peakInnerRadius), m_backgroundOuterRadius(peakOuterRadius) {
  if (peakRadius == m_backgroundOuterRadius) {
    m_backgroundOuterRadius.reset();
  }
}

/**
 * @brief Serialize to JSON object
 * @return JSON object as std::string
 */
std::string PeakShapeSpherical::toJSON() const {
  Json::Value root;
  PeakShapeBase::buildCommon(root);
  root["radius"] = Json::Value(m_radius);

  if (m_backgroundInnerRadius && m_backgroundOuterRadius) {
    root["background_outer_radius"] = Json::Value(m_backgroundOuterRadius.value());
    root["background_inner_radius"] = Json::Value(m_backgroundInnerRadius.value());
  }

  return Mantid::JsonHelpers::jsonToString(root);
}

/**
 * @brief Clone object as deep copy
 * @return pointer to new object
 */
PeakShapeSpherical *PeakShapeSpherical::clone() const { return new PeakShapeSpherical(*this); }

std::string PeakShapeSpherical::shapeName() const { return sphereShapeName(); }

bool PeakShapeSpherical::operator==(const PeakShapeSpherical &other) const {
  return PeakShapeBase::operator==(other) && other.radius() == this->radius() &&
         other.backgroundInnerRadius() == this->backgroundInnerRadius() &&
         other.backgroundOuterRadius() == this->backgroundOuterRadius();
}

/**
 * @brief Get radius of sphere
 * @param type Which radius to get.
 * @return radius
 */
std::optional<double> PeakShapeSpherical::radius(RadiusType type) const {

  std::optional<double> value;
  switch (type) {
  case (RadiusType::Radius):
    value = std::optional<double>{m_radius};
    break;
  case (RadiusType::OuterRadius):
    value = m_backgroundOuterRadius;
    break;
  case (RadiusType::InnerRadius):
    value = m_backgroundInnerRadius;
    break;
  }
  return value;
}

/**
 * @brief Get the background outer radius. The outer radius may not be set, so
 * this is optional.
 * @return std::optional outer radius
 */
std::optional<double> PeakShapeSpherical::backgroundOuterRadius() const { return m_backgroundOuterRadius; }

/**
 * @brief Get the background inner radius. The inner radius may not be set, so
 * this is optional.
 * @return std::optional inner radius.
 */
std::optional<double> PeakShapeSpherical::backgroundInnerRadius() const { return m_backgroundInnerRadius; }

/**
 * @brief PeakShapeSpherical::sphereShapeName
 * @return Spherical shape name for this type.
 */
const std::string PeakShapeSpherical::sphereShapeName() { return "spherical"; }

} // namespace Mantid::DataObjects
