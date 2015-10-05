#include "MantidQtSliceViewer/PeakOverlayMultiCross.h"
#include "MantidQtSliceViewer/PeaksPresenter.h"
#include "MantidQtMantidWidgets/InputController.h"
#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qwt_scale_div.h>
#include <qpainter.h>
#include <QPen>
#include <QMouseEvent>
#include <QApplication>


using namespace Mantid::Kernel;
using namespace Mantid::Geometry;


namespace MantidQt
{
  namespace SliceViewer
  {

    //----------------------------------------------------------------------------------------------
    /** Constructor
    */
    PeakOverlayMultiCross::PeakOverlayMultiCross(PeaksPresenter* const presenter, QwtPlot * plot, QWidget * parent,
                                                 const VecPhysicalCrossPeak&  vecPhysicalPeaks, const int plotXIndex, const int plotYIndex, const QColor& peakColour)
      : PeakOverlayInteractive ( presenter, plot, plotXIndex, plotYIndex, parent ),
      m_physicalPeaks(vecPhysicalPeaks),
      m_peakColour(peakColour),
      m_cachedOccupancyIntoView(0),
      m_cachedOccupancyInView(0)
    {
        if(vecPhysicalPeaks.size() > 0)
        {
            // Cache the occupancy if we can, that way if all physical peaks are removed, we still keep the occupancy settings.
            VecPhysicalCrossPeak::value_type firstPhysicalPeak = vecPhysicalPeaks.front();
            m_cachedOccupancyIntoView = firstPhysicalPeak->getOccupancyIntoView();
            m_cachedOccupancyInView = firstPhysicalPeak->getOccupancyInView();
        }
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
    /// Paint the overlay
    void PeakOverlayMultiCross::doPaintPeaks(QPaintEvent * /*event*/)
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
          painter.end();
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
      m_cachedOccupancyInView = fraction;
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
      m_cachedOccupancyIntoView = fraction;
    }

    double PeakOverlayMultiCross::getOccupancyInView() const
    {
      return m_cachedOccupancyInView;
    }

    double PeakOverlayMultiCross::getOccupancyIntoView() const
    {
      return m_cachedOccupancyIntoView;
    }

    bool PeakOverlayMultiCross::positionOnly() const
    {
      return true;
    }

    double PeakOverlayMultiCross::getRadius() const
    {
      return m_physicalPeaks[0]->getEffectiveRadius();
    }

    bool PeakOverlayMultiCross::isBackgroundShown() const
    {
      return false; // The background is not displayed for this view type.
    }

    QColor PeakOverlayMultiCross::getBackgroundColour() const
    {
      return m_peakColour; // Doesn't really do anything since there is no background for a cross marker.
    }

    QColor PeakOverlayMultiCross::getForegroundColour() const
    {
      return m_peakColour;
    }

    void PeakOverlayMultiCross::takeSettingsFrom(const PeakOverlayView * const source)
    {
        this->changeForegroundColour(source->getForegroundColour());
        this->changeBackgroundColour(source->getBackgroundColour());
        this->changeOccupancyIntoView(source->getOccupancyIntoView());
        this->changeOccupancyInView(source->getOccupancyInView());
        this->showBackgroundRadius(source->isBackgroundShown());
    }

  } // namespace Mantid
} // namespace SliceViewer
