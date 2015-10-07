#ifndef MANTID_SLICEVIEWER_PEAKOVERLAYMULTISPHERE_H_
#define MANTID_SLICEVIEWER_PEAKOVERLAYMULTISPHERE_H_

#include "MantidQtSliceViewer/PeakOverlayInteractive.h"
#include "MantidQtSliceViewer/PhysicalSphericalPeak.h"
#include "DllOption.h"

class QwtPlot;
class QWidget;

namespace MantidQt
{

namespace MantidWidgets {
class InputController;
}

namespace SliceViewer
{
   class PeaksPresenter;

  /** Widget representing a peak sphere on the plot. Used for representing spherically integrated peaks.
    
    @date 2012-08-22

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
  class EXPORT_OPT_MANTIDQT_SLICEVIEWER PeakOverlayMultiSphere : public PeakOverlayInteractive
  {
    Q_OBJECT

  public:
    /// Constructor
    PeakOverlayMultiSphere(PeaksPresenter* const presenter, QwtPlot * plot, QWidget * parent, const VecPhysicalSphericalPeak& vecPhysicalPeaks, const int plotXIndex, const int plotYIndex,
                           const QColor& peakColour, const QColor& backColour);
    /// Destructor
    virtual ~PeakOverlayMultiSphere();
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
    /// Show the background radius
    virtual void showBackgroundRadius(const bool show);
    /// Get a bounding box for this peak.
    virtual PeakBoundingBox getBoundingBox(const int peakIndex) const;
    /// Changes the size of the overlay to be the requested fraction of the current view width.
    virtual void changeOccupancyInView(const double fraction);
    /// Changes the size of the overlay to be the requested fraction of the view depth.
    virtual void changeOccupancyIntoView(const double fraction);
    /// Get the peak size (width/2 as a fraction of total width)  on projection
    virtual double getOccupancyInView() const;
    /// Get the peaks size into the projection (effective radius as a fraction of z range)
    virtual double getOccupancyIntoView() const;
    /// Getter indicating that the view is position only
    virtual bool positionOnly() const;
    /// Get the radius of the peak objects.
    virtual double getRadius() const;
    /// Determine if the background radius is shown.
    virtual bool isBackgroundShown() const;
    /// Get the current background colour
    virtual QColor getBackgroundColour() const;
    /// Get the current foreground colour
    virtual QColor getForegroundColour() const;
    /// Take settings from another view
    void takeSettingsFrom(const PeakOverlayView * const);

  private:

    /// Draw the peak representations. Pure virtual on base class.
    virtual void doPaintPeaks(QPaintEvent *);
    /// Physical peak object
    VecPhysicalSphericalPeak m_physicalPeaks;
    /// Peak colour
    QColor m_peakColour;
    /// Back colour
    QColor m_backColour;
    /// Peaks in the workspace that are viewable in the present view.
    std::vector<bool> m_viewablePeaks;
    /// Show the background radius.
    bool m_showBackground;
    /// Input controller.
  };


} // namespace SliceViewer
} // namespace Mantid

#endif  /* MANTID_SLICEVIEWER_PEAKOVERLAY_H_ */
