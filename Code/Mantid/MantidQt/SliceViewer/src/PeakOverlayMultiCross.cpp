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
      : QWidget( parent ),
      m_presenter(presenter),
      m_plot(plot),
      m_physicalPeaks(vecPhysicalPeaks),
      m_plotXIndex(plotXIndex),
      m_plotYIndex(plotYIndex),
      m_peakColour(peakColour),
      m_tool(NULL), m_defaultCursor(plot->cursor())
    {
      setAttribute(Qt::WA_NoMousePropagation, false);
      setAttribute(Qt::WA_MouseTracking, true);

      this->setVisible(true);
      setUpdatesEnabled(true);

      //setAttribute(Qt::WA_TransparentForMouseEvents);
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
    */
    PeakOverlayMultiCross::~PeakOverlayMultiCross()
    {
        this->peakDisplayMode();
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
          painter.end();
        }
      }
      if(m_tool){
          QPainter painter(this);
          m_tool->onPaint(painter);
          painter.end();
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

    void PeakOverlayMultiCross::peakDeletionMode() {
        QApplication::restoreOverrideCursor();
        auto* temp = m_tool;
        auto* eraseTool = new MantidQt::MantidWidgets::InputControllerErase(this);
        connect(eraseTool,SIGNAL(erase(QRect)),this,SLOT(erasePeaks(QRect)), Qt::QueuedConnection);
        m_tool = eraseTool;
        delete temp;
    }

    void PeakOverlayMultiCross::peakAdditionMode() {
        QApplication::restoreOverrideCursor();
        auto* temp = m_tool;
        auto* addTool = new MantidQt::MantidWidgets::InputControllerPick(this);
        connect(addTool,SIGNAL(pickPointAt(int,int)),this,SLOT(addPeakAt(int,int)));
        m_tool = addTool;
        delete temp;
    }

    void PeakOverlayMultiCross::peakDisplayMode() {
        QApplication::restoreOverrideCursor();
        if(m_tool){
            delete m_tool;
            m_tool = NULL;
            m_plot->setCursor(m_defaultCursor);
        }
    }

    void PeakOverlayMultiCross::mousePressEvent(QMouseEvent* e)
    {
        if(m_tool) {
          m_tool->mousePressEvent( e );
        }else{
            e->ignore();
        }
    }

    void PeakOverlayMultiCross::mouseMoveEvent(QMouseEvent* e)
    {
        if(m_tool) {
          m_tool->mouseMoveEvent( e );
          this->update();
        }
        e->ignore();

    }

    void PeakOverlayMultiCross::mouseReleaseEvent(QMouseEvent* e)
    {
        if(m_tool) {
          m_tool->mouseReleaseEvent( e );
        }else{
            e->ignore();
        }
    }

    void PeakOverlayMultiCross::wheelEvent(QWheelEvent* e)
    {
        if(m_tool) {
          m_tool->wheelEvent( e );
        }else{
            e->ignore();
        }
    }

    void PeakOverlayMultiCross::keyPressEvent(QKeyEvent* e)
    {
        if(m_tool) {
          m_tool->keyPressEvent( e );
        }else{
            e->ignore();
        }
    }

    void PeakOverlayMultiCross::enterEvent(QEvent *e)
    {
        if(m_tool) {
          m_tool->enterEvent( e );
        }else{
            e->ignore();
        }
    }

    void PeakOverlayMultiCross::leaveEvent(QEvent *e)
    {
        if(m_tool) {
          m_tool->leaveEvent( e );
        }else{
            e->ignore();
        }
    }

    void PeakOverlayMultiCross::addPeakAt(int coordX, int coordY) {

        QwtScaleMap xMap = m_plot->canvasMap(m_plotXIndex);
        QwtScaleMap yMap = m_plot->canvasMap(m_plotYIndex);

        const double plotX = xMap.invTransform(double(coordX));
        const double plotY = yMap.invTransform(double(coordY));

        m_presenter->addPeakAt(plotX, plotY);
    }


    void PeakOverlayMultiCross::erasePeaks(const QRect &rect)
    {
        QwtScaleMap xMap = m_plot->canvasMap(m_plotXIndex);
        QwtScaleMap yMap = m_plot->canvasMap(m_plotYIndex);

        const Left left(xMap.invTransform(rect.left()));
        const Right right(xMap.invTransform(rect.right()));
        const Top top(yMap.invTransform(rect.top()));
        const Bottom bottom(yMap.invTransform(rect.bottom()));
        const SlicePoint slicePoint(-1); // Not required.

        m_presenter->deletePeaksIn(PeakBoundingBox(left, right, top, bottom, slicePoint));

    }

  } // namespace Mantid
} // namespace SliceViewer
