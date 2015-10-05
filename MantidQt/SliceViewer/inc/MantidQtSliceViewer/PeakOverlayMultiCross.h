#ifndef MANTID_SLICEVIEWER_PEAKOVERLAYMULTICROSS_H_
#define MANTID_SLICEVIEWER_PEAKOVERLAYMULTICROSS_H_

#include "MantidQtSliceViewer/PeakOverlayInteractive.h"
#include "DllOption.h"
#include "MantidQtSliceViewer/PhysicalCrossPeak.h"

class QPaintEvent;
class QwtPlot;

namespace MantidQt
{

namespace MantidWidgets {
// Forward declaration
class InputController;
}

namespace SliceViewer
{

   class PeaksPresenter;
  /** Widget representing visible peaks in the plot. 
    
    @date 2013-06-10

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
  class EXPORT_OPT_MANTIDQT_SLICEVIEWER PeakOverlayMultiCross : public PeakOverlayInteractive
  {
    Q_OBJECT

  public:
    /// Constructor
    PeakOverlayMultiCross(PeaksPresenter* const peaksPresenter, QwtPlot * plot, QWidget * parent, const VecPhysicalCrossPeak& vecPhysicalPeaks, const int plotXIndex, const int plotYIndex,
                          const QColor& peakColour);
    /// Destructor
    virtual ~PeakOverlayMultiCross();
    /// Set the slice point at position.
    virtual void setSlicePoint(const double& point, const std::vector<bool>& viewablePeaks);
    /// Hide the view.
    virtual void hideView();
    /// Show the view.
    virtual void showView();
    /// Update the view.
    virtual void updateView();
    /// Move the position of the peak, by using a different configuration of the existing origin indexes.
    void movePosition(Mantid::Geometry::PeakTransform_sptr peakTransform);
    /// Change foreground colour
    virtual void changeForegroundColour(const QColor);
    /// Change background colour
    virtual void changeBackgroundColour(const QColor);
    /// Get a bounding box for this peak.
    virtual PeakBoundingBox getBoundingBox(const int peakIndex) const;
    /// Changes the size of the overlay to be the requested fraction of the current view width.
    virtual void changeOccupancyInView(const double fraction);
    /// Changes the size of the overlay to be the requested fraction of the view depth.
    virtual void changeOccupancyIntoView(const double fraction);
    /// Get the occupancy in the view.
    virtual double getOccupancyInView() const;
    /// Get the occupancy into the view.
    virtual double getOccupancyIntoView() const;
    /// Flag indicating that the peak is position only.
    bool positionOnly() const;
    /// Get the effective radius.
    virtual double getRadius() const;
    /// Is the background radius visible
    virtual bool isBackgroundShown() const;
    /// Get the foreground colour
    virtual QColor getForegroundColour() const;
    /// Get the background colour
    virtual QColor getBackgroundColour() const;

    /// Take settings from another view
    virtual void takeSettingsFrom(const PeakOverlayView * const);

  private:

    /// Pure virtual on PeakOverlayInteractive
    virtual void doPaintPeaks(QPaintEvent*);

    /// Physical model of the spacial cross peaks
    VecPhysicalCrossPeak m_physicalPeaks;
    /// Peak colour
    QColor m_peakColour;
    /// Peaks in the workspace that are viewable in the present view.
    std::vector<bool> m_viewablePeaks;
    /// Cached occupancy into the view
    double m_cachedOccupancyIntoView;
    /// Cached occupancy onto view
    double m_cachedOccupancyInView;

  };


} // namespace SliceViewer
} // namespace Mantid

#endif  /* MANTID_SLICEVIEWER_PEAKOVERLAYCROSS_H_ */
