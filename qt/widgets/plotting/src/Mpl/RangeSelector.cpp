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
          std::get<0>(m_plot->getAxisRange()),
          std::get<1>(m_plot->getAxisRange()), defaultLineKwargs())) {
  Q_UNUSED(type);
  Q_UNUSED(visible);
  Q_UNUSED(infoOnly);

  m_plot->canvas()->draw();

  connect(m_plot, SIGNAL(mouseDown(QPoint)), this,
          SLOT(handleMouseDown(QPoint)));
  connect(m_plot, SIGNAL(mouseMove(QPoint)), this,
          SLOT(handleMouseMove(QPoint)));
  connect(m_plot, SIGNAL(mouseUp(QPoint)), this, SLOT(handleMouseUp(QPoint)));

  connect(m_plot, SIGNAL(redraw()), this, SLOT(redrawMarker()));
}

void RangeSelector::setRange(const std::pair<double, double> &range) {
  setRange(range.first, range.second);
}

void RangeSelector::setRange(const double min, const double max) {
  m_rangeMarker->setXRange(min, max);
  m_plot->replot();
  emit selectionChanged(min, max);
}

std::pair<double, double> RangeSelector::getRange() const {
  const auto range = m_rangeMarker->getXRange();
  return std::make_pair(std::get<0>(range), std::get<1>(range));
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
    const auto range = m_rangeMarker->getXRange();
    emit selectionChanged(std::get<0>(range), std::get<1>(range));
  }
}

void RangeSelector::handleMouseUp(const QPoint &point) {
  UNUSED_ARG(point);
  m_rangeMarker->mouseMoveStop();
}

void RangeSelector::redrawMarker() { m_rangeMarker->redraw(); }

} // namespace MantidWidgets
} // namespace MantidQt
