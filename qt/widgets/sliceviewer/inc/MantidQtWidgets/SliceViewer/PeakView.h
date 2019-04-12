// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_SLICEVIEWER_PEAK_VIEW_H_
#define MANTID_SLICEVIEWER_PEAK_VIEW_H_

#include "MantidQtWidgets/SliceViewer/NonOrthogonalAxis.h"
#include "MantidQtWidgets/SliceViewer/PeakOverlayInteractive.h"
#include "PeakRepresentation.h"

namespace MantidQt {
namespace SliceViewer {

/** PeakView : Holds a collection of peaks of any type and coordinates
               them being drawn.
*/
class PeakView : public PeakOverlayInteractive {
public:
  PeakView(PeaksPresenter *const presenter, QwtPlot *plot, QWidget *parent,
           const VecPeakRepresentation &vecPeakRepresentation,
           const int plotXIndex, const int plotYIndex,
           PeakViewColor foregroundColor, PeakViewColor backgroundColor,
           double largestEffectiveRadius);

  virtual ~PeakView();

  /// Set the slice point at position.
  void setSlicePoint(const double &point,
                     const std::vector<bool> &viewablePeaks) override;

  /// Hide the view.
  void hideView() override;

  /// Show the view.
  void showView() override;

  /// Update the view.
  void updateView() override;

  /// Move the position of the peak, by using a different configuration of the
  /// existing origin indexes.
  void
  movePosition(Mantid::Geometry::PeakTransform_sptr peakTransform) override;
  void
  movePositionNonOrthogonal(Mantid::Geometry::PeakTransform_sptr peakTransform,
                            NonOrthogonalAxis &info) override;

  /// Show the background radius
  void showBackgroundRadius(const bool show) override;

  /// Get a bounding box for this peak.
  PeakBoundingBox getBoundingBox(const int peakIndex) const override;

  /// Changes the size of the overlay to be the requested fraction of the
  /// current view width.
  void changeOccupancyInView(const double fraction) override;

  /// Changes the size of the overlay to be the requested fraction of the view
  /// depth.
  void changeOccupancyIntoView(const double fraction) override;

  /// Get the peak size (width/2 as a fraction of total width)  on projection
  double getOccupancyInView() const override;

  /// Get the peaks size into the projection (effective radius as a fraction
  /// of z range)
  double getOccupancyIntoView() const override;

  /// Getter indicating that the view is position only
  bool positionOnly() const override;

  /// Get the radius of the peak objects.
  double getRadius() const override;

  /// Determine if the background radius is shown.
  bool isBackgroundShown() const override;

  /// Take settings from another view
  void takeSettingsFrom(const PeakOverlayView *const /*source*/) override;

  /// Change foreground colour -- overload for PeakViewColor
  void changeForegroundColour(const PeakViewColor peakViewColor) override;

  /// Change background colour -- overload for PeakViewColor
  void changeBackgroundColour(const PeakViewColor peakViewColor) override;

  /// Get the current background colour
  PeakViewColor getBackgroundPeakViewColor() const override;

  /// Get the current foreground colour
  PeakViewColor getForegroundPeakViewColor() const override;

private:
  /// Draw the peak representations. Pure virtual on base class.
  void doPaintPeaks(QPaintEvent * /*event*/) override;

  /// The actual peak objects
  VecPeakRepresentation m_peaks;

  /// Peaks in the workspace that are viewable in the present view.
  std::vector<bool> m_viewablePeaks;

  /// Cached occupancy into the view
  double m_cachedOccupancyIntoView;

  /// Cached occupancy onto view
  double m_cachedOccupancyInView;

  /// Show the background radius.
  bool m_showBackground;

  /// Foreground color
  PeakViewColor m_foregroundColor;
  PeakViewColor m_backgroundColor;

  /// Largeste effective radius of all sub-representations
  double m_largestEffectiveRadius;
};
} // namespace SliceViewer
} // namespace MantidQt

#endif
