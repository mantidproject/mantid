// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALFInstrumentView.h"

#include "ALFInstrumentPresenter.h"
#include "ALFInstrumentWidget.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidQtWidgets/Common/FileFinderWidget.h"
#include "MantidQtWidgets/Common/InputController.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidget.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetPickTab.h"
#include "MantidQtWidgets/InstrumentView/UnwrappedSurface.h"

#include <string>

#include <QMessageBox>
#include <QSizePolicy>
#include <QSpacerItem>
#include <QString>

namespace MantidQt::CustomInterfaces {

ALFInstrumentView::ALFInstrumentView(QWidget *parent)
    : QWidget(parent), m_settingsGroup("CustomInterfaces/ALFView"), m_sample(), m_vanadium(), m_instrumentWidget(),
      m_presenter() {}

void ALFInstrumentView::setUpInstrument(std::string const &fileName) {
  m_instrumentWidget = new ALFInstrumentWidget(QString::fromStdString(fileName));

  connect(m_instrumentWidget, SIGNAL(instrumentActorReset()), this, SLOT(reconnectInstrumentActor()));
  connect(m_instrumentWidget, SIGNAL(surfaceTypeChanged(int)), this, SLOT(reconnectSurface()));
  reconnectInstrumentActor();
  reconnectSurface();

  auto pickTab = m_instrumentWidget->getPickTab();
  connect(pickTab->getSelectTubeButton(), SIGNAL(clicked()), this, SLOT(selectWholeTube()));
}

QWidget *ALFInstrumentView::generateSampleLoadWidget() {
  m_sample = new API::FileFinderWidget(this);
  m_sample->setLabelText("Sample");
  m_sample->setLabelMinWidth(150);
  m_sample->allowMultipleFiles(false);
  m_sample->setInstrumentOverride("ALF");
  m_sample->isForRunFiles(true);

  connect(m_sample, SIGNAL(fileFindingFinished()), this, SLOT(sampleLoaded()));

  return m_sample;
}

QWidget *ALFInstrumentView::generateVanadiumLoadWidget() {
  m_vanadium = new API::FileFinderWidget(this);
  m_vanadium->isOptional(true);
  m_vanadium->setLabelText("Vanadium");
  m_vanadium->setLabelMinWidth(150);
  m_vanadium->allowMultipleFiles(false);
  m_vanadium->setInstrumentOverride("ALF");
  m_vanadium->isForRunFiles(true);

  connect(m_vanadium, SIGNAL(fileFindingFinished()), this, SLOT(vanadiumLoaded()));

  return m_vanadium;
}

void ALFInstrumentView::loadSettings() {
  QSettings settings;

  // Load the last used vanadium run
  settings.beginGroup(m_settingsGroup);
  auto const vanadiumRun = settings.value("vanadium-run", "");
  settings.endGroup();

  if (!vanadiumRun.toString().isEmpty()) {
    m_vanadium->setUserInput(vanadiumRun);
  }
}

void ALFInstrumentView::saveSettings() {
  QSettings settings;
  settings.beginGroup(m_settingsGroup);
  settings.setValue("vanadium-run", m_vanadium->getText());
  settings.endGroup();
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

void ALFInstrumentView::subscribePresenter(IALFInstrumentPresenter *presenter) { m_presenter = presenter; }

std::optional<std::string> ALFInstrumentView::getSampleFile() const {
  auto name = m_sample->getFilenames();
  if (name.size() > 0)
    return name[0].toStdString();
  return std::nullopt;
}

std::optional<std::string> ALFInstrumentView::getVanadiumFile() const {
  auto name = m_vanadium->getFilenames();
  if (name.size() > 0)
    return name[0].toStdString();
  return std::nullopt;
}

void ALFInstrumentView::setSampleRun(std::string const &runNumber) {
  m_sample->setText(QString::fromStdString(runNumber));
}

void ALFInstrumentView::setVanadiumRun(std::string const &runNumber) {
  m_vanadium->setText(QString::fromStdString(runNumber));
}

void ALFInstrumentView::sampleLoaded() {
  if (m_sample->getText().isEmpty()) {
    return;
  }

  if (!m_sample->isValid()) {
    warningBox(m_sample->getFileProblem().toStdString());
    return;
  }
  m_presenter->loadSample();
}

void ALFInstrumentView::vanadiumLoaded() {
  if (!m_vanadium->isValid()) {
    warningBox(m_vanadium->getFileProblem().toStdString());
    return;
  }
  m_presenter->loadVanadium();
}

void ALFInstrumentView::notifyInstrumentActorReset() { m_presenter->notifyInstrumentActorReset(); }

void ALFInstrumentView::notifyShapeChanged() { m_presenter->notifyShapeChanged(); }

MantidWidgets::IInstrumentActor const &ALFInstrumentView::getInstrumentActor() const {
  return m_instrumentWidget->getInstrumentActor();
}

std::vector<DetectorTube> ALFInstrumentView::getSelectedDetectors() const {
  auto const surface = std::dynamic_pointer_cast<MantidQt::MantidWidgets::UnwrappedSurface>(
      m_instrumentWidget->getInstrumentDisplay()->getSurface());

  std::vector<size_t> detectorIndices;
  // Find the detectors which are being intersected by the "masked" shapes.
  surface->getIntersectingDetectors(detectorIndices);
  // Find all the detector indices in the entirety of the selected tubes
  return m_instrumentWidget->findWholeTubeDetectorIndices(detectorIndices);
}

void ALFInstrumentView::selectWholeTube() {
  auto pickTab = m_instrumentWidget->getPickTab();
  pickTab->setPlotType(MantidQt::MantidWidgets::IWPickPlotType::TUBE_INTEGRAL);
  pickTab->setTubeXUnits(MantidQt::MantidWidgets::IWPickXUnits::OUT_OF_PLANE_ANGLE);
}

void ALFInstrumentView::notifyWholeTubeSelected(std::size_t pickID) {
  m_presenter->notifyTubesSelected(m_instrumentWidget->findWholeTubeDetectorIndices({pickID}));
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

void ALFInstrumentView::warningBox(std::string const &message) {
  QMessageBox::warning(this, "ALFView", QString::fromStdString(message));
}

} // namespace MantidQt::CustomInterfaces
