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
    PeakOverlay(QwtPlot * plot, QWidget * parent, const QPointF& origin, const QPointF& radius);
    virtual ~PeakOverlay();
    
    void setPlaneDistance(const double& distance); 

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
    QPointF m_origin;
    QPointF m_radius;
    const double m_opacityMax;
    const double m_opacityMin;
    double m_opacityAtDistance;

    double m_radiusXAtDistance;
    double m_radiusYAtDistance;

  };


} // namespace SliceViewer
} // namespace Mantid

#endif  /* MANTID_SLICEVIEWER_PEAKOVERLAY_H_ */
