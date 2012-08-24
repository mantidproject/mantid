#ifndef MANTID_SLICEVIEWER_PEAKOVERLAY_H_
#define MANTID_SLICEVIEWER_PEAKOVERLAY_H_

#include "DllOption.h"
#include "MantidKernel/System.h"
#include <q3iconview.h>
#include <QtCore/QtCore>
#include <QtGui/qwidget.h>
#include <qwt_plot.h>
#include <qwt_plot_spectrogram.h>
#include <qpainter.h>



namespace MantidQt
{
namespace SliceViewer
{

  /** GUI for overlaying a peak ellipse on the plot.
    
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
  class EXPORT_OPT_MANTIDQT_SLICEVIEWER PeakOverlay : public QWidget
  {
    Q_OBJECT

  public:
    /// Constructor
    PeakOverlay(QwtPlot * plot, QWidget * parent, const QPointF& origin, const QPointF& radius);
    /// Destructor
    virtual ~PeakOverlay();
    /// Set the distance between the origin and the plane in the z-md-coordinate system.
    void setPlaneDistance(const double& dz); 
    /// Get the origin. md x, md y
    const QPointF & getOrigin() const;
    double getRadius() const;

  signals:
    

  private:

    //QRect drawHandle(QPainter & painter, QPointF coords, QColor brush);
    void paintEvent(QPaintEvent *event);

    QSize sizeHint() const;
    QSize size() const;
    int height() const;
    int width() const;

    /// QwtPlot containing this
    QwtPlot * m_plot;
    /// Origin md-x, md-y
    QPointF m_origin;
    /// Radius md-x, md-y
    QPointF m_radius;
    /// Max opacity
    const double m_opacityMax;
    /// Min opacity
    const double m_opacityMin;
    /// Cached opacity at the distance z from origin
    double m_opacityAtDistance;
    /// Cached radius at the distance z from origin
    double m_radiusXAtDistance;
    /// Cached radius x at the distance x from origin, in md-x coordinates
    double m_radiusYAtDistance;
    /// Cached radius y at the distance y from origin, in md-y coordinates
  };


} // namespace SliceViewer
} // namespace Mantid

#endif  /* MANTID_SLICEVIEWER_PEAKOVERLAY_H_ */
