#include "MantidQtSliceViewer/PeakOverlaySphere.h"
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
  PeakOverlaySphere::PeakOverlaySphere(QwtPlot * plot, QWidget * parent, const Mantid::Kernel::V3D& origin, const double& radius, const QColor& peakColour)
  : QWidget( parent ),
    m_plot(plot),
    m_physicalPeak(origin, radius),
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
  PeakOverlaySphere::~PeakOverlaySphere()
  {
  }

  
  void PeakOverlaySphere::setSlicePoint(const double& z)
  {
    m_physicalPeak.setSlicePoint(z);

    this->update(); //repaint
  }

  //----------------------------------------------------------------------------------------------
  /// Return the recommended size of the widget
  QSize PeakOverlaySphere::sizeHint() const
  {
    //TODO: Is there a smarter way to find the right size?
    return QSize(20000, 20000);
    // Always as big as the canvas
    //return m_plot->canvas()->size();
  }

  QSize PeakOverlaySphere::size() const
  { return m_plot->canvas()->size(); }
  int PeakOverlaySphere::height() const
  { return m_plot->canvas()->height(); }
  int PeakOverlaySphere::width() const
  { return m_plot->canvas()->width(); }

  //----------------------------------------------------------------------------------------------
  /// Paint the overlay
  void PeakOverlaySphere::paintEvent(QPaintEvent * /*event*/)
  {
    const QwtDoubleInterval intervalY = m_plot->axisScaleDiv(QwtPlot::yLeft)->interval();
    const QwtDoubleInterval intervalX = m_plot->axisScaleDiv(QwtPlot::xBottom)->interval();
    
    const double scaleY = height()/(intervalY.width());
    const double scaleX = width()/(intervalX.width());

    // Calculate the physical drawing aspects using the Physical Peak.
    auto drawObject = m_physicalPeak.draw(height(), width(), intervalY.width(), intervalX.width());

    // Linear Transform from MD coordinates into Windows/Qt coordinates for ellipse rendering. TODO: This can be done outside of paintEvent.
    const int xOrigin = m_plot->transform( QwtPlot::xBottom, drawObject.peakOrigin.X() );
    const int yOrigin = m_plot->transform( QwtPlot::yLeft, drawObject.peakOrigin.Y() );
    const QPointF originWindows(xOrigin, yOrigin);


    QPainter painter(this);
    painter.setRenderHint( QPainter::Antialiasing );
    
    // Draw Outer circle
    QPen pen(m_peakColour);
    /* Note we are creating an ellipse here and generating a filled effect by controlling the line thickness.
       Since the linewidth takes a single scalar value, we choose to use x as the scale value.
    */
    pen.setWidth(static_cast<int>(std::abs(drawObject.peakLineWidth)));
    painter.setPen( pen );  
    
    pen.setStyle(Qt::SolidLine);
    painter.setOpacity(drawObject.peakOpacityAtDistance); //Set the pre-calculated opacity
    painter.drawEllipse( originWindows, drawObject.peakOuterRadiusX, drawObject.peakOuterRadiusY );
    
  }

  void PeakOverlaySphere::updateView()
  {
    this->update();
  }

  void PeakOverlaySphere::hideView()
  {
    this->hide();
  }

  void PeakOverlaySphere::showView()
  {
    this->show();
  }

  void PeakOverlaySphere::movePosition(PeakTransform_sptr transform)
  {
    m_physicalPeak.movePosition(transform);
  }

} // namespace Mantid
} // namespace SliceViewer
