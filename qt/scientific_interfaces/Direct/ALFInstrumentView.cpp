// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALFInstrumentView.h"

#include "ALFInstrumentPresenter.h"
#include "ALFInstrumentWidget.h"
#include "MantidQtWidgets/Common/InputController.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidget.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetPickTab.h"
#include "MantidQtWidgets/InstrumentView/UnwrappedSurface.h"

#include <string>

#include <QPushButton>
#include <QString>

namespace MantidQt::CustomInterfaces {

ALFInstrumentView::ALFInstrumentView(QWidget *parent) : ALFInstrumentViewBase(parent), m_instrumentWidget() {}

void ALFInstrumentView::setUpInstrument(std::string const &fileName) {
  m_instrumentWidget = new ALFInstrumentWidget(QString::fromStdString(fileName));

  connect(m_instrumentWidget, SIGNAL(instrumentActorReset()), this, SLOT(reconnectInstrumentActor()));
  connect(m_instrumentWidget, SIGNAL(surfaceTypeChanged(int)), this, SLOT(reconnectSurface()));
  connect(m_instrumentWidget, SIGNAL(surfaceTypeChanged(int)), this, SLOT(notifyShapeChanged()));
  reconnectInstrumentActor();
  reconnectSurface();

  auto pickTab = m_instrumentWidget->getPickTab();
  connect(pickTab->getSelectTubeButton(), SIGNAL(clicked()), this, SLOT(selectWholeTube()));
}

void ALFInstrumentView::reconnectInstrumentActor() {
  connect(&m_instrumentWidget->getInstrumentActor(), SIGNAL(refreshView()), this, SLOT(notifyInstrumentActorReset()));
}

void ALFInstrumentView::reconnectSurface() {
  auto surface = m_instrumentWidget->getInstrumentDisplay()->getSurface().get();

  // This signal has been disconnected as we do not want a copy and paste event to update the analysis plot, unless
  // the pasted shape is subsequently moved
  // connect(surface, SIGNAL(shapeCreated()), this, SLOT(notifyShapeChanged()));
  connect(surface, SIGNAL(shapeChangeFinished()), this, SLOT(notifyShapeChanged()));
  connect(surface, SIGNAL(shapesRemoved()), this, SLOT(notifyShapeChanged()));
  connect(surface, SIGNAL(shapesCleared()), this, SLOT(notifyShapeChanged()));
  connect(surface, SIGNAL(singleComponentPicked(size_t)), this, SLOT(notifyWholeTubeSelected(size_t)));
}

QWidget *ALFInstrumentView::getInstrumentView() { return m_instrumentWidget; }

MantidWidgets::IInstrumentActor const &ALFInstrumentView::getInstrumentActor() const {
  return m_instrumentWidget->getInstrumentActor();
}

std::vector<DetectorTube> ALFInstrumentView::getSelectedDetectors() const {
  auto const surface = m_instrumentWidget->getInstrumentDisplay()->getSurface();
  auto const unwrappedSurface = std::dynamic_pointer_cast<MantidQt::MantidWidgets::UnwrappedSurface>(surface);

  if (!unwrappedSurface) {
    return {};
  }

  std::vector<size_t> detectorIndices;
  // Find the detectors which are being intersected by the "masked" shapes.
  unwrappedSurface->getIntersectingDetectors(detectorIndices);
  // Find all the detector indices in the entirety of the selected tubes
  return m_instrumentWidget->findWholeTubeDetectorIndices(detectorIndices);
}

void ALFInstrumentView::selectWholeTube() {
  auto pickTab = m_instrumentWidget->getPickTab();
  pickTab->setPlotType(MantidQt::MantidWidgets::IWPickPlotType::TUBE_INTEGRAL);
  pickTab->setTubeXUnits(MantidQt::MantidWidgets::IWPickXUnits::OUT_OF_PLANE_ANGLE);
}

void ALFInstrumentView::notifyWholeTubeSelected(std::size_t pickID) {
  auto const pickTab = m_instrumentWidget->getPickTab();
  if (pickTab->getSelectTubeButton()->isChecked()) {
    m_presenter->notifyTubesSelected(m_instrumentWidget->findWholeTubeDetectorIndices({pickID}));
  }
}

void ALFInstrumentView::clearShapes() {
  auto surface = m_instrumentWidget->getInstrumentDisplay()->getSurface();
  surface->blockSignals(true);
  surface->clearMaskedShapes();
  surface->blockSignals(false);
}

void ALFInstrumentView::drawRectanglesAbove(std::vector<DetectorTube> const &tubes) {
  if (!tubes.empty()) {
    m_instrumentWidget->drawRectanglesAbove(tubes);
  }
}

} // namespace MantidQt::CustomInterfaces
