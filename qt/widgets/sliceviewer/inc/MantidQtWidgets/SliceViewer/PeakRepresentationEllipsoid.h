// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_SLICEVIEWER_PEAK_REPRESENTATION_ELLIPSOID_H
#define MANTID_SLICEVIEWER_PEAK_REPRESENTATION_ELLIPSOID_H

#include "MantidKernel/V2D.h"
#include "MantidQtWidgets/SliceViewer/EllipsoidPlaneSliceCalculator.h"
#include "MantidQtWidgets/SliceViewer/PeakRepresentation.h"
namespace MantidQt {
namespace SliceViewer {

/** PeakRepresentationEllipsoid : Draws an ellipse for elliptical peaks.
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
  void setSlicePoint(const double & /*z*/) override;
  /// Transform the coordinates.
  void
  movePosition(Mantid::Geometry::PeakTransform_sptr peakTransform) override;
  void
  movePositionNonOrthogonal(Mantid::Geometry::PeakTransform_sptr peakTransform,
                            NonOrthogonalAxis &info) override {
    (void)info;
    movePosition(peakTransform);
  }
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
} // namespace SliceViewer
} // namespace MantidQt
#endif
