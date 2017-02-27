#include "MantidAPI/DetectorInfo.h"
#include "MantidCrystal/SCDCalibratePanels.h"
#include "MantidCrystal/CalibrationHelpers.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace Mantid {
namespace Crystal {

namespace {
  constexpr double RAD_TO_DEG = 180. / M_PI;
}

/**
 * Converts a Quaternion to a corresponding matrix produce Rotx*Roty*Rotz,
 * corresponding to the order Mantid uses in calculating rotations
 * @param Q The Quaternion. It will be normalized to represent a rotation
 * @param Rotx The angle in degrees for rotating around the x-axis
 * @param Roty The angle in degrees for rotating around the y-axis
 * @param Rotz The angle in degrees for rotating around the z-axis
 */
void CalibrationHelpers::Quat2RotxRotyRotz(const Quat Q, double &Rotx,
                                           double &Roty, double &Rotz) {
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
 * @param L0 The distance from source to sample (should be positive)
 * @param newSampPos The relative shift for the new sample position
 * @param pmapOld The Parameter map from the original instrument (not
 *NewInstrument). "Clones" relevant information into the newInstrument's
 *parameter map.
 */
void CalibrationHelpers::fixUpSampleAndSourcePositions(
    boost::shared_ptr<const Instrument> newInstrument, double const L0,
    const V3D newSampPos, DetectorInfo &detectorInfo) {
  boost::shared_ptr<ParameterMap> pmap = newInstrument->getParameterMap();

  IComponent_const_sptr source = newInstrument->getSource();
  IComponent_const_sptr sample = newInstrument->getSample();

  V3D samplePos = detectorInfo.samplePosition();
  if (samplePos != newSampPos) {
    detectorInfo.setPosition(*sample, newSampPos);
  }
  V3D sourceRelPos = source->getRelativePos();
  V3D sourcePos = detectorInfo.sourcePosition();
  V3D parentSourcePos = sourcePos - sourceRelPos;
  V3D source2sampleDir = samplePos - source->getPos();

  double scalee = L0 / source2sampleDir.norm();
  V3D newsourcePos = newSampPos - source2sampleDir * scalee;
  V3D newsourceRelPos = newsourcePos - parentSourcePos;

  detectorInfo.setPosition(*source, newsourceRelPos);
}

/**
 *  Copies positional entries in pmapSv to pmap starting at bank_const and
 *parents.
 *
 *  @param bank_const  the starting component for copying entries.
 *  @param pmap the Parameter Map to be updated
 *  @param pmapSv the original Parameter Map
 *
 */
void CalibrationHelpers::updateBankParams(
    boost::shared_ptr<const Geometry::IComponent> bank_const,
    boost::shared_ptr<Geometry::ParameterMap> pmap,
    boost::shared_ptr<const Geometry::ParameterMap> pmapSv) {
  std::vector<V3D> posv = pmapSv->getV3D(bank_const->getName(), "pos");

  if (!posv.empty()) {
    V3D pos = posv[0];
    pmap->addDouble(bank_const.get(), "x", pos.X());
    pmap->addDouble(bank_const.get(), "y", pos.Y());
    pmap->addDouble(bank_const.get(), "z", pos.Z());
    pmap->addV3D(bank_const.get(), "pos", pos);
  }

  boost::shared_ptr<Parameter> rot = pmapSv->get(bank_const.get(), ("rot"));
  if (rot) {
    pmap->addQuat(bank_const.get(), "rot", rot->value<Quat>());
  }

  std::vector<double> scalex =
      pmapSv->getDouble(bank_const->getName(), "scalex");
  std::vector<double> scaley =
      pmapSv->getDouble(bank_const->getName(), "scaley");
  if (!scalex.empty()) {
    pmap->addDouble(bank_const.get(), "scalex", scalex[0]);
  }
  if (!scaley.empty()) {
    pmap->addDouble(bank_const.get(), "scaley", scaley[0]);
  }

  boost::shared_ptr<const Geometry::IComponent> parent =
      bank_const->getParent();
  if (parent) {
    updateBankParams(parent, pmap, pmapSv);
  }
}

/**
 *  Updates the ParameterMap for NewInstrument to reflect the changes in the
 *associated panel information
 *
 * @param bankNames The names of the banks(panels) that will be updated
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
 * @param pmapOld The Parameter map from the original instrument(not
 *newInstrument). "Clones" relevant information into the NewInstrument's
 *parameter map.
 * @param rotCenters Rotate the centers of the panels(the same amount) with the
 *rotation of panels around their center
 */
void CalibrationHelpers::fixUpBankParameterMap(
    const std::vector<std::string> bankNames,
    boost::shared_ptr<const Instrument> newInstrument, const V3D pos,
    const Quat rot, double const detWScale, double const detHtScale,
    boost::shared_ptr<const ParameterMap> const pmapOld, bool rotCenters) {
  boost::shared_ptr<ParameterMap> pmap = newInstrument->getParameterMap();

  for (const auto &bankName : bankNames) {

    boost::shared_ptr<const IComponent> bank1 =
        newInstrument->getComponentByName(bankName);
    boost::shared_ptr<const Geometry::RectangularDetector> bank =
        boost::dynamic_pointer_cast<const RectangularDetector>(
            bank1); // Component
    updateBankParams(bank, pmap, pmapOld);

    Quat RelRot = bank->getRelativeRot();
    Quat newRelRot = rot * RelRot;
    double rotx, roty, rotz;
    Quat2RotxRotyRotz(newRelRot, rotx, roty, rotz);

    pmap->addRotationParam(bank.get(), std::string("rotx"), rotx);
    pmap->addRotationParam(bank.get(), std::string("roty"), roty);
    pmap->addRotationParam(bank.get(), std::string("rotz"), rotz);
    pmap->addQuat(bank.get(), "rot",
                  newRelRot); // Should not have had to do this???
    //---------Rotate center of bank ----------------------
    V3D Center = bank->getPos();
    V3D Center_orig(Center);
    if (rotCenters)
      rot.rotate(Center);

    V3D pos1 = bank->getRelativePos();

    pmap->addPositionCoordinate(bank.get(), std::string("x"),
                                pos.X() + pos1.X() + Center.X() -
                                    Center_orig.X());
    pmap->addPositionCoordinate(bank.get(), std::string("y"),
                                pos.Y() + pos1.Y() + Center.Y() -
                                    Center_orig.Y());
    pmap->addPositionCoordinate(bank.get(), std::string("z"),
                                pos.Z() + pos1.Z() + Center.Z() -
                                    Center_orig.Z());

    Quat2RotxRotyRotz(rot, rotx, roty, rotz);

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
    // cout<<"Thru param fix for "<<bankName<<". pos="<<bank->getPos()<<'\n';
  } // For @ bank
}

} // namespace Crystal
} // namespace Mantid
