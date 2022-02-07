// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataObjects/PeakShapeFactory.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Geometry {
// Forward declaration
class PeakShape;
} // namespace Geometry
namespace DataObjects {
/** PeakNoShapeFactory : Factory method for types of NoShape
 */
class DLLExport PeakNoShapeFactory : public PeakShapeFactory {
public:
  // Factory method
  Mantid::Geometry::PeakShape *create(const std::string &source) const override;
  // Set successor. No shape will not delegate.
  void setSuccessor(std::shared_ptr<const PeakShapeFactory> successorFactory) override;

private:
};

} // namespace DataObjects
} // namespace Mantid
