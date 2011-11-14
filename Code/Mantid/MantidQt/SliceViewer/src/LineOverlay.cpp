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
  void LineOverlay::paintEvent(QPaintEvent */*event*/)
  {

    QPainter painter(this);
    int r = rand() % 255;
    int g = rand() % 255;
    int b = rand() % 255;
    painter.setPen(QColor(r,g,b));
    painter.setBrush(QBrush(QColor(r,g,b)));

//    int x1 = m_plot->transform( QwtPlot::xBottom, m_pointA.x() );
//    int y1 = m_plot->transform( QwtPlot::yLeft, m_pointA.y() );
//    int x2 = m_plot->transform( QwtPlot::xBottom, m_pointB.x() );
//    int y2 = m_plot->transform( QwtPlot::yLeft, m_pointB.y() );

    //std::cout << "LineOverlay::paintEvent " << x1 << "," << y1 << " : " << x2 << "," << y2 << "\n";

//    painter.drawLine(x1,y1, x2,y2);
  }


} // namespace Mantid
} // namespace SliceViewer
