// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ISISDiagnostics.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/Logger.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"
#include "MantidQtWidgets/Common/WorkspaceUtils.h"
#include "MantidQtWidgets/Spectroscopy/InterfaceUtils.h"

#include <MantidAPI/FileFinder.h>
#include <MantidQtWidgets/Spectroscopy/SettingsWidget/SettingsHelper.h>
#include <QFileInfo>

using namespace Mantid::API;

using namespace MantidQt::MantidWidgets::WorkspaceUtils;
using namespace MantidQt::CustomInterfaces::InterfaceUtils;

namespace {
Mantid::Kernel::Logger g_log("ISISDiagnostics");
}

namespace MantidQt::CustomInterfaces {
//----------------------------------------------------------------------------------------------
/** Constructor
 */
ISISDiagnostics::ISISDiagnostics(IDataReduction *idrUI, QWidget *parent) : DataReductionTab(idrUI, parent) {
  m_uiForm.setupUi(parent);
  setRunWidgetPresenter(std::make_unique<RunPresenter>(this, m_uiForm.runWidget));
  setOutputPlotOptionsPresenter(
      std::make_unique<OutputPlotOptionsPresenter>(m_uiForm.ipoPlotOptions, PlotWidget::Spectra));

  m_uiForm.ppRawPlot->setCanvasColour(QColor(240, 240, 240));
  m_uiForm.ppSlicePreview->setCanvasColour(QColor(240, 240, 240));
  m_uiForm.ppRawPlot->watchADS(false);

  // Property Tree
  m_propTrees["SlicePropTree"] = new QtTreePropertyBrowser();
  m_uiForm.properties->addWidget(m_propTrees["SlicePropTree"]);

  // Editor Factories
  auto *doubleEditorFactory = new DoubleEditorFactory();
  auto *checkboxFactory = new QtCheckBoxFactory();
  m_propTrees["SlicePropTree"]->setFactoryForManager(m_dblManager, doubleEditorFactory);
  m_propTrees["SlicePropTree"]->setFactoryForManager(m_blnManager, checkboxFactory);

  // Create Properties
  m_properties["PreviewSpec"] = m_dblManager->addProperty("Preview Spectrum");
  m_dblManager->setDecimals(m_properties["PreviewSpec"], 0);
  m_dblManager->setMinimum(m_properties["PreviewSpec"], 1);

  m_properties["SpecMin"] = m_dblManager->addProperty("Spectra Min");
  m_dblManager->setDecimals(m_properties["SpecMin"], 0);
  m_dblManager->setMinimum(m_properties["SpecMin"], 1);

  m_properties["SpecMax"] = m_dblManager->addProperty("Spectra Max");
  m_dblManager->setDecimals(m_properties["SpecMax"], 0);
  m_dblManager->setMinimum(m_properties["SpecMax"], 1);

  m_properties["PeakStart"] = m_dblManager->addProperty("Start");
  m_properties["PeakEnd"] = m_dblManager->addProperty("End");

  m_properties["BackgroundStart"] = m_dblManager->addProperty("Start");
  m_properties["BackgroundEnd"] = m_dblManager->addProperty("End");

  m_properties["UseTwoRanges"] = m_blnManager->addProperty("Use Two Ranges");

  m_properties["PeakRange"] = m_grpManager->addProperty("Peak");
  m_properties["PeakRange"]->addSubProperty(m_properties["PeakStart"]);
  m_properties["PeakRange"]->addSubProperty(m_properties["PeakEnd"]);

  m_properties["BackgroundRange"] = m_grpManager->addProperty("Background");
  m_properties["BackgroundRange"]->addSubProperty(m_properties["BackgroundStart"]);
  m_properties["BackgroundRange"]->addSubProperty(m_properties["BackgroundEnd"]);

  m_propTrees["SlicePropTree"]->addProperty(m_properties["PreviewSpec"]);
  m_propTrees["SlicePropTree"]->addProperty(m_properties["SpecMin"]);
  m_propTrees["SlicePropTree"]->addProperty(m_properties["SpecMax"]);
  m_propTrees["SlicePropTree"]->addProperty(m_properties["PeakRange"]);
  m_propTrees["SlicePropTree"]->addProperty(m_properties["UseTwoRanges"]);
  m_propTrees["SlicePropTree"]->addProperty(m_properties["BackgroundRange"]);

  // Slice plot
  auto peakRangeSelector = m_uiForm.ppRawPlot->addRangeSelector("SlicePeak");
  auto backgroundRangeSelector = m_uiForm.ppRawPlot->addRangeSelector("SliceBackground");

  // Setup second range
  backgroundRangeSelector->setColour(Qt::darkGreen); // Dark green for background

  // SIGNAL/SLOT CONNECTIONS

  // Update properties when a range selector is changed
  connect(peakRangeSelector, &MantidWidgets::RangeSelector::selectionChanged, this,
          &ISISDiagnostics::rangeSelectorDropped);
  connect(backgroundRangeSelector, &MantidWidgets::RangeSelector::selectionChanged, this,
          &ISISDiagnostics::rangeSelectorDropped);

  // Update range selctors when a property is changed
  connect(m_dblManager, &QtDoublePropertyManager::valueChanged, this, &ISISDiagnostics::doublePropertyChanged);
  // Enable/disable second range options when checkbox is toggled
  connect(m_blnManager, &QtBoolPropertyManager::valueChanged, this, &ISISDiagnostics::sliceTwoRanges);
  // Enables/disables calibration file selection when user toggles Use
  // Calibratin File checkbox
  connect(m_uiForm.ckUseCalibration, &QCheckBox::toggled, this, &ISISDiagnostics::sliceCalib);
  // Plot slice miniplot when file has finished loading
  connect(m_uiForm.dsInputFiles, &FileFinderWidget::filesFoundChanged, this, &ISISDiagnostics::handleNewFile);
  // Shows message on run button when Mantid is finding the file for a given run
  // number
  connect(m_uiForm.dsInputFiles, &FileFinderWidget::findingFiles, this, &ISISDiagnostics::pbRunFinding);
  // Reverts run button back to normal when file finding has finished
  connect(m_uiForm.dsInputFiles, &FileFinderWidget::fileFindingFinished, this, &ISISDiagnostics::pbRunFinished);
  // Handles running, plotting and saving
  connect(m_uiForm.pbSave, &QPushButton::clicked, this, &ISISDiagnostics::saveClicked);

  // Set default UI state
  sliceTwoRanges(nullptr, false);
  m_uiForm.ckUseCalibration->setChecked(false);
  sliceCalib(false);
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ISISDiagnostics::~ISISDiagnostics() {
  m_propTrees["SlicePropTree"]->unsetFactoryForManager(m_dblManager);
  m_propTrees["SlicePropTree"]->unsetFactoryForManager(m_blnManager);
}

void ISISDiagnostics::handleRun() {
  QString suffix = "_" + getAnalyserName() + getReflectionName() + "_slice";
  QString filenames = m_uiForm.dsInputFiles->getFilenames().join(",");

  std::vector<int> spectraRange;
  spectraRange.emplace_back(static_cast<int>(m_dblManager->value(m_properties["SpecMin"])));
  spectraRange.emplace_back(static_cast<int>(m_dblManager->value(m_properties["SpecMax"])));

  std::vector<double> peakRange;
  peakRange.emplace_back(m_dblManager->value(m_properties["PeakStart"]));
  peakRange.emplace_back(m_dblManager->value(m_properties["PeakEnd"]));

  IAlgorithm_sptr sliceAlg = AlgorithmManager::Instance().create("TimeSlice");
  sliceAlg->initialize();

  sliceAlg->setProperty("InputFiles", filenames.toStdString());
  sliceAlg->setProperty("SpectraRange", spectraRange);
  sliceAlg->setProperty("PeakRange", peakRange);
  sliceAlg->setProperty("OutputNameSuffix", suffix.toStdString());
  sliceAlg->setProperty("OutputWorkspace", "IndirectDiagnostics_Workspaces");

  if (m_uiForm.ckUseCalibration->isChecked()) {
    QString calibWsName = m_uiForm.dsCalibration->getCurrentDataName();
    sliceAlg->setProperty("CalibrationWorkspace", calibWsName.toStdString());
  }

  if (m_blnManager->value(m_properties["UseTwoRanges"])) {
    std::vector<double> backgroundRange;
    backgroundRange.emplace_back(m_dblManager->value(m_properties["BackgroundStart"]));
    backgroundRange.emplace_back(m_dblManager->value(m_properties["BackgroundEnd"]));
    sliceAlg->setProperty("BackgroundRange", backgroundRange);
  }

  connect(m_batchAlgoRunner, &API::BatchAlgorithmRunner::batchComplete, this, &ISISDiagnostics::algorithmComplete);
  m_plotOptionsPresenter->watchADS(false);
  runAlgorithm(sliceAlg);
}

void ISISDiagnostics::handleValidation(IUserInputValidator *validator) const {
  // Check raw input
  validator->checkFileFinderWidgetIsValid("Input", m_uiForm.dsInputFiles);
  if (m_uiForm.ckUseCalibration->isChecked())
    validator->checkDataSelectorIsValid("Calibration", m_uiForm.dsCalibration);

  // Check peak range
  auto rangeOne =
      std::make_pair(m_dblManager->value(m_properties["PeakStart"]), m_dblManager->value(m_properties["PeakEnd"]));
  validator->checkValidRange("Range One", rangeOne);

  // Check background range
  bool useTwoRanges = m_blnManager->value(m_properties["UseTwoRanges"]);
  if (useTwoRanges) {
    auto rangeTwo = std::make_pair(m_dblManager->value(m_properties["BackgroundStart"]),
                                   m_dblManager->value(m_properties["BackgroundEnd"]));
    validator->checkValidRange("Range Two", rangeTwo);

    validator->checkRangesDontOverlap(rangeOne, rangeTwo);
  }

  // Check spectra range
  auto specRange =
      std::make_pair(m_dblManager->value(m_properties["SpecMin"]), m_dblManager->value(m_properties["SpecMax"]) + 1);
  validator->checkValidRange("Spectra Range", specRange);
}

/**
 * Handles completion of the algorithm.
 *
 * @param error If the algorithm failed
 */
void ISISDiagnostics::algorithmComplete(bool error) {
  m_plotOptionsPresenter->watchADS(true);
  disconnect(m_batchAlgoRunner, &API::BatchAlgorithmRunner::batchComplete, this, &ISISDiagnostics::algorithmComplete);
  m_runPresenter->setRunEnabled(true);
  m_uiForm.pbSave->setEnabled(!error);

  if (error)
    return;

  WorkspaceGroup_sptr sliceOutputGroup =
      AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("IndirectDiagnostics_Workspaces");
  if (sliceOutputGroup->size() == 0) {
    g_log.warning("No result workspaces, cannot plot preview.");
    return;
  }

  // Update the preview plots
  sliceAlgDone(false);

  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Sets default spectra, peak and background ranges.
 */
void ISISDiagnostics::updateInstrumentConfiguration() {
  try {
    setDefaultInstDetails(getInstrumentDetails());
  } catch (std::exception const &ex) {
    showMessageBox(ex.what());
  }
}

void ISISDiagnostics::setDefaultInstDetails(QMap<QString, QString> const &instrumentDetails) {
  auto const instrument = getInstrumentDetail(instrumentDetails, "instrument");
  auto const spectraMin = getInstrumentDetail(instrumentDetails, "spectra-min").toDouble();
  auto const spectraMax = getInstrumentDetail(instrumentDetails, "spectra-max").toDouble();

  // Set the search instrument for runs
  m_uiForm.dsInputFiles->setInstrumentOverride(instrument);

  // Set spectra range
  m_dblManager->setMaximum(m_properties["SpecMin"], spectraMax);
  m_dblManager->setMinimum(m_properties["SpecMax"], spectraMin);

  m_dblManager->setValue(m_properties["SpecMin"], spectraMin);
  m_dblManager->setValue(m_properties["SpecMax"], spectraMax);
  m_dblManager->setValue(m_properties["PreviewSpec"], spectraMin);
}

void ISISDiagnostics::handleNewFile() {
  if (!m_uiForm.dsInputFiles->isValid())
    return;

  QString filename = m_uiForm.dsInputFiles->getFirstFilename();

  QFileInfo fi(filename);
  QString wsname = fi.baseName();

  int specMin = static_cast<int>(m_dblManager->value(m_properties["SpecMin"]));
  int specMax = static_cast<int>(m_dblManager->value(m_properties["SpecMax"]));

  if (!loadFile(filename.toStdString(), wsname.toStdString(), specMin, specMax, SettingsHelper::loadHistory())) {
    emit showMessageBox("Unable to load file.\nCheck whether your file exists "
                        "and matches the selected instrument in the "
                        "EnergyTransfer tab.");
    return;
  }

  auto const inputWorkspace = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsname.toStdString());

  auto const previewSpec = static_cast<int>(m_dblManager->value(m_properties["PreviewSpec"])) - specMin;

  m_uiForm.ppRawPlot->clear();
  m_uiForm.ppRawPlot->addSpectrum("Raw", inputWorkspace->clone(), previewSpec);

  auto const xLimits = getXRangeFromWorkspace(inputWorkspace);
  setPeakRangeLimits(xLimits.first, xLimits.second);
  setBackgroundRangeLimits(xLimits.first, xLimits.second);

  setPeakRange(getInstrumentDetail("peak-start").toDouble(), getInstrumentDetail("peak-end").toDouble());
  setBackgroundRange(getInstrumentDetail("back-start").toDouble(), getInstrumentDetail("back-end").toDouble());

  m_uiForm.ppRawPlot->resizeX();
  m_uiForm.ppRawPlot->replot();
}

void ISISDiagnostics::setPeakRangeLimits(double peakMin, double peakMax) {
  auto slicePeak = m_uiForm.ppRawPlot->getRangeSelector("SlicePeak");
  setRangeLimits(slicePeak, peakMin, peakMax, "PeakStart", "PeakEnd");
}

void ISISDiagnostics::setBackgroundRangeLimits(double backgroundMin, double backgroundMax) {
  auto sliceBackground = m_uiForm.ppRawPlot->getRangeSelector("SliceBackground");
  setRangeLimits(sliceBackground, backgroundMin, backgroundMax, "BackgroundStart", "BackgroundEnd");
}

void ISISDiagnostics::setRangeLimits(MantidWidgets::RangeSelector *rangeSelector, double minimum, double maximum,
                                     QString const &minPropertyName, QString const &maxPropertyName) {
  setPlotPropertyRange(rangeSelector, m_properties[minPropertyName], m_properties[maxPropertyName],
                       qMakePair(minimum, maximum));
}

void ISISDiagnostics::setPeakRange(double minimum, double maximum) {
  auto slicePeak = m_uiForm.ppRawPlot->getRangeSelector("SlicePeak");
  setRangeSelector(slicePeak, m_properties["PeakStart"], m_properties["PeakEnd"], qMakePair(minimum, maximum));
}

void ISISDiagnostics::setBackgroundRange(double minimum, double maximum) {
  auto sliceBackground = m_uiForm.ppRawPlot->getRangeSelector("SliceBackground");
  setRangeSelector(sliceBackground, m_properties["BackgroundStart"], m_properties["BackgroundEnd"],
                   qMakePair(minimum, maximum));
}

/**
 * Set if the second slice range selectors should be shown on the plot
 *
 * @param state :: True to show the second range selectors, false to hide
 */
void ISISDiagnostics::sliceTwoRanges(QtProperty * /*unused*/, bool state) {
  m_uiForm.ppRawPlot->getRangeSelector("SliceBackground")->setVisible(state);
}

/**
 * Enables/disables the calibration file field and validator
 *
 * @param state :: True to enable calibration file, false otherwise
 */
void ISISDiagnostics::sliceCalib(bool state) { m_uiForm.dsCalibration->setEnabled(state); }

void ISISDiagnostics::rangeSelectorDropped(double min, double max) {
  MantidWidgets::RangeSelector *from = qobject_cast<MantidWidgets::RangeSelector *>(sender());

  disconnect(m_dblManager, &QtDoublePropertyManager::valueChanged, this, &ISISDiagnostics::doublePropertyChanged);

  if (from == m_uiForm.ppRawPlot->getRangeSelector("SlicePeak")) {
    m_dblManager->setValue(m_properties["PeakStart"], min);
    m_dblManager->setValue(m_properties["PeakEnd"], max);
  } else if (from == m_uiForm.ppRawPlot->getRangeSelector("SliceBackground")) {
    m_dblManager->setValue(m_properties["BackgroundStart"], min);
    m_dblManager->setValue(m_properties["BackgroundEnd"], max);
  }

  connect(m_dblManager, &QtDoublePropertyManager::valueChanged, this, &ISISDiagnostics::doublePropertyChanged);
}

/**
 * Handles a double property being changed in the property browser.
 *
 * @param prop :: Pointer to the QtProperty
 * @param val :: New value
 */
void ISISDiagnostics::doublePropertyChanged(QtProperty *prop, double val) {
  auto peakRangeSelector = m_uiForm.ppRawPlot->getRangeSelector("SlicePeak");
  auto backgroundRangeSelector = m_uiForm.ppRawPlot->getRangeSelector("SliceBackground");

  disconnect(m_dblManager, &QtDoublePropertyManager::valueChanged, this, &ISISDiagnostics::doublePropertyChanged);

  if (prop == m_properties["PeakStart"]) {
    setRangeSelectorMin(m_properties["PeakStart"], m_properties["PeakEnd"], peakRangeSelector, val);
  } else if (prop == m_properties["PeakEnd"]) {
    setRangeSelectorMax(m_properties["PeakStart"], m_properties["PeakEnd"], peakRangeSelector, val);
  } else if (prop == m_properties["BackgroundStart"]) {
    setRangeSelectorMin(m_properties["BackgroundStart"], m_properties["BackgroundEnd"], backgroundRangeSelector, val);
  } else if (prop == m_properties["BackgroundEnd"]) {
    setRangeSelectorMax(m_properties["BackgroundStart"], m_properties["BackgroundEnd"], backgroundRangeSelector, val);
  } else if (prop == m_properties["PreviewSpec"])
    handleNewFile();
  else if (prop == m_properties["SpecMin"]) {
    m_dblManager->setMinimum(m_properties["SpecMax"], val + 1);
    m_dblManager->setMinimum(m_properties["PreviewSpec"], val);
  } else if (prop == m_properties["SpecMax"]) {
    m_dblManager->setMaximum(m_properties["SpecMin"], val - 1);
    m_dblManager->setMaximum(m_properties["PreviewSpec"], val);
  }

  connect(m_dblManager, &QtDoublePropertyManager::valueChanged, this, &ISISDiagnostics::doublePropertyChanged);
}

/**
 * Updates the preview plot when the algorithm is complete.
 *
 * @param error True if the algorithm was stopped due to error, false otherwise
 */
void ISISDiagnostics::sliceAlgDone(bool error) {
  disconnect(m_batchAlgoRunner, &API::BatchAlgorithmRunner::batchComplete, this, &ISISDiagnostics::sliceAlgDone);

  if (error)
    return;

  QStringList filenames = m_uiForm.dsInputFiles->getFilenames();
  if (filenames.size() < 1)
    return;

  WorkspaceGroup_sptr sliceOutputGroup =
      AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("IndirectDiagnostics_Workspaces");
  if (sliceOutputGroup->size() == 0) {
    g_log.warning("No result workspaces, cannot plot preview.");
    return;
  }

  MatrixWorkspace_sptr sliceWs = std::dynamic_pointer_cast<MatrixWorkspace>(sliceOutputGroup->getItem(0));
  if (!sliceWs) {
    g_log.warning("No result workspaces, cannot plot preview.");
    return;
  }

  // Set workspace for Python export as the first result workspace
  m_pythonExportWsName = sliceWs->getName();

  setOutputPlotOptionsWorkspaces(sliceOutputGroup->getNames());

  // Plot result spectrum
  m_uiForm.ppSlicePreview->clear();
  m_uiForm.ppSlicePreview->addSpectrum("Slice", sliceWs, 0);
  m_uiForm.ppSlicePreview->resizeX();

  // Ungroup the output workspace
  sliceOutputGroup->removeAll();
  AnalysisDataService::Instance().remove("IndirectDiagnostics_Workspaces");
}

void ISISDiagnostics::setFileExtensionsByName(bool filter) {
  QStringList const noSuffices{""};
  auto const tabName("ISISDiagnostics");
  m_uiForm.dsCalibration->setFBSuffixes(filter ? getCalibrationFBSuffixes(tabName) : getCalibrationExtensions(tabName));
  m_uiForm.dsCalibration->setWSSuffixes(filter ? getCalibrationWSSuffixes(tabName) : noSuffices);
}

void ISISDiagnostics::setLoadHistory(bool doLoadHistory) {
  m_uiForm.dsCalibration->setLoadProperty("LoadHistory", doLoadHistory);
}

/**
 * Called when the FileFinder starts finding the files.
 */
void ISISDiagnostics::pbRunFinding() {
  m_runPresenter->setRunText("Finding files...");
  m_uiForm.dsInputFiles->setEnabled(false);
}

/**
 * Called when the FileFinder has finished finding the files.
 */
void ISISDiagnostics::pbRunFinished() {
  if (!m_uiForm.dsInputFiles->isValid())
    m_runPresenter->setRunText("Invalid Run(s)");
  else
    m_runPresenter->setRunEnabled(true);
  m_uiForm.dsInputFiles->setEnabled(true);
}

/**
 * Handles saving workspace
 */
void ISISDiagnostics::saveClicked() {
  if (checkADSForPlotSaveWorkspace(m_pythonExportWsName, false))
    addSaveWorkspaceToQueue(QString::fromStdString(m_pythonExportWsName));
  m_batchAlgoRunner->executeBatchAsync();
}

void ISISDiagnostics::setSaveEnabled(bool enabled) { m_uiForm.pbSave->setEnabled(enabled); }

} // namespace MantidQt::CustomInterfaces
