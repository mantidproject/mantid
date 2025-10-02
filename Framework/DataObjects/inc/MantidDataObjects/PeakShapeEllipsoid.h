// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataObjects/PeakShapeBase.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/V3D.h"

namespace Mantid {
namespace DataObjects {

/** PeakShapeEllipsoid : PeakShape representing a 3D ellipsoid
 */
class MANTID_DATAOBJECTS_DLL PeakShapeEllipsoid : public PeakShapeBase {
public:
  /// Constructor
  PeakShapeEllipsoid(const std::array<Mantid::Kernel::V3D, 3> &directions, const std::array<double, 3> &abcRadii,
                     const std::array<double, 3> &abcRadiiBackgroundInner,
                     const std::array<double, 3> &abcRadiiBackgroundOuter, Kernel::SpecialCoordinateSystem frame,
                     std::string algorithmName = std::string(), int algorithmVersion = -1,
                     const Mantid::Kernel::V3D &translation = {0.0, 0.0, 0.0});
  /// Equals operator
  bool operator==(const PeakShapeEllipsoid &other) const;
  /// Get radii
  const std::array<double, 3> &abcRadii() const;
  /// Get background inner radii
  const std::array<double, 3> &abcRadiiBackgroundInner() const;
  /// Get background outer radii
  const std::array<double, 3> &abcRadiiBackgroundOuter() const;
  /// Get ellipsoid directions
  const std::array<Mantid::Kernel::V3D, 3> &directions() const;
  /// Get translation of center
  const Kernel::V3D &translation() const;
  /// Get ellipsoid directions in a specified frame
  std::array<Kernel::V3D, 3> getDirectionInSpecificFrame(Kernel::Matrix<double> &invertedGoniometerMatrix) const;

  /// PeakShape interface
  std::string toJSON() const override;
  /// Clone ellipsoid
  PeakShapeEllipsoid *clone() const override;
  /// Get the peak shape
  std::string shapeName() const override;

  /// PeakBase interface
  std::optional<double> radius(RadiusType type = RadiusType::Radius) const override;
  static const std::string ellipsoidShapeName();

private:
  /// principle axis
  std::array<Mantid::Kernel::V3D, 3> m_directions;
  /// radii
  std::array<double, 3> m_abc_radii;
  /// inner radii
  std::array<double, 3> m_abc_radiiBackgroundInner;
  /// outer radii
  std::array<double, 3> m_abc_radiiBackgroundOuter;
  /// translation of center
  Mantid::Kernel::V3D m_translation;
};

using PeakShapeEllipsoid_sptr = std::shared_ptr<PeakShapeEllipsoid>;
using PeakShapeEllipsoid_const_sptr = std::shared_ptr<const PeakShapeEllipsoid>;

} // namespace DataObjects
} // namespace Mantid
