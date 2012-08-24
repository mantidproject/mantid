#include "MantidQtSliceViewer/PeakOverlay.h"
#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qwt_scale_div.h>
#include <iostream>
#include <qpainter.h>
#include <QRect>
#include <QPen>
#include <QBrush>
#include <QShowEvent>
#include "MantidKernel/Utils.h"

using namespace Mantid::Kernel;


namespace MantidQt
{
namespace SliceViewer
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  PeakOverlay::PeakOverlay(QwtPlot * plot, QWidget * parent, const QPointF& origin, const QPointF& radius)
  : QWidget( parent ),
    m_plot(plot),
    m_origin(origin),
    m_radius(radius),
    m_opacityMax(1),
    m_opacityMin(0.1)
  {
    //setAttribute(Qt::WA_TransparentForMouseEvents);
    // We need mouse events all the time
    setMouseTracking(true);
    //setAttribute(Qt::WA_TransparentForMouseEvents);
    // Make sure mouse propagates
    setAttribute(Qt::WA_NoMousePropagation, false);
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  PeakOverlay::~PeakOverlay()
  {
  }

  void PeakOverlay::setPlaneDistance(const double& distance)
  {
    const double distanceSQ = distance * distance;
    m_radiusXAtDistance = std::sqrt( (m_radius.x() * m_radius.x()) - distanceSQ );
    m_radiusYAtDistance = std::sqrt( (m_radius.y() * m_radius.y()) - distanceSQ );

    // Apply a linear transform to convert from a distance to an opacity between opacityMin and opacityMax.
    m_opacityAtDistance = ((m_opacityMin - m_opacityMax)/m_radius.x()) * distance  + m_opacityMax;
    m_opacityAtDistance = m_opacityAtDistance >= m_opacityMin ? m_opacityAtDistance : m_opacityMin;

    this->update(); //repaint
  }

  const QPointF & PeakOverlay::getOrigin() const
  { return m_origin; }

  double PeakOverlay::getRadius() const
  { return m_radius; }

  //----------------------------------------------------------------------------------------------
  /// Return the recommended size of the widget
  QSize PeakOverlay::sizeHint() const
  {
    //TODO: Is there a smarter way to find the right size?
    return QSize(20000, 20000);
    // Always as big as the canvas
    //return m_plot->canvas()->size();
  }

  QSize PeakOverlay::size() const
  { return m_plot->canvas()->size(); }
  int PeakOverlay::height() const
  { return m_plot->canvas()->height(); }
  int PeakOverlay::width() const
  { return m_plot->canvas()->width(); }



  //----------------------------------------------------------------------------------------------
  /// Paint the overlay
  void PeakOverlay::paintEvent(QPaintEvent * /*event*/)
  {
    // Linear Transform from MD coordinates into Windows/Qt coordinates for ellipse rendering.
    const int xOrigin = m_plot->transform( QwtPlot::xBottom, m_origin.x() );
    const int yOrigin = m_plot->transform( QwtPlot::yLeft, m_origin.y() );
    const QPointF originWindows(xOrigin, yOrigin);
    
    const double xMin = m_plot->axisScaleDiv(QwtPlot::xBottom)->lowerBound();
    const double  xMax = m_plot->axisScaleDiv(QwtPlot::xBottom)->upperBound();
    const double scaleX = width()/(xMax - xMin);

    const double  yMin = m_plot->axisScaleDiv(QwtPlot::yLeft)->lowerBound();
    const double  yMax = m_plot->axisScaleDiv(QwtPlot::yLeft)->upperBound();
    const double scaleY = height()/(yMax - yMin);

    int rx = scaleX * m_radiusXAtDistance;
    int ry = scaleY * m_radiusYAtDistance;

    // Draw circle and inner circle.
    QPainter painter(this);
    painter.setRenderHint( QPainter::Antialiasing );

    painter.setOpacity(m_opacityAtDistance);
    painter.setBrush(Qt::cyan);
    painter.drawEllipse( originWindows, rx, ry );

    QPen pen( Qt::green );
    pen.setWidth(2);
    painter.setPen( pen );
    painter.drawEllipse( originWindows, rx, ry );

  }


} // namespace Mantid
} // namespace SliceViewer
