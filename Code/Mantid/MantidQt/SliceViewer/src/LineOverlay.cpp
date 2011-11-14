#include "MantidQtSliceViewer/LineOverlay.h"
#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <iostream>
#include <qpainter.h>


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
  void LineOverlay::paintEvent(QPaintEvent */*event*/)
  {
    return;
    QPainter painter(this);
    painter.setCompositionMode( QPainter::CompositionMode_Xor );

    int r = rand() % 255;
    int g = rand() % 255;
    int b = rand() % 255;
    painter.setBrush(QBrush(QColor(r,g,b)));

    QPointF diff = m_pointB - m_pointA;
    // Angle of the "width" perpendicular to the line
    double angle = atan2(diff.y(), diff.x()) + M_PI / 2.0;
    QPointF widthOffset( m_width * cos(angle), m_width * sin(angle) );

    // Rectangle with a rotation
    QPointF pA1 = m_pointA + widthOffset;
    QPointF pA2 = m_pointA - widthOffset;
    QPointF pB1 = m_pointB + widthOffset;
    QPointF pB2 = m_pointB - widthOffset;

    // --- Draw the central line ---
    QPen centerPen(QColor(155,155,155, 250));
    centerPen.setWidthF(10.0);
    centerPen.setCapStyle(Qt::FlatCap);
    painter.setPen(centerPen);
    painter.drawLine(transform(m_pointA), transform(m_pointB));

    // --- Draw the box ---
    QPen boxPen(QColor(255,255,255, 250));
    boxPen.setWidthF(1.0);
    painter.setPen(boxPen);
    painter.drawLine(transform(pA1), transform(pB1));
    painter.drawLine(transform(pB1), transform(pB2));
    painter.drawLine(transform(pB2), transform(pA2));
    painter.drawLine(transform(pA2), transform(pA1));

    //std::cout << "LineOverlay::paintEvent " << x1 << "," << y1 << " : " << x2 << "," << y2 << "\n";
  }


} // namespace Mantid
} // namespace SliceViewer
