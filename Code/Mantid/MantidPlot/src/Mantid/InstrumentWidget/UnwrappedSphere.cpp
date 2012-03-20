#include "UnwrappedSphere.h"
#include "MantidGeometry/IDetector.h"
#include <cmath>

UnwrappedSphere::UnwrappedSphere(const InstrumentActor* rootActor, const Mantid::Kernel::V3D &origin, const Mantid::Kernel::V3D &axis):
    UnwrappedSurface(rootActor,origin,axis)
{
  init();
}

//------------------------------------------------------------------------------
/** Convert physical position to UV projection
 *
 * @param u :: set to U
 * @param v :: set to V
 * @param pos :: position in 3D
 */
void UnwrappedSphere::project(double & u, double & v, double & uscale, double & vscale, const Mantid::Kernel::V3D & pos) const
{
  // projection to cylinder axis
  v = pos.scalar_prod(m_zaxis);
  double x = pos.scalar_prod(m_xaxis);
  double y = pos.scalar_prod(m_yaxis);

  double r = sqrt(x*x+y*y+v*v);
  uscale = 1./sqrt(x*x+y*y);
  vscale = 1./r;

  u = applyUCorrection( -atan2(y,x) );
  v = -acos(v/r);
}

void UnwrappedSphere::calcRot(const UnwrappedDetector& udet, Mantid::Kernel::Quat& R)const
{
  // Basis vectors for a detector image on the screen
  const Mantid::Kernel::V3D X(-1,0,0);
  const Mantid::Kernel::V3D Y(0,1,0);
  const Mantid::Kernel::V3D Z(0,0,-1);

  // Find basis with x axis pointing to the detector from the sample,
  // z axis is coplanar with x and m_zaxis, and y making the basis right handed
  Mantid::Kernel::V3D x,y,z;
  z = udet.detector->getPos() - m_pos;
  z.normalize();
  y = m_zaxis;
  x = y.cross_prod(z);
  if (x.nullVector())
  {
    x = m_xaxis;
  }
  x.normalize();
  y = z.cross_prod(x);
  Mantid::Kernel::Quat R1;
  InstrumentActor::BasisRotation(x,y,z,X,Y,Z,R1);

  R = R1 * udet.detector->getRotation();

}

double UnwrappedSphere::uPeriod()const
{
  return 2 * M_PI;
}
