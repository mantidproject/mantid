#ifndef MANTID_CRYSTAL_CALIBRATIONHELPERS_H_
#define MANTID_CRYSTAL_CALIBRATIONHELPERS_H_

#include "MantidAPI/DetectorInfo.h"
#include "MantidGeometry/Instrument.h"

namespace Mantid {
namespace Crystal {

/** CalibrationHelpers : This contains helper methods to move source, sample and
  detector positions/rotations for an instrument according to some calibration
  information.

  Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
namespace CalibrationHelpers {

DLLExport void
adjustUpSampleAndSourcePositions(const Geometry::Instrument &newInstrument,
                                 double const L0, const Kernel::V3D &newSampPos,
                                 API::DetectorInfo &detectorInfo);

DLLExport void
adjustBankPositionsAndSizes(const std::vector<std::string> &bankNames,
                            const Geometry::Instrument &newInstrument,
                            const Kernel::V3D &pos, const Kernel::Quat &rot,
                            const double detWScale, const double detHtScale,
                            API::DetectorInfo &detectorInfo);

} // namespace CalibrationHelpers
} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_CALIBRATIONHELPERS_H_ */