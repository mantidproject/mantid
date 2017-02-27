#include "MantidAPI/DetectorInfo.h"
#include "MantidCrystal/CalibrationHelpers.h"

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace Mantid {
namespace Crystal {

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

} // namespace Crystal
} // namespace Mantid
