#include "MantidQtSliceViewer/PeakOverlayMultiSphere.h"
#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qwt_scale_div.h>
#include <qpainter.h>
#include <QPen>
#include <QMouseEvent>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace MantidQt
{
  namespace SliceViewer
  {

    //----------------------------------------------------------------------------------------------
    /** Constructor
     */
    PeakOverlayMultiSphere::PeakOverlayMultiSphere(QwtPlot * plot, QWidget * parent, const VecPhysicalSphericalPeak& vecPhysicalPeaks , const QColor& peakColour, const QColor& backColour) :
        QWidget(parent), m_plot(plot), m_physicalPeaks(vecPhysicalPeaks), m_peakColour(peakColour), m_backColour(backColour), m_showBackground(false)
    {
      setAttribute(Qt::WA_NoMousePropagation, false);
      this->setVisible(true);
      setUpdatesEnabled(true);

      setAttribute(Qt::WA_TransparentForMouseEvents);
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
     */
    PeakOverlayMultiSphere::~PeakOverlayMultiSphere()
    {
    }

    void PeakOverlayMultiSphere::setSlicePoint(const double& z, const std::vector<bool>& viewablePeaks)
    {
      m_viewablePeaks = viewablePeaks;
      for(size_t i = 0; i < m_viewablePeaks.size(); ++i)
      {
        if(m_viewablePeaks[i])
        {
          m_physicalPeaks[i]->setSlicePoint(z);
        }
      }
      this->update(); //repaint
    }

    //----------------------------------------------------------------------------------------------
    /// Return the recommended size of the widget
    QSize PeakOverlayMultiSphere::sizeHint() const
    {
      //TODO: Is there a smarter way to find the right size?
      return QSize(20000, 20000);
      // Always as big as the canvas
      //return m_plot->canvas()->size();
    }

    QSize PeakOverlayMultiSphere::size() const
    {
      return m_plot->canvas()->size();
    }
    int PeakOverlayMultiSphere::height() const
    {
      return m_plot->canvas()->height();
    }
    int PeakOverlayMultiSphere::width() const
    {
      return m_plot->canvas()->width();
    }

    //----------------------------------------------------------------------------------------------
    /// Paint the overlay
    void PeakOverlayMultiSphere::paintEvent(QPaintEvent * /*event*/)
    {
      for(size_t i = 0; i < m_viewablePeaks.size(); ++i)
      {
        if(m_viewablePeaks[i])
        {
        const QwtDoubleInterval intervalY = m_plot->axisScaleDiv(QwtPlot::yLeft)->interval();
        const QwtDoubleInterval intervalX = m_plot->axisScaleDiv(QwtPlot::xBottom)->interval();

        // Calculate the physical drawing aspects using the Physical Peak.
        auto drawObject = m_physicalPeaks[i]->draw(height(), width(), intervalY.width(), intervalX.width());

        // Linear Transform from MD coordinates into Windows/Qt coordinates for ellipse rendering. TODO: This can be done outside of paintEvent.
        const int xOrigin = m_plot->transform(QwtPlot::xBottom, drawObject.peakOrigin.X());
        const int yOrigin = m_plot->transform(QwtPlot::yLeft, drawObject.peakOrigin.Y());
        const QPointF originWindows(xOrigin, yOrigin);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setOpacity(drawObject.peakOpacityAtDistance); //Set the pre-calculated opacity

        QPainterPath peakRadiusInnerPath;
        peakRadiusInnerPath.addEllipse(originWindows, drawObject.peakInnerRadiusX,
            drawObject.peakInnerRadiusY);
        QPen pen(m_peakColour);
        pen.setWidth(2);
        pen.setStyle(Qt::DashLine);
        painter.strokePath(peakRadiusInnerPath, pen);

        if (m_physicalPeaks[i]->getShowBackgroundRadius())
        {
          QPainterPath backgroundOuterPath;
          backgroundOuterPath.setFillRule(Qt::WindingFill);
          backgroundOuterPath.addEllipse(originWindows, drawObject.backgroundOuterRadiusX,
              drawObject.backgroundOuterRadiusY);
          QPainterPath backgroundInnerPath;
          backgroundInnerPath.addEllipse(originWindows, drawObject.backgroundInnerRadiusX,
              drawObject.backgroundInnerRadiusY);
          QPainterPath backgroundRadiusFill = backgroundOuterPath.subtracted(backgroundInnerPath);
          painter.fillPath(backgroundRadiusFill, m_backColour);
        }
      }
      }
    }

    void PeakOverlayMultiSphere::updateView()
    {
      this->update();
    }

    void PeakOverlayMultiSphere::hideView()
    {
      this->hide();
    }

    void PeakOverlayMultiSphere::showView()
    {
      this->show();
    }

    void PeakOverlayMultiSphere::movePosition(PeakTransform_sptr transform)
    {
      for(size_t i = 0; i < m_physicalPeaks.size(); ++i)
      { 
        m_physicalPeaks[i]->movePosition(transform);
      }
    }

    void PeakOverlayMultiSphere::changeForegroundColour(const QColor colour)
    {
      this->m_peakColour = colour;
    }

    void PeakOverlayMultiSphere::changeBackgroundColour(const QColor colour)
    {
      this->m_backColour = colour;
    }

    void PeakOverlayMultiSphere::showBackgroundRadius(const bool show)
    {
      for(size_t i = 0; i < m_physicalPeaks.size(); ++i)
      { 
        m_physicalPeaks[i]->showBackgroundRadius(show);
      }
      m_showBackground = show;
    }

    /**
     @param peakIndex: Index of the peak to fetch the bounding box for.
     @return bounding box for peak in windows coordinates.
     */
    PeakBoundingBox PeakOverlayMultiSphere::getBoundingBox(const int peakIndex) const
    {
      return m_physicalPeaks[peakIndex]->getBoundingBox();
    }

    void PeakOverlayMultiSphere::changeOccupancyInView(const double)
    {
      // DO NOTHING
    }

    void PeakOverlayMultiSphere::changeOccupancyIntoView(const double)
    {
      // DO NOTHING
    }

    double PeakOverlayMultiSphere::getOccupancyInView() const
    {
      throw std::runtime_error("PeakOverlaySphere::getOccupancyInView() not implemented");
    }

    double PeakOverlayMultiSphere::getOccupancyIntoView() const
    {
      throw std::runtime_error("PeakOverlaySphere::getOccupancyIntoView() not implemented");
    }

    bool PeakOverlayMultiSphere::positionOnly() const
    {
      return false;
    }

    double PeakOverlayMultiSphere::getRadius() const
    {
      return m_physicalPeaks[0]->getRadius();
    }

    bool PeakOverlayMultiSphere::isBackgroundShown() const
    {
      return m_showBackground;
    }

    QColor PeakOverlayMultiSphere::getBackgroundColour() const
    {
      return m_backColour;
    }

    QColor PeakOverlayMultiSphere::getForegroundColour() const
    {
      return m_peakColour;
    }

  } // namespace Mantid
} // namespace SliceViewer
