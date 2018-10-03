// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAOBJECTS_PEAKSHAPESPHERICAL_H_
#define MANTID_DATAOBJECTS_PEAKSHAPESPHERICAL_H_

#include "MantidDataObjects/PeakShapeBase.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include "MantidKernel/System.h"
#include <boost/optional.hpp>
#include <string>

namespace Mantid {
namespace DataObjects {

/** PeakShapeSpherical : PeakShape for a spherical peak
 */
class DLLExport PeakShapeSpherical : public PeakShapeBase {
public:
  /// Constructor
  PeakShapeSpherical(const double &peakRadius,
                     Kernel::SpecialCoordinateSystem frame,
                     std::string algorithmName = std::string(),
                     int algorithmVersion = -1);
  /// Constructor
  PeakShapeSpherical(const double &peakRadius, const double &peakInnerRadius,
                     const double &peakOuterRadius,
                     Kernel::SpecialCoordinateSystem frame,
                     std::string algorithmName = std::string(),
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
  boost::optional<double>
  radius(RadiusType type = RadiusType::Radius) const override;
  /// Peak outer background radius
  boost::optional<double> backgroundOuterRadius() const;
  /// Peak inner background radius
  boost::optional<double> backgroundInnerRadius() const;
  /// Non-instance shape name
  static const std::string sphereShapeName();

private:
  /// Peak radius
  double m_radius;
  /// Background inner radius;
  boost::optional<double> m_backgroundInnerRadius;
  /// Background outer radius
  boost::optional<double> m_backgroundOuterRadius;
};

} // namespace DataObjects
} // namespace Mantid

#endif /* MANTID_DATAOBJECTS_PEAKSHAPESPHERICAL_H_ */
