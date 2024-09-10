// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataObjects/PeakShapeBase.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include <optional>
#include <string>

namespace Mantid {
namespace DataObjects {

/** PeakShapeSpherical : PeakShape for a spherical peak
 */
class MANTID_DATAOBJECTS_DLL PeakShapeSpherical : public PeakShapeBase {
public:
  /// Constructor
  PeakShapeSpherical(const double &peakRadius, Kernel::SpecialCoordinateSystem frame,
                     std::string algorithmName = std::string(), int algorithmVersion = -1);
  /// Constructor
  PeakShapeSpherical(const double &peakRadius, const double &peakInnerRadius, const double &peakOuterRadius,
                     Kernel::SpecialCoordinateSystem frame, std::string algorithmName = std::string(),
                     int algorithmVersion = -1);
  /// Serialization method
  std::string toJSON() const override;
  /// Clone the peak shape
  PeakShapeSpherical *clone() const override;
  /// Shape name
  std::string shapeName() const override;
  /// Equals operator
  bool operator==(const PeakShapeSpherical &other) const;
  /// Peak radius
  std::optional<double> radius(RadiusType type = RadiusType::Radius) const override;
  /// Peak outer background radius
  std::optional<double> backgroundOuterRadius() const;
  /// Peak inner background radius
  std::optional<double> backgroundInnerRadius() const;
  /// Non-instance shape name
  static const std::string sphereShapeName();

private:
  /// Peak radius
  double m_radius;
  /// Background inner radius;
  std::optional<double> m_backgroundInnerRadius;
  /// Background outer radius
  std::optional<double> m_backgroundOuterRadius;
};

} // namespace DataObjects
} // namespace Mantid
