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
    m_creation = true; // Will create with the mouse
    m_middleButton = false;
    m_dragHandle = HandleNone;

    m_pointA = QPointF(0.0, 0.0);
    m_pointB = QPointF(1.0, 1.0);
    m_width = 0.1;
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
  LineOverlay::~LineOverlay()
  {
  }
  
  //----------------------------------------------------------------------------------------------
  /** Set point A's position
   * @param pointA :: plot coordinates */
  void LineOverlay::setPointA(QPointF pointA)
  {
    m_pointA = pointA;
    this->update(); //repaint
    emit lineChanging(m_pointA, m_pointB, m_width);
  }

  /** Set point B's position
   * @param pointB :: plot coordinates */
  void LineOverlay::setPointB(QPointF pointB)
  {
    m_pointB = pointB;
    this->update(); //repaint
    emit lineChanging(m_pointA, m_pointB, m_width);
  }

  /** Set the width of integration
   * @param width :: in plot coordinates */
  void LineOverlay::setWidth(double width)
  {
    m_width = width;
    this->update(); //repaint
    emit lineChanging(m_pointA, m_pointB, m_width);
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
    //TODO: Is there a smarter way to find the right size?
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
  /** Inverse transform: from pixels to plot coords
   * @param pixels :: location in pixels
   * @return plot coordinates (float)   */
  QPointF LineOverlay::invTransform(QPoint pixels) const
  {
    double xA = m_plot->invTransform( QwtPlot::xBottom, pixels.x() );
    double yA = m_plot->invTransform( QwtPlot::yLeft, pixels.y() );
    return QPointF(xA, yA);
  }

  //----------------------------------------------------------------------------------------------
  /** Draw a handle (for dragging) at the given plot coordinates */
  QRect LineOverlay::drawHandle(QPainter & painter, QPointF coords, QColor brush)
  {
    int size = 8;
    QPoint center = transform(coords);
    QRect marker(center.x()-size/2, center.y()-size/2, size, size);
    painter.setPen(QColor(255,0,0));
    painter.setBrush(brush);
    painter.drawRect(marker);
    return marker;
  }

  //----------------------------------------------------------------------------------------------
  /// Paint the overlay
  void LineOverlay::paintEvent(QPaintEvent */*event*/)
  {
    // Don't paint until created
    // Also, don't paint while middle-click dragging (panning) the underlying pic
    if (m_creation || m_middleButton)
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


    QPen boxPen(QColor(255,255,255, 255));
    QPen centerPen(QColor(192,192,192, 128));

#if QT_VERSION >= 0x040100
    // Special XOR pixel drawing
    painter.setCompositionMode( QPainter::RasterOp_SourceXorDestination );
#else
    // RHEL5 has an old version of QT?
    boxPen = QPen(QColor(255,128,128, 255));
#endif

//    painter.setCompositionMode( QPainter::CompositionMode_Plus );

    // --- Draw the box ---
//    boxPen = QColor(r,g,b, 255);
    boxPen.setWidthF(1.0);
    painter.setPen(boxPen);
    painter.drawLine(transform(pA1), transform(pB1));
    painter.drawLine(transform(pB1), transform(pB2));
    painter.drawLine(transform(pB2), transform(pA2));
    painter.drawLine(transform(pA2), transform(pA1));

    // Go back to normal drawing mode
    painter.setCompositionMode( QPainter::CompositionMode_SourceOver );

    // --- Draw the central line ---
    centerPen.setWidth(2);
    centerPen.setCapStyle(Qt::FlatCap);
    painter.setPen(centerPen);
    painter.drawLine(transform(m_pointA), transform(m_pointB));

    // --- Draw and store the rects of the 4 handles ---
    m_handles.clear();
    m_handles.push_back(drawHandle(painter, m_pointA, QColor(255,255,255) ));
    m_handles.push_back(drawHandle(painter, m_pointB, QColor(0,0,0) ));
    m_handles.push_back(drawHandle(painter, (m_pointA + m_pointB)/2 + widthOffset, QColor(0,255,255)) );
    m_handles.push_back(drawHandle(painter, (m_pointA + m_pointB)/2 - widthOffset, QColor(0,255,255)) );
  }


  //==============================================================================================
  //================================= MOUSE HANDLING =============================================
  //==============================================================================================

  //-----------------------------------------------------------------------------------------------
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

  //-----------------------------------------------------------------------------------------------
  /** Event when the mouse moves
   * @param mouse event info */
  void LineOverlay::mouseMoveEvent(QMouseEvent * event)
  {
    if (event->buttons() & Qt::MidButton)
      m_middleButton = true;

    // --- Initial creation mode - wait for first click ----
    if (m_creation)
    {
      // TODO: Custom mouse cursor?
      this->setCursor(Qt::PointingHandCursor);
      return;
    }


    // --- Initial creation mode ----
    if (m_dragHandle != HandleNone)
    {
      // Currently dragging!
      QPointF current = this->invTransform( event->pos() );
      QPointF diff = m_pointB - m_pointA;
      double width = 0;

      switch (m_dragHandle)
      {
      case HandleA:
        setPointA(current);
        break;
      case HandleB:
        setPointB(current);
        break;

      case HandleWidthBottom:
      case HandleWidthTop:
        // Find the distance between the mouse and the line (see http://mathworld.wolfram.com/Point-LineDistance2-Dimensional.html )
        width = fabs( diff.x()*(current.y()-m_pointA.y()) - (current.x() - m_pointA.x())*diff.y() )
            / sqrt(diff.x()*diff.x() + diff.y()*diff.y());
        setWidth(width);
        break;

      default:
        break;
      }
    }
    else
    {
      // ---- Just moving the mouse -------------
      if (event->buttons() == Qt::NoButton)
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
          // Pass-through event to underlying widget if not over a marker
          event->ignore();
          break;
        }
      }
      else
        // Don't change mouse cursor if dragging the underlying control
        event->ignore();
    }
  }

  //-----------------------------------------------------------------------------------------------
  /** Event when the mouse button is pressed down
   * @param mouse event info */
  void LineOverlay::mousePressEvent(QMouseEvent * event)
  {

    // First left-click = create!
    if (m_creation && (event->buttons() & Qt::LeftButton))
    {
      QPointF pt = this->invTransform( event->pos() );
      m_pointB = pt;
      setPointA(pt);
      // And now we are in drag B mode
      m_creation = false;
      m_dragStart = pt;
      m_dragHandle = HandleB;
      return;
    }

    LineOverlay::eHandleID hdl = mouseOverHandle(event->pos());
    // Drag with the left mouse button
    if (hdl != HandleNone && (event->buttons() & Qt::LeftButton))
    {
      // Start dragging
      m_dragHandle = hdl;
      m_dragStart = this->invTransform( event->pos() );
    }
    else
      // Pass-through event to underlying widget if not over a marker
      event->ignore();

  }

  //-----------------------------------------------------------------------------------------------
  /** Event when the mouse moves
   * @param mouse event info */
  void LineOverlay::mouseReleaseEvent(QMouseEvent * event)
  {
    if (!(event->buttons() & Qt::MidButton))
      m_middleButton = false;

    if (m_dragHandle != HandleNone)
    {
      // Stop draggin
      m_dragHandle = HandleNone;
      // Drag is over - signal that
      emit lineChanged(m_pointA, m_pointB, m_width);
    }
    else
      // Pass-through event to underlying widget if not dragging
      event->ignore();

  }


} // namespace Mantid
} // namespace SliceViewer
