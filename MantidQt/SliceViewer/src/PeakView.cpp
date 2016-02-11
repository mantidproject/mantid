#include "MantidQtSliceViewer/PeakView.h"

#include <QPainter>
#include <qwt_plot.h>
#include <qwt_scale_div.h>
#include <qwt_double_interval.h>

namespace MantidQt
{
namespace SliceViewer
{

PeakView::PeakView(PeaksPresenter *const presenter, QwtPlot *plot,
                   QWidget *parent,
                   const VecPeakRepresentation &vecPeakRepresentation,
                   const int plotXIndex, const int plotYIndex, PeakViewColor foregroundColor, PeakViewColor backgroundColor)
    : PeakOverlayInteractive(presenter, plot, plotXIndex, plotYIndex, parent),
      m_peaks(vecPeakRepresentation),
      m_cachedOccupancyIntoView(0), m_cachedOccupancyInView(0),
      m_showBackground(false), m_foregroundColor(foregroundColor), m_backgroundColor(backgroundColor)
{
}

PeakView::~PeakView() {}

void PeakView::doPaintPeaks(QPaintEvent *)
{
    const auto windowHeight = height();
    const auto windowWidth = width();
    const auto viewHeight = m_plot->axisScaleDiv(QwtPlot::yLeft)->interval().width();
    const auto viewWidth = m_plot->axisScaleDiv(QwtPlot::xBottom)->interval().width();

    for (size_t i = 0; i < m_viewablePeaks.size(); ++i) {
        if (m_viewablePeaks[i]) {
            // Get the peak
            auto& peak = m_peaks[i];
            const auto& origin = peak->getOrigin();

            // Set up the view information
            const auto xOriginWindow = m_plot->transform(QwtPlot::xBottom, origin.X());
            const auto yOriginWindow = m_plot->transform(QwtPlot::yLeft, origin.Y());

            PeakRepresentationViewInformation peakRepresentationViewInformation;
            peakRepresentationViewInformation.windowHeight = windowHeight;
            peakRepresentationViewInformation.windowWidth = windowWidth;
            peakRepresentationViewInformation.viewHeight = viewHeight;
            peakRepresentationViewInformation.viewWidth = viewWidth;
            peakRepresentationViewInformation.xOriginWindow = xOriginWindow;
            peakRepresentationViewInformation.yOriginWindow = yOriginWindow;

            QPainter painter(this);
            peak->draw(painter, m_foregroundColor, m_backgroundColor, peakRepresentationViewInformation);
        }
    }
}

/** Set the distance between the plane and the center of the peak in md
coordinates
@param point : position of the plane slice in the z dimension.
@param viewablePeaks: collection of flags indicating the index of the peaks
which are viewable.
*/
void PeakView::setSlicePoint(const double &point,
                             const std::vector<bool> &viewablePeaks)
{
    m_viewablePeaks = viewablePeaks;
    for (size_t i = 0; i < m_viewablePeaks.size(); ++i) {
        if (m_viewablePeaks[i]) // is peak at this index visible.
        {
            m_peaks[i]->setSlicePoint(point);
        }
    }
    this->update();
}

void PeakView::hideView() { this->hide(); }

void PeakView::showView() { this->show(); }

void PeakView::updateView() { this->update(); }

void PeakView::movePosition(Mantid::Geometry::PeakTransform_sptr peakTransform)
{
    for (auto &peak : m_peaks) {
        peak->movePosition(peakTransform);
    }
}

void PeakView::showBackgroundRadius(const bool )
{
#if 0
  THIS IS A SPHERE ISSUE
  for(const auto& peak : m_peaks) {
    peak->showBackgroundRadius(show);
  }
  m_showBackground = show;
#endif
}

PeakBoundingBox PeakView::getBoundingBox(const int peakIndex) const
{
    return m_peaks[peakIndex]->getBoundingBox();
}

void PeakView::changeOccupancyInView(const double fraction)
{
  // TODO check if this makes sense

    for (const auto &peak : m_peaks) {
        peak->setOccupancyInView(fraction);
    }
    m_cachedOccupancyInView = fraction;
}

void PeakView::changeOccupancyIntoView(const double fraction)
{
  // TODO check if this makes sense
    for (const auto &peak : m_peaks) {
        peak->setOccupancyIntoView(fraction);
    }
    m_cachedOccupancyIntoView = fraction;
}

double PeakView::getOccupancyInView() const
{
    // TODO how to set this for differnt peak types
  return 0.0;
}

double PeakView::getOccupancyIntoView() const
{
    // TODO how to set this for differnt peak types
  return 0.0;
}

bool PeakView::positionOnly() const
{
    // TODO how to set this for differnt peak types, probably always false from
    // now on
  return false;
}

double PeakView::getRadius() const
{
    // TODO how to set this for differnt peak types
  return 0.0;
}

bool PeakView::isBackgroundShown() const
{
    // TODO how to set this for differnt peak types, probably always false from
    // now on
    return true;
}

/// Get the current background colour
QColor PeakView::getBackgroundColour() const
{
    // TODO how to set this for differnt peak types, probably always false from
    // now on
  return QColor();
}

/// Get the current foreground colour
QColor PeakView::getForegroundColour() const
{
    /// TODO Have a general color object with a map for the differnt
    /// representations
    return QColor();
}

void PeakView::changeForegroundColour(const QColor)
{
    // TODO how to change color for differnt representations:
    // this->m_peakColour = QColor(colour);
}

/// Change background colour
void PeakView::changeBackgroundColour(const QColor)
{
    // TODO how to change color for differnt representations:
  // Introduce PeakColorView struct
}

void PeakView::takeSettingsFrom(const PeakOverlayView *const)
{
    // TODO how to set this for differnt peak types, probably always false from
    // now on
}
}
}
