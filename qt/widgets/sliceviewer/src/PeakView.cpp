// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/SliceViewer/PeakView.h"
#include "MantidQtWidgets/SliceViewer/SliceViewer.h"

#include <QPainter>
#include <qwt_double_interval.h>
#include <qwt_plot.h>
#include <qwt_scale_div.h>

namespace MantidQt {
namespace SliceViewer {

PeakView::PeakView(PeaksPresenter *const presenter, QwtPlot *plot,
                   QWidget *parent,
                   const VecPeakRepresentation &vecPeakRepresentation,
                   const int plotXIndex, const int plotYIndex,
                   PeakViewColor foregroundColor, PeakViewColor backgroundColor,
                   double largestEffectiveRadius)
    : PeakOverlayInteractive(presenter, plot, plotXIndex, plotYIndex, parent),
      m_peaks(vecPeakRepresentation), m_cachedOccupancyIntoView(0.015),
      m_cachedOccupancyInView(0.015), m_showBackground(false),
      m_foregroundColor(foregroundColor), m_backgroundColor(backgroundColor),
      m_largestEffectiveRadius(largestEffectiveRadius) {}

PeakView::~PeakView() {}

void PeakView::doPaintPeaks(QPaintEvent * /*event*/) {
  const auto windowHeight = height();
  const auto windowWidth = width();
  const auto viewHeight =
      m_plot->axisScaleDiv(QwtPlot::yLeft)->interval().width();
  const auto viewWidth =
      m_plot->axisScaleDiv(QwtPlot::xBottom)->interval().width();

  for (size_t i = 0; i < m_viewablePeaks.size(); ++i) {
    if (m_viewablePeaks[i]) {
      QPainter painter(this);
      // Get the peak
      auto &peak = m_peaks[i];
      const auto &origin = peak->getOrigin();

      // Set up the view information
      const auto xOriginWindow =
          m_plot->transform(QwtPlot::xBottom, origin.X());
      const auto yOriginWindow = m_plot->transform(QwtPlot::yLeft, origin.Y());

      PeakRepresentationViewInformation peakRepresentationViewInformation;
      peakRepresentationViewInformation.windowHeight = windowHeight;
      peakRepresentationViewInformation.windowWidth = windowWidth;
      peakRepresentationViewInformation.viewHeight = viewHeight;
      peakRepresentationViewInformation.viewWidth = viewWidth;
      peakRepresentationViewInformation.xOriginWindow = xOriginWindow;
      peakRepresentationViewInformation.yOriginWindow = yOriginWindow;

      peak->draw(painter, m_foregroundColor, m_backgroundColor,
                 peakRepresentationViewInformation);
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
                             const std::vector<bool> &viewablePeaks) {
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

void PeakView::movePosition(
    Mantid::Geometry::PeakTransform_sptr peakTransform) {
  for (auto &peak : m_peaks) {
    peak->movePosition(peakTransform);
  }
}

void PeakView::movePositionNonOrthogonal(
    Mantid::Geometry::PeakTransform_sptr peakTransform,
    NonOrthogonalAxis &info) {
  for (auto &peak : m_peaks) {
    peak->movePositionNonOrthogonal(peakTransform, info);
  }
}

void PeakView::showBackgroundRadius(const bool show) {
  for (const auto &peak : m_peaks) {
    peak->showBackgroundRadius(show);
  }
  m_showBackground = show;
}

PeakBoundingBox PeakView::getBoundingBox(const int peakIndex) const {
  return m_peaks[peakIndex]->getBoundingBox();
}

void PeakView::changeOccupancyInView(const double fraction) {
  for (const auto &peak : m_peaks) {
    peak->setOccupancyInView(fraction);
  }
  m_cachedOccupancyInView = fraction;
}

void PeakView::changeOccupancyIntoView(const double fraction) {
  for (const auto &peak : m_peaks) {
    peak->setOccupancyIntoView(fraction);
  }
  m_cachedOccupancyIntoView = fraction;
}

double PeakView::getOccupancyInView() const { return m_cachedOccupancyInView; }

double PeakView::getOccupancyIntoView() const {
  return m_cachedOccupancyIntoView;
}

bool PeakView::positionOnly() const { return false; }

double PeakView::getRadius() const { return m_largestEffectiveRadius; }

bool PeakView::isBackgroundShown() const { return m_showBackground; }

void PeakView::takeSettingsFrom(const PeakOverlayView *const source) {
  // Pass on the color settings
  this->changeForegroundColour(source->getForegroundPeakViewColor());
  this->changeBackgroundColour(source->getBackgroundPeakViewColor());

  // Pass on the information regarding the background color - not relvant for
  // cross-type peak
  this->showBackgroundRadius(source->isBackgroundShown());

  // Pass on the information which only concerns the cross-type peak.
  this->changeOccupancyIntoView(source->getOccupancyIntoView());
  this->changeOccupancyInView(source->getOccupancyInView());
}

void PeakView::changeForegroundColour(const PeakViewColor peakViewColor) {
  m_foregroundColor = peakViewColor;
}

void PeakView::changeBackgroundColour(const PeakViewColor peakViewColor) {
  m_backgroundColor = peakViewColor;
}

PeakViewColor PeakView::getBackgroundPeakViewColor() const {
  return m_backgroundColor;
}

PeakViewColor PeakView::getForegroundPeakViewColor() const {
  return m_foregroundColor;
}
} // namespace SliceViewer
} // namespace MantidQt
