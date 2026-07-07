// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALFInstrumentViewBase.h"

#include "ALFInstrumentPresenter.h"
#include "ALFView.h"
#include "MantidQtWidgets/Common/FileFinderWidget.h"

#include <string>

#include <QMessageBox>
#include <QSettings>
#include <QString>

namespace MantidQt::CustomInterfaces {

ALFInstrumentViewBase::ALFInstrumentViewBase(QWidget *parent)
    : QWidget(parent), m_presenter(), m_settingsGroup("CustomInterfaces/ALFView"), m_sample(), m_vanadium() {}

QWidget *ALFInstrumentViewBase::generateSampleLoadWidget() {
  m_sample = new API::FileFinderWidget(this);
  m_sample->setLabelText("Sample");
  m_sample->setLabelMinWidth(150);
  m_sample->allowMultipleFiles(false);
  m_sample->setInstrumentOverride("ALF");
  m_sample->isForRunFiles(true);

  connect(m_sample, SIGNAL(fileFindingFinished()), this, SLOT(sampleLoaded()));

  return m_sample;
}

QWidget *ALFInstrumentViewBase::generateVanadiumLoadWidget() {
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

void ALFInstrumentViewBase::loadSettings() {
  QSettings settings;

  // Load the last used vanadium run
  settings.beginGroup(m_settingsGroup);
  auto const vanadiumRun = settings.value("vanadium-run", "");
  settings.endGroup();

  if (!vanadiumRun.toString().isEmpty()) {
    disable("Loading vanadium");
    m_vanadium->setUserInput(vanadiumRun);
  }
}

void ALFInstrumentViewBase::saveSettings() {
  QSettings settings;
  settings.beginGroup(m_settingsGroup);
  settings.setValue("vanadium-run", m_vanadium->getText());
  settings.endGroup();
}

void ALFInstrumentViewBase::disable(std::string const &reason) {
  if (auto parent = static_cast<ALFView *>(parentWidget())) {
    parent->disable(reason);
  }
}

void ALFInstrumentViewBase::enable() {
  if (auto parent = static_cast<ALFView *>(parentWidget())) {
    parent->enable();
  }
}

void ALFInstrumentViewBase::subscribePresenter(IALFInstrumentPresenter *presenter) { m_presenter = presenter; }

std::optional<std::string> ALFInstrumentViewBase::getSampleFile() const {
  auto name = m_sample->getFilenames();
  if (name.size() > 0)
    return name[0].toStdString();
  return std::nullopt;
}

std::optional<std::string> ALFInstrumentViewBase::getVanadiumFile() const {
  auto name = m_vanadium->getFilenames();
  if (name.size() > 0)
    return name[0].toStdString();
  return std::nullopt;
}

void ALFInstrumentViewBase::setSampleRun(std::string const &runNumber) {
  m_sample->setText(QString::fromStdString(runNumber));
}

void ALFInstrumentViewBase::setVanadiumRun(std::string const &runNumber) {
  m_vanadium->setText(QString::fromStdString(runNumber));
}

void ALFInstrumentViewBase::sampleLoaded() {
  if (!m_sample->getText().isEmpty() && !m_sample->isValid()) {
    displayWarning(m_sample->getFileProblem().toStdString());
    return;
  }
  m_presenter->loadSample();
}

void ALFInstrumentViewBase::vanadiumLoaded() {
  if (!m_vanadium->isValid()) {
    enable();
    displayWarning(m_vanadium->getFileProblem().toStdString());
    return;
  }
  m_presenter->loadVanadium();
}

void ALFInstrumentViewBase::notifyInstrumentActorReset() { m_presenter->notifyInstrumentActorReset(); }

void ALFInstrumentViewBase::notifyShapeChanged() { m_presenter->notifyShapeChanged(); }

void ALFInstrumentViewBase::displayWarning(std::string const &message) {
  QMessageBox::warning(this, "ALFView", QString::fromStdString(message));
}

} // namespace MantidQt::CustomInterfaces
