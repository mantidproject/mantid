#include "MantidQtSliceViewer/PeakOverlayMulti.h"
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
  PeakOverlayMulti::PeakOverlayMulti(QwtPlot * plot, QWidget * parent, const VecPhysicalCrossPeak&  vecPhysicalPeaks, const QColor& peakColour)
  : QWidget( parent ),
    m_plot(plot),
    m_physicalPeaks(vecPhysicalPeaks),
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
  PeakOverlayMulti::~PeakOverlayMulti()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Set the distance between the plane and the center of the peak in md coordinates

  @param z : position of the plane slice in the z dimension.

  */
  void PeakOverlayMulti::setSlicePoint(const double& z)
  {
    for(int i = 0; i < m_physicalPeaks.size(); ++i)
    {
    m_physicalPeaks[i]->setSlicePoint(z);
    }
    this->update(); //repaint
  }


  //----------------------------------------------------------------------------------------------
  /// Return the recommended size of the widget
  QSize PeakOverlayMulti::sizeHint() const
  {
    //TODO: Is there a smarter way to find the right size?
    return QSize(20000, 20000);
    // Always as big as the canvas
    //return m_plot->canvas()->size();
  }

  QSize PeakOverlayMulti::size() const
  { return m_plot->canvas()->size(); }
  int PeakOverlayMulti::height() const
  { return m_plot->canvas()->height(); }
  int PeakOverlayMulti::width() const
  { return m_plot->canvas()->width(); }

  //----------------------------------------------------------------------------------------------
  /// Paint the overlay
  void PeakOverlayMulti::paintEvent(QPaintEvent * /*event*/)
  {
    

    for(int i = 0; i < m_physicalPeaks.size(); ++i)
    {
     
    if(m_physicalPeaks[i]->isViewable()) // This check will not be necessary when we move to using PeaksInRegion!
    {
      auto drawObject = m_physicalPeaks[i]->draw(height(), width());

      const int xOriginWindows = m_plot->transform( QwtPlot::xBottom, drawObject.peakOrigin.X() );
      const int yOriginWindows = m_plot->transform( QwtPlot::yLeft, drawObject.peakOrigin.Y() );
QPainter painter(this);
    painter.setRenderHint( QPainter::Antialiasing );
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
  }

  void PeakOverlayMulti::updateView()
  {
    this->update();
  }

  void PeakOverlayMulti::hideView()
  {
    this->hide();
  }

  void PeakOverlayMulti::showView()
  {
    this->show();
  }

  void PeakOverlayMulti::movePosition(PeakTransform_sptr transform)
  {
    for(int i = 0; i < m_physicalPeaks.size(); ++i)
    {
      m_physicalPeaks[i]->movePosition(transform);
    }
  }

  void PeakOverlayMulti::changeForegroundColour(const QColor colour)
  {
    this->m_peakColour = QColor(colour);
  }

  void PeakOverlayMulti::changeBackgroundColour(const QColor)
  {
    // Do nothing with the background colour for a peak widget of this type.
  }

  /**
  @return bounding box for peak in windows coordinates.
  */
  PeakBoundingBox PeakOverlayMulti::getBoundingBox() const
  {
    return m_physicalPeaks[0]->getBoundingBox(); // Hack
  }

  /**
     * Set the occupancy into the view as a fraction of the current view width.
     * @param fraction to use.
     */
    void PeakOverlayMulti::changeOccupancyInView(const double fraction)
    {
      for(int i = 0; i < m_physicalPeaks.size(); ++i)
    {
     
      m_physicalPeaks[i]->setOccupancyInView(fraction);
      }
    }

    /**
     * Set the occupancy into the view as a fraction of the current view depth.
     * @param fraction to use.
     */
    void PeakOverlayMulti::changeOccupancyIntoView(const double fraction)
    {
      for(int i = 0; i < m_physicalPeaks.size(); ++i)
    {
     
      m_physicalPeaks[i]->setOccupancyIntoView(fraction);
      }
    }

    double PeakOverlayMulti::getOccupancyInView() const
    {
      for(int i = 0; i < m_physicalPeaks.size(); ++i)
    {
     
      return m_physicalPeaks[i]->getOccupancyInView();
      }
    }

    double PeakOverlayMulti::getOccupancyIntoView() const
    {
      for(int i = 0; i < m_physicalPeaks.size(); ++i)
    {
     
      return m_physicalPeaks[i]->getOccupancyIntoView();
      }
    }

    bool PeakOverlayMulti::positionOnly() const
    {
      for(int i = 0; i < m_physicalPeaks.size(); ++i)
    {
     
      return true;
      }
    }

  } // namespace Mantid
} // namespace SliceViewer
