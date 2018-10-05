// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CRYSTAL_CALIBRATIONHELPERS_H_
#define MANTID_CRYSTAL_CALIBRATIONHELPERS_H_

#include "MantidGeometry/Instrument.h"

namespace Mantid {
namespace Geometry {
class ComponentInfo;
}
namespace Crystal {

/** CalibrationHelpers : This contains helper methods to move source, sample and
  detector positions/rotations for an instrument according to some calibration
  information.
*/
namespace CalibrationHelpers {

DLLExport void
adjustUpSampleAndSourcePositions(double const L0, const Kernel::V3D &newSampPos,
                                 Geometry::ComponentInfo &componentInfo);

DLLExport void
adjustBankPositionsAndSizes(const std::vector<std::string> &bankNames,
                            const Geometry::Instrument &newInstrument,
                            const Kernel::V3D &pos, const Kernel::Quat &rot,
                            const double detWScale, const double detHtScale,
                            Geometry::ComponentInfo &componentInfo);

} // namespace CalibrationHelpers
} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_CALIBRATIONHELPERS_H_ */
