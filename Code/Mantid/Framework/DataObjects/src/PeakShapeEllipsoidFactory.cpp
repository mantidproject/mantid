#include "MantidDataObjects/PeakShapeEllipsoidFactory.h"
#include "MantidDataObjects/PeakShapeEllipsoid.h"
#include "MantidAPI/SpecialCoordinateSystem.h"

#include <jsoncpp/json/json.h>
#include <stdexcept>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace DataObjects {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
PeakShapeEllipsoidFactory::PeakShapeEllipsoidFactory() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
PeakShapeEllipsoidFactory::~PeakShapeEllipsoidFactory() {}

/**
 * @brief Create the PeakShape
 * @param source : source JSON
 * @return PeakShape via this factory or it's successors
 */
PeakShape *PeakShapeEllipsoidFactory::create(const std::string &source) const {
  Json::Reader reader;
  Json::Value root;
  PeakShape *product = NULL;
  if (reader.parse(source, root)) {
    const std::string shape = root["shape"].asString();
    if (shape == PeakShapeEllipsoid::ellipsoidShapeName()) {

      const std::string algorithmName(root["algorithm_name"].asString());
      const int algorithmVersion(root["algorithm_version"].asInt());
      const SpecialCoordinateSystem frame(
          static_cast<SpecialCoordinateSystem>(root["frame"].asInt()));
      std::vector<double> abcRadii, abcRadiiBackgroundInner,
          abcRadiiBackgroundOuter;
      abcRadii.push_back(root["radius0"].asDouble());
      abcRadii.push_back(root["radius1"].asDouble());
      abcRadii.push_back(root["radius2"].asDouble());
      abcRadiiBackgroundInner.push_back(
          root["background_inner_radius0"].asDouble());
      abcRadiiBackgroundInner.push_back(
          root["background_inner_radius1"].asDouble());
      abcRadiiBackgroundInner.push_back(
          root["background_inner_radius2"].asDouble());
      abcRadiiBackgroundOuter.push_back(
          root["background_outer_radius0"].asDouble());
      abcRadiiBackgroundOuter.push_back(
          root["background_outer_radius1"].asDouble());
      abcRadiiBackgroundOuter.push_back(
          root["background_outer_radius2"].asDouble());

      std::vector<V3D> directions(3);
      directions[0].fromString(root["direction0"].asString());
      directions[1].fromString(root["direction1"].asString());
      directions[2].fromString(root["direction2"].asString());

      product = new PeakShapeEllipsoid(
          directions, abcRadii, abcRadiiBackgroundInner,
          abcRadiiBackgroundOuter, frame, algorithmName, algorithmVersion);

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
void PeakShapeEllipsoidFactory::setSuccessor(
    boost::shared_ptr<const PeakShapeFactory> successorFactory) {
  this->m_successor = successorFactory;
}


} // namespace DataObjects
} // namespace Mantid
