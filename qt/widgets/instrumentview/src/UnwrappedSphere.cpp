#include "MantidQtWidgets/InstrumentView/UnwrappedSphere.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidQtWidgets/InstrumentView/UnwrappedDetector.h"
#include <cmath>

namespace MantidQt {
namespace MantidWidgets {

UnwrappedSphere::UnwrappedSphere(const InstrumentActor *rootActor,
                                 const Mantid::Kernel::V3D &origin,
                                 const Mantid::Kernel::V3D &axis)
    : RotationSurface(rootActor, origin, axis) {
  init();
}

//------------------------------------------------------------------------------
/** Convert physical position to UV projection
 *
 * @param pos :: position in 3D
 * @param u :: set to U
 * @param v :: set to V
 * @param uscale :: scaling for u direction
 * @param vscale :: scaling for v direction
 */
void UnwrappedSphere::project(const Mantid::Kernel::V3D &pos, double &u,
                              double &v, double &uscale, double &vscale) const {
  // projection to cylinder axis
  v = pos.scalar_prod(m_zaxis);
  double x = pos.scalar_prod(m_xaxis);
  double y = pos.scalar_prod(m_yaxis);

  double r = sqrt(x * x + y * y + v * v);
  uscale = 1. / sqrt(x * x + y * y);
  vscale = 1. / r;

  u = applyUCorrection(-atan2(y, x));
  v = -acos(v / r);
}

void UnwrappedSphere::rotate(const UnwrappedDetector &udet,
                             Mantid::Kernel::Quat &R) const {
  // rotation from the global axes to those where
  // the z axis points to the detector
  Mantid::Kernel::Quat R1;
  // direction in which to look: from sample to detector
  Mantid::Kernel::V3D eye;
  const auto &componentInfo = m_instrActor->componentInfo();
  eye = m_pos - componentInfo.position(udet.detIndex);
  if (!eye.nullVector()) {
    InstrumentActor::rotateToLookAt(eye, m_zaxis, R1);
  }
  // add detector's own rotation
  R = R1 * componentInfo.rotation(udet.detIndex);
}

} // namespace MantidWidgets
} // namespace MantidQt
