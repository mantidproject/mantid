// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectFitDataView.h"

using namespace Mantid::API;

namespace {

bool isWorkspaceLoaded(std::string const &workspaceName) {
  return AnalysisDataService::Instance().doesExist(workspaceName);
}

} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectFitDataView::IndirectFitDataView(QWidget *parent)
    : IIndirectFitDataView(parent), m_dataForm(new Ui::IndirectFitDataForm) {
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

QTableWidget *IndirectFitDataView::getDataTable() const {
  return m_dataForm->tbFitData;
}

bool IndirectFitDataView::isMultipleDataTabSelected() const {
  return currentIndex() == 1;
}

bool IndirectFitDataView::isResolutionHidden() const {
  return m_dataForm->dsResolution->isHidden();
}

std::string IndirectFitDataView::getSelectedSample() const {
  return m_dataForm->dsSample->getCurrentDataName().toStdString();
}

std::string IndirectFitDataView::getSelectedResolution() const {
  return m_dataForm->dsResolution->getCurrentDataName().toStdString();
}

void IndirectFitDataView::readSettings(const QSettings &settings) {
  const auto group = settings.group();
  m_dataForm->dsSample->readSettings(group);
  m_dataForm->dsResolution->readSettings(group);
}

void IndirectFitDataView::disableMultipleDataTab() { setTabEnabled(1, false); }

QStringList IndirectFitDataView::getSampleWSSuffices() const {
  return m_dataForm->dsSample->getWSSuffixes();
}

QStringList IndirectFitDataView::getSampleFBSuffices() const {
  return m_dataForm->dsSample->getFBSuffixes();
}

QStringList IndirectFitDataView::getResolutionWSSuffices() const {
  return m_dataForm->dsResolution->getWSSuffixes();
}

QStringList IndirectFitDataView::getResolutionFBSuffices() const {
  return m_dataForm->dsResolution->getFBSuffixes();
}

void IndirectFitDataView::setSampleWSSuffices(const QStringList &suffices) {
  m_dataForm->dsSample->setWSSuffixes(suffices);
}

void IndirectFitDataView::setSampleFBSuffices(const QStringList &suffices) {
  m_dataForm->dsSample->setFBSuffixes(suffices);
}

void IndirectFitDataView::setResolutionWSSuffices(const QStringList &suffices) {
  m_dataForm->dsResolution->setWSSuffixes(suffices);
}

void IndirectFitDataView::setResolutionFBSuffices(const QStringList &suffices) {
  m_dataForm->dsResolution->setFBSuffixes(suffices);
}

bool IndirectFitDataView::isSampleWorkspaceSelectorVisible() const {
  return m_dataForm->dsSample->isWorkspaceSelectorVisible();
}

void IndirectFitDataView::setSampleWorkspaceSelectorIndex(
    const QString &workspaceName) {
  m_dataForm->dsSample->setWorkspaceSelectorIndex(workspaceName);
  m_dataForm->dsSample->setSelectorIndex(1);
}

UserInputValidator &
IndirectFitDataView::validate(UserInputValidator &validator) {
  if (currentIndex() == 0)
    return validateSingleData(validator);
  return validateMultipleData(validator);
}

UserInputValidator &
IndirectFitDataView::validateMultipleData(UserInputValidator &validator) {
  if (m_dataForm->tbFitData->rowCount() == 0)
    validator.addErrorMessage("No input data has been provided.");
  return validator;
}

UserInputValidator &
IndirectFitDataView::validateSingleData(UserInputValidator &validator) {
  validator = validateSample(validator);
  if (!isResolutionHidden())
    validator = validateResolution(validator);
  return validator;
}

UserInputValidator &
IndirectFitDataView::validateSample(UserInputValidator &validator) {
  const auto sampleIsLoaded = isWorkspaceLoaded(getSelectedSample());
  validator.checkDataSelectorIsValid("Sample Input", m_dataForm->dsSample);

  if (!sampleIsLoaded)
    emit sampleLoaded(QString::fromStdString(getSelectedSample()));
  return validator;
}

UserInputValidator &
IndirectFitDataView::validateResolution(UserInputValidator &validator) {
  const auto resolutionIsLoaded = isWorkspaceLoaded(getSelectedResolution());
  validator.checkDataSelectorIsValid("Resolution Input",
                                     m_dataForm->dsResolution);

  if (!resolutionIsLoaded)
    emit resolutionLoaded(QString::fromStdString(getSelectedResolution()));
  return validator;
}

void IndirectFitDataView::displayWarning(const std::string &warning) {
  QMessageBox::warning(parentWidget(), "MantidPlot - Warning",
                       QString::fromStdString(warning));
}

void IndirectFitDataView::setResolutionHidden(bool hide) {
  m_dataForm->lbResolution->setHidden(hide);
  m_dataForm->dsResolution->setHidden(hide);
}

void IndirectFitDataView::emitViewSelected(int index) {
  if (index == 0)
    emit singleDataViewSelected();
  else
    emit multipleDataViewSelected();
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
