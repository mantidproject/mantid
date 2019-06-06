// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Plotting/Mpl/RangeSelector.h"
#include "MantidQtWidgets/Plotting/Mpl/PreviewPlot.h"

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
      m_minMarker(m_plot->canvas(), colour.name(QColor::HexRgb),
                  getDefaultRange(m_limits).first, defaultLineKwargs()) {
  m_plot->canvas()->draw();
  m_minMarker.redraw();
  Q_UNUSED(visible);
  Q_UNUSED(infoOnly);
}

void RangeSelector::setRange(const std::pair<double, double> &range) {
  setRange(range.first, range.second);
}

void RangeSelector::setRange(const double min, const double max) {
  m_minimum = min;
  m_maximum = max;
}

void RangeSelector::setMinimum(double value) {
  if (value != m_minimum)
    m_minimum = (value > m_limits.first) ? value : m_limits.first;
}

void RangeSelector::setMaximum(double value) {
  if (value != m_maximum)
    m_maximum = (value < m_limits.second) ? value : m_limits.second;
}

double RangeSelector::getMinimum() { return m_minimum; }

double RangeSelector::getMaximum() { return m_maximum; }

void RangeSelector::detach() {
  m_minMarker.remove();
  m_plot->canvas()->draw();
}

void RangeSelector::setColour(QColor colour) {
  m_minMarker.setColor(colour.name(QColor::HexRgb));
  m_minMarker.redraw();
}

void RangeSelector::mouseMoveEvent() {}

} // namespace MantidWidgets
} // namespace MantidQt
