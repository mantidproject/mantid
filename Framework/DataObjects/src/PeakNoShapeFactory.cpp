// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/PeakNoShapeFactory.h"
#include "MantidDataObjects/NoShape.h"

namespace Mantid {
namespace DataObjects {

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
