// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/CalibrationHelpers.h"
#include "MantidAPI/ResizeRectangularDetectorHelper.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace Mantid::Crystal::CalibrationHelpers {
/**
 * Updates the ComponentInfo for the workspace containing newInstrument to
 *reflect the position of the source
 *
 *reflect the new source position
 * @param L0 The new distance from source to sample (should be positive)
 * @param newSampPos The relative shift for the new sample position
 * @param componentInfo ComponentInfo for the workspace being updated
 */
void adjustUpSampleAndSourcePositions(const double L0, const V3D &newSampPos, ComponentInfo &componentInfo) {

  if (L0 <= 0)
    throw std::runtime_error("L0 is negative, must be positive.");

  const V3D &oldSourceToSampleDir = componentInfo.samplePosition() - componentInfo.sourcePosition();
  const double oldL1 = componentInfo.l1();

  V3D samplePos = componentInfo.samplePosition();
  if (samplePos != newSampPos) {
    componentInfo.setPosition(componentInfo.sample(), newSampPos);
  }

  double scalee = L0 / oldL1;
  V3D newSourcePos = newSampPos - oldSourceToSampleDir * scalee;

  componentInfo.setPosition(componentInfo.source(), newSourcePos);
}

/**
 * Updates ComponentInfo to reflect the changes in the associated panel
 *information
 *
 * @param bankNames The names of the banks (panels) that will be updated
 * @param pos The quantity to be added to the current relative position of the
 *banks in bankNames.
 * @param rot The quantity to be added to the current relative rotations of the
 *banks in bankNames.
 * @param detWScale The factor to multiply the current detector width by to get
 *the new detector width for the banks in bankNames.
 * @param detHtScale The factor to multiply the current detector height by to
 *get the new detector height for the banks in bankNames.
 * @param componentInfo ComponentInfo object for the modifications
 */
void adjustBankPositionsAndSizes(const std::vector<std::string> &bankNames, const V3D &pos, const Quat &rot,
                                 const double detWScale, const double detHtScale, ComponentInfo &componentInfo) {
  for (const auto &bankName : bankNames) {
    const size_t bankComponentIndex = componentInfo.indexOfAny(bankName);
    const size_t parentIndex = componentInfo.parent(bankComponentIndex);

    Quat relRot = componentInfo.relativeRotation(bankComponentIndex);
    Quat parentRot = componentInfo.rotation(parentIndex);
    Quat newRot = parentRot * rot * relRot;

    componentInfo.setRotation(bankComponentIndex, newRot);

    V3D rotatedPos = V3D(pos);
    parentRot.rotate(rotatedPos);

    componentInfo.setPosition(bankComponentIndex, rotatedPos + componentInfo.position(bankComponentIndex));

    std::vector<double> oldScalex = componentInfo.getNumberParameter(bankComponentIndex, "scalex", false);
    std::vector<double> oldScaley = componentInfo.getNumberParameter(bankComponentIndex, "scaley", false);

    double scalex, scaley;
    if (!oldScalex.empty())
      scalex = oldScalex[0] * detWScale;
    else
      scalex = detWScale;

    if (!oldScaley.empty())
      scaley = oldScaley[0] * detHtScale;
    else
      scaley = detHtScale;

    componentInfo.addDouble(bankComponentIndex, "scalex", scalex);
    componentInfo.addDouble(bankComponentIndex, "scaley", scaley);

    if (detWScale != 1.0 || detHtScale != 1.0)
      applyRectangularDetectorScaleToComponentInfo(componentInfo, bankComponentIndex, detWScale, detHtScale);
  }
}

} // namespace Mantid::Crystal::CalibrationHelpers
