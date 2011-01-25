#ifndef UNWRAPPEDCYLINDER_H
#define UNWRAPPEDCYLINDER_H

#include "UnwrappedSurface.h"

/**
  * Implementation of UnwrappedSurface as a cylinder
  */
class UnwrappedCylinder: public UnwrappedSurface
{
public:
  UnwrappedCylinder(const InstrumentActor* rootActor,const Mantid::Geometry::V3D& origin,const Mantid::Geometry::V3D& axis);
protected:
  void calcUV(UnwrappedDetector& udet);
  void calcRot(UnwrappedDetector& udet, Mantid::Geometry::Quat& R);
  double uPeriod()const;
};

#endif // UNWRAPPEDCYLINDER_H
