#ifndef MANTID_SLICEVIEWER_PEAKOVERLAY_VIEW_H_
#define MANTID_SLICEVIEWER_PEAKOVERLAY_VIEW_H_

#include "MantidKernel/System.h"
#include "MantidKernel/V2D.h"
#include "MantidAPI/PeakTransform.h"
#include "MantidQtSliceViewer/PeakPalette.h"
#include "MantidQtSliceViewer/PeakBoundingBox.h"
#include <QPointF>
#include <boost/shared_ptr.hpp>

namespace MantidQt
{
  namespace SliceViewer
  {
    /** Abstract view in MVP model representing a PeakOverlay.
    
    @date 2012-08-24

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    class DLLExport PeakOverlayView
    {
    public:
      /// Set the position of the slice point.
      virtual void setSlicePoint(const double&, const std::vector<bool>&) = 0;
      /// Update the view.
      virtual void updateView() = 0;
      /// Hide the view.
      virtual void hideView() = 0;
      /// Show the view.
      virtual void showView() = 0;
      /// Move the peak overlay to a new position.
      virtual void movePosition(Mantid::API::PeakTransform_sptr peakTransform) = 0;
      /// Change foreground colour
      virtual void changeForegroundColour(const QColor) = 0;
      /// Change background colour
      virtual void changeBackgroundColour(const QColor) = 0;
      /// Show the background radius
      virtual void showBackgroundRadius(const bool){}
      /// Changes the size of the overlay to be the requested fraction of the current view width.
      virtual void changeOccupancyInView(const double fraction) = 0;
      /// Changes the size of the overlay to be the requested fraction of the view depth.
      virtual void changeOccupancyIntoView(const double fraction) = 0;
      /// Get a bounding box around the peak in windows coordinates.
      virtual PeakBoundingBox getBoundingBox(const int peakIndex) const = 0;
      /// Get the peak size (width/2 as a fraction of total width)  on projection
      virtual double getOccupancyInView() const = 0;
      /// Get the peaks size into the projection (effective radius as a fraction of z range)
      virtual double getOccupancyIntoView() const = 0;
      /// Get the flag indicating that the view represents the position only.
      virtual bool positionOnly() const = 0;
      /// Get radius or effective radius of view items.
      virtual double getRadius() const = 0;
      /// Determine if the background is shown.
      virtual bool isBackgroundShown() const = 0;
      /// Get the current background colour
      virtual QColor getBackgroundColour() const = 0;
      /// Get the current foreground colour
      virtual QColor getForegroundColour() const = 0;
      /// Destructor
      virtual ~PeakOverlayView()
      {
      }
    };

    typedef boost::shared_ptr<const PeakOverlayView> PeakOverlayView_const_sptr;
    typedef boost::shared_ptr<PeakOverlayView> PeakOverlayView_sptr;

  }
}

#endif /* MANTID_SLICEVIEWER_PEAKOVERLAY_VIEW_H_ */
