#ifndef MANTID_DATAOBJECTS_PEAKSHAPENONE_H_
#define MANTID_DATAOBJECTS_PEAKSHAPENONE_H_

#include "MantidKernel/System.h"
#include "MantidDataObjects/PeakShapeBase.h"

namespace Mantid {
namespace DataObjects {

/** PeakShapeNone : No peak shape positional representation only.

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
class DLLExport PeakShapeNone : public PeakShapeBase {
public:
  /// Constructor
  PeakShapeNone(const Mantid::Kernel::VMD &peakCentre,
                API::SpecialCoordinateSystem frame,
                std::string algorithmName = std::string(),
                int algorithmVersion = -1);
  /// Destructor
  virtual ~PeakShapeNone();
  /// Copy constructor
  PeakShapeNone(const PeakShapeNone &other);
  /// Assignment operator
  PeakShapeNone &operator=(const PeakShapeNone &other);
  /// Serialization method
  virtual std::string toJSON() const;
  /// Clone the peak shape
  virtual PeakShapeNone *clone() const;
  /// Shape name
  std::string shapeName() const;
  /// Equals operator
  bool operator==(const PeakShapeNone &other) const;
};

} // namespace DataObjects
} // namespace Mantid

#endif /* MANTID_DATAOBJECTS_PEAKSHAPENONE_H_ */
