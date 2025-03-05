// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "ISISEnergyTransferView.h"
#include "Common/DetectorGroupingOptions.h"
#include "ISISEnergyTransferPresenter.h"
#include "MantidQtWidgets/Spectroscopy/DataValidationHelper.h"
#include "MantidQtWidgets/Spectroscopy/RunWidget/RunView.h"

#include "MantidQtWidgets/Common/AlgorithmDialog.h"
#include "MantidQtWidgets/Common/InterfaceManager.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

using namespace DataValidationHelper;
using namespace Mantid::API;
using namespace MantidQt::API;

namespace MantidQt::CustomInterfaces {

IETView::IETView(QWidget *parent) {
  m_uiForm.setupUi(parent);

  connect(m_uiForm.pbPlotTime, &QPushButton::clicked, this, &IETView::plotRawClicked);
  connect(m_uiForm.dsRunFiles, &FileFinderWidget::findingFiles, this, &IETView::pbRunFinding);
  connect(m_uiForm.dsRunFiles, &FileFinderWidget::fileFindingFinished, this, &IETView::pbRunFinished);
  connect(m_uiForm.dsCalibrationFile, &DataSelector::dataReady, this, &IETView::handleDataReady);
  connect(m_uiForm.pbSave, &QPushButton::clicked, this, &IETView::saveClicked);

  m_uiForm.dsCalibrationFile->isOptional(true);

  m_groupingWidget = new DetectorGroupingOptions(m_uiForm.fDetectorGrouping);
  m_uiForm.fDetectorGrouping->layout()->addWidget(m_groupingWidget);
  connect(m_groupingWidget, &DetectorGroupingOptions::saveCustomGrouping, this, &IETView::saveCustomGroupingClicked);
}

void IETView::subscribePresenter(IIETPresenter *presenter) { m_presenter = presenter; }

IETRunData IETView::getRunData() const {
  IETInputData inputDetails(m_uiForm.dsRunFiles->getFilenames().join(",").toStdString(),
                            m_uiForm.dsRunFiles->getText().toStdString(), m_uiForm.ckSumFiles->isChecked(),
                            m_uiForm.ckLoadLogFiles->isChecked(), m_uiForm.ckUseCalib->isChecked(),
                            m_uiForm.dsCalibrationFile->getCurrentDataName().toStdString());

  IETConversionData conversionDetails(m_uiForm.spEfixed->value(), m_uiForm.spSpectraMin->value(),
                                      m_uiForm.spSpectraMax->value());

  IETBackgroundData backgroundDetails(m_uiForm.ckBackgroundRemoval->isChecked(), m_uiForm.spBackgroundStart->value(),
                                      m_uiForm.spBackgroundEnd->value());

  IETAnalysisData analysisDetails(m_uiForm.ckDetailedBalance->isChecked(), m_uiForm.spDetailedBalance->value());

  IETRebinData rebinDetails(!m_uiForm.ckDoNotRebin->isChecked(), m_uiForm.cbRebinType->currentText().toStdString(),
                            m_uiForm.spRebinLow->value(), m_uiForm.spRebinHigh->value(), m_uiForm.spRebinWidth->value(),
                            m_uiForm.leRebinString->text().toStdString());

  IETOutputData outputDetails(m_uiForm.ckCm1Units->isChecked(), m_uiForm.ckFold->isChecked());

  IETRunData runParams(inputDetails, conversionDetails, m_groupingWidget->groupingProperties(), backgroundDetails,
                       analysisDetails, rebinDetails, outputDetails);

  return runParams;
}

IETPlotData IETView::getPlotData() const {
  IETInputData inputDetails(m_uiForm.dsRunFiles->getFilenames().join(",").toStdString(),
                            m_uiForm.dsRunFiles->getText().toStdString(), m_uiForm.ckSumFiles->isChecked(),
                            m_uiForm.ckLoadLogFiles->isChecked(), m_uiForm.ckUseCalib->isChecked(),
                            m_uiForm.dsCalibrationFile->getCurrentDataName().toStdString());

  IETConversionData conversionDetails(m_uiForm.spEfixed->value(), m_uiForm.spPlotTimeSpecMin->value(),
                                      m_uiForm.spPlotTimeSpecMax->value());

  IETBackgroundData backgroundDetails(m_uiForm.ckBackgroundRemoval->isChecked(), m_uiForm.spBackgroundStart->value(),
                                      m_uiForm.spBackgroundEnd->value());

  IETPlotData plotParams(inputDetails, conversionDetails, backgroundDetails);

  return plotParams;
}

IETSaveData IETView::getSaveData() const {
  IETSaveData saveTypes(m_uiForm.ckSaveNexus->isChecked(), m_uiForm.ckSaveSPE->isChecked(),
                        m_uiForm.ckSaveASCII->isChecked(), m_uiForm.ckSaveAclimax->isChecked(),
                        m_uiForm.ckSaveDaveGrp->isChecked());

  return saveTypes;
}

std::string IETView::getGroupOutputOption() const { return m_uiForm.cbGroupOutput->currentText().toStdString(); }

bool IETView::getGroupOutputCheckbox() const { return m_uiForm.ckGroupOutput->isChecked(); }

IOutputNameView *IETView::getOutputName() const { return m_uiForm.outNameWidget; }

IRunView *IETView::getRunView() const { return m_uiForm.runWidget; }

IOutputPlotOptionsView *IETView::getPlotOptionsView() const { return m_uiForm.ipoPlotOptions; }

std::string IETView::getFirstFilename() const { return m_uiForm.dsRunFiles->getFirstFilename().toStdString(); }

std::string IETView::getInputText() const { return m_uiForm.dsRunFiles->getText().toStdString(); }

bool IETView::isRunFilesValid() const { return m_uiForm.dsRunFiles->isValid(); }

void IETView::validateCalibrationFileType(IUserInputValidator *uiv) const {
  validateDataIsOfType(uiv, m_uiForm.dsCalibrationFile, "Calibration", DataType::Calib);
}

void IETView::validateRebinString(IUserInputValidator *uiv) const {
  uiv->checkFieldIsNotEmpty("Rebin string", m_uiForm.leRebinString, m_uiForm.valRebinString);
}

std::optional<std::string> IETView::validateGroupingProperties(std::size_t const &spectraMin,
                                                               std::size_t const &spectraMax) const {
  return m_groupingWidget->validateGroupingProperties(spectraMin, spectraMax);
}

bool IETView::showRebinWidthPrompt() const {
  const char *text = "The Binning width is currently negative, this suggests "
                     "you wish to use logarithmic binning.\n"
                     " Do you want to use Logarithmic Binning?";
  int result = QMessageBox::question(nullptr, tr("Logarithmic Binning"), tr(text), QMessageBox::Yes, QMessageBox::No,
                                     QMessageBox::NoButton);

  return result == QMessageBox::Yes;
}

void IETView::showSaveCustomGroupingDialog(std::string const &customGroupingOutput,
                                           std::string const &defaultGroupingFilename,
                                           std::string const &saveDirectory) const {
  QHash<QString, QString> props;
  props["InputWorkspace"] = QString::fromStdString(customGroupingOutput);
  props["OutputFile"] = QString::fromStdString(saveDirectory + defaultGroupingFilename);

  InterfaceManager interfaceManager;
  auto *dialog = interfaceManager.createDialogFromName("SaveDetectorsGrouping", -1, nullptr, false, props, "",
                                                       QStringList("OutputFile"));

  dialog->show();
  dialog->raise();
  dialog->activateWindow();
}

void IETView::displayWarning(std::string const &message) const {
  QMessageBox::warning(nullptr, "", QString::fromStdString(message));
}

void IETView::setCalibVisible(bool visible) {
  m_uiForm.ckUseCalib->setVisible(visible);
  m_uiForm.dsCalibrationFile->setVisible(visible);
}

void IETView::setEfixedVisible(bool visible) {
  m_uiForm.spEfixed->setVisible(visible);
  m_uiForm.lbEfixed->setVisible(visible);
}

void IETView::setBackgroundSectionVisible(bool visible) { m_uiForm.gbBackgroundRemoval->setVisible(visible); }

void IETView::setPlotTimeSectionVisible(bool visible) { m_uiForm.gbPlotTime->setVisible(visible); }

void IETView::setAnalysisSectionVisible(bool visible) { m_uiForm.gbAnalysis->setVisible(visible); }

void IETView::setPlottingOptionsVisible(bool visible) { m_uiForm.fPlottingOptions->setVisible(visible); }

void IETView::setAclimaxSaveVisible(bool visible) { m_uiForm.ckSaveAclimax->setVisible(visible); }

void IETView::setSPEVisible(bool visible) { m_uiForm.ckSaveSPE->setVisible(visible); }

void IETView::setFoldMultipleFramesVisible(bool visible) { m_uiForm.ckFold->setVisible(visible); }

void IETView::setOutputInCm1Visible(bool visible) { m_uiForm.ckCm1Units->setVisible(visible); }

void IETView::setGroupOutputCheckBoxVisible(bool visible) { m_uiForm.ckGroupOutput->setVisible(visible); }

void IETView::setGroupOutputDropdownVisible(bool visible) { m_uiForm.cbGroupOutput->setVisible(visible); }

void IETView::setDetailedBalance(double detailedBalance) { m_uiForm.spDetailedBalance->setValue(detailedBalance); }

void IETView::setRunFilesEnabled(bool enable) { m_uiForm.dsRunFiles->setEnabled(enable); }

void IETView::setSingleRebin(bool enable) {
  m_uiForm.valRebinLow->setVisible(enable);
  m_uiForm.valRebinWidth->setVisible(enable);
  m_uiForm.valRebinHigh->setVisible(enable);
}

void IETView::setMultipleRebin(bool enable) { m_uiForm.valRebinString->setVisible(enable); }

void IETView::setSaveEnabled(bool enable) {
  m_uiForm.pbSave->setEnabled(enable);
  m_uiForm.ckSaveAclimax->setEnabled(enable);
  m_uiForm.ckSaveASCII->setEnabled(enable);
  m_uiForm.ckSaveDaveGrp->setEnabled(enable);
  m_uiForm.ckSaveNexus->setEnabled(enable);
  m_uiForm.ckSaveSPE->setEnabled(enable);
}

void IETView::setPlotTimeIsPlotting(bool plotting) {
  m_uiForm.pbPlotTime->setText(plotting ? "Plotting..." : "Plot");
  setEnableOutputOptions(!plotting);
}

void IETView::setFileExtensionsByName(QStringList calibrationFbSuffixes, QStringList calibrationWSSuffixes) {
  m_uiForm.dsCalibrationFile->setFBSuffixes(calibrationFbSuffixes);
  m_uiForm.dsCalibrationFile->setWSSuffixes(calibrationWSSuffixes);
}

void IETView::setLoadHistory(bool doLoadHistory) {
  m_uiForm.dsCalibrationFile->setLoadProperty("LoadHistory", doLoadHistory);
}

void IETView::setInstrumentSpectraRange(int specMin, int specMax) {
  m_uiForm.spSpectraMin->setRange(specMin, specMax);
  m_uiForm.spSpectraMin->setValue(specMin);

  m_uiForm.spSpectraMax->setRange(specMin, specMax);
  m_uiForm.spSpectraMax->setValue(specMax);

  m_uiForm.spPlotTimeSpecMin->setRange(1, specMax);
  m_uiForm.spPlotTimeSpecMin->setValue(1);

  m_uiForm.spPlotTimeSpecMax->setRange(1, specMax);
  m_uiForm.spPlotTimeSpecMax->setValue(1);
}

void IETView::setInstrumentRebinning(std::vector<double> const &rebinParams, std::string const &rebinText, bool checked,
                                     int tabIndex) {
  m_uiForm.ckDoNotRebin->setChecked(checked);
  m_uiForm.cbRebinType->setCurrentIndex(tabIndex);
  m_uiForm.spRebinLow->setValue(rebinParams[0]);
  m_uiForm.spRebinWidth->setValue(rebinParams[1]);
  m_uiForm.spRebinHigh->setValue(rebinParams[2]);
  m_uiForm.leRebinString->setText(QString::fromStdString(rebinText));
}

void IETView::setInstrumentGrouping(std::string const &instrumentName) {

  setGroupOutputCheckBoxVisible(instrumentName == "OSIRIS");
  setGroupOutputDropdownVisible(instrumentName == "IRIS");

  m_groupingWidget->setGroupingMethod(instrumentName == "TOSCA" ? "IPF" : "Individual");
  m_uiForm.cbGroupOutput->clear();
  m_uiForm.cbGroupOutput->addItem(QString::fromStdString(IETGroupOption::UNGROUPED));
  m_uiForm.cbGroupOutput->addItem(QString::fromStdString(IETGroupOption::GROUP));
  if (instrumentName == "IRIS") {
    m_uiForm.cbGroupOutput->addItem(QString::fromStdString(IETGroupOption::SAMPLECHANGERGROUPED));
  }
}

void IETView::setInstrumentEFixed(std::string const &instrumentName, double eFixed) {
  QStringList qens;
  qens << "IRIS"
       << "OSIRIS";
  auto instName = QString::fromStdString(instrumentName);
  m_uiForm.spEfixed->setEnabled(qens.contains(instName));
  m_uiForm.dsRunFiles->setInstrumentOverride(instName);
  m_uiForm.spEfixed->setValue(eFixed);
}

void IETView::setInstrumentSpecDefault(std::map<std::string, bool> &specMap) {
  setBackgroundSectionVisible(specMap["irsORosiris"]);
  setPlotTimeSectionVisible(specMap["irsORosiris"]);
  setAclimaxSaveVisible(specMap["irsORosiris"]);
  setFoldMultipleFramesVisible(specMap["irsORosiris"]);
  setOutputInCm1Visible(specMap["irsORosiris"]);

  setSPEVisible(specMap["toscaORtfxa"]);
  setAnalysisSectionVisible(specMap["toscaORtfxa"]);
  setCalibVisible(specMap["toscaORtfxa"]);
  setEfixedVisible(specMap["toscaORtfxa"]);

  m_uiForm.ckCm1Units->setChecked(specMap["defaultEUnits"]);
  m_uiForm.ckSaveNexus->setChecked(specMap["defaultSaveNexus"]);
  m_uiForm.ckSaveASCII->setChecked(specMap["defaultSaveASCII"]);
  m_uiForm.ckFold->setChecked(specMap["defaultFoldMultiple"]);
}

void IETView::setEnableOutputOptions(bool const enable) {
  setPlotTimeEnabled(enable);
  setSaveEnabled(enable);
}

void IETView::showMessageBox(std::string const &message) {
  QMessageBox::warning(this, "Warning!", QString::fromStdString(message));
}

void IETView::saveClicked() { m_presenter->notifySaveClicked(); }

void IETView::plotRawClicked() { m_presenter->notifyPlotRawClicked(); }

void IETView::saveCustomGroupingClicked(std::string const &customGrouping) {
  m_presenter->notifySaveCustomGroupingClicked(customGrouping);
}

void IETView::pbRunFinished() { m_presenter->notifyRunFinished(); }

void IETView::handleDataReady() {
  auto uiv = std::make_unique<UserInputValidator>();
  validateDataIsOfType(uiv.get(), m_uiForm.dsCalibrationFile, "Calibration", DataType::Calib);

  auto const errorMessage = uiv->generateErrorMessage();
  if (!errorMessage.empty())
    showMessageBox(errorMessage);
}

void IETView::pbRunFinding() {
  m_presenter->notifyFindingRun();
  m_uiForm.dsRunFiles->setEnabled(false);
}

void IETView::setPlotTimeEnabled(bool enable) {
  m_uiForm.pbPlotTime->setEnabled(enable);
  m_uiForm.spPlotTimeSpecMin->setEnabled(enable);
  m_uiForm.spPlotTimeSpecMax->setEnabled(enable);
}

} // namespace MantidQt::CustomInterfaces
