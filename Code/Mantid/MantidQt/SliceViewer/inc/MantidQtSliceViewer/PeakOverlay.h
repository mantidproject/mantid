#ifndef MANTID_SLICEVIEWER_PEAKOVERLAY_H_
#define MANTID_SLICEVIEWER_PEAKOVERLAY_H_

#include "DllOption.h"
#include "MantidKernel/System.h"
#include "MantidKernel/V3D.h"
#include <q3iconview.h>
#include <QtCore/QtCore>
#include <QtGui/qwidget.h>
#include <qwt_plot.h>
#include <qpainter.h>
#include "MantidQtSliceViewer/PeakOverlayView.h"


namespace MantidQt
{
namespace SliceViewer
{

  /** Widget representing a peak ellipse on the plot.
    
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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class EXPORT_OPT_MANTIDQT_SLICEVIEWER PeakOverlay : public QWidget, public PeakOverlayView
  {
    Q_OBJECT

  public:
    /// Constructor
    PeakOverlay(QwtPlot * plot, QWidget * parent, const Mantid::Kernel::V3D& origin, const double& radius);
    /// Destructor
    virtual ~PeakOverlay();
    /// Set the slice point at position.
    virtual void setSlicePoint(const double& point); 
    /// Update the view.
    virtual void updateView();
    /// Get the origin. md x, md y
    const Mantid::Kernel::V3D & getOrigin() const;
    double getRadius() const;

  private:

    //QRect drawHandle(QPainter & painter, QPointF coords, QColor brush);
    void paintEvent(QPaintEvent *event);

    QSize sizeHint() const;
    QSize size() const;
    int height() const;
    int width() const;

    /// QwtPlot containing this
    QwtPlot * m_plot;
    /// Origin md-x, md-y, and md-z
    Mantid::Kernel::V3D m_origin;
    /// Radius md-x, md-y
    double m_radius;
    /// Max opacity
    const double m_opacityMax;
    /// Min opacity
    const double m_opacityMin;
    /// Cached opacity at the distance z from origin
    double m_opacityAtDistance;
    /// Cached radius at the distance z from origin
    double m_radiusAtDistance;
    /// Cached radius at distance
  };


} // namespace SliceViewer
} // namespace Mantid

#endif  /* MANTID_SLICEVIEWER_PEAKOVERLAY_H_ */
