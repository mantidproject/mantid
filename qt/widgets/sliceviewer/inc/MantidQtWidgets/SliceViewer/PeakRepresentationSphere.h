// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_SLICEVIEWER_PEAK_REPRESENTATION_SPHERE_H
#define MANTID_SLICEVIEWER_PEAK_REPRESENTATION_SPHERE_H

#include "MantidQtWidgets/SliceViewer/PeakRepresentation.h"

namespace MantidQt {
namespace SliceViewer {

/** PeakRepresentationSphere : Draws a circle for spherical peaks.
 */
class EXPORT_OPT_MANTIDQT_SLICEVIEWER PeakRepresentationSphere
    : public PeakRepresentation {
public:
  PeakRepresentationSphere(const Mantid::Kernel::V3D &origin,
                           const double &peakRadius,
                           const double &backgroundInnerRadius,
                           const double &backgroundOuterRadius);

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
