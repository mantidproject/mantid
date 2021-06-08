// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/System.h"
#include <memory>

namespace Mantid {
namespace Geometry {
// Forward declaration
class PeakShape;
} // namespace Geometry
namespace DataObjects {

/** PeakShapeFactory : Factory for creating peak shapes
 */
class DLLExport PeakShapeFactory {
public:
  /// Destructor
  virtual ~PeakShapeFactory() = default;
  /// Make the product
  virtual Mantid::Geometry::PeakShape *create(const std::string &source) const = 0;
  /// Set the successor factory. create will be called on that if this instance
  /// is not suitable.
  virtual void setSuccessor(std::shared_ptr<const PeakShapeFactory> successorFactory) = 0;
};

/// Helper typedef
using PeakShapeFactory_sptr = std::shared_ptr<PeakShapeFactory>;
/// Helper typedef
using PeakShapeFactory_const_sptr = std::shared_ptr<const PeakShapeFactory>;

} // namespace DataObjects
} // namespace Mantid
