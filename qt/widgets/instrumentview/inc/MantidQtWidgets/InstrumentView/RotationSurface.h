#ifndef ROTATIONSURFACE_H
#define ROTATIONSURFACE_H

#include "UnwrappedSurface.h"

namespace MantidQt {
namespace MantidWidgets {

/**
* @class RotationSurface
* @brief Performs projection of an instrument onto a 2D surface of rotation:
* cylinder, sphere, ...
*/
class RotationSurface : public UnwrappedSurface {
public:
  RotationSurface(const InstrumentActor *rootActor,
                  const Mantid::Kernel::V3D &origin,
                  const Mantid::Kernel::V3D &axis);
  void init() override;
  // Get the value of the u-correction - a shift in the u-coord added to
  // automatically determined uv coordinates
  QPointF getUCorrection() const { return QPointF(m_u_min, m_u_max); }
  // Set new value for the u-correction
  void setUCorrection(double umin, double umax);
  // Set automatic u-correction
  void setAutomaticUCorrection();
  // Is u-correction applied manually?
  bool isManualUCorrection() const { return m_manual_u_correction; }

protected:
  /// Period in the u coordinate. 2pi by default.
  virtual double uPeriod() const { return 2 * M_PI; }

  /// Given the u and v coords for all detectors find their min and max values
  /// and set m_u_min, m_u_max, m_v_min, m_v_max
  void findUVBounds();

  /// Automatic generation of the projection coordinates may leave a gap
  /// in u when the surface is unwrapped. This method tries to minimize
  /// this gap by shifting the origin of the u axis.
  void findAndCorrectUGap();

  /// Applies the shift (u-correction) found by findAndCorrectUGap() to a
  /// u-value.
  /// This method should only be used inside an implementation of
  /// UnwrappedSurface::project().
  double applyUCorrection(double u) const;

  /// Update the view rect to offset for the U correction
  void updateViewRectForUCorrection();

private:
  void findAxes();
  std::vector<size_t> retrieveSurfaceDetectors() const;
  void correctUCoords(double manual_u_min, double manual_u_max);
  void createUnwrappedDetectors();

protected:
  /// Calculate UV offsets from the view rect
  std::pair<double, double> calculateViewRectOffsets();

  const Mantid::Kernel::V3D m_pos; ///< Origin (sample position)
  const Mantid::Kernel::V3D
      m_zaxis; ///< The z axis of the surface specific coord system
  Mantid::Kernel::V3D m_xaxis; ///< The x axis
  Mantid::Kernel::V3D m_yaxis; ///< The y axis
  bool m_manual_u_correction;  ///< Flag set to prevent automatic
  /// FindAndCorrectUGap()
};
} // MantidWidgets
} // MantidQt

#endif // ROTATIONSURFACE_H
