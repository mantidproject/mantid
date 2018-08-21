#include "MantidCrystal/CalibrationHelpers.h"
#include "MantidAPI/ResizeRectangularDetectorHelper.h"
#include "MantidCrystal/SCDCalibratePanels.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace Mantid {
namespace Crystal {

namespace CalibrationHelpers {
/**
 * Updates the ComponentInfo for the workspace containing newInstrument to
 *reflect the position of the source
 *
 *reflect the new source position
 * @param L0 The new distance from source to sample (should be positive)
 * @param newSampPos The relative shift for the new sample position
 * @param componentInfo ComponentInfo for the workspace being updated
 */
void adjustUpSampleAndSourcePositions(const double L0, const V3D &newSampPos,
                                      ComponentInfo &componentInfo) {

  if (L0 <= 0)
    throw std::runtime_error("L0 is negative, must be positive.");

  const V3D &oldSourceToSampleDir =
      componentInfo.samplePosition() - componentInfo.sourcePosition();
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
 * Updates ComponentInfo for newInstrument to reflect the changes in the
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
 * @param componentInfo ComponentInfo object for the modifications
 */
void adjustBankPositionsAndSizes(const std::vector<std::string> &bankNames,
                                 const Instrument &newInstrument,
                                 const V3D &pos, const Quat &rot,
                                 const double detWScale,
                                 const double detHtScale,
                                 ComponentInfo &componentInfo) {
  boost::shared_ptr<ParameterMap> pmap = newInstrument.getParameterMap();

  for (const auto &bankName : bankNames) {
    boost::shared_ptr<const IComponent> bank1 =
        newInstrument.getComponentByName(bankName);
    boost::shared_ptr<const Geometry::RectangularDetector> bank =
        boost::dynamic_pointer_cast<const RectangularDetector>(bank1);

    Quat relRot = bank->getRelativeRot();
    Quat parentRot = bank->getParent()->getRotation();
    Quat newRot = parentRot * rot * relRot;

    const auto bankComponentIndex =
        componentInfo.indexOf(bank->getComponentID());
    componentInfo.setRotation(bankComponentIndex, newRot);

    V3D rotatedPos = V3D(pos);
    bank->getParent()->getRotation().rotate(rotatedPos);

    componentInfo.setPosition(bankComponentIndex, rotatedPos + bank->getPos());

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
      applyRectangularDetectorScaleToComponentInfo(
          componentInfo, bank->getComponentID(), detWScale, detHtScale);
  }
}

} // namespace CalibrationHelpers
} // namespace Crystal
} // namespace Mantid
