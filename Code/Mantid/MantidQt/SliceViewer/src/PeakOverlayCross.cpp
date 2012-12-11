#include "MantidQtSliceViewer/PeakOverlayCross.h"
#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qwt_scale_div.h>
#include <qpainter.h>
#include <QPen>
#include <QMouseEvent>

using namespace Mantid::Kernel;


namespace MantidQt
{
namespace SliceViewer
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  PeakOverlayCross::PeakOverlayCross(QwtPlot * plot, QWidget * parent, const Mantid::Kernel::V3D& origin, const double& maxZ, const double& minZ, const QColor& peakColour)
  : QWidget( parent ),
    m_plot(plot),
    m_originalOrigin(origin),
    m_origin(origin),
    m_effectiveRadius((maxZ - minZ)*0.015),
    m_opacityMax(0.8),
    m_opacityMin(0.0),
    m_crossViewFraction(0.015),
    m_peakColour(peakColour)
  {
    setAttribute(Qt::WA_NoMousePropagation, false);
    this->setVisible(true);
    setUpdatesEnabled(true);

    setAttribute(Qt::WA_TransparentForMouseEvents);
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  PeakOverlayCross::~PeakOverlayCross()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Set the distance between the plane and the center of the peak in md coordinates

  @param z : position of the plane slice in the z dimension.

  */
  void PeakOverlayCross::setSlicePoint(const double& z)
  {
    const double distanceAbs = std::abs(z - m_origin.Z());

    // Apply a linear transform to convert from a distance to an opacity between opacityMin and opacityMax.
    m_opacityAtDistance = ((m_opacityMin - m_opacityMax)/m_effectiveRadius) * distanceAbs  + m_opacityMax;
    m_opacityAtDistance = m_opacityAtDistance >= m_opacityMin ? m_opacityAtDistance : m_opacityMin;

    this->update(); //repaint
  }

  const Mantid::Kernel::V3D & PeakOverlayCross::getOrigin() const
  { return m_origin; }

  //----------------------------------------------------------------------------------------------
  /// Return the recommended size of the widget
  QSize PeakOverlayCross::sizeHint() const
  {
    //TODO: Is there a smarter way to find the right size?
    return QSize(20000, 20000);
    // Always as big as the canvas
    //return m_plot->canvas()->size();
  }

  QSize PeakOverlayCross::size() const
  { return m_plot->canvas()->size(); }
  int PeakOverlayCross::height() const
  { return m_plot->canvas()->height(); }
  int PeakOverlayCross::width() const
  { return m_plot->canvas()->width(); }

  //----------------------------------------------------------------------------------------------
  /// Paint the overlay
  void PeakOverlayCross::paintEvent(QPaintEvent * /*event*/)
  {
    QPainter painter(this);
    painter.setRenderHint( QPainter::Antialiasing );

    const int xOrigin = m_plot->transform( QwtPlot::xBottom, m_origin.X() );
    const int yOrigin = m_plot->transform( QwtPlot::yLeft, m_origin.Y() );
    const QPointF originWindows(xOrigin, yOrigin);
    
    QPen pen(m_peakColour);
    pen.setWidth(2); 
    painter.setPen( pen );  
    
    pen.setStyle(Qt::SolidLine);
    painter.setOpacity(m_opacityAtDistance); //Set the pre-calculated opacity

    const int halfCrossHeight = int(double(height()) * m_crossViewFraction);
    const int halfCrossWidth = halfCrossHeight;

    QPoint bottomL(originWindows.x() - halfCrossWidth, originWindows.y() - halfCrossHeight);
    QPoint bottomR(originWindows.x() + halfCrossWidth, originWindows.y() - halfCrossHeight);
    QPoint topL(originWindows.x() - halfCrossWidth, originWindows.y() + halfCrossHeight);
    QPoint topR(originWindows.x() + halfCrossWidth, originWindows.y() + halfCrossHeight);

    painter.drawLine(bottomL, topR);
    painter.drawLine(bottomR, topL);
  }

  void PeakOverlayCross::updateView()
  {
    this->update();
  }

  void PeakOverlayCross::hideView()
  {
    this->hide();
  }

  void PeakOverlayCross::showView()
  {
    this->show();
  }

  void PeakOverlayCross::movePosition(PeakTransform_sptr transform)
  {
    // Will have the plots x, y, and z aligned to the correct h, k, l value.
    m_origin = transform->transform(this->m_originalOrigin);
  }

} // namespace Mantid
} // namespace SliceViewer