#ifndef UNWRAPPEDSPHERE_H
#define UNWRAPPEDSPHERE_H

#include "RotationSurface.h"

namespace MantidQt {
namespace MantidWidgets {

/**
 * Implementation of UnwrappedSurface as a cylinder
 */
class UnwrappedSphere : public RotationSurface {
public:
  UnwrappedSphere(const InstrumentActor *rootActor,
                  const Mantid::Kernel::V3D &origin,
                  const Mantid::Kernel::V3D &axis);

protected:
  void rotate(const UnwrappedDetector &udet,
              Mantid::Kernel::Quat &R) const override;
  void project(const Mantid::Kernel::V3D &pos, double &u, double &v,
               double &uscale, double &vscale) const override;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif // UNWRAPPEDSPHERE_H
