#include "MantidQtSliceViewer/PeakOverlayMultiCross.h"
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
    PeakOverlayMultiCross::PeakOverlayMultiCross(QwtPlot * plot, QWidget * parent, const VecPhysicalCrossPeak&  vecPhysicalPeaks, const QColor& peakColour)
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
    PeakOverlayMultiCross::~PeakOverlayMultiCross()
    {
    }

    //----------------------------------------------------------------------------------------------
    /** Set the distance between the plane and the center of the peak in md coordinates

    @param z : position of the plane slice in the z dimension.
    @param viewablePeaks: collection of flags indicating the index of the peaks which are viewable.

    */
    void PeakOverlayMultiCross::setSlicePoint(const double& z, const std::vector<bool>& viewablePeaks)
    {
      m_viewablePeaks = viewablePeaks;
      for(size_t i = 0; i < m_viewablePeaks.size(); ++i)
      {
        if(m_viewablePeaks[i]) // is peak at this index visible.
        {
          m_physicalPeaks[i]->setSlicePoint(z);
        }
      }
      this->update(); //repaint
    }


    //----------------------------------------------------------------------------------------------
    /// Return the recommended size of the widget
    QSize PeakOverlayMultiCross::sizeHint() const
    {
      //TODO: Is there a smarter way to find the right size?
      return QSize(20000, 20000);
      // Always as big as the canvas
      //return m_plot->canvas()->size();
    }

    QSize PeakOverlayMultiCross::size() const
    { return m_plot->canvas()->size(); }
    int PeakOverlayMultiCross::height() const
    { return m_plot->canvas()->height(); }
    int PeakOverlayMultiCross::width() const
    { return m_plot->canvas()->width(); }

    //----------------------------------------------------------------------------------------------
    /// Paint the overlay
    void PeakOverlayMultiCross::paintEvent(QPaintEvent * /*event*/)
    {
      for(size_t i = 0; i < m_viewablePeaks.size(); ++i)
      {
        if(m_viewablePeaks[i]) // Only draw those peaks that are viewable.
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

    void PeakOverlayMultiCross::updateView()
    {
      this->update();
    }

    void PeakOverlayMultiCross::hideView()
    {
      this->hide();
    }

    void PeakOverlayMultiCross::showView()
    {
      this->show();
    }

    void PeakOverlayMultiCross::movePosition(PeakTransform_sptr transform)
    {
      for(size_t i = 0; i < m_physicalPeaks.size(); ++i)
      {
        m_physicalPeaks[i]->movePosition(transform);
      }
    }

    void PeakOverlayMultiCross::changeForegroundColour(const QColor colour)
    {
      this->m_peakColour = QColor(colour);
    }

    void PeakOverlayMultiCross::changeBackgroundColour(const QColor)
    {
      // Do nothing with the background colour for a peak widget of this type.
    }

    /**
    @param peakIndex: Index of the peak to fetch the bounding box for.
    @return bounding box for peak in windows coordinates.
    */
    PeakBoundingBox PeakOverlayMultiCross::getBoundingBox(const int peakIndex) const
    {
      return m_physicalPeaks[peakIndex]->getBoundingBox(); 
    }

    /**
    * Set the occupancy into the view as a fraction of the current view width.
    * @param fraction to use.
    */
    void PeakOverlayMultiCross::changeOccupancyInView(const double fraction)
    {
      for(size_t i = 0; i < m_physicalPeaks.size(); ++i)
      {
        m_physicalPeaks[i]->setOccupancyInView(fraction);
      }
    }

    /**
    * Set the occupancy into the view as a fraction of the current view depth.
    * @param fraction to use.
    */
    void PeakOverlayMultiCross::changeOccupancyIntoView(const double fraction)
    {
      for(size_t i = 0; i < m_physicalPeaks.size(); ++i)
      {
        m_physicalPeaks[i]->setOccupancyIntoView(fraction);
      }
    }

    double PeakOverlayMultiCross::getOccupancyInView() const
    {
      return m_physicalPeaks[0]->getOccupancyInView();
    }

    double PeakOverlayMultiCross::getOccupancyIntoView() const
    {
      return m_physicalPeaks[0]->getOccupancyIntoView();
    }

    bool PeakOverlayMultiCross::positionOnly() const
    {
      return true;
    }

    double PeakOverlayMultiCross::getRadius() const
    {
      return m_physicalPeaks[0]->getEffectiveRadius();
    }

  } // namespace Mantid
} // namespace SliceViewer
