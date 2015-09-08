#include "MantidQtCustomInterfaces/Indirect/ContainerSubtraction.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"

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
          SLOT(newData(const QString &)));
  connect(m_uiForm.spPreviewSpec, SIGNAL(valueChanged(int)), this,
          SLOT(plotPreview(int)));

  m_uiForm.spPreviewSpec->setMinimum(0);
  m_uiForm.spPreviewSpec->setMaximum(0);
}

void ContainerSubtraction::setup() {}

void ContainerSubtraction::run() {
  API::BatchAlgorithmRunner::AlgorithmRuntimeProps absCorProps;
  IAlgorithm_sptr applyCorrAlg =
      AlgorithmManager::Instance().create("ApplyPaalmanPingsCorrection");
  applyCorrAlg->initialize();
  QString sampleWsName = m_uiForm.dsSample->getCurrentDataName();
  MatrixWorkspace_sptr sampleWs =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          sampleWsName.toStdString());
  m_originalSampleUnits = sampleWs->getAxis(0)->unit()->unitID();

  // If not in wavelength then do conversion
  if (m_originalSampleUnits != "Wavelength") {
    g_log.information(
        "Sample workspace not in wavelength, need to convert to continue.");
    absCorProps["SampleWorkspace"] =
        addConvertUnitsStep(sampleWs, "Wavelength");
  } else {
    absCorProps["SampleWorkspace"] = sampleWsName.toStdString();
  }

  QString canWsName = m_uiForm.dsContainer->getCurrentDataName();
  MatrixWorkspace_sptr canWs =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          canWsName.toStdString());

  // If not in wavelength then do conversion
  std::string originalCanUnits = canWs->getAxis(0)->unit()->unitID();
  if (originalCanUnits != "Wavelength") {
    g_log.information("Container workspace not in wavelength, need to "
                      "convert to continue.");
    absCorProps["CanWorkspace"] = addConvertUnitsStep(canWs, "Wavelength");
  } else {
    absCorProps["CanWorkspace"] = canWsName.toStdString();
  }

  bool useCanScale = m_uiForm.ckScaleCan->isChecked();
  if (useCanScale) {
    double canScaleFactor = m_uiForm.spCanScale->value();
    applyCorrAlg->setProperty("CanScaleFactor", canScaleFactor);
  }

  // Check for same binning across sample and container
  if (!checkWorkspaceBinningMatches(sampleWs, canWs)) {
    QString text = "Binning on sample and container does not match."
                   "Would you like to rebin the sample to match the container?";

    int result = QMessageBox::question(NULL, tr("Rebin sample?"), tr(text),
                                       QMessageBox::Yes, QMessageBox::No,
                                       QMessageBox::NoButton);

    if (result == QMessageBox::Yes) {
      addRebinStep(sampleWsName, canWsName);
    } else {
      m_batchAlgoRunner->clearQueue();
      g_log.error("Cannot apply absorption corrections using a sample and "
                  "container with different binning.");
      return;
    }
  }

  // Generate output workspace name
  QString containerWsName = m_uiForm.dsContainer->getCurrentDataName();
  int sampleNameCutIndex = sampleWsName.lastIndexOf("_");
  if (sampleNameCutIndex == -1)
    sampleNameCutIndex = sampleWsName.length();
  int containerNameCutIndex = containerWsName.indexOf("_");
  if (containerNameCutIndex == -1)
    containerNameCutIndex = containerWsName.length();

  const QString outputWsName = sampleWsName.left(sampleNameCutIndex) +
                               "_Subtract_" +
                               containerWsName.left(containerNameCutIndex);

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
  uiv.checkDataSelectorIsValid("Sample", m_uiForm.dsSample);
  uiv.checkDataSelectorIsValid("Container", m_uiForm.dsContainer);
  MatrixWorkspace_sptr sampleWs;
  QString sample = m_uiForm.dsSample->getCurrentDataName();
  QString sampleType = sample.right(sample.length() - sample.lastIndexOf("_"));
  QString container = m_uiForm.dsContainer->getCurrentDataName();
  QString containerType =
      container.right(sample.length() - container.lastIndexOf("_"));

  g_log.debug() << "Sample type is: " << sampleType.toStdString() << std::endl;
  g_log.debug() << "Container type is: " << containerType.toStdString()
                << std::endl;

  if (containerType != sampleType)
    uiv.addErrorMessage(
        "Sample and can workspaces must contain the same type of data.");

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
void ContainerSubtraction::newData(const QString &dataName) {
  const MatrixWorkspace_sptr sampleWs =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          dataName.toStdString());
  m_uiForm.spPreviewSpec->setMaximum(
      static_cast<int>(sampleWs->getNumberHistograms()) - 1);

  // Plot the sample curve
  m_uiForm.ppPreview->clear();
  m_uiForm.ppPreview->addSpectrum("Sample", sampleWs, 0, Qt::black);
}

/**
 * Replots the preview plot.
 *
 * @param specIndex Spectrum index to plot
 */
void ContainerSubtraction::plotPreview(int specIndex) {
  m_uiForm.ppPreview->clear();

  // Plot sample
  m_uiForm.ppPreview->addSpectrum(
      "Sample", m_uiForm.dsSample->getCurrentDataName(), specIndex, Qt::black);

  // Plot result
  if (!m_pythonExportWsName.empty())
    m_uiForm.ppPreview->addSpectrum(
        "Subtracted", QString::fromStdString(m_pythonExportWsName), specIndex,
        Qt::green);

  // Plot container
  m_uiForm.ppPreview->addSpectrum("Container",
                                  m_uiForm.dsContainer->getCurrentDataName(),
                                  specIndex, Qt::red);
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

  // Handle Mantid plotting
  QString plotType = m_uiForm.cbPlotOutput->currentText();

  if (plotType == "Spectra" || plotType == "Both")
    plotSpectrum(QString::fromStdString(m_pythonExportWsName));

  if (plotType == "Contour" || plotType == "Both")
    plot2D(QString::fromStdString(m_pythonExportWsName));
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

  // Add save algorithms if required
  bool save = m_uiForm.ckSave->isChecked();
  if (save)
    addSaveWorkspaceToQueue(QString::fromStdString(m_pythonExportWsName));

  // Run algorithm queue
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(postProcessComplete(bool)));
  m_batchAlgoRunner->executeBatchAsync();
}

} // namespace CustomInterfaces
} // namespace MantidQt