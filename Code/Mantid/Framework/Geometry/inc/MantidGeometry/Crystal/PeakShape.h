#ifndef MANTID_GEOMETRY_PEAKSHAPE_H_
#define MANTID_GEOMETRY_PEAKSHAPE_H_

#include "MantidKernel/System.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include <string>
#include <boost/shared_ptr.hpp>

namespace Mantid {
namespace Geometry {

/** PeakShape : Abstract type to describes the shape of a peak.

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
class DLLExport PeakShape {
public:
  /// Coordinte frame used upon creation
  virtual Mantid::Kernel::SpecialCoordinateSystem frame() const = 0;
  /// Serialize
  virtual std::string toJSON() const = 0;
  /// Deep copy this
  virtual PeakShape *clone() const = 0;
  /// Algorithm
  virtual std::string algorithmName() const = 0;
  /// Algorithm Version
  virtual int algorithmVersion() const = 0;
  /// Shape name
  virtual std::string shapeName() const = 0;
  /// Destructor
  virtual ~PeakShape() {}
};

typedef boost::shared_ptr<PeakShape> PeakShape_sptr;
typedef boost::shared_ptr<const PeakShape> PeakShape_const_sptr;

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_PEAKSHAPE_H_ */
