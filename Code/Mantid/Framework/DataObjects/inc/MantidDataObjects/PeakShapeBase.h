#ifndef MANTID_DATAOBJECTS_PEAKSHAPEBASE_H_
#define MANTID_DATAOBJECTS_PEAKSHAPEBASE_H_

#include "MantidKernel/System.h"
#include "MantidGeometry/Crystal/PeakShape.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include <string>

namespace Json {
// Forward declaration
class Value;
}

namespace Mantid {
namespace DataObjects {

/** PeakShapeBase : Base class for concrete PeakShapes containing common code.

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
class DLLExport PeakShapeBase : public Mantid::Geometry::PeakShape {

public:
  /// Constructor
  PeakShapeBase(Kernel::SpecialCoordinateSystem frame,
                std::string algorithmName = std::string(),
                int algorithmVersion = -1);
  /// Destructor
  virtual ~PeakShapeBase();
  /// Get the coordinate frame
  Kernel::SpecialCoordinateSystem frame() const;
  /// Get the name of the algorithm used to make this shape
  std::string algorithmName() const;
  /// Get the version of the algorithm used to make this shape
  int algorithmVersion() const;

protected:
  /// Copy constructor
  PeakShapeBase(const PeakShapeBase &other);

  /// Assignment operator
  PeakShapeBase &operator=(const PeakShapeBase &other);

  /// Special coordinate system
  Mantid::Kernel::SpecialCoordinateSystem m_frame;

  /// Generating algorithm name
  std::string m_algorithmName;

  /// Generating algorithm version
  int m_algorithmVersion;

  /// Build common parts of outgoing JSON serialization
  void buildCommon(Json::Value &root) const;

  /// Equals operator
  bool operator==(const PeakShapeBase &other) const;
};

} // namespace DataObjects
} // namespace Mantid

#endif /* MANTID_DATAOBJECTS_PEAKSHAPEBASE_H_ */
