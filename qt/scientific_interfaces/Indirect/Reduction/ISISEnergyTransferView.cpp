// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "ISISEnergyTransferView.h"
#include "Common/IndirectDataValidationHelper.h"

#include "MantidQtWidgets/Common/AlgorithmDialog.h"
#include "MantidQtWidgets/Common/InterfaceManager.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

using namespace IndirectDataValidationHelper;
using namespace Mantid::API;
using namespace MantidQt::API;

namespace MantidQt::CustomInterfaces {

IETView::IETView(IETViewSubscriber *subscriber, QWidget *parent) : m_subscriber(subscriber) {
  m_uiForm.setupUi(parent);

  connect(m_uiForm.cbGroupingOptions, SIGNAL(currentIndexChanged(const QString &)), this,
          SLOT(mappingOptionSelected(const QString &)));
  connect(m_uiForm.pbSaveCustomGrouping, SIGNAL(clicked()), this, SLOT(saveCustomGroupingClicked()));
  connect(m_uiForm.pbPlotTime, SIGNAL(clicked()), this, SLOT(plotRawClicked()));
  connect(m_uiForm.dsRunFiles, SIGNAL(fileTextChanged(const QString &)), this, SLOT(pbRunEditing()));
  connect(m_uiForm.dsRunFiles, SIGNAL(findingFiles()), this, SLOT(pbRunFinding()));
  connect(m_uiForm.dsRunFiles, SIGNAL(fileFindingFinished()), this, SLOT(pbRunFinished()));
  connect(m_uiForm.dsCalibrationFile, SIGNAL(dataReady(QString const &)), this, SLOT(handleDataReady()));
  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(saveClicked()));

  m_uiForm.dsCalibrationFile->isOptional(true);

  mappingOptionSelected(m_uiForm.cbGroupingOptions->currentText());

  QRegExp re("([0-9]+[-:+]?[0-9]*([+]?[0-9]*)*,[ ]?)*[0-9]+[-:+]?[0-9]*([+]?[0-9]*)*");
  m_uiForm.leCustomGroups->setValidator(new QRegExpValidator(re, this));
}

IETView::~IETView() {}

IETRunData IETView::getRunData() const {
  IETInputData inputDetails(m_uiForm.dsRunFiles->getFilenames().join(",").toStdString(),
                            m_uiForm.dsRunFiles->getText().toStdString(), m_uiForm.ckSumFiles->isChecked(),
                            m_uiForm.ckLoadLogFiles->isChecked(), m_uiForm.ckUseCalib->isChecked(),
                            m_uiForm.dsCalibrationFile->getCurrentDataName().toStdString());

  IETConversionData conversionDetails(m_uiForm.spEfixed->value(), m_uiForm.spSpectraMin->value(),
                                      m_uiForm.spSpectraMax->value());

  IETGroupingData groupingDetails(m_uiForm.cbGroupingOptions->currentText().toStdString(),
                                  m_uiForm.spNumberGroups->value(),
                                  m_uiForm.dsMapFile->getFirstFilename().toStdString());

  IETBackgroundData backgroundDetails(m_uiForm.ckBackgroundRemoval->isChecked(), m_uiForm.spBackgroundStart->value(),
                                      m_uiForm.spBackgroundEnd->value());

  IETAnalysisData analysisDetails(m_uiForm.ckDetailedBalance->isChecked(), m_uiForm.spDetailedBalance->value());

  IETRebinData rebinDetails(!m_uiForm.ckDoNotRebin->isChecked(), m_uiForm.cbRebinType->currentText().toStdString(),
                            m_uiForm.spRebinLow->value(), m_uiForm.spRebinHigh->value(), m_uiForm.spRebinWidth->value(),
                            m_uiForm.leRebinString->text().toStdString());

  IETOutputData outputDetails(m_uiForm.ckCm1Units->isChecked(), m_uiForm.ckFold->isChecked());

  IETRunData runParams(inputDetails, conversionDetails, groupingDetails, backgroundDetails, analysisDetails,
                       rebinDetails, outputDetails);

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

std::string IETView::getCustomGrouping() const { return m_uiForm.leCustomGroups->text().toStdString(); }

std::string IETView::getGroupOutputOption() const { return m_uiForm.cbGroupOutput->currentText().toStdString(); }

bool IETView::getGroupOutputCheckbox() const { return m_uiForm.ckGroupOutput->isChecked(); }

IndirectPlotOptionsView *IETView::getPlotOptionsView() const { return m_uiForm.ipoPlotOptions; }

std::string IETView::getFirstFilename() const { return m_uiForm.dsRunFiles->getFirstFilename().toStdString(); }

bool IETView::isRunFilesValid() const { return m_uiForm.dsRunFiles->isValid(); }

void IETView::validateCalibrationFileType(UserInputValidator &uiv) const {
  validateDataIsOfType(uiv, m_uiForm.dsCalibrationFile, "Calibration", DataType::Calib);
}

void IETView::validateRebinString(UserInputValidator &uiv) const {
  uiv.checkFieldIsNotEmpty("Rebin string", m_uiForm.leRebinString, m_uiForm.valRebinString);
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
  enable = !m_outputWorkspaces.empty() ? enable : false;
  m_uiForm.pbSave->setEnabled(enable);
  m_uiForm.ckSaveAclimax->setEnabled(enable);
  m_uiForm.ckSaveASCII->setEnabled(enable);
  m_uiForm.ckSaveDaveGrp->setEnabled(enable);
  m_uiForm.ckSaveNexus->setEnabled(enable);
  m_uiForm.ckSaveSPE->setEnabled(enable);
}

void IETView::setPlotTimeIsPlotting(bool plotting) {
  m_uiForm.pbPlotTime->setText(plotting ? "Plotting..." : "Plot");
  setButtonsEnabled(!plotting);
}

void IETView::setFileExtensionsByName(QStringList calibrationFbSuffixes, QStringList calibrationWSSuffixes) {
  m_uiForm.dsCalibrationFile->setFBSuffixes(calibrationFbSuffixes);
  m_uiForm.dsCalibrationFile->setWSSuffixes(calibrationWSSuffixes);
}

void IETView::setOutputWorkspaces(std::vector<std::string> const &outputWorkspaces) {
  m_outputWorkspaces = outputWorkspaces;
}

void IETView::setInstrumentDefault(InstrumentData const &instrumentDetails) {
  auto const instrumentName = instrumentDetails.getInstrument();
  auto const specMin = instrumentDetails.getDefaultSpectraMin();
  auto const specMax = instrumentDetails.getDefaultSpectraMax();

  m_uiForm.dsRunFiles->setInstrumentOverride(QString::fromStdString(instrumentName));

  QStringList qens;
  qens << "IRIS"
       << "OSIRIS";
  m_uiForm.spEfixed->setEnabled(qens.contains(QString::fromStdString(instrumentName)));

  QStringList allowDefaultGroupingInstruments;
  allowDefaultGroupingInstruments << "TOSCA";
  includeExtraGroupingOption(allowDefaultGroupingInstruments.contains(QString::fromStdString(instrumentName)),
                             "Default");

  m_uiForm.spSpectraMin->setMinimum(specMin);
  m_uiForm.spSpectraMin->setMaximum(specMax);
  m_uiForm.spSpectraMin->setValue(specMin);

  m_uiForm.spSpectraMax->setMinimum(specMin);
  m_uiForm.spSpectraMax->setMaximum(specMax);
  m_uiForm.spSpectraMax->setValue(specMax);

  m_uiForm.spPlotTimeSpecMin->setMinimum(1);
  m_uiForm.spPlotTimeSpecMin->setMaximum(specMax);
  m_uiForm.spPlotTimeSpecMin->setValue(1);

  m_uiForm.spPlotTimeSpecMax->setMinimum(1);
  m_uiForm.spPlotTimeSpecMax->setMaximum(specMax);
  m_uiForm.spPlotTimeSpecMax->setValue(1);

  m_uiForm.spEfixed->setValue(instrumentDetails.getDefaultEfixed());

  auto const rebinDefault = QString::fromStdString(instrumentDetails.getDefaultRebin());
  if (!rebinDefault.isEmpty()) {
    m_uiForm.leRebinString->setText(rebinDefault);
    m_uiForm.ckDoNotRebin->setChecked(false);
    auto const rbp = rebinDefault.split(",", Qt::SkipEmptyParts);
    if (rbp.size() == 3) {
      m_uiForm.spRebinLow->setValue(rbp[0].toDouble());
      m_uiForm.spRebinWidth->setValue(rbp[1].toDouble());
      m_uiForm.spRebinHigh->setValue(rbp[2].toDouble());
      m_uiForm.cbRebinType->setCurrentIndex(0);
    } else {
      m_uiForm.cbRebinType->setCurrentIndex(1);
    }
  } else {
    m_uiForm.ckDoNotRebin->setChecked(true);
    m_uiForm.spRebinLow->setValue(0.0);
    m_uiForm.spRebinWidth->setValue(0.0);
    m_uiForm.spRebinHigh->setValue(0.0);
    m_uiForm.leRebinString->setText("");
  }

  m_uiForm.cbGroupOutput->clear();

  m_uiForm.cbGroupOutput->addItem(QString::fromStdString(IETGroupOption::UNGROUPED));
  m_uiForm.cbGroupOutput->addItem(QString::fromStdString(IETGroupOption::GROUP));
  if (instrumentName == "IRIS") {
    m_uiForm.cbGroupOutput->addItem(QString::fromStdString(IETGroupOption::SAMPLECHANGERGROUPED));
  }

  m_uiForm.ckCm1Units->setChecked(instrumentDetails.getDefaultUseDeltaEInWavenumber());
  m_uiForm.ckSaveNexus->setChecked(instrumentDetails.getDefaultSaveNexus());
  m_uiForm.ckSaveASCII->setChecked(instrumentDetails.getDefaultSaveASCII());
  m_uiForm.ckFold->setChecked(instrumentDetails.getDefaultFoldMultipleFrames());
}

void IETView::updateRunButton(bool enabled, std::string const &enableOutputButtons, QString const &message,
                              QString const &tooltip) {
  setRunEnabled(enabled);
  m_uiForm.pbRun->setText(message);
  m_uiForm.pbRun->setToolTip(tooltip);
  if (enableOutputButtons != "unchanged") {
    auto const enableButtons = enableOutputButtons == "enable";
    setPlotTimeEnabled(enableButtons);
    setSaveEnabled(enableButtons);
  }
}

void IETView::showMessageBox(const QString &message) { m_subscriber->notifyNewMessage(message); }

void IETView::saveClicked() { m_subscriber->notifySaveClicked(); }

void IETView::runClicked() { m_subscriber->notifyRunClicked(); }

void IETView::plotRawClicked() { m_subscriber->notifyPlotRawClicked(); }

void IETView::saveCustomGroupingClicked() { m_subscriber->notifySaveCustomGroupingClicked(); }

void IETView::pbRunFinished() { m_subscriber->notifyRunFinished(); }

void IETView::handleDataReady() {
  UserInputValidator uiv;
  validateDataIsOfType(uiv, m_uiForm.dsCalibrationFile, "Calibration", DataType::Calib);

  auto const errorMessage = uiv.generateErrorMessage();
  if (!errorMessage.isEmpty())
    emit showMessageBox(errorMessage);
}

void IETView::mappingOptionSelected(const QString &groupType) {
  if (groupType == "File")
    m_uiForm.swGrouping->setCurrentIndex(0);
  else if (groupType == "Groups")
    m_uiForm.swGrouping->setCurrentIndex(1);
  else if (groupType == "Custom")
    m_uiForm.swGrouping->setCurrentIndex(2);
  else
    m_uiForm.swGrouping->setCurrentIndex(3);
}

void IETView::pbRunEditing() {
  updateRunButton(false, "unchanged", "Editing...", "Run numbers are currently being edited.");
}

void IETView::pbRunFinding() {
  updateRunButton(false, "unchanged", "Finding files...", "Searching for data files for the run numbers entered...");
  m_uiForm.dsRunFiles->setEnabled(false);
}

int IETView::getGroupingOptionIndex(QString const &option) {
  auto const index = m_uiForm.cbGroupingOptions->findText(option);
  return index >= 0 ? index : 0;
}

bool IETView::isOptionHidden(QString const &option) {
  auto const index = m_uiForm.cbGroupingOptions->findText(option);
  return index == -1;
}

void IETView::includeExtraGroupingOption(bool includeOption, QString const &option) {
  if (includeOption && isOptionHidden(option)) {
    m_uiForm.cbGroupingOptions->addItem(option);
    m_uiForm.cbGroupingOptions->setCurrentIndex(getGroupingOptionIndex(option));
  } else if (!includeOption && !isOptionHidden(option))
    m_uiForm.cbGroupingOptions->removeItem(getGroupingOptionIndex(option));
}

void IETView::setRunEnabled(bool enable) { m_uiForm.pbRun->setEnabled(enable); }

void IETView::setPlotTimeEnabled(bool enable) {
  m_uiForm.pbPlotTime->setEnabled(enable);
  m_uiForm.spPlotTimeSpecMin->setEnabled(enable);
  m_uiForm.spPlotTimeSpecMax->setEnabled(enable);
}

void IETView::setButtonsEnabled(bool enable) {
  setRunEnabled(enable);
  setPlotTimeEnabled(enable);
  setSaveEnabled(enable);
}
} // namespace MantidQt::CustomInterfaces