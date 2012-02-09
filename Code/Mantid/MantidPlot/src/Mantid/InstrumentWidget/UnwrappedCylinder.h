#ifndef UNWRAPPEDCYLINDER_H
#define UNWRAPPEDCYLINDER_H

#include "UnwrappedSurface.h"

/**
  * Implementation of UnwrappedSurface as a cylinder
  */
class UnwrappedCylinder: public UnwrappedSurface
{
public:
  UnwrappedCylinder(const InstrumentActor* rootActor,const Mantid::Kernel::V3D& origin,const Mantid::Kernel::V3D& axis);
protected:
  void project(double & u, double & v, double & uscale, double & vscale, const Mantid::Kernel::V3D & pos) const;
  void calcRot(const UnwrappedDetector& udet, Mantid::Kernel::Quat& R)const;
  double uPeriod()const;
};

#endif // UNWRAPPEDCYLINDER_H
