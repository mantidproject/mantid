// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAOBJECTS_PEAKSHAPEELLIPSOID_H_
#define MANTID_DATAOBJECTS_PEAKSHAPEELLIPSOID_H_

#include "MantidDataObjects/PeakShapeBase.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/System.h"
#include "MantidKernel/V3D.h"

namespace Mantid {
namespace DataObjects {

/** PeakShapeEllipsoid : PeakShape representing a 3D ellipsoid
 */
class DLLExport PeakShapeEllipsoid : public PeakShapeBase {
public:
  /// Constructor
  PeakShapeEllipsoid(const std::vector<Mantid::Kernel::V3D> &directions,
                     const std::vector<double> &abcRadii,
                     const std::vector<double> &abcRadiiBackgroundInner,
                     const std::vector<double> &abcRadiiBackgroundOuter,
                     Kernel::SpecialCoordinateSystem frame,
                     std::string algorithmName = std::string(),
                     int algorithmVersion = -1);
  /// Equals operator
  bool operator==(const PeakShapeEllipsoid &other) const;
  /// Get radii
  const std::vector<double> &abcRadii() const;
  /// Get background inner radii
  const std::vector<double> &abcRadiiBackgroundInner() const;
  /// Get background outer radii
  const std::vector<double> &abcRadiiBackgroundOuter() const;
  /// Get ellipsoid directions
  const std::vector<Mantid::Kernel::V3D> &directions() const;
  /// Get ellipsoid directions in a specified frame
  std::vector<Kernel::V3D> getDirectionInSpecificFrame(
      Kernel::Matrix<double> &invertedGoniometerMatrix) const;

  /// PeakShape interface
  std::string toJSON() const override;
  /// Clone ellipsoid
  PeakShapeEllipsoid *clone() const override;
  /// Get the peak shape
  std::string shapeName() const override;

  /// PeakBase interface
  boost::optional<double>
  radius(RadiusType type = RadiusType::Radius) const override;
  static const std::string ellipsoidShapeName();

private:
  /// principle axis
  std::vector<Mantid::Kernel::V3D> m_directions;
  /// radii
  std::vector<double> m_abc_radii;
  /// inner radii
  std::vector<double> m_abc_radiiBackgroundInner;
  /// outer radii
  std::vector<double> m_abc_radiiBackgroundOuter;
};

using PeakShapeEllipsoid_sptr = boost::shared_ptr<PeakShapeEllipsoid>;
using PeakShapeEllipsoid_const_sptr =
    boost::shared_ptr<const PeakShapeEllipsoid>;

} // namespace DataObjects
} // namespace Mantid

#endif /* MANTID_DATAOBJECTS_PEAKSHAPEELLIPSOID_H_ */
