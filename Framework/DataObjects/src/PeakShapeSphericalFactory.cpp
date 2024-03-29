// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/PeakShapeSphericalFactory.h"
#include "MantidDataObjects/PeakShapeSpherical.h"
#include "MantidJson/Json.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include "MantidKernel/VMD.h"
#include <json/json.h>

namespace Mantid::DataObjects {

using namespace Mantid::Kernel;

/**
 * @brief PeakShapeSphericalFactory::create : Creational method
 * @param source : Source JSON
 * @return PeakShape object
 */
Mantid::Geometry::PeakShape *PeakShapeSphericalFactory::create(const std::string &source) const {
  Json::Value root;
  Mantid::Geometry::PeakShape *product = nullptr;
  if (Mantid::JsonHelpers::parse(source, &root)) {
    const std::string shape = root["shape"].asString();
    if (shape == PeakShapeSpherical::sphereShapeName()) {

      const std::string algorithmName(root["algorithm_name"].asString());
      const int algorithmVersion(root["algorithm_version"].asInt());
      const auto frame(static_cast<SpecialCoordinateSystem>(root["frame"].asInt()));
      const double radius(root["radius"].asDouble());

      if (!root["background_outer_radius"].empty() && !root["background_inner_radius"].empty()) {
        const double backgroundOuterRadius(root["background_outer_radius"].asDouble());
        const double backgroundInnerRadius(root["background_inner_radius"].asDouble());
        product = new PeakShapeSpherical(radius, backgroundInnerRadius, backgroundOuterRadius, frame, algorithmName,
                                         algorithmVersion);
      }

      else {

        product = new PeakShapeSpherical(radius, frame, algorithmName, algorithmVersion);
      }
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
void PeakShapeSphericalFactory::setSuccessor(PeakShapeFactory_const_sptr successorFactory) {
  m_successor = successorFactory;
}

} // namespace Mantid::DataObjects
