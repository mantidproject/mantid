#include "Stretch.h"
#include "../General/UserInputValidator.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/WorkspaceGroup.h"

using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("Stretch");
}

namespace MantidQt {
namespace CustomInterfaces {
Stretch::Stretch(QWidget *parent)
    : IndirectBayesTab(parent), m_previewSpec(0), m_save(false) {
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
  connect(m_uiForm.pbPlot, SIGNAL(clicked()), this, SLOT(plotWorkspaces()));
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(saveWorkspaces()));
  connect(m_uiForm.pbPlotPreview, SIGNAL(clicked()), this,
          SLOT(plotCurrentPreview()));
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
  const auto sampleName = m_uiForm.dsSample->getCurrentDataName().toStdString();
  const auto resName =
      m_uiForm.dsResolution->getCurrentDataName().toStdString();

  auto saveDirectory = Mantid::Kernel::ConfigService::Instance().getString(
      "defaultsave.directory");
  if (saveDirectory.compare("") == 0) {
    const char *textMessage =
        "BayesStretch requires a default save directory and "
        "one is not currently set."
        " If run, the algorithm will default to saving files "
        "to the current working directory."
        " Would you still like to run the algorithm?";
    int result = QMessageBox::question(nullptr, tr("Save Directory"),
                                       tr(textMessage), QMessageBox::Yes,
                                       QMessageBox::No, QMessageBox::NoButton);
    if (result == QMessageBox::No) {
      return;
    }
  }

  // Obtain save and plot state
  m_plotType = m_uiForm.cbPlot->currentText().toStdString();

  // Collect input from options section
  const auto background = m_uiForm.cbBackground->currentText().toStdString();

  // Collect input from the properties browser
  const auto eMin = m_properties["EMin"]->valueText().toDouble();
  const auto eMax = m_properties["EMax"]->valueText().toDouble();
  const auto beta = m_properties["Beta"]->valueText().toLong();
  const auto sigma = m_properties["Sigma"]->valueText().toLong();
  const auto nBins = m_properties["SampleBinning"]->valueText().toLong();

  // Bool options
  const auto elasticPeak = m_uiForm.chkElasticPeak->isChecked();
  const auto sequence = m_uiForm.chkSequentialFit->isChecked();

  // Construct OutputNames
  auto cutIndex = sampleName.find_last_of("_");
  auto baseName = sampleName.substr(0, cutIndex);
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

  m_uiForm.cbPlot->setEnabled(true);
  m_plotType = m_uiForm.cbPlot->currentText().toStdString();
}

/**
* Handles the saving and plotting of workspaces after execution
*/
void Stretch::algorithmComplete(const bool &error) {
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
             SLOT(algorithmComplete(bool)));

  if (error)
    return;

  // Enables plot and save
  m_uiForm.cbPlot->setEnabled(true);
  m_uiForm.pbPlot->setEnabled(true);
  m_uiForm.pbSave->setEnabled(true);
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

/**
 * Handles the plotting of workspace post algorithm completion
 */
void Stretch::plotWorkspaces() {

  WorkspaceGroup_sptr fitWorkspace;
  fitWorkspace = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
      m_fitWorkspaceName);

  auto sigma = QString::fromStdString(fitWorkspace->getItem(0)->getName());
  auto beta = QString::fromStdString(fitWorkspace->getItem(1)->getName());
  // Check Sigma and Beta workspaces exist
  if (sigma.right(5).compare("Sigma") == 0) {
    if (beta.right(4).compare("Beta") == 0) {

      // Plot Beta workspace
      QString pyInput = "from mantidplot import plot2D\n";
      if (m_plotType.compare("All") == 0 || m_plotType.compare("Beta") == 0) {
        pyInput += "importMatrixWorkspace('";
        pyInput += beta;
        pyInput += "').plotGraph2D()\n";
      }
      // Plot Sigma workspace
      if (m_plotType.compare("All") == 0 || m_plotType.compare("Sigma") == 0) {
        pyInput += "importMatrixWorkspace('";
        pyInput += sigma;
        pyInput += "').plotGraph2D()\n";
      }
      m_pythonRunner.runPythonCode(pyInput);
    }
  } else {
    g_log.error(
        "Beta and Sigma workspace were not found and could not be plotted.");
  }
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
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          filename.toStdString());
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

} // namespace CustomInterfaces
} // namespace MantidQt
