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
    ~PeakOverlayMultiCross() override;
    /// Set the slice point at position.
    void setSlicePoint(const double &point,
                       const std::vector<bool> &viewablePeaks) override;
    /// Hide the view.
    void hideView() override;
    /// Show the view.
    void showView() override;
    /// Update the view.
    void updateView() override;
    /// Move the position of the peak, by using a different configuration of the existing origin indexes.
    void
    movePosition(Mantid::Geometry::PeakTransform_sptr peakTransform) override;
    /// Change foreground colour
    void changeForegroundColour(const QColor) override;
    /// Change background colour
    void changeBackgroundColour(const QColor) override;
    /// Get a bounding box for this peak.
    PeakBoundingBox getBoundingBox(const int peakIndex) const override;
    /// Changes the size of the overlay to be the requested fraction of the current view width.
    void changeOccupancyInView(const double fraction) override;
    /// Changes the size of the overlay to be the requested fraction of the view depth.
    void changeOccupancyIntoView(const double fraction) override;
    /// Get the occupancy in the view.
    double getOccupancyInView() const override;
    /// Get the occupancy into the view.
    double getOccupancyIntoView() const override;
    /// Flag indicating that the peak is position only.
    bool positionOnly() const override;
    /// Get the effective radius.
    double getRadius() const override;
    /// Is the background radius visible
    bool isBackgroundShown() const override;
    /// Get the foreground colour
    QColor getForegroundColour() const override;
    /// Get the background colour
    QColor getBackgroundColour() const override;

    /// Take settings from another view
    void takeSettingsFrom(const PeakOverlayView *const) override;

  private:

    /// Pure virtual on PeakOverlayInteractive
    void doPaintPeaks(QPaintEvent *) override;

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
