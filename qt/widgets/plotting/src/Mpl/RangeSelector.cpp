// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Plotting/Mpl/RangeSelector.h"
#include "MantidQtWidgets/Plotting/Mpl/PreviewPlot.h"

using namespace MantidQt::Widgets::MplCpp;

namespace {

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
    : QObject(), m_plot(plot),
      m_rangeMarker(std::make_unique<RangeMarker>(
          m_plot->canvas(), colour.name(QColor::HexRgb),
          std::get<0>(getAxisRange(type)), std::get<1>(getAxisRange(type)),
          selectTypeAsQString(type), defaultLineKwargs())),
      m_visible(visible) {
  Q_UNUSED(infoOnly);

  m_plot->canvas()->draw();

  connect(m_plot, SIGNAL(mouseDown(QPoint)), this,
          SLOT(handleMouseDown(QPoint)));
  connect(m_plot, SIGNAL(mouseMove(QPoint)), this,
          SLOT(handleMouseMove(QPoint)));
  connect(m_plot, SIGNAL(mouseUp(QPoint)), this, SLOT(handleMouseUp(QPoint)));

  connect(m_plot, SIGNAL(redraw()), this, SLOT(redrawMarker()));
}

std::tuple<double, double>
RangeSelector::getAxisRange(const SelectType &type) const {
  switch (type) {
  case SelectType::XMINMAX:
    return m_plot->getAxisRange(AxisID::XBottom);
  case SelectType::YMINMAX:
    return m_plot->getAxisRange(AxisID::YLeft);
  }
  throw std::runtime_error(
      "Incorrect SelectType provided. Select types are XMINMAX and YMINMAX.");
}

QString RangeSelector::selectTypeAsQString(const SelectType &type) const {
  switch (type) {
  case SelectType::XMINMAX:
    return "XMinMax";
  case SelectType::YMINMAX:
    return "YMinMax";
  }
  throw std::runtime_error(
      "Incorrect SelectType provided. Select types are XMINMAX and YMINMAX.");
}

void RangeSelector::setLimits(const std::pair<double, double> &limits) {
  setLimits(limits.first, limits.second);
}

void RangeSelector::setLimits(const double min, const double max) {
  // Not used for Mpl as limits are set when creating the range marker.
  Q_UNUSED(min);
  Q_UNUSED(max);
}

std::pair<double, double> RangeSelector::getLimits() const {
  const auto limits = m_plot->getAxisRange();
  return std::make_pair(std::get<0>(limits), std::get<1>(limits));
}

void RangeSelector::setRange(const std::pair<double, double> &range) {
  setRange(range.first, range.second);
}

void RangeSelector::setRange(const double min, const double max) {
  m_rangeMarker->setRange(min, max);
  m_plot->replot();
  emit selectionChanged(min, max);
}

std::pair<double, double> RangeSelector::getRange() const {
  const auto range = m_rangeMarker->getRange();
  return std::make_pair(std::get<0>(range), std::get<1>(range));
}

void RangeSelector::setMinimum(const double min) {
  m_rangeMarker->setMinimum(min);
  emit selectionChanged(min, m_rangeMarker->getMaximum());
}

void RangeSelector::setMaximum(const double max) {
  m_rangeMarker->setMaximum(max);
  emit selectionChanged(m_rangeMarker->getMinimum(), max);
}

double RangeSelector::getMinimum() const { return m_rangeMarker->getMinimum(); }

double RangeSelector::getMaximum() const { return m_rangeMarker->getMaximum(); }

void RangeSelector::setVisible(bool visible) {
  m_visible = visible;
  m_plot->replot();
}

void RangeSelector::detach() {
  m_rangeMarker->remove();
  m_plot->canvas()->draw();
}

void RangeSelector::setColour(const QColor &colour) {
  m_rangeMarker->setColor(colour.name(QColor::HexRgb));
}

void RangeSelector::handleMouseDown(const QPoint &point) {
  m_rangeMarker->mouseMoveStart(point.x(), point.y());
}

void RangeSelector::handleMouseMove(const QPoint &point) {
  const auto markerMoved = m_rangeMarker->mouseMove(point.x(), point.y());

  if (markerMoved) {
    m_plot->replot();
    const auto range = m_rangeMarker->getRange();
    emit selectionChanged(std::get<0>(range), std::get<1>(range));
  }
}

void RangeSelector::handleMouseUp(const QPoint &point) {
  UNUSED_ARG(point);
  m_rangeMarker->mouseMoveStop();
}

void RangeSelector::redrawMarker() {
  if (m_visible)
    m_rangeMarker->redraw();
}

} // namespace MantidWidgets
} // namespace MantidQt
