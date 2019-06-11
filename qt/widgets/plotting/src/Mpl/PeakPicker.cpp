// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Plotting/Mpl/PeakPicker.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidQtWidgets/Plotting/Mpl/PreviewPlot.h"

using namespace Mantid::API;
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

PeakPicker::PeakPicker(PreviewPlot *plot, const QColor &colour)
    : QObject(), m_plot(plot), m_peak(nullptr),
      m_peakMarker(std::make_unique<PeakMarker>(
          m_plot->canvas(), 1, std::get<0>(m_plot->getAxisRange()),
          std::get<1>(m_plot->getAxisRange(AxisID::YLeft)), 0.0, 0.0,
          std::get<1>(m_plot->getAxisRange(AxisID::YLeft)))) {
  UNUSED_ARG(colour);

  m_plot->canvas()->draw();

  connect(m_plot, SIGNAL(mouseDown(QPoint)), this,
          SLOT(handleMouseDown(QPoint)));
  connect(m_plot, SIGNAL(mouseMove(QPoint)), this,
          SLOT(handleMouseMove(QPoint)));
  connect(m_plot, SIGNAL(mouseUp(QPoint)), this, SLOT(handleMouseUp(QPoint)));

  connect(m_plot, SIGNAL(redraw()), this, SLOT(redrawMarker()));
}

void PeakPicker::setPeak(const Mantid::API::IPeakFunction_const_sptr &peak) {
  if (peak) {
    m_peak = boost::dynamic_pointer_cast<Mantid::API::IPeakFunction>(
        Mantid::API::FunctionFactory::Instance().createInitialized(
            peak->asString()));
    m_peakMarker->updatePeak(peak->centre(), peak->height(), peak->fwhm());
  }
}

IPeakFunction_sptr PeakPicker::peak() const { return m_peak; }

void PeakPicker::handleMouseDown(const QPoint &point) {
  m_peakMarker->mouseMoveStart(point.x(), point.y());
  if (m_peakMarker->isMoving())
    m_peakMarker->select();
}

void PeakPicker::handleMouseMove(const QPoint &point) {
  const auto markerMoved = m_peakMarker->mouseMove(point.x(), point.y());

  if (markerMoved) {
    m_plot->replot();
    // const auto range = m_peakMarker->getXRange();
    // emit selectionChanged(std::get<0>(range), std::get<1>(range));
  }
}

void PeakPicker::handleMouseUp(const QPoint &point) {
  UNUSED_ARG(point);
  m_peakMarker->mouseMoveStop();
}

void PeakPicker::redrawMarker() { m_peakMarker->redraw(); }

} // namespace MantidWidgets
} // namespace MantidQt
