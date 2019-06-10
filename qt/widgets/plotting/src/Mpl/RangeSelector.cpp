// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Plotting/Mpl/RangeSelector.h"
#include "MantidQtWidgets/Plotting/Mpl/PreviewPlot.h"

#include <QApplication>
#include <QCursor>

using namespace MantidQt::Widgets::MplCpp;

namespace {

std::pair<double, double> toPair(const std::tuple<double, double> &range) {
  return std::make_pair(std::get<0>(range), std::get<1>(range));
}

std::pair<double, double>
getDefaultRange(const std::pair<double, double> &limits) {
  const auto minimum = limits.first;
  const auto maximum = limits.second;
  const auto offset = (maximum - minimum) * 0.2;
  return std::make_pair(minimum + offset, maximum - offset);
}

QHash<QString, QVariant> defaultLineKwargs() {
  QHash<QString, QVariant> kwargs;
  kwargs.insert("line_style", QString("--"));
  return kwargs;
}

} // namespace

namespace MantidQt {
namespace MantidWidgets {

RangeSelector::RangeSelector(PreviewPlot *plot, SelectType type, bool visible,
                             bool infoOnly, const QColor &colour)
    : QObject(), m_plot(plot), m_type(type),
      m_limits(toPair(m_plot->getAxisRange())),
      m_minimum(getDefaultRange(m_limits).first),
      m_maximum(getDefaultRange(m_limits).second),
      m_minMarker(std::make_unique<VerticalMarker>(
          m_plot->canvas(), colour.name(QColor::HexRgb), m_minimum,
          defaultLineKwargs())),
      m_maxMarker(std::make_unique<VerticalMarker>(
          m_plot->canvas(), colour.name(QColor::HexRgb), m_maximum,
          defaultLineKwargs())) {
  Q_UNUSED(visible);
  Q_UNUSED(infoOnly);

  m_plot->canvas()->draw();

  connect(m_plot, SIGNAL(mouseDown(QPoint)), this,
          SLOT(handleMouseDown(QPoint)));
  connect(m_plot, SIGNAL(mouseMove(QPoint)), this,
          SLOT(handleMouseMove(QPoint)));
  connect(m_plot, SIGNAL(mouseUp(QPoint)), this, SLOT(handleMouseUp(QPoint)));

  connect(m_plot, SIGNAL(redraw()), this, SLOT(redrawMarkers()));
}

void RangeSelector::setRange(const std::pair<double, double> &range) {
  setRange(range.first, range.second);
}

void RangeSelector::setRange(const double min, const double max) {
  setMinimum(min);
  setMaximum(max);
}

void RangeSelector::setMinimum(double value) {
  if (value != m_minimum) {
    m_minimum = (value > m_limits.first) ? value : m_limits.first;
    m_minMarker->setXPosition(m_minimum);
    m_plot->replot();
    emit selectionChanged(m_minimum, m_maximum);
  }
}

void RangeSelector::setMaximum(double value) {
  if (value != m_maximum) {
    m_maximum = (value < m_limits.second) ? value : m_limits.second;
    m_maxMarker->setXPosition(m_maximum);
    m_plot->replot();
    emit selectionChanged(m_minimum, m_maximum);
  }
}

double RangeSelector::getMinimum() { return m_minimum; }

double RangeSelector::getMaximum() { return m_maximum; }

void RangeSelector::detach() {
  m_minMarker->remove();
  m_maxMarker->remove();
  m_plot->canvas()->draw();
}

void RangeSelector::setColour(QColor colour) {
  m_minMarker->setColor(colour.name(QColor::HexRgb));
  m_maxMarker->setColor(colour.name(QColor::HexRgb));
  redrawMarkers();
}

void RangeSelector::handleMouseDown(const QPoint &point) {
  const auto coords =
      m_minMarker->transformPixelsToCoords(point.x(), point.y());
  const auto xCoord = std::get<0>(coords);
  const auto yCoord = std::get<1>(coords);

  m_minMarker->mouseMoveStart(xCoord, yCoord);
  m_maxMarker->mouseMoveStart(xCoord, yCoord);
  updateCursor();
}

void RangeSelector::handleMouseMove(const QPoint &point) {
  const auto coords =
      m_minMarker->transformPixelsToCoords(point.x(), point.y());
  const auto xCoord = std::get<0>(coords);

  const auto minMoved = m_minMarker->mouseMove(xCoord);
  const auto maxMoved = m_maxMarker->mouseMove(xCoord);

  if (minMoved || maxMoved) {
    m_plot->replot();
    updateMinMax(xCoord, minMoved, maxMoved);
  }
}

void RangeSelector::updateMinMax(const double x, bool minMoved, bool maxMoved) {
  if (minMoved && x != m_minimum) {
    m_minimum = x;
    emit selectionChanged(m_minimum, m_maximum);
  } else if (maxMoved && x != m_maximum) {
    m_maximum = x;
    emit selectionChanged(m_minimum, m_maximum);
  }
}

void RangeSelector::handleMouseUp(const QPoint &point) {
  UNUSED_ARG(point);
  m_minMarker->mouseMoveStop();
  m_maxMarker->mouseMoveStop();
  updateCursor();
}

void RangeSelector::updateCursor() {
  if ((m_minMarker->isMoving() || m_maxMarker->isMoving()) &&
      !QApplication::overrideCursor()) {
    QApplication::setOverrideCursor(Qt::SizeHorCursor);
  } else {
    QApplication::restoreOverrideCursor();
  }
}

void RangeSelector::redrawMarkers() {
  m_minMarker->redraw();
  m_maxMarker->redraw();
}

} // namespace MantidWidgets
} // namespace MantidQt
