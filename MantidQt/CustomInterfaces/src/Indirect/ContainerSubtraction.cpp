#include "MantidQtCustomInterfaces/Indirect/ContainerSubtraction.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/Unit.h"

using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("ContainerSubtraction");
}

namespace MantidQt {
namespace CustomInterfaces {
ContainerSubtraction::ContainerSubtraction(QWidget *parent)
    : CorrectionsTab(parent) {
  m_uiForm.setupUi(parent);

  // Connect slots
  connect(m_uiForm.dsSample, SIGNAL(dataReady(const QString &)), this,
          SLOT(newSample(const QString &)));
  connect(m_uiForm.dsContainer, SIGNAL(dataReady(const QString &)), this,
          SLOT(newContainer(const QString &)));
  connect(m_uiForm.spPreviewSpec, SIGNAL(valueChanged(int)), this,
          SLOT(plotPreview(int)));
  connect(m_uiForm.spCanScale, SIGNAL(valueChanged(double)), this,
          SLOT(updateCan()));
  connect(m_uiForm.spShift, SIGNAL(valueChanged(double)), this,
          SLOT(updateCan()));
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(saveClicked()));
  connect(m_uiForm.pbPlot, SIGNAL(clicked()), this, SLOT(plotClicked()));

  m_uiForm.spPreviewSpec->setMinimum(0);
  m_uiForm.spPreviewSpec->setMaximum(0);
}

void ContainerSubtraction::setup() {}

void ContainerSubtraction::run() {
  API::BatchAlgorithmRunner::AlgorithmRuntimeProps absCorProps;
  IAlgorithm_sptr applyCorrAlg =
      AlgorithmManager::Instance().create("ApplyPaalmanPingsCorrection");
  applyCorrAlg->initialize();
  MatrixWorkspace_sptr sampleWs =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          m_sampleWorkspaceName);
  m_originalSampleUnits = sampleWs->getAxis(0)->unit()->unitID();

  // Check if using shift / scale
  const bool shift = m_uiForm.ckShiftCan->isChecked();
  const bool scale = m_uiForm.ckScaleCan->isChecked();

  // If not in wavelength then do conversion
  if (m_originalSampleUnits != "Wavelength") {
    g_log.information(
        "Sample workspace not in wavelength, need to convert to continue.");
    absCorProps["SampleWorkspace"] =
        addConvertUnitsStep(sampleWs, "Wavelength");
  } else {
    absCorProps["SampleWorkspace"] = m_sampleWorkspaceName;
  }

  const auto canName = m_uiForm.dsContainer->getCurrentDataName().toStdString();
  const auto cloneName = "__algorithm_can";

  IAlgorithm_sptr cloneAlg =
      AlgorithmManager::Instance().create("CloneWorkspace");
  cloneAlg->initialize();
  cloneAlg->setLogging(false);
  cloneAlg->setProperty("InputWorkspace", canName);
  cloneAlg->setProperty("OutputWorkspace", cloneName);
  cloneAlg->execute();

  MatrixWorkspace_sptr canWs =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(cloneName);

  // Check for same binning across sample and container
  if (shift) {

    IAlgorithm_sptr scaleX = AlgorithmManager::Instance().create("ScaleX");
    scaleX->initialize();
    scaleX->setLogging(false);
    scaleX->setProperty("InputWorkspace", canWs);
    scaleX->setProperty("Factor", m_uiForm.spShift->value());
    scaleX->setProperty("Operation", "Add");
    scaleX->setProperty("OutputWorkspace", cloneName);
    scaleX->execute();

    IAlgorithm_sptr rebin =
        AlgorithmManager::Instance().create("RebinToWorkspace");
    rebin->initialize();
    rebin->setLogging(false);
    rebin->setProperty("WorkspaceToRebin", cloneName);
    rebin->setProperty("WorkspaceToMatch", m_sampleWorkspaceName);
    rebin->setProperty("OutputWorkspace", cloneName);
    rebin->execute();

  } else {
    if (!checkWorkspaceBinningMatches(sampleWs, canWs)) {
      const char *text =
          "Binning on sample and container does not match."
          "Would you like to rebin the container to match the sample?";

      int result = QMessageBox::question(NULL, tr("Rebin sample?"), tr(text),
                                         QMessageBox::Yes, QMessageBox::No,
                                         QMessageBox::NoButton);

      if (result == QMessageBox::Yes) {
        IAlgorithm_sptr rebin =
            AlgorithmManager::Instance().create("RebinToWorkspace");
        rebin->initialize();
        rebin->setProperty("WorkspaceToRebin", canWs);
        rebin->setProperty("WorkspaceToMatch", sampleWs);
        rebin->setProperty("OutputWorkspace", cloneName);
        rebin->execute();
      } else {
        m_batchAlgoRunner->clearQueue();
        g_log.error("Cannot apply absorption corrections using a sample and "
                    "container with different binning.");
        return;
      }
    }
  }

  // If not in wavelength then do conversion
  std::string originalCanUnits = canWs->getAxis(0)->unit()->unitID();
  if (originalCanUnits != "Wavelength") {
    g_log.information("Container workspace not in wavelength, need to "
                      "convert to continue.");
    absCorProps["CanWorkspace"] = addConvertUnitsStep(canWs, "Wavelength");
  } else {
    absCorProps["CanWorkspace"] = cloneName;
  }

  if (scale) {
    double canScaleFactor = m_uiForm.spCanScale->value();
    applyCorrAlg->setProperty("CanScaleFactor", canScaleFactor);
  }

  // Generate output workspace name
  QString QStrContainerWs = QString::fromStdString(m_containerWorkspaceName);
  auto QStrSampleWs = QString::fromStdString(m_sampleWorkspaceName);
  int sampleNameCutIndex = QStrSampleWs.lastIndexOf("_");
  if (sampleNameCutIndex == -1)
    sampleNameCutIndex = QStrSampleWs.length();

  MatrixWorkspace_sptr containerWs =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          m_containerWorkspaceName);
  std::string runNum = "";
  int containerNameCutIndex = 0;
  if (containerWs->run().hasProperty("run_number")) {
    runNum = containerWs->run().getProperty("run_number")->value();
  } else {
    containerNameCutIndex = QStrContainerWs.indexOf("_");
    if (containerNameCutIndex == -1)
      containerNameCutIndex = QStrContainerWs.length();
  }

  QString outputWsName = QStrSampleWs.left(sampleNameCutIndex) + "_Subtract_";
  if (runNum.compare("") != 0) {
    outputWsName += QString::fromStdString(runNum);
  } else {
    outputWsName += QStrContainerWs.left(containerNameCutIndex);
  }

  outputWsName += "_red";

  applyCorrAlg->setProperty("OutputWorkspace", outputWsName.toStdString());

  // Add corrections algorithm to queue
  m_batchAlgoRunner->addAlgorithm(applyCorrAlg, absCorProps);

  // Run algorithm queue
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(absCorComplete(bool)));
  m_batchAlgoRunner->executeBatchAsync();

  // Set the result workspace for Python script export
  m_pythonExportWsName = outputWsName.toStdString();
}

/**
 * Adds a rebin to workspace step to the calculation for when using a sample and
 *container that
 * have different binning.
 *
 * @param toRebin
 * @param toMatch
 */
void ContainerSubtraction::addRebinStep(QString toRebin, QString toMatch) {
  API::BatchAlgorithmRunner::AlgorithmRuntimeProps rebinProps;
  rebinProps["WorkspaceToMatch"] = toMatch.toStdString();

  IAlgorithm_sptr rebinAlg =
      AlgorithmManager::Instance().create("RebinToWorkspace");
  rebinAlg->initialize();

  rebinAlg->setProperty("WorkspaceToRebin", toRebin.toStdString());
  rebinAlg->setProperty("OutputWorkspace", toRebin.toStdString());

  m_batchAlgoRunner->addAlgorithm(rebinAlg, rebinProps);
}

/**
 * Validates the user input in the UI
 * @return if the input was valid
 */
bool ContainerSubtraction::validate() {
  UserInputValidator uiv;

  // Check valid inputs
  const bool samValid =
      uiv.checkDataSelectorIsValid("Sample", m_uiForm.dsSample);
  const bool canValid =
      uiv.checkDataSelectorIsValid("Container", m_uiForm.dsContainer);

  if (samValid && canValid) {
    // Check Sample is of same type as container (e.g. _red/_sqw)
    const auto QStrSampleName = QString::fromStdString(m_sampleWorkspaceName);
    const auto sampleType = QStrSampleName.right(
        QStrSampleName.length() - QStrSampleName.lastIndexOf("_"));
    const QString containerName = m_uiForm.dsContainer->getCurrentDataName();
    const QString containerType = containerName.right(
        containerName.length() - containerName.lastIndexOf("_"));

    g_log.debug() << "Sample type is: " << sampleType.toStdString() << '\n';
    g_log.debug() << "Container type is: " << containerType.toStdString()
                  << '\n';

    if (containerType != sampleType)
      uiv.addErrorMessage(
          "Sample and can workspaces must contain the same type of data.");

    // Get Workspaces for histogram checking
    MatrixWorkspace_sptr sampleWs =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            m_sampleWorkspaceName);
    MatrixWorkspace_sptr containerWs =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            containerName.toStdString());

    // Check sample has the same number of Histograms as the contianer
    const size_t sampleHist = sampleWs->getNumberHistograms();
    const size_t containerHist = containerWs->getNumberHistograms();

    if (sampleHist != containerHist) {
      uiv.addErrorMessage(
          " Sample and Container do not have a matching number of Histograms.");
    }
  }

  // Show errors if there are any
  if (!uiv.isAllInputValid())
    emit showMessageBox(uiv.generateErrorMessage());

  return uiv.isAllInputValid();
}

void ContainerSubtraction::loadSettings(const QSettings &settings) {
  m_uiForm.dsContainer->readSettings(settings.group());
  m_uiForm.dsSample->readSettings(settings.group());
}

/**
 * Displays the sample data on the plot preview
 * @param dataName Name of new data source
 */
void ContainerSubtraction::newSample(const QString &dataName) {
  // Remove old sample and fit
  m_uiForm.ppPreview->removeSpectrum("Subtracted");
  m_uiForm.ppPreview->removeSpectrum("Sample");

  // Get new workspace
  const MatrixWorkspace_sptr sampleWs =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          dataName.toStdString());
  m_uiForm.spPreviewSpec->setMaximum(
      static_cast<int>(sampleWs->getNumberHistograms()) - 1);

  // Plot the sample curve
  m_uiForm.ppPreview->addSpectrum("Sample", sampleWs, 0, Qt::black);
  m_sampleWorkspaceName = dataName.toStdString();

  // Set min/max container shift
  auto min = sampleWs->getXMin();
  auto max = sampleWs->getXMax();

  m_uiForm.spShift->setMinimum(min);
  m_uiForm.spShift->setMaximum(max);
}

/**
* Displays the container data on the plot preview
* @param dataName Name of new data source
*/
void ContainerSubtraction::newContainer(const QString &dataName) {
  // Remove old container and fit
  m_uiForm.ppPreview->removeSpectrum("Subtracted");
  m_uiForm.ppPreview->removeSpectrum("Container");

  // Get new workspace
  const MatrixWorkspace_sptr containerWs =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          dataName.toStdString());

  // Clone container for use in preprocessing
  IAlgorithm_sptr clone = AlgorithmManager::Instance().create("CloneWorkspace");
  clone->initialize();
  clone->setProperty("InputWorkspace", containerWs);
  clone->setProperty("Outputworkspace", "__processed_can");
  clone->execute();
  m_containerWorkspaceName = "__processed_can";

  // Plot new container
  m_uiForm.ppPreview->addSpectrum("Container", containerWs, 0, Qt::red);
  m_containerWorkspaceName = "__processed_can";
}

/**
 * Handles Container curve in the miniplot when scale or shift is updated
 */
void ContainerSubtraction::updateCan() {
  if (m_uiForm.dsContainer->getCurrentDataName().compare("") != 0) {
    IAlgorithm_sptr scale = AlgorithmManager::Instance().create("Scale");
    scale->initialize();
    scale->setLogging(false);
    scale->setProperty(
        "InputWorkspace",
        m_uiForm.dsContainer->getCurrentDataName().toStdString());
    scale->setProperty("OutputWorkspace", m_containerWorkspaceName);
    scale->setProperty("Operation", "Multiply");
    scale->setProperty("Factor", m_uiForm.spCanScale->value());
    scale->execute();
    IAlgorithm_sptr scaleX = AlgorithmManager::Instance().create("ScaleX");
    scaleX->initialize();
    scaleX->setLogging(false);
    scaleX->setProperty("InputWorkspace", m_containerWorkspaceName);
    scaleX->setProperty("OutputWorkspace", m_containerWorkspaceName);
    scaleX->setProperty("Factor", m_uiForm.spShift->value());
    scaleX->setProperty("Operation", "Add");
    scaleX->execute();
  }
  if (m_sampleWorkspaceName.compare("") != 0) {
    IAlgorithm_sptr rebin =
        AlgorithmManager::Instance().create("RebinToWorkspace");
    rebin->initialize();
    rebin->setLogging(false);
    rebin->setProperty("WorkspaceToRebin", m_containerWorkspaceName);
    rebin->setProperty("WorkspaceToMatch", m_sampleWorkspaceName);
    rebin->setProperty("OutputWorkspace", m_containerWorkspaceName);
    rebin->execute();
  }
  plotPreview(m_uiForm.spPreviewSpec->value());
}

/**
 * Replots the preview plot.
 *
 * @param wsIndex workspace index to plot
 */
void ContainerSubtraction::plotPreview(int wsIndex) {
  m_uiForm.ppPreview->clear();

  // Plot sample
  m_uiForm.ppPreview->addSpectrum("Sample",
                                  QString::fromStdString(m_sampleWorkspaceName),
                                  wsIndex, Qt::black);

  // Plot result
  if (!m_pythonExportWsName.empty())
    m_uiForm.ppPreview->addSpectrum(
        "Subtracted", QString::fromStdString(m_pythonExportWsName), wsIndex,
        Qt::green);

  m_uiForm.ppPreview->addSpectrum(
      "Container", QString::fromStdString(m_containerWorkspaceName), wsIndex,
      Qt::red);
}

void ContainerSubtraction::postProcessComplete(bool error) {
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
             SLOT(postProcessComplete(bool)));

  if (error) {
    emit showMessageBox("Unable to process corrected workspace.\nSee Results "
                        "Log for more details.");
    return;
  }

  // Handle preview plot
  plotPreview(m_uiForm.spPreviewSpec->value());

  // Clean up unwanted workspaces
  IAlgorithm_sptr deleteAlg =
      AlgorithmManager::Instance().create("DeleteWorkspace");
  deleteAlg->initialize();
  deleteAlg->setProperty("Workspace", "__algorithm_can");
  deleteAlg->execute();
  const auto conv =
      AnalysisDataService::Instance().doesExist("__algorithm_can_Wavelength");
  if (conv) {
    deleteAlg->setProperty("Workspace", "__algorithm_can_Wavelength");
    deleteAlg->execute();
  }
}

/**
 * Handles completion of the abs. correction algorithm.
 *
 * @param error True if algorithm failed.
 */
void ContainerSubtraction::absCorComplete(bool error) {
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
             SLOT(absCorComplete(bool)));

  if (error) {
    emit showMessageBox(
        "Unable to apply corrections.\nSee Results Log for more details.");
    return;
  }

  // Convert back to original sample units
  if (m_originalSampleUnits != "Wavelength") {
    auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        m_pythonExportWsName);
    std::string eMode("");
    if (m_originalSampleUnits == "dSpacing")
      eMode = "Elastic";
    addConvertUnitsStep(ws, m_originalSampleUnits, "", eMode);
  }

  if (m_uiForm.ckShiftCan->isChecked()) {
    IAlgorithm_sptr shiftLog =
        AlgorithmManager::Instance().create("AddSampleLog");
    shiftLog->initialize();

    shiftLog->setProperty("Workspace", m_pythonExportWsName);
    shiftLog->setProperty("LogName", "container_shift");
    shiftLog->setProperty("LogType", "Number");
    shiftLog->setProperty(
        "LogText", boost::lexical_cast<std::string>(m_uiForm.spShift->value()));
    m_batchAlgoRunner->addAlgorithm(shiftLog);
  }
  // Enable post process plotting and saving
  m_uiForm.cbPlotOutput->setEnabled(true);
  m_uiForm.pbPlot->setEnabled(true);
  m_uiForm.pbSave->setEnabled(true);

  // Run algorithm queue
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(postProcessComplete(bool)));
  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Handles saving of workspace
 */
void ContainerSubtraction::saveClicked() {

  // Check workspace exists
  if (checkADSForPlotSaveWorkspace(m_pythonExportWsName, false))
    addSaveWorkspaceToQueue(QString::fromStdString(m_pythonExportWsName));
  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Handles Mantid plotting of workspace
 */
void ContainerSubtraction::plotClicked() {
  QString plotType = m_uiForm.cbPlotOutput->currentText();

  if (checkADSForPlotSaveWorkspace(m_pythonExportWsName, true)) {

    if (plotType == "Spectra" || plotType == "Both")
      plotSpectrum(QString::fromStdString(m_pythonExportWsName));

    if (plotType == "Contour" || plotType == "Both")
      plot2D(QString::fromStdString(m_pythonExportWsName));
  }
}

} // namespace CustomInterfaces
} // namespace MantidQt
