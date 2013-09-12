#ifndef PANELSSURFACE_H
#define PANELSSURFACE_H

#include "UnwrappedSurface.h"

/**
  * @class PanelsSurface
  * @brief Performs projection of an instrument onto a 2D surface of rotation: cylinder, sphere, ...
  */
class PanelsSurface: public UnwrappedSurface
{
public:
  PanelsSurface(const InstrumentActor* rootActor,const Mantid::Kernel::V3D& origin,const Mantid::Kernel::V3D& axis);
  void init();

protected:

  const Mantid::Kernel::V3D m_pos;   ///< Origin (sample position)
  const Mantid::Kernel::V3D m_zaxis; ///< The z axis of the surface specific coord system
  Mantid::Kernel::V3D m_xaxis;       ///< The x axis
  Mantid::Kernel::V3D m_yaxis;       ///< The y axis
  double m_u_correction;             ///< Correction to u calculated by project() after findAndCorrectUGap()

};

#endif // PANELSSURFACE_H
