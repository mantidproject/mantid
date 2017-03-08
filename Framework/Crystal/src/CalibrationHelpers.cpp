#include "MantidAPI/DetectorInfo.h"
#include "MantidAPI/ResizeRectangularDetectorHelper.h"
#include "MantidCrystal/SCDCalibratePanels.h"
#include "MantidCrystal/CalibrationHelpers.h"

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace Mantid {
namespace Crystal {

namespace CalibrationHelpers {
/**
 * Updates the DetectorInfo for the workspace containing newInstrument to
 *reflect the position of the source
 *
 * @param newInstrument The instrument whose parameter map will be changed to
 *reflect the new source position
 * @param L0 The new distance from source to sample (should be positive)
 * @param newSampPos The relative shift for the new sample position
 * @param detectorInfo DetectorInfo for the workspace being updated
 */
void adjustUpSampleAndSourcePositions(const Instrument &newInstrument,
                                      const double L0, const V3D &newSampPos,
                                      DetectorInfo &detectorInfo) {

  if (L0 <= 0)
    throw std::runtime_error("L0 is negative, must be positive.");

  IComponent_const_sptr source = newInstrument.getSource();
  IComponent_const_sptr sample = newInstrument.getSample();

  const V3D &oldSourceToSampleDir =
      detectorInfo.samplePosition() - detectorInfo.sourcePosition();
  const double oldL1 = detectorInfo.l1();

  V3D samplePos = detectorInfo.samplePosition();
  if (samplePos != newSampPos) {
    detectorInfo.setPosition(*sample, newSampPos);
  }

  double scalee = L0 / oldL1;
  V3D newSourcePos = newSampPos - oldSourceToSampleDir * scalee;

  detectorInfo.setPosition(*source, newSourcePos);
}

/**
 * Updates DetectorInfo for newInstrument to reflect the changes in the
 *associated panel information
 *
 * @param bankNames The names of the banks (panels) that will be updated
 * @param newInstrument The instrument whose parameter map will be changed to
 *reflect the new values below
 * @param pos The quantity to be added to the current relative position, from
 *old NewInstrument, of the banks in bankNames.
 * @param rot The quantity to be added to the current relative rotations, from
 *old NewInstrument, of the banks in bankNames.
 * @param detWScale The factor to multiply the current detector width, from old
 *NewInstrument, by to get the new detector width for the banks in bankNames.
 * @param detHtScale The factor to multiply the current detector height, from
 *old NewInstrument, by to get the new detector height for the banks in
 *bankNames.
 * @param detectorInfo DetectorInfo object for the
 */
void adjustBankPositionsAndSizes(const std::vector<std::string> &bankNames,
                                 const Instrument &newInstrument,
                                 const V3D &pos, const Quat &rot,
                                 const double detWScale,
                                 const double detHtScale,
                                 DetectorInfo &detectorInfo) {
  boost::shared_ptr<ParameterMap> pmap = newInstrument.getParameterMap();

  for (const auto &bankName : bankNames) {
    boost::shared_ptr<const IComponent> bank1 =
        newInstrument.getComponentByName(bankName);
    boost::shared_ptr<const Geometry::RectangularDetector> bank =
        boost::dynamic_pointer_cast<const RectangularDetector>(bank1);

    Quat relRot = bank->getRelativeRot();
    Quat parentRot = bank->getParent()->getRotation();
    Quat newRot = parentRot * rot * relRot;

    detectorInfo.setRotation(*bank, newRot);

    V3D rotatedPos = V3D(pos);
    bank->getParent()->getRotation().rotate(rotatedPos);

    detectorInfo.setPosition(*bank, rotatedPos + bank->getPos());

    std::vector<double> oldScalex =
        pmap->getDouble(bank->getName(), std::string("scalex"));
    std::vector<double> oldScaley =
        pmap->getDouble(bank->getName(), std::string("scaley"));

    double scalex, scaley;
    if (!oldScalex.empty())
      scalex = oldScalex[0] * detWScale;
    else
      scalex = detWScale;

    if (!oldScaley.empty())
      scaley = oldScaley[0] * detHtScale;
    else
      scaley = detHtScale;

    pmap->addDouble(bank.get(), std::string("scalex"), scalex);
    pmap->addDouble(bank.get(), std::string("scaley"), scaley);

    if (detWScale != 1.0 || detHtScale != 1.0)
      applyRectangularDetectorScaleToDetectorInfo(detectorInfo, *bank,
                                                  detWScale, detHtScale);
  }
}

} // namespace CalibrationHelpers
} // namespace Crystal
} // namespace Mantid
