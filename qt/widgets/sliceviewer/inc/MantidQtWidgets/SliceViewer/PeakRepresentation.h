// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_SLICEVIEWER_PEAK_REPRESENTATION_H
#define MANTID_SLICEVIEWER_PEAK_REPRESENTATION_H

#include "MantidDataObjects/AffineMatrixParameter.h"
#include "MantidGeometry/Crystal/PeakTransform.h"
#include "MantidQtWidgets/SliceViewer/NonOrthogonalAxis.h"
#include "MantidQtWidgets/SliceViewer/PeakPrimitives.h"
#include "MantidQtWidgets/SliceViewer/PeakViewColor.h"
#include <boost/optional.hpp>

class QPainter;

namespace MantidQt {
namespace SliceViewer {
struct EXPORT_OPT_MANTIDQT_SLICEVIEWER PeakRepresentationViewInformation {
  double windowHeight;
  double windowWidth;
  double viewHeight;
  double viewWidth;
  int xOriginWindow;
  int yOriginWindow;
};

class PeakBoundingBox;

/// Alisas for a boost optional double.
using optional_double = boost::optional<double>;

/** PeakRepresentation : Allows the draw a general visual peak shape.
 */
class EXPORT_OPT_MANTIDQT_SLICEVIEWER PeakRepresentation {
public:
  virtual ~PeakRepresentation() {}

  /// Draw template method
  void draw(QPainter &painter, PeakViewColor &foregroundColor,
            PeakViewColor &backgroundColor,
            PeakRepresentationViewInformation viewInformation);
  /// Setter for the slice point
  virtual void setSlicePoint(const double &) = 0;
  /// Transform the coordinates.
  virtual void
  movePosition(Mantid::Geometry::PeakTransform_sptr peakTransform) = 0;
  virtual void
  movePositionNonOrthogonal(Mantid::Geometry::PeakTransform_sptr peakTransform,
                            NonOrthogonalAxis &info) = 0;
  /// Get the bounding box.
  virtual PeakBoundingBox getBoundingBox() const = 0;
  /// Set the size of the cross peak in the viewing plane
  virtual void setOccupancyInView(const double fraction) = 0;
  /// Set the size of the cross peak into the viewing plane
  virtual void setOccupancyIntoView(const double fraction) = 0;
  /// Get the effective peak radius.
  virtual double getEffectiveRadius() const = 0;
  /// Gets the origin
  virtual const Mantid::Kernel::V3D &getOrigin() const = 0;
  /// Show the background radius
  virtual void showBackgroundRadius(const bool show) = 0;

protected:
  virtual std::shared_ptr<PeakPrimitives>
  getDrawingInformation(PeakRepresentationViewInformation viewInformation) = 0;
  virtual void doDraw(QPainter &painter, PeakViewColor &foregroundColor,
                      PeakViewColor &backgroundColor,
                      std::shared_ptr<PeakPrimitives> drawingInformation,
                      PeakRepresentationViewInformation viewInformation) = 0;
};

using PeakRepresentation_sptr = std::shared_ptr<PeakRepresentation>;
using VecPeakRepresentation = std::vector<PeakRepresentation_sptr>;
} // namespace SliceViewer
} // namespace MantidQt

#endif
