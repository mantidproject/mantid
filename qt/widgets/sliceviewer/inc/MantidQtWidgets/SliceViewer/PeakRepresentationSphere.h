#ifndef MANTID_SLICEVIEWER_PEAK_REPRESENTATION_SPHERE_H
#define MANTID_SLICEVIEWER_PEAK_REPRESENTATION_SPHERE_H

#include "MantidQtWidgets/SliceViewer/PeakRepresentation.h"

namespace MantidQt {
namespace SliceViewer {

/** PeakRepresentationSphere : Draws a circle for spherical peaks.

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at:
  <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class EXPORT_OPT_MANTIDQT_SLICEVIEWER PeakRepresentationSphere
    : public PeakRepresentation {
public:
  PeakRepresentationSphere(const Mantid::Kernel::V3D &origin,
                           const double &peakRadius,
                           const double &backgroundInnerRadius,
                           const double &backgroundOuterRadius);

  /// Setter for the slice point
  void setSlicePoint(const double &) override;
  /// Transform the coordinates.
  void
  movePosition(Mantid::Geometry::PeakTransform_sptr peakTransform) override;
  /// Get the bounding box.
  PeakBoundingBox getBoundingBox() const override;
  /// Set the size of the cross peak in the viewing plane
  void setOccupancyInView(const double fraction) override;
  /// Set the size of the cross peak into the viewing plane
  void setOccupancyIntoView(const double fraction) override;
  /// Get the effective peak radius.
  double getEffectiveRadius() const override;
  /// Get the origin
  const Mantid::Kernel::V3D &getOrigin() const override;
  /// Show the background radius
  void showBackgroundRadius(const bool show) override;

  /**
   * The zoom-out factor ensures that the sphere can be viewed
   * in its entirety in full-screen or default mode.
   **/
  double getZoomOutFactor() const;

protected:
  std::shared_ptr<PeakPrimitives> getDrawingInformation(
      PeakRepresentationViewInformation viewInformation) override;
  void doDraw(QPainter &painter, PeakViewColor &foregroundColor,
              PeakViewColor &backgroundColor,
              std::shared_ptr<PeakPrimitives> drawingInformation,
              PeakRepresentationViewInformation viewInformation) override;

private:
  /// Original origin x=h, y=k, z=l
  const Mantid::Kernel::V3D m_originalOrigin;
  /// Origin md-x, md-y, and md-z
  Mantid::Kernel::V3D m_origin;
  /// actual peak radius
  const double m_peakRadius;
  /// Peak background inner radius
  const double m_backgroundInnerRadius;
  /// Peak background outer radius
  double m_backgroundOuterRadius;
  /// Max opacity
  const double m_opacityMax;
  /// Min opacity
  const double m_opacityMin;
  /// Cached opacity at the distance z from origin
  double m_cachedOpacityAtDistance;
  /// Cached radius at the distance z from origin
  optional_double m_peakRadiusAtDistance;
  /// Cached opacity gradient.
  const double m_cachedOpacityGradient;
  /// Cached radius squared.
  const double m_peakRadiusSQ;
  /// Cached background inner radius sq.
  const double m_backgroundInnerRadiusSQ;
  /// Cached background outer radius sq.
  double m_backgroundOuterRadiusSQ;
  /// Flag to indicate that the background radius should be drawn.
  bool m_showBackgroundRadius;
  /// Inner radius at distance.
  optional_double m_backgroundInnerRadiusAtDistance;
  /// Outer radius at distance.
  optional_double m_backgroundOuterRadiusAtDistance;
  /// Zoom out factor
  const double zoomOutFactor = 2.;
};
} // namespace SliceViewer
} // namespace MantidQt
#endif
