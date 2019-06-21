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

SingleSelector::SingleSelector(PreviewPlot *plot, SelectType type,
                               double position, bool visible, bool infoOnly,
                               const QColor &colour)
    : QObject(), m_plot(plot),
      m_singleMarker(std::make_unique<SingleMarker>(
          m_plot->canvas(), colour.name(QColor::HexRgb), position,
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

void SingleSelector::setBounds(const std::pair<double, double> &bounds) {
  setBounds(bounds.first, bounds.second);
}

void SingleSelector::setBounds(const double min, const double max) {
  m_singleMarker->setBounds(min, max);
}

void SingleSelector::setLowerBound(const double min) {
  m_singleMarker->setLowerBound(min);
}

void SingleSelector::setUpperBound(const double max) {
  m_singleMarker->setUpperBound(max);
}

void SingleSelector::setPosition(const double position) {
  const auto positionChanged = m_singleMarker->setPosition(position);
  if (positionChanged) {
    m_plot->replot();
    emit valueChanged(position);
  }
}

double SingleSelector::getPosition() const {
  return m_singleMarker->getPosition();
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
  if (m_visible) {
    const auto dataCoords = m_plot->toDataCoords(point);
    m_singleMarker->mouseMoveStart(dataCoords.x(), dataCoords.y());
  }
}

void SingleSelector::handleMouseMove(const QPoint &point) {
  if (m_visible) {
    const auto dataCoords = m_plot->toDataCoords(point);
    const auto markerMoved =
        m_singleMarker->mouseMove(dataCoords.x(), dataCoords.y());

    if (markerMoved) {
      m_plot->replot();
      const auto newPosition = getPosition();
      emit valueChanged(newPosition);
    }
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
