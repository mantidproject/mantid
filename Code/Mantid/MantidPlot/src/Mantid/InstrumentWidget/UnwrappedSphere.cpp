#include "UnwrappedSphere.h"
#include "MantidGeometry/IDetector.h"
#include <cmath>

UnwrappedSphere::UnwrappedSphere(const InstrumentActor *rootActor, const Mantid::Geometry::V3D &origin, const Mantid::Geometry::V3D &axis):
    UnwrappedSurface(rootActor,origin,axis)
{
  init();
}

void UnwrappedSphere::calcUV(UnwrappedDetector& udet)
{
  //static const double pi2 = 2*acos(-1.);
  Mantid::Geometry::V3D pos = udet.detector->getPos() - m_pos;

  // projection to cylinder axis
  udet.v = pos.scalar_prod(m_zaxis);
  double x = pos.scalar_prod(m_xaxis);
  double y = pos.scalar_prod(m_yaxis);

  double r = sqrt(x*x+y*y+udet.v*udet.v);
  udet.uscale = 1./sqrt(x*x+y*y);
  udet.vscale = 1./r;

  udet.u = -atan2(y,x);
  udet.v = -acos(udet.v/r);

  calcSize(udet,Mantid::Geometry::V3D(-1,0,0),Mantid::Geometry::V3D(0,1,0));

}

void UnwrappedSphere::calcRot(UnwrappedDetector& udet, Mantid::Geometry::Quat& R)
{
  // Basis vectors for a detector image on the screen
  const Mantid::Geometry::V3D X(-1,0,0);
  const Mantid::Geometry::V3D Y(0,1,0);
  const Mantid::Geometry::V3D Z(0,0,-1);

  // Find basis with x axis pointing to the detector from the sample,
  // z axis is coplanar with x and m_zaxis, and y making the basis right handed
  Mantid::Geometry::V3D x,y,z;
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
  Mantid::Geometry::Quat R1;
  BasisRotation(x,y,z,X,Y,Z,R1);

  R = R1 * udet.detector->getRotation();

}

double UnwrappedSphere::uPeriod()const
{
  return 2 * M_PI;
}
