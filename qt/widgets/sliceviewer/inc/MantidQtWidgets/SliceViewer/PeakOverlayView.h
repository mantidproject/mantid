// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_SLICEVIEWER_PEAKOVERLAY_VIEW_H_
#define MANTID_SLICEVIEWER_PEAKOVERLAY_VIEW_H_

#include "MantidDataObjects/AffineMatrixParameter.h"
#include "MantidGeometry/Crystal/PeakTransform.h"
#include "MantidKernel/V2D.h"
#include "MantidQtWidgets/SliceViewer/NonOrthogonalAxis.h"
#include "MantidQtWidgets/SliceViewer/PeakBoundingBox.h"
#include "MantidQtWidgets/SliceViewer/PeakPalette.h"
#include "MantidQtWidgets/SliceViewer/PeakViewColor.h"
#include <QPointF>
#include <boost/shared_ptr.hpp>

namespace MantidQt {
namespace SliceViewer {

/** Abstract view in MVP model representing a PeakOverlay.

@date 2012-08-24
*/
class EXPORT_OPT_MANTIDQT_SLICEVIEWER PeakOverlayView {
public:
  /// Set the position of the slice point.
  virtual void setSlicePoint(const double &, const std::vector<bool> &) = 0;
  /// Update the view.
  virtual void updateView() = 0;
  /// Hide the view.
  virtual void hideView() = 0;
  /// Show the view.
  virtual void showView() = 0;
  /// Move the peak overlay to a new position.
  virtual void
  movePosition(Mantid::Geometry::PeakTransform_sptr peakTransform) = 0;
  /// Show the background radius
  virtual void showBackgroundRadius(const bool /*unused*/) {}
  virtual void
  movePositionNonOrthogonal(Mantid::Geometry::PeakTransform_sptr peakTransform,
                            NonOrthogonalAxis &info) = 0;
  /// Show the background radius
  /// Changes the size of the overlay to be the requested fraction of the
  /// current view width.
  virtual void changeOccupancyInView(const double fraction) = 0;
  /// Changes the size of the overlay to be the requested fraction of the view
  /// depth.
  virtual void changeOccupancyIntoView(const double fraction) = 0;
  /// Get a bounding box around the peak in windows coordinates.
  virtual PeakBoundingBox getBoundingBox(const int peakIndex) const = 0;
  /// Get the peak size (width/2 as a fraction of total width)  on projection
  virtual double getOccupancyInView() const = 0;
  /// Get the peaks size into the projection (effective radius as a fraction of
  /// z range)
  virtual double getOccupancyIntoView() const = 0;
  /// Get the flag indicating that the view represents the position only.
  virtual bool positionOnly() const = 0;
  /// Get radius or effective radius of view items.
  virtual double getRadius() const = 0;
  /// Determine if the background is shown.
  virtual bool isBackgroundShown() const = 0;
  /// Enter deletion mode
  virtual void peakDeletionMode() = 0;
  /// Enter addition mode
  virtual void peakAdditionMode() = 0;
  /// Enter normal view mode
  virtual void peakDisplayMode() = 0;
  /// Take settings from.
  virtual void takeSettingsFrom(PeakOverlayView const *const) = 0;
  /// Change foreground colour -- overload for PeakViewColor
  virtual void changeForegroundColour(const PeakViewColor) = 0;
  /// Change background colour -- overload for PeakViewColor
  virtual void changeBackgroundColour(const PeakViewColor) = 0;
  /// Get the current background colour
  virtual PeakViewColor getBackgroundPeakViewColor() const = 0;
  /// Get the current foreground colour
  virtual PeakViewColor getForegroundPeakViewColor() const = 0;
  /// Destructor
  virtual ~PeakOverlayView() {}
};

using PeakOverlayView_const_sptr = boost::shared_ptr<const PeakOverlayView>;
using PeakOverlayView_sptr = boost::shared_ptr<PeakOverlayView>;
} // namespace SliceViewer
} // namespace MantidQt

#endif /* MANTID_SLICEVIEWER_PEAKOVERLAY_VIEW_H_ */
