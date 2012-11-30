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
#include <qcolor.h>
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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class EXPORT_OPT_MANTIDQT_SLICEVIEWER PeakOverlay : public QWidget, public PeakOverlayView
  {
    Q_OBJECT

  public:
    /// Constructor
    PeakOverlay(QwtPlot * plot, QWidget * parent, const Mantid::Kernel::V3D& origin, const double& radius, const QColor& peakColour);
    /// Destructor
    virtual ~PeakOverlay();
    /// Set the slice point at position.
    virtual void setSlicePoint(const double& point); 
    /// Hide the view.
    virtual void hideView();
    /// Show the view.
    virtual void showView();
    /// Update the view.
    virtual void updateView();
    /// Get the origin. md x, md y
    const Mantid::Kernel::V3D & getOrigin() const;
    /// Get the radius.
    double getRadius() const;
    /// Move the position of the peak, by using a different configuration of the existing origin indexes.
    void movePosition(PeakTransform_sptr peakTransform);

  private:

    //QRect drawHandle(QPainter & painter, QPointF coords, QColor brush);
    void paintEvent(QPaintEvent *event);

    QSize sizeHint() const;
    QSize size() const;
    int height() const;
    int width() const;

    /// QwtPlot containing this
    QwtPlot * m_plot;
    /// Original origin x=h, y=k, z=l
    const Mantid::Kernel::V3D m_originalOrigin;
    /// Origin md-x, md-y, and md-z
    Mantid::Kernel::V3D m_origin;
    /// actual peak radius
    const double m_radius;
    /// normalisation value.
    double m_normalisation;
    /// Max opacity
    const double m_opacityMax;
    /// Min opacity
    const double m_opacityMin;
    /// Peak colour
    QColor m_peakColour;
    /// Cached opacity at the distance z from origin
    double m_opacityAtDistance;
    /// Cached radius at the distance z from origin
    double m_radiusAtDistance;
    /// Cached scale
    double m_scale;
  };


} // namespace SliceViewer
} // namespace Mantid

#endif  /* MANTID_SLICEVIEWER_PEAKOVERLAY_H_ */
