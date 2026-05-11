// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "QtPreviewInstrumentDisplay.h"
#include "MantidQtWidgets/InstrumentView/InstrumentActor.h"
#include "MantidQtWidgets/InstrumentView/UnwrappedCylinder.h"

#include <QVBoxLayout>

using MantidQt::MantidWidgets::ProjectionSurface;

namespace MantidQt::CustomInterfaces::ISISReflectometry {

QtPreviewInstrumentDisplay::QtPreviewInstrumentDisplay(QWidget *placeholder, std::function<void()> onShapeChanged,
                                                       std::unique_ptr<IInstViewModel> instViewModel)
    : m_placeholder(placeholder), m_instViewModel(std::move(instViewModel)),
      m_onShapeChanged(std::move(onShapeChanged)) {
  resetInstView();
}

QtPreviewInstrumentDisplay::~QtPreviewInstrumentDisplay() { disconnectSurfaceSignals(); }

void QtPreviewInstrumentDisplay::updateWorkspace(Mantid::API::MatrixWorkspace_sptr &workspace) {
  m_instViewModel->updateWorkspace(workspace);
}

void QtPreviewInstrumentDisplay::resetInstView() {
  disconnectSurfaceSignals();
  m_instDisplay = std::make_unique<MantidWidgets::InstrumentDisplay>(m_placeholder);
}

void QtPreviewInstrumentDisplay::plotInstView() {
  auto *actor = m_instViewModel->getInstrumentViewActor();
  if (!actor || !m_instDisplay) {
    return;
  }
  disconnectSurfaceSignals();
  auto widgetSize = m_instDisplay->currentWidget()->size();
  m_instDisplay->setSurface(std::make_shared<MantidWidgets::UnwrappedCylinder>(
      actor, m_instViewModel->getSamplePos(), m_instViewModel->getAxis(), widgetSize, false));
  connectSurfaceSignals();
}

void QtPreviewInstrumentDisplay::setInstViewZoomMode() {
  auto surface = getSurface();
  if (surface)
    surface->setInteractionMode(ProjectionSurface::MoveMode);
}

void QtPreviewInstrumentDisplay::setInstViewEditMode() {
  auto surface = getSurface();
  if (surface)
    surface->setInteractionMode(ProjectionSurface::EditShapeMode);
}

void QtPreviewInstrumentDisplay::setInstViewSelectRectMode() {
  auto surface = getSurface();
  if (surface) {
    surface->setInteractionMode(ProjectionSurface::EditShapeMode);
    surface->startCreatingShape2D("rectangle", Qt::green, QColor(255, 255, 255, 80));
  }
}

std::vector<Mantid::detid_t> QtPreviewInstrumentDisplay::getSelectedDetectorIDs() const {
  std::vector<Mantid::detid_t> result;
  if (m_instDisplay)
    if (auto surface = m_instDisplay->getSurface()) {
      std::vector<size_t> detIndices;
      surface->getMaskedDetectors(detIndices);
      result = m_instViewModel->detIndicesToDetIDs(detIndices);
    }
  return result;
}

void QtPreviewInstrumentDisplay::disconnectSurfaceSignals() {
  for (auto &conn : m_surfaceConnections) {
    QObject::disconnect(conn);
  }
  m_surfaceConnections.clear();
}

void QtPreviewInstrumentDisplay::connectSurfaceSignals() {
  const auto &surface = getSurface();
  if (!surface)
    return;
  auto callback = [this]() {
    if (m_onShapeChanged)
      m_onShapeChanged();
  };
  m_surfaceConnections.push_back(
      QObject::connect(surface.get(), &ProjectionSurface::shapeChangeFinished, m_placeholder, callback));
  m_surfaceConnections.push_back(
      QObject::connect(surface.get(), &ProjectionSurface::shapesRemoved, m_placeholder, callback));
  m_surfaceConnections.push_back(
      QObject::connect(surface.get(), &ProjectionSurface::shapesCleared, m_placeholder, callback));
}

MantidQt::MantidWidgets::ProjectionSurface_sptr QtPreviewInstrumentDisplay::getSurface() {
  if (!m_instDisplay)
    return nullptr;
  return m_instDisplay->getSurface();
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
