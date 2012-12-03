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
  PeakOverlay::PeakOverlay(QwtPlot * plot, QWidget * parent, const Mantid::Kernel::V3D& origin, const double& radius, const QColor& peakColour)
  : QWidget( parent ),
    m_plot(plot),
    m_originalOrigin(origin),
    m_origin(origin),
    m_radius(radius),
    m_opacityMax(0.8),
    m_opacityMin(0.0),
    m_peakColour(peakColour)
  {
    setAttribute(Qt::WA_NoMousePropagation, false);
    this->setVisible(true);
    setUpdatesEnabled(true);
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

  @param atPoint : distance from the peak center in the md coordinates of the z-axis.

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
    const double distanceSQ = (z - m_origin.Z()) * (z - m_origin.Z());
    const double distance = std::sqrt(distanceSQ);
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
  { 
    return m_radius;
  }

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

    const QwtDoubleInterval intervalY = m_plot->axisScaleDiv(QwtPlot::yLeft)->interval();
    const QwtDoubleInterval intervalX = m_plot->axisScaleDiv(QwtPlot::xBottom)->interval();
    
    const double scaleY = height()/(intervalY.width());
    const double scaleX = width()/(intervalX.width());

    const double innerRadiusX = scaleX * m_radiusAtDistance;
    const double innerRadiusY = scaleY * m_radiusAtDistance;

    double outerRadiusX = scaleX * getRadius();
    double outerRadiusY = scaleY * getRadius();

    const double lineWidthX = outerRadiusX - innerRadiusX;
    const double lineWidthY = outerRadiusY - innerRadiusY;
    outerRadiusX -= lineWidthX/2;
    outerRadiusY -= lineWidthY/2;

    QPainter painter(this);
    painter.setRenderHint( QPainter::Antialiasing );
    
    // Draw Outer circle
    QPen pen(m_peakColour);
    /* Note we are creating an ellipse here and generating a filled effect by controlling the line thickness.
       Since the linewidth takes a single scalar value, we choose to use x as the scale value.
    */
    pen.setWidth(static_cast<int>(std::abs(lineWidthX)));
    painter.setPen( pen );  
    
    pen.setStyle(Qt::SolidLine);
    painter.setOpacity(m_opacityAtDistance); //Set the pre-calculated opacity
    painter.drawEllipse( originWindows, outerRadiusX, outerRadiusY );
    
  }

  void PeakOverlay::updateView()
  {
    this->update();
  }

  void PeakOverlay::hideView()
  {
    this->hide();
  }

  void PeakOverlay::showView()
  {
    this->show();
  }

  void PeakOverlay::movePosition(const PeakTransform& transform)
  {
    // Will have the plots x, y, and z aligned to the correct h, k, l value.
    m_origin = transform.transform(this->m_originalOrigin);
  }

} // namespace Mantid
} // namespace SliceViewer
