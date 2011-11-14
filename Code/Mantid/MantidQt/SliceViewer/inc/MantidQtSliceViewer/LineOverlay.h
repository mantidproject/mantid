#ifndef MANTID_SLICEVIEWER_LINEOVERLAY_H_
#define MANTID_SLICEVIEWER_LINEOVERLAY_H_

#include "DllOption.h"
#include "MantidKernel/System.h"
#include <q3iconview.h>
#include <QtCore/QtCore>
#include <QtGui/qwidget.h>
#include <qwt_plot.h>


namespace MantidQt
{
namespace SliceViewer
{

  /** GUI for overlaying a line with a width onto the plot
   * in the SliceViewer. Should be generic to overlays on any QwtPlot.
   * Drag/droppable.
    
    @date 2011-11-14

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
  class EXPORT_OPT_MANTIDQT_SLICEVIEWER LineOverlay : public QWidget
  {
    Q_OBJECT

  public:
    LineOverlay(QwtPlot * parent);
    virtual ~LineOverlay();
    
    void setPointA(QPointF pointA);
    void setPointB(QPointF pointB);
    void setWidth(double width);

    const QPointF & getPointA() const;
    const QPointF & getPointB() const;
    double getWidth() const;

  private:
    QSize sizeHint() const;
    QSize size() const;
    int height() const;
    int width() const;
    void paintEvent(QPaintEvent *event);

  protected:
    /// QwtPlot containing this
    QwtPlot * m_plot;

    /// First point of the line (in coordinates of the plot)
    QPointF m_pointA;
    /// Second point of the line (in coordinates of the plot)
    QPointF m_pointB;
    /// Width of the line (in coordinates of the plot)
    double m_width;

  };


} // namespace SliceViewer
} // namespace Mantid

#endif  /* MANTID_SLICEVIEWER_LINEOVERLAY_H_ */
