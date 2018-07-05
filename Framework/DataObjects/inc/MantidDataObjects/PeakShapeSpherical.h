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
