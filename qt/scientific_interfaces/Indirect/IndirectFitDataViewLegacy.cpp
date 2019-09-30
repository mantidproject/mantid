// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectFitDataViewLegacy.h"

using namespace Mantid::API;

namespace {

bool isWorkspaceLoaded(std::string const &workspaceName) {
  return AnalysisDataService::Instance().doesExist(workspaceName);
}

} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectFitDataViewLegacy::IndirectFitDataViewLegacy(QWidget *parent)
    : IIndirectFitDataViewLegacy(parent),
      m_dataForm(new Ui::IndirectFitDataForm) {
  m_dataForm->setupUi(this);
  m_dataForm->dsResolution->hide();
  m_dataForm->lbResolution->hide();

  connect(m_dataForm->dsSample, SIGNAL(dataReady(const QString &)), this,
          SIGNAL(sampleLoaded(const QString &)));
  connect(m_dataForm->dsResolution, SIGNAL(dataReady(const QString &)), this,
          SIGNAL(resolutionLoaded(const QString &)));
  connect(m_dataForm->pbAdd, SIGNAL(clicked()), this, SIGNAL(addClicked()));
  connect(m_dataForm->pbRemove, SIGNAL(clicked()), this,
          SIGNAL(removeClicked()));

  connect(this, SIGNAL(currentChanged(int)), this, SLOT(emitViewSelected(int)));

  m_dataForm->dsSample->isOptional(true);
  m_dataForm->dsResolution->isOptional(true);
}

QTableWidget *IndirectFitDataViewLegacy::getDataTable() const {
  return m_dataForm->tbFitData;
}

bool IndirectFitDataViewLegacy::isMultipleDataTabSelected() const {
  return currentIndex() == 1;
}

bool IndirectFitDataViewLegacy::isResolutionHidden() const {
  return m_dataForm->dsResolution->isHidden();
}

std::string IndirectFitDataViewLegacy::getSelectedSample() const {
  return m_dataForm->dsSample->getCurrentDataName().toStdString();
}

std::string IndirectFitDataViewLegacy::getSelectedResolution() const {
  return m_dataForm->dsResolution->getCurrentDataName().toStdString();
}

void IndirectFitDataViewLegacy::readSettings(const QSettings &settings) {
  const auto group = settings.group();
  m_dataForm->dsSample->readSettings(group);
  m_dataForm->dsResolution->readSettings(group);
}

void IndirectFitDataViewLegacy::disableMultipleDataTab() {
  setTabEnabled(1, false);
}

QStringList IndirectFitDataViewLegacy::getSampleWSSuffices() const {
  return m_dataForm->dsSample->getWSSuffixes();
}

QStringList IndirectFitDataViewLegacy::getSampleFBSuffices() const {
  return m_dataForm->dsSample->getFBSuffixes();
}

QStringList IndirectFitDataViewLegacy::getResolutionWSSuffices() const {
  return m_dataForm->dsResolution->getWSSuffixes();
}

QStringList IndirectFitDataViewLegacy::getResolutionFBSuffices() const {
  return m_dataForm->dsResolution->getFBSuffixes();
}

void IndirectFitDataViewLegacy::setSampleWSSuffices(
    const QStringList &suffices) {
  m_dataForm->dsSample->setWSSuffixes(suffices);
}

void IndirectFitDataViewLegacy::setSampleFBSuffices(
    const QStringList &suffices) {
  m_dataForm->dsSample->setFBSuffixes(suffices);
}

void IndirectFitDataViewLegacy::setResolutionWSSuffices(
    const QStringList &suffices) {
  m_dataForm->dsResolution->setWSSuffixes(suffices);
}

void IndirectFitDataViewLegacy::setResolutionFBSuffices(
    const QStringList &suffices) {
  m_dataForm->dsResolution->setFBSuffixes(suffices);
}

bool IndirectFitDataViewLegacy::isSampleWorkspaceSelectorVisible() const {
  return m_dataForm->dsSample->isWorkspaceSelectorVisible();
}

void IndirectFitDataViewLegacy::setSampleWorkspaceSelectorIndex(
    const QString &workspaceName) {
  m_dataForm->dsSample->setWorkspaceSelectorIndex(workspaceName);
  m_dataForm->dsSample->setSelectorIndex(1);
}

UserInputValidator &
IndirectFitDataViewLegacy::validate(UserInputValidator &validator) {
  if (currentIndex() == 0)
    return validateSingleData(validator);
  return validateMultipleData(validator);
}

UserInputValidator &
IndirectFitDataViewLegacy::validateMultipleData(UserInputValidator &validator) {
  if (m_dataForm->tbFitData->rowCount() == 0)
    validator.addErrorMessage("No input data has been provided.");
  return validator;
}

UserInputValidator &
IndirectFitDataViewLegacy::validateSingleData(UserInputValidator &validator) {
  validator = validateSample(validator);
  if (!isResolutionHidden())
    validator = validateResolution(validator);
  return validator;
}

UserInputValidator &
IndirectFitDataViewLegacy::validateSample(UserInputValidator &validator) {
  const auto sampleIsLoaded = isWorkspaceLoaded(getSelectedSample());
  validator.checkDataSelectorIsValid("Sample Input", m_dataForm->dsSample);

  if (!sampleIsLoaded)
    emit sampleLoaded(QString::fromStdString(getSelectedSample()));
  return validator;
}

UserInputValidator &
IndirectFitDataViewLegacy::validateResolution(UserInputValidator &validator) {
  const auto resolutionIsLoaded = isWorkspaceLoaded(getSelectedResolution());
  validator.checkDataSelectorIsValid("Resolution Input",
                                     m_dataForm->dsResolution);

  if (!resolutionIsLoaded)
    emit resolutionLoaded(QString::fromStdString(getSelectedResolution()));
  return validator;
}

void IndirectFitDataViewLegacy::displayWarning(const std::string &warning) {
  QMessageBox::warning(parentWidget(), "MantidPlot - Warning",
                       QString::fromStdString(warning));
}

void IndirectFitDataViewLegacy::setResolutionHidden(bool hide) {
  m_dataForm->lbResolution->setHidden(hide);
  m_dataForm->dsResolution->setHidden(hide);
}

void IndirectFitDataViewLegacy::emitViewSelected(int index) {
  if (index == 0)
    emit singleDataViewSelected();
  else
    emit multipleDataViewSelected();
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
