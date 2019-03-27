// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ISISDiagnostics.h"

#include "../General/UserInputValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/Logger.h"

#include <QFileInfo>

using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("ISISDiagnostics");
}

namespace MantidQt {
namespace CustomInterfaces {
//----------------------------------------------------------------------------------------------
/** Constructor
 */
ISISDiagnostics::ISISDiagnostics(IndirectDataReduction *idrUI, QWidget *parent)
    : IndirectDataReductionTab(idrUI, parent) {
  m_uiForm.setupUi(parent);

  // Property Tree
  m_propTrees["SlicePropTree"] = new QtTreePropertyBrowser();
  m_uiForm.properties->addWidget(m_propTrees["SlicePropTree"]);

  // Editor Factories
  DoubleEditorFactory *doubleEditorFactory = new DoubleEditorFactory();
  QtCheckBoxFactory *checkboxFactory = new QtCheckBoxFactory();
  m_propTrees["SlicePropTree"]->setFactoryForManager(m_dblManager,
                                                     doubleEditorFactory);
  m_propTrees["SlicePropTree"]->setFactoryForManager(m_blnManager,
                                                     checkboxFactory);

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
  m_properties["BackgroundRange"]->addSubProperty(
      m_properties["BackgroundStart"]);
  m_properties["BackgroundRange"]->addSubProperty(
      m_properties["BackgroundEnd"]);

  m_propTrees["SlicePropTree"]->addProperty(m_properties["PreviewSpec"]);
  m_propTrees["SlicePropTree"]->addProperty(m_properties["SpecMin"]);
  m_propTrees["SlicePropTree"]->addProperty(m_properties["SpecMax"]);
  m_propTrees["SlicePropTree"]->addProperty(m_properties["PeakRange"]);
  m_propTrees["SlicePropTree"]->addProperty(m_properties["UseTwoRanges"]);
  m_propTrees["SlicePropTree"]->addProperty(m_properties["BackgroundRange"]);

  // Slice plot
  auto peakRangeSelector = m_uiForm.ppRawPlot->addRangeSelector("SlicePeak");
  auto backgroundRangeSelector =
      m_uiForm.ppRawPlot->addRangeSelector("SliceBackground");

  // Setup second range
  backgroundRangeSelector->setColour(
      Qt::darkGreen); // Dark green for background
  backgroundRangeSelector->setRange(peakRangeSelector->getRange());

  // SIGNAL/SLOT CONNECTIONS

  // Update instrument information when a new instrument config is selected
  connect(this, SIGNAL(newInstrumentConfiguration()), this,
          SLOT(setDefaultInstDetails()));

  // Update properties when a range selector is changed
  connect(peakRangeSelector, SIGNAL(selectionChangedLazy(double, double)), this,
          SLOT(rangeSelectorDropped(double, double)));
  connect(backgroundRangeSelector, SIGNAL(selectionChangedLazy(double, double)),
          this, SLOT(rangeSelectorDropped(double, double)));

  // Update range selctors when a property is changed
  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
          SLOT(doublePropertyChanged(QtProperty *, double)));
  // Enable/disable second range options when checkbox is toggled
  connect(m_blnManager, SIGNAL(valueChanged(QtProperty *, bool)), this,
          SLOT(sliceTwoRanges(QtProperty *, bool)));
  // Enables/disables calibration file selection when user toggles Use
  // Calibratin File checkbox
  connect(m_uiForm.ckUseCalibration, SIGNAL(toggled(bool)), this,
          SLOT(sliceCalib(bool)));

  // Plot slice miniplot when file has finished loading
  connect(m_uiForm.dsInputFiles, SIGNAL(filesFoundChanged()), this,
          SLOT(handleNewFile()));
  // Shows message on run buton when user is inputting a run number
  connect(m_uiForm.dsInputFiles, SIGNAL(fileTextChanged(const QString &)), this,
          SLOT(pbRunEditing()));
  // Shows message on run button when Mantid is finding the file for a given run
  // number
  connect(m_uiForm.dsInputFiles, SIGNAL(findingFiles()), this,
          SLOT(pbRunFinding()));
  // Reverts run button back to normal when file finding has finished
  connect(m_uiForm.dsInputFiles, SIGNAL(fileFindingFinished()), this,
          SLOT(pbRunFinished()));
  // Handles running, plotting and saving
  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));
  connect(m_uiForm.pbPlot, SIGNAL(clicked()), this, SLOT(plotClicked()));
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(saveClicked()));

  connect(this,
          SIGNAL(updateRunButton(bool, std::string const &, QString const &,
                                 QString const &)),
          this,
          SLOT(updateRunButton(bool, std::string const &, QString const &,
                               QString const &)));

  // Set default UI state
  sliceTwoRanges(nullptr, false);
  m_uiForm.ckUseCalibration->setChecked(false);
  sliceCalib(false);
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ISISDiagnostics::~ISISDiagnostics() {}

void ISISDiagnostics::setup() {}

void ISISDiagnostics::run() {
  QString suffix = "_" + getAnalyserName() + getReflectionName() + "_slice";
  QString filenames = m_uiForm.dsInputFiles->getFilenames().join(",");

  std::vector<long> spectraRange;
  spectraRange.push_back(
      static_cast<long>(m_dblManager->value(m_properties["SpecMin"])));
  spectraRange.push_back(
      static_cast<long>(m_dblManager->value(m_properties["SpecMax"])));

  std::vector<double> peakRange;
  peakRange.push_back(m_dblManager->value(m_properties["PeakStart"]));
  peakRange.push_back(m_dblManager->value(m_properties["PeakEnd"]));

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
    backgroundRange.push_back(
        m_dblManager->value(m_properties["BackgroundStart"]));
    backgroundRange.push_back(
        m_dblManager->value(m_properties["BackgroundEnd"]));
    sliceAlg->setProperty("BackgroundRange", backgroundRange);
  }

  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(algorithmComplete(bool)));
  runAlgorithm(sliceAlg);
}

bool ISISDiagnostics::validate() {
  UserInputValidator uiv;

  // Check raw input
  uiv.checkMWRunFilesIsValid("Input", m_uiForm.dsInputFiles);
  if (m_uiForm.ckUseCalibration->isChecked())
    uiv.checkMWRunFilesIsValid("Calibration", m_uiForm.dsInputFiles);

  // Check peak range
  auto rangeOne = std::make_pair(m_dblManager->value(m_properties["PeakStart"]),
                                 m_dblManager->value(m_properties["PeakEnd"]));
  uiv.checkValidRange("Range One", rangeOne);

  // Check background range
  bool useTwoRanges = m_blnManager->value(m_properties["UseTwoRanges"]);
  if (useTwoRanges) {
    auto rangeTwo =
        std::make_pair(m_dblManager->value(m_properties["BackgroundStart"]),
                       m_dblManager->value(m_properties["BackgroundEnd"]));
    uiv.checkValidRange("Range Two", rangeTwo);

    uiv.checkRangesDontOverlap(rangeOne, rangeTwo);
  }

  // Check spectra range
  auto specRange =
      std::make_pair(m_dblManager->value(m_properties["SpecMin"]),
                     m_dblManager->value(m_properties["SpecMax"]) + 1);
  uiv.checkValidRange("Spectra Range", specRange);

  QString error = uiv.generateErrorMessage();
  bool isError = error != "";

  if (isError)
    g_log.warning(error.toStdString());

  return !isError;
}

/**
 * Handles completion of the algorithm.
 *
 * @param error If the algorithm failed
 */
void ISISDiagnostics::algorithmComplete(bool error) {
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
             SLOT(algorithmComplete(bool)));

  if (error)
    return;

  WorkspaceGroup_sptr sliceOutputGroup =
      AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
          "IndirectDiagnostics_Workspaces");
  if (sliceOutputGroup->size() == 0) {
    g_log.warning("No result workspaces, cannot plot preview.");
    return;
  }

  for (size_t i = 0; i < sliceOutputGroup->size(); i++) {
    QString wsName =
        QString::fromStdString(sliceOutputGroup->getItem(i)->getName());
  }
  // Enable plot and save buttons
  m_uiForm.pbSave->setEnabled(true);
  m_uiForm.pbPlot->setEnabled(true);

  // Update the preview plots
  sliceAlgDone(false);

  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Sets default spectra, peak and background ranges.
 */
void ISISDiagnostics::setDefaultInstDetails() {
  try {
    setDefaultInstDetails(getInstrumentDetails());
  } catch (std::exception const &ex) {
    g_log.warning(ex.what());
  }
}

void ISISDiagnostics::setDefaultInstDetails(
    QMap<QString, QString> const &instrumentDetails) {
  // Set the search instrument for runs
  m_uiForm.dsInputFiles->setInstrumentOverride(
      getInstrumentDetail(instrumentDetails, "instrument"));

  auto const specMin =
      getInstrumentDetail(instrumentDetails, "spectra-min").toDouble();
  auto const specMax =
      getInstrumentDetail(instrumentDetails, "spectra-max").toDouble();

  // Set spectra range
  m_dblManager->setMaximum(m_properties["SpecMin"], specMax);
  m_dblManager->setMinimum(m_properties["SpecMax"], specMin);

  m_dblManager->setValue(m_properties["SpecMin"], specMin);
  m_dblManager->setValue(m_properties["SpecMax"], specMax);
  m_dblManager->setValue(m_properties["PreviewSpec"], specMin);

  // Set peak and background ranges
  if (instrumentDetails.size() >= 8) {
    auto const peakStart = getInstrumentDetail(instrumentDetails, "peak-start");
    auto const peakEnd = getInstrumentDetail(instrumentDetails, "peak-end");
    auto const backStart = getInstrumentDetail(instrumentDetails, "back-start");
    auto const backEnd = getInstrumentDetail(instrumentDetails, "back-end");

    setRangeSelector(m_uiForm.ppRawPlot->getRangeSelector("SlicePeak"),
                     m_properties["PeakStart"], m_properties["PeakEnd"],
                     qMakePair(peakStart.toDouble(), peakEnd.toDouble()));

    setRangeSelector(m_uiForm.ppRawPlot->getRangeSelector("SliceBackground"),
                     m_properties["BackgroundStart"],
                     m_properties["BackgroundEnd"],
                     qMakePair(backStart.toDouble(), backEnd.toDouble()));
  }
}

void ISISDiagnostics::handleNewFile() {
  if (!m_uiForm.dsInputFiles->isValid())
    return;

  QString filename = m_uiForm.dsInputFiles->getFirstFilename();

  QFileInfo fi(filename);
  QString wsname = fi.baseName();

  int specMin = static_cast<int>(m_dblManager->value(m_properties["SpecMin"]));
  int specMax = static_cast<int>(m_dblManager->value(m_properties["SpecMax"]));

  if (!loadFile(filename, wsname, specMin, specMax)) {
    emit showMessageBox("Unable to load file.\nCheck whether your file exists "
                        "and matches the selected instrument in the "
                        "EnergyTransfer tab.");
    return;
  }

  Mantid::API::MatrixWorkspace_sptr input =
      boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
          Mantid::API::AnalysisDataService::Instance().retrieve(
              wsname.toStdString()));

  const auto &dataX = input->x(0);
  QPair<double, double> range(dataX.front(), dataX.back());
  int previewSpec =
      static_cast<int>(m_dblManager->value(m_properties["PreviewSpec"])) -
      specMin;

  m_uiForm.ppRawPlot->clear();
  m_uiForm.ppRawPlot->addSpectrum("Raw", input, previewSpec);

  setPlotPropertyRange(m_uiForm.ppRawPlot->getRangeSelector("SlicePeak"),
                       m_properties["PeakStart"], m_properties["PeakEnd"],
                       range);
  setPlotPropertyRange(m_uiForm.ppRawPlot->getRangeSelector("SliceBackground"),
                       m_properties["BackgroundStart"],
                       m_properties["BackgroundEnd"], range);

  m_uiForm.ppRawPlot->resizeX();
}

/**
 * Set if the second slice range selectors should be shown on the plot
 *
 * @param state :: True to show the second range selectors, false to hide
 */
void ISISDiagnostics::sliceTwoRanges(QtProperty *, bool state) {
  m_uiForm.ppRawPlot->getRangeSelector("SliceBackground")->setVisible(state);
}

/**
 * Enables/disables the calibration file field and validator
 *
 * @param state :: True to enable calibration file, false otherwise
 */
void ISISDiagnostics::sliceCalib(bool state) {
  m_uiForm.dsCalibration->setEnabled(state);
}

void ISISDiagnostics::rangeSelectorDropped(double min, double max) {
  MantidWidgets::RangeSelector *from =
      qobject_cast<MantidWidgets::RangeSelector *>(sender());

  if (from == m_uiForm.ppRawPlot->getRangeSelector("SlicePeak")) {
    m_dblManager->setValue(m_properties["PeakStart"], min);
    m_dblManager->setValue(m_properties["PeakEnd"], max);
  } else if (from == m_uiForm.ppRawPlot->getRangeSelector("SliceBackground")) {
    m_dblManager->setValue(m_properties["BackgroundStart"], min);
    m_dblManager->setValue(m_properties["BackgroundEnd"], max);
  }
}

/**
 * Handles a double property being changed in the property browser.
 *
 * @param prop :: Pointer to the QtProperty
 * @param val :: New value
 */
void ISISDiagnostics::doublePropertyChanged(QtProperty *prop, double val) {
  auto peakRangeSelector = m_uiForm.ppRawPlot->getRangeSelector("SlicePeak");
  auto backgroundRangeSelector =
      m_uiForm.ppRawPlot->getRangeSelector("SliceBackground");

  if (prop == m_properties["PeakStart"])
    peakRangeSelector->setMinimum(val);
  else if (prop == m_properties["PeakEnd"])
    peakRangeSelector->setMaximum(val);
  else if (prop == m_properties["BackgroundStart"])
    backgroundRangeSelector->setMinimum(val);
  else if (prop == m_properties["BackgroundEnd"])
    backgroundRangeSelector->setMaximum(val);
  else if (prop == m_properties["PreviewSpec"])
    handleNewFile();
  else if (prop == m_properties["SpecMin"]) {
    m_dblManager->setMinimum(m_properties["SpecMax"], val + 1);
    m_dblManager->setMinimum(m_properties["PreviewSpec"], val);
  } else if (prop == m_properties["SpecMax"]) {
    m_dblManager->setMaximum(m_properties["SpecMin"], val - 1);
    m_dblManager->setMaximum(m_properties["PreviewSpec"], val);
  }
}

/**
 * Updates the preview plot when the algorithm is complete.
 *
 * @param error True if the algorithm was stopped due to error, false otherwise
 */
void ISISDiagnostics::sliceAlgDone(bool error) {
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
             SLOT(sliceAlgDone(bool)));

  if (error)
    return;

  QStringList filenames = m_uiForm.dsInputFiles->getFilenames();
  if (filenames.size() < 1)
    return;

  WorkspaceGroup_sptr sliceOutputGroup =
      AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
          "IndirectDiagnostics_Workspaces");
  if (sliceOutputGroup->size() == 0) {
    g_log.warning("No result workspaces, cannot plot preview.");
    return;
  }

  MatrixWorkspace_sptr sliceWs = boost::dynamic_pointer_cast<MatrixWorkspace>(
      sliceOutputGroup->getItem(0));
  if (!sliceWs) {
    g_log.warning("No result workspaces, cannot plot preview.");
    return;
  }

  // Set workspace for Python export as the first result workspace
  m_pythonExportWsName = sliceWs->getName();

  // Plot result spectrum
  m_uiForm.ppSlicePreview->clear();
  m_uiForm.ppSlicePreview->addSpectrum("Slice", sliceWs, 0);
  m_uiForm.ppSlicePreview->resizeX();

  // Ungroup the output workspace
  sliceOutputGroup->removeAll();
  AnalysisDataService::Instance().remove("IndirectDiagnostics_Workspaces");
}

/**
 * Called when a user starts to type / edit the runs to load.
 */
void ISISDiagnostics::pbRunEditing() {
  updateRunButton(false, "unchanged", "Editing...",
                  "Run numbers are curently being edited.");
}

/**
 * Called when the FileFinder starts finding the files.
 */
void ISISDiagnostics::pbRunFinding() {
  updateRunButton(false, "unchanged", "Finding files...",
                  "Searchig for data files for the run numbers entered...");
  m_uiForm.dsInputFiles->setEnabled(false);
}

/**
 * Called when the FileFinder has finished finding the files.
 */
void ISISDiagnostics::pbRunFinished() {
  if (!m_uiForm.dsInputFiles->isValid())
    updateRunButton(
        false, "unchanged", "Invalid Run(s)",
        "Cannot find data files for some of the run numbers enetered.");
  else
    updateRunButton();

  m_uiForm.dsInputFiles->setEnabled(true);
}

/**
 * Handle when Run is clicked
 */
void ISISDiagnostics::runClicked() { runTab(); }

/**
 * Handles mantid plotting
 */
void ISISDiagnostics::plotClicked() {
  setPlotIsPlotting(true);
  if (checkADSForPlotSaveWorkspace(m_pythonExportWsName, true))
    plotSpectrum(QString::fromStdString(m_pythonExportWsName));
  setPlotIsPlotting(false);
}

/**
 * Handles saving workspace
 */
void ISISDiagnostics::saveClicked() {
  if (checkADSForPlotSaveWorkspace(m_pythonExportWsName, false))
    addSaveWorkspaceToQueue(QString::fromStdString(m_pythonExportWsName));
  m_batchAlgoRunner->executeBatchAsync();
}

void ISISDiagnostics::setRunEnabled(bool enabled) {
  m_uiForm.pbRun->setEnabled(enabled);
}

void ISISDiagnostics::setPlotEnabled(bool enabled) {
  m_uiForm.pbPlot->setEnabled(enabled);
}

void ISISDiagnostics::setSaveEnabled(bool enabled) {
  m_uiForm.pbSave->setEnabled(enabled);
}

void ISISDiagnostics::setOutputButtonsEnabled(
    std::string const &enableOutputButtons) {
  bool enable = enableOutputButtons == "enable" ? true : false;
  setPlotEnabled(enable);
  setSaveEnabled(enable);
}

void ISISDiagnostics::updateRunButton(bool enabled,
                                      std::string const &enableOutputButtons,
                                      QString const message,
                                      QString const tooltip) {
  setRunEnabled(enabled);
  m_uiForm.pbRun->setText(message);
  m_uiForm.pbRun->setToolTip(tooltip);
  if (enableOutputButtons != "unchanged")
    setOutputButtonsEnabled(enableOutputButtons);
}

void ISISDiagnostics::setPlotIsPlotting(bool plotting) {
  m_uiForm.pbPlot->setText(plotting ? "Plotting..." : "Plot Result");
  setPlotEnabled(!plotting);
  setRunEnabled(!plotting);
  setSaveEnabled(!plotting);
}

} // namespace CustomInterfaces
} // namespace MantidQt
