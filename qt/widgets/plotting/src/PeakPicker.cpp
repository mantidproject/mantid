// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Plotting/PeakPicker.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidQtWidgets/Plotting/PreviewPlot.h"

using namespace Mantid::API;
using namespace MantidQt::Widgets::MplCpp;

namespace MantidQt::MantidWidgets {

PeakPicker::PeakPicker(PreviewPlot *plot)
    : QObject(), m_plot(plot), m_peak(nullptr),
      m_peakMarker(std::make_unique<PeakMarker>(m_plot->canvas(), 1, 1.0, 0.0, 0.0, 0.0)) {
  m_plot->canvas()->draw();

  connect(m_plot, SIGNAL(mouseDown(QPoint)), this, SLOT(handleMouseDown(QPoint)));
  connect(m_plot, SIGNAL(mouseMove(QPoint)), this, SLOT(handleMouseMove(QPoint)));
  connect(m_plot, SIGNAL(mouseUp(QPoint)), this, SLOT(handleMouseUp(QPoint)));
  connect(m_plot, SIGNAL(mouseHovering(QPoint)), this, SLOT(handleMouseHovering(QPoint)));

  connect(m_plot, SIGNAL(redraw()), this, SLOT(redrawMarker()));
}

void PeakPicker::redraw() { m_peakMarker->redraw(); }

void PeakPicker::remove() { m_peakMarker->remove(); }

void PeakPicker::setPeak(const Mantid::API::IPeakFunction_const_sptr &peak) {
  if (peak) {
    m_peak = std::dynamic_pointer_cast<IPeakFunction>(peak->clone());
    m_peakMarker->updatePeak(peak->centre(), peak->height(), peak->fwhm());
  }
}

IPeakFunction_sptr PeakPicker::peak() const { return m_peak; }

void PeakPicker::select(bool select) {
  if (select)
    m_peakMarker->select();
  else
    m_peakMarker->deselect();
}

void PeakPicker::setVisible(bool visible) { m_peakMarker->setVisible(visible); }

void PeakPicker::handleMouseDown(const QPoint &point) {
  const auto dataCoords = m_plot->toDataCoords(point);
  m_peakMarker->mouseMoveStart(dataCoords.x(), dataCoords.y());

  if (m_peakMarker->isMoving())
    m_peakMarker->select();
}

void PeakPicker::handleMouseMove(const QPoint &point) {
  const auto dataCoords = m_plot->toDataCoords(point);
  const auto markerMoved = m_peakMarker->mouseMove(dataCoords.x(), dataCoords.y());

  if (markerMoved) {
    m_plot->replot();
    const auto [centre, height, fwhm] = m_peakMarker->peakProperties();
    m_peak->setCentre(centre);
    m_peak->setHeight(height);
    m_peak->setFwhm(fwhm);
    emit changed();
  }
}

void PeakPicker::handleMouseUp(const QPoint &point) {
  UNUSED_ARG(point);
  m_peakMarker->mouseMoveStop();
}

void PeakPicker::handleMouseHovering(const QPoint &point) {
  const auto dataCoords = m_plot->toDataCoords(point);
  m_peakMarker->mouseMoveHover(dataCoords.x(), dataCoords.y());
}

void PeakPicker::redrawMarker() { m_peakMarker->redraw(); }

} // namespace MantidQt::MantidWidgets
