#include "MantidQtCustomInterfaces/Indirect/ContainerSubtraction.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"

#include "MantidAPI/Axis.h"

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
    absCorProps["SampleWorkspace"] = sampleWsName.toStdString();
  }

  // Construct Can input name
  QString canWsName = m_uiForm.dsContainer->getCurrentDataName();
  QString canCloneName = canWsName;
  if (shift) {
	canCloneName += "_Shifted";
  }

  // Clone can for use in algorithm
  MatrixWorkspace_sptr canWs =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          canWsName.toStdString());
  IAlgorithm_sptr clone = AlgorithmManager::Instance().create("CloneWorkspace");
  clone->initialize();
  clone->setProperty("InputWorkspace", canWs);
  clone->setProperty("Outputworkspace", canCloneName.toStdString());
  clone->execute();
  MatrixWorkspace_sptr canCloneWs =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          canCloneName.toStdString());

  if (shift) {
    IAlgorithm_sptr scaleX = AlgorithmManager::Instance().create("ScaleX");
    scaleX->initialize();
    scaleX->setProperty("InputWorkspace", canCloneWs);
    scaleX->setProperty("OutputWorkspace", canCloneName.toStdString());
    scaleX->setProperty("Factor", m_uiForm.spShift->value());
    scaleX->setProperty("Operation", "Add");
    scaleX->execute();
    IAlgorithm_sptr rebin =
        AlgorithmManager::Instance().create("RebinToWorkspace");
    rebin->initialize();
    rebin->setProperty("WorkspaceToRebin", canCloneWs);
    rebin->setProperty("WorkspaceToMatch", sampleWs);
    rebin->setProperty("OutputWorkspace", canCloneName.toStdString());
    rebin->execute();
  }

  // Check for same binning across sample and container
  if (shift) {
	  addRebinStep(canCloneName, sampleWsName);
  }
  else {
	  if (!checkWorkspaceBinningMatches(sampleWs, canCloneWs)) {
            const char *text =
                "Binning on sample and container does not match."
                "Would you like to rebin the container to match the sample?";

            int result = QMessageBox::question(
                NULL, tr("Rebin sample?"), tr(text), QMessageBox::Yes,
                QMessageBox::No, QMessageBox::NoButton);

            if (result == QMessageBox::Yes) {
              IAlgorithm_sptr rebin =
                  AlgorithmManager::Instance().create("RebinToWorkspace");
              rebin->initialize();
              rebin->setProperty("WorkspaceToRebin", canCloneWs);
              rebin->setProperty("WorkspaceToMatch", sampleWs);
              rebin->setProperty("OutputWorkspace", canCloneName.toStdString());
              rebin->execute();
                  }
		  else {
			  m_batchAlgoRunner->clearQueue();
			  g_log.error("Cannot apply absorption corrections using a sample and "
				  "container with different binning.");
			  return;
		  }
	  }
  }

  // If not in wavelength then do conversion
  std::string originalCanUnits = canCloneWs->getAxis(0)->unit()->unitID();
  if (originalCanUnits != "Wavelength") {
    g_log.information("Container workspace not in wavelength, need to "
                      "convert to continue.");
    absCorProps["CanWorkspace"] = addConvertUnitsStep(canCloneWs, "Wavelength");
  } else {
    absCorProps["CanWorkspace"] = canCloneName.toStdString();
  }

  if (scale) {
    double canScaleFactor = m_uiForm.spCanScale->value();
    applyCorrAlg->setProperty("CanScaleFactor", canScaleFactor);
  }

  // Generate output workspace name
  QString containerWsName = m_uiForm.dsContainer->getCurrentDataName();
  int sampleNameCutIndex = sampleWsName.lastIndexOf("_");
  if (sampleNameCutIndex == -1)
    sampleNameCutIndex = sampleWsName.length();

  MatrixWorkspace_sptr containerWs =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          containerWsName.toStdString());
  std::string runNum = "";
  int containerNameCutIndex = 0;
  if (containerWs->run().hasProperty("run_number")) {
    runNum = containerWs->run().getProperty("run_number")->value();
  } else {
    containerNameCutIndex = containerWsName.indexOf("_");
    if (containerNameCutIndex == -1)
      containerNameCutIndex = containerWsName.length();
  }

  QString outputWsName = sampleWsName.left(sampleNameCutIndex) + "_Subtract_";
  if (runNum.compare("") != 0) {
    outputWsName += QString::fromStdString(runNum);
  } else {
    outputWsName += containerWsName.left(containerNameCutIndex);
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
    const QString sampleName = m_uiForm.dsSample->getCurrentDataName();
    const QString sampleType =
        sampleName.right(sampleName.length() - sampleName.lastIndexOf("_"));
    const QString containerName = m_uiForm.dsContainer->getCurrentDataName();
    const QString containerType = containerName.right(
        containerName.length() - containerName.lastIndexOf("_"));

    g_log.debug() << "Sample type is: " << sampleType.toStdString()
                  << std::endl;
    g_log.debug() << "Container type is: " << containerType.toStdString()
                  << std::endl;

    if (containerType != sampleType)
      uiv.addErrorMessage(
          "Sample and can workspaces must contain the same type of data.");

    // Get Workspaces for histogram checking
    MatrixWorkspace_sptr sampleWs =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            sampleName.toStdString());
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
 * @param wsIndex workspace index to plot
 */
void ContainerSubtraction::plotPreview(int wsIndex) {
  m_uiForm.ppPreview->clear();

  // Plot sample
  m_uiForm.ppPreview->addSpectrum(
      "Sample", m_uiForm.dsSample->getCurrentDataName(), wsIndex, Qt::black);

  // Plot result
  if (!m_pythonExportWsName.empty())
    m_uiForm.ppPreview->addSpectrum(
        "Subtracted", QString::fromStdString(m_pythonExportWsName), wsIndex,
        Qt::green);

  const bool shift = m_uiForm.ckShiftCan->isChecked();
  const bool scale = m_uiForm.ckScaleCan->isChecked();

 //Scale can
  if (scale) {
    auto canName = m_uiForm.dsContainer->getCurrentDataName();
    if (shift) {
      canName += "_Shifted";
    }
    IAlgorithm_sptr scaleCan = AlgorithmManager::Instance().create("Scale");
    scaleCan->initialize();
    scaleCan->setProperty("InputWorkspace", canName.toStdString());
    scaleCan->setProperty("OutputWorkspace", "__container_corrected");
    scaleCan->setProperty("Factor", m_uiForm.spCanScale->value());
    scaleCan->setProperty("Operation", "Multiply");
    scaleCan->execute();
  }

  // Plot container
  if (scale) {
    m_uiForm.ppPreview->addSpectrum("Container", "__container_corrected",
      wsIndex, Qt::red);
  } else {
    if (shift) {
      m_uiForm.ppPreview->addSpectrum(
          "Container",
          (m_uiForm.dsContainer->getCurrentDataName() + "_Shifted"), wsIndex,
          Qt::red);
    } else {
      m_uiForm.ppPreview->addSpectrum(
          "Container", m_uiForm.dsContainer->getCurrentDataName(), wsIndex,
          Qt::red);
    }
  }
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

  // Run algorithm queue
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(postProcessComplete(bool)));
  m_batchAlgoRunner->executeBatchAsync();
}

} // namespace CustomInterfaces
} // namespace MantidQt