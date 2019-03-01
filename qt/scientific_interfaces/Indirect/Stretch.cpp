// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "Stretch.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/WorkspaceGroup.h"

using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("Stretch");

bool doesExistInADS(std::string const &workspaceName) {
  return AnalysisDataService::Instance().doesExist(workspaceName);
}

WorkspaceGroup_sptr getADSWorkspaceGroup(std::string const &workspaceName) {
  return AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
      workspaceName);
}

MatrixWorkspace_sptr getADSMatrixWorkspace(std::string const &workspaceName) {
  return AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
      workspaceName);
}

} // namespace

namespace MantidQt {
namespace CustomInterfaces {
Stretch::Stretch(QWidget *parent)
    : IndirectBayesTab(parent), m_sampleFBExtensions({"_red.nxs", "_sqw.nxs"}),
      m_sampleWSExtensions({"_red", "_sqw"}),
      m_resolutionFBExtensions({"_res.nxs"}),
      m_resolutionWSExtensions({"_res"}), m_previewSpec(0), m_save(false) {
  m_uiForm.setupUi(parent);

  // Create range selector
  auto eRangeSelector = m_uiForm.ppPlot->addRangeSelector("StretchERange");
  connect(eRangeSelector, SIGNAL(minValueChanged(double)), this,
          SLOT(minValueChanged(double)));
  connect(eRangeSelector, SIGNAL(maxValueChanged(double)), this,
          SLOT(maxValueChanged(double)));

  // Add the properties browser to the ui form
  m_uiForm.treeSpace->addWidget(m_propTree);

  m_properties["EMin"] = m_dblManager->addProperty("EMin");
  m_properties["EMax"] = m_dblManager->addProperty("EMax");
  m_properties["SampleBinning"] = m_dblManager->addProperty("Sample Binning");
  m_properties["Sigma"] = m_dblManager->addProperty("Sigma");
  m_properties["Beta"] = m_dblManager->addProperty("Beta");

  m_dblManager->setDecimals(m_properties["EMin"], NUM_DECIMALS);
  m_dblManager->setDecimals(m_properties["EMax"], NUM_DECIMALS);
  m_dblManager->setDecimals(m_properties["SampleBinning"], INT_DECIMALS);
  m_dblManager->setDecimals(m_properties["Sigma"], INT_DECIMALS);
  m_dblManager->setDecimals(m_properties["Beta"], INT_DECIMALS);

  m_propTree->addProperty(m_properties["EMin"]);
  m_propTree->addProperty(m_properties["EMax"]);
  m_propTree->addProperty(m_properties["SampleBinning"]);
  m_propTree->addProperty(m_properties["Sigma"]);
  m_propTree->addProperty(m_properties["Beta"]);

  formatTreeWidget(m_propTree, m_properties);

  // default values
  m_dblManager->setValue(m_properties["Sigma"], 50);
  m_dblManager->setMinimum(m_properties["Sigma"], 1);
  m_dblManager->setMaximum(m_properties["Sigma"], 200);
  m_dblManager->setValue(m_properties["Beta"], 50);
  m_dblManager->setMinimum(m_properties["Beta"], 1);
  m_dblManager->setMaximum(m_properties["Beta"], 200);
  m_dblManager->setValue(m_properties["SampleBinning"], 1);
  m_dblManager->setMinimum(m_properties["SampleBinning"], 1);

  // Connect the data selector for the sample to the mini plot
  connect(m_uiForm.dsSample, SIGNAL(dataReady(const QString &)), this,
          SLOT(handleSampleInputReady(const QString &)));
  connect(m_uiForm.chkSequentialFit, SIGNAL(toggled(bool)), m_uiForm.cbPlot,
          SLOT(setEnabled(bool)));
  // Connect preview spectrum spinner to handler
  connect(m_uiForm.spPreviewSpectrum, SIGNAL(valueChanged(int)), this,
          SLOT(previewSpecChanged(int)));
  m_uiForm.spPreviewSpectrum->setMaximum(0);

  // Connect the plot and save push buttons
  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));
  connect(m_uiForm.pbPlot, SIGNAL(clicked()), this, SLOT(plotWorkspaces()));
  connect(m_uiForm.pbPlotContour, SIGNAL(clicked()), this,
          SLOT(plotContourClicked()));
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(saveWorkspaces()));
  connect(m_uiForm.pbPlotPreview, SIGNAL(clicked()), this,
          SLOT(plotCurrentPreview()));
}

void Stretch::setFileExtensionsByName(bool filter) {
  QStringList const noSuffixes{""};
  m_uiForm.dsSample->setFBSuffixes(filter ? m_sampleFBExtensions
                                          : getAllowedExtensions());
  m_uiForm.dsSample->setWSSuffixes(filter ? m_sampleWSExtensions : noSuffixes);
  m_uiForm.dsResolution->setFBSuffixes(filter ? m_resolutionFBExtensions
                                              : getAllowedExtensions());
  m_uiForm.dsResolution->setWSSuffixes(filter ? m_resolutionWSExtensions
                                              : noSuffixes);
}

void Stretch::setup() {}

/**
 * Validate the form to check the program can be run
 *
 * @return :: Whether the form was valid
 */
bool Stretch::validate() {
  UserInputValidator uiv;
  uiv.checkDataSelectorIsValid("Sample", m_uiForm.dsSample);
  uiv.checkDataSelectorIsValid("Resolution", m_uiForm.dsResolution);

  QString errors = uiv.generateErrorMessage();
  if (!errors.isEmpty()) {
    emit showMessageBox(errors);
    return false;
  }

  return true;
}

/**
 * Collect the settings on the GUI and build a python
 * script that runs Stretch
 */
void Stretch::run() {

  // Workspace input
  auto const sampleName = m_uiForm.dsSample->getCurrentDataName().toStdString();
  auto const resName =
      m_uiForm.dsResolution->getCurrentDataName().toStdString();

  // Collect input from options section
  auto const background = m_uiForm.cbBackground->currentText().toStdString();

  // Collect input from the properties browser
  auto const eMin = m_properties["EMin"]->valueText().toDouble();
  auto const eMax = m_properties["EMax"]->valueText().toDouble();
  auto const beta = m_properties["Beta"]->valueText().toLong();
  auto const sigma = m_properties["Sigma"]->valueText().toLong();
  auto const nBins = m_properties["SampleBinning"]->valueText().toLong();

  // Bool options
  auto const elasticPeak = m_uiForm.chkElasticPeak->isChecked();
  auto const sequence = m_uiForm.chkSequentialFit->isChecked();

  // Construct OutputNames
  auto const cutIndex = sampleName.find_last_of("_");
  auto const baseName = sampleName.substr(0, cutIndex);
  m_fitWorkspaceName = baseName + "_Stretch_Fit";
  m_contourWorkspaceName = baseName + "_Stretch_Contour";

  auto stretch = AlgorithmManager::Instance().create("BayesStretch");
  stretch->initialize();
  stretch->setProperty("SampleWorkspace", sampleName);
  stretch->setProperty("ResolutionWorkspace", resName);
  stretch->setProperty("EMin", eMin);
  stretch->setProperty("EMax", eMax);
  stretch->setProperty("SampleBins", nBins);
  stretch->setProperty("Elastic", elasticPeak);
  stretch->setProperty("Background", background);
  stretch->setProperty("NumberSigma", sigma);
  stretch->setProperty("NumberBeta", beta);
  stretch->setProperty("Loop", sequence);
  stretch->setProperty("OutputWorkspaceFit", m_fitWorkspaceName);
  stretch->setProperty("OutputWorkspaceContour", m_contourWorkspaceName);

  m_batchAlgoRunner->addAlgorithm(stretch);
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(algorithmComplete(bool)));
  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Handles the saving and plotting of workspaces after execution
 */
void Stretch::algorithmComplete(const bool &error) {
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
             SLOT(algorithmComplete(bool)));

  setRunIsRunning(false);
  if (error) {
    setPlotResultEnabled(false);
    setPlotContourEnabled(false);
    setSaveResultEnabled(false);
  } else {
    if (doesExistInADS(m_contourWorkspaceName))
      populateContourWorkspaceComboBox();
    else
      setPlotContourEnabled(false);
  }
}

void Stretch::populateContourWorkspaceComboBox() {
  m_uiForm.cbPlotContour->clear();
  auto const contourGroup = getADSWorkspaceGroup(m_contourWorkspaceName);
  auto const contourNames = contourGroup->getNames();
  for (auto const &name : contourNames)
    m_uiForm.cbPlotContour->addItem(QString::fromStdString(name));
}

/**
 * Handles the saving of workspaces post algorithm completion
 * when save button is clicked
 */
void Stretch::saveWorkspaces() {

  auto fitWorkspace = QString::fromStdString(m_fitWorkspaceName);
  auto contourWorkspace = QString::fromStdString(m_contourWorkspaceName);
  // Check workspaces exist
  IndirectTab::checkADSForPlotSaveWorkspace(m_fitWorkspaceName, false);
  IndirectTab::checkADSForPlotSaveWorkspace(m_contourWorkspaceName, false);

  auto saveDir = QString::fromStdString(
      Mantid::Kernel::ConfigService::Instance().getString(
          "defaultsave.directory"));
  // Check validity of save path
  const auto fitFullPath = saveDir.append(fitWorkspace).append(".nxs");
  const auto contourFullPath = saveDir.append(contourWorkspace).append(".nxs");
  addSaveWorkspaceToQueue(fitWorkspace, fitFullPath);
  addSaveWorkspaceToQueue(contourWorkspace, fitFullPath);
  m_batchAlgoRunner->executeBatchAsync();
}

void Stretch::runClicked() {
  if (validateTab()) {
    auto const saveDirectory =
        Mantid::Kernel::ConfigService::Instance().getString(
            "defaultsave.directory");
    displayMessageAndRun(saveDirectory);
  }
}

void Stretch::displayMessageAndRun(std::string const &saveDirectory) {
  if (saveDirectory.empty()) {
    int const result = displaySaveDirectoryMessage();
    if (result != QMessageBox::No) {
      setRunIsRunning(true);
      runTab();
    }
  } else {
    setRunIsRunning(true);
    runTab();
  }
}

int Stretch::displaySaveDirectoryMessage() {
  char const *textMessage =
      "BayesStretch requires a default save directory and "
      "one is not currently set."
      " If run, the algorithm will default to saving files "
      "to the current working directory."
      " Would you still like to run the algorithm?";
  return QMessageBox::question(nullptr, tr("Save Directory"), tr(textMessage),
                               QMessageBox::Yes, QMessageBox::No,
                               QMessageBox::NoButton);
}

/**
 * Handles the plotting of workspace post algorithm completion
 */
void Stretch::plotWorkspaces() {
  setPlotResultIsPlotting(true);
  WorkspaceGroup_sptr fitWorkspace;
  fitWorkspace = getADSWorkspaceGroup(m_fitWorkspaceName);

  auto sigma = QString::fromStdString(fitWorkspace->getItem(0)->getName());
  auto beta = QString::fromStdString(fitWorkspace->getItem(1)->getName());
  // Check Sigma and Beta workspaces exist
  if (sigma.right(5).compare("Sigma") == 0 &&
      beta.right(4).compare("Beta") == 0) {
    QString pyInput = "from mantidplot import plot2D\n";

    std::string const plotType = m_uiForm.cbPlot->currentText().toStdString();
    if (plotType == "All" || plotType == "Beta") {
      pyInput += "importMatrixWorkspace('";
      pyInput += beta;
      pyInput += "').plotGraph2D()\n";
    }
    if (plotType == "All" || plotType == "Sigma") {
      pyInput += "importMatrixWorkspace('";
      pyInput += sigma;
      pyInput += "').plotGraph2D()\n";
    }

    m_pythonRunner.runPythonCode(pyInput);
  } else {
    g_log.error(
        "Beta and Sigma workspace were not found and could not be plotted.");
  }
  setPlotResultIsPlotting(false);
}

void Stretch::plotContourClicked() {
  setPlotContourIsPlotting(true);

  auto const workspaceName = m_uiForm.cbPlotContour->currentText();
  if (checkADSForPlotSaveWorkspace(workspaceName.toStdString(), true)) {
    QString pyInput = "from mantidplot import plot2D\nimportMatrixWorkspace('" +
                      workspaceName + "').plotGraph2D()\n";
    m_pythonRunner.runPythonCode(pyInput);
  }
  setPlotContourIsPlotting(false);
}

/**
 * Set the data selectors to use the default save directory
 * when browsing for input files.
 *
 * @param settings :: The current settings
 */
void Stretch::loadSettings(const QSettings &settings) {
  m_uiForm.dsSample->readSettings(settings.group());
  m_uiForm.dsResolution->readSettings(settings.group());
}

/**
 * Plots the loaded file to the miniplot and sets the guides
 * and the range
 *
 * @param filename :: The name of the workspace to plot
 */
void Stretch::handleSampleInputReady(const QString &filename) {
  m_uiForm.ppPlot->addSpectrum("Sample", filename, 0);
  // update the maximum and minimum range bar positions
  QPair<double, double> range = m_uiForm.ppPlot->getCurveRange("Sample");
  auto eRangeSelector = m_uiForm.ppPlot->getRangeSelector("StretchERange");
  setRangeSelector(eRangeSelector, m_properties["EMin"], m_properties["EMax"],
                   range);
  setPlotPropertyRange(eRangeSelector, m_properties["EMin"],
                       m_properties["EMax"], range);
  // update the current positions of the range bars
  eRangeSelector->setMinimum(range.first);
  eRangeSelector->setMaximum(range.second);

  // set the max spectrum
  MatrixWorkspace_const_sptr sampleWs =
      getADSMatrixWorkspace(filename.toStdString());
  const int spectra = static_cast<int>(sampleWs->getNumberHistograms());
  m_uiForm.spPreviewSpectrum->setMaximum(spectra - 1);
}

/**
 * Sets a new preview spectrum for the mini plot.
 *
 * @param value workspace index
 */
void Stretch::previewSpecChanged(int value) {
  m_previewSpec = value;

  if (!m_uiForm.dsSample->isValid())
    return;

  m_uiForm.ppPlot->clear();

  QString sampleName = m_uiForm.dsSample->getCurrentDataName();
  m_uiForm.ppPlot->addSpectrum("Sample", sampleName, m_previewSpec);
}

/**
 * plots the current miniplot preview
 */
void Stretch::plotCurrentPreview() {
  if (m_uiForm.ppPlot->hasCurve("Sample")) {
    plotSpectrum(m_uiForm.dsSample->getCurrentDataName(), m_previewSpec);
  }
}

/**
 * Updates the property manager when the lower guide is moved on the mini plot
 *
 * @param min :: The new value of the lower guide
 */
void Stretch::minValueChanged(double min) {
  m_dblManager->setValue(m_properties["EMin"], min);
}

/**
 * Updates the property manager when the upper guide is moved on the mini plot
 *
 * @param max :: The new value of the upper guide
 */
void Stretch::maxValueChanged(double max) {
  m_dblManager->setValue(m_properties["EMax"], max);
}

/**
 * Handles when properties in the property manager are updated.
 *
 * @param prop :: The property being updated
 * @param val :: The new value for the property
 */
void Stretch::updateProperties(QtProperty *prop, double val) {
  UNUSED_ARG(val);

  auto eRangeSelector = m_uiForm.ppPlot->getRangeSelector("StretchERange");

  if (prop == m_properties["EMin"] || prop == m_properties["EMax"]) {
    auto bounds = qMakePair(m_dblManager->value(m_properties["EMin"]),
                            m_dblManager->value(m_properties["EMax"]));
    setRangeSelector(eRangeSelector, m_properties["EMin"], m_properties["EMax"],
                     bounds);
  }
}

void Stretch::setRunEnabled(bool enabled) {
  m_uiForm.pbRun->setEnabled(enabled);
}

void Stretch::setPlotResultEnabled(bool enabled) {
  m_uiForm.pbPlot->setEnabled(enabled);
  m_uiForm.cbPlot->setEnabled(enabled);
}

void Stretch::setPlotContourEnabled(bool enabled) {
  m_uiForm.pbPlotContour->setEnabled(enabled);
  m_uiForm.cbPlotContour->setEnabled(enabled);
}

void Stretch::setSaveResultEnabled(bool enabled) {
  m_uiForm.pbSave->setEnabled(enabled);
}

void Stretch::setButtonsEnabled(bool enabled) {
  setRunEnabled(enabled);
  setPlotResultEnabled(enabled);
  setPlotContourEnabled(enabled);
  setSaveResultEnabled(enabled);
}

void Stretch::setRunIsRunning(bool running) {
  m_uiForm.pbRun->setText(running ? "Running..." : "Run");
  setButtonsEnabled(!running);
}

void Stretch::setPlotResultIsPlotting(bool plotting) {
  m_uiForm.pbPlot->setText(plotting ? "Plotting..." : "Plot");
  setButtonsEnabled(!plotting);
}

void Stretch::setPlotContourIsPlotting(bool plotting) {
  m_uiForm.pbPlotContour->setText(plotting ? "Plotting..." : "Plot Contour");
  setButtonsEnabled(!plotting);
}

} // namespace CustomInterfaces
} // namespace MantidQt
