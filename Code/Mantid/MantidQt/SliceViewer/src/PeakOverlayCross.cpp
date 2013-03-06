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
    m_physicalPeak(origin, maxZ, minZ),
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
    m_physicalPeak.setSlicePoint(z);

    this->update(); //repaint
  }


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
    if(m_physicalPeak.isViewable())
    {
      QPainter painter(this);
      painter.setRenderHint( QPainter::Antialiasing );

      auto drawObject = m_physicalPeak.draw(height(), width());

      const int xOriginWindows = m_plot->transform( QwtPlot::xBottom, drawObject.peakOrigin.X() );
      const int yOriginWindows = m_plot->transform( QwtPlot::yLeft, drawObject.peakOrigin.Y() );

      QPen pen(m_peakColour);
      pen.setWidth(drawObject.peakLineWidth); 
      painter.setPen( pen );  

      pen.setStyle(Qt::SolidLine);
      painter.setOpacity(drawObject.peakOpacityAtDistance); //Set the pre-calculated opacity

      const int halfCrossHeight = drawObject.peakHalfCrossHeight;
      const int halfCrossWidth = drawObject.peakHalfCrossWidth;

      QPoint bottomL(xOriginWindows - halfCrossWidth, yOriginWindows - halfCrossHeight);
      QPoint bottomR(xOriginWindows + halfCrossWidth, yOriginWindows - halfCrossHeight);
      QPoint topL(xOriginWindows - halfCrossWidth, yOriginWindows + halfCrossHeight);
      QPoint topR(xOriginWindows + halfCrossWidth, yOriginWindows + halfCrossHeight);

      painter.drawLine(bottomL, topR);
      painter.drawLine(bottomR, topL);
    }
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
    m_physicalPeak.movePosition(transform);
  }

  void PeakOverlayCross::changeForegroundColour(const QColor colour)
  {
    this->m_peakColour = QColor(colour);
  }

  void PeakOverlayCross::changeBackgroundColour(const QColor)
  {
    // Do nothing with the background colour for a peak widget of this type.
  }

  /**
  @return bounding box for peak in windows coordinates.
  */
  PeakBoundingBox PeakOverlayCross::getBoundingBox() const
  {
    return m_physicalPeak.getBoundingBox();
  }

  /**
     * Set the occupancy into the view as a fraction of the current view width.
     * @param fraction to use.
     */
    void PeakOverlayCross::changeOccupancyInView(const double fraction)
    {
      m_physicalPeak.setOccupancyInView(fraction);
    }

    /**
     * Set the occupancy into the view as a fraction of the current view depth.
     * @param fraction to use.
     */
    void PeakOverlayCross::changeOccupancyIntoView(const double fraction)
    {
      m_physicalPeak.setOccupancyIntoView(fraction);
    }

    double PeakOverlayCross::getOccupancyInView() const
    {
      return m_physicalPeak.getOccupancyInView();
    }

    double PeakOverlayCross::getOccupancyIntoView() const
    {
      return m_physicalPeak.getOccupancyIntoView();
    }

    bool PeakOverlayCross::positionOnly() const
    {
      return true;
    }

  } // namespace Mantid
} // namespace SliceViewer
