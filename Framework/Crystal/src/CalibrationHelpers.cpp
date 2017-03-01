#include "MantidAPI/DetectorInfo.h"
#include "MantidCrystal/SCDCalibratePanels.h"
#include "MantidCrystal/CalibrationHelpers.h"

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace Mantid {
namespace Crystal {

namespace CalibrationHelpers {

constexpr double RAD_TO_DEG = 180. / M_PI;

/**
 * Converts a Quaternion to a corresponding matrix produce Rotx*Roty*Rotz,
 * corresponding to the order Mantid uses in calculating rotations
 * @param Q The Quaternion. It will be normalized to represent a rotation
 * @param Rotx The angle in degrees for rotating around the x-axis
 * @param Roty The angle in degrees for rotating around the y-axis
 * @param Rotz The angle in degrees for rotating around the z-axis
 */
void quatToRotxRotyRotz(const Quat Q, double &Rotx, double &Roty,
                        double &Rotz) {
  Quat R(Q);
  R.normalize();
  V3D X(1, 0, 0);
  V3D Y(0, 1, 0);
  V3D Z(0, 0, 1);
  R.rotate(X);
  R.rotate(Y);
  R.rotate(Z);
  if (Z[1] != 0 || Z[2] != 0) {
    double tx = atan2(-Z[1], Z[2]);
    double tz = atan2(-Y[0], X[0]);
    double cosy = Z[2] / cos(tx);
    double ty = atan2(Z[0], cosy);
    Rotx = (tx * RAD_TO_DEG);
    Roty = (ty * RAD_TO_DEG);
    Rotz = (tz * RAD_TO_DEG);
  } else // roty = 90 0r 270 def
  {
    double k = 1;
    if (Z[0] < 0)
      k = -1;
    double roty = k * 90;
    double rotx = 0;
    double rotz = atan2(X[2], Y[2]);

    Rotx = (rotx * RAD_TO_DEG);
    Roty = (roty * RAD_TO_DEG);
    Rotz = (rotz * RAD_TO_DEG);
  }
}

/**
 * Updates the ParameterMap for NewInstrument to reflect the position of the
 *source.
 *
 * @param newInstrument The instrument whose parameter map will be changed to
 *reflect the new source position
 * @param L0 The new distance from source to sample (should be positive)
 * @param newSampPos The relative shift for the new sample position
 * @param detectorInfo DetectorInfo for the workspace being updated
 */
void fixUpSampleAndSourcePositions(
    boost::shared_ptr<const Instrument> newInstrument, double const L0,
    const V3D newSampPos, DetectorInfo &detectorInfo) {

  IComponent_const_sptr source = newInstrument->getSource();
  IComponent_const_sptr sample = newInstrument->getSample();

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
 * Updates the ParameterMap for newInstrument to reflect the changes in the
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
 * @param rotCenters Rotate the centers of the panels(the same amount) with the
 *rotation of panels around their center
 * @param detectorInfo DetectorInfo object for the
 */
void fixUpBankPositionsAndSizes(
    const std::vector<std::string> bankNames,
    boost::shared_ptr<const Instrument> newInstrument, const V3D pos,
    const Quat rot, double const detWScale, double const detHtScale,
    DetectorInfo &detectorInfo) {
  boost::shared_ptr<ParameterMap> pmap = newInstrument->getParameterMap();

  for (const auto &bankName : bankNames) {
    boost::shared_ptr<const IComponent> bank1 =
        newInstrument->getComponentByName(bankName);
    boost::shared_ptr<const Geometry::RectangularDetector> bank =
        boost::dynamic_pointer_cast<const RectangularDetector>(bank1);

    Quat relRot = bank->getRelativeRot();
    Quat parentRot = bank->getParent()->getRotation();
    Quat newRelRot = parentRot * rot * relRot;

    detectorInfo.setRotation(*bank, newRelRot);

    V3D rotatedPos = V3D(pos);
    bank->getParent()->getRotation().rotate(rotatedPos);

    detectorInfo.setPosition(*bank, bank->getPos() + rotatedPos);

    // TODO: Use ResizeRectangularDetectorHelper from PR #18906
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
  }
}

} // namespace CalibrationHelpers
} // namespace Crystal
} // namespace Mantid
