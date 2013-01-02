#ifndef MANTID_SLICEVIEWER_PEAKOVERLAYSPHERE_H_
#define MANTID_SLICEVIEWER_PEAKOVERLAYSPHERE_H_

#include "DllOption.h"
#include "MantidKernel/System.h"
#include "MantidKernel/V3D.h"
#include <q3iconview.h>
#include <QtCore/QtCore>
#include <QtGui/qwidget.h>
#include <qwt_plot.h>
#include <qpainter.h>
#include <qcolor.h>
#include "MantidQtSliceViewer/PeakOverlayView.h"
#include "MantidQtSliceViewer/PhysicalSphericalPeak.h"


namespace MantidQt
{
namespace SliceViewer
{

  /** Widget representing a peak sphere on the plot. Used for representing spherically integrated peaks.
    
    @date 2012-08-22

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class EXPORT_OPT_MANTIDQT_SLICEVIEWER PeakOverlaySphere : public QWidget, public PeakOverlayView
  {
    Q_OBJECT

  public:
    /// Constructor
    PeakOverlaySphere(QwtPlot * plot, QWidget * parent, const Mantid::Kernel::V3D& origin, const double& peakRadius, const double& backgroundInnerRadius, const double& backgroundOuterRadius, const QColor& peakColour);
    /// Destructor
    virtual ~PeakOverlaySphere();
    /// Set the slice point at position.
    virtual void setSlicePoint(const double& point); 
    /// Hide the view.
    virtual void hideView();
    /// Show the view.
    virtual void showView();
    /// Update the view.
    virtual void updateView();
    /// Move the position of the peak, by using a different configuration of the existing origin indexes.
    void movePosition(PeakTransform_sptr peakTransform);
    /// Change foreground colour
    virtual void changeForegroundColour(const QColor);
    /// Change background colour
    virtual void changeBackgroundColour(const QColor);
    /// Show the background radius
    virtual void showBackgroundRadius(const bool show);

  private:

    //QRect drawHandle(QPainter & painter, QPointF coords, QColor brush);
    void paintEvent(QPaintEvent *event);

    QSize sizeHint() const;
    QSize size() const;
    int height() const;
    int width() const;

    /// QwtPlot containing this
    QwtPlot * m_plot;
    /// Physical peak object
    PhysicalSphericalPeak m_physicalPeak;
    /// Peak colour
    QColor m_peakColour;
  };


} // namespace SliceViewer
} // namespace Mantid

#endif  /* MANTID_SLICEVIEWER_PEAKOVERLAY_H_ */
