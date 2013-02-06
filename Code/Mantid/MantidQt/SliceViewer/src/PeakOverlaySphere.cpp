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
    PeakOverlaySphere::PeakOverlaySphere(QwtPlot * plot, QWidget * parent,
        const Mantid::Kernel::V3D& origin, const double& peakRadius, const double& backgroundInnerRadius,
        const double& backgroundOuterRadius, const QColor& peakColour, const QColor& backColour) :
        QWidget(parent), m_plot(plot), m_physicalPeak(origin, peakRadius, backgroundInnerRadius,
            backgroundOuterRadius), m_peakColour(peakColour), m_backColour(backColour)
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
    {
      return m_plot->canvas()->size();
    }
    int PeakOverlaySphere::height() const
    {
      return m_plot->canvas()->height();
    }
    int PeakOverlaySphere::width() const
    {
      return m_plot->canvas()->width();
    }

    //----------------------------------------------------------------------------------------------
    /// Paint the overlay
    void PeakOverlaySphere::paintEvent(QPaintEvent * /*event*/)
    {
      if (m_physicalPeak.isViewablePeak())
      {
        const QwtDoubleInterval intervalY = m_plot->axisScaleDiv(QwtPlot::yLeft)->interval();
        const QwtDoubleInterval intervalX = m_plot->axisScaleDiv(QwtPlot::xBottom)->interval();

        // Calculate the physical drawing aspects using the Physical Peak.
        auto drawObject = m_physicalPeak.draw(height(), width(), intervalY.width(), intervalX.width());

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

        if (m_physicalPeak.isViewableBackground())
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

    void PeakOverlaySphere::changeForegroundColour(const QColor colour)
    {
      this->m_peakColour = colour;
    }

    void PeakOverlaySphere::changeBackgroundColour(const QColor colour)
    {
      this->m_backColour = colour;
    }

    void PeakOverlaySphere::showBackgroundRadius(const bool show)
    {
      m_physicalPeak.showBackgroundRadius(show);
    }

    /**
     @return bounding box for peak in windows coordinates.
     */
    PeakBoundingBox PeakOverlaySphere::getBoundingBox() const
    {
      return m_physicalPeak.getBoundingBox();
    }

    void PeakOverlaySphere::changeOccupancyInView(const double)
    {
      // DO NOTHING
    }

    void PeakOverlaySphere::changeOccupancyIntoView(const double)
    {
      // DO NOTHING
    }

    double PeakOverlaySphere::getOccupancyInView() const
    {
      throw std::runtime_error("PeakOverlaySphere::getOccupancyInView() not implemented");
    }

    double PeakOverlaySphere::getOccupancyIntoView() const
    {
      throw std::runtime_error("PeakOverlaySphere::getOccupancyIntoView() not implemented");
    }

    bool PeakOverlaySphere::positionOnly() const
    {
      false;
    }

  } // namespace Mantid
} // namespace SliceViewer
