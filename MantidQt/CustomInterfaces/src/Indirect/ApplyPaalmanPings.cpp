
#include "MantidQtCustomInterfaces/Indirect/ApplyPaalmanPings.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/TextAxis.h"

#include <QStringList>

using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("ApplyPaalmanPings");
}

namespace MantidQt {
namespace CustomInterfaces {
ApplyPaalmanPings::ApplyPaalmanPings(QWidget *parent) : CorrectionsTab(parent) {
  m_uiForm.setupUi(parent);

  connect(m_uiForm.cbGeometry, SIGNAL(currentIndexChanged(int)), this,
          SLOT(handleGeometryChange(int)));
  connect(m_uiForm.dsSample, SIGNAL(dataReady(const QString &)), this,
          SLOT(newData(const QString &)));
  connect(m_uiForm.spPreviewSpec, SIGNAL(valueChanged(int)), this,
          SLOT(plotPreview(int)));

  m_uiForm.spPreviewSpec->setMinimum(0);
  m_uiForm.spPreviewSpec->setMaximum(0);
}

void ApplyPaalmanPings::setup() {}

/**
 * Disables corrections when using S(Q, w) as input data.
 *
 * @param dataName Name of new data source
 */
void ApplyPaalmanPings::newData(const QString &dataName) {
  const MatrixWorkspace_sptr sampleWs =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          dataName.toStdString());
  m_uiForm.spPreviewSpec->setMaximum(
      static_cast<int>(sampleWs->getNumberHistograms()) - 1);

  // Plot the sample curve
  m_uiForm.ppPreview->clear();
  m_uiForm.ppPreview->addSpectrum("Sample", sampleWs, 0, Qt::black);
}

void ApplyPaalmanPings::run() {
  // Create / Initialize algorithm
  API::BatchAlgorithmRunner::AlgorithmRuntimeProps absCorProps;
  IAlgorithm_sptr applyCorrAlg =
      AlgorithmManager::Instance().create("ApplyPaalmanPingsCorrection");
  applyCorrAlg->initialize();

  // get Sample Workspace
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

  const bool useCan = m_uiForm.ckUseCan->isChecked();
  const bool useShift = m_uiForm.ckShiftCan->isChecked();
  const bool useCorrections = m_uiForm.ckUseCorrections->isChecked();
  // Get Can and Clone
  if (useCan) {
    QString canWsName = m_uiForm.dsContainer->getCurrentDataName();
    MatrixWorkspace_sptr canWs =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            canWsName.toStdString());
    QString canCloneName = canWsName + "_Shifted";
    IAlgorithm_sptr clone =
        AlgorithmManager::Instance().create("CloneWorkspace");
    clone->initialize();
    clone->setProperty("InputWorkspace", canWs);
    clone->setProperty("Outputworkspace", canCloneName.toStdString());
    clone->execute();
    MatrixWorkspace_sptr canCloneWs =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            canCloneName.toStdString());

    IAlgorithm_sptr scaleX = AlgorithmManager::Instance().create("ScaleX");
    scaleX->initialize();
    scaleX->setProperty("InputWorkspace", canCloneWs);
    scaleX->setProperty("OutputWorkspace", canCloneName.toStdString());
    scaleX->setProperty("Factor", m_uiForm.spCanShift->value());
    scaleX->setProperty("Operation", "Add");
    scaleX->execute();
    IAlgorithm_sptr rebin =
        AlgorithmManager::Instance().create("RebinToWorkspace");
    rebin->initialize();
    rebin->setProperty("WorkspaceToRebin", canCloneWs);
    rebin->setProperty("WorkspaceToMatch", sampleWs);
    rebin->setProperty("OutputWorkspace", canCloneName.toStdString());
    rebin->execute();

    // If not in wavelength then do conversion
    std::string originalCanUnits = canCloneWs->getAxis(0)->unit()->unitID();
    if (originalCanUnits != "Wavelength") {
      g_log.information("Container workspace not in wavelength, need to "
                        "convert to continue.");
      absCorProps["CanWorkspace"] =
          addConvertUnitsStep(canCloneWs, "Wavelength");
    } else {
      absCorProps["CanWorkspace"] = canWsName.toStdString();
    }

    bool useCanScale = m_uiForm.ckScaleCan->isChecked();
    if (useCanScale) {
      double canScaleFactor = m_uiForm.spCanScale->value();
      applyCorrAlg->setProperty("CanScaleFactor", canScaleFactor);
    }

    if (useShift) {
      addRebinStep(canCloneName, sampleWsName);
    } else {
      // Check for same binning across sample and container
      if (!checkWorkspaceBinningMatches(sampleWs, canCloneWs)) {
        const char *text =
            "Binning on sample and container does not match."
            "Would you like to rebin the container to match the sample?";

        int result = QMessageBox::question(NULL, tr("Rebin sample?"), tr(text),
                                           QMessageBox::Yes, QMessageBox::No,
                                           QMessageBox::NoButton);

        if (result == QMessageBox::Yes) {
          addRebinStep(canCloneName, sampleWsName);
        } else {
          m_batchAlgoRunner->clearQueue();
          g_log.error("Cannot apply absorption corrections using a sample and "
                      "container with different binning.");
          return;
        }
      }
    }
  }

  if (useCorrections) {
    QString correctionsWsName = m_uiForm.dsCorrections->getCurrentDataName();

    WorkspaceGroup_sptr corrections =
        AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
            correctionsWsName.toStdString());
    bool interpolateAll = false;
    for (size_t i = 0; i < corrections->size(); i++) {
      MatrixWorkspace_sptr factorWs =
          boost::dynamic_pointer_cast<MatrixWorkspace>(corrections->getItem(i));

      // Check for matching binning
      if (sampleWs && (sampleWs->blocksize() != factorWs->blocksize())) {
        int result;
        if (interpolateAll) {
          result = QMessageBox::Yes;
        } else {
          std::string text = "Number of bins on sample and " +
                             factorWs->name() + " workspace does not match.\n" +
                             "Would you like to interpolate this workspace to "
                             "match the sample?";

          result = QMessageBox::question(
              NULL, tr("Interpolate corrections?"), tr(text.c_str()),
              QMessageBox::YesToAll, QMessageBox::Yes, QMessageBox::No);
        }

        switch (result) {
        case QMessageBox::YesToAll:
          interpolateAll = true;
        // fall through
        case QMessageBox::Yes:
          addInterpolationStep(factorWs, absCorProps["SampleWorkspace"]);
          break;
        default:
          m_batchAlgoRunner->clearQueue();
          g_log.error("ApplyPaalmanPings cannot run with corrections that do "
                      "not match sample binning.");
          return;
        }
      }
    }

    applyCorrAlg->setProperty("CorrectionsWorkspace",
                              correctionsWsName.toStdString());
  }

  // Generate output workspace name
  int nameCutIndex = sampleWsName.lastIndexOf("_");
  if (nameCutIndex == -1)
    nameCutIndex = sampleWsName.length();

  QString correctionType;
  switch (m_uiForm.cbGeometry->currentIndex()) {
  case 0:
    correctionType = "flt";
    break;
  case 1:
    correctionType = "cyl";
    break;
  case 2:
    correctionType = "anl";
    break;
  }
  QString outputWsName = sampleWsName.left(nameCutIndex);

  // Using corrections
  if (m_uiForm.ckUseCorrections->isChecked()) {
    outputWsName += "_" + correctionType + "_Corrected";
  } else {
    outputWsName += "_Subtracted";
  }

  // Using container
  if (m_uiForm.ckUseCan->isChecked()) {
    auto containerWsName = m_uiForm.dsContainer->getCurrentDataName();
    MatrixWorkspace_sptr containerWs =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            containerWsName.toStdString());
    auto run = containerWs->run();
    if (run.hasProperty("run_number")) {
      outputWsName +=
          "_" + QString::fromStdString(run.getProperty("run_number")->value());
    } else {
      auto canCutIndex = containerWsName.indexOf("_");
      outputWsName += "_" + containerWsName.left(canCutIndex);
    }
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
void ApplyPaalmanPings::addRebinStep(QString toRebin, QString toMatch) {
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
 * Adds a spline interpolation as a step in the calculation for using legacy
 *correction factor
 * workspaces.
 *
 * @param toInterpolate Pointer to the workspace to interpolate
 * @param toMatch Name of the workspace to match
 */
void ApplyPaalmanPings::addInterpolationStep(MatrixWorkspace_sptr toInterpolate,
                                             std::string toMatch) {
  API::BatchAlgorithmRunner::AlgorithmRuntimeProps interpolationProps;
  interpolationProps["WorkspaceToMatch"] = toMatch;

  IAlgorithm_sptr interpolationAlg =
      AlgorithmManager::Instance().create("SplineInterpolation");
  interpolationAlg->initialize();

  interpolationAlg->setProperty("WorkspaceToInterpolate",
                                toInterpolate->name());
  interpolationAlg->setProperty("OutputWorkspace", toInterpolate->name());

  m_batchAlgoRunner->addAlgorithm(interpolationAlg, interpolationProps);
}

/**
 * Handles completion of the abs. correction algorithm.
 *
 * @param error True if algorithm failed.
 */
void ApplyPaalmanPings::absCorComplete(bool error) {
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
             SLOT(absCorComplete(bool)));
  const bool useCan = m_uiForm.ckUseCan->isChecked();
  const bool useShift = m_uiForm.ckShiftCan->isChecked();
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
  if (useCan) {
    if (useShift) {
      IAlgorithm_sptr shiftLog =
          AlgorithmManager::Instance().create("AddSampleLog");
      shiftLog->initialize();

      shiftLog->setProperty("Workspace", m_pythonExportWsName);
      shiftLog->setProperty("LogName", "container_shift");
      shiftLog->setProperty("LogType", "Number");
      shiftLog->setProperty("LogText", boost::lexical_cast<std::string>(
                                           m_uiForm.spCanShift->value()));
      m_batchAlgoRunner->addAlgorithm(shiftLog);
    }
  }
  // Run algorithm queue
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(postProcessComplete(bool)));
  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Handles completion of the unit conversion and saving algorithm.
 *
 * @param error True if algorithm failed.
 */
void ApplyPaalmanPings::postProcessComplete(bool error) {
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

bool ApplyPaalmanPings::validate() {
  UserInputValidator uiv;

  uiv.checkDataSelectorIsValid("Sample", m_uiForm.dsSample);

  MatrixWorkspace_sptr sampleWs;

  bool useCan = m_uiForm.ckUseCan->isChecked();
  bool useCorrections = m_uiForm.ckUseCorrections->isChecked();

  if (!(useCan || useCorrections))
    uiv.addErrorMessage("Must use either container subtraction or corrections");

  if (useCan) {
    uiv.checkDataSelectorIsValid("Container", m_uiForm.dsContainer);

    // Check can and sample workspaces are the same "type" (reduced or S(Q, w))
    QString sample = m_uiForm.dsSample->getCurrentDataName();
    QString sampleType =
        sample.right(sample.length() - sample.lastIndexOf("_"));
    QString container = m_uiForm.dsContainer->getCurrentDataName();
    QString containerType =
        container.right(container.length() - container.lastIndexOf("_"));

    g_log.debug() << "Sample type is: " << sampleType.toStdString()
                  << std::endl;
    g_log.debug() << "Can type is: " << containerType.toStdString()
                  << std::endl;

    if (containerType != sampleType)
      uiv.addErrorMessage(
          "Sample and can workspaces must contain the same type of data.");
  }

  if (useCorrections) {
    if (m_uiForm.dsCorrections->getCurrentDataName().compare("") == 0) {
      uiv.addErrorMessage(
          "Use Correction must contain a corrections file or workspace.");
    } else {

      QString correctionsWsName = m_uiForm.dsCorrections->getCurrentDataName();
      WorkspaceGroup_sptr corrections =
          AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
              correctionsWsName.toStdString());
      for (size_t i = 0; i < corrections->size(); i++) {
        // Check it is a MatrixWorkspace
        MatrixWorkspace_sptr factorWs =
            boost::dynamic_pointer_cast<MatrixWorkspace>(
                corrections->getItem(i));
        if (!factorWs) {
          QString msg = "Correction factor workspace " + QString::number(i) +
                        " is not a MatrixWorkspace";
          uiv.addErrorMessage(msg);
          continue;
        }

        // Check X unit is wavelength
        Mantid::Kernel::Unit_sptr xUnit = factorWs->getAxis(0)->unit();
        if (xUnit->caption() != "Wavelength") {
          QString msg = "Correction factor workspace " +
                        QString::fromStdString(factorWs->name()) +
                        " is not in wavelength";
          uiv.addErrorMessage(msg);
        }
      }
    }
  }

  // Show errors if there are any
  if (!uiv.isAllInputValid())
    emit showMessageBox(uiv.generateErrorMessage());

  return uiv.isAllInputValid();
}

void ApplyPaalmanPings::loadSettings(const QSettings &settings) {
  m_uiForm.dsCorrections->readSettings(settings.group());
  m_uiForm.dsContainer->readSettings(settings.group());
  m_uiForm.dsSample->readSettings(settings.group());
}

/**
 * Handles when the type of geometry changes
 *
 * Updates the file extension to search for
 */
void ApplyPaalmanPings::handleGeometryChange(int index) {
  QString ext("");
  switch (index) {
  case 0:
    // Geometry is flat
    ext = "_flt_abs";
    break;
  case 1:
    // Geometry is cylinder
    ext = "_cyl_abs";
    break;
  case 2:
    // Geometry is annulus
    ext = "_ann_abs";
    break;
  }
  m_uiForm.dsCorrections->setWSSuffixes(QStringList(ext));
  m_uiForm.dsCorrections->setFBSuffixes(QStringList(ext + ".nxs"));
}

/**
 * Replots the preview plot.
 *
 * @param wsIndex Spectrum index to plot
 */
void ApplyPaalmanPings::plotPreview(int wsIndex) {
  bool useCan = m_uiForm.ckUseCan->isChecked();

  m_uiForm.ppPreview->clear();

  // Plot sample
  m_uiForm.ppPreview->addSpectrum(
      "Sample", m_uiForm.dsSample->getCurrentDataName(), wsIndex, Qt::black);

  // Plot result
  if (!m_pythonExportWsName.empty())
    m_uiForm.ppPreview->addSpectrum(
        "Corrected", QString::fromStdString(m_pythonExportWsName), wsIndex,
        Qt::green);

  // Scale can
  if (useCan) {
    if (m_uiForm.ckScaleCan->isChecked()) {
      auto canName = m_uiForm.dsContainer->getCurrentDataName();
      if (m_uiForm.ckShiftCan->isChecked()) {
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
    if (m_uiForm.ckScaleCan->isChecked()) {
      m_uiForm.ppPreview->addSpectrum("Container", "__container_corrected",
                                      wsIndex, Qt::red);
    } else {
      if (m_uiForm.ckShiftCan->isChecked()) {
        m_uiForm.ppPreview->addSpectrum(
            "Container",
            (m_uiForm.dsContainer->getCurrentDataName() + "_Shifted"),
            wsIndex, Qt::red);
      } else {
        m_uiForm.ppPreview->addSpectrum(
            "Container", m_uiForm.dsContainer->getCurrentDataName(), wsIndex,
            Qt::red);
      }
    }
  }
}

} // namespace CustomInterfaces
} // namespace MantidQt