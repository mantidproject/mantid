#ifndef UNWRAPPEDCYLINDER_H
#define UNWRAPPEDCYLINDER_H

#include "RotationSurface.h"

/**
  * Implementation of UnwrappedSurface as a cylinder
  */
class UnwrappedCylinder: public RotationSurface
{
public:
  UnwrappedCylinder(const InstrumentActor* rootActor,const Mantid::Kernel::V3D& origin,const Mantid::Kernel::V3D& axis);
protected:
  void project(const Mantid::Kernel::V3D & pos, double & u, double & v, double & uscale, double & vscale) const;
  void rotate(const UnwrappedDetector& udet, Mantid::Kernel::Quat& R)const;
};

#endif // UNWRAPPEDCYLINDER_H
