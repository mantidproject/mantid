#ifndef MANTID_SLICEVIEWER_PEAK_REPRESENTATION_ELLIPSOID_H
#define MANTID_SLICEVIEWER_PEAK_REPRESENTATION_ELLIPSOID_H

#include "MantidQtSliceViewer/PeakRepresentation.h"
#include "MantidQtSliceViewer/EllipsoidPlaneSliceCalculator.h"
#include "MantidKernel/V2D.h"
namespace MantidQt {
namespace SliceViewer {

/** PeakRepresentationEllipsoid : Draws an ellipse for elliptical peaks.

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
class EXPORT_OPT_MANTIDQT_SLICEVIEWER PeakRepresentationEllipsoid
    : public PeakRepresentation {
public:
  PeakRepresentationEllipsoid(
      const Mantid::Kernel::V3D &origin, const std::vector<double> peakRadii,
      const std::vector<double> backgroundInnerRadii,
      const std::vector<double> backgroundOuterRadii,
      const std::vector<Mantid::Kernel::V3D> directions,
      std::shared_ptr<Mantid::SliceViewer::EllipsoidPlaneSliceCalculator>
          calculator);

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

  static const double zeroRadius;
  /// Get the zoom out factor
  double getZoomOutFactor() const;

protected:
  std::shared_ptr<PeakPrimitives> getDrawingInformation(
      PeakRepresentationViewInformation viewInformation) override;
  void doDraw(QPainter &painter, PeakViewColor &foregroundColor,
              PeakViewColor &backgroundColor,
              std::shared_ptr<PeakPrimitives> drawingInformation,
              PeakRepresentationViewInformation viewInformation) override;

private:
  //---------- Original collections
  /// Original origin x=h, y=k, z=l
  Mantid::Kernel::V3D m_originalOrigin;
  /// Original directions
  std::vector<Mantid::Kernel::V3D> m_originalDirections;
  /// Original cached opacity gradient
  Mantid::Kernel::V3D m_originalCachedOpacityGradient;

  // -----------Working copies of collections
  /// Origin md-x, md-y, and md-z
  Mantid::Kernel::V3D m_origin;
  /// Direction in md-x, md-y and md-z
  std::vector<Mantid::Kernel::V3D> m_directions;
  /// Actual peak radii
  const std::vector<double> m_peakRadii;
  /// Peak background inner radii
  const std::vector<double> m_backgroundInnerRadii;
  /// Peak background outer radius
  const std::vector<double> m_backgroundOuterRadii;

  /// Max opacity
  const double m_opacityMax;
  /// Min opacity
  const double m_opacityMin;
  /// Cached opacity at the distance z from origin
  double m_cachedOpacityAtDistance;
  /// Cached opacity gradient
  Mantid::Kernel::V3D m_cachedOpacityGradient;

  // ---- Drawing information of the 2D ellipses
  /// Angle between the x axis and the major ellipse axis
  double m_angleEllipse;

  /// Radii of the ellipse. First entry is the Major axis, second the minor axis
  std::vector<double> m_radiiEllipse;
  std::vector<double> m_radiiEllipseBackgroundInner;
  std::vector<double> m_radiiEllipseBackgroundOuter;

  // Origin of the ellipse
  Mantid::Kernel::V3D m_originEllipse;
  Mantid::Kernel::V3D m_originEllipseBackgroundInner;
  Mantid::Kernel::V3D m_originEllipseBackgroundOuter;

  /// Flag to indicate that the background radius should be drawn.
  bool m_showBackgroundRadii;

  /// A calculator to extract the ellipse parameters
  std::shared_ptr<Mantid::SliceViewer::EllipsoidPlaneSliceCalculator>
      m_calculator;
};
}
}
#endif
