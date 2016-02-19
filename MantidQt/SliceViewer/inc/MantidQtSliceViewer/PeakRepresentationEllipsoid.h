#ifndef MANTID_SLICEVIEWER_PEAK_REPRESENTATION_ELLIPSOID_H
#define MANTID_SLICEVIEWER_PEAK_REPRESENTATION_ELLIPSOID_H

#include "MantidQtSliceViewer/PeakRepresentation.h"

namespace MantidQt
{
namespace SliceViewer
{

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
class PeakRepresentationEllipsoid : public PeakRepresentation
{
public:
  PeakRepresentationEllipsoid(const Mantid::Kernel::V3D &origin,
                           const std::vector<double> peakRadii,
                           const std::vector<double> backgroundInnerRadii,
                           const std::vector<double> backgroundOuterRadii,
                           const std::vector<Mantid::Kernel::V3D> directions);

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
  /// Get the width occupancy (fractional in the projection plane).
  double getOccupancyInView() const override;
  /// Get the depth occupancy (fractional into the projection plane)
  double getOccupancyIntoView() const override;
  /// Get the origin
  const Mantid::Kernel::V3D &getOrigin() const override;
  /// Show the background radius
  void showBackgroundRadius(const bool show) override;

protected:
  std::shared_ptr<PeakPrimitives> getDrawingInformation(
      PeakRepresentationViewInformation viewInformation) override;
  void doDraw(QPainter &painter, PeakViewColor &foregroundColor,
              PeakViewColor &backgroundColor,
              std::shared_ptr<PeakPrimitives> drawingInformation,
              PeakRepresentationViewInformation viewInformation) override;

private:
  /// Original origin x=h, y=k, z=l
  Mantid::Kernel::V3D m_originalOrigin;
  /// Original directions
  std::vector<Mantid::Kernel::V3D> m_originalDirections;


  /// Origin md-x, md-y, and md-z
  Mantid::Kernel::V3D  m_origin;
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


  // TODO: check how to incorporate in an elliptical scenario


  /// Flag to indicate that the background radius should be drawn.
  bool m_showBackgroundRadii;


};
}
}
#endif
