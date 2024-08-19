// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/InstrumentView/UnwrappedCylinder.h"
#include "MantidQtWidgets/InstrumentView/UnwrappedDetector.h"

#include "MantidGeometry/Instrument/ComponentInfo.h"

namespace MantidQt::MantidWidgets {

UnwrappedCylinder::UnwrappedCylinder(const InstrumentActor *rootActor, const Mantid::Kernel::V3D &origin,
                                     const Mantid::Kernel::V3D &axis, const QSize &widgetSize,
                                     const bool maintainAspectRatio)
    : RotationSurface(rootActor, origin, axis, widgetSize, maintainAspectRatio) {
  init();
}

//------------------------------------------------------------------------------
/** Convert detector (physical position) to UV projection
 *
 * @param detIndex :: detector index in DetectorInfo or ComponentInfo
 * @param u :: set to U
 * @param v :: set to V
 * @param uscale :: scaling for u direction
 * @param vscale :: scaling for v direction
 */
void UnwrappedCylinder::project(const size_t detIndex, double &u, double &v, double &uscale, double &vscale) const {
  const auto &componentInfo = m_instrActor->componentInfo();
  auto pos = componentInfo.position(detIndex) - m_pos;
  project(pos, u, v, uscale, vscale);
}

void UnwrappedCylinder::project(const Mantid::Kernel::V3D &position, double &u, double &v, double &uscale,
                                double &vscale) const {
  double z = position.scalar_prod(m_zaxis);
  double x = position.scalar_prod(m_xaxis);
  double y = position.scalar_prod(m_yaxis);
  // use equal area cylindrical projection with v = sin(latitude), u = longitude
  v = z / sqrt(x * x + y * y + z * z);
  u = applyUCorrection(-atan2(y, x));

  uscale = 1. / sqrt(x * x + y * y);
  vscale = 1. / sqrt(x * x + y * y + z * z);
}

void UnwrappedCylinder::rotate(const UnwrappedDetector &udet, Mantid::Kernel::Quat &R) const {
  // direction in which to look
  Mantid::Kernel::V3D eye;
  const auto &componentInfo = m_instrActor->componentInfo();
  // rotation from the global axes to those where
  // the z axis points to the detector
  Mantid::Kernel::Quat R1;
  eye = m_pos - componentInfo.position(udet.detIndex);
  if (!eye.nullVector()) {
    // eye must point towards the detector and be perpendicular to the
    // cylinder's axis
    const auto up = normalize(m_zaxis);
    eye = eye - up * eye.scalar_prod(up);
    if (!eye.nullVector()) {
      eye.normalize();
      InstrumentActor::rotateToLookAt(eye, up, R1);
    }
  }
  // add detector's own rotation
  R = R1 * componentInfo.rotation(udet.detIndex);
}

} // namespace MantidQt::MantidWidgets
