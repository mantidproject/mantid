#include "MantidQtSliceViewer/PeakOverlay.h"
#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <iostream>
#include <qpainter.h>
#include <QRect>
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
  PeakOverlay::PeakOverlay(QwtPlot * plot, QWidget * parent)
  : QWidget( parent ),
    m_plot(plot)
  {
    m_origin = QPointF(0.0, 0.0);
    m_radius = 0.1;
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
  
  ////----------------------------------------------------------------------------------------------
  ///** Reset the line. User will have to click to create it */
  //void PeakOverlay::reset()
  //{
  //  m_creation = true; // Will create with the mouse
  //  m_rightButton = false;
  //  m_dragHandle = HandleNone;
  //  this->update();
  //}



  void PeakOverlay::setOrigin(QPointF origin)
  {
    m_origin = origin;
    this->update(); //repaint
  }

  void PeakOverlay::setRadius(double radius)
  {
    m_radius = radius;
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

    QPainter painter(this);
    painter.setRenderHint( QPainter::Antialiasing );

    //painter.setPen( Qt::black );
    painter.setBrush(Qt::red);
    painter.drawEllipse( m_origin, width()/2, height()/2 );

  }


} // namespace Mantid
} // namespace SliceViewer
