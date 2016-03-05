#include "MantidDataObjects/PeakNoShapeFactory.h"
#include "MantidDataObjects/NoShape.h"

namespace Mantid {
namespace DataObjects {

void PeakNoShapeFactory::setSuccessor(
    boost::shared_ptr<const PeakShapeFactory>) {}

/**
 * @brief Creational method
 * @return new NoShape object
 */
Mantid::Geometry::PeakShape *
PeakNoShapeFactory::create(const std::string &) const {
  return new NoShape;
}

} // namespace DataObjects
} // namespace Mantid
