// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/PeakShapeEllipsoidFactory.h"
#include "MantidDataObjects/PeakShapeEllipsoid.h"
#include "MantidJson/Json.h"
#include "MantidKernel/SpecialCoordinateSystem.h"

#include <json/json.h>
#include <stdexcept>

using namespace Mantid::Kernel;

namespace Mantid::DataObjects {

/**
 * @brief Create the PeakShape
 * @param source : source JSON
 * @return PeakShape via this factory or it's successors
 */
Mantid::Geometry::PeakShape *PeakShapeEllipsoidFactory::create(const std::string &source) const {
  Json::Value root;
  Mantid::Geometry::PeakShape *product = nullptr;
  if (Mantid::JsonHelpers::parse(source, &root)) {
    const std::string shape = root["shape"].asString();
    if (shape == PeakShapeEllipsoid::ellipsoidShapeName()) {

      const std::string algorithmName(root["algorithm_name"].asString());
      const int algorithmVersion(root["algorithm_version"].asInt());
      const auto frame(static_cast<SpecialCoordinateSystem>(root["frame"].asInt()));
      PeakEllipsoidExtent abcRadii{root["radius0"].asDouble(), root["radius1"].asDouble(), root["radius2"].asDouble()};
      PeakEllipsoidExtent abcRadiiBackgroundInner{root["background_inner_radius0"].asDouble(),
                                                  root["background_inner_radius1"].asDouble(),
                                                  root["background_inner_radius2"].asDouble()};
      PeakEllipsoidExtent abcRadiiBackgroundOuter{root["background_outer_radius0"].asDouble(),
                                                  root["background_outer_radius1"].asDouble(),
                                                  root["background_outer_radius2"].asDouble()};
      PeakEllipsoidFrame directions;
      directions[0].fromString(root["direction0"].asString());
      directions[1].fromString(root["direction1"].asString());
      directions[2].fromString(root["direction2"].asString());

      product = new PeakShapeEllipsoid(directions, abcRadii, abcRadiiBackgroundInner, abcRadiiBackgroundOuter, frame,
                                       algorithmName, algorithmVersion);

    } else {
      if (m_successor) {
        product = m_successor->create(source);
      } else {
        throw std::invalid_argument("PeakShapeSphericalFactory:: No successor "
                                    "factory able to process : " +
                                    source);
      }
    }

  } else {

    throw std::invalid_argument("PeakShapeSphericalFactory:: Source JSON for "
                                "the peak shape is not valid: " +
                                source);
  }
  return product;
}

/**
 * @brief Set successor
 * @param successorFactory : successor
 */
void PeakShapeEllipsoidFactory::setSuccessor(std::shared_ptr<const PeakShapeFactory> successorFactory) {
  this->m_successor = successorFactory;
}

} // namespace Mantid::DataObjects
