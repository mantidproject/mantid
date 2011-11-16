#include "MantidQtSliceViewer/LineOverlay.h"
#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <iostream>
#include <qpainter.h>
#include <QRect>
#include <QShowEvent>


namespace MantidQt
{
namespace SliceViewer
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  LineOverlay::LineOverlay(QwtPlot * parent)
  : QWidget( parent->canvas() ),
    m_plot(parent)
  {
    m_pointA = QPointF(0.0, 0.0);
    m_pointB = QPointF(1.0, 1.0);
    m_width = 0.1;
    setAttribute(Qt::WA_TransparentForMouseEvents);
    // We need mouse events all the time
    setMouseTracking(true);
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  LineOverlay::~LineOverlay()
  {
  }
  
  //----------------------------------------------------------------------------------------------
  /** Set point A's position
   * @param pointA :: plot coordinates */
  void LineOverlay::setPointA(QPointF pointA)
  {
    m_pointA = pointA;
  }

  /** Set point B's position
   * @param pointB :: plot coordinates */
  void LineOverlay::setPointB(QPointF pointB)
  {
    m_pointB = pointB;
  }

  /** Set the width of integration
   * @param width :: in plot coordinates */
  void LineOverlay::setWidth(double width)
  {
    m_width = width;
  }

  //----------------------------------------------------------------------------------------------
  /// @return point A's position in plot coordinates
  const QPointF & LineOverlay::getPointA() const
  { return m_pointA; }

  /// @return point B's position in plot coordinates
  const QPointF & LineOverlay::getPointB() const
  { return m_pointB; }

  /// @return width of the line in plot coordinates
  double LineOverlay::getWidth() const
  { return m_width; }


  //----------------------------------------------------------------------------------------------
  /// Return the recommended size of the widget
  QSize LineOverlay::sizeHint() const
  {
    return QSize(20000, 20000);
    // Always as big as the canvas
    //return m_plot->canvas()->size();
  }

  QSize LineOverlay::size() const
  { return m_plot->canvas()->size(); }
  int LineOverlay::height() const
  { return m_plot->canvas()->height(); }
  int LineOverlay::width() const
  { return m_plot->canvas()->width(); }


  //----------------------------------------------------------------------------------------------
  /** Tranform from plot coordinates to pixel coordinates
   * @param coords :: coordinate point in plot coordinates
   * @return pixel coordinates */
  QPoint LineOverlay::transform(QPointF coords) const
  {
    int xA = m_plot->transform( QwtPlot::xBottom, coords.x() );
    int yA = m_plot->transform( QwtPlot::yLeft, coords.y() );
    return QPoint(xA, yA);
  }

  //----------------------------------------------------------------------------------------------
  /** Draw a handle (for dragging) at the given plot coordinates */
  QRect LineOverlay::drawHandle(QPainter & painter, QPointF coords)
  {
    int size = 8;
    QPoint center = transform(coords);
    QRect marker(center.x()-size/2, center.y()-size/2, size, size);
    painter.setPen(QColor(255,0,0));
    painter.setBrush(QColor(0,0,255));
    painter.drawRect(marker);
    return marker;
  }

  //----------------------------------------------------------------------------------------------
  /// Paint the overlay
  void LineOverlay::paintEvent(QPaintEvent */*event*/)
  {
    return;
    QPainter painter(this);
//    int r = rand() % 255;
//    int g = rand() % 255;
//    int b = rand() % 255;
//    painter.setBrush(QBrush(QColor(r,g,b)));

    QPointF diff = m_pointB - m_pointA;
    // Angle of the "width" perpendicular to the line
    double angle = atan2(diff.y(), diff.x()) + M_PI / 2.0;
    QPointF widthOffset( m_width * cos(angle), m_width * sin(angle) );

    // Rectangle with a rotation
    QPointF pA1 = m_pointA + widthOffset;
    QPointF pA2 = m_pointA - widthOffset;
    QPointF pB1 = m_pointB + widthOffset;
    QPointF pB2 = m_pointB - widthOffset;

    // Special XOR pixel drawing

    //painter.setCompositionMode( QPainter::RasterOp_SourceXorDestination ); // RHEL5 is too old for this ship

    // --- Draw the box ---
    QPen boxPen(QColor(255,255,255, 255));
    boxPen.setWidthF(1.0);
    painter.setPen(boxPen);
    painter.drawLine(transform(pA1), transform(pB1));
    painter.drawLine(transform(pB1), transform(pB2));
    painter.drawLine(transform(pB2), transform(pA2));
    painter.drawLine(transform(pA2), transform(pA1));

    // Go back to normal drawing mode
    painter.setCompositionMode( QPainter::CompositionMode_SourceOver );

    // --- Draw the central line ---
    QPen centerPen(QColor(255,255,255, 128));
    centerPen.setWidth(2);
    centerPen.setCapStyle(Qt::FlatCap);
    painter.setPen(centerPen);
    painter.drawLine(transform(m_pointA), transform(m_pointB));

    // --- Draw and store the rects of the 4 handles ---
    m_handles.clear();
    m_handles.push_back(drawHandle(painter, m_pointA));
    m_handles.push_back(drawHandle(painter, m_pointB));
    m_handles.push_back(drawHandle(painter, (m_pointA + m_pointB)/2 + widthOffset));
    m_handles.push_back(drawHandle(painter, (m_pointA + m_pointB)/2 - widthOffset));
  }


  //==============================================================================================
  //================================= MOUSE HANDLING =============================================
  //==============================================================================================

  /** Return the handle ID over which the mouse is
   * @param pos :: position in pixels of mouse */
  LineOverlay::eHandleID LineOverlay::mouseOverHandle(QPoint pos)
  {
    for (int i=0; i < m_handles.size(); i++)
    {
      if (m_handles[i].contains(pos))
      {
        return eHandleID(i);
      }
    }
    return HandleNone;
  }

  /** Event when the mouse moves */
  void LineOverlay::mouseMoveEvent(QMouseEvent * event)
  {
    LineOverlay::eHandleID hdl = mouseOverHandle(event->pos());
    switch (hdl)
    {
    case HandleA:
    case HandleB:
      this->setCursor(Qt::SizeHorCursor);
      break;
    case HandleWidthBottom:
    case HandleWidthTop:
      this->setCursor(Qt::SizeVerCursor);
      break;
    default:
      this->setCursor(Qt::CrossCursor);
      break;
    }
  }


} // namespace Mantid
} // namespace SliceViewer
