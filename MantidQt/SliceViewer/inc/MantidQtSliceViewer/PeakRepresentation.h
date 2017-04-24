#ifndef MANTID_SLICEVIEWER_PEAK_REPRESENTATION_H
#define MANTID_SLICEVIEWER_PEAK_REPRESENTATION_H

#include "MantidQtSliceViewer/PeakPrimitives.h"
#include "MantidQtSliceViewer/PeakViewColor.h"
#include "MantidGeometry/Crystal/PeakTransform.h"
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
typedef boost::optional<double> optional_double;

/** PeakRepresentation : Allows the draw a general visual peak shape.

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

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
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

typedef std::shared_ptr<PeakRepresentation> PeakRepresentation_sptr;
typedef std::vector<PeakRepresentation_sptr> VecPeakRepresentation;
}
}

#endif
