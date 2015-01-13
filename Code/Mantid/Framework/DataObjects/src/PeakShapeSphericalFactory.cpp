#include "MantidDataObjects/PeakShapeSphericalFactory.h"
#include "MantidDataObjects/PeakShapeSpherical.h"
#include "MantidAPI/SpecialCoordinateSystem.h"
#include <jsoncpp/json/json.h>
#include <MantidKernel/VMD.h>

namespace Mantid {
namespace DataObjects {

using namespace Mantid::API;

//----------------------------------------------------------------------------------------------
/** Constructor
 */
PeakShapeSphericalFactory::PeakShapeSphericalFactory() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
PeakShapeSphericalFactory::~PeakShapeSphericalFactory() {}

/**
 * @brief PeakShapeSphericalFactory::create : Creational method
 * @param source : Source JSON
 * @return PeakShape object
 */
PeakShape *PeakShapeSphericalFactory::create(const std::string &source) const {
  Json::Reader reader;
  Json::Value root;
  PeakShape *product = NULL;
  if (reader.parse(source, root)) {
    const std::string shape = root["shape"].asString();
    if (shape == "spherical") {

      const std::string algorithmName(root["algorithm_name"].asString());
      const int algorithmVersion(root["algorithm_version"].asInt());
      const SpecialCoordinateSystem frame(
          static_cast<SpecialCoordinateSystem>(root["frame"].asInt()));
      const double radius(root["radius"].asDouble());
      Json::Value centre(root["centre"]);
      Kernel::VMD centre_nd(centre.size());
      for (Json::ArrayIndex i = 0; i < centre.size(); ++i) {
        centre_nd[i] = centre[i].asFloat();
      }
      product = new PeakShapeSpherical(centre_nd, radius, frame, algorithmName,
                                       algorithmVersion);
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
    if (m_successor) {
      product = m_successor->create(source);
    } else {
      throw std::invalid_argument("PeakShapeSphericalFactory:: Source JSON for "
                                  "the peak shape is not valid: " +
                                  source);
    }
  }
  return product;
}

void PeakShapeSphericalFactory::setSuccessor(
    PeakShapeFactory_const_sptr successorFactory) {
  m_successor = successorFactory;
}

} // namespace DataObjects
} // namespace Mantid
