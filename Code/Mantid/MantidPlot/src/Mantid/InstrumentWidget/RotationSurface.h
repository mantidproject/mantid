#ifndef ROTATIONSURFACE_H
#define ROTATIONSURFACE_H

#include "UnwrappedSurface.h"

/**
  * @class RotationSurface
  * @brief Performs projection of an instrument onto a 2D surface of rotation: cylinder, sphere, ...
  */
class RotationSurface: public UnwrappedSurface
{
public:
  RotationSurface(const InstrumentActor* rootActor,const Mantid::Kernel::V3D& origin,const Mantid::Kernel::V3D& axis);
  void init();

protected:

  /// Period in the u coordinate. 2pi by default.
  virtual double uPeriod() const {return 2 * M_PI;}

  /// Automatic generation of the projection coordinates may leave a gap
  /// in u when the surface is unwrapped. This method tries to minimize
  /// this gap by shifting the origin of the u axis.
  void findAndCorrectUGap();

  /// Applies the shift (u-correction) found by findAndCorrectUGap() to a u-value.
  /// This method should only be used inside an implementation of UnwrappedSurface::project().
  double applyUCorrection(double u)const;

  const Mantid::Kernel::V3D m_pos;   ///< Origin (sample position)
  const Mantid::Kernel::V3D m_zaxis; ///< The z axis of the surface specific coord system
  Mantid::Kernel::V3D m_xaxis;       ///< The x axis
  Mantid::Kernel::V3D m_yaxis;       ///< The y axis
  double m_u_correction;             ///< Correction to u calculated by project() after findAndCorrectUGap()

};

#endif // ROTATIONSURFACE_H
