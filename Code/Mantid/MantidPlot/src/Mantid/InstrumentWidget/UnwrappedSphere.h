#ifndef UNWRAPPEDSPHERE_H
#define UNWRAPPEDSPHERE_H

#include "UnwrappedSurface.h"

/**
  * Implementation of UnwrappedSurface as a cylinder
  */
class UnwrappedSphere: public UnwrappedSurface
{
public:
  UnwrappedSphere(const InstrumentActor* rootActor,const Mantid::Kernel::V3D& origin,const Mantid::Kernel::V3D& axis);
protected:
  void calcUV(UnwrappedDetector& udet);
  void calcRot(const UnwrappedDetector& udet, Mantid::Kernel::Quat& R)const;
  void project(double & u, double & v, double & uscale, double & vscale, const Mantid::Kernel::V3D & pos) const;
  double uPeriod()const;
};

#endif // UNWRAPPEDSPHERE_H
