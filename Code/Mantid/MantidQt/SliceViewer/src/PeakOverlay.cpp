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
  PeakOverlay::PeakOverlay(QwtPlot * plot, QWidget * parent, const Mantid::Kernel::V3D& origin, const double& radius)
  : QWidget( parent ),
    m_plot(plot),
    m_origin(origin),
    m_radius(radius),
    m_opacityMax(1),
    m_opacityMin(0.1)
  {
    setAttribute(Qt::WA_NoMousePropagation, false);
    this->setVisible(true);
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  PeakOverlay::~PeakOverlay()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Set the distance between the plane and the center of the peak in md coordinates

  ASCII diagram below to demonstrate how dz (distance in z) is used to determine the radius of the sphere-plane intersection at that point,
  resloves both rx and ry. Also uses the distance to calculate the opacity to apply.

  @param atPoint : distance from the peak cetner in the md coordinates of the z-axis.

       /---------\
      /           \
  ---/---------rx--\---------------- plane
     |    dz|     /| peak
     |      |   /  |
     |      . /    |
     |             |
     \             /
      \           /
       \---------/
  */
  void PeakOverlay::setSlicePoint(const double& z)
  {
    const double distance = z - m_origin.Z();
    const double distanceSQ = distance * distance;
    const double radSQ = m_radius * m_radius;

    if(distanceSQ < radSQ)
    {
      m_radiusAtDistance = std::sqrt( radSQ - distanceSQ );
    }
    else
    {
      m_radiusAtDistance = 0;
    }
    

    // Apply a linear transform to convert from a distance to an opacity between opacityMin and opacityMax.
    m_opacityAtDistance = ((m_opacityMin - m_opacityMax)/m_radius) * distance  + m_opacityMax;
    m_opacityAtDistance = m_opacityAtDistance >= m_opacityMin ? m_opacityAtDistance : m_opacityMin;

    this->update(); //repaint
  }

  const Mantid::Kernel::V3D & PeakOverlay::getOrigin() const
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
    // Linear Transform from MD coordinates into Windows/Qt coordinates for ellipse rendering. TODO: This can be done outside of paintEvent.
    const int xOrigin = m_plot->transform( QwtPlot::xBottom, m_origin.X() );
    const int yOrigin = m_plot->transform( QwtPlot::yLeft, m_origin.Y() );
    const QPointF originWindows(xOrigin, yOrigin);

    const QwtDoubleInterval interval = m_plot->axisScaleDiv(QwtPlot::yLeft)->interval();
    const double yMin = interval.minValue();
    const double yMax = interval.maxValue();
    const double scale = height()/(yMax - yMin);

    const double radius = scale * m_radiusAtDistance;

    // Draw circle and inner circle.
    QPainter painter(this);
    painter.setRenderHint( QPainter::Antialiasing );

    painter.setOpacity(m_opacityAtDistance); //Set the pre-calculated opacity
    painter.setBrush(Qt::cyan);
    painter.drawEllipse( originWindows, radius, radius );

    QPen pen( Qt::green );
    pen.setWidth(2);
    painter.setPen( pen );
    painter.drawEllipse( originWindows, radius, radius );

  }

  void PeakOverlay::updateView()
  {
    this->update();
    this->repaint();
  }


} // namespace Mantid
} // namespace SliceViewer
