#ifndef MANTID_DATAOBJECTS_NOSHAPE_H_
#define MANTID_DATAOBJECTS_NOSHAPE_H_

#include "MantidGeometry/Crystal/PeakShape.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace DataObjects {

/** PeakShapeNone : No peak shape. Null Object. For unintegrated peaks.

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
  boost::optional<double> radius(RadiusType) const override {
    return boost::optional<double>{};
  }
  /// Return the shape name
  static const std::string noShapeName();
};

} // namespace DataObjects
} // namespace Mantid

#endif /* MANTID_DATAOBJECTS_NOSHAPE_H_ */
