// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
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
      m_type(type), m_visible(visible), m_markerMoving(false) {
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
  throw std::runtime_error("Incorrect SelectType provided. Select types are "
                           "XMINMAX and YMINMAX.");
}

QString RangeSelector::selectTypeAsQString(const SelectType &type) const {
  switch (type) {
  case SelectType::XMINMAX:
    return "XMinMax";
  case SelectType::YMINMAX:
    return "YMinMax";
  }
  throw std::runtime_error("Incorrect SelectType provided. Select types are "
                           "XMINMAX and YMINMAX.");
}

void RangeSelector::setRange(const std::pair<double, double> &range) {
  setRange(range.first, range.second);
}

void RangeSelector::setRange(double min, double max) {
  m_rangeMarker->setRange(min, max);
  emit selectionChanged(min, max);
  emit minValueChanged(min);
  emit maxValueChanged(max);
  m_plot->replot();
}

std::pair<double, double> RangeSelector::getRange() const {
  const auto range = m_rangeMarker->getRange();
  return std::make_pair(std::get<0>(range), std::get<1>(range));
}

void RangeSelector::setMinimum(double min) {
  m_rangeMarker->setMinimum(min);
  emit minValueChanged(min);
  m_plot->replot();
}

void RangeSelector::setMaximum(double max) {
  m_rangeMarker->setMaximum(max);
  emit maxValueChanged(max);
  m_plot->replot();
}

double RangeSelector::getMinimum() const { return m_rangeMarker->getMinimum(); }

double RangeSelector::getMaximum() const { return m_rangeMarker->getMaximum(); }

void RangeSelector::setVisible(bool visible) {
  m_visible = visible;
  m_plot->replot();
}

void RangeSelector::setBounds(const double min, const double max) {
  m_rangeMarker->setBounds(min, max);
}

void RangeSelector::detach() {
  m_rangeMarker->remove();
  m_plot->canvas()->draw();
}

void RangeSelector::setColour(const QColor &colour) {
  m_rangeMarker->setColor(colour.name(QColor::HexRgb));
}

void RangeSelector::handleMouseDown(const QPoint &point) {
  if (m_visible && !m_plot->selectorActive()) {
    const auto dataCoords = m_plot->toDataCoords(point);
    m_rangeMarker->mouseMoveStart(dataCoords.x(), dataCoords.y());

    m_markerMoving = m_rangeMarker->isMoving();
    if (m_markerMoving)
      m_plot->setSelectorActive(true);
  }
}

void RangeSelector::handleMouseMove(const QPoint &point) {
  if (m_visible && m_markerMoving) {
    const auto dataCoords = m_plot->toDataCoords(point);
    const auto markerMoved =
        m_rangeMarker->mouseMove(dataCoords.x(), dataCoords.y());

    if (markerMoved) {
      const auto range = getRange();
      emit selectionChanged(range.first, range.second);
      emit minValueChanged(range.first);
      emit maxValueChanged(range.second);
      m_plot->replot();
    }
  }
}

void RangeSelector::handleMouseUp(const QPoint &point) {
  UNUSED_ARG(point);
  if (m_markerMoving) {
    m_rangeMarker->mouseMoveStop();
    m_plot->setSelectorActive(false);
    m_markerMoving = false;
  }
}

void RangeSelector::redrawMarker() {
  if (m_visible)
    m_rangeMarker->redraw();
}

} // namespace MantidWidgets
} // namespace MantidQt
