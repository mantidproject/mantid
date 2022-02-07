// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/Crystal/PeakShape.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace DataObjects {

/** PeakShapeNone : No peak shape. Null Object. For unintegrated peaks.
 */
class DLLExport NoShape : public Mantid::Geometry::PeakShape {

public:
  /// Serialize
  std::string toJSON() const override;
  /// Clone
  NoShape *clone() const override;
  /// Return the algorithn name
  std::string algorithmName() const override;
  /// Return the algorithm version
  int algorithmVersion() const override;
  /// Return the shape name
  std::string shapeName() const override;
  /// Get the coordinate frame
  Kernel::SpecialCoordinateSystem frame() const override;
  boost::optional<double> radius(RadiusType) const override { return boost::optional<double>{}; }
  /// Return the shape name
  static const std::string noShapeName();
};

} // namespace DataObjects
} // namespace Mantid
