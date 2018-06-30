#ifndef MANTID_DATAOBJECTS_PEAKSHAPEELLIPSOID_H_
#define MANTID_DATAOBJECTS_PEAKSHAPEELLIPSOID_H_

#include "MantidDataObjects/PeakShapeBase.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/System.h"
#include "MantidKernel/V3D.h"

namespace Mantid {
namespace DataObjects {

/** PeakShapeEllipsoid : PeakShape representing a 3D ellipsoid

  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
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
