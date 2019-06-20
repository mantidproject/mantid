// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Plotting/Mpl/SingleSelector.h"
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

SingleSelector::SingleSelector(PreviewPlot *plot, SelectType type, bool visible,
                               bool infoOnly, const QColor &colour)
    : QObject(), m_plot(plot),
      m_singleMarker(std::make_unique<SingleMarker>(
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
SingleSelector::getAxisRange(const SelectType &type) const {
  switch (type) {
  case SelectType::XSINGLE:
    return m_plot->getAxisRange(AxisID::XBottom);
  case SelectType::YSINGLE:
    return m_plot->getAxisRange(AxisID::YLeft);
  }
  throw std::runtime_error("Incorrect SelectType provided. Select types are "
                           "XSINGLE and YSINGLE.");
}

QString SingleSelector::selectTypeAsQString(const SelectType &type) const {
  switch (type) {
  case SelectType::XSINGLE:
    return "XSingle";
  case SelectType::YSINGLE:
    return "YSingle";
  }
  throw std::runtime_error("Incorrect SelectType provided. Select types are "
                           "XSINGLE and YSINGLE.");
}

void SingleSelector::setRange(const std::pair<double, double> &range) {
  setRange(range.first, range.second);
}

void SingleSelector::setRange(const double min, const double max) {
  m_singleMarker->setRange(min, max);
  m_plot->replot();
  emit selectionChanged(min, max);
}

std::pair<double, double> SingleSelector::getRange() const {
  const auto range = m_singleMarker->getRange();
  return std::make_pair(std::get<0>(range), std::get<1>(range));
}

void SingleSelector::setMinimum(const double min) {
  m_singleMarker->setMinimum(min);
  emit minValueChanged(min);
}

void SingleSelector::setMaximum(const double max) {
  m_singleMarker->setMaximum(max);
  emit maxValueChanged(max);
}

double SingleSelector::getMinimum() const {
  return m_singleMarker->getMinimum();
}

double SingleSelector::getMaximum() const {
  return m_singleMarker->getMaximum();
}

void SingleSelector::setVisible(bool visible) {
  m_visible = visible;
  m_plot->replot();
}

void SingleSelector::detach() {
  m_singleMarker->remove();
  m_plot->canvas()->draw();
}

void SingleSelector::setColour(const QColor &colour) {
  m_singleMarker->setColor(colour.name(QColor::HexRgb));
}

void SingleSelector::handleMouseDown(const QPoint &point) {
  const auto dataCoords = m_plot->toDataCoords(point);
  m_singleMarker->mouseMoveStart(dataCoords.x(), dataCoords.y());
}

void SingleSelector::handleMouseMove(const QPoint &point) {
  const auto dataCoords = m_plot->toDataCoords(point);
  const auto markerMoved =
      m_singleMarker->mouseMove(dataCoords.x(), dataCoords.y());

  if (markerMoved) {
    m_plot->replot();
    const auto range = getRange();
    emit selectionChanged(range.first, range.second);
  }
}

void SingleSelector::handleMouseUp(const QPoint &point) {
  UNUSED_ARG(point);
  m_singleMarker->mouseMoveStop();
}

void SingleSelector::redrawMarker() {
  if (m_visible)
    m_singleMarker->redraw();
}

} // namespace MantidWidgets
} // namespace MantidQt
