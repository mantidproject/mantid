// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataObjects/DllConfig.h"
#include "MantidGeometry/Crystal/PeakShape.h"

namespace Mantid {
namespace DataObjects {

/** PeakShapeNone : No peak shape. Null Object. For unintegrated peaks.
 */
class MANTID_DATAOBJECTS_DLL NoShape : public Mantid::Geometry::PeakShape {

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
  std::optional<double> radius(RadiusType) const override { return std::optional<double>{}; }
  /// Return the shape name
  static const std::string noShapeName();
};

} // namespace DataObjects
} // namespace Mantid
