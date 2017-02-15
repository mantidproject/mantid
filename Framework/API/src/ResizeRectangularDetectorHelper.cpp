#include "MantidAPI/DetectorInfo.h"
#include "MantidAPI/ResizeRectangularDetectorHelper.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/EigenConversionHelpers.h"

namespace Mantid {
namespace API {

/** Update pixel positions in DetectorInfo after scale parameters have been set
 * for a RectangularDetector.
 *
 * This does *not* set the scale parameters for detector in the ParameterMap. */
void applyRectangularDetectorScaleToDetectorInfo(
    DetectorInfo &detectorInfo, const Geometry::RectangularDetector &detector,
    const double scaleX, const double scaleY) {

  // Precompute transformation: Undo translation, undo rotation, scale, rotate
  // back, translate back:
  Eigen::Affine3d transformation(
      Eigen::Scaling(Eigen::Vector3d(scaleX, scaleY, 1)));
  const auto rotation = toQuaterniond(detector.getRotation());
  transformation.rotate(rotation.conjugate());
  transformation.prerotate(rotation);
  const auto origin = Kernel::toVector3d(detector.getPos());
  transformation.translate(-origin);
  transformation.pretranslate(origin);

  const auto idstart = detector.idstart();
  const auto idstep = detector.idstep();
  const auto count = detector.xpixels() * detector.ypixels();
  for (detid_t id = idstart; id < idstart + idstep * count; ++id) {
    const auto index = detectorInfo.indexOf(id);
    const auto pos = Kernel::toVector3d(detectorInfo.position(index));
    detectorInfo.setPosition(index, Kernel::toV3D(transformation * pos));
  }
}

} // namespace API
} // namespace Mantid
