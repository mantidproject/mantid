#ifndef MANTID_SLICEVIEWER_PEAKOVERLAY_H_
#define MANTID_SLICEVIEWER_PEAKOVERLAY_H_

#include "DllOption.h"
#include "MantidKernel/System.h"
#include <q3iconview.h>
#include <QtCore/QtCore>
#include <QtGui/qwidget.h>
#include <qwt_plot.h>
#include <qpainter.h>


namespace MantidQt
{
namespace SliceViewer
{

  /** GUI for overlaying a peak circle on the plot.
    
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

    /// Enum giving IDs to the 4 handles on the widget
    enum eHandleID
    {
      HandleNone = -1,
      HandleA = 0,
      HandleB = 1,
      HandleWidthTop = 2,
      HandleWidthBottom = 3,
      HandleCenter = 4 // Anywhere inside the center
    };

  public:
    PeakOverlay(QwtPlot * plot, QWidget * parent);
    virtual ~PeakOverlay();
    
    void reset();

    void setOrigin(QPointF origin);
    void setRadius(double radius);

    const QPointF & getOrigin() const;
    double getRadius() const;

  signals:
    

  private:

    //QRect drawHandle(QPainter & painter, QPointF coords, QColor brush);
    void paintEvent(QPaintEvent *event);

    //eHandleID mouseOverHandle(QPoint pos);
    //bool mouseOverCenter(QPoint pos);
    //void handleDrag(QMouseEvent * event);
    //void mouseMoveEvent(QMouseEvent * event);
    //void mousePressEvent(QMouseEvent * event);
    //void mouseReleaseEvent(QMouseEvent * event);

    QSize sizeHint() const;
    QSize size() const;
    int height() const;
    int width() const;

  protected:

    /// QwtPlot containing this
    QwtPlot * m_plot;

    QPointF m_origin;

    double m_radius;

  };


} // namespace SliceViewer
} // namespace Mantid

#endif  /* MANTID_SLICEVIEWER_PEAKOVERLAY_H_ */
