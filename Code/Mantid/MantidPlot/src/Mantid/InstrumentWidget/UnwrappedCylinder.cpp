#include "UnwrappedCylinder.h"

#include "MantidGeometry/IDetector.h"

UnwrappedCylinder::UnwrappedCylinder(const InstrumentActor* rootActor, const Mantid::Kernel::V3D &origin, const Mantid::Kernel::V3D &axis):
    RotationSurface(rootActor,origin,axis)
{
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
void UnwrappedCylinder::project(const Mantid::Kernel::V3D & pos, double & u, double & v, double & uscale, double & vscale) const
{
  // projection to cylinder axis
  v = pos.scalar_prod(m_zaxis);
  double x = pos.scalar_prod(m_xaxis);
  double y = pos.scalar_prod(m_yaxis);
  u = applyUCorrection( -atan2(y,x) );

  uscale = 1./sqrt(x*x+y*y);
  vscale = 1.;
}

void UnwrappedCylinder::rotate(const UnwrappedDetector& udet, Mantid::Kernel::Quat& R)const
{
  Mantid::Kernel::V3D eye, up;
  eye = m_pos - udet.detector->getPos();
  up = m_zaxis;
  up.normalize();
  eye = eye - up * eye.scalar_prod(up);
  eye.normalize();

  Mantid::Kernel::Quat R1;
  InstrumentActor::rotateToLookAt(eye, up, R1);
  R =  R1 * udet.detector->getRotation();
}

