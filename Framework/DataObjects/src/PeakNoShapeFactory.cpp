#include "MantidDataObjects/PeakNoShapeFactory.h"
#include "MantidDataObjects/NoShape.h"

namespace Mantid {
namespace DataObjects {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
PeakNoShapeFactory::PeakNoShapeFactory() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
PeakNoShapeFactory::~PeakNoShapeFactory() {}

void PeakNoShapeFactory::setSuccessor(
    boost::shared_ptr<const PeakShapeFactory> /*successorFactory*/) {}

/**
 * @brief Creational method
 * @return new NoShape object
 */
Mantid::Geometry::PeakShape *
PeakNoShapeFactory::create(const std::string & /*source*/) const {
  return new NoShape;
}

} // namespace DataObjects
} // namespace Mantid
